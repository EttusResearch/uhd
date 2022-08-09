#
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X400 implementation module
"""

import threading
import copy
from time import sleep
from os import path
from collections import namedtuple
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm import tlv_eeprom
from usrp_mpm.cores import WhiteRabbitRegsControl
from usrp_mpm.components import ZynqComponents
from usrp_mpm.sys_utils import dtoverlay
from usrp_mpm.sys_utils import ectool
from usrp_mpm.sys_utils import i2c_dev
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev
from usrp_mpm.rpc_server import no_claim, no_rpc
from usrp_mpm.mpmutils import assert_compat_number, poll_with_timeout
from usrp_mpm.periph_manager import PeriphManagerBase
from usrp_mpm.xports import XportMgrUDP
from usrp_mpm.periph_manager.x4xx_periphs import MboardRegsControl
from usrp_mpm.periph_manager.x4xx_periphs import CtrlportRegs
from usrp_mpm.periph_manager.x4xx_dio_control import DioControl
from usrp_mpm.periph_manager.x4xx_periphs import QSFPModule
from usrp_mpm.periph_manager.x4xx_periphs import get_temp_sensor
from usrp_mpm.periph_manager.x4xx_mb_cpld import MboardCPLD
from usrp_mpm.periph_manager.x4xx_clk_aux import ClockingAuxBrdControl
from usrp_mpm.periph_manager.x4xx_clk_mgr import X4xxClockMgr
from usrp_mpm.periph_manager.x4xx_gps_mgr import X4xxGPSMgr
from usrp_mpm.periph_manager.x4xx_rfdc_ctrl import X4xxRfdcCtrl
from usrp_mpm.dboard_manager.x4xx_db_iface import X4xxDboardIface
from usrp_mpm.dboard_manager.zbx import ZBX


X400_DEFAULT_EXT_CLOCK_FREQ = 10e6
X400_DEFAULT_MASTER_CLOCK_RATE = 122.88e6
X400_DEFAULT_TIME_SOURCE = X4xxClockMgr.TIME_SOURCE_INTERNAL
X400_DEFAULT_CLOCK_SOURCE = X4xxClockMgr.CLOCK_SOURCE_INTERNAL
X400_DEFAULT_ENABLE_PPS_EXPORT = True
X400_FPGA_COMPAT = (7, 7)
X400_DEFAULT_TRIG_DIRECTION = ClockingAuxBrdControl.DIRECTION_OUTPUT
X400_MONITOR_THREAD_INTERVAL = 1.0 # seconds
QSFPModuleConfig = namedtuple("QSFPModuleConfig", "modprs modsel devsymbol")
X400_QSFP_I2C_CONFIGS = [
        QSFPModuleConfig(modprs='QSFP0_MODPRS', modsel='QSFP0_MODSEL_n', devsymbol='qsfp0_i2c'),
        QSFPModuleConfig(modprs='QSFP1_MODPRS', modsel='QSFP1_MODSEL_n', devsymbol='qsfp1_i2c')]
RPU_SUCCESS_REPORT = 'Success'
RPU_FAILURE_REPORT = 'Failure'
RPU_REMOTEPROC_FIRMWARE_PATH = '/lib/firmware'
RPU_REMOTEPROC_PREFIX_PATH = '/sys/class/remoteproc/remoteproc'
RPU_REMOTEPROC_PROPERTY_FIRMWARE = 'firmware'
RPU_REMOTEPROC_PROPERTY_STATE = 'state'
RPU_STATE_COMMAND_START = 'start'
RPU_STATE_COMMAND_STOP = 'stop'
RPU_STATE_OFFLINE = 'offline'
RPU_STATE_RUNNING = 'running'
RPU_MAX_FIRMWARE_SIZE = 0x100000
RPU_MAX_STATE_CHANGE_TIME_IN_MS = 10000
RPU_STATE_CHANGE_POLLING_INTERVAL_IN_MS = 100

DIOAUX_EEPROM = "dioaux_eeprom"
DIOAUX_PID = 0x4003

# pylint: disable=too-few-public-methods
class EepromTagMap:
    """
    Defines the tagmap for EEPROMs matching this magic.
    The tagmap is a dictionary mapping an 8-bit tag to a NamedStruct instance.
    The canonical list of tags and the binary layout of the associated structs
    is defined in mpm/tools/tlv_eeprom/usrp_eeprom.h. Only the subset relevant
    to MPM are included below.
    """
    magic = 0x55535250
    tagmap = {
        # 0x10: usrp_eeprom_board_info
        0x10: tlv_eeprom.NamedStruct('< H H H 7s 1x',
                                     ['pid', 'rev', 'rev_compat', 'serial']),
        # 0x11: usrp_eeprom_module_info
        0x11: tlv_eeprom.NamedStruct('< H H 7s 1x',
                                     ['module_pid', 'module_rev', 'module_serial']),
    }


###############################################################################
# Transport managers
###############################################################################
class X400XportMgrUDP(XportMgrUDP):
    "X400-specific UDP configuration"
    iface_config = {
        'sfp0': {
            'label': 'misc-enet-regs0',
            'type': 'sfp',
        },
        'sfp0_1': {
            'label': 'misc-enet-regs0-1',
            'type': 'sfp',
        },
        'sfp0_2': {
            'label': 'misc-enet-regs0-2',
            'type': 'sfp',
        },
        'sfp0_3': {
            'label': 'misc-enet-regs0-3',
            'type': 'sfp',
        },
        'sfp1': {
            'label': 'misc-enet-regs1',
            'type': 'sfp',
        },
        'sfp1_1': {
            'label': 'misc-enet-regs1-1',
            'type': 'sfp',
        },
        'sfp1_2': {
            'label': 'misc-enet-regs1-2',
            'type': 'sfp',
        },
        'sfp1_3': {
            'label': 'misc-enet-regs1-3',
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
class x4xx(ZynqComponents, PeriphManagerBase):
    """
    Holds X400 specific attributes and methods
    """
    #########################################################################
    # Overridables
    #
    # See PeriphManagerBase for documentation on these fields. We try and keep
    # them in the same order as they are in PeriphManagerBase for easier lookup.
    #########################################################################
    pids = {0x0410: 'x410'}
    description = "X400-Series Device"
    eeprom_search = PeriphManagerBase._EepromSearch.SYMBOL
    # This is not in the overridables section from PeriphManagerBase, but we use
    # it below
    eeprom_magic = EepromTagMap.magic
    mboard_eeprom_offset = 0
    mboard_eeprom_max_len = 256
    mboard_eeprom_magic = eeprom_magic
    mboard_info = {"type": "x4xx"}
    mboard_max_rev = 7  # RevG
    max_num_dboards = 2
    mboard_sensor_callback_map = {
        # List of motherboard sensors that are always available. There are also
        # GPS sensors, but they get added during __init__() only when there is
        # a GPS available.
        'ref_locked': 'get_ref_lock_sensor',
        'fan0': 'get_fan0_sensor',
        'fan1': 'get_fan1_sensor',
        'temp_fpga' : 'get_fpga_temp_sensor',
        'temp_internal' : 'get_internal_temp_sensor',
        'temp_main_power' : 'get_main_power_temp_sensor',
        'temp_scu_internal' : 'get_scu_internal_temp_sensor',
    }
    db_iface = X4xxDboardIface
    dboard_eeprom_magic = eeprom_magic
    updateable_components = {
        'fpga': {
            'callback': "update_fpga",
            'path': '/lib/firmware/{}.bin',
            'reset': True,
            'check_dts_for_compatibility': True,
            'compatibility': {
                'fpga': {
                    'current': X400_FPGA_COMPAT,
                    'oldest': (7, 0),
                },
                'cpld_ifc' : {
                    'current': (2, 0),
                    'oldest': (2, 0),
                },
                'db_gpio_ifc': {
                    'current': (1, 0),
                    'oldest': (1, 0),
                },
                'rf_core_100m': {
                    'current': (1, 0),
                    'oldest': (1, 0),
                },
                'rf_core_400m': {
                    'current': (1, 0),
                    'oldest': (1, 0),
                },
            }
        },
        'dts': {
            'callback': "update_dts",
            'path': '/lib/firmware/{}.dts',
            'output': '/lib/firmware/{}.dtbo',
            'reset': False,
        },
    }
    discoverable_features = ["ref_clk_calibration", "time_export", "trig_io_mode", "gpio_power"]
    #
    # End of overridables from PeriphManagerBase
    ###########################################################################


    # X400-specific settings
    # Label for the mboard UIO
    mboard_regs_label = "mboard-regs"
    ctrlport_regs_label = "ctrlport-mboard-regs"
    # Label for the white rabbit UIO
    wr_regs_label = "wr-regs"
    # Override the list of updateable components
    # X4xx specific discoverable features

    @classmethod
    def generate_device_info(cls, eeprom_md, mboard_info, dboard_infos):
        """
        Hard-code our product map
        """
        # Add the default PeriphManagerBase information first
        device_info = super().generate_device_info(
            eeprom_md, mboard_info, dboard_infos)
        # Then add X4xx-specific information
        mb_pid = eeprom_md.get('pid')
        device_info['product'] = cls.pids.get(mb_pid, 'unknown')
        module_serial = eeprom_md.get('module_serial')
        if module_serial is not None:
            device_info['serial'] = module_serial
        return device_info

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

    def _init_mboard_overlays(self):
        """
        Load all required overlays for this motherboard
        Overriden from the base implementation to force apply even if
        the overlay was already loaded.
        """
        requested_overlays = self.list_required_dt_overlays(
            self.device_info,
        )
        self.log.debug("Motherboard requests device tree overlays: {}".format(
            requested_overlays
        ))
        # Remove all overlays before applying new ones
        for overlay in requested_overlays:
            dtoverlay.rm_overlay_safe(overlay)
        for overlay in requested_overlays:
            dtoverlay.apply_overlay_safe(overlay)
        # Need to wait here a second to make sure the ethernet interfaces are up
        # TODO: Fine-tune this number, or wait for some smarter signal.
        sleep(1)

    ###########################################################################
    # Ctor and device initialization tasks
    ###########################################################################
    def __init__(self, args):
        super(x4xx, self).__init__()

        self._tear_down = False
        self._rpu_initialized = False
        self._status_monitor_thread = None
        self._master_clock_rate = None
        self._gps_mgr = None
        self._clk_mgr = None
        self._safe_sync_source = {
            'clock_source': X400_DEFAULT_CLOCK_SOURCE,
            'time_source': X400_DEFAULT_TIME_SOURCE,
        }
        self.rfdc = None
        self.mboard_regs_control = None
        self.ctrlport_regs = None
        self.cpld_control = None
        self.dio_control = None
        try:
            self._init_peripherals(args)
            self.init_dboards(args)
            # We need to init dio_control separately from peripherals
            # since it needs information about available dboards
            self._init_dio_control(args)
            self._clk_mgr.set_dboard_reset_cb(
                lambda enable: [db.reset_clock(enable) for db in self.dboards])
        except Exception as ex:
            self.log.error("Failed to initialize motherboard: %s", str(ex), exc_info=ex)
            self._initialization_status = str(ex)
            self._device_initialized = False
        if not self._device_initialized:
            # Don't try and figure out what's going on. Just give up.
            return
        try:
            if not args.get('skip_boot_init', False):
                self.init(args)
        except Exception as ex:
            self.log.warning("Failed to initialize device on boot: %s", str(ex))

        # Freeze the RFDC calibration by default
        self.rfdc.set_cal_frozen(1, 1, "both")
        self.rfdc.set_cal_frozen(1, 0, "both")

    # The parent class versions of these functions require access to self, but
    # these versions don't.
    # pylint: disable=no-self-use
    def _read_mboard_eeprom_data(self, eeprom_path):
        """ Returns a tuple (eeprom_dict, eeprom_rawdata) for the motherboard
        EEPROM.
        """
        return tlv_eeprom.read_eeprom(eeprom_path, EepromTagMap.tagmap,
                                      EepromTagMap.magic, None)

    def _read_dboard_eeprom_data(self, eeprom_path):
        """ Returns a tuple (eeprom_dict, eeprom_rawdata) for a daughterboard
        EEPROM.
        """
        return tlv_eeprom.read_eeprom(eeprom_path, EepromTagMap.tagmap,
                                      EepromTagMap.magic, None)
    # pylint: enable=no-self-use

    def _check_fpga_compat(self):
        " Throw an exception if the compat numbers don't match up "
        actual_compat = self.mboard_regs_control.get_compat_number()
        self.log.debug("Actual FPGA compat number: {:d}.{:d}".format(
            actual_compat[0], actual_compat[1]
        ))
        assert_compat_number(
            X400_FPGA_COMPAT,
            actual_compat,
            component="FPGA",
            fail_on_old_minor=False,
            log=self.log
        )

    def _init_gps_mgr(self):
        """
        Initialize the GPS manager and the sensors.
        Note that mpmd_impl queries all available sensors at initialization
        time, in order to populate the property tree. That means we can't
        dynamically load/unload sensors. Instead, we have to make sure that
        the sensors can handle the GPS sensors, even when it's disabled. That
        is pushed into the GPS manager class.
        """
        self.log.debug("Found GPS, adding sensors.")
        gps_mgr = X4xxGPSMgr(self._clocking_auxbrd, self.log)
        # We can't use _add_public_methods(), because we only want a subset of
        # the public methods. Also, we want to know which sensors were added so
        # we can also add them to mboard_sensor_callback_map.
        new_methods = gps_mgr.extend(self)
        self.mboard_sensor_callback_map.update(new_methods)
        return gps_mgr

    def _monitor_status(self):
        """
        Status monitoring thread: This should be executed in a thread. It will
        continuously monitor status of the following peripherals:

        - REF lock (update back-panel REF LED)
        """
        self.log.trace("Launching monitor loop...")
        cond = threading.Condition()
        cond.acquire()
        while not self._tear_down:
            ref_locked = self.get_ref_lock_sensor()['value'] == 'true'
            if self._clocking_auxbrd is not None:
                self._clocking_auxbrd.set_ref_lock_led(ref_locked)
            # Now wait
            if cond.wait_for(
                    lambda: self._tear_down,
                    X400_MONITOR_THREAD_INTERVAL):
                break
        cond.release()
        self.log.trace("Terminating monitor loop.")

    def _assert_rfdc_powered(self):
        """
        Assert that RFdc power is enabled, throw RuntimeError otherwise.
        """
        if not self._rfdc_powered.get():
            err_msg = "RFDC is not powered on"
            self.log.error(err_msg)
            raise RuntimeError(err_msg)

    def _get_serial_number(self):
        """
        Read the serial number from eeprom, falling back to the board S/N
        if the module S/N is not populated.
        """
        serial_number = self._eeprom_head.get("module_serial")
        if serial_number is None:
            self.log.warning(
                'Module serial number not programmed, falling back to motherboard serial')
            serial_number = self._eeprom_head["serial"]
        return serial_number.rstrip(b'\x00')

    def _init_peripherals(self, args):
        """
        Turn on all peripherals. This may throw an error on failure, so make
        sure to catch it.
        """
        # Sanity checks
        assert self.mboard_info.get('product') in self.pids.values(), \
            "Device product could not be determined!"
        # Init peripherals
        self._rfdc_powered = Gpio('RFDC_POWERED', Gpio.INPUT)
        # Init RPU Manager
        self.log.trace("Initializing RPU manager peripheral...")
        self.init_rpu()
        # Init clocking aux board
        self.log.trace("Initializing Clocking Aux Board controls...")
        has_gps = False
        try:
            self._clocking_auxbrd = ClockingAuxBrdControl()
            self.log.trace("Initialized Clocking Aux Board controls")
            has_gps = self._clocking_auxbrd.is_gps_supported()
        except RuntimeError:
            self.log.warning(
                "GPIO I2C bus could not be found for the Clocking Aux Board, "
                "disabling Clocking Aux Board functionality.")
            self._clocking_auxbrd = None
        self._safe_sync_source = {
            'clock_source': X4xxClockMgr.CLOCK_SOURCE_MBOARD,
            'time_source': X4xxClockMgr.TIME_SOURCE_INTERNAL,
        }

        initial_clock_source = args.get('clock_source', X400_DEFAULT_CLOCK_SOURCE)
        if self._clocking_auxbrd:
            self._add_public_methods(self._clocking_auxbrd, "clkaux")
        else:
            initial_clock_source = X4xxClockMgr.CLOCK_SOURCE_MBOARD

        # Init CPLD before talking to clocking ICs
        cpld_spi_node = dt_symbol_get_spidev('mb_cpld')
        self.cpld_control = MboardCPLD(cpld_spi_node, self.log)
        self.cpld_control.check_signature()
        self.cpld_control.check_compat_version()
        self.cpld_control.trace_git_hash()

        self._assert_rfdc_powered()
        # Init clocking after CPLD as the SPLL communication is relying on it.
        # We try and guess the correct master clock rate here based on defaults
        # and args. Since we are still in __init__(), the args that come from mpm.conf
        # are empty. We can't detect the real default MCR, because we need
        # the RFDC controls for that -- but they won't work without clocks. So
        # let's pick a sensible default MCR value, init the clocks, and fix the
        # MCR value further down.
        self._master_clock_rate = float(
            args.get('master_clock_rate', X400_DEFAULT_MASTER_CLOCK_RATE))
        sample_clock_freq, _, is_legacy_mode, _ = \
            X4xxRfdcCtrl.master_to_sample_clk[self._master_clock_rate]
        self._clk_mgr = X4xxClockMgr(
            initial_clock_source,
            time_source=args.get('time_source', X400_DEFAULT_TIME_SOURCE),
            ref_clock_freq=float(args.get(
                'ext_clock_freq', X400_DEFAULT_EXT_CLOCK_FREQ)),
            sample_clock_freq=sample_clock_freq,
            is_legacy_mode=is_legacy_mode,
            clk_aux_board=self._clocking_auxbrd,
            cpld_control=self.cpld_control,
            log=self.log)
        self._add_public_methods(
            self._clk_mgr,
            prefix="",
            filter_cb=lambda name, method: not hasattr(method, '_norpc'),
            allow_overwrite=True
        )

        # Overlay must be applied after clocks have been configured
        self.overlay_apply()

        # Init Mboard Regs
        self.log.trace("Initializing MBoard reg controls...")
        serial_number = self._get_serial_number()
        self.mboard_regs_control = MboardRegsControl(
            self.mboard_regs_label, self.log)
        self._check_fpga_compat()
        self.mboard_regs_control.set_serial_number(serial_number)
        self.mboard_regs_control.get_git_hash()
        self.mboard_regs_control.get_build_timestamp()
        self._clk_mgr.mboard_regs_control = self.mboard_regs_control

        # Create control for RFDC
        self.rfdc = X4xxRfdcCtrl(self._clk_mgr.get_spll_freq, self.log)
        self._add_public_methods(
            self.rfdc, prefix="",
            filter_cb=lambda name, method: not hasattr(method, '_norpc')
        )

        self._update_fpga_type()

        # Force reset the RFDC to ensure it is in a good state
        self.rfdc.set_reset(reset=True)
        self.rfdc.set_reset(reset=False)

        # Synchronize SYSREF and clock distributed to all converters
        self.rfdc.sync()
        self._clk_mgr.set_rfdc_reset_cb(self.rfdc.set_reset)

        # The initial default mcr only works if we have an FPGA with
        # a decimation of 2. But we need the overlay applied before we
        # can detect decimation, and that requires clocks to be initialized.
        self.set_master_clock_rate(self.rfdc.get_default_mcr())

        # Init ctrlport endpoint
        self.ctrlport_regs = CtrlportRegs(self.ctrlport_regs_label, self.log)

        # Init IPass cable status forwarding and CMI
        self.cpld_control.set_serial_number(serial_number)
        self.cpld_control.set_cmi_device_ready(
            self.mboard_regs_control.is_pcie_present())
        # The CMI transmission can be disabled by setting the cable status
        # to be not connected. All images except for the LV PCIe variant
        # provide a fixed "cables are unconnected" status. The LV PCIe image
        # reports the correct status. As the FPGA holds this information it
        # is possible to always enable the iPass cable present forwarding.
        self.ctrlport_regs.enable_cable_present_forwarding(True)

        # Init QSFP modules
        for idx, config in enumerate(X400_QSFP_I2C_CONFIGS):
            attr = QSFPModule(
                config.modprs, config.modsel, config.devsymbol, self.log)
            setattr(self, "_qsfp_module{}".format(idx), attr)
            self._add_public_methods(attr, "qsfp{}".format(idx))

        # Init GPS
        if has_gps:
            self._gps_mgr = self._init_gps_mgr()
        # Init CHDR transports
        self._xport_mgrs = {
            'udp': X400XportMgrUDP(self.log, args),
        }
        # Spawn status monitoring thread
        self.log.trace("Spawning status monitor thread...")
        self._status_monitor_thread = threading.Thread(
            target=self._monitor_status,
            name="X4xxStatusMonitorThread",
            daemon=True,
        )
        self._status_monitor_thread.start()
        # Init complete.
        self.log.debug("Device info: {}".format(self.device_info))

    def _init_dio_control(self, _):
        """
        Turn on gpio peripherals. This may throw an error on failure, so make
        sure to catch it.
        """
        if self._check_compat_aux_board(DIOAUX_EEPROM, DIOAUX_PID):
            self.dio_control = DioControl(self.mboard_regs_control,
                                          self.cpld_control, self.log,
                                          self.dboards)
            # add dio_control public methods to MPM API
            self._add_public_methods(self.dio_control, "dio")

    def _check_compat_aux_board(self, name, pid):
        """
        Check whether auxiliary board given by name and pid can be found
        :param name: symbol name of the auxiliary board which is used as
                     lookup for the dictionary of available boards.
        :param pid:  PID the board must have to be considered compatible
        :return True if board is available with matching PID,
                False otherwise
        """
        assert(isinstance(self._aux_board_infos, dict)), "No EEPROM data"
        board_info = self._aux_board_infos.get(name, None)
        if board_info is None:
            self.log.warning("Board for %s not present" % name)
            return False
        if board_info.get("pid", 0) != pid:
            self.log.error("Expected PID for board %s to be 0x%04x but found "
                           "0x%04x" % (name, pid, board_info["pid"]))
            return False
        self.log.debug("Found compatible board for %s "
                       "(PID: 0x%04x)" % (name, board_info["pid"]))
        return True

    def init_rpu(self):
        """
        Initializes the RPU image manager
        """
        if self._rpu_initialized:
            return

        # Check presence/state of RPU cores
        try:
            for core_number in [0, 1]:
                self.log.trace(
                    "RPU Core %d state: %s",
                    core_number,
                    self.get_rpu_state(core_number))
                # TODO [psisterh, 5 Dec 2019]
                # Should we force core to
                #   stop if running or in error state?
            self.log.trace("Initialized RPU cores successfully.")
            self._rpu_initialized = True
        except FileNotFoundError:
            self.log.warning(
                "Failed to initialize RPU: remoteproc sysfs not present.")

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

        # We need to disable the PPS out during clock and dboard initialization in order
        # to avoid glitches.
        if self._clocking_auxbrd is not None:
            self._clocking_auxbrd.set_trig(False)

        # If the caller has not specified clock_source or time_source, set them
        # to the default values.
        args['clock_source'] = args.get('clock_source', X400_DEFAULT_CLOCK_SOURCE)
        args['time_source'] = args.get('time_source', X400_DEFAULT_TIME_SOURCE)
        self.set_sync_source(args)

        # If a Master Clock Rate was specified,
        # re-configure the Sample PLL and all downstream clocks
        if 'master_clock_rate' in args:
            self.set_master_clock_rate(float(args['master_clock_rate']))

        # Initialize CtrlportRegs (manually opens the UIO resource for faster access)
        self.ctrlport_regs.init()

        # Note: The parent class takes care of calling init() on all the
        # daughterboards
        result = super(x4xx, self).init(args)

        # Now the clocks are all enabled, we can also enable PPS export:
        if self._clocking_auxbrd is not None:
            self._clocking_auxbrd.set_trig(
                args.get('pps_export', X400_DEFAULT_ENABLE_PPS_EXPORT),
                args.get('trig_direction', X400_DEFAULT_TRIG_DIRECTION)
                )

        return result

    def deinit(self):
        """
        Clean up after a UHD session terminates.
        """
        if not self._device_initialized:
            self.log.warning(
                "Cannot run deinit(), device was never fully initialized!")
            return

        if self.get_ref_lock_sensor()['unit'] != 'locked':
            self.log.error("ref clocks aren't locked, falling back to default")
            source = {"clock_source": X400_DEFAULT_CLOCK_SOURCE,
                      "time_source": X400_DEFAULT_TIME_SOURCE
                     }
            self.set_sync_source(source)
        super(x4xx, self).deinit()
        self.ctrlport_regs.deinit()

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        For X400, this means the overlay.
        """
        self.log.trace("Tearing down X4xx device...")
        self._tear_down = True
        if self._device_initialized:
            self._status_monitor_thread.join(3 * X400_MONITOR_THREAD_INTERVAL)
            if self._status_monitor_thread.is_alive():
                self.log.error("Could not terminate monitor thread! "
                               "This could result in resource leaks.")
        # call tear_down on daughterboards first
        super(x4xx, self).tear_down()
        if self.dio_control is not None:
            self.dio_control.tear_down()
        self.rfdc.tear_down()
        self._clk_mgr.unset_cbs()
        # remove x4xx overlay
        active_overlays = self.list_active_overlays()
        self.log.trace("X4xx has active device tree overlays: {}".format(
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

    def is_db_gpio_ifc_present(self, slot_id):
        """
        Return if daughterboard GPIO interface at 'slot_id' is present in the FPGA
        """
        db_gpio_version = self.mboard_regs_control.get_db_gpio_ifc_version(slot_id)
        return db_gpio_version[0] > 0

    ###########################################################################
    # Clock/Time API
    ###########################################################################
    def get_clock_sources(self):
        """
        Lists all available clock sources.
        """
        return self._clk_mgr.get_clock_sources()

    def set_clock_source(self, *args):
        """
        Ensures the new reference clock source and current time source pairing
        is valid and sets both by calling set_sync_source().
        """
        clock_source = args[0]
        time_source = self._clk_mgr.get_time_source()
        assert clock_source is not None
        assert time_source is not None
        if (clock_source, time_source) not in self._clk_mgr.valid_sync_sources:
            old_time_source = time_source
            if clock_source in (
                    X4xxClockMgr.CLOCK_SOURCE_MBOARD,
                    X4xxClockMgr.CLOCK_SOURCE_INTERNAL):
                time_source = X4xxClockMgr.TIME_SOURCE_INTERNAL
            elif clock_source == X4xxClockMgr.CLOCK_SOURCE_EXTERNAL:
                time_source = X4xxClockMgr.TIME_SOURCE_EXTERNAL
            elif clock_source == X4xxClockMgr.CLOCK_SOURCE_GPSDO:
                time_source = X4xxClockMgr.TIME_SOURCE_GPSDO
            self.log.warning(
                f"Time source '{old_time_source}' is an invalid selection with "
                f"clock source '{clock_source}'. "
                f"Coercing time source to '{time_source}'")
        self.set_sync_source({
            "clock_source": clock_source, "time_source": time_source})

    def set_clock_source_out(self, enable):
        """
        Allows routing the clock configured as source on the clk aux board to
        the RefOut terminal. This only applies to internal, gpsdo and nsync.
        """
        self._clk_mgr.set_clock_source_out(enable)

    def get_time_sources(self):
        " Returns list of valid time sources "
        return self._clk_mgr.get_time_sources()

    def set_time_source(self, time_source):
        """
        Set a time source

        This will call set_sync_source() internally, and use the current clock
        source as a clock source. If the current clock source plus the requested
        time source is not a valid combination, it will coerce the clock source
        to a valid choice and print a warning.
        """
        clock_source = self._clk_mgr.get_clock_source()
        assert clock_source is not None
        assert time_source is not None
        if (clock_source, time_source) not in self._clk_mgr.valid_sync_sources:
            old_clock_source = clock_source
            if time_source == X4xxClockMgr.TIME_SOURCE_QSFP0:
                clock_source = X4xxClockMgr.CLOCK_SOURCE_MBOARD
            elif time_source == X4xxClockMgr.TIME_SOURCE_INTERNAL:
                clock_source = X4xxClockMgr.CLOCK_SOURCE_MBOARD
            elif time_source == X4xxClockMgr.TIME_SOURCE_EXTERNAL:
                clock_source = X4xxClockMgr.CLOCK_SOURCE_EXTERNAL
            elif time_source == X4xxClockMgr.TIME_SOURCE_GPSDO:
                clock_source = X4xxClockMgr.CLOCK_SOURCE_GPSDO
            self.log.warning(
                'Clock source {} is an invalid selection with time source {}. '
                'Coercing clock source to {}'
                .format(old_clock_source, time_source, clock_source))
        self.set_sync_source(
            {"time_source": time_source, "clock_source": clock_source})

    def set_sync_source(self, args):
        """
        Selects reference clock and PPS sources. Unconditionally re-applies the
        time source to ensure continuity between the reference clock and time
        rates.
        Note that if we change the source such that the time source is changed
        to 'external', then we need to also disable exporting the reference
        clock (RefOut and PPS-In are the same SMA connector).
        """
        # Check the clock source, time source, and combined pair are valid:
        clock_source = args.get('clock_source', self._clk_mgr.get_clock_source())
        if clock_source not in self.get_clock_sources():
            raise ValueError(f'Clock source {clock_source} is not a valid selection')
        time_source = args.get('time_source', self._clk_mgr.get_time_source())
        if time_source not in self.get_time_sources():
            raise ValueError(f'Time source {time_source} is not a valid selection')
        if (clock_source, time_source) not in self._clk_mgr.valid_sync_sources:
            raise ValueError(
                f'Clock and time source pair ({clock_source}, {time_source}) is '
                'not a valid selection')
        # Sanity checks complete. Now check if we need to disable the RefOut.
        # Reminder: RefOut and PPSIn share an SMA. Besides, you can't export an
        # external clock. We are thus not checking for time_source == 'external'
        # because that's a subset of clock_source == 'external'.
        # We also disable clock exports for 'mboard', because the mboard clock
        # does not get routed back to the clocking aux board and thus can't be
        # exported either.
        if clock_source in (X4xxClockMgr.CLOCK_SOURCE_EXTERNAL,
                            X4xxClockMgr.CLOCK_SOURCE_MBOARD) and \
                                    self._clocking_auxbrd:
            self._clocking_auxbrd.export_clock(enable=False)
        # Now the clock manager can do its thing.
        ret_val = self._clk_mgr.set_sync_source(clock_source, time_source)
        if ret_val == self._clk_mgr.SetSyncRetVal.NOP:
            return
        try:
            # Re-set master clock rate. If this doesn't work, it will time out
            # and throw an exception. We need to put the device back into a safe
            # state in that case.
            self.set_master_clock_rate(self._master_clock_rate)
            # Restore the nco frequency to the same values as before the sync source
            # was changed, to ensure the device transmission/acquisition continues at
            # the requested frequency.
            self.rfdc.rfdc_restore_nco_freq()
            # Do the same for the calibration freeze state
            self.rfdc.rfdc_restore_cal_freeze()
        except RuntimeError as ex:
            err = f"Setting clock_source={clock_source},time_source={time_source} " \
                  f"failed, falling back to {self._safe_sync_source}. Error: " \
                  f"{ex}"
            self.log.error(err)
            if args.get('__noretry__', False):
                self.log.error("Giving up.")
            else:
                self.set_sync_source({**self._safe_sync_source, '__noretry__': True})
            raise

    def set_master_clock_rate(self, master_clock_rate):
        """
        Sets the master clock rate by configuring the RFDC decimation and SPLL,
        and then resetting downstream clocks.
        """
        if master_clock_rate not in self.rfdc.master_to_sample_clk:
            self.log.error('Unsupported master clock rate selection {}'
                           .format(master_clock_rate))
            raise RuntimeError('Unsupported master clock rate selection')
        sample_clock_freq, decimation, is_legacy_mode, halfband = \
                self.rfdc.master_to_sample_clk[master_clock_rate]
        for db_idx, _ in enumerate(self.dboards):
            db_rfdc_resamp, db_halfband = self.rfdc.get_rfdc_resampling_factor(db_idx)
            if db_rfdc_resamp != decimation or db_halfband != halfband:
                msg = (f'master_clock_rate {master_clock_rate} is not compatible '
                       f'with FPGA which expected decimation {db_rfdc_resamp}')
                self.log.error(msg)
                raise RuntimeError(msg)
        self.log.trace(f"Set master clock rate (SPLL) to: {master_clock_rate}")
        self._clk_mgr.set_spll_rate(sample_clock_freq, is_legacy_mode)
        self._master_clock_rate = master_clock_rate
        self.rfdc.sync()
        self._clk_mgr.config_pps_to_timekeeper(master_clock_rate)

    def set_trigger_io(self, direction):
        """
        Switch direction of clocking board Trigger I/O SMA socket.
        IMPORTANT! Ensure downstream devices depending on TRIG I/O's output ignore
        this signal when calling this method or re-run their synchronization routine
        after calling this method. The output-enable control is async. to the output.
        :param self:
        :param direction: "off" trigger io socket unused
                          "pps_output" device will output PPS signal
                          "input" PPS is fed into the device from external
        :return: success status as boolean
        """
        OFF = "off"
        INPUT = "input"
        PPS_OUTPUT = "pps_output"
        directions = [OFF, INPUT, PPS_OUTPUT]

        if not self._clocking_auxbrd:
            raise RuntimeError("No clocking aux board available")
        if not direction in directions:
            raise RuntimeError("Invalid trigger io direction (%s). Use one of %s"
                               % (direction, directions))

        # Switching order of trigger I/O lines depends on requested direction.
        # Always turn on new driver last so both drivers cannot be active
        # simultaneously.
        if direction == INPUT:
            self.mboard_regs_control.set_trig_io_output(False)
            self._clocking_auxbrd.set_trig(1, ClockingAuxBrdControl.DIRECTION_INPUT)
        elif direction == PPS_OUTPUT:
            self._clocking_auxbrd.set_trig(1, ClockingAuxBrdControl.DIRECTION_OUTPUT)
            self.mboard_regs_control.set_trig_io_output(True)
        else: # direction == OFF:
            self.mboard_regs_control.set_trig_io_output(False)
            self._clocking_auxbrd.set_trig(0)

        return True

    ###########################################################################
    # EEPROMs
    ###########################################################################
    def get_db_eeprom(self, dboard_idx):
        """
        See PeriphManagerBase.get_db_eeprom() for docs.
        """
        try:
            dboard = self.dboards[dboard_idx]
        except IndexError:
            error_msg = "Attempted to access invalid dboard index `{}' " \
                        "in get_db_eeprom()!".format(dboard_idx)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        db_eeprom_data = copy.copy(dboard.device_info)
        return db_eeprom_data

    ###########################################################################
    # Component updating
    ###########################################################################
    # Note: Component updating functions defined by ZynqComponents
    @no_rpc
    def _update_fpga_type(self):
        """Update the fpga type stored in the updateable components"""
        fpga_string = "{}_{}".format(
            self.mboard_regs_control.get_fpga_type(),
            self.rfdc.get_dsp_bw())
        self.log.debug("Updating mboard FPGA type info to {}".format(fpga_string))
        self.updateable_components['fpga']['type'] = fpga_string

    #######################################################################
    # Timekeeper API
    #######################################################################
    def get_master_clock_rate(self):
        """ Return the master clock rate set during init """
        return self._master_clock_rate

    def get_clocks(self):
        """
        Gets the RFNoC-related clocks present in the FPGA design
        """
        # TODO: The 200 and 40 MHz clocks should not be hard coded, and ideally
        # be linked to the FPGA image somehow
        return [
            {
                'name': 'radio_clk',
                'freq': str(self.get_master_clock_rate()),
                'mutable': 'true'
            },
            {
                'name': 'bus_clk',
                'freq': str(200e6),
            },
            {
                'name': 'ctrl_clk',
                'freq': str(40e6),
            }
        ]

    ###########################################################################
    # GPIO API
    ###########################################################################
    def get_gpio_banks(self):
        """
        Returns a list of GPIO banks over which MPM has any control
        """
        if self.dio_control is None:
            return []
        return self.dio_control.get_gpio_banks()

    def get_gpio_srcs(self, bank: str):
        """
        Return a list of valid GPIO sources for a given bank
        """
        if self.dio_control is None:
            return []
        return self.dio_control.get_gpio_srcs(bank)

    def get_gpio_src(self, bank: str):
        """
        Return the currently selected GPIO source for a given bank. The return
        value is a list of strings. The length of the vector is identical to
        the number of controllable GPIO pins on this bank. CUSTOM is for
        miscellaneous pin source, and USER_APP is for LabView pin source.
        """
        if self.dio_control is None:
            raise RuntimeError("Unable to query GPIO source: No valid DIO board installed.")
        return self.dio_control.get_gpio_src(bank)

    def set_gpio_src(self, bank: str, *src):
        """
        Set the GPIO source for a given bank.
        src input is big-endian
        Usage:
        > set_gpio_src <bank> <srcs>
        > set_gpio_src GPIO0 PS DB1_RF1 PS PS MPM PS PS PS MPM USER_APP PS
        """
        if self.dio_control is None:
            raise RuntimeError("Unable to set GPIO source: No valid DIO board installed.")
        self.dio_control.set_gpio_src(bank, *src)

    ###########################################################################
    # Utility for validating RPU core number
    ###########################################################################
    @no_rpc
    def _validate_rpu_core_number(self, core_number):
        if ((core_number < 0) or (core_number > 1)):
            raise RuntimeError("RPU core number must be 0 or 1.")


    ###########################################################################
    # Utility for validating RPU state change command
    ###########################################################################
    @no_rpc
    def _validate_rpu_state(self, new_state_command, previous_state):
        if ((new_state_command != RPU_STATE_COMMAND_START)
                and (new_state_command != RPU_STATE_COMMAND_STOP)):
            raise RuntimeError("RPU state command must be start or stop.")
        if ((new_state_command == RPU_STATE_COMMAND_START)
                and (previous_state == RPU_STATE_RUNNING)):
            raise RuntimeError("RPU already running.")
        if ((new_state_command == RPU_STATE_COMMAND_STOP)
                and (previous_state == RPU_STATE_OFFLINE)):
            raise RuntimeError("RPU already offline.")

    ###########################################################################
    # Utility for validating RPU firmware
    ###########################################################################
    @no_rpc
    def _validate_rpu_firmware(self, firmware):
        file_path = path.join(RPU_REMOTEPROC_FIRMWARE_PATH, firmware)
        if not path.isfile(file_path):
            raise RuntimeError("Specified firmware does not exist.")

    ###########################################################################
    # Utility for reading contents of a file
    ###########################################################################
    @no_rpc
    def _read_file(self, file_path):
        self.log.trace("_read_file: file_path= %s", file_path)
        with open(file_path, 'r') as f:
            return f.read().strip()


    ###########################################################################
    # Utility for writing contents of a file
    ###########################################################################
    @no_rpc
    def _write_file(self, file_path, data):
        self.log.trace("_write_file: file_path= %s, data= %s", file_path, data)
        with open(file_path, 'w') as f:
            f.write(data)


    ###########################################################################
    # RPU Image Deployment API
    ###########################################################################
    def get_rpu_state(self, core_number, validate=True):
        """ Report the state for the specified RPU core """
        if validate:
            self._validate_rpu_core_number(core_number)
        return self._read_file(
            path.join(
                RPU_REMOTEPROC_PREFIX_PATH + str(core_number),
                RPU_REMOTEPROC_PROPERTY_STATE))


    def set_rpu_state(self, core_number, new_state_command, validate=True):
        """ Set the specified state for the specified RPU core """
        if not self._rpu_initialized:
            self.log.warning(
                "Failed to set RPU state: RPU peripheral not "\
                "initialized.")
            return RPU_FAILURE_REPORT
        # OK, RPU is initialized, now go set its state:
        if validate:
            self._validate_rpu_core_number(core_number)
        previous_state = self.get_rpu_state(core_number, False)
        if validate:
            self._validate_rpu_state(new_state_command, previous_state)
        self._write_file(
            path.join(
                RPU_REMOTEPROC_PREFIX_PATH + str(core_number),
                RPU_REMOTEPROC_PROPERTY_STATE),
            new_state_command)

        # Give RPU core time to change state (might load new fw)
        poll_with_timeout(
            lambda: previous_state != self.get_rpu_state(core_number, False),
            RPU_MAX_STATE_CHANGE_TIME_IN_MS,
            RPU_STATE_CHANGE_POLLING_INTERVAL_IN_MS)

        # Quick validation of new state
        resulting_state = self.get_rpu_state(core_number, False)
        if ((new_state_command == RPU_STATE_COMMAND_START)
                and (resulting_state != RPU_STATE_RUNNING)):
            raise RuntimeError('Unable to start specified RPU core.')
        if ((new_state_command == RPU_STATE_COMMAND_STOP)
                and (resulting_state != RPU_STATE_OFFLINE)):
            raise RuntimeError('Unable to stop specified RPU core.')
        return RPU_SUCCESS_REPORT

    def get_rpu_firmware(self, core_number):
        """ Report the firmware for the specified RPU core """
        self._validate_rpu_core_number(core_number)
        return self._read_file(
            path.join(
                RPU_REMOTEPROC_PREFIX_PATH + str(core_number),
                RPU_REMOTEPROC_PROPERTY_FIRMWARE))


    def set_rpu_firmware(self, core_number, firmware, start=0):
        """ Deploy the image at the specified path to the RPU """
        self.log.trace("set_rpu_firmware")
        self.log.trace(
            "image path: %s, core_number: %d, start?: %d",
            firmware,
            core_number,
            start)

        if not self._rpu_initialized:
            self.log.warning(
                "Failed to deploy RPU image: "\
                "RPU peripheral not initialized.")
            return RPU_FAILURE_REPORT
        # RPU is initialized, now go set firmware:
        self._validate_rpu_core_number(core_number)
        self._validate_rpu_firmware(firmware)

        # Stop the core if necessary
        if self.get_rpu_state(core_number, False) == RPU_STATE_RUNNING:
            self.set_rpu_state(core_number, RPU_STATE_COMMAND_STOP, False)

        # Set the new firmware path
        self._write_file(
            path.join(
                RPU_REMOTEPROC_PREFIX_PATH + str(core_number),
                RPU_REMOTEPROC_PROPERTY_FIRMWARE),
            firmware)

        # Start the core if requested
        if start != 0:
            self.set_rpu_state(core_number, RPU_STATE_COMMAND_START, False)
        return RPU_SUCCESS_REPORT

    #######################################################################
    # Debugging
    # Provides temporary methods for arbitrary hardware access while
    # development for these components is ongoing.
    #######################################################################
    def peek_ctrlport(self, addr):
        """ Peek the MPM Endpoint to ctrlport registers on the FPGA """
        return '0x{:X}'.format(self.ctrlport_regs.peek32(addr))

    def poke_ctrlport(self, addr, val):
        """ Poke the MPM Endpoint to ctrlport registers on the FPGA """
        self.ctrlport_regs.poke32(addr, val)

    def peek_cpld(self, addr):
        """ Peek the PS portion of the MB CPLD """
        return '0x{:X}'.format(self.cpld_control.peek32(addr))

    def poke_cpld(self, addr, val):
        """ Poke the PS portion of the MB CPLD """
        self.cpld_control.poke32(addr, val)

    def peek_mb(self, addr):
        """ Peek the MB Regs """
        return '0x{:X}'.format(
            self.mboard_regs_control.peek32(addr))

    def poke_mb(self, addr, val):
        """ Poke the MB CPLD """
        self.mboard_regs_control.poke32(addr, val)

    def peek_db(self, db_id, addr):
        """ Peek the DB CPLD, even if the DB is not discovered by MPM """
        assert db_id in (0, 1)
        self.cpld_control.enable_daughterboard(db_id)
        return '0x{:X}'.format(
            self.ctrlport_regs.get_db_cpld_iface(db_id).peek32(addr))

    def poke_db(self, db_id, addr, val):
        """ Poke the DB CPLD, even if the DB is not discovered by MPM """
        assert db_id in (0, 1)
        self.cpld_control.enable_daughterboard(db_id)
        self.ctrlport_regs.get_db_cpld_iface(db_id).poke32(addr, val)

    def peek_clkaux(self, addr):
        """Peek the ClkAux DB over SPI"""
        return '0x{:X}'.format(self._clocking_auxbrd.peek8(addr))

    def poke_clkaux(self, addr, val):
        """Poke the ClkAux DB over SPI"""
        self._clocking_auxbrd.poke8(addr, val)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_ref_lock_sensor(self):
        """
        Return main refclock lock status. This is the lock status of the
        reference and sample PLLs.
        """
        lock_status = self._clk_mgr.get_ref_locked()
        return {
            'name': 'ref_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_fpga_temp_sensor(self):
        """ Get temperature sensor reading of the X4xx FPGA. """
        self.log.trace("Reading FPGA temperature.")
        return get_temp_sensor(["RFSoC"], log=self.log)

    def get_main_power_temp_sensor(self):
        """
        Get temperature sensor reading of PM-BUS devices which supply
        0.85V power supply to RFSoC.
        """
        self.log.trace("Reading PMBus Power Supply Chip(s) temperature.")
        return get_temp_sensor(["PMBUS-0", "PMBUS-1"], log=self.log)

    def get_scu_internal_temp_sensor(self):
        """ Get temperature sensor reading of STM32 SCU's internal sensor. """
        self.log.trace("Reading SCU internal temperature.")
        return get_temp_sensor(["EC Internal"], log=self.log)

    def get_internal_temp_sensor(self):
        """ TODO: Determine how to interpret this function """
        self.log.warning("Reading internal temperature is not yet implemented.")
        return {
            'name': 'temperature',
            'type': 'REALNUM',
            'unit': 'C',
            'value': '-1'
        }

    def _get_fan_sensor(self, fan='fan0'):
        """ Get fan speed. """
        self.log.trace("Reading {} speed sensor.".format(fan))
        fan_rpm = -1
        try:
            fan_rpm_all = ectool.get_fan_rpm()
            fan_rpm = fan_rpm_all[fan]
        except Exception as ex:
            self.log.warning(
                "Error occurred when getting {} speed value: {} "
                .format(fan, str(ex)))
        return {
            'name': fan,
            'type': 'INTEGER',
            'unit': 'rpm',
            'value': str(fan_rpm)
        }

    def get_fan0_sensor(self):
        """ Get fan0 speed. """
        return self._get_fan_sensor('fan0')

    def get_fan1_sensor(self):
        """ Get fan1 speed."""
        return self._get_fan_sensor('fan1')

    def get_gps_sensor_status(self):
        """
        Get all status info of GPS in a single string. This is a debugging API.
        """
        assert self._gps_mgr
        self.log.trace("Reading all GPS status pins")
        return f"""
            {self.get_gps_lock_sensor()}
            {self.get_gps_alarm_sensor()}
            {self.get_gps_warmup_sensor()}
            {self.get_gps_survey_sensor()}
            {self.get_gps_phase_lock_sensor()}
        """
