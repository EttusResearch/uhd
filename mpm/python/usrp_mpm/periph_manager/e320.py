#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E320 implementation module
"""

import bisect
import copy
import re
from functools import partial
from six import iteritems
from usrp_mpm.compat_num import CompatNumber
from usrp_mpm.components import ZynqComponents
from usrp_mpm.dboard_manager import Neon
from usrp_mpm.gpsd_iface import GPSDIfaceExtension
from usrp_mpm.mpmutils import assert_compat_number, str2bool
from usrp_mpm.periph_manager import PeriphManagerBase
from usrp_mpm.rpc_server import no_rpc
from usrp_mpm.sys_utils import dtoverlay
from usrp_mpm.sys_utils.sysfs_thermal import read_thermal_sensor_value, read_thermal_sensors_value
from usrp_mpm.sys_utils.udev import get_spidev_nodes
from usrp_mpm.xports import XportMgrUDP
from usrp_mpm.periph_manager.e320_periphs import MboardRegsControl

E320_DEFAULT_INT_CLOCK_FREQ = 20e6
E320_DEFAULT_EXT_CLOCK_FREQ = 10e6
E320_DEFAULT_CLOCK_SOURCE = 'internal'
E320_DEFAULT_TIME_SOURCE = 'internal'
E320_DEFAULT_ENABLE_GPS = True
E320_DEFAULT_ENABLE_FPGPIO = True
E320_FPGA_COMPAT = (6, 1)
E320_REMOTE_STREAMING_COMPAT = (6, 1)
E320_DBOARD_SLOT_IDX = 0
E320_GPIO_BANKS = ["FP0",]
E320_GPIO_SRC_PS = "PS"
# We use the index positions of RFA and RFB to map between name and radio index
E320_GPIO_SRCS = ("RFA", "RFB", E320_GPIO_SRC_PS)
E320_FPGPIO_WIDTH = 8

###############################################################################
# Transport managers
###############################################################################
# pylint: disable=too-few-public-methods
class E320XportMgrUDP(XportMgrUDP):
    "E320-specific UDP configuration"
    iface_config = {
        'sfp0': {
            'label': 'misc-enet-regs',
            'type': 'sfp',
        },
        'int0': {
            'label': 'misc-enet-int-regs',
            'type': 'internal',
        },
        'eth0': {
            'label': '',
            'type': 'forward',
        }
    }


# pylint: enable=too-few-public-methods

###############################################################################
# Main Class
###############################################################################
# We need to disable the no-self-use check, because we might require self to
# become an RPC method, but PyLint doesnt' know that.
# pylint: disable=no-self-use
class e320(ZynqComponents, PeriphManagerBase):
    """
    Holds E320 specific attributes and methods
    """
    #########################################################################
    # Overridables
    #
    # See PeriphManagerBase for documentation on these fields
    #########################################################################
    description = "E300-Series Device"
    pids = {0xe320: 'e320'}
    mboard_eeprom_addr = "e0004000.i2c"
    mboard_eeprom_offset = 0
    mboard_eeprom_max_len = 256
    mboard_info = {"type": "e3xx",
                   "product": "e320"
                  }
    # This is the latest HW revision that his version of MPM is aware of. This
    # version of MPM will be able to run with any hardware which has a rev_compat
    # field that is equal or less than this value.
    # Note: If the hardware is revved in a non-compatible way, eeprom-init.c
    # must also be updated (derive_rev_compat).
    mboard_max_rev = 4  # rev E
    mboard_sensor_callback_map = {
        'ref_locked': 'get_ref_lock_sensor',
        'gps_locked': 'get_gps_lock_sensor',
        'fan': 'get_fan_sensor',
        'temp_fpga' : 'get_fpga_temp_sensor',
        'temp_internal' : 'get_internal_temp_sensor',
        'temp_rf_channelA' : 'get_rf_channelA_temp_sensor',
        'temp_rf_channelB' : 'get_rf_channelB_temp_sensor',
        'temp_main_power' : 'get_main_power_temp_sensor',
    }
    max_num_dboards = 1

    # We're on a Zynq target, so the following two come from the Zynq standard
    # device tree overlay (tree/arch/arm/boot/dts/zynq-7000.dtsi)
    dboard_spimaster_addrs = ["e0006000.spi", "e0007000.spi"]
    # E320-specific settings
    # Label for the mboard UIO
    mboard_regs_label = "mboard-regs"
    # Override the list of updateable components
    updateable_components = {
        'fpga': {
            'callback': "update_fpga",
            'path': '/lib/firmware/{}.bin',
            'reset': True,
        },
        'dts': {
            'callback': "update_dts",
            'path': '/lib/firmware/{}.dts',
            'output': '/lib/firmware/{}.dtbo',
            'reset': False,
        },
    }

    @staticmethod
    def list_required_dt_overlays(device_info):
        """
        Lists device tree overlays that need to be applied before this class can
        be used. List of strings.
        Are applied in order.

        eeprom_md -- Dictionary of info read out from the mboard EEPROM
        device_args -- Arbitrary dictionary of info, typically user-defined
        """
        return [device_info['product']]

    ###########################################################################
    # Ctor and device initialization tasks
    ###########################################################################
    def __init__(self, args):
        super(e320, self).__init__()
        self.overlay_apply()
        self.init_dboards(args)
        if not self._device_initialized:
            # Don't try and figure out what's going on. Just give up.
            return
        self._tear_down = False
        self._ext_clock_freq = E320_DEFAULT_EXT_CLOCK_FREQ
        self._clock_source = None
        self._time_source = None
        self._gpsd = None
        self._xport_adapter_mgrs = {}
        self.dboard = self.dboards[E320_DBOARD_SLOT_IDX]
        for sensor_name, sensor_cb_name in self.mboard_sensor_callback_map.items():
            if sensor_name[:5] == 'temp_':
                setattr(self, sensor_cb_name, partial(self.get_temp_sensor, sensor_name))
        try:
            self._init_peripherals(args)
        except BaseException as ex:
            self.log.error("Failed to initialize motherboard: %s", str(ex))
            self._initialization_status = str(ex)
            self._device_initialized = False

    def _init_dboards(self, _, override_dboard_pids, default_args):
        """
        Initialize all the daughterboards.

        Note: This gets called by PeriphManagerBase.init_dboards(). We override
        the base class's implementation in order to avoid initializing our one
        "dboard" in the same way that, for example, N310's dboards are initialized.
        Specifically,
        - skip dboard EEPROM setup (we don't have one)
        - change the way we handle SPI devices

        (dboard_infos) -- N/A
        override_dboard_pids -- List of dboard PIDs to force
        default_args -- Default args
        """
        if override_dboard_pids:
            self.log.warning("Overriding daughterboard PIDs with: {}"
                             .format(override_dboard_pids))
            raise NotImplementedError("Can't override dboard pids")
        # The DBoard PID is the same as the MBoard PID
        db_pid = list(self.pids.keys())[0]
        # Set up the SPI nodes
        spi_nodes = []
        for spi_addr in self.dboard_spimaster_addrs:
            for spi_node in get_spidev_nodes(spi_addr):
                bisect.insort(spi_nodes, spi_node)
        self.log.trace("Found spidev nodes: {0}".format(spi_nodes))

        if not spi_nodes:
            self.log.warning("No SPI nodes for dboard %d.", E320_DBOARD_SLOT_IDX)
        dboard_info = {
            'eeprom_md': self.mboard_info,
            'eeprom_rawdata': self._eeprom_rawdata,
            'pid': db_pid,
            'spi_nodes': spi_nodes,
            'default_args': default_args,
        }
        # This will actually instantiate the dboard class:
        self.dboards.append(Neon(E320_DBOARD_SLOT_IDX, **dboard_info))
        assert len(self.dboards) == 1

    def _check_fpga_compat(self):
        " Throw an exception if the compat numbers don't match up "
        actual_compat = self.mboard_regs_control.get_compat_number()
        self.log.debug("Actual FPGA compat number: {:d}.{:d}".format(
            actual_compat[0], actual_compat[1]
        ))
        assert_compat_number(
            E320_FPGA_COMPAT,
            self.mboard_regs_control.get_compat_number(),
            component="FPGA",
            fail_on_old_minor=True,
            log=self.log
        )
        if CompatNumber(actual_compat) >= CompatNumber(E320_REMOTE_STREAMING_COMPAT):
            self.fpga_features.add('remote_udp_streaming')
        self.log.debug(
            "FPGA supports the following features: {}"
            .format(", ".join(self.fpga_features)))

    def _init_ref_clock_and_time(self, default_args):
        """
        Initialize clock and time sources. After this function returns, the
        reference signals going to the FPGA are valid.
        """
        self._ext_clock_freq = float(
            default_args.get('ext_clock_freq', E320_DEFAULT_EXT_CLOCK_FREQ)
        )
        self.set_clock_source(
            default_args.get('clock_source', E320_DEFAULT_CLOCK_SOURCE)
        )
        self.set_time_source(
            default_args.get('time_source', E320_DEFAULT_TIME_SOURCE)
        )

    def _init_peripherals(self, args):
        """
        Turn on all peripherals. This may throw an error on failure, so make
        sure to catch it.

        Peripherals are initialized in the order of least likely to fail, to most
        likely.
        """
        # Sanity checks
        assert self.mboard_info.get('product') in self.pids.values(), \
            "Device product could not be determined!"
        # Init Mboard Regs
        self.mboard_regs_control = MboardRegsControl(
            self.mboard_regs_label, self.log)
        self.mboard_regs_control.get_git_hash()
        self.mboard_regs_control.get_build_timestamp()
        self._check_fpga_compat()
        self._update_fpga_type()
        # Init peripherals
        self.enable_gps(
            enable=str2bool(
                args.get('enable_gps', E320_DEFAULT_ENABLE_GPS)
            )
        )
        self.enable_fp_gpio(
            enable=args.get('enable_fp_gpio', E320_DEFAULT_ENABLE_FPGPIO)
        )
        # Init clocking
        self._init_ref_clock_and_time(args)
        # Init GPSd iface and GPS sensors
        self._init_gps_sensors()
        # Init CHDR transports
        self._xport_mgrs = {
            'udp': E320XportMgrUDP(self.log, args)
        }
        # Init complete.
        self.log.debug("mboard info: {}".format(self.mboard_info))

    def _init_gps_sensors(self):
        "Init and register the GPSd Iface and related sensor functions"
        self.log.trace("Initializing GPSd interface")
        self._gpsd = GPSDIfaceExtension()
        new_methods = self._gpsd.extend(self)
        for method_name in new_methods:
            try:
                # Extract the sensor name from the getter
                sensor_name = re.search(r"get_(.*)_sensor", method_name).group(1)
                # Register it with the MB sensor framework
                self.mboard_sensor_callback_map[sensor_name] = method_name
                self.log.trace("Adding %s sensor function", sensor_name)
            except AttributeError:
                # re.search will return None is if can't find the sensor name
                self.log.warning("Error while registering sensor function: %s", method_name)

    ###########################################################################
    # Session init and deinit
    ###########################################################################
    def init(self, args):
        """
        Calls init() on the parent class, and then programs the Ethernet
        dispatchers accordingly.
        """
        if not self._device_initialized:
            self.log.warning(
                "Cannot run init(), device was never fully initialized!")
            return False
        args = self._update_default_args(args)
        self.set_clock_source(args.get("clock_source", E320_DEFAULT_CLOCK_SOURCE))
        self.set_time_source(args.get("time_source", E320_DEFAULT_TIME_SOURCE))
        result = super(e320, self).init(args)
        return result

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        For E320, this means the overlay.
        """
        self.log.trace("Tearing down E320 device...")
        self._tear_down = True
        active_overlays = self.list_active_overlays()
        self.log.trace("E320 has active device tree overlays: {}".format(
            active_overlays
        ))
        for overlay in active_overlays:
            dtoverlay.rm_overlay(overlay)


    ###########################################################################
    # Device info
    ###########################################################################
    def get_device_info_dyn(self):
        """
        Append the device info with current IP addresses.
        """
        if not self._device_initialized:
            return {}
        device_info = self._xport_mgrs['udp'].get_xport_info()
        device_info.update({
            'fpga_version': "{}.{}".format(
                *self.mboard_regs_control.get_compat_number()),
            'fpga_version_hash': "{:x}.{}".format(
                *self.mboard_regs_control.get_git_hash()),
            'fpga': self.updateable_components.get('fpga', {}).get('type', ""),
        })
        return device_info

    ###########################################################################
    # Clock/Time API
    ###########################################################################
    def get_clock_sources(self):
        " Lists all available clock sources. "
        self.log.trace("Listing available clock sources...")
        return ('external', 'internal', 'gpsdo')

    def get_clock_source(self):
        " Returns the currently selected clock source "
        return self._clock_source

    def set_clock_source(self, *args):
        """
        Switch reference clock.

        Throws if clock_source is not a valid value.
        """
        clock_source = args[0]
        assert clock_source in self.get_clock_sources()
        self.log.debug("Setting clock source to `{}'".format(clock_source))
        if clock_source == self.get_clock_source():
            self.log.trace("Nothing to do -- clock source already set.")
            return
        self._clock_source = clock_source
        ref_clk_freq = self.get_ref_clock_freq()
        self.mboard_regs_control.set_clock_source(clock_source, ref_clk_freq)
        self.log.debug("Reference clock frequency is: {} MHz".format(
            ref_clk_freq/1e6
        ))
        self.dboard.update_ref_clock_freq(ref_clk_freq)

    def set_ref_clock_freq(self, freq):
        """
        Tell our USRP what the frequency of the external reference clock is.

        Will throw if it's not a valid value.
        """
        # Other frequencies have not been tested
        assert freq in (10e6, 20e6)
        self.log.debug("We've been told the external reference clock " \
                       "frequency is {} MHz.".format(freq / 1e6))
        if self._ext_clock_freq == freq:
            self.log.trace("New external reference clock frequency " \
                           "assignment matches previous assignment. Ignoring " \
                           "update command.")
            return
        self._ext_clock_freq = freq
        if self.get_clock_source() == 'external':
            self.log.trace(f"Updating reference clock to {freq/1e6} MHz...")
            self.dboard.update_ref_clock_freq(freq)

    def get_ref_clock_freq(self):
        " Returns the currently active reference clock frequency"
        clock_source = self.get_clock_source()
        if clock_source in ("internal", "gpsdo"):
            return E320_DEFAULT_INT_CLOCK_FREQ
        # elif clock_source == "external":
        return self._ext_clock_freq

    def get_time_sources(self):
        " Returns list of valid time sources "
        return ['internal', 'external', 'gpsdo']

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source

    def set_time_source(self, time_source):
        " Set a time source "
        assert time_source in self.get_time_sources()
        if time_source == self.get_time_source():
            self.log.trace("Nothing to do -- time source already set.")
            return
        self._time_source = time_source
        self.mboard_regs_control.set_time_source(time_source, self.get_ref_clock_freq())

    def get_sync_sources(self):
        """
        List sync sources.
        """
        valid_sync_sources = {
            # clock, time. Reminder: 'internal' is an alias for 'gpsdo'
            # pylint: disable=bad-whitespace
            ('internal', 'internal'),
            ('external', 'internal'),
            ('external', 'external'),
            ('gpsdo',    'gpsdo'   ),
            ('gpsdo',    'internal'),
            # pylint: enable=bad-whitespace
        }
        return [{
            "time_source": time_source,
            "clock_source": clock_source
        } for (clock_source, time_source) in valid_sync_sources]

    ###########################################################################
    # GPIO API
    ###########################################################################
    def get_gpio_banks(self):
        """
        Returns a list of GPIO banks over which MPM has any control
        """
        return E320_GPIO_BANKS

    def get_gpio_srcs(self, bank):
        """
        Return a list of valid GPIO sources for a given bank
        """
        assert bank in self.get_gpio_banks(), "Invalid GPIO bank: {}".format(bank)
        return E320_GPIO_SRCS

    def get_gpio_src(self, bank):
        """
        Return the currently selected GPIO source for a given bank. The return
        value is a list of strings. The length of the vector is identical to
        the number of controllable GPIO pins on this bank.
        """
        assert bank in self.get_gpio_banks(), "Invalid GPIO bank: {}".format(bank)
        gpio_master_reg = self.mboard_regs_control.get_fp_gpio_master()
        gpio_radio_src_reg = self.mboard_regs_control.get_fp_gpio_radio_src()
        def get_gpio_src_i(gpio_pin_index):
            """
            Return the current radio source given a pin index.
            """
            if gpio_master_reg & (1 << gpio_pin_index):
                return E320_GPIO_SRC_PS
            radio_src = (gpio_radio_src_reg >> (2 * gpio_pin_index)) & 0b11
            assert radio_src in (0, 1)
            return E320_GPIO_SRCS[radio_src]
        return [get_gpio_src_i(i) for i in range(E320_FPGPIO_WIDTH)]

    def set_gpio_src(self, bank, src):
        """
        Set the GPIO source for a given bank.
        """
        assert bank in self.get_gpio_banks(), "Invalid GPIO bank: {}".format(bank)
        assert len(src) == E320_FPGPIO_WIDTH, \
            "Invalid number of GPIO sources!"
        gpio_master_reg = 0x00
        gpio_radio_src_reg = self.mboard_regs_control.get_fp_gpio_radio_src()
        for src_index, src_name in enumerate(src):
            if src_name not in self.get_gpio_srcs(bank):
                raise RuntimeError(
                    "Invalid GPIO source name `{}' at bit position {}!"
                    .format(src_name, src_index))
            gpio_master_flag = (src_name == E320_GPIO_SRC_PS)
            gpio_master_reg = gpio_master_reg | (gpio_master_flag << src_index)
            if gpio_master_flag:
                continue
            # If PS is not the master, we also need to update the radio source:
            radio_index = E320_GPIO_SRCS.index(src_name)
            gpio_radio_src_reg = gpio_radio_src_reg | (radio_index << (2*src_index))
        self.log.trace("Updating GPIO source: master==0x{:02X} radio_src={:04X}"
                       .format(gpio_master_reg, gpio_radio_src_reg))
        self.mboard_regs_control.set_fp_gpio_master(gpio_master_reg)
        self.mboard_regs_control.set_fp_gpio_radio_src(gpio_radio_src_reg)

    ###########################################################################
    # Hardware peripheral controls
    ###########################################################################
    def enable_gps(self, enable):
        """
        Turn power to the GPS (CLK_GPS_PWR_EN) off or on.
        """
        self.mboard_regs_control.enable_gps(enable)

    def enable_fp_gpio(self, enable):
        """
        Turn power to the front panel GPIO off or on and set voltage
        to 3.3V.
        """
        self.log.trace("{} power to front-panel GPIO".format(
            "Enabling" if enable else "Disabling"
        ))
        self.mboard_regs_control.enable_fp_gpio(enable)

    def set_fp_gpio_voltage(self, value):
        """
        Set Front Panel GPIO voltage (3.3 Volts)
        """
        self.log.trace("Setting front-panel GPIO voltage to {:3.1f} V".format(value))
        self.mboard_regs_control.set_fp_gpio_voltage(value)

    def get_fp_gpio_voltage(self):
        """
        Get Front Panel GPIO voltage (1.8, 2.5 or 3.3 Volts)
        """
        value = self.mboard_regs_control.get_fp_gpio_voltage()
        self.log.trace("Current front-panel GPIO voltage {:3.1f} V".format(value))
        return value

    def set_channel_mode(self, channel_mode):
        "Set channel mode in FPGA and select which tx channel to use"
        self.mboard_regs_control.set_channel_mode(channel_mode)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_ref_lock_sensor(self):
        """
        Get refclk lock from CLK_MUX_OUT signal from ADF4002
        """
        self.log.trace("Querying ref lock status from adf4002.")
        lock_status = self.mboard_regs_control.get_refclk_lock()
        return {
            'name': 'ref_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_temp_sensor(self, sensor_name):
        """
        Get temperature sensor reading of the E320.
        """
        temp_sensor_map = {
            "temp_internal" : 0,
            "temp_rf_channelA" : 1,
            "temp_fpga" : 2,
            "temp_rf_channelB" : 3,
            "temp_main_power" : 4
        }
        self.log.trace("Reading temperature.")
        return_val = '-1'
        sensor = temp_sensor_map[sensor_name]
        try:
            raw_val = read_thermal_sensors_value('cros-ec-thermal', 'temp')[sensor]
            return_val = str(raw_val / 1000)
        except ValueError:
            self.log.warning("Error when converting temperature value")
        except KeyError:
            self.log.warning("Can't read temp on thermal_zone {}".format(sensor))
        return {
            'name': sensor_name,
            'type': 'REALNUM',
            'unit': 'C',
            'value': return_val
        }

    def get_gps_lock_sensor(self):
        """
        Get lock status of GPS as a sensor dict
        """
        gps_locked = bool(self.mboard_regs_control.get_gps_locked_val())
        return {
            'name': 'gps_lock',
            'type': 'BOOLEAN',
            'unit': 'locked' if gps_locked else 'unlocked',
            'value': str(gps_locked).lower(),
        }

    def get_fan_sensor(self):
        """
        Return a sensor dictionary containing the RPM of the cooling device/fan0
        """
        self.log.trace("Reading cooling device.")
        return_val = '-1'
        try:
            raw_val = read_thermal_sensor_value('Fan', 'cur_state')
            return_val = str(raw_val)
        except ValueError:
            self.log.warning("Error when converting fan speed value")
        except KeyError:
            self.log.warning("Can't read cur_state on Fan")
        return {
            'name': 'cooling fan',
            'unit': 'rpm',
            'type': 'INTEGER',
            'value': return_val
        }

    ###########################################################################
    # EEPROMs
    ###########################################################################
    def get_mb_eeprom(self):
        """
        Return a dictionary with EEPROM contents.

        All key/value pairs are string -> string.

        We don't actually return the EEPROM contents, instead, we return the
        mboard info again. This filters the EEPROM contents to what we think
        the user wants to know/see.
        """
        return self.mboard_info

    def set_mb_eeprom(self, eeprom_vals):
        """
        See PeriphManagerBase.set_mb_eeprom() for docs.
        """
        self.log.warn("Called set_mb_eeprom(), but not implemented!")
        raise NotImplementedError

    def get_db_eeprom(self, dboard_idx):
        """
        See PeriphManagerBase.get_db_eeprom() for docs.
        """
        if dboard_idx != E320_DBOARD_SLOT_IDX:
            self.log.warn("Trying to access invalid dboard index {}. "
                          "Using the only dboard.".format(dboard_idx))
        db_eeprom_data = copy.copy(self.dboard.device_info)
        for blob_id, blob in iteritems(self.dboard.get_user_eeprom_data()):
            if blob_id in db_eeprom_data:
                self.log.warn("EEPROM user data contains invalid blob ID "
                              "%s", blob_id)
            else:
                db_eeprom_data[blob_id] = blob
        return db_eeprom_data

    def set_db_eeprom(self, dboard_idx, eeprom_data):
        """
        Write new EEPROM contents with eeprom_map.

        Arguments:
        dboard_idx -- Slot index of dboard (can only be E320_DBOARD_SLOT_IDX)
        eeprom_data -- Dictionary of EEPROM data to be written. It's up to the
                       specific device implementation on how to handle it.
        """
        if dboard_idx != E320_DBOARD_SLOT_IDX:
            self.log.warn("Trying to access invalid dboard index {}. "
                          "Using the only dboard.".format(dboard_idx))
        safe_db_eeprom_user_data = {}
        for blob_id, blob in iteritems(eeprom_data):
            if blob_id in self.dboard.device_info:
                error_msg = "Trying to overwrite read-only EEPROM " \
                            "entry `{}'!".format(blob_id)
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            if not isinstance(blob, str) and not isinstance(blob, bytes):
                error_msg = "Blob data for ID `{}' is not a " \
                            "string!".format(blob_id)
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            assert isinstance(blob, str)
            safe_db_eeprom_user_data[blob_id] = blob.encode('ascii')
        self.dboard.set_user_eeprom_data(safe_db_eeprom_user_data)

    ###########################################################################
    # Component updating
    ###########################################################################
    # Note: Component updating functions defined by ZynqComponents
    @no_rpc
    def _update_fpga_type(self):
        """Update the fpga type stored in the updateable components"""
        fpga_type = self.mboard_regs_control.get_fpga_type()
        self.log.debug("Updating mboard FPGA type info to {}".format(fpga_type))
        self.updateable_components['fpga']['type'] = fpga_type

    #######################################################################
    # Timekeeper API
    #######################################################################
    def get_clocks(self):
        """
        Gets the RFNoC-related clocks present in the FPGA design
        """
        return [
            {
                'name': 'radio_clk',
                'freq': str(self.dboard.get_master_clock_rate()),
                'mutable': 'true'
            },
            {
                'name': 'bus_clk',
                'freq': str(200e6),
            }
        ]
