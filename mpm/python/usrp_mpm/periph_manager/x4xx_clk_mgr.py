#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X400 Clocking Management

This module handles the analog clocks on the X4x0 motherboard. The clocking
architecture of the motherboard is spread out between a clocking auxiliary board,
which contains a GPS-displicined OCXO, but also connects an external reference
to the motherboard. It also houses a PLL for deriving a clock from the network
(eCPRI). The clocking aux board has its own control class (ClockingAuxBrdControl)
which also contains controls for the eCPRI PLL.

The motherboard itself has two main PLLs for clocking purposes: The Sample PLL
(also SPLL) is used to create all clocks used for RF-related purposes. It creates
the sample clock (a very fast clock, ~3 GHz) and the PLL reference clock (PRC)
which is used as a timebase for the daughterboard CPLD and a reference for the
LO synthesizers (50-64 MHz).

Its input is the base reference clock (BRC). This clock comes either from the
clocking aux board, which in turn can provide a reference from the OCXO (with or
without GPS-disciplining) or from the external reference input SMA port.
The BRC is typically 10-25 MHz.

The BRC can also come from the reference PLL (RPLL), when the clock source is
set to 'mboard'. The RPLL produces clocks that are consumed by the GTY banks
(for Ethernet and Aurora), but it can also generate a reference clock for
the SPLL. By default, its reference is a fixed 100 MHz clock, but it can also be
driven by the eCPRI PLL, which itself can be driven by a clock from the GTY banks,
which is the case if the clock source is set to 'nsync'.

The master clock rate (MCR) is not actually controlled in this module, but it
depends on the sample clock rate. It also depends on the RFDC settings, so it is
controlled in x4xx.py, which has access to both RFDC and X4xxClockMgr.

Block diagram (for more details, see the schematic)::

 ┌────────────────────────────────────────────────────────┐
 │ Clocking Aux Board                                     │
 │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐           │
 │  │ GPSDO  ├─> OCXO   │ │External│ │eCPRI/  │           │
 │  │        │ │        │ │        │ │nsync   │           │
 │  └────────┘ └────┬───┘ └───┬────┘ └────────┘           │
 │                  │         │                           │
 │                  │         │                           │
 │      ┌───────────v─────────v───┐   ┌───────────┐       │
 │      │                         │   │eCPRI PLL  │       │
 │      └┐          MUX          ┌┘   │LMK05318   │       │
 │       └─┐                   ┌─┘    │           │       │
 │         └─┬─────────────────┘      └──┬────────┘       │
 │           │                           │                │
 └───────────┼───────────────────────────┼────────────────┘
             │                           │
             │  ┌─────────────┐          │
          ┌──v──v┐            │          │
          │ MUX  │            │          │  ┌───── 100 MHz
          └──┬───┘            │          │  │
             │Base Ref. Clock │          │  │
     ┌───────v───────┐        │  ┌───────v──v──┐
     │ Sample PLL    │        └──┤Reference PLL│
     │ LMK04832      │           │LMK03328     │
     │               │           │             │
     │               │           │             │
     └──┬─────────┬──┘           └────┬────────┘
        │         │                   │
        │         │                   │
        v         v                   v
    Sample      PLL Reference        GTY Banks
    Clock       Clock


