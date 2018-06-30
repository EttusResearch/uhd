#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Magnesium dboard implementation module
"""

from __future__ import print_function
import os
import time
import threading
import math
import re
from six import iterkeys, iteritems
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.dboard_manager.lmk_mg import LMK04828Mg
from usrp_mpm.dboard_manager.mg_periphs import TCA6408, MgCPLD
from usrp_mpm.dboard_manager.mg_periphs import DboardClockControl
from usrp_mpm.cores import nijesdcore
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.mpmutils import async_exec
from usrp_mpm.sys_utils.uio import open_uio
from usrp_mpm.sys_utils.udev import get_eeprom_paths
from usrp_mpm.cores import ClockSynchronizer
from usrp_mpm.bfrfs import BufferFS

INIT_CALIBRATION_TABLE = {"TX_BB_FILTER"              :   0x0001,
                          "ADC_TUNER"                 :   0x0002,
                          "TIA_3DB_CORNER"            :   0x0004,
                          "DC_OFFSET"                 :   0x0008,
                          "TX_ATTENUATION_DELAY"      :   0x0010,
                          "RX_GAIN_DELAY"             :   0x0020,
                          "FLASH_CAL"                 :   0x0040,
                          "PATH_DELAY"                :   0x0080,
                          "TX_LO_LEAKAGE_INTERNAL"    :   0x0100,
                          "TX_LO_LEAKAGE_EXTERNAL"    :   0x0200,
                          "TX_QEC_INIT"               :   0x0400,
                          "LOOPBACK_RX_LO_DELAY"      :   0x0800,
                          "LOOPBACK_RX_RX_QEC_INIT"   :   0x1000,
                          "RX_LO_DELAY"               :   0x2000,
                          "RX_QEC_INIT"               :   0x4000,
                          "BASIC"                     :   0x4F,
                          "OFF"                       :   0x00,
                          "DEFAULT"                   :   0x4DFF,
                          "ALL"                       :   0x7DFF,
                         }

TRACKING_CALIBRATION_TABLE = {"TRACK_RX1_QEC"         :   0x01,
                              "TRACK_RX2_QEC"         :   0x02,
                              "TRACK_ORX1_QEC"        :   0x04,
                              "TRACK_ORX2_QEC"        :   0x08,
                              "TRACK_TX1_LOL"         :   0x10,
                              "TRACK_TX2_LOL"         :   0x20,
                              "TRACK_TX1_QEC"         :   0x40,
                              "TRACK_TX2_QEC"         :   0x80,
                              "OFF"                   :   0x00,
                              "RX_QEC"                :   0x03,
                              "TX_QEC"                :   0xC0,
                              "TX_LOL"                :   0x30,
                              "DEFAULT"               :   0xC3,
                              "ALL"                   :   0xF3,
                             }


def create_spidev_iface_lmk(dev_node):
    """
    Create a regs iface from a spidev node
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        3, # SPI mode
        8, # Addr shift
        0, # Data shift
        1<<23, # Read flag
        0, # Write flag
    )

def create_spidev_iface_cpld(dev_node):
    """
    Create a regs iface from a spidev node
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        0, # SPI mode
        16, # Addr shift
        0, # Data shift
        1<<23, # Read flag
        0, # Write flag
    )

def create_spidev_iface_phasedac(dev_node):
    """
    Create a regs iface from a spidev node (ADS5681)
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        1, # SPI mode
        16, # Addr shift
        0, # Data shift
        0, # Read flag (phase DAC is write-only)
        0, # Write flag
    )

