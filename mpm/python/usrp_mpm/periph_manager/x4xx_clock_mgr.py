#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4x0 Motherboard Clocking Management

This module handles the clocking on the X4x0 motherboard and the RFSoC. The clocking
architecture of the motherboard is spread out between a clocking auxiliary board,
which contains a GPS-displicined OCXO, but also connects an external reference
to the motherboard. It also houses a PLL for deriving a clock from the network
(e.g., via eCPRI). The clocking aux board has its own control class which also
contains controls for the eCPRI PLL (ClockingAuxBrdControl).

The motherboard itself has two main PLLs for clocking purposes: The Sample PLL
(also SPLL) is used to create all clocks used for RF-related purposes. It creates
the sample clock (a very fast clock, ~3 GHz for X410/ZBX) and the PLL reference
clock (PRC) which is used as a timebase for the daughterboard CPLD and a
reference for the LO synthesizers (50-64 MHz).

The SPLL input is the base reference clock (BRC). This clock comes either from the
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
controlled in x4xx.py, which has access to both RFDC and X4xxClockMgr. The
corresponding clock is generated in the FPGA using the PRC as a reference clock,
using an MMCM.

For a block diagram, cf. usrp_x4xx.dox. For more details, see the schematic.
"""

from enum import Enum
from usrp_mpm.mpmutils import parse_multi_device_arg
from usrp_mpm.periph_manager.x4xx_clk_aux import ClockingAuxBrdControl
from usrp_mpm.periph_manager.x4xx_clock_types import RpllRefSel, BrcSource
from usrp_mpm.periph_manager.x4xx_clock_ctrl import X4xxClockCtrl
from usrp_mpm.rpc_server import no_rpc

class X4xxClockManager:
    """
    X4xx Clock Manager: This class handles all the clocking-related features of
    the X4xx. It relies on other classes:
    - X4xxClockCtrl: This class contains the actual knobs to twiddle, controls
                     for the various PLLs, etc.
    - X4xxClockPolicy: This is a device-specific class (e.g., it would be a
                       different class for X410 than something else). It
                       encodes all the various settings for the different
                       clocking settings.
    - X4xxRfdcCtrl: This is for controlling the RFDC. Unlike the first two
                    classes, this is passed in, and not exclusively owned by this
                    clock manager object.

    This class implements many of the public MPM APIs (meaning, some RPC calls
    made from UHD go directly into this class).

    The one clocking-related thing not controlled by this class is the
    behaviour of the TRIG-IO connector on the X4xx back panel (it is controlled
    in the main x4xx class).

    For more details on the clocking management, refer to the module docstring.
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

    X400_DEFAULT_TIME_SOURCE = TIME_SOURCE_INTERNAL
    X400_DEFAULT_CLOCK_SOURCE = CLOCK_SOURCE_INTERNAL

    # 25 MHz is the default BRC provided by the RPLL
    X400_DEFAULT_INT_CLOCK_FREQ = 25e6
    # With no other info, we assume that the user will be providing a 10 MHz
    # external reference clock
    X400_DEFAULT_EXT_CLOCK_FREQ = 10e6
    # this is not the frequency out of the GPSDO(GPS Lite, 20MHz) itself but
    # the GPSDO on the CLKAUX board is used to fine tune the OCXO via EFC
    # which is running at 10MHz
    X400_GPSDO_OCXO_CLOCK_FREQ = 10e6
    # This is the fixed oscillator used as a reliable input to the RPLL
    X400_RPLL_SECREF_RATE = 100e6
    X400_DEFAULT_MGT_CLOCK_RATE = 156.25e6

    X400_MIN_EXT_REF_FREQ = 1e6
    X400_MAX_EXT_REF_FREQ = 50e6

    class SetSyncRetVal(Enum):
        """ Return value options for _set_sync_source() """
        OK = 'OK'
        NOP = 'nop'
        FAIL = 'failure'

    def __init__(self,
                 args,
                 clk_policy,
                 clk_aux_board,
                 cpld_control,
                 log):
        self.log = log.getChild("ClkMgr")
        self._clocking_auxbrd = clk_aux_board
        if self._clocking_auxbrd:
            self._safe_sync_source = {
                'clock_source': self.X400_DEFAULT_CLOCK_SOURCE,
                'time_source': self.X400_DEFAULT_TIME_SOURCE,
            }
        else:
            self._safe_sync_source = {
                'clock_source': self.CLOCK_SOURCE_MBOARD,
                'time_source': self.TIME_SOURCE_INTERNAL,
            }
        self.clk_policy = clk_policy
        # Parse args
        self._time_source = args.get(
            'time_source', self._safe_sync_source['time_source'])
        self._clock_source = args.get(
            'clock_source', self._safe_sync_source['clock_source'])
        if not self._clocking_auxbrd:
            self._clock_source = self.CLOCK_SOURCE_MBOARD
        # This is the reference clock frequency generated by the motherboard
        # itself (from the RPLL)
        self._int_clock_freq = self.X400_DEFAULT_INT_CLOCK_FREQ
        # Here, we only allow a scalar master_clock_rate value, even though the
        # device may support multiple rates. However, those need to be set in
        # init(). Note that the clock policy doesn't know anything yet other than
        # the EEPROM contents (i.e., it knows which device we're on) because
        # without clocks, we can't read from the FPGA registers. The default
        # MCR thus is going to be a rough guess, at best.
        self._master_clock_rates = [
            float(args.get('master_clock_rate', self.clk_policy.get_default_mcr()))]
        # This is the reference clock frequency that comes from the clocking
        # aux board
        self._ext_clock_freq = None
        self._set_ref_clock_freq(
            float(args.get('ext_clock_freq', self.X400_DEFAULT_EXT_CLOCK_FREQ)),
            update_clocks=False)
        # If there is a valid PRIREF clock signal available at the RPLL (e.g.,
        # when SyncE is being used), set this to whatever the rate of the input
        # clock signal is.
        # Note: The secondary reference (SECREF) is always enabled, it's a
        # fixed, reliable 100 MHz oscillator.
        self._rpll_priref_rate = None
        # These need to be added later (when __init__ is called, the call site
        # does not yet have these available, because it needs to init some
        # clocks for that)
        self.rfdc = None
        self._set_reset_db_clocks = lambda x: None
        # Init peripherals
        self._init_available_srcs()
        if self._time_source not in self.get_time_sources():
            raise ValueError(f"Invalid time source configured: {self._time_source}")
        if self._clock_source not in self.get_clock_sources():
            raise ValueError(f"Invalid clock source configured: {self._clock_source}")
        self.clk_ctrl = X4xxClockCtrl(cpld_control, log)
        self._init_ref_clock_and_time()
        self._init_meas_clock()
        ### IMPORTANT! The clocking initialization is not complete until
        ### finalize_init() was called. This is as far as we get for now.


    ###########################################################################
    # Initialization code.
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
        self.log.trace(f"Available time sources are: {self._avail_time_sources}")


    def _init_ref_clock_and_time(self):
        """
        Initialize clock and time sources. After this function returns, the
        reference signals going to the FPGA are valid.

        This is only called once, during __init__(). Calling it again will set
        clocks to defaults, but is also redundant since clocks do not need to be
        initialized twice.
        """
        # Now initializes and reconfigure all clocks.
        # If clock_source and ref_clock_freq are not provided, they will not be changed.
        # If any other parameters are not provided, they will be configured with
        # default values.
        self._reset_clocks(value=True, reset_list=['cpld'])
        # This is a pared-down version of set_sync_source(), but without
        # configuring any clocks.
        self._set_brc_source(self.get_clock_source(), self.get_time_source())
        # Now configure the PLLs
        self._config_rpll(
            self.X400_DEFAULT_MGT_CLOCK_RATE,
            self.X400_DEFAULT_INT_CLOCK_FREQ,
            RpllRefSel.SECONDARY)
        clk_config = self.clk_policy.get_config(
            self.get_ref_clock_freq(), self._master_clock_rates)
        self.clk_ctrl.config_spll(clk_config.spll_config)
        self._reset_clocks(value=False, reset_list=['cpld'])


    def _init_meas_clock(self):
        """
        Initialize the TDC measurement clock. After this function returns, the
        FPGA TDC meas_clock is valid.
        """
        # This may or may not be used for X400. Leave as a place holder
        self.log.trace("TDC measurement clock not yet implemented.")

    @no_rpc
    def finalize_init(self, args, mboard_regs_control, rfdc):
        """
        Store a reference to the RFdc object, and update internal state.

        This completes initialization of the clocking. After this call, all
        clocks are ticking at a useful rate.
        """
        self.clk_ctrl.mboard_regs_control = mboard_regs_control
        self.rfdc = rfdc
        # Update the policy with new info on the FPGA image:
        self.clk_policy.set_dsp_info(self.rfdc.get_dsp_info())

        # Now do the full MCR init for the first time. When this is done, all
        # the clocks will be ticking. Now, the policy should know about the
        # FPGA capabilities, and can choose the correct default MCR.
        initial_mcr = \
            float(args.get('master_clock_rate', self.clk_policy.get_default_mcr()))
        try:
            initial_mcr = self.clk_policy.coerce_mcr([initial_mcr])
        except ValueError:
            self.log.warning(
                f"Requested initial master clock rate {initial_mcr/1e6} MHz is invalid!")
            initial_mcr = self.clk_policy.get_default_mcr()
        self._master_clock_rates = initial_mcr

        # Force reset the RFDC to ensure it is in a good state. Note that for
        # pulling the RFDC out of reset, we need access to the rfdc object, and
        # the clocking policy needs access to the DSP info, meaning this is the
        # earliest point during initialization when we can do this reset.
        self._reset_clocks(True, ('mmcm', 'rfdc'))
        self._reset_clocks(False, ('mmcm', 'rfdc'))

        # Devices that use non-default clocking parameters may require an explicit
        # startup after the RFDC reset:
        self.rfdc.startup_tiles(quiet=True)

        # The initial default mcr only works if we have an FPGA with
        # a decimation of 2. But we need the overlay applied before we
        # can detect decimation, and that requires clocks to be initialized.
        self.set_master_clock_rate([initial_mcr[0],])


    @no_rpc
    def set_dboard_reset_cb(self, db_reset_cb):
        """
        Provide callbacks to set/reset daughterboard clocks. The controls for
        these live elsewhere (not on X4xxClockCtrl), thus they have to be given
        to us after the dboards have been initialized.
        """
        self._set_reset_db_clocks = db_reset_cb


    @no_rpc
    def init(self, args):
        """
        Called by x4xx.init(), when a UHD session initializes.

        Rules for argument handling:
        - If clock_source and/or time_source are not given, then we use the
          defaults (internal)
        - If master_clock_rate is not given, we use whatever the device last
          was set to.
        - If ext_clock_freq is not given, we use whatever the device was last
          set to (this only matters for 'external' clock source!)
        - If converter_rate is not given, we use the maximum possible converter
          rate. If it is given, but master_clock_rate is not given, we ignore it.
        - If clock source, time source, ext_clock_freq, and master clock rate do
          not change from run to run, we skip re-initializing the RFDC chain.
          - The user can override this by specifying `force_reinit` (like with
            N310).
          - If converter_rate is given, we always force reinit. This could be
            optimized away if desired.
        """
        args['clock_source'] = args.get('clock_source', self.X400_DEFAULT_CLOCK_SOURCE)
        args['time_source'] = args.get('time_source', self.X400_DEFAULT_TIME_SOURCE)
        self.clk_policy.args = args
        # This flag will be used to force a full run of the clocking initialization.
        # If False, MPM may still decide to do a full clocking initialization,
        # depending on other settings.
        force_set_mcr = bool(args.get('force_reinit', False))
        # If this is None, the user desires the MCR to not change
        desired_master_clock_rates = args.get('master_clock_rate')
        desired_converter_rates = args.get('converter_rate')
        # If the user specifies everything, then we always force setting
        # everything. This can be optimized in the future.
        force_set_mcr = desired_master_clock_rates and desired_converter_rates
        # Convert desired master clock rate from string into a tuple. We cover
        # the special case where a single rate was given to populate both
        # daughterboard rates.
        if desired_master_clock_rates:
            desired_master_clock_rates = \
                parse_multi_device_arg(args['master_clock_rate'], conv=float)
            if len(desired_master_clock_rates) == 1 and \
                    len(self._master_clock_rates) > 1:
                desired_master_clock_rates = \
                    desired_master_clock_rates * len(self._master_clock_rates)
        if 'ext_clock_freq' in args:
            new_ext_clock_freq = float(args.get('ext_clock_freq'))
            if new_ext_clock_freq != self._ext_clock_freq:
                # This will update self._ext_clock_freq and validate, but not
                # apply the settings to hardware.
                self._set_ref_clock_freq(new_ext_clock_freq, update_clocks=False)
                # Now we force an update to all clocking further down.
                if args['clock_source'] == self.CLOCK_SOURCE_EXTERNAL:
                    if not desired_master_clock_rates:
                        desired_master_clock_rates = self._master_clock_rates
                    force_set_mcr = True
        # set_sync_source() will cause a reconfiguration of the master clock rate
        # if anything changed. By modifying self._master_clock_rates, we can
        # use that to immediately set the sync source, the external ref clock freq,
        # and the MCR all in one go. If we called set_master_clock_rate()
        # separately, there's a theoretical chance that we would fail to lock
        # the old MCR with the new ext_clock_freq, or vice versa.
        if desired_master_clock_rates and \
                tuple(desired_master_clock_rates) != tuple(self._master_clock_rates):
            self._master_clock_rates = desired_master_clock_rates
            force_set_mcr = True
        if desired_converter_rates and not desired_master_clock_rates:
            self.log.warning("Passing `converter_rate` argument without obligatory "
                             "`master_clock_rate` argument. Ignoring `converter_rate`.")
        if force_set_mcr:
            args['force_reinit'] = True
        self.set_sync_source(args)


    @no_rpc
    def deinit(self):
        """
        Called by x4xx.deinit(), when a UHD session terminates.

        This will put clocks etc. into a valid state, if they previously weren't
        in one.
        """
        if not self.clk_ctrl.get_ref_locked():
            self.log.error("ref clocks aren't locked, falling back to default")
            self.set_sync_source(self._safe_sync_source)


    @no_rpc
    def tear_down(self):
        """
        Called by x4xx.tear_down(), when the device driver is being unloaded.
        """
        self.rfdc = None
        self._set_reset_db_clocks = None


    ###########################################################################
    # Internal helpers/workers
    ###########################################################################
    def _set_sync_source(self, clock_source, time_source, force=False):
        """
        See set_sync_source() for docs. This is the internal helper that does
        the actual work, but it assumes clock_source and time_source are
        validated/sanitized.

        This method checks if the new clock/time source are different from the
        current ones. If not, it will return SetSyncRetVal.NOP and not touch
        the hardware.

        If anything changed, then all clocks will be put into reset. Call
        set_master_clock_rate() in that case to get them running again!
        """
        assert (clock_source, time_source) in self.valid_sync_sources, \
            f'Clock and time source pair ({clock_source}, {time_source}) is ' \
            'not a valid selection'
        # Now see if we can keep the current settings, or if we need to run an
        # update of sync sources:
        if not force and \
                clock_source == self._clock_source and \
                time_source == self._time_source:
            if self.clk_ctrl.get_ref_locked():
                # Nothing changed, no need to do anything
                self.log.trace("New sync source assignment matches "
                               "previous assignment. Ignoring update command.")
                return self.SetSyncRetVal.NOP
            self.log.debug(
                "Although the clock source has not changed, some PLLs "
                "are not locked. Setting clock source again...")
        # Start setting sync source
        self.log.debug(
            f"Setting sync source to time_source={time_source}, "
            f"clock_source={clock_source}")
        self._time_source = time_source
        # Reset downstream clocks (excluding RPLL)
        self._reset_clocks(True, ('db_clock', 'cpld', 'rfdc', 'mmcm', 'spll'))
        self._set_brc_source(clock_source, time_source)
        self._clock_source = clock_source
        return self.SetSyncRetVal.OK

    def _config_rpll(
            self,
            usr_clk_rate: float,
            internal_brc_rate: float,
            internal_brc_source: RpllRefSel):
        """
        Configures the LMK03328 to generate the desired MGT reference clocks
        and internal BRC rate.

        Currently, the MGT protocol selection is not supported, but a custom
        usr_clk_rate can be generated from PLL1.

        usr_clk_rate - the custom clock rate to generate from PLL1
        internal_brc_rate - the rate to output as the BRC
        internal_brc_source - the reference source which drives the RPLL
                              (primary or secondary). Primary is the clocking
                              aux card, secondary is the 100 MHz oscillator.
        """
        assert isinstance(internal_brc_source, RpllRefSel)
        if internal_brc_source == RpllRefSel.PRIMARY and \
                self._rpll_priref_rate is None:
            err = f"Invalid internal BRC source '{internal_brc_source}' was selected."
            self.log.error(err)
            raise RuntimeError(err)
        ref_rate = self._rpll_priref_rate \
                if internal_brc_source == RpllRefSel.PRIMARY \
                else self.X400_RPLL_SECREF_RATE
        self.clk_ctrl.config_rpll(
            internal_brc_source,
            ref_rate,
            internal_brc_rate,
            usr_clk_rate)
        # The internal BRC rate will only change when _config_rpll is called
        # with a new internal BRC rate
        self._int_clock_freq = internal_brc_rate


    def _config_mmcm(self, clk_settings):
        """
        Configure the MMCM which provides various internal clocks to the
        data path after the RFdc.

        Also waits for the MMCM to be done with reconfiguration.
        """
        self.log.trace("Entering _config_mmcm()")
        assert self.rfdc
        # First configure all MMCM settings:
        if not clk_settings.mmcm_use_defaults:
            self.log.debug("Updating MMCM")
            # This reconfigures, but does not wait for a valid lock.
            self.rfdc.rfdc_configure_mmcm(
                clk_settings.mmcm_input_divider,
                clk_settings.mmcm_feedback_divider,
                clk_settings.mmcm_output_div_map)
            self.log.trace('MMCM Configuration applied, now waiting for MMCM to lock...')
            self.rfdc.reset_mmcm(reset=False)
        else:
            self.log.debug("Using default values for MMCM")

    def _set_ref_clock_freq(self, freq, update_clocks=True):
        """
        Tell our USRP what the frequency of the external reference clock is.

        Will throw if it's not a valid value.
        """
        if self._ext_clock_freq == freq:
            return
        if freq < self.X400_MIN_EXT_REF_FREQ or freq > self.X400_MAX_EXT_REF_FREQ:
            raise RuntimeError(
                'External reference clock frequency is out of the valid range.')
        self.clk_policy.validate_ref_clock_freq(freq, self._master_clock_rates)
        self._ext_clock_freq = freq
        # If the external source is currently selected we also need to re-apply the
        # time_source. This call also updates the dboards' rates.
        if update_clocks and self.get_clock_source() == self.CLOCK_SOURCE_EXTERNAL:
            self.set_sync_source({'clock_source': self._clock_source,
                                  'time_source': self._time_source})

    def _set_brc_source(self, clock_source, time_source):
        """
        Switches the Base Reference Clock Source between internal, external,
        mboard, and gpsdo using the GPIO pin and clocking aux board control.
        'internal' is a clock source internal to the clocking aux board, but
        external to the motherboard.
        Should not be called outside of set_sync_source() or
        _init_ref_clock_and_time() without proper reset and reconfig of
        downstream clocks.
        """
        if clock_source == self.CLOCK_SOURCE_MBOARD:
            if self._clocking_auxbrd:
                self._clocking_auxbrd.export_clock(False)
            self.clk_ctrl.select_brc_source(BrcSource.RPLL)
        else:
            if self._clocking_auxbrd is None:
                self.log.error(f'Invalid BRC selection {clock_source}. No clocking aux '
                               'board was found.')
                raise RuntimeError(f'Invalid BRC selection {clock_source}')
            self.clk_ctrl.select_brc_source(BrcSource.CLK_AUX)
            if clock_source == self.CLOCK_SOURCE_EXTERNAL:
                # This case is a bit special: We also need to tell the clocking
                # aux board if we plan to consume the external time reference or
                # not.
                time_src_board = \
                    ClockingAuxBrdControl.SOURCE_EXTERNAL \
                    if time_source == self.TIME_SOURCE_EXTERNAL \
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
                self.log.error(f'Invalid BRC selection {clock_source}')
                raise RuntimeError(f'Invalid BRC selection {clock_source}')
        self.log.debug(f"Base reference clock source is: {clock_source}")

    def _reset_clocks(self, value, reset_list):
        """
        Shuts down all clocks downstream to upstream or clears reset on all
        clocks upstream to downstream. Specify the list of clocks to reset in
        reset_list. The order of clocks specified in the reset_list does not
        affect the order in which the clocks are reset.
        """
        if value:
            self.log.trace(f"Reset clocks: {reset_list}")
            if 'db_clock' in reset_list:
                self._set_reset_db_clocks(value)
            if 'cpld' in reset_list:
                self.clk_ctrl.reset_clock(value, 'cpld')
            if 'rfdc' in reset_list:
                assert self.rfdc
                self.rfdc.reset_rfdc(reset=value)
            if 'mmcm' in reset_list:
                self.rfdc.reset_mmcm(reset=value)
            if 'spll' in reset_list:
                self.clk_ctrl.reset_clock(value, 'spll')
            if 'rpll' in reset_list:
                self.clk_ctrl.reset_clock(value, 'rpll')
        else:
            self.log.trace(f"Bring clocks out of reset: {reset_list}")
            # Inverse order from resetting
            if 'rpll' in reset_list:
                self.clk_ctrl.reset_clock(value, 'rpll')
            if 'spll' in reset_list:
                self.clk_ctrl.reset_clock(value, 'spll')
            if 'mmcm' in reset_list:
                assert self.rfdc
                self.rfdc.reset_mmcm(reset=value)
            if 'rfdc' in reset_list:
                assert self.rfdc
                self.rfdc.reset_rfdc(reset=value)
            if 'cpld' in reset_list:
                self.clk_ctrl.reset_clock(value, 'cpld')
            if 'db_clock' in reset_list:
                self._set_reset_db_clocks(value)


    ###########################################################################
    # Public APIS, but not MPM APIs (these can be called by x4xx or the
    # DB iface)
    ###########################################################################
    @no_rpc
    def set_master_clock_rate(self, master_clock_rates):
        """
        Sets the master clock rate by configuring the RFDC decimation and SPLL,
        and then resetting downstream clocks.

        Arguments:
        master_clock_rates -- An array either of length 1 (then this rate will be
                             applied to all daughterboards) or of length equal
                             to the number of daughterboards, if the current
                             design supports separate clock rates per dboard.
                             If the design only supports a single rate, then it
                             must be of length 1.
                             A scalar value will be interpreted as a list of
                             length 1.
        """
        master_clock_rates = master_clock_rates \
                if isinstance(master_clock_rates, (list, tuple)) \
                else [master_clock_rates]
        max_num_rates = self.clk_policy.get_num_rates()
        if len(master_clock_rates) == 1:
            master_clock_rates = [master_clock_rates[0]] * max_num_rates
        elif max_num_rates == 1:
            # User provided > 1 rate, but we can only support one
            raise RuntimeError(
                'Invalid number of clock rates provided! Must provide a single rate.')
        elif len(master_clock_rates) != max_num_rates:
            raise RuntimeError(
                f'Invalid number of clock rates provided! Must provide either a '
                f'single rate or {max_num_rates} rates.')
        # Get the validated and rounded MCR back
        master_clock_rates = self.clk_policy.coerce_mcr(master_clock_rates)
        clk_settings = self.clk_policy.get_config(
            self.get_ref_clock_freq(), master_clock_rates)
        # Reset everything downstream from SPLL
        self._reset_clocks(True, ('mmcm', 'rfdc', 'cpld', 'db_clock'))
        # The following call will return only when the SPLL successfully locks
        # to the new settings:
        self.clk_ctrl.config_spll(clk_settings.spll_config)
        # When the SPLL is configured and locked, its output dividers are
        # synchronized (share a common flank). Next, we need to synchronize the
        # R-dividers to the common PPS signals. Because we need to wait for the
        # PPS, this function may take > 1s to execute, worst-case.
        self.clk_ctrl.sync_spll_clocks(
            "internal_pps" if self._time_source == self.TIME_SOURCE_INTERNAL else "external_pps",
            self.get_ref_clock_freq())
        # At this point the SPLL is sync'd in time and frequency to the reference.
        # From now on, no-one will be touching the SPLL until we call
        # set_master_clock_rate() again.
        # Bring MMCM out of reset, and reconfigure. The MMCM lock status
        # is monitored at the end of the MMCM DRP access, as it is required
        # for the MMCM to report the DRP configuration as complete.
        self.rfdc.reset_mmcm(reset=False, check_locked=False)
        self.rfdc.rfdc_update_mmcm_regs()
        self._config_mmcm(clk_settings)
        # Bring RFDC out of reset, reset tiles and reconfigure RFDC
        self._reset_clocks(False, ('rfdc',))
        self.rfdc.reset_tiles()
        self.rfdc.configure(clk_settings.spll_config.output_freq, clk_settings.rfdc_configs)
        # If non-default clocking parameters are used, reset_tiles() may not be successful in
        # the first place, so we have to start the tiles explicitely again after the
        # reconfiguration:
        self.rfdc.startup_tiles()
        # All settings are applied: Now bring other clocks out of reset. Note
        # that cpld and db_clock don't depend on MMCM or RFDC, but we don't need
        # them so they only come back now.
        self._reset_clocks(False, ('cpld', 'db_clock'))
        self._master_clock_rates = master_clock_rates
        # Configure PPS forwarding to timekeepers. The requirement is that this
        # be called after sync_spll_clocks() was called.
        for tk_idx, mcr in enumerate(master_clock_rates):
            self.clk_ctrl.configure_pps_forwarding(tk_idx, True, mcr)


    @no_rpc
    def get_master_clock_rate(self, db_idx=0):
        """ Return the master clock rate set during init """
        return self._master_clock_rates[0] \
                if len(self._master_clock_rates) == 1 \
                else self._master_clock_rates[db_idx]

    @no_rpc
    def get_ref_clock_freq(self):
        """
        Returns the currently active reference clock frequency (BRC).

        Note: We don't measure this frequency, we need to be told what it is.
        This happens during __init__() or init().

        If the value that is passed into here and the actual reference clock
        frequency don't match, then things may fail.
        """
        clock_source = self.get_clock_source()
        if clock_source == self.CLOCK_SOURCE_MBOARD:
            return self._int_clock_freq
        if clock_source == self.CLOCK_SOURCE_GPSDO:
            return self.X400_GPSDO_OCXO_CLOCK_FREQ
        # clock_source == "external":
        return self._ext_clock_freq

    @no_rpc
    def get_converter_rate(self, db_idx):
        """
        Returns the rate at which the ADC/DAC on a given daughterboard is
        sampling.
        """
        if not self.rfdc:
            self.log.info(
                "Querying converter rate from SPLL, RFDC is not yet enabled!")
            return self.clk_ctrl.get_spll_freq()
        return self.rfdc.get_converter_rate(db_idx)

    @no_rpc
    def get_prc_rate(self):
        """
        Return the PRC rate
        """
        return self.clk_ctrl.get_prc_rate()


    ###########################################################################
    # Clock/Time API (this is a public MPM API)
    ###########################################################################
    def get_spll_freq(self):
        """
        Return the output frequency of the SPLL that is routed to the RFDC PLLs.

        Note: To get the actual converter rate, don't use this. Instead, use
        get_dboard_sample_rate().
        """
        return self.clk_ctrl.get_spll_freq()

    def get_clock_source(self):
        " Return the currently selected clock source "
        return self._clock_source

    def get_clock_sources(self):
        """
        Lists all available clock sources.
        """
        return self._avail_clk_sources

    def set_clock_source(self, *args):
        """
        Ensures the new reference clock source and current time source pairing
        is valid and sets both by calling set_sync_source().
        """
        clock_source = args[0]
        time_source = self.get_time_source()
        assert clock_source is not None
        assert time_source is not None
        if (clock_source, time_source) not in self.valid_sync_sources:
            old_time_source = time_source
            if clock_source in (
                    self.CLOCK_SOURCE_MBOARD, self.CLOCK_SOURCE_INTERNAL):
                time_source = self.TIME_SOURCE_INTERNAL
            elif clock_source == self.CLOCK_SOURCE_EXTERNAL:
                time_source = self.TIME_SOURCE_EXTERNAL
            elif clock_source == self.CLOCK_SOURCE_GPSDO:
                time_source = self.TIME_SOURCE_GPSDO
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
        if self._clocking_auxbrd is None:
            raise RuntimeError("No clocking aux board available")
        clock_source = self.get_clock_source()
        if self.get_time_source() == self.TIME_SOURCE_EXTERNAL:
            raise RuntimeError(
                'Cannot export clock when using external time reference!')
        if clock_source not in self._clocking_auxbrd.VALID_CLK_EXPORTS:
            raise RuntimeError(f"Invalid source to export: `{clock_source}'")
        return self._clocking_auxbrd.export_clock(enable)

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source


    def get_time_sources(self):
        " Returns list of valid time sources "
        return self._avail_time_sources

    def set_time_source(self, time_source):
        """
        Set a time source

        This will call set_sync_source() internally, and use the current clock
        source as a clock source. If the current clock source plus the requested
        time source is not a valid combination, it will coerce the clock source
        to a valid choice and print a warning.
        """
        clock_source = self.get_clock_source()
        assert clock_source is not None
        assert time_source is not None
        if (clock_source, time_source) not in self.valid_sync_sources:
            old_clock_source = clock_source
            if time_source == self.TIME_SOURCE_QSFP0:
                clock_source = self.CLOCK_SOURCE_MBOARD
            elif time_source == self.TIME_SOURCE_INTERNAL:
                clock_source = self.CLOCK_SOURCE_MBOARD
            elif time_source == self.TIME_SOURCE_EXTERNAL:
                clock_source = self.CLOCK_SOURCE_EXTERNAL
            elif time_source == self.TIME_SOURCE_GPSDO:
                clock_source = self.CLOCK_SOURCE_GPSDO
            self.log.warning(
                f"Clock source {old_clock_source} is an invalid selection with "
                f"time source {time_source}. Coercing clock source to {clock_source}")
        self.set_sync_source(
            {"time_source": time_source, "clock_source": clock_source})

    def get_sync_sources(self):
        """
        Enumerates permissible sync sources.
        """
        return [{
            "time_source": time_source,
            "clock_source": clock_source
        } for (clock_source, time_source) in self.valid_sync_sources]

    def set_sync_source(self, args):
        """
        Selects reference clock and PPS sources. Unconditionally re-applies the
        time source to ensure continuity between the reference clock and time
        rates.
        Note that if we change the source such that the time source is changed
        to 'external', then we need to also disable exporting the reference
        clock (RefOut and PPS-In are the same SMA connector).
        """
        assert self.rfdc
        # Check the clock source, time source, and combined pair are valid:
        clock_source = args.get('clock_source', self.get_clock_source())
        if clock_source not in self.get_clock_sources():
            raise ValueError(f'Clock source {clock_source} is not a valid selection')
        time_source = args.get('time_source', self.get_time_source())
        if time_source not in self.get_time_sources():
            raise ValueError(f'Time source {time_source} is not a valid selection')
        if (clock_source, time_source) not in self.valid_sync_sources:
            raise ValueError(
                f'Clock and time source pair ({clock_source}, {time_source}) is '
                'not a valid selection')
        # Also check the external reference clock frequency is valid
        if clock_source == self.CLOCK_SOURCE_EXTERNAL:
            self.clk_policy.validate_ref_clock_freq(
                self.get_ref_clock_freq(), self._master_clock_rates)
        # Sanity checks complete. Now check if we need to disable the RefOut.
        # Reminder: RefOut and PPSIn share an SMA. Besides, you can't export an
        # external clock. We are thus not checking for time_source == 'external'
        # because that's a subset of clock_source == 'external'.
        # We also disable clock exports for 'mboard', because the mboard clock
        # does not get routed back to the clocking aux board and thus can't be
        # exported either.
        if clock_source in (self.CLOCK_SOURCE_EXTERNAL, self.CLOCK_SOURCE_MBOARD) \
                and self._clocking_auxbrd:
            self._clocking_auxbrd.export_clock(enable=False)
        # Now configure the sync sources:
        force_update = args.get("force_reinit", False)
        ret_val = self._set_sync_source(clock_source, time_source, force_update)
        if ret_val == self.SetSyncRetVal.NOP and not force_update:
            self.log.debug("Skipping reconfiguration of clocks.")
            return
        try:
            self.log.debug(
                f"Reconfiguring clock configuration for a master clock rate of "
                f"{self._master_clock_rates}")
            # Re-set master clock rate. If this doesn't work, it will time out
            # and throw an exception. We need to put the device back into a safe
            # state in that case.
            self.set_master_clock_rate(self._master_clock_rates)
            # Restore the nco frequency to the same values as before the sync source
            # was changed, to ensure the device transmission/acquisition continues at
            # the requested frequency.
            self.rfdc.rfdc_restore_nco_freq()
            # Do the same for the calibration freeze state
            self.rfdc.rfdc_restore_cal_freeze()
            # Call synchronize to trigger the tile latency determination again. This is necessary
            # to prevent us from bad EVM after changing the sync source.
            self.synchronize(None, False)
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


    #######################################################################
    # Timekeeper API (this is a public MPM API)
    #######################################################################
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
    # Ref-Clk tuning word APIs.
    #
    # These calls will be available as MPM calls. They are a discoverable
    # feature.
    ###########################################################################
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

    ###########################################################################
    # Synchronization API
    ###########################################################################
    def synchronize(self, sync_args, finalize):
        """
        See PeriphManagerBase.synchronize() for the full documentation.
        """
        def determine_sync_settings():
            """
            This is the algorithm that is run when finalize is False: Determine
            the values.
            """
            self.log.trace("Determining sync settings...")
            sync_args_result = {}
            rates = [self.rfdc.get_converter_rate(db_idx) for db_idx in range(2)]
            if rates[0] != rates[1]:
                self.log.debug("Synchronizing daughterboards separately.")
                db_keys = [0, 1]
                db_keys = [
                    (0, 'adc_latency0', 'dac_latency0'),
                    (1, 'adc_latency1', 'dac_latency1'),
                ]
            else:
                db_keys = [('all', 'adc_latency', 'dac_latency')]
            for db_key, adc_lat_key, dac_lat_key in db_keys:
                adc_latencies, dac_latencies = \
                    self.rfdc.determine_tile_latencies(db_key)
                self.log.debug(
                    f"Determined ADC tile latencies: "
                    f"{','.join(f'{k}={v}' for k,v in adc_latencies.items())}")
                self.log.debug(
                    f"Determined DAC tile latencies: "
                    f"{','.join(f'{k}={v}' for k,v in dac_latencies.items())}")
                # We need to pick a scalar latency value from all the latencies.
                # For now, we just pick the first.
                adc_latency = adc_latencies[0]
                dac_latency = dac_latencies[0]
                sync_args_result[adc_lat_key] = str(adc_latency)
                sync_args_result[dac_lat_key] = str(dac_latency)
            return sync_args_result

        def finalize_sync_settings(sync_args):
            """
            This is the algorithm that is run when finalize is True: Apply the
            latency values.
            """
            self.log.trace("Finalizing synchronization settings.")
            if 'adc_latency' in sync_args and \
                    'dac_latency' in sync_args:
                return self.rfdc.set_tile_latencies(
                    'all',
                    int(sync_args['adc_latency']),
                    int(sync_args['dac_latency']))
            if all(
                    lambda x: x in sync_args \
                    for x in ('adc_latency0', 'adc_latency1',
                              'dac_latency0', 'dac_latency1')):
                return \
                    self.rfdc.set_tile_latencies(
                        0,
                        int(sync_args['adc_latency0']),
                        int(sync_args['dac_latency0'])) and \
                    self.rfdc.set_tile_latencies(
                        1,
                        int(sync_args['adc_latency1']),
                        int(sync_args['dac_latency1']))
            self.log.error("Invalid sync args provided!")
            raise RuntimeError("Invalid sync args provided!")
        # Go, go, go!
        if not finalize:
            return determine_sync_settings()
        if not finalize_sync_settings(sync_args):
            self.log.error("Failed to apply synchronization settings!")
        return {}

    def aggregate_sync_data(self, collated_sync_data):
        """
        See PeriphManagerBase.aggregate_sync_data() for the full documentation.
        """
        self.log.trace("Aggregating sync data...")
        return {} if not collated_sync_data else collated_sync_data[0]


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
        self.clk_ctrl.enable_ecpri_clocks(enable, clock)

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
        previous_priref_rate = self._rpll_priref_rate
        self._rpll_priref_rate = 25e6

        # LMK28PRIRefClk only available when nsync is source, as lmk
        # is powered off otherwise
        self._set_sync_source(clock_source='nsync', time_source=self._time_source)

        # Add LMK28PRIRefClk as an available RPLL reference source
        # 1 => PRIREF source; source is output at 25 MHz
        # TODO: enable out4 on LMK
        self._config_rpll(self.X400_DEFAULT_MGT_CLOCK_RATE,
                          self.X400_DEFAULT_INT_CLOCK_FREQ,
                          RpllRefSel.PRIMARY)

        # remove clkaux_nsync_clk as a valid reference source for later calls
        # to _config_rpll(), it is only valid in this configuration
        self._rpll_priref_rate = previous_priref_rate


    def get_fpga_aux_ref_freq(self):
        """
        Return the tick count of an FPGA counter which measures the width of
        the PPS signal on the FPGA_AUX_REF FPGA input using a 40 MHz clock.
        Main use case until we support eCPRI is manufacturing testing.
        A return value of 0 indicates absence of a valid PPS signal on the
        FPGA_AUX_REF line.
        """
        return self.clk_ctrl.get_fpga_aux_ref_freq()