The code in this module controls the RPLL and SPLL as well as some of the muxing.
The eCPRI PLL is controlled from the ClockingAuxBrdControl class.
Most importantly, this class controls the sequencing of configuring clocks. This
means it won't only switch muxes and configure PLLs, but will also do things in
the right order, and put components in reset where necessary.
For this reason, it requires callbacks to reset RFDC and daughterboard clocks.
"""

from enum import Enum
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.sys_utils import i2c_dev
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev
from usrp_mpm.periph_manager.x4xx_periphs import MboardRegsControl
from usrp_mpm.periph_manager.x4xx_sample_pll import LMK04832X4xx
from usrp_mpm.periph_manager.x4xx_reference_pll import LMK03328X4xx
from usrp_mpm.periph_manager.x4xx_clk_aux import ClockingAuxBrdControl
from usrp_mpm.mpmutils import poll_with_timeout
from usrp_mpm.rpc_server import no_rpc

# this is not the frequency out of the GPSDO(GPS Lite, 20MHz) itself but
# the GPSDO on the CLKAUX board is used to fine tune the OCXO via EFC
# which is running at 10MHz
X400_GPSDO_OCXO_CLOCK_FREQ = 10e6
X400_RPLL_I2C_LABEL = 'rpll_i2c'
X400_DEFAULT_RPLL_REF_SOURCE = '100M_reliable_clk'
X400_DEFAULT_MGT_CLOCK_RATE = 156.25e6
X400_DEFAULT_INT_CLOCK_FREQ = 25e6

class X4xxClockMgr:
    """
    X4x0 Clocking Manager

    The clocking subsystem of X4x0 is very complex. This class is designed to
    capture all clocking-related logic specific to the X4x0.

    This class controls clock and time sources.
    """
    CLOCK_SOURCE_MBOARD = "mboard"
    CLOCK_SOURCE_INTERNAL = ClockingAuxBrdControl.SOURCE_INTERNAL
    CLOCK_SOURCE_EXTERNAL = ClockingAuxBrdControl.SOURCE_EXTERNAL
    CLOCK_SOURCE_GPSDO = ClockingAuxBrdControl.SOURCE_GPSDO
    CLOCK_SOURCE_NSYNC = ClockingAuxBrdControl.SOURCE_NSYNC

    TIME_SOURCE_INTERNAL = "internal"
    TIME_SOURCE_EXTERNAL = "external"
    TIME_SOURCE_GPSDO = "gpsdo"
    TIME_SOURCE_QSFP0 = "qsfp0"

    # All valid sync_sources for X4xx in the form of (clock_source, time_source)
    valid_sync_sources = {
        (CLOCK_SOURCE_MBOARD, TIME_SOURCE_INTERNAL),
        (CLOCK_SOURCE_INTERNAL, TIME_SOURCE_INTERNAL),
        (CLOCK_SOURCE_EXTERNAL, TIME_SOURCE_EXTERNAL),
        (CLOCK_SOURCE_EXTERNAL, TIME_SOURCE_INTERNAL),
        (CLOCK_SOURCE_GPSDO, TIME_SOURCE_GPSDO),
        (CLOCK_SOURCE_GPSDO, TIME_SOURCE_INTERNAL),
        (CLOCK_SOURCE_NSYNC, TIME_SOURCE_INTERNAL),
    }

    class SetSyncRetVal(Enum):
        OK = 'OK'
        NOP = 'nop'
        FAIL = 'failure'

    def __init__(self,
                 clock_source,
                 time_source,
                 ref_clock_freq,
                 sample_clock_freq,
                 is_legacy_mode,
                 clk_aux_board,
                 cpld_control,
                 log):
        # Store parent objects
        self.log = log.getChild("ClkMgr")
        self._cpld_control = cpld_control
        self._clocking_auxbrd = clk_aux_board
        self._time_source = time_source
        self._clock_source = clock_source
        self._int_clock_freq = X400_DEFAULT_INT_CLOCK_FREQ
        self._ext_clock_freq = ref_clock_freq
        # Preallocate other objects to satisfy linter
        self.mboard_regs_control = None
        self._sample_pll = None
        self._reference_pll = None
        self._rpll_i2c_bus = None
        self._base_ref_clk_select = None
        self._set_reset_rfdc = lambda **kwargs: None
        self._set_reset_db_clocks = lambda *args: None
        self._rpll_reference_sources = {}
        # Init peripherals
        self._init_available_srcs()
        self._init_clk_peripherals()
        # Now initialize the clocks themselves
        self._init_ref_clock_and_time(
            clock_source,
            ref_clock_freq,
            sample_clock_freq,
            is_legacy_mode,
        )
        self._init_meas_clock()
        self._cpld_control.enable_pll_ref_clk()

    ###########################################################################
    # Initialization code
    ###########################################################################
    def _init_available_srcs(self):
        """
        Initialize the available clock and time sources.
        """
        has_gps = self._clocking_auxbrd and self._clocking_auxbrd.is_gps_supported()
        self._avail_clk_sources = [self.CLOCK_SOURCE_MBOARD]
        if self._clocking_auxbrd:
            self._avail_clk_sources.extend([
                self.CLOCK_SOURCE_INTERNAL,
                self.CLOCK_SOURCE_EXTERNAL])
            if self._clocking_auxbrd.is_nsync_supported():
                self._avail_clk_sources.append(self.CLOCK_SOURCE_NSYNC)
            if has_gps:
                self._avail_clk_sources.append(self.CLOCK_SOURCE_GPSDO)
        self.log.trace(f"Available clock sources are: {self._avail_clk_sources}")
        self._avail_time_sources = [
            self.TIME_SOURCE_INTERNAL, self.TIME_SOURCE_EXTERNAL, self.TIME_SOURCE_QSFP0]
        if has_gps:
            self._avail_time_sources.append(self.TIME_SOURCE_GPSDO)
        self.log.trace("Available time sources are: {}".format(self._avail_time_sources))

    def _init_clk_peripherals(self):
        """
        Initialize objects for peripherals owned by this class. Most importantly,
        this includes the RPLL and SPLL control classes.
        """
        # Create SPI and I2C interfaces to the LMK registers
        spll_spi_node = dt_symbol_get_spidev('spll')
        sample_lmk_regs_iface = lib.spi.make_spidev_regs_iface(
            spll_spi_node,
            1000000,    # Speed (Hz)
            0x3,        # SPI mode
            8,          # Addr shift
            0,          # Data shift
            1<<23,      # Read flag
            0,          # Write flag
        )
        # Initialize I2C connection to RPLL
        self._rpll_i2c_bus = i2c_dev.dt_symbol_get_i2c_bus(X400_RPLL_I2C_LABEL)
        if self._rpll_i2c_bus is None:
            raise RuntimeError("RPLL I2C bus could not be found")
        reference_lmk_regs_iface = lib.i2c.make_i2cdev_regs_iface(
            self._rpll_i2c_bus,
            0x54,   # addr
            False,  # ten_bit_addr
            100,    # timeout_ms
            1       # reg_addr_size
        )
        self._sample_pll = LMK04832X4xx(sample_lmk_regs_iface, self.log)
        self._reference_pll = LMK03328X4xx(reference_lmk_regs_iface, self.log)
        # Init BRC select GPIO control
        self._base_ref_clk_select = Gpio('BASE-REFERENCE-CLOCK-SELECT', Gpio.OUTPUT, 1)

    def _init_ref_clock_and_time(self,
                                 clock_source,
                                 ref_clock_freq,
                                 sample_clock_freq,
                                 is_legacy_mode,
                                 ):
        """
        Initialize clock and time sources. After this function returns, the
        reference signals going to the FPGA are valid.

        This is only called once, during __init__(). Calling it again will set
        clocks to defaults, but is also redundant since clocks do not need to be
        initialized twice.
        """
        # A dictionary of tuples (source #, rate) corresponding to each
        # available RPLL reference source.
        # source # 1 => PRIREF source
        # source # 2 => SECREF source
        self._rpll_reference_sources = {X400_DEFAULT_RPLL_REF_SOURCE: (2, 100e6)}
        reference_rates = [None, None]
        for source, rate in self._rpll_reference_sources.values():
            reference_rates[source-1] = rate
        self._reference_pll.reference_rates = reference_rates
        # Now initializes and reconfigure all clocks.
        # If clock_source and ref_clock_freq are not provided, they will not be changed.
        # If any other parameters are not provided, they will be configured with
        # default values.
        self._reset_clocks(value=True, reset_list=['cpld'])
        if clock_source is not None:
            self._set_brc_source(clock_source)
        if ref_clock_freq is not None:
            self._set_ref_clock_freq(ref_clock_freq)
        self._config_rpll(
            X400_DEFAULT_MGT_CLOCK_RATE,
            X400_DEFAULT_INT_CLOCK_FREQ,
            X400_DEFAULT_RPLL_REF_SOURCE)
        self._config_spll(sample_clock_freq, is_legacy_mode)
        self._reset_clocks(value=False, reset_list=['cpld'])

    def _init_meas_clock(self):
        """
        Initialize the TDC measurement clock. After this function returns, the
        FPGA TDC meas_clock is valid.
        """
        # This may or may not be used for X400. Leave as a place holder
        self.log.debug("TDC measurement clock not yet implemented.")

    ###########################################################################
    # Public APIs (that are not exposed as MPM calls)
    ###########################################################################
    @no_rpc
    def set_rfdc_reset_cb(self, rfdc_reset_cb):
        """
        Set reference to RFDC control. Ideally, we'd get that in __init__(), but
        due to order of operations, it's not ready yet when we call that.
        """
        self._set_reset_rfdc = rfdc_reset_cb

    @no_rpc
    def set_dboard_reset_cb(self, db_reset_cb):
        """
        Set reference to RFDC control. Ideally, we'd get that in __init__(), but
        due to order of operations, it's not ready yet when we call that.
        """
        self._set_reset_db_clocks = db_reset_cb

    @no_rpc
    def unset_cbs(self):
        """
        Removes any stored references to our owning X4xx class instance
        """
        self._set_reset_rfdc = None
        self._set_reset_db_clocks = None

    @no_rpc
    def config_pps_to_timekeeper(self, master_clock_rate):
        """ Configures the path from the PPS to the timekeeper"""
        pps_source = "internal_pps" \
            if self._time_source == self.TIME_SOURCE_INTERNAL \
            else "external_pps"
        self._sync_spll_clocks(pps_source)
        self._configure_pps_forwarding(True, master_clock_rate)

    @no_rpc
    def get_clock_sources(self):
        """
        Lists all available clock sources.
        """
        return self._avail_clk_sources

    @no_rpc
    def get_time_sources(self):
        " Returns list of valid time sources "
        return self._avail_time_sources

    @no_rpc
    def get_ref_clock_freq(self):
        " Returns the currently active reference clock frequency (BRC) "
        clock_source = self.get_clock_source()
        if clock_source == self.CLOCK_SOURCE_MBOARD:
            return self._int_clock_freq
        if clock_source == self.CLOCK_SOURCE_GPSDO:
            return X400_GPSDO_OCXO_CLOCK_FREQ
        # clock_source == "external":
        return self._ext_clock_freq

    @no_rpc
    def get_ref_locked(self):
        """
        Return lock status both RPLL and SPLL.
        """
        ref_pll_status = self._reference_pll.get_status()
        sample_pll_status = self._sample_pll.get_status()
        return all([
            ref_pll_status['PLL1 lock'],
            ref_pll_status['PLL2 lock'],
            sample_pll_status['PLL1 lock'],
            sample_pll_status['PLL2 lock'],
        ])

    @no_rpc
    def set_spll_rate(self, sample_clock_freq, is_legacy_mode):
        """
        Safely set the output rate of the sample PLL.

        This will do the required resets.
        """
        self._reset_clocks(value=True, reset_list=('rfdc', 'cpld', 'db_clock'))
        self._config_spll(sample_clock_freq, is_legacy_mode)
        self._reset_clocks(value=False, reset_list=('rfdc', 'cpld', 'db_clock'))

    @no_rpc
    def set_sync_source(self, clock_source, time_source):
        """
        Selects reference clock and PPS sources. Unconditionally re-applies the
        time source to ensure continuity between the reference clock and time
        rates.
        Note that if we change the source such that the time source is changed
        to 'external', then we need to also disable exporting the reference
        clock (RefOut and PPS-In are the same SMA connector).
        """
        assert (clock_source, time_source) in self.valid_sync_sources, \
            f'Clock and time source pair ({clock_source}, {time_source}) is ' \
            'not a valid selection'
        # Now see if we can keep the current settings, or if we need to run an
        # update of sync sources:
        if (clock_source == self._clock_source) and (time_source == self._time_source):
            spll_status = self._sample_pll.get_status()
            rpll_status = self._reference_pll.get_status()
            if (spll_status['PLL1 lock'] and spll_status['PLL2 lock'] and
                    rpll_status['PLL1 lock'] and rpll_status['PLL2 lock']):
                # Nothing change no need to do anything
                self.log.trace("New sync source assignment matches "
                               "previous assignment. Ignoring update command.")
                return self.SetSyncRetVal.NOP
            self.log.debug(
                "Although the clock source has not changed, some PLLs "
                "are not locked. Setting clock source again...")
            self.log.trace("- SPLL status: {}".format(spll_status))
            self.log.trace("- RPLL status: {}".format(rpll_status))
        # Start setting sync source
        self.log.debug(
            f"Setting sync source to time_source={time_source}, "
            f"clock_source={clock_source}")
        self._time_source = time_source
        # Reset downstream clocks (excluding RPLL)
        self._reset_clocks(value=True, reset_list=('db_clock', 'cpld', 'rfdc', 'spll'))
        self._set_brc_source(clock_source)
        return self.SetSyncRetVal.OK

    @no_rpc
    def set_clock_source_out(self, enable=True):
        """
        Allows routing the clock configured as source on the clk aux board to
        the RefOut terminal. This only applies to internal, gpsdo and nsync.
        """
        clock_source = self.get_clock_source()
        if self.get_time_source() == self.TIME_SOURCE_EXTERNAL:
            raise RuntimeError(
                'Cannot export clock when using external time reference!')
        if clock_source not in self._clocking_auxbrd.VALID_CLK_EXPORTS:
            raise RuntimeError(f"Invalid source to export: `{clock_source}'")
        if self._clocking_auxbrd is None:
            raise RuntimeError("No clocking aux board available")
        return self._clocking_auxbrd.export_clock(enable)

    ###########################################################################
    # Top-level BIST APIs
    #
    # These calls will be available as MPM calls. They are only needed by BIST.
    ###########################################################################
    def enable_ecpri_clocks(self, enable=True, clock='both'):
        """
        Enable or disable the export of FABRIC and GTY_RCV eCPRI
        clocks. Main use case until we support eCPRI is manufacturing
        testing.
        """
        self.mboard_regs_control.enable_ecpri_clocks(enable, clock)

    def nsync_change_input_source(self, source):
        """
        Switches the input reference source of the clkaux lmk (the "eCPRI PLL").

        Valid options are: fabric_clk, gty_rcv_clk, and sec_ref.

        fabric_clk and gty_rcv_clk are clock sources from the mboard.
        They are both inputs to the primary reference source of the clkaux lmk.
        sec_ref is the default reference select for the clkaux lmk, it has
        two inputs: Ref in or internal and GPS mode

        Only a public API for the BIST.
        """
        assert source in (
            self._clocking_auxbrd.SOURCE_NSYNC_LMK_PRI_FABRIC_CLK,
            self._clocking_auxbrd.SOURCE_NSYNC_LMK_PRI_GTY_RCV_CLK,
            self._clocking_auxbrd.NSYNC_SEC_REF,
        )
        if source == self._clocking_auxbrd.SOURCE_NSYNC_LMK_PRI_FABRIC_CLK:
            self.enable_ecpri_clocks(True, 'fabric')
            self._clocking_auxbrd.set_nsync_ref_select(
                self._clocking_auxbrd.NSYNC_PRI_REF)
            self._clocking_auxbrd.set_nsync_pri_ref_source(source)
        elif source == self._clocking_auxbrd.SOURCE_NSYNC_LMK_PRI_GTY_RCV_CLK:
            self.enable_ecpri_clocks(True, 'gty_rcv')
            self._clocking_auxbrd.set_nsync_ref_select(
                self._clocking_auxbrd.NSYNC_PRI_REF)
            self._clocking_auxbrd.set_nsync_pri_ref_source(source)
        else:
            self._clocking_auxbrd.set_nsync_ref_select(
                self._clocking_auxbrd.NSYNC_SEC_REF)

    def config_rpll_to_nsync(self):
        """
        Configures the rpll to use the LMK28PRIRefClk output
        by the clkaux LMK
        """
        # LMK28PRIRefClk only available when nsync is source, as lmk
        # is powered off otherwise
        self.set_sync_source(clock_source='nsync', time_source=self._time_source)

        # Add LMK28PRIRefClk as an available RPLL reference source
        # 1 => PRIREF source; source is output at 25 MHz
        # TODO: enable out4 on LMK
        previous_ref_rate = self._reference_pll.reference_rates[0]
        self._rpll_reference_sources['clkaux_nsync_clk'] = (1, 25e6)
        self._reference_pll.reference_rates[0] = 25e6
        self._config_rpll(X400_DEFAULT_MGT_CLOCK_RATE,
                          X400_DEFAULT_INT_CLOCK_FREQ,
                          'clkaux_nsync_clk')

        # remove clkaux_nsync_clk as a valid reference source for later calls
        # to _config_rpll(), it is only valid in this configuration
        self._reference_pll.reference_rates[0] = previous_ref_rate
        del self._rpll_reference_sources['clkaux_nsync_clk']

    def get_fpga_aux_ref_freq(self):
        """
        Return the tick count of an FPGA counter which measures the width of
        the PPS signal on the FPGA_AUX_REF FPGA input using a 40 MHz clock.
        Main use case until we support eCPRI is manufacturing testing.
        A return value of 0 indicates absence of a valid PPS signal on the
        FPGA_AUX_REF line.
        """
        return self.mboard_regs_control.get_fpga_aux_ref_freq()

    ###########################################################################
    # Top-level APIs
    #
    # These calls will be available as MPM calls
    ###########################################################################
    def get_clock_source(self):
        " Return the currently selected clock source "
        return self._clock_source

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source

    def get_spll_freq(self):
        """ Returns the output frequency setting of the SPLL """
        return self._sample_pll.output_freq

    def get_prc_rate(self):
        """
        Returns the rate of the PLL Reference Clock (PRC) which is
        routed to the daughterboards.
        Note: The ref clock will change if the sample clock frequency
        is modified.
        """
        prc_clock_map = {
            2.94912e9:  61.44e6,
            3e9:        62.5e6,
            # 3e9:      50e6, RF Legacy mode will be checked separately
            3.072e9:    64e6,
        }

        # RF Legacy Mode always has a PRC rate of 50 MHz
        if self._sample_pll.is_legacy_mode:
            return 50e6
        # else:
        return prc_clock_map[self.get_spll_freq()]

    def set_ref_clk_tuning_word(self, tuning_word, out_select=0):
        """
        Set the tuning word for the clocking aux board DAC. This wull update the
        tuning word used by the DAC.
        """
        if self._clocking_auxbrd is not None:
            self._clocking_auxbrd.config_dac(tuning_word, out_select)
        else:
            raise RuntimeError("No clocking aux board available")

    def get_ref_clk_tuning_word(self, out_select=0):
        """
        Get the tuning word configured for the clocking aux board DAC.
        """
        if self._clocking_auxbrd is None:
            raise RuntimeError("No clocking aux board available")
        return self._clocking_auxbrd.read_dac(out_select)

    def store_ref_clk_tuning_word(self, tuning_word):
        """
        Store the given tuning word in the clocking aux board ID EEPROM.
        """
        if self._clocking_auxbrd is not None:
            self._clocking_auxbrd.store_tuning_word(tuning_word)
        else:
            raise RuntimeError("No clocking aux board available")

    def get_sync_sources(self):
        """
        Enumerates permissible sync sources.
        """
        return [{
            "time_source": time_source,
            "clock_source": clock_source
        } for (clock_source, time_source) in self.valid_sync_sources]


    ###########################################################################
    # Low-level controls
    ###########################################################################
    def _reset_clocks(self, value, reset_list):
        """
        Shuts down all clocks downstream to upstream or clears reset on all
        clocks upstream to downstream. Specify the list of clocks to reset in
        reset_list. The order of clocks specified in the reset_list does not
        affect the order in which the clocks are reset.
        """
        if value:
            self.log.trace("Reset clocks: {}".format(reset_list))
            if 'db_clock' in reset_list:
                self._set_reset_db_clocks(value)
            if 'cpld' in reset_list:
                self._cpld_control.enable_pll_ref_clk(enable=False)
            if 'rfdc' in reset_list:
                self._set_reset_rfdc(reset=True)
            if 'spll' in reset_list:
                self._sample_pll.reset(value, hard=True)
            if 'rpll' in reset_list:
                self._reference_pll.reset(value, hard=True)
        else:
            self.log.trace("Bring clocks out of reset: {}".format(reset_list))
            if 'rpll' in reset_list:
                self._reference_pll.reset(value, hard=True)
            if 'spll' in reset_list:
                self._sample_pll.reset(value, hard=True)
            if 'rfdc' in reset_list:
                self._set_reset_rfdc(reset=False)
            if 'cpld' in reset_list:
                self._cpld_control.enable_pll_ref_clk(enable=True)
            if 'db_clock' in reset_list:
                self._set_reset_db_clocks(value)

    def _config_rpll(self, usr_clk_rate, internal_brc_rate, internal_brc_source):
        """
        Configures the LMK03328 to generate the desired MGT reference clocks
        and internal BRC rate.

        Currently, the MGT protocol selection is not supported, but a custom
        usr_clk_rate can be generated from PLL1.

        usr_clk_rate - the custom clock rate to generate from PLL1
        internal_brc_rate - the rate to output as the BRC
        internal_brc_source - the reference source which drives the RPLL
        """
        if internal_brc_source not in self._rpll_reference_sources:
            self.log.error('Invalid internal BRC source of {} was selected.'
                           .format(internal_brc_source))
            raise RuntimeError('Invalid internal BRC source of {} was selected.'
                               .format(internal_brc_source))
        ref_select = self._rpll_reference_sources[internal_brc_source][0]

        # If the desired rate matches the rate of the primary reference source,
        # directly passthrough that reference source
        if internal_brc_rate == self._reference_pll.reference_rates[0]:
            brc_select = 'bypass'
        else:
            brc_select = 'PLL'
        self._reference_pll.init()
        self._reference_pll.config(
            ref_select, internal_brc_rate, usr_clk_rate, brc_select)
        # The internal BRC rate will only change when _config_rpll is called
        # with a new internal BRC rate
        self._int_clock_freq = internal_brc_rate

    def _config_spll(self, sample_clock_freq, is_legacy_mode):
        """
        Configures the SPLL for the specified master clock rate.
        """
        self._sample_pll.init()
        self._sample_pll.config(sample_clock_freq, self.get_ref_clock_freq(),
                                is_legacy_mode)

    def _set_brc_source(self, clock_source):
        """
        Switches the Base Reference Clock Source between internal, external,
        mboard, and gpsdo using the GPIO pin and clocking aux board control.
        internal is a clock source internal to the clocking aux board, but
        external to the motherboard.
        Should not be called outside of set_sync_source or _init_ref_clock_and_time
        without proper reset and reconfig of downstream clocks.
        """
        if clock_source == self.CLOCK_SOURCE_MBOARD:
            self._base_ref_clk_select.set(1)
            if self._clocking_auxbrd:
                self._clocking_auxbrd.export_clock(False)
        else:
            if self._clocking_auxbrd is None:
                self.log.error('Invalid BRC selection {}. No clocking aux '
                               'board was found.'.format(clock_source))
                raise RuntimeError('Invalid BRC selection {}'.format(clock_source))
            self._base_ref_clk_select.set(0)
            if clock_source == self.CLOCK_SOURCE_EXTERNAL:
                # This case is a bit special: We also need to tell the clocking
                # aux board if we plan to consume the external time reference or
                # not.
                time_src_board = \
                    ClockingAuxBrdControl.SOURCE_EXTERNAL \
                    if self._time_source == self.TIME_SOURCE_EXTERNAL \
                    else ClockingAuxBrdControl.SOURCE_INTERNAL
                self._clocking_auxbrd.set_source(
                    ClockingAuxBrdControl.SOURCE_EXTERNAL, time_src_board)
            elif clock_source == self.CLOCK_SOURCE_INTERNAL:
                self._clocking_auxbrd.set_source(ClockingAuxBrdControl.SOURCE_INTERNAL)
            elif clock_source == self.CLOCK_SOURCE_GPSDO:
                self._clocking_auxbrd.set_source(ClockingAuxBrdControl.SOURCE_GPSDO)
            elif clock_source == self.CLOCK_SOURCE_NSYNC:
                self._clocking_auxbrd.set_source(ClockingAuxBrdControl.SOURCE_NSYNC)
            else:
                self.log.error('Invalid BRC selection {}'.format(clock_source))
                raise RuntimeError('Invalid BRC selection {}'.format(clock_source))
        self._clock_source = clock_source
        self.log.debug(f"Base reference clock source is: {clock_source}")

    def _sync_spll_clocks(self, pps_source="internal_pps"):
        """
        Synchronize base reference clock (BRC) and PLL reference clock (PRC)
        at start of PPS trigger.

        Uses the LMK 04832 pll1_r_divider_sync to synchronize BRC with PRC.
        This sync method uses a callback to actual trigger the sync. Before
        the trigger is pulled (CLOCK_CTRL_PLL_SYNC_TRIGGER) PPS source is
        configured base on current reference clock and pps_source. After sync
        trigger the method waits for 1sec for the CLOCK_CTRL_PLL_SYNC_DONE
        to go high.

        :param pps_source: select whether internal ("internal_pps") or external
                           ("external_pps") PPS should be used. This parameter
                           is taken into account when the current clock source
                           is external. If the current clock source is set to
                           internal then this parameter is not taken into
                           account.
        :return:           success state of sync call
        :raises RuntimeError: for invalid combinations of reference clock and
                              pps_source
        """
        def select_pps():
            """
            Select PPS source based on current clock source and pps_source.

            This returns the bits for the motherboard CLOCK_CTRL register that
            control the PPS source.
            """
            EXT_PPS = "external_pps"
            INT_PPS = "internal_pps"
            PPS_SOURCES = (EXT_PPS, INT_PPS)
            assert pps_source in PPS_SOURCES, \
                "%s not in %s" % (pps_source, PPS_SOURCES)

            supported_configs = {
                (10E6, EXT_PPS): MboardRegsControl.CLOCK_CTRL_PPS_EXT,
                (10E6, INT_PPS): MboardRegsControl.CLOCK_CTRL_PPS_INT_10MHz,
                (25E6, INT_PPS): MboardRegsControl.CLOCK_CTRL_PPS_INT_25MHz
            }

            config = (self.get_ref_clock_freq(), pps_source)
            if config not in supported_configs:
                raise RuntimeError("Unsupported combination of reference clock "
                                   "(%.2E) and PPS source (%s) for PPS sync." %
                                   config)
            return supported_configs[config]

        return self._sample_pll.pll1_r_divider_sync(
            lambda: self.mboard_regs_control.pll_sync_trigger(select_pps()))

    def _configure_pps_forwarding(self, enable, master_clock_rate, delay=1.0):
        """
        Configures the PPS forwarding to the sample clock domain (master
        clock rate). This function assumes _sync_spll_clocks function has
        already been executed.

        :param enable: Boolean to choose whether PPS is forwarded to the
                       sample clock domain.

        :param delay:  Delay in seconds from the PPS rising edge to the edge
                       occurence in the application. This value has to be in
                       range 0 < x <= 1. In order to forward the PPS signal
                       from base reference clock to sample clock an aligned
                       rising edge of the clock is required. This can be
                       created by the _sync_spll_clocks function. Based on the
                       greatest common divisor of the two clock rates there
                       are multiple occurences of an aligned edge each second.
                       One of these aligned edges has to be chosen for the
                       PPS forwarding by setting this parameter.

        :return:       None, Exception on error
        """
        return self.mboard_regs_control.configure_pps_forwarding(
            enable, master_clock_rate, self.get_prc_rate(), delay)

    def _set_ref_clock_freq(self, freq):
        """
        Tell our USRP what the frequency of the external reference clock is.

        Will throw if it's not a valid value.
        """
        if (freq < 1e6) or (freq > 50e6):
            raise RuntimeError('External reference clock frequency is out of the valid range.')
        if (freq % 40e3) != 0:
            # TODO: implement exception of a 50e3 step size for 200MSPS
            raise RuntimeError('External reference clock frequency is of incorrect step size.')
        self._ext_clock_freq = freq
        # If the external source is currently selected we also need to re-apply the
        # time_source. This call also updates the dboards' rates.
        if self.get_clock_source() == self.CLOCK_SOURCE_EXTERNAL:
            self.set_sync_source(self._clock_source, self._time_source)
