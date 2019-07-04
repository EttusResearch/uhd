#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Rhodium dboard implementation module
"""

from __future__ import print_function
import os
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.dboard_manager.rh_periphs import TCA6408, FPGAtoDbGPIO, FPGAtoLoDist
from usrp_mpm.dboard_manager.rh_init import RhodiumInitManager
from usrp_mpm.dboard_manager.rh_periphs import RhCPLD
from usrp_mpm.dboard_manager.rh_periphs import DboardClockControl
from usrp_mpm.cores import nijesdcore
from usrp_mpm.dboard_manager.adc_rh import AD9695Rh
from usrp_mpm.dboard_manager.dac_rh import DAC37J82Rh
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import open_uio
from usrp_mpm.user_eeprom import BfrfsEEPROM


###############################################################################
# SPI Helpers
###############################################################################

def create_spidev_iface_lmk(dev_node):
    """
    Create a regs iface from a spidev node
    """
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        1000000, # Speed (Hz)
        0,       # SPI mode
        8,       # Addr shift
        0,       # Data shift
        1<<23,   # Read flag
        0        # Write flag
    )

def create_spidev_iface_cpld(dev_node):
    """
    Create a regs iface from a spidev node (CPLD register protocol)
    """
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        1000000, # Speed (Hz)
        0,       # SPI mode
        17,      # Addr shift
        0,       # Data shift
        1<<16,   # Read flag
        0        # Write flag
    )

def create_spidev_iface_cpld_gain_loader(dev_node):
    """
    Create a regs iface from a spidev node (CPLD gain table protocol)
    """
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        1000000, # Speed (Hz)
        0,       # SPI mode
        16,      # Addr shift
        4,       # Data shift
        0,       # Read flag
        1<<3     # Write flag
    )

def create_spidev_iface_phasedac(dev_node):
    """
    Create a regs iface from a spidev node (AD5683)

    The data shift for the SPI interface is defined based on the command
    operation defined in the AD5683 datasheet.
    Each SPI transaction is 24-bit: [23:20] -> command; [19:0] -> data
    The 4 LSBs are all don't cares (Xs), regardless of the DAC's resolution.
    Therefore, to simplify DAC writes, we compensate for all the don't care
    bits with the data shift parameter here (4), thus 16-bit data field.
    Special care must be taken when writing to the control register,
    since the 6-bit payload is placed in [19:14] of the SPI transaction,
    which is equivalent to bits [15:10] of our 16-bit data field.
    For futher details, please refer to the AD5683's datasheet.
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        1,       # SPI mode
        20,      # Addr shift
        4,       # Data shift
        0,       # Read flag (phase DAC is write-only)
        0,       # Write flag
    )

def create_spidev_iface_adc(dev_node):
    """
    Create a regs iface from a spidev node (AD9695)
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        0, # SPI mode
        8, # Addr shift
        0, # Data shift
        1<<23, # Read flag
        0, # Write flag
    )

def create_spidev_iface_dac(dev_node):
    """
    Create a regs iface from a spidev node (DAC37J82)
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        0,       # SPI mode
        16,      # Addr shift
        0,       # Data shift
        1<<23,   # Read flag
        0,       # Write flag
    )


###############################################################################
# Main dboard control class
###############################################################################