###############################################################################
# Peripherals
###############################################################################
###############################################################################
# Main dboard control class
###############################################################################
class Magnesium(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x150]
    rx_sensor_callback_map = {
        'lowband_lo_locked': 'get_lowband_tx_lo_locked_sensor',
        'ad9371_lo_locked': 'get_ad9371_tx_lo_locked_sensor',
    }
    tx_sensor_callback_map = {
        'lowband_lo_locked': 'get_lowband_rx_lo_locked_sensor',
        'ad9371_lo_locked': 'get_ad9371_rx_lo_locked_sensor',
    }
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"cpld": 0, "lmk": 1, "mykonos": 2, "phase_dac": 3}
    ### End of overridables #################################################
    # Class-specific, but constant settings:
    spi_factories = {
        "cpld": create_spidev_iface_cpld,
        "lmk": create_spidev_iface_lmk,
        "phase_dac": create_spidev_iface_phasedac,
    }
    #file system path to i2c-adapter/mux
    base_i2c_adapter = '/sys/class/i2c-adapter'
    # Map I2C channel to slot index
    i2c_chan_map = {0: 'i2c-9', 1: 'i2c-10'}
    # This map describes how the user data is stored in EEPROM. If a dboard rev
    # changes the way the EEPROM is used, we add a new entry. If a dboard rev
    # is not found in the map, then we go backward until we find a suitable rev
    user_eeprom = {
        2: { # RevC
            'label': "e0004000.i2c",
            'offset': 1024,
            'max_size': 32786 - 1024,
            'alignment': 1024,
        },
    }
    # DAC is initialized to midscale automatically on power-on: 16-bit DAC, so midpoint
    # is at 2^15 = 32768. However, the linearity of the DAC is best just below that
    # point, so we set it to the (carefully calculated) alternate value instead.
    INIT_PHASE_DAC_WORD = 31000 # Intentionally decimal
    PHASE_DAC_SPI_ADDR = 0x0
    # External PPS pipeline delay from the PPS captured at the FPGA to TDC input,
    # in reference clock ticks
    EXT_PPS_DELAY = 5
    # Variable PPS delay before the RP/SP pulsers begin. Fixed value for the N3xx devices.
    N3XX_INT_PPS_DELAY = 4
    default_master_clock_rate = 125e6
    default_time_source = 'internal'
    default_current_jesd_rate = 2500e6

    def __init__(self, slot_idx, **kwargs):
        super(Magnesium, self).__init__(slot_idx, **kwargs)
        self.log = get_logger("Magnesium-{}".format(slot_idx))
        self.log.trace("Initializing Magnesium daughterboard, slot index %d",
                       self.slot_idx)
        self.rev = int(self.device_info['rev'])
        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        # This is a default ref clock freq, it must be updated before init() is
        # called!
        self.ref_clock_freq = None
        # These will get updated during init()
        self.master_clock_rate = None
        self.current_jesd_rate = None
        # Predeclare some attributes to make linter happy:
        self.lmk = None
        self._port_expander = None
        self.mykonos = None
        self.eeprom_fs = None
        self.eeprom_path = None
        self.cpld = None
        self._init_cals_mask = 0
        self._tracking_cals_mask = 0
        self._init_cals_timeout = 0
        # Now initialize all peripherals. If that doesn't work, put this class
        # into a non-functional state (but don't crash, or we can't talk to it
        # any more):
        try:
            self._init_periphs()
            self._periphs_initialized = True
        except Exception as ex:
            self.log.error("Failed to initialize peripherals: %s",
                           str(ex))
            self._periphs_initialized = False

    def _init_periphs(self):
        """
        Initialize power and peripherals that don't need user-settings
        """
        self._port_expander = TCA6408(self._get_i2c_dev(self.slot_idx))
        self._power_on()
        self.log.debug("Loading C++ drivers...")

        # The Mykonos TX DeFramer lane crossbar requires configuration on a per-slot
        # basis due to motherboard MGT lane swapping.
        # The RX framer lane crossbar configuration
        # is identical for both slots and is hard-coded within the Mykonos API.
        deserializer_lane_xbar = 0xD2 if self.slot_idx == 0 else 0x72

        self._device = lib.dboards.magnesium_manager(
            self._spi_nodes['mykonos'],
            deserializer_lane_xbar
        )
        self.mykonos = self._device.get_radio_ctrl()
        self.spi_lock = self._device.get_spi_lock()
        self.log.trace("Loaded C++ drivers.")
        self._init_myk_api(self.mykonos)
        self.eeprom_fs, self.eeprom_path = self._init_user_eeprom(
            self._get_user_eeprom_info(self.rev)
        )
        self.log.trace("Loading SPI devices...")
        self._spi_ifaces = {
            key: self.spi_factories[key](self._spi_nodes[key])
            for key in self.spi_factories
        }
        self.cpld = MgCPLD(self._spi_ifaces['cpld'], self.log)
        self.device_info['cpld_rev'] = \
                str(self.cpld.major_rev) + '.' + str(self.cpld.minor_rev)

    def _power_on(self):
        " Turn on power to daughterboard "
        self.log.trace("Powering on slot_idx={}...".format(self.slot_idx))
        self._port_expander.set("PWR-EN-3.6V")
        self._port_expander.set("PWR-EN-1.5V")
        self._port_expander.set("PWR-EN-5.5V")
        self._port_expander.set("LED")

    def _power_off(self):
        " Turn off power to daughterboard "
        self.log.trace("Powering off slot_idx={}...".format(self.slot_idx))
        self._port_expander.reset("PWR-EN-3.6V")
        self._port_expander.reset("PWR-EN-1.5V")
        self._port_expander.reset("PWR-EN-5.5V")
        self._port_expander.reset("LED")

    def _get_i2c_dev(self, slot_idx):
        " Return the I2C path for this daughterboard "
        import pyudev
        context = pyudev.Context()
        i2c_dev_path = os.path.join(
            self.base_i2c_adapter,
            self.i2c_chan_map[slot_idx]
        )
        return pyudev.Devices.from_sys_path(context, i2c_dev_path)

    def _init_myk_api(self, myk):
        """
        Propagate the C++ Mykonos API into Python land.
        """
        def export_method(obj, method):
            " Export a method object, including docstring "
            meth_obj = getattr(obj, method)
            def func(*args):
                " Functor for storing docstring too "
                return meth_obj(*args)
            func.__doc__ = meth_obj.__doc__
            return func
        self.log.trace("Forwarding AD9371 methods to Magnesium class...")
        for method in [
                x for x in dir(self.mykonos)
                if not x.startswith("_") and \
                        callable(getattr(self.mykonos, x))]:
            self.log.trace("adding {}".format(method))
            setattr(self, method, export_method(myk, method))

    def _get_user_eeprom_info(self, rev):
        """
        Return an EEPROM access map (from self.user_eeprom) based on the rev.
        """
        rev_for_lookup = rev
        while rev_for_lookup not in self.user_eeprom:
            if rev_for_lookup < 0:
                raise RuntimeError("Could not find a user EEPROM map for "
                                   "revision %d!", rev)
            rev_for_lookup -= 1
        assert rev_for_lookup in self.user_eeprom, \
                "Invalid EEPROM lookup rev!"
        return self.user_eeprom[rev_for_lookup]

    def _init_user_eeprom(self, eeprom_info):
        """
        Reads out user-data EEPROM, and intializes a BufferFS object from that.
        """
        self.log.trace("Initializing EEPROM user data...")
        eeprom_paths = get_eeprom_paths(eeprom_info.get('label'))
        self.log.trace("Found the following EEPROM paths: `{}'".format(
            eeprom_paths))
        eeprom_path = eeprom_paths[self.slot_idx]
        self.log.trace("Selected EEPROM path: `{}'".format(eeprom_path))
        user_eeprom_offset = eeprom_info.get('offset', 0)
        self.log.trace("Selected EEPROM offset: %d", user_eeprom_offset)
        user_eeprom_data = open(eeprom_path, 'rb').read()[user_eeprom_offset:]
        self.log.trace("Total EEPROM size is: %d bytes", len(user_eeprom_data))
        # FIXME verify EEPROM sectors
        return BufferFS(
            user_eeprom_data,
            max_size=eeprom_info.get('max_size'),
            alignment=eeprom_info.get('alignment', 1024),
            log=self.log
        ), eeprom_path


    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """
        def _init_lmk(lmk_spi, ref_clk_freq, master_clk_rate,
                      pdac_spi, init_phase_dac_word, phase_dac_spi_addr):
            """
            Sets the phase DAC to initial value, and then brings up the LMK
            according to the selected ref clock frequency.
            Will throw if something fails.
            """
            self.log.trace("Initializing Phase DAC to d{}.".format(
                init_phase_dac_word
            ))
            pdac_spi.poke16(phase_dac_spi_addr, init_phase_dac_word)
            return LMK04828Mg(
                lmk_spi,
                self.spi_lock,
                ref_clk_freq,
                master_clk_rate,
                self.log
            )
        def _sync_db_clock():
            " Synchronizes the DB clock to the common reference "
            reg_offset = 0x200
            ref_clk_freq = self.ref_clock_freq
            ext_pps_delay = self.EXT_PPS_DELAY
            if args.get('time_source', self.default_time_source) == 'sfp0':
                reg_offset = 0x400
                ref_clk_freq = 62.5e6
                ext_pps_delay = 1 # only 1 flop between the WR core output and the TDC input
            synchronizer = ClockSynchronizer(
                dboard_ctrl_regs,
                self.lmk,
                self._spi_ifaces['phase_dac'],
                reg_offset,
                self.master_clock_rate,
                ref_clk_freq,
                860E-15, # fine phase shift. TODO don't hardcode. This should live in the EEPROM
                self.INIT_PHASE_DAC_WORD,
                self.PHASE_DAC_SPI_ADDR,
                ext_pps_delay,
                self.N3XX_INT_PPS_DELAY,
                self.slot_idx)
            # The radio clock traces on the motherboard are 69 ps longer for Daughterboard B
            # than Daughterboard A. We want both of these clocks to align at the converters
            # on each board, so adjust the target value for DB B. This is an N3xx series
            # peculiarity and will not apply to other motherboards.
            trace_delay_offset = {0:  0.0e-0,
                                  1: 69.0e-12}[self.slot_idx]
            offset = synchronizer.run(
                num_meas=[512, 128],
                target_offset = trace_delay_offset)
            offset_error = abs(offset)
            if offset_error > 100e-12:
                self.log.error("Clock synchronizer measured an offset of {:.1f} ps!".format(
                    offset_error*1e12
                ))
                raise RuntimeError("Clock synchronizer measured an offset of {:.1f} ps!".format(
                    offset_error*1e12
                ))
            else:
                self.log.debug("Residual synchronization error: {:.1f} ps.".format(
                    offset_error*1e12
                ))
            synchronizer = None
            self.log.debug("Sample Clock Synchronization Complete!")
        ## Go, go, go!
        # Sanity checks and input validation:
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        if not self._periphs_initialized:
            error_msg = "Cannot run init(), peripherals are not initialized!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        if 'ref_clk_freq' in args:
            self.ref_clock_freq = float(args['ref_clk_freq'])
            assert self.ref_clock_freq in (10e6, 20e6, 25e6)
        assert self.ref_clock_freq is not None
        master_clock_rate = \
            float(args.get('master_clock_rate',
                           self.default_master_clock_rate))
        assert master_clock_rate in (122.88e6, 125e6, 153.6e6), \
                "Invalid master clock rate: {:.02f} MHz".format(
                    master_clock_rate / 1e6)
        master_clock_rate_changed = master_clock_rate != self.master_clock_rate
        if master_clock_rate_changed:
            self.master_clock_rate = master_clock_rate
            self.log.debug("Updating master clock rate to {:.02f} MHz!".format(
                self.master_clock_rate / 1e6
            ))
        # Init some more periphs:
        # The following peripherals are only used during init, so we don't want
        # to hang on to them for the full lifetime of the Magnesium class. This
        # helps us close file descriptors associated with the UIO objects.
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            self.log.trace("Creating jesdcore object...")
            jesdcore = nijesdcore.NIMgJESDCore(dboard_ctrl_regs, self.slot_idx)
            # Now get cracking with the actual init sequence:
            self.log.trace("Creating dboard clock control object...")
            db_clk_control = DboardClockControl(dboard_ctrl_regs, self.log)
            self.log.debug("Reset Dboard Clocking and JESD204B interfaces...")
            db_clk_control.reset_mmcm()
            jesdcore.reset()
            self.log.trace("Initializing LMK...")
            self.lmk = _init_lmk(
                self._spi_ifaces['lmk'],
                self.ref_clock_freq,
                self.master_clock_rate,
                self._spi_ifaces['phase_dac'],
                self.INIT_PHASE_DAC_WORD,
                self.PHASE_DAC_SPI_ADDR,
            )
            db_clk_control.enable_mmcm()
            # Synchronize DB Clocks
            _sync_db_clock()
            self.log.debug("Sample Clocks and Phase DAC Configured Successfully!")
            # Clocks and PPS are now fully active!
            self.mykonos.set_master_clock_rate(self.master_clock_rate)
            self.init_jesd(jesdcore, args)
            jesdcore = None # Help with garbage collection
            # That's all that requires access to the dboard regs!
        if bool(args.get('rfic_digital_loopback')):
            self.log.warning("RF Functionality Disabled: JESD204b digital loopback " \
                             "enabled inside Mykonos!")
            self.mykonos.enable_jesd_loopback(1)
        else:
            self.mykonos.start_radio()
        return True

    def _parse_and_convert_cal_args(self, table, cal_args):
        """Parse calibration string and convert it to a number

        Arguments:
            table {dictionary} -- a look up table that map a type of calibration
                                  to its bit mask.(defined in AD9375-UG992)
            cal_args {string} --  string arguments from user in form of "CAL1|CAL2|CAL3"
                                  or "CAL1 CAL2 CAL3"  or "CAL1;CAL2;CAL3"

        Returns:
            int -- calibration value bit mask.
        """
        value = 0
        try:
            return int(cal_args, 0)
        except ValueError:
            pass
        for key in re.split(r'[;|\s]\s*', cal_args):
            value_tmp = table.get(key.upper())
            if (value_tmp) != None:
                value |= value_tmp
            else:
                self.log.warning(
                    "Calibration key `%s' is not in calibration table. "
                    "Ignoring this key.",
                    key.upper()
                )
        return value

    def init_rf_cal(self, args):
        " Setup RF CAL "
        self.log.debug("Setting up RF CAL...")
        try:
            self._init_cals_mask = \
                    self._parse_and_convert_cal_args(
                        INIT_CALIBRATION_TABLE,
                        args.get('init_cals', 'DEFAULT')
                    )
            self._tracking_cals_mask = \
                    self._parse_and_convert_cal_args(
                        TRACKING_CALIBRATION_TABLE,
                        args.get('tracking_cals', 'DEFAULT')
                    )
            self._init_cals_timeout = int(
                args.get('init_cals_timeout',
                         str(self.mykonos.DEFAULT_INIT_CALS_TIMEOUT))
                , 0
            )
        except ValueError as ex:
            self.log.warning("init() args missing or error using default \
                             value seeing following exception print out.")
            self.log.warning("{}".format(ex))
            self._init_cals_mask = self._parse_and_convert_cal_args(
                INIT_CALIBRATION_TABLE, 'DEFAULT')
            self._tracking_cals_mask = self._parse_and_convert_cal_args(
                TRACKING_CALIBRATION_TABLE, 'DEFAULT')
            self._init_cals_timeout = self.mykonos.DEFAULT_INIT_CALS_TIMEOUT
        self.log.debug("args[init_cals]=0x{:02X}"
                       .format(self._init_cals_mask))
        self.log.debug("args[tracking_cals]=0x{:02X}"
                       .format(self._tracking_cals_mask))
        async_exec(
            self.mykonos,
            "setup_cal",
            self._init_cals_mask,
            self._tracking_cals_mask,
            self._init_cals_timeout
        )

    def init_lo_source(self, args):
        """Set all LO

        This function will initialize all LO to user specified sources.
        If there's no source is specified, the default one will be used.

        Arguments:
            args {string:string} -- device arguments.
        """
        self.log.debug("Setting up LO source..")
        rx_lo_source = args.get("rx_lo_source", "internal")
        tx_lo_source = args.get("tx_lo_source", "internal")
        self.mykonos.set_lo_source("RX", rx_lo_source)
        self.mykonos.set_lo_source("TX", tx_lo_source)
        self.log.debug("RX LO source is set at {}".format(self.mykonos.get_lo_source("RX")))
        self.log.debug("TX LO source is set at {}".format(self.mykonos.get_lo_source("TX")))

    def init_jesd(self, jesdcore, args):
        """
        Bring up the JESD link between Mykonos and the N310.
        All clocks must be set up and stable before starting this routine.
        """
        jesdcore.check_core()

        # JESD Lane Rate only depends on the master_clock_rate selection, since all
        # other link parameters (LMFS,N) remain constant.
        L = 4
        M = 4
        F = 2
        S = 1
        N = 16
        new_rate = self.master_clock_rate * M * N * (10.0/8) / L / S
        self.log.trace("Calculated JESD204b lane rate is {} Gbps".format(new_rate/1e9))
        self.set_jesd_rate(jesdcore, new_rate)

        self.log.trace("Pulsing Mykonos Hard Reset...")
        self.cpld.reset_mykonos()
        self.log.trace("Initializing Mykonos...")
        self.init_lo_source(args)
        self.mykonos.begin_initialization()
        # Multi-chip Sync requires two SYSREF pulses at least 17us apart.
        jesdcore.send_sysref_pulse()
        time.sleep(0.001) # 17us... ish.
        jesdcore.send_sysref_pulse()
        async_exec(self.mykonos, "finish_initialization")
        # TODO:can we call this after JESD?
        self.init_rf_cal(args)
        self.log.trace("Starting JESD204b Link Initialization...")
        # Generally, enable the source before the sink. Start with the DAC side.
        self.log.trace("Starting FPGA framer...")
        jesdcore.init_framer()
        self.log.trace("Starting Mykonos deframer...")
        self.mykonos.start_jesd_rx()
        # Now for the ADC link. Note that the Mykonos framer will not start issuing CGS
        # characters until SYSREF is received by the framer. Therefore we enable the
        # framer in Mykonos and the FPGA, send a SYSREF pulse to everyone, and then
        # start the deframer in the FPGA.
        self.log.trace("Starting Mykonos framer...")
        self.mykonos.start_jesd_tx()
        jesdcore.enable_lmfc(True)
        jesdcore.send_sysref_pulse()
        # Allow a bit of time for SYSREF to reach Mykonos and then CGS to appear. In
        # several experiments this time requirement was only in the 100s of nanoseconds.
        time.sleep(0.001)
        self.log.trace("Starting FPGA deframer...")
        jesdcore.init_deframer()

        # Allow a bit of time for CGS/ILA to complete.
        time.sleep(0.100)
        error_flag = False
        if not jesdcore.get_framer_status():
            self.log.error("JESD204b FPGA Core Framer is not synced!")
            error_flag = True
        if not self.check_mykonos_deframer_status():
            self.log.error("Mykonos JESD204b Deframer is not synced!")
            error_flag = True
        if not jesdcore.get_deframer_status():
            self.log.error("JESD204b FPGA Core Deframer is not synced!")
            error_flag = True
        if not self.check_mykonos_framer_status():
            self.log.error("Mykonos JESD204b Framer is not synced!")
            error_flag = True
        if (self.mykonos.get_multichip_sync_status() & 0xB) != 0xB:
            self.log.error("Mykonos Multi-chip Sync failed!")
            error_flag = True
        if error_flag:
            raise RuntimeError('JESD204B Link Initialization Failed. See MPM logs for details.')
        self.log.debug("JESD204B Link Initialization & Training Complete")

    def check_mykonos_framer_status(self):
        " Return True if Mykonos Framer is in good state "
        rb = self.mykonos.get_framer_status()
        self.log.trace("Mykonos Framer Status Register: 0x{:04X}".format(rb & 0xFF))
        tx_state =   {0: 'CGS',
                      1: 'ILAS',
                      2: 'ADC Data'}[rb & 0b11]
        ilas_state = {0: 'CGS',
                      1: '1st Multframe',
                      2: '2nd Multframe',
                      3: '3rd Multframe',
                      4: '4th Multframe',
                      5: 'Last Multframe',
                      6: 'invalid state',
                      7: 'ILAS Complete'}[(rb & 0b11100) >> 2]
        sysref_rx =              (rb & (0b1 << 5)) > 0
        fifo_ptr_delta_changed = (rb & (0b1 << 6)) > 0
        sysref_phase_error =     (rb & (0b1 << 7)) > 0
        # According to emails with ADI, fifo_ptr_delta_changed may be buggy.
        # Deterministic latency is still achieved even when this bit is toggled, so
        # ADI's recommendation is to ignore it. The expected state of this bit 0, but
        # occasionally it toggles to 1. It is unclear why exactly this happens.
        success = ((tx_state == 'ADC Data') &
                   (ilas_state == 'ILAS Complete') &
                   sysref_rx &
                   (not sysref_phase_error))
        logger = self.log.trace if success else self.log.warning
        logger("Mykonos Framer, TX State: %s", tx_state)
        logger("Mykonos Framer, ILAS State: %s", ilas_state)
        logger("Mykonos Framer, SYSREF Received: {}".format(sysref_rx))
        logger("Mykonos Framer, FIFO Ptr Delta Change: {} (ignored, possibly buggy)".format(fifo_ptr_delta_changed))
        logger("Mykonos Framer, SYSREF Phase Error: {}".format(sysref_phase_error))
        return success

    def check_mykonos_deframer_status(self):
        " Return True if Mykonos Deframer is in good state "
        rb = self.mykonos.get_deframer_status()
        self.log.trace("Mykonos Deframer Status Register: 0x{:04X}".format(rb & 0xFF))

        frame_symbol_error =  (rb & (0b1 << 0)) > 0
        ilas_multifrm_error = (rb & (0b1 << 1)) > 0
        ilas_framing_error =  (rb & (0b1 << 2)) > 0
        ilas_checksum_valid = (rb & (0b1 << 3)) > 0
        prbs_error =          (rb & (0b1 << 4)) > 0
        sysref_received =     (rb & (0b1 << 5)) > 0
        deframer_irq =        (rb & (0b1 << 6)) > 0
        success = ((not frame_symbol_error) &
                   (not ilas_multifrm_error) &
                   (not ilas_framing_error) &
                   ilas_checksum_valid &
                   (not prbs_error) &
                   sysref_received &
                   (not deframer_irq))
        logger = self.log.trace if success else self.log.warning
        logger("Mykonos Deframer, Frame Symbol Error: {}".format(frame_symbol_error))
        logger("Mykonos Deframer, ILAS Multiframe Error: {}".format(ilas_multifrm_error))
        logger("Mykonos Deframer, ILAS Frame Error: {}".format(ilas_framing_error))
        logger("Mykonos Deframer, ILAS Checksum Valid: {}".format(ilas_checksum_valid))
        logger("Mykonos Deframer, PRBS Error: {}".format(prbs_error))
        logger("Mykonos Deframer, SYSREF Received: {}".format(sysref_received))
        logger("Mykonos Deframer, Deframer IRQ Received: {}".format(deframer_irq))
        return success

    def set_jesd_rate(self, jesdcore, new_rate, force=False):
        """
        Make the QPLL and GTX changes required to change the JESD204B core rate.
        """
        # The core is directly compiled for 125 MHz sample rate, which
        # corresponds to a lane rate of 2.5 Gbps. The same QPLL and GTX settings apply
        # for the 122.88 MHz sample rate.
        #
        # The higher LTE rate, 153.6 MHz, requires changes to the default configuration
        # of the MGT components. This function performs the required changes in the
        # following order (as recommended by UG476).
        #
        # 1) Modify any QPLL settings.
        # 2) Perform the QPLL reset routine by pulsing reset then waiting for lock.
        # 3) Modify any GTX settings.
        # 4) Perform the GTX reset routine by pulsing reset and waiting for reset done.

        assert new_rate in (2457.6e6, 2500e6, 3072e6)

        # On first run, we have no idea how the FPGA is configured... so let's force an
        # update to our rate.
        force = force or (self.current_jesd_rate is None)

        skip_drp = False
        if not force:
            #           Current     New       Skip?
            skip_drp = {2457.6e6 : {2457.6e6: True,  2500.0e6: True,  3072.0e6:False},
                        2500.0e6 : {2457.6e6: True,  2500.0e6: True,  3072.0e6:False},
                        3072.0e6 : {2457.6e6: False, 2500.0e6: False, 3072.0e6:True}}[self.current_jesd_rate][new_rate]

        if skip_drp:
            self.log.trace("Current lane rate is compatible with the new rate. Skipping "
                           "reconfiguration.")

        # These are the only registers in the QPLL and GTX that change based on the
        # selected line rate. The MGT wizard IP was generated for each of the rates and
        # reference clock frequencies and then diffed to create this table.
        QPLL_CFG         = {2457.6e6: 0x680181, 2500e6: 0x680181, 3072e6: 0x06801C1}[new_rate]
        QPLL_FBDIV       = {2457.6e6:    0x120, 2500e6:    0x120, 3072e6:      0x80}[new_rate]
        MGT_PMA_RSV      = {2457.6e6: 0x1E7080, 2500e6: 0x1E7080, 3072e6:   0x18480}[new_rate]
        MGT_RX_CLK25_DIV = {2457.6e6:        5, 2500e6:        5, 3072e6:         7}[new_rate]
        MGT_TX_CLK25_DIV = {2457.6e6:        5, 2500e6:        5, 3072e6:         7}[new_rate]
        MGT_RXOUT_DIV    = {2457.6e6:        4, 2500e6:        4, 3072e6:         2}[new_rate]
        MGT_TXOUT_DIV    = {2457.6e6:        4, 2500e6:        4, 3072e6:         2}[new_rate]
        MGT_RXCDR_CFG    = {2457.6e6:0x03000023ff10100020, 2500e6:0x03000023ff10100020, 3072e6:0x03000023ff10200020}[new_rate]


        # 1-2) Do the QPLL first
        if not skip_drp:
            self.log.trace("Changing QPLL settings to support {} Gbps".format(new_rate/1e9))
            jesdcore.set_drp_target('qpll', 0)
            # QPLL_CONFIG is spread across two regs: 0x32 (dedicated) and 0x33 (shared)
            reg_x32 = QPLL_CFG & 0xFFFF # [16:0] -> [16:0]
            reg_x33 = jesdcore.drp_access(rd=True, addr=0x33)
            reg_x33 = (reg_x33 & 0xF800) | ((QPLL_CFG >> 16) & 0x7FF)  # [26:16] -> [11:0]
            jesdcore.drp_access(rd=False, addr=0x32, wr_data=reg_x32)
            jesdcore.drp_access(rd=False, addr=0x33, wr_data=reg_x33)
            # QPLL_FBDIV is shared with other settings in reg 0x36
            reg_x36 = jesdcore.drp_access(rd=True, addr=0x36)
            reg_x36 = (reg_x36 & 0xFC00) | (QPLL_FBDIV & 0x3FF)  # in bits [9:0]
            jesdcore.drp_access(rd=False, addr=0x36, wr_data=reg_x36)

        # Run the QPLL reset sequence and prep the MGTs for modification.
        jesdcore.init()

        # 3-4) And the 4 MGTs second
        if not skip_drp:
            self.log.trace("Changing MGT settings to support {} Gbps"
                           .format(new_rate/1e9))
            for lane in range(4):
                jesdcore.set_drp_target('mgt', lane)
                # MGT_PMA_RSV is split over 0x99 (LSBs) and 0x9A
                reg_x99 = MGT_PMA_RSV & 0xFFFF
                reg_x9a = (MGT_PMA_RSV >> 16) & 0xFFFF
                jesdcore.drp_access(rd=False, addr=0x99, wr_data=reg_x99)
                jesdcore.drp_access(rd=False, addr=0x9A, wr_data=reg_x9a)
                # MGT_RX_CLK25_DIV is embedded with others in 0x11. The
                # encoding for the DRP register value is one less than the
                # desired value.
                reg_x11 = jesdcore.drp_access(rd=True, addr=0x11)
                reg_x11 = (reg_x11 & 0xF83F) | \
                          ((MGT_RX_CLK25_DIV-1 & 0x1F) << 6) # [10:6]
                jesdcore.drp_access(rd=False, addr=0x11, wr_data=reg_x11)
                # MGT_TX_CLK25_DIV is embedded with others in 0x6A. The
                # encoding for the DRP register value is one less than the
                # desired value.
                reg_x6a = jesdcore.drp_access(rd=True, addr=0x6A)
                reg_x6a = (reg_x6a & 0xFFE0) | (MGT_TX_CLK25_DIV-1 & 0x1F) # [4:0]
                jesdcore.drp_access(rd=False, addr=0x6A, wr_data=reg_x6a)
                # MGT_RXCDR_CFG is split over 0xA8 (LSBs) through 0xAD
                for reg_num, reg_addr in enumerate(range(0xA8, 0xAE)):
                    reg_data = (MGT_RXCDR_CFG >> 16*reg_num) & 0xFFFF
                    jesdcore.drp_access(rd=False, addr=reg_addr, wr_data=reg_data)
                # MGT_RXOUT_DIV and MGT_TXOUT_DIV are embedded together in
                # 0x88. The encoding for the DRP register value is
                # drp_val=log2(attribute)
                reg_x88 = (int(math.log(MGT_RXOUT_DIV, 2)) & 0x7) | \
                         ((int(math.log(MGT_TXOUT_DIV, 2)) & 0x7) << 4) # RX=[2:0] TX=[6:4]
                jesdcore.drp_access(rd=False, addr=0x88, wr_data=reg_x88)
            self.log.trace("GTX settings changed to support {} Gbps"
                           .format(new_rate/1e9))
            jesdcore.disable_drp_target()

        self.log.trace("JESD204b Lane Rate set to {} Gbps!"
                       .format(new_rate/1e9))
        self.current_jesd_rate = new_rate
        return

    def get_user_eeprom_data(self):
        """
        Return a dict of blobs stored in the user data section of the EEPROM.
        """
        return {
            blob_id: self.eeprom_fs.get_blob(blob_id)
            for blob_id in iterkeys(self.eeprom_fs.entries)
        }

    def set_user_eeprom_data(self, eeprom_data):
        """
        Update the local EEPROM with the data from eeprom_data.

        The actual writing to EEPROM can take some time, and is thus kicked
        into a background task. Don't call set_user_eeprom_data() quickly in
        succession. Also, while the background task is running, reading the
        EEPROM is unavailable and MPM won't be able to reboot until it's
        completed.
        However, get_user_eeprom_data() will immediately return the correct
        data after this method returns.
        """
        for blob_id, blob in iteritems(eeprom_data):
            self.eeprom_fs.set_blob(blob_id, blob)
        self.log.trace("Writing EEPROM info to `{}'".format(self.eeprom_path))
        eeprom_offset = self.user_eeprom[self.rev]['offset']
        def _write_to_eeprom_task(path, offset, data, log):
            " Writer task: Actually write to file "
            # Note: This can be sped up by only writing sectors that actually
            # changed. To do so, this function would need to read out the
            # current state of the file, do some kind of diff, and then seek()
            # to the different sectors. When very large blobs are being
            # written, it doesn't actually help all that much, of course,
            # because in that case, we'd anyway be changing most of the EEPROM.
            with open(path, 'r+b') as eeprom_file:
                log.trace("Seeking forward to `{}'".format(offset))
                eeprom_file.seek(eeprom_offset)
                log.trace("Writing a total of {} bytes.".format(
                    len(self.eeprom_fs.buffer)))
                eeprom_file.write(data)
                log.trace("EEPROM write complete.")
        thread_id = "eeprom_writer_task_{}".format(self.slot_idx)
        if any([x.name == thread_id for x in threading.enumerate()]):
            # Should this be fatal?
            self.log.warn("Another EEPROM writer thread is already active!")
        writer_task = threading.Thread(
            target=_write_to_eeprom_task,
            args=(
                self.eeprom_path,
                eeprom_offset,
                self.eeprom_fs.buffer,
                self.log
            ),
            name=thread_id,
        )
        writer_task.start()
        # Now return and let the copy finish on its own. The thread will detach
        # and MPM won't terminate this process until the thread is complete.
        # This does not stop anyone from killing this process (and the thread)
        # while the EEPROM write is happening, though.

    def get_master_clock_rate(self):
        " Return master clock rate (== sampling rate) "
        return self.master_clock_rate

    def update_ref_clock_freq(self, freq):
        """
        Call this function if the frequency of the reference clock changes (the
        10, 20, 25 MHz one). Note: Won't actually re-run any settings.
        """
        assert freq in (10e6, 20e6, 25e6), \
                "Invalid ref clock frequency: {}".format(freq)
        self.log.trace("Changing ref clock frequency to %f MHz", freq/1e6)
        self.ref_clock_freq = freq


    ##########################################################################
    # Sensors
    ##########################################################################
    def get_ref_lock(self):
        """
        Returns True if the LMK reference is locked.

        Note: This does not return a sensor dict. The sensor API call is
        in the motherboard class.
        """
        if self.lmk is None:
            self.log.trace("LMK object not yet initialized, defaulting to " \
                           "no ref locked!")
            return False
        lmk_lock_status = self.lmk.check_plls_locked()
        self.log.trace("LMK lock status is: {}".format(lmk_lock_status))
        return lmk_lock_status

    def get_lowband_lo_lock(self, which):
        """
        Return LO lock status (Boolean!) of the lowband LOs. 'which' must be
        either 'tx' or 'rx'
        """
        assert which.lower() in ('tx', 'rx')
        return self.cpld.get_lo_lock_status(which.upper())

    def get_ad9371_lo_lock(self, which):
        """
        Return LO lock status (Boolean!) of the lowband LOs. 'which' must be
        either 'tx' or 'rx'
        """
        return self.mykonos.get_lo_locked(which.upper())

    def get_lowband_tx_lo_locked_sensor(self, chan):
        " TX lowband LO lock sensor "
        self.log.trace("Querying TX lowband LO lock status for chan %d...",
                       chan)
        lock_status = self.get_lowband_lo_lock('tx')
        return {
            'name': 'lowband_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_lowband_rx_lo_locked_sensor(self, chan):
        " RX lowband LO lock sensor "
        self.log.trace("Querying RX lowband LO lock status for chan %d...",
                       chan)
        lock_status = self.get_lowband_lo_lock('rx')
        return {
            'name': 'lowband_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_ad9371_tx_lo_locked_sensor(self, chan):
        " TX ad9371 LO lock sensor "
        self.log.trace("Querying TX AD9371 LO lock status for chan %d...", chan)
        lock_status = self.get_ad9371_lo_lock('tx')
        return {
            'name': 'ad9371_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_ad9371_rx_lo_locked_sensor(self, chan):
        " RX ad9371 LO lock sensor "
        self.log.trace("Querying RX AD9371 LO lock status for chan %d...", chan)
        lock_status = self.get_ad9371_lo_lock('tx')
        return {
            'name': 'ad9371_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }


    ##########################################################################
    # Debug
    ##########################################################################
    def cpld_peek(self, addr):
        """
        Debug for accessing the CPLD via the RPC shell.
        """
        return self.cpld.peek16(addr)

    def cpld_poke(self, addr, data):
        """
        Debug for accessing the CPLD via the RPC shell.
        """
        self.cpld.poke16(addr, data)
        return self.cpld.peek16(addr)

    def dump_jesd_core(self):
        " Debug method to dump all JESD core regs "
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            for i in range(0x2000, 0x2110, 0x10):
                print(("0x%04X " % i), end=' ')
                for j in range(0, 0x10, 0x4):
                    print(("%08X" % dboard_ctrl_regs.peek32(i + j)), end=' ')
                print("")

    def dbcore_peek(self, addr):
        """
        Debug for accessing the DB Core registers via the RPC shell.
        """
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            rd_data = dboard_ctrl_regs.peek32(addr)
            self.log.trace("DB Core Register 0x{:04X} response: 0x{:08X}".format(addr, rd_data))
            return rd_data

    def dbcore_poke(self, addr, data):
        """
        Debug for accessing the DB Core registers via the RPC shell.
        """
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            self.log.trace("Writing DB Core Register 0x{:04X} with 0x{:08X}...".format(addr, data))
            dboard_ctrl_regs.poke32(addr, data)
