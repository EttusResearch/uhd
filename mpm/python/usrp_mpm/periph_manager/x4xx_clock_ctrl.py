#
# Copyright 2021-2022 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4x0 Motherboard Clocking Control

This module contains the X4xxClockCtrl class, which is the worker bee for the
X4xxClockManager class.

The code in this module controls the RPLL and SPLL as well as some of the muxing.
The eCPRI PLL is controlled from the ClockingAuxBrdControl class.
"""

from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.sys_utils import i2c_dev
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev
from usrp_mpm.periph_manager.x4xx_periphs import MboardRegsControl
from usrp_mpm.periph_manager.x4xx_sample_pll import LMK04832X4xx
from usrp_mpm.periph_manager.x4xx_reference_pll import LMK03328X4xx
from usrp_mpm.periph_manager.x4xx_clock_types import \
        RpllRefSel, RpllBrcSrcSel, BrcSource

X400_RPLL_I2C_LABEL = 'rpll_i2c'

class X4xxClockCtrl:
    """
    X4x0 Clocking Control

    This class controls various parts of the X4xx clocking:
    - Sample PLL (LMK04832)
    - Reference PLL (LMK03328)
    - Base Reference Clock source selection (via GPIO)
    - Clocking-related motherboard registers
    - Clocking-related settings on the motherboard CPLD

    This class exposes APIs to control all of these components. The sequencing
    logic etc. is encoded in the X4xxClockManager class.
    """
    def __init__(self, cpld_control, log):
        # Store parent objects
        self.log = log.getChild("ClkMgr")
        self._cpld_control = cpld_control
        # Preallocate other objects to satisfy linter
        self.mboard_regs_control = None # Needs to be set later from outside!
        self._sample_pll = None
        self._reference_pll = None
        self._base_ref_clk_select = None
        # Init peripherals
        self._init_clk_peripherals()

    ###########################################################################
    # Initialization code
    ###########################################################################
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
        rpll_i2c_bus = i2c_dev.dt_symbol_get_i2c_bus(X400_RPLL_I2C_LABEL)
        if rpll_i2c_bus is None:
            raise RuntimeError("RPLL I2C bus could not be found")
        self.log.trace("Using RPLL I2C bus: %s", str(rpll_i2c_bus))
        reference_lmk_regs_iface = lib.i2c.make_i2cdev_regs_iface(
            rpll_i2c_bus,
            0x54,   # addr
            False,  # ten_bit_addr
            100,    # timeout_ms
            1       # reg_addr_size
        )
        self._sample_pll = LMK04832X4xx(sample_lmk_regs_iface, self.log)
        self._reference_pll = LMK03328X4xx(reference_lmk_regs_iface, self.log)
        # Init BRC select GPIO control
        self._base_ref_clk_select = Gpio('BASE-REFERENCE-CLOCK-SELECT', Gpio.OUTPUT, 1)

    ###########################################################################
    # Low-level controls
    ###########################################################################
    def reset_clock(self, value, clock_to_reset):
        """
        Helper API to reset or re-enable a clock.
        """
        if clock_to_reset == 'cpld':
            self._cpld_control.enable_pll_ref_clk(enable=not value)
        if clock_to_reset == 'spll':
            self._sample_pll.reset(value, hard=True)
        if clock_to_reset == 'rpll':
            self._reference_pll.reset(value, hard=True)

    def config_rpll(self,
                    internal_brc_source: RpllRefSel,
                    ref_rate: float,
                    internal_brc_rate: float,
                    usr_clk_rate: float):
        """
        Configures the LMK03328 to generate the desired MGT reference clocks
        and internal BRC rate.

        Currently, the MGT protocol selection is not supported, but a custom
        usr_clk_rate can be generated from PLL1.

        internal_brc_source - the reference source which drives the RPLL
                              (primary or secondary). Primary is the clocking
                              aux card, secondary is the 100 MHz oscillator.
        ref_rate - The frequency which is being provided as reference at the
                   input corresponding to internal_brc_source.
        internal_brc_rate - the rate to output as the BRC
        usr_clk_rate - the custom clock rate to generate from PLL1
        """
        assert internal_brc_source in RpllRefSel
        assert internal_brc_source == RpllRefSel.SECONDARY or \
                ref_rate is not None

        # If the desired rate matches the rate of the primary reference source,
        # directly passthrough that reference source
        if internal_brc_source == RpllRefSel.PRIMARY and \
                internal_brc_rate == ref_rate:
            brc_select = RpllBrcSrcSel.BYPASS
        else:
            brc_select = RpllBrcSrcSel.PLL

        # TODO: Questionable if we always need to call init() here
        self._reference_pll.init()
        self._reference_pll.config(
            internal_brc_source,
            ref_rate,
            internal_brc_rate,
            usr_clk_rate,
            brc_select)

    def config_spll(self, pll_settings: dict):
        """
        Configures the SPLL for the specified master clock rate.
        """
        # TODO: Questionable if we always need to call init() here
        self._sample_pll.init()
        self._sample_pll.config(pll_settings)

    def get_spll_freq(self):
        """
        Returns the output frequency setting of the SPLL that is routed to the
        ADCs/DACs.
        """
        return self._sample_pll.output_freq

    def get_prc_rate(self):
        """
        Returns the rate of the PLL Reference Clock (PRC) which is
        routed to the daughterboards (as an LO reference and CPLD clock) and to
        the FPGA (used to generate data_clk and rfdc_clk from the MMCM).
        Note: The PRC will change if the sample clock frequency is modified.
        """
        return self._sample_pll.prc_freq

    def select_brc_source(self, brc_src: BrcSource):
        """
        Configures the GaAs switch (U10/U11) that controls which BRC is used.

        """
        if brc_src == BrcSource.RPLL:
            self._base_ref_clk_select.set(1)
        elif brc_src == BrcSource.CLK_AUX:
            self._base_ref_clk_select.set(0)
        else:
            raise RuntimeError("Invalid BRC source.")

    def sync_spll_clocks(self, pps_source: str, ref_clock_freq: float):
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

            config = (ref_clock_freq, pps_source)
            if config not in supported_configs:
                raise RuntimeError("Unsupported combination of reference clock "
                                   "(%.2E) and PPS source (%s) for PPS sync." %
                                   config)
            return supported_configs[config]

        return self._sample_pll.pll1_r_divider_sync(
            lambda: self.mboard_regs_control.pll_sync_trigger(select_pps()))

    def configure_pps_forwarding(
            self,
            tk_idx: int,
            enable: bool,
            radio_clock_rate: float,
            delay: float = 1.0):
        """
        Configures the PPS forwarding to the sample clock domain (master
        clock rate). This function assumes sync_spll_clocks function has
        already been executed.

        :param tk_idx: Which timekeeper to configure.
        :param enable: Boolean to choose whether PPS is forwarded to the
                       sample clock domain.

        :param delay:  Delay in seconds from the PPS rising edge to the edge
                       occurence in the application. This value has to be in
                       range 0 < x <= 1. In order to forward the PPS signal
                       from base reference clock to sample clock an aligned
                       rising edge of the clock is required. This can be
                       created by the sync_spll_clocks function. Based on the
                       greatest common divisor of the two clock rates there
                       are multiple occurences of an aligned edge each second.
                       One of these aligned edges has to be chosen for the
                       PPS forwarding by setting this parameter.

        :return:       None, Exception on error
        """
        # FIXME MULTI_RATE enable multiple MCRs / timekeepers, use tk_idx
        return self.mboard_regs_control.configure_pps_forwarding(
            enable, tk_idx, radio_clock_rate, self.get_prc_rate(), delay)


    def get_ref_locked(self):
        """
        Return lock status both RPLL and SPLL.
        """
        ref_pll_status = self._reference_pll.get_status()
        sample_pll_status = self._sample_pll.get_status()
        self.log.trace(
            f"RPLL1 lock: {ref_pll_status['PLL1 lock']} "
            f"RPLL2 lock: {ref_pll_status['PLL2 lock']} "
            f"SPLL1 lock: {sample_pll_status['PLL1 lock']} "
            f"SPLL2 lock: {sample_pll_status['PLL2 lock']}")
        return all([
            ref_pll_status['PLL1 lock'],
            ref_pll_status['PLL2 lock'],
            sample_pll_status['PLL1 lock'],
            sample_pll_status['PLL2 lock'],
        ])

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

    def get_fpga_aux_ref_freq(self):
        """
        Return the tick count of an FPGA counter which measures the width of
        the PPS signal on the FPGA_AUX_REF FPGA input using a 40 MHz clock.
        Main use case until we support eCPRI is manufacturing testing.
        A return value of 0 indicates absence of a valid PPS signal on the
        FPGA_AUX_REF line.
        """
        return self.mboard_regs_control.get_fpga_aux_ref_freq()