class Rhodium(BfrfsEEPROM, DboardManagerBase):
    """
    Holds all dboard specific information and methods of the Rhodium dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x152]
    #file system path to i2c-adapter/mux
    base_i2c_adapter = '/sys/class/i2c-adapter'
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {
        "cpld"             : 0,
        "cpld_gain_loader" : 0,
        "lmk"              : 1,
        "phase_dac"        : 2,
        "adc"              : 3,
        "dac"              : 4}
    ### End of overridables #################################################
    # Class-specific, but constant settings:
    spi_factories = {
        "cpld": create_spidev_iface_cpld,
        "cpld_gain_loader": create_spidev_iface_cpld_gain_loader,
        "lmk": create_spidev_iface_lmk,
        "phase_dac": create_spidev_iface_phasedac,
        "adc": create_spidev_iface_adc,
        "dac": create_spidev_iface_dac
    }
    # Map I2C channel to slot index
    i2c_chan_map = {0: 'i2c-9', 1: 'i2c-10'}
    user_eeprom = {
        0: { # dt-compat=0
            'label': "e0004000.i2c",
            'offset': 1024,
            'max_size': 32786 - 1024,
            'alignment': 1024,
        },
    }
    default_master_clock_rate = 245.76e6
    default_time_source = 'internal'
    default_current_jesd_rate = 4915.2e6

    # Provide a mapping of direction and pin number to
    # pin name and active state (0 = active-low) for
    # LO out ports
    lo_out_pin_map = {
        'RX' : [('RX_OUT0_CTRL', 0),
                ('RX_OUT1_CTRL', 1),
                ('RX_OUT2_CTRL', 0),
                ('RX_OUT3_CTRL', 1)],
        'TX' : [('TX_OUT0_CTRL', 0),
                ('TX_OUT1_CTRL', 1),
                ('TX_OUT2_CTRL', 0),
                ('TX_OUT3_CTRL', 1)]}

    # Provide mapping of direction to pin name for LO
    # in port
    lo_in_pin_map = {
        'RX' : 'RX_INSWITCH_CTRL',
        'TX' : 'TX_INSWITCH_CTRL'}

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        self.log = get_logger("Rhodium-{}".format(slot_idx))
        self.log.trace("Initializing Rhodium daughterboard, slot index %d",
                       self.slot_idx)
        self.rev = int(self.device_info['rev'])
        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        # This is a default ref clock freq, it must be updated before init() is
        # called!
        self.ref_clock_freq = None
        # These will get updated during init()
        self.master_clock_rate = None
        self.sampling_clock_rate = None
        self.current_jesd_rate = None
        # Predeclare some attributes to make linter happy:
        self.lmk = None
        self._port_expander = None
        self._lo_dist = None
        self.cpld = None
        # If _init_args is None, it means that init() hasn't yet been called.
        self._init_args = None
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
        def _get_i2c_dev():
            " Return the I2C path for this daughterboard "
            import pyudev
            context = pyudev.Context()
            i2c_dev_path = os.path.join(
                self.base_i2c_adapter,
                self.i2c_chan_map[self.slot_idx]
            )
            return pyudev.Devices.from_sys_path(context, i2c_dev_path)
        def _init_spi_devices():
            " Returns abstraction layers to all the SPI devices "
            self.log.trace("Loading SPI interfaces...")
            return {
                key: self.spi_factories[key](self._spi_nodes[key])
                for key in self._spi_nodes
            }
        self._port_expander = TCA6408(_get_i2c_dev())
        self._daughterboard_gpio = FPGAtoDbGPIO(self.slot_idx)
        if FPGAtoLoDist.lo_dist_present(_get_i2c_dev()):
            self.log.info("Enabling LO distribution board")
            self._lo_dist = FPGAtoLoDist(_get_i2c_dev())
        else:
            self.log.debug("No LO distribution board detected")
        self.log.debug("Turning on Module and RF power supplies")
        self._power_on()
        BfrfsEEPROM.__init__(self)
        self._spi_ifaces = _init_spi_devices()
        self.log.debug("Loaded SPI interfaces!")
        self.cpld = RhCPLD(self._spi_ifaces['cpld'], self.log)
        self.log.debug("Loaded CPLD interfaces!")
        # Create DAC interface (analog output is disabled).
        self.log.trace("Creating DAC control object...")
        self.dac = DAC37J82Rh(self.slot_idx, self._spi_ifaces['dac'], self.log)
        # Create ADC interface (JESD204B link is powered down).
        self.log.trace("Creating ADC control object...")
        self.adc = AD9695Rh(self.slot_idx, self._spi_ifaces['adc'], self.log)
        self.log.info("Successfully loaded all peripherals!")

    def _power_on(self):
        " Turn on power to daughterboard "
        self.log.trace("Powering on slot_idx={}...".format(self.slot_idx))
        self._daughterboard_gpio.set(FPGAtoDbGPIO.DB_POWER_ENABLE, 1)
        self._daughterboard_gpio.set(FPGAtoDbGPIO.RF_POWER_ENABLE, 1)
        # Check each power good signal

    def _power_off(self):
        " Turn off power to daughterboard "
        self.log.trace("Powering off slot_idx={}...".format(self.slot_idx))
        self._daughterboard_gpio.set(FPGAtoDbGPIO.DB_POWER_ENABLE, 0)
        self._daughterboard_gpio.set(FPGAtoDbGPIO.RF_POWER_ENABLE, 0)

    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """
        # Sanity checks and input validation:
        self.log.info("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        if not self._periphs_initialized:
            error_msg = "Cannot run init(), peripherals are not initialized!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        # Check if ref clock freq changed (would require a full init)
        ref_clk_freq_changed = False
        if 'ref_clk_freq' in args:
            new_ref_clock_freq = float(args['ref_clk_freq'])
            assert new_ref_clock_freq in (10e6, 20e6, 25e6)
            if new_ref_clock_freq != self.ref_clock_freq:
                self.ref_clock_freq = new_ref_clock_freq
                ref_clk_freq_changed = True
                self.log.debug(
                    "Updating reference clock frequency to {:.02f} MHz!"
                    .format(self.ref_clock_freq / 1e6)
                )
        assert self.ref_clock_freq is not None
        # Check if master clock freq changed (would require a full init)
        new_master_clock_rate = \
            float(args.get('master_clock_rate', self.default_master_clock_rate))
        assert new_master_clock_rate in (200e6, 245.76e6, 250e6), \
                "Invalid master clock rate: {:.02f} MHz".format(new_master_clock_rate / 1e6)
        master_clock_rate_changed = new_master_clock_rate != self.master_clock_rate
        if master_clock_rate_changed:
            self.master_clock_rate = new_master_clock_rate
            self.log.debug("Updating master clock rate to {:.02f} MHz!".format(
                self.master_clock_rate / 1e6
            ))
            # From the host's perspective (i.e. UHD), master_clock_rate is thought as
            # the data rate that the radio NoC block works on (200/245.76/250 MSPS).
            # For Rhodium, that rate is different from the RF sampling rate = JESD rate
            # (400/491.52/500 MHz). The FPGA has fixed half-band filters that decimate
            # and interpolate between the radio block and the JESD core.
            # Therefore, the board configuration through MPM relies on the sampling freq.,
            # so a sampling_clock_rate value is internally set based on the master_clock_rate
            # parameter given by the host.
            self.sampling_clock_rate = 2 * self.master_clock_rate
            self.log.trace("Updating sampling clock rate to {:.02f} MHz!".format(
                self.sampling_clock_rate / 1e6
            ))
        # Track if we're able to do a "fast reinit", which means there were no
        # major changes and can skip all slow initialization steps.
        fast_reinit = \
            not bool(args.get("force_reinit", False)) \
            and not master_clock_rate_changed \
            and not ref_clk_freq_changed
        if fast_reinit:
            self.log.debug("Attempting fast re-init with the following settings: "
                           "master_clock_rate={} MHz ref_clk_freq={} MHz"
                           .format(self.master_clock_rate / 1e6, self.ref_clock_freq / 1e6))
            init_result = True
        else:
            init_result = RhodiumInitManager(self, self._spi_ifaces).init(args)
        if init_result:
            self._init_args = args
        return init_result

    def enable_lo_export(self, direction, enable):
        """
        For N321 devices. If enable is true, connect the RX 1:4 splitter to the
        daughterboard LO export. If enable is false, connect the splitter to
        LO input port 1 instead.

        Asserts if there is no LO distribution board attached (e.g. device is
        not an N321, or this is the daughterboard in slot B)
        """

        assert self._lo_dist is not None
        assert direction in ('RX', 'TX')
        pin = self.lo_in_pin_map[direction]
        pin_val = 0 if enable else 1
        self.log.debug("LO Distribution: 1:4 splitter connected to {0} {1}".format(
            direction, {True: "DB export", False: "Input 0"}[enable]))
        self.log.trace("Net name: {0}, Pin value: {1}".format(pin, pin_val))
        self._lo_dist.set(pin, pin_val)

    def enable_lo_output(self, direction, port_number, enable):
        """
        For N321 devices. If enable is true, connect the RX 1:4 splitter to the
        daughterboard LO export. If enable is false, connect the splitter to
        LO input port 1 instead.

        Asserts if there is no LO distribution board attached (e.g. device is
        not an N321, or this is the daughterboard in slot B)
        """

        assert self._lo_dist is not None
        assert direction in ('RX', 'TX')
        assert port_number in (0, 1, 2, 3)
        pin_info = self.lo_out_pin_map[direction][port_number]
        # enable XNOR active_high = desired pinout value
        pin_val = 1 if not (enable ^ pin_info[1]) else 0
        self.log.debug("LO Distribution: {0} Out{1} is {2}".format(
            direction, port_number, {True: "active", False: "terminated"}[enable]))
        self.log.trace("Net name: {0}, Pin value: {1}".format(pin_info[0], pin_val))
        self._lo_dist.set(pin_info[0], pin_val)

    def is_lo_dist_present(self):
        """
        Returns true if this daughterboard has a LO distribution board
        attached and initialized, otherwise false.
        """
        return self._lo_dist is not None

    ##########################################################################
    # Clocking control APIs
    ##########################################################################

    def set_clk_safe_state(self):
        """
        Disable all components that could react badly to a sudden change in
        clocking. After calling this method, all clocks will be off. Calling
        _reinit() will turn them on again.
        """
        if self._init_args is None:
            # Then we're already in a safe state
            return
        # Put the ADC and the DAC in a safe state because they receive a LMK's clock.
        # The DAC37J82 datasheet only recommends disabling its analog output before
        # a clock is provided to the chip.
        self.dac.tx_enable(False)
        self.adc.power_down_channel(True)
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as radio_regs:
            # Clear the Sample Clock enables and place the MMCM in reset.
            db_clk_control = DboardClockControl(radio_regs, self.log)
            db_clk_control.reset_mmcm()
            # Place the JESD204b core in reset, mainly to reset QPLL/CPLLs.
            jesdcore = nijesdcore.NIJESDCore(radio_regs, self.slot_idx,
                                             **RhodiumInitManager.JESD_DEFAULT_ARGS)
            jesdcore.reset()
            # The reference clock is handled elsewhere since it is a motherboard-
            # level clock.

    def _reinit(self, master_clock_rate):
        """
        This will re-run init(). We store all the settings in _init_args, so we
        will bring the device into the same state that it was before, with the
        exception of frequency and gain. Those need to be re-set by UHD in order
        not to invalidate the UHD caches.
        """
        args = self._init_args
        args["master_clock_rate"] = master_clock_rate
        args["ref_clk_freq"] = self.ref_clock_freq
        # If we add API calls to reset the cals, they need to update
        # self._init_args
        self.master_clock_rate = None # <= This will force a re-init
        self.init(args)
        # self.master_clock_rate is now OK again

    def set_master_clock_rate(self, rate):
        """
        Set the master clock rate to rate. Note this will trigger a
        re-initialization of the entire clocking, unless rate matches the
        current master clock rate.
        """
        if rate == self.master_clock_rate:
            self.log.debug(
                "New master clock rate assignment matches previous assignment. "
                "Ignoring set_master_clock_rate() command.")
            return self.master_clock_rate
        self._reinit(rate)
        return rate

    def get_master_clock_rate(self):
        " Return master clock rate (== sampling rate / 2) "
        return self.master_clock_rate

    def update_ref_clock_freq(self, freq, **kwargs):
        """
        Call this function if the frequency of the reference clock changes
        (the 10, 20, 25 MHz one).

        If this function is called while the device is in an initialized state,
        it will also re-trigger the initialization sequence.

        No need to set the device in a safe state because (presumably) the user
        has already switched the clock rate externally. All we need to do now
        is re-initialize with the new rate.
        """
        assert freq in (10e6, 20e6, 25e6), \
                "Invalid ref clock frequency: {}".format(freq)
        self.log.trace("Changing ref clock frequency to %f MHz", freq/1e6)
        self.ref_clock_freq = freq
        if self._init_args is not None:
            self._init_args = {**self._init_args, **kwargs}
            self.log.info("Re-initializing daughter board. This may take some time.")
            self._reinit(self.master_clock_rate)
            self.log.debug("Daughter board re-initialization done.")

    def enable_tx_lowband_lo(self, enable):
        """
        Enables or disables the TX lowband LO output from the LMK on the
        daughterboard.
        """
        self.lmk.enable_tx_lb_lo(enable)

    def enable_rx_lowband_lo(self, enable):
        """
        Enables or disables the RX lowband LO output from the LMK on the
        daughterboard.
        """
        self.lmk.enable_rx_lb_lo(enable)


    ##########################################################################
    # Debug
    ##########################################################################

    def cpld_peek(self, addr):
        """
        Debug for accessing the CPLD via the RPC shell.
        """
        self.log.trace("CPLD Signature: 0x{:X}".format(self.cpld.peek16(0x00)))
        revision_msb = self.cpld.peek16(0x04)
        self.log.trace("CPLD Revision:  0x{:X}"
                       .format(self.cpld.peek16(0x03) | (revision_msb << 16)))
        return self.cpld.peek16(addr)

    def cpld_poke(self, addr, data):
        """
        Debug for accessing the CPLD via the RPC shell.
        """
        self.log.trace("CPLD Signature: 0x{:X}".format(self.cpld.peek16(0x00)))
        revision_msb = self.cpld.peek16(0x04)
        self.log.trace("CPLD Revision:  0x{:X}"
                       .format(self.cpld.peek16(0x03) | (revision_msb << 16)))
        self.cpld.poke16(addr, data)
        return self.cpld.peek16(addr)

    def lmk_peek(self, addr):
        """
        Debug for accessing the LMK via the RPC shell.
        """
        lmk_regs = self._spi_ifaces['lmk']
        self.log.trace("LMK Chip ID: 0x{:X}".format(lmk_regs.peek8(0x03)))
        return lmk_regs.peek8(addr)

    def lmk_poke(self, addr, data):
        """
        Debug for accessing the LMK via the RPC shell.
        """
        lmk_regs = self._spi_ifaces['lmk']
        self.log.trace("LMK Chip ID: 0x{:X}".format(lmk_regs.peek8(0x03)))
        lmk_regs.poke8(addr, data)
        return lmk_regs.peek8(addr)

    def pdac_poke(self, addr, data):
        """
        Debug for accessing the Phase DAC via the RPC shell.
        """
        pdac_regs = self._spi_ifaces['phase_dac']
        pdac_regs.poke16(addr, data)
        return

    def adc_peek(self, addr):
        """
        Debug for accessing the ADC via the RPC shell.
        """
        adc_regs = self._spi_ifaces['adc']
        self.log.trace("ADC Chip ID: 0x{:X}".format(adc_regs.peek8(0x04)))
        return adc_regs.peek8(addr)

    def adc_poke(self, addr, data):
        """
        Debug for accessing the ADC via the RPC shell
        """
        adc_regs = self._spi_ifaces['adc']
        self.log.trace("ADC Chip ID: 0x{:X}".format(adc_regs.peek8(0x04)))
        adc_regs.poke8(addr, data)
        return adc_regs.peek8(addr)

    def dump_jesd_core(self):
        """
        Debug for reading out all JESD core registers via RPC shell
        """
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as radio_regs:
            for i in range(0x2000, 0x2110, 0x10):
                print(("0x%04X " % i), end=' ')
                for j in range(0, 0x10, 0x4):
                    print(("%08X" % radio_regs.peek32(i + j)), end=' ')
                print("")
