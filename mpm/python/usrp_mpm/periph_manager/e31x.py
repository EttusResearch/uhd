#
# Copyright 2018-2019 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E310 implementation module
"""

import copy
import re
from six import itervalues
from usrp_mpm.components import ZynqComponents
from usrp_mpm.dboard_manager import E31x_db
from usrp_mpm.gpsd_iface import GPSDIfaceExtension
from usrp_mpm.mpmutils import assert_compat_number, str2bool
from usrp_mpm.periph_manager import PeriphManagerBase
from usrp_mpm.rpc_server import no_rpc
from usrp_mpm.sys_utils import dtoverlay
from usrp_mpm.sys_utils.sysfs_thermal import read_sysfs_sensors_value
from usrp_mpm.sys_utils.udev import get_spidev_nodes
from usrp_mpm.xports import XportMgrUDP
from usrp_mpm.periph_manager.e31x_periphs import MboardRegsControl
from usrp_mpm.sys_utils.udev import get_eeprom_paths
from usrp_mpm import e31x_legacy_eeprom

E310_DEFAULT_CLOCK_SOURCE = 'internal'
E310_DEFAULT_TIME_SOURCE = 'internal'
E310_DEFAULT_ENABLE_FPGPIO = True
E310_DEFAULT_DONT_RELOAD_FPGA = False # False means idle image gets reloaded
E310_FPGA_COMPAT = (6, 0)
E310_DBOARD_SLOT_IDX = 0
E310_GPIO_SRC_PS = "PS"
# We use the index positions of RFA and RFB to map between name and radio index
E310_GPIO_SRCS = ("RFA", "RFB", E310_GPIO_SRC_PS)
E310_FPGPIO_WIDTH = 6
E310_GPIO_BANKS = ["INT0",]

###############################################################################
# Transport managers
###############################################################################
# pylint: disable=too-few-public-methods

class E310XportMgrUDP(XportMgrUDP):
    "E310-specific UDP configuration"
    iface_config = {
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
class e31x(ZynqComponents, PeriphManagerBase):
    """
    Holds E310 specific attributes and methods
    """
    #########################################################################
    # Overridables
    #
    # See PeriphManagerBase for documentation on these fields
    #########################################################################
    description = "E300-Series Device"
    # 0x77d2 and 0x77d3
    pids = {0x77D2: 'e310_sg1', #sg1
            0x77D3: 'e310_sg3'} #sg3
    # The E310 has a single EEPROM that stores both DB and MB information
    mboard_eeprom_addr = "e0004000.i2c"
    mboard_eeprom_offset = 0
    mboard_eeprom_max_len = 64
    # We have two nvem paths on the E310.
    # This one ensures that we get the right path for the MB.
    mboard_eeprom_path_index = 1
    mboard_info = {"type": "e3xx"}
    mboard_sensor_callback_map = {
        'ref_locked': 'get_ref_lock_sensor',
        'gps_locked': 'get_gps_lock_sensor',
        'temp_fpga' : 'get_fpga_temp_sensor',
        'temp_mb' : 'get_mb_temp_sensor',
    }
    # The E310 has a single EEPROM that stores both DB and MB information
    dboard_eeprom_addr = "e0004000.i2c"
    dboard_eeprom_path_index = 0
    # Actual DB EEPROM bytes are just 28. Reading just a couple more.
    # Refer e300_eeprom_manager.hpp
    dboard_eeprom_max_len = 32
    max_num_dboards = 1
    # We're on a Zynq target, so the following two come from the Zynq standard
    # device tree overlay (tree/arch/arm/boot/dts/zynq-7000.dtsi)
    dboard_spimaster_addrs = ["e0006000.spi"]
    # E310-specific settings
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
    # This class removes the overlay in tear_down() resulting
    # in stale references to methods in the RPC server. Setting
    # this to True ensures that the RPC server clears all registered
    # methods on unclaim() and registers them on the following claim().
    clear_rpc_registry_on_unclaim = True

    @classmethod
    def generate_device_info(cls, eeprom_md, mboard_info, dboard_infos):
        """
        Generate dictionary describing the device.
        """
        # Add the default PeriphManagerBase information first
        device_info = super().generate_device_info(
            eeprom_md, mboard_info, dboard_infos)
        # Then add E31x-specific information
        mb_pid = eeprom_md.get('pid')
        device_info['product'] = cls.pids.get(mb_pid, 'unknown')
        return device_info

    @staticmethod
    def list_required_dt_overlays(device_info):
        """
        Returns the name of the overlay for the regular image (not idle).
        Either returns e310_sg1 or e310_sg3.
        """
        return [device_info['product']]
    ### End of overridables ###################################################

    @staticmethod
    def get_idle_dt_overlay(device_info):
        """
        Overlay to be applied to enter low power idle state.
        """
        # e.g. e310_sg3_idle
        idle_overlay = device_info['product'] + '_idle'
        return idle_overlay

    ###########################################################################
    # Ctor and device initialization tasks
    ###########################################################################
    def __init__(self, args):
        """
        Does partial initialization which loads low power idle image
        """
        self._time_source = None
        self._gpsd = None
        self.dboards = []
        self.dboard = None
        self.mboard_regs_control = None
        self._xport_mgrs = {}
        self._initialization_status = ""
        self._device_initialized = False
        self.args_cached = args
        # This will load the regular image to obtain all FPGA info
        super(e31x, self).__init__()
        args = self._update_default_args(args)
        # Permanently store the value from mpm.conf:
        self._do_not_reload_default = \
            str2bool(args.get("no_reload_fpga", E310_DEFAULT_DONT_RELOAD_FPGA))
        # This flag can change depending on UHD args:
        self._do_not_reload = self._do_not_reload_default
        # If we don't want to reload, we'll complete initialization now:
        if self._do_not_reload:
            try:
                self.log.info("Not reloading FPGA images!")
                self._init_normal()
            except BaseException as ex:
                self.log.error("Failed to initialize motherboard: %s", str(ex))
                self._initialization_status = str(ex)
                self._device_initialized = False
        else: # Otherwise, put the USRP into low-power mode:
            # Start clean by removing MPM-owned overlays.
            active_overlays = self.list_active_overlays()
            mpm_overlays = self.list_owned_overlays()
            for overlay in active_overlays:
                if overlay in mpm_overlays:
                    dtoverlay.rm_overlay(overlay)
            # Apply idle overlay on boot to save power until
            # an application tries to use the device.
            self.apply_idle_overlay()
            self._device_initialized = False
        self._init_gps_sensors()

    def _init_gps_sensors(self):
        """
        Init and register the GPSd Iface and related sensor functions

        Note: The GPS chip is not connected to the FPGA, so this is initialized
        regardless of the idle state
        """
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


    def _init_normal(self):
        """
        Does full initialization. This gets called during claim(), because the
        E310 usually gets freshly initialized on every UHD session for power
        usage reasons, unless no_reload_fpga was provided in mpm.conf.
        """
        if self._device_initialized:
            return
        if self.is_idle():
            self.remove_idle_overlay()
        self.overlay_apply()
        self.init_dboards(self.args_cached)
        if not self._device_initialized:
            # Don't try and figure out what's going on. Just give up.
            return
        self._time_source = None
        self.dboard = self.dboards[E310_DBOARD_SLOT_IDX]
        try:
            self._init_peripherals(self.args_cached)
        except BaseException as ex:
            self.log.error("Failed to initialize motherboard: %s", str(ex))
            self._initialization_status = str(ex)
            self._device_initialized = False

    def _init_dboards(self, dboard_infos, override_dboard_pids, default_args):
        """
        Initialize all the daughterboards

        dboard_infos -- List of dictionaries as returned from
                       PeriphManagerBase._get_dboard_eeprom_info()
        override_dboard_pids -- List of dboard PIDs to force
        default_args -- Default args
        """
        # Overriding DB PIDs doesn't work here, the DB is coupled to the MB
        if override_dboard_pids:
            raise NotImplementedError("Can't override dboard pids")
        # We have only one dboard
        dboard_info = dboard_infos[0]
        # Set up the SPI nodes
        assert len(self.dboard_spimaster_addrs) == 1
        spi_nodes = get_spidev_nodes(self.dboard_spimaster_addrs[0])
        assert spi_nodes
        self.log.trace("Found spidev nodes: {0}".format(str(spi_nodes)))
        dboard_info.update({
            'spi_nodes': spi_nodes,
            'default_args': default_args,
        })
        self.dboards.append(E31x_db(E310_DBOARD_SLOT_IDX, **dboard_info))
        assert len(self.dboards) == 1

    def _check_fpga_compat(self):
        " Throw an exception if the compat numbers don't match up "
        actual_compat = self.mboard_regs_control.get_compat_number()
        self.log.debug("Actual FPGA compat number: {:d}.{:d}".format(
            actual_compat[0], actual_compat[1]
        ))
        assert_compat_number(
            E310_FPGA_COMPAT,
            self.mboard_regs_control.get_compat_number(),
            component="FPGA",
            fail_on_old_minor=True,
            log=self.log
        )

    def _init_ref_clock_and_time(self, default_args):
        """
        Initialize clock and time sources. After this function returns, the
        reference signals going to the FPGA are valid.
        """
        self.set_clock_source(
            default_args.get('clock_source', E310_DEFAULT_CLOCK_SOURCE)
        )
        self.set_time_source(
            default_args.get('time_source', E310_DEFAULT_TIME_SOURCE)
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
        # Init clocking
        self._init_ref_clock_and_time(args)
        # Init CHDR transports
        self._xport_mgrs = {
            'udp': E310XportMgrUDP(self.log, args)
        }
        # Init complete.
        self.log.debug("mboard info: {}".format(self.mboard_info))

    def _read_mboard_eeprom(self):
        """
        Read out mboard EEPROM.
        Returns a tuple: (eeprom_dict, eeprom_rawdata), where the the former is
        a de-serialized dictionary representation of the data, and the latter
        is a binary string with the raw data.

        If no EEPROM is defined, returns empty values.
        """
        (self._eeprom_head, self.eeprom_rawdata) = {}, b''
        eeprom_path = \
            get_eeprom_paths(self.mboard_eeprom_addr)[self.mboard_eeprom_path_index]
        if not eeprom_path:
            self.log.error("Could not identify EEPROM path for %s!",
                           self.mboard_eeprom_addr)
            return
        self.log.trace("MB EEPROM: Using path {}".format(eeprom_path))
        (self._eeprom_head, self.eeprom_rawdata) = e31x_legacy_eeprom.read_eeprom(
            True, # is_motherboard
            eeprom_path,
            self.mboard_eeprom_offset,
            e31x_legacy_eeprom.MboardEEPROM.eeprom_header_format,
            e31x_legacy_eeprom.MboardEEPROM.eeprom_header_keys,
            self.mboard_eeprom_max_len
        )
        self.log.trace("Read %d bytes of EEPROM data.", len(self.eeprom_rawdata))

    def _read_dboard_eeprom_data(self, dboard_eeprom_path):
        return e31x_legacy_eeprom.read_eeprom(
            False, # is not motherboard.
            dboard_eeprom_path,
            self.dboard_eeprom_offset,
            e31x_legacy_eeprom.DboardEEPROM.eeprom_header_format,
            e31x_legacy_eeprom.DboardEEPROM.eeprom_header_keys,
            self.dboard_eeprom_max_len
        )

    ###########################################################################
    # Session init and deinit
    ###########################################################################
    def claim(self):
        """
        Fully initializes a device when the rpc_server claim()
        gets called to revive the device from idle state to be used
        by an UHD application
        """
        super(e31x, self).claim()
        try:
            self._init_normal()
        except BaseException as ex:
            self.log.error("e31x claim() failed: %s", str(ex))

    def init(self, args):
        """
        Calls init() on the parent class, and updates time/clock source.
        """
        if not self._device_initialized:
            self.log.warning(
                "Cannot run init(), device was never fully initialized!")
            return False
        args = self._update_default_args(args)
        self.set_clock_source(args.get("clock_source", E310_DEFAULT_CLOCK_SOURCE))
        self.set_time_source(args.get("time_source", E310_DEFAULT_TIME_SOURCE))
        if "no_reload_fpga" in args:
            self._do_not_reload = \
                str2bool(args.get("no_reload_fpga")) or args.get("no_reload_fpga") == ""
        result = super(e31x, self).init(args)
        return result

    def apply_idle_overlay(self):
        """
        Load all overlays required to go into idle power savings mode.
        """
        idle_overlay = self.get_idle_dt_overlay(self.device_info)
        self.log.debug(
            "Motherboard requests device tree overlay for Idle power savings mode: {}"
            .format(idle_overlay))
        dtoverlay.apply_overlay_safe(idle_overlay)

    def remove_idle_overlay(self):
        """
        Remove idle mode overlay.
        """
        idle_overlay = self.get_idle_dt_overlay(self.device_info)
        self.log.trace("Removing Idle overlay: {}".format(
            idle_overlay
        ))
        dtoverlay.rm_overlay(idle_overlay)

    def list_owned_overlays(self):
        """
        Lists all overlays that can be possibly applied by MPM.
        """
        all_overlays = self.list_required_dt_overlays(self.device_info)
        all_overlays.append(self.get_idle_dt_overlay(self.device_info))
        return all_overlays

    def deinit(self):
        """
        Clean up after a UHD session terminates.
        """
        if not self._device_initialized:
            self.log.warning(
                "Cannot run deinit(), device was never fully initialized!")
            return
        super(e31x, self).deinit()
        if not self._do_not_reload:
            self.tear_down()
        # Reset back to value from _default_args (mpm.conf)
        self._do_not_reload = self._do_not_reload_default

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        For E310, this means the overlay.
        """
        self.log.trace("Tearing down E310 device...")
        self.dboards = []
        self.dboard.tear_down()
        self.dboard = None
        self.mboard_regs_control = None
        self._device_initialized = False
        active_overlays = self.list_active_overlays()
        self.log.trace("E310 has active device tree overlays: {}".format(
            active_overlays
        ))
        for overlay in active_overlays:
            dtoverlay.rm_overlay(overlay)
        self.apply_idle_overlay()
        self.log.debug("Teardown complete!")

    def is_idle(self):
        """
        Determine if the device is in the idle state.
        """
        active_overlays = self.list_active_overlays()
        idle_overlay = self.get_idle_dt_overlay(self.device_info)
        is_idle = idle_overlay in active_overlays
        if is_idle:
            self.log.trace("Found idle overlay: %s", idle_overlay)
        return is_idle

    ###########################################################################
    # Device info
    ###########################################################################
    def get_device_info_dyn(self):
        """
        Append the device info with current IP addresses.
        """
        if not self._device_initialized:
            return {}
        device_info = {}
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
        return (E310_DEFAULT_CLOCK_SOURCE,)

    def get_clock_source(self):
        " Returns the currently selected clock source "
        return E310_DEFAULT_CLOCK_SOURCE

    def set_clock_source(self, *args):
        """
        Note: E310 only supports one clock source ('internal'), so no need to do
        an awful lot here.
        """
        clock_source = args[0]
        assert clock_source in self.get_clock_sources(), \
            "Cannot set to invalid clock source: {}".format(clock_source)

    def get_time_sources(self):
        " Returns list of valid time sources "
        return ['internal', 'external', 'gpsdo']

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source

    def set_time_source(self, time_source):
        " Set a time source "
        assert time_source in self.get_time_sources(), \
            "Cannot set to invalid time source: {}".format(time_source)
        if time_source == self.get_time_source():
            self.log.trace("Nothing to do -- time source already set.")
            return
        self._time_source = time_source
        self.mboard_regs_control.set_time_source(time_source)

    def get_sync_sources(self):
        """
        List sync sources.
        """
        return [{
            "time_source": time_source,
            "clock_source": E310_DEFAULT_CLOCK_SOURCE
        } for time_source in self.get_time_sources()]

    ###########################################################################
    # GPIO API
    ###########################################################################
    def get_gpio_banks(self):
        """
        Returns a list of GPIO banks over which MPM has any control
        """
        return E310_GPIO_BANKS

    def get_gpio_srcs(self, bank):
        """
        Return a list of valid GPIO sources for a given bank
        """
        assert bank in self.get_gpio_banks(), "Invalid GPIO bank: {}".format(bank)
        return E310_GPIO_SRCS

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
                return E310_GPIO_SRC_PS
            radio_src = (gpio_radio_src_reg >> (2 * gpio_pin_index)) & 0b11
            assert radio_src in (0, 1)
            return E310_GPIO_SRCS[radio_src]
        return [get_gpio_src_i(i) for i in range(E310_FPGPIO_WIDTH)]

    def set_gpio_src(self, bank, src):
        """
        Set the GPIO source for a given bank.
        """
        assert bank in self.get_gpio_banks(), "Invalid GPIO bank: {}".format(bank)
        assert len(src) == E310_FPGPIO_WIDTH, \
            "Invalid number of GPIO sources!"
        gpio_master_reg = 0x00
        gpio_radio_src_reg = self.mboard_regs_control.get_fp_gpio_radio_src()
        for src_index, src_name in enumerate(src):
            if src_name not in self.get_gpio_srcs(bank):
                raise RuntimeError(
                    "Invalid GPIO source name `{}' at bit position {}!"
                    .format(src_name, src_index))
            gpio_master_flag = (src_name == E310_GPIO_SRC_PS)
            gpio_master_reg = gpio_master_reg | (gpio_master_flag << src_index)
            if gpio_master_flag:
                continue
            # If PS is not the master, we also need to update the radio source:
            radio_index = E310_GPIO_SRCS.index(src_name)
            gpio_radio_src_reg = gpio_radio_src_reg | (radio_index << (2*src_index))
        self.log.trace("Updating GPIO source: master==0x{:02X} radio_src={:03X}"
                       .format(gpio_master_reg, gpio_radio_src_reg))
        self.mboard_regs_control.set_fp_gpio_master(gpio_master_reg)
        self.mboard_regs_control.set_fp_gpio_radio_src(gpio_radio_src_reg)

    ###########################################################################
    # Hardware peripheral controls
    ###########################################################################
    def set_channel_mode(self, channel_mode):
        "Set channel mode in FPGA and select which tx channel to use"
        self.mboard_regs_control.set_channel_mode(channel_mode)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_ref_lock_sensor(self):
        """
        Return main refclock lock status. In the FPGA, this is the reflck output
        of the ppsloop module.
        """
        self.log.trace("Querying ref lock status.")
        lock_status = bool(self.mboard_regs_control.get_refclk_lock())
        return {
            'name': 'ref_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_gps_lock_sensor(self):
        """
        Get lock status of GPS as a sensor dict
        """
        gps_locked = self._gpsd.get_gps_lock()
        return {
            'name': 'gps_lock',
            'type': 'BOOLEAN',
            'unit': 'locked' if gps_locked else 'unlocked',
            'value': str(gps_locked).lower(),
        }

    def get_mb_temp_sensor(self):
        """
        Get temperature sensor reading of the E310.
        """
        self.log.trace("Reading temperature.")
        temp = '-1'
        raw_val = {}
        data_probes = ['temp1_input']
        try:
            for data_probe in data_probes:
                raw_val[data_probe] = read_sysfs_sensors_value(
                    'jc-42.4-temp', data_probe, 'hwmon', 'name')[0]
            temp = str(raw_val['temp1_input'] / 1000)
        except ValueError:
            self.log.warning("Error when converting temperature value")
        except KeyError:
            self.log.warning("Can't read MB temperature!")
        return {
            'name': 'temp_mb',
            'type': 'REALNUM',
            'unit': 'C',
            'value': temp
        }

    def get_fpga_temp_sensor(self):
        """
        Get temperature sensor reading of the E310.
        """
        self.log.trace("Reading temperature.")
        temp = '-1'
        raw_val = {}
        data_probes = ['in_temp0_raw', 'in_temp0_scale', 'in_temp0_offset']
        try:
            for data_probe in data_probes:
                raw_val[data_probe] = read_sysfs_sensors_value(
                    'xadc', data_probe, 'iio', 'name')[0]
            temp = str((raw_val['in_temp0_raw'] + raw_val['in_temp0_offset']) \
                    * raw_val['in_temp0_scale'] / 1000)
        except ValueError:
            self.log.warning("Error when converting temperature value")
        except KeyError:
            self.log.warning("Can't read FPGA temperature!")
        return {
            'name': 'temp_fpga',
            'type': 'REALNUM',
            'unit': 'C',
            'value': temp
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

    def set_mb_eeprom(self, _eeprom_vals):
        """
        See PeriphManagerBase.set_mb_eeprom() for docs.
        """
        self.log.warn("Called set_mb_eeprom(), but not implemented!")

    def get_db_eeprom(self, dboard_idx):
        """
        See PeriphManagerBase.get_db_eeprom() for docs.
        """
        if dboard_idx != E310_DBOARD_SLOT_IDX:
            self.log.warn("Trying to access invalid dboard index {}. "
                          "Using the only dboard.".format(dboard_idx))
        db_eeprom_data = copy.copy(self.dboard.device_info)
        return db_eeprom_data

    def set_db_eeprom(self, _dboard_idx, _eeprom_data):
        """
        See PeriphManagerBase.set_db_eeprom() for docs.
        """
        self.log.warn("Called set_db_eeprom(), but not implemented!")

    ###########################################################################
    # Component updating
    ###########################################################################
    # Note: Component updating functions defined by ZynqComponents
    @no_rpc
    def _update_fpga_type(self):
        """Update the fpga type stored in the updateable components"""
        fpga_type = "" # FIXME
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
                'freq': str(100e6),
            }
        ]
