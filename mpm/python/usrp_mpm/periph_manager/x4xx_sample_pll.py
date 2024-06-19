#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK04832 driver for use with X4xx

On the X4xx, we also refer to this as the "sample PLL". It generates the clock
that drives the RFdcs, as well as the PRC which is both a reference clock for
the daughterboards (LOs, CPLD) and for the FPGA (it generates data_clk, rfdc_clk
from this).
"""

from usrp_mpm.chips import LMK04832
from usrp_mpm.periph_manager.x4xx_clock_types import Spll1Vco
from usrp_mpm.sys_utils.gpio import Gpio


class LMK04832X4xx(LMK04832):
    """
    X4xx-specific subclass of the Sample Clock PLL LMK04832 controls.

    X4xx-specific usage notes of the LMK04832:
    - The PLL is driven in nested 0-delay mode, using an external feedback of
      the SysRef signal, going directly from CLKout9 to FBCLKin. We don't use
      an internal feedback because the feedback trace matches the length of the
      SysRef trace to the SoC.
    - All our configurations use a PLL2 R-divider of 1. We thus don't have to
      synchronize PLL2 R-divider.
    """

    # maximum: 1,1023, but limited as greater dividers generate too low Fin for RFDC anyway
    PRC_OUT_DIVIDERS = range(1, 1023)
    # local SCLK Digital Delay set to minimum value 8, see LMK Data Sheet Section 8.6.2.2.7
    SCLK_DIG_DELAY = 8

    # Obtainable SYSREF frequencies for each VCXO frequency
    SYSREF_CONFIG = {
        100e6: (
            {
                "SYSREF_FREQ": 2.5e6,
                "PDF": 50e3,
            },
            {
                "SYSREF_FREQ": 1.25e6,
                "PDF": 50e3,
            },
            {
                "SYSREF_FREQ": 0.625e6,
                "PDF": 25e3,
            },
            {
                "SYSREF_FREQ": 0.5e6,
                "PDF": 50e3,
            },
        ),
        122.88e6: (
            {
                "SYSREF_FREQ": 2.56e6,
                "PDF": 40e3,
            },
            {
                "SYSREF_FREQ": 1.28e6,
                "PDF": 40e3,
            },
            {
                "SYSREF_FREQ": 0.64e6,
                "PDF": 40e3,
            },
        ),
    }

    def __init__(self, pll_regs_iface, log=None):
        LMK04832.__init__(self, pll_regs_iface, log)
        self._output_freq = None
        self._prc_freq = None
        self.int_vco = 0x1

        self._sclk_pll_reset = Gpio("SAMPLE-CLOCK-PLL-RESET", Gpio.OUTPUT, 0)
        self._sclk_pll_select = Gpio("SAMPLE-CLOCK-PLL-VCXO-SELECT", Gpio.OUTPUT, 0)

    @property
    def output_freq(self):
        """Return the sample clock output frequency"""
        if self._output_freq is None:
            self.log.error(
                "The Sample PLL was never configured before " "checking the output frequency!"
            )
            raise RuntimeError(
                "The Sample PLL was never configured before " "checking the output frequency!"
            )
        return self._output_freq

    @property
    def prc_freq(self):
        """
        Return the PRC rate. This is the rate that the SPLL provides to the
        MMCM. On X410, the typical setup is such that the MMCM outputs the same
        clock frequency as PRC rate, but on other special cases, this
        SPLL might provide a different PRC frequency to the MMCM than the MMCM
        outputs to its downstream consumers of the PRC.
        """
        if self._prc_freq is None:
            self.log.error(
                "The Sample PLL was never configured before " "checking the PRC frequency!"
            )
            raise RuntimeError(
                "The Sample PLL was never configured before " "checking the PRC frequency!"
            )
        return self._prc_freq

    def init(self):
        """
        Perform a soft reset, configure SPI readback, and verify chip ID
        """
        self.reset(False, hard=True)
        self.reset(True)
        self.reset(False)
        if not self.verify_chip_id():
            raise Exception("unable to locate LMK04832!")

    def reset(self, value=True, hard=False):
        """
        Perform a hard reset from the GPIO pins or a soft reset from the LMK register
        """
        if hard:
            self._sclk_pll_reset.set(value)
        else:
            self.soft_reset(value)

        if not value:
            # Enable 4-wire spi readback after a reset. 4-wire SPI is disabled
            # by default after a reset of the LMK, but is required to perform
            # SPI reads on the x4xx.
            self.enable_4wire_spi()

    def set_vcxo(self, source_freq: Spll1Vco):
        """
        Selects either the 100e6 MHz or 122.88e6 MHz VCXO for the PLL1 loop of
        the LMK04832.
        """
        assert source_freq in Spll1Vco
        self.log.trace(f"Selected PLL1 VCXO source {source_freq.name}")
        self._sclk_pll_select.set(source_freq.value)

    def _get_clk_digital_delay(self, clock_divider=None):
        """
        Lookup the digital delay for a given clock divider according to table 18 of
        LMK04832 datasheet. Return a default value of 20 adjusted by the value from the table.
        A default value of '20' is chosen to allow the global + local sysref delay (both min 8)
        to sum up to this value (including small adjustments based on the clock divider).
        If no clock divider is given, the default value is returned.
        """
        digital_delay = 20

        # apply adjustments according to table 18 of LMK04832 datasheet
        # e.g. with a Dev. CLK divider of '2' the Data Sheet mentions an adjustment value
        # of '-2' which requires us to compensate by adding '+2' to the digital delay
        adjustments = {2: 2, 3: 2, 5: -3, 6: -1}

        digital_delay += adjustments.get(clock_divider, 0)

        return digital_delay

    def config(self, cfg):
        """
        Configures the LMK04832 according to the given configuration object.
        """
        self.log.trace("Applying configuration: %s", str(cfg))

        # Decide which internal VCO to take depending on frequency
        vco_freq = cfg.output_freq * cfg.output_divider
        if LMK04832.LMK_VCO0_RANGE_MIN <= vco_freq <= LMK04832.LMK_VCO0_RANGE_MAX:
            self.int_vco = 0x0
        elif LMK04832.LMK_VCO1_RANGE_MIN <= vco_freq <= LMK04832.LMK_VCO1_RANGE_MAX:
            self.int_vco = 0x1
        else:
            self.log.error(
                f"Desired LMK VCO frequency {vco_freq/1e6} MHz "
                "cannot be generated with any of the internal VCOs."
            )
            raise RuntimeError(
                f"Desired LMK VCO frequency {vco_freq/1e6} MHz "
                "cannot be generated with any of the internal VCOs."
            )

        self._output_freq = cfg.output_freq
        self._prc_freq = vco_freq / cfg.prc_divider

        self.log.trace(
            f"Configuring SPLL to output frequency of {cfg.output_freq} Hz, "
            f"BRC frequency is {cfg.ref_freq/1e6} MHz, "
            f"PRC rate is {self._prc_freq/1e6} MHz"
        )

        self.set_vcxo(cfg.vcxo_freq)

        # Clear hard reset and trigger soft reset
        self.reset(False, hard=True)
        self.reset(True, hard=False)
        self.reset(False, hard=False)

        prescaler = self.pll2_pre_to_reg(cfg.pll2_prescaler)
        # The lower nibble of the prescaler register defines OSCin_FREQ and
        # PLL2_REF_2X_EN. On X4x0, the former must always be ==1 (PLL2 ref input
        # is 63 MHz < f <= 127 MHz as there are two clock options (100 and 122.88 MHz)
        # to choose from) and the latter must be ==0 (double disabled).
        # This is not something we configure through the cfg object, so we check
        # it's the correct, valid value.
        assert (prescaler & 0x1F) == 0x4

        # Get digital delay values
        rfdc_clk_ddly = self._get_clk_digital_delay(cfg.output_divider)
        prc_clk_ddly = self._get_clk_digital_delay(cfg.prc_divider)

        # Sysref delay is derived from the formula in the LMK04832 datasheet chapter 8.3.5
        # Take the constant SYSREF SCLK digital delay from each clock pair
        sysref_delay = self._get_clk_digital_delay() - 1 - self.SCLK_DIG_DELAY
        self.log.trace(
            f"Global SYSREF Digital Delay: {sysref_delay}, "
            f"local SCLK Digital Delay: {self.SCLK_DIG_DELAY}, "
            f"PRC CLK Digital Delay: {prc_clk_ddly}, "
            f"RFDC CLK Digital Delay: {rfdc_clk_ddly}"
        )

        # The RFDC clock output dividers may be set to 2 or 3 depending on the output frequency.
        # According to the LMK04832 datasheet table 18, we need to program the dividers to a
        # value of '4' first. Afterwards we can set them to their actual divider value.
        # For simplification we are programming all registers which consume cfg.output_divider
        # to 4 first and don't require additional divider value checks.
        self.pokes8(
            (
                (0x0100, 0x04),
                (0x0108, 0x04),
                (0x0130, 0x04),
            )
        )

        # We need longer lines to properly document the PLL settings
        # fmt: off
        # pylint: disable=line-too-long
        # CLKout Config
        self.pokes8((
            # For the output divider we only use the first 8 bits as otherwise the
            # input frequency to the RFDC will get too small anyway.
            (0x0100, cfg.output_divider), # set Device CLKout 0 and 1 divider
            (0x0101, rfdc_clk_ddly), # set Digital Delay for CLKout 0 and 1
            (0x0102, 0x60), # enable CLKout 0 and 1, set output and input drive level high, enable digital delay for 0 and 1
            (0x0103, 0x44), # select Device clock output for 0, enable CLKout 0 and 1, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x0104, 0x11), # select Device Clock output for 1, power down SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x0105, 0x00), # disable analog SYSREF delay for 0 and 1, default analog delay value set to 125 ps
            (0x0106, 0x08), # set local SYSREF digital delay to 8, but it is not used here
            (0x0107, 0x55), # set CLK output 0 and 1 to LVPECL 2000 mV
            (0x0108, cfg.output_divider), # set Device CLKout 2 and 3 divider
            (0x0109, rfdc_clk_ddly), # set Digital Delay for CLKout 2 and 3
            (0x010A, 0x60), # enable CLKout 2 and 3, set output and input drive level high, enable digital delay for 2 and 3
            (0x010B, 0x44), # select Device clock output for 2, enable CLKout 2 and 3, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x010C, 0x11), # select Device Clock output for 3, power down SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x010D, 0x00), # disable analog SYSREF delay for 2 and 3, default analog delay value set to 125 ps
            (0x010E, 0x08), # set local SYSREF digital delay to 8, but it is not used here
            (0x010F, 0x55), # set CLK output 2 and 3 to LVPECL 2000 mV
            (0x0110, cfg.prc_divider), # set Device CLKout 4 and 5 divider
            (0x0111, prc_clk_ddly), # set Digital Delay for CLKout 4 and 5
            (0x0112, 0x60), # enable CLKout 4 and 5, set output and input drive level high, enable digital delay for 4 and 5
            (0x0113, 0x44), # select Device clock output for 4, enable CLKout 4 and 5, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x0114, 0x11), # select Device Clock output for 5, power down SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x0115, 0x00), # disable analog SYSREF delay for 4 and 5, default analog delay value set to 125 ps
            (0x0116, 0x08), # set local SYSREF digital delay to 8, but it is not used here
            (0x0117, 0x44), # set CLK output 4 and 5 to LVPECL 1600 mV
            (0x0118, cfg.prc_divider), # set Device CLKout 6 and 7 divider
            (0x0119, prc_clk_ddly), # set Digital Delay for CLKout 6 and 7
            (0x011A, 0x60 if (cfg.prc_to_db) else 0x80), # disables CLKout6_7 if prc_to_db==false, otherwise enable CLKout 6 and 7, set output and input drive level high, enable digital delay for 6 and 7
            (0x011B, 0x44), # select Device clock output for 6, enable CLKout 6 and 7, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x011C, 0x11), # select Device Clock output for 7, power down SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x011D, 0x00), # disable analog SYSREF delay for 6 and 7, default analog delay value set to 125 ps
            (0x011E, 0x08), # set local SYSREF digital delay to 8, but it is not used here
            (0x011F, 0x44), # set CLK output 6 and 7 to LVPECL 1600 mV
            (0x0120, cfg.prc_divider), # set Device CLKout 8 and 9 divider
            (0x0121, prc_clk_ddly), # set Digital Delay for CLKout 8 and 9
            (0x0122, 0x60), # enable CLKout 8 and 9, set output and input drive level high, enable digital delay for 8 and 9
            (0x0123, 0x54), # select Device clock output for 8, disable Device CLKout 8, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x0124, 0x21), # select SYSREF Clock output for 9, enable SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x0125, 0x00), # disable analog SYSREF delay for 8 and 9, default analog delay value set to 125 ps
            (0x0126, 0x08), # set local SYSREF SCLK digital delay to 8
            (0x0127, 0x44), # set CLK output 8 and 9 to LVPECL 1600 mV
            (0x0128, cfg.output_divider), # set Device CLKout 10 and 11 divider
            (0x0129, rfdc_clk_ddly), # set Digital Delay for CLKout 10 and 11
            (0x012A, 0x60), # enable CLKout 10 and 11, set output and input drive level high, enable digital delay for 10 and 11
            (0x012B, 0x74), # select SYSREF clock output for 10, disable CLKout 10 and 11, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x012C, 0x21), # select SYSREF Clock output for 11, enable SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x012D, 0x00), # disable analog SYSREF delay for 10 and 11, default analog delay value set to 125 ps
            (0x012E, 0x08), # set local SYSREF SCLK digital delay to 8
            (0x012F, 0x44), # set CLK output 10 and 11 to LVPECL 1600 mV
            (0x0130, cfg.output_divider), # set Device CLKout 12 and 13 divider
            (0x0131, rfdc_clk_ddly), # set Digital Delay for CLKout 12 and 13
            (0x0132, 0x60), # enable CLKout 12 and 13, set output and input drive level high, enable digital delay for 12 and 13
            (0x0133, 0x44), # select Device clock output for 12, enable CLKout 12 and 13, high performance bypass disabled, duty cycle correction enabled, normal polarity, no half step adjustment
            (0x0134, 0x11), # select Device Clock output for 13, power down SYSREF path, normal SYSREF disable mode, normal SYSREF polarity, enable half step adjustment
            (0x0135, 0x00), # disable analog SYSREF delay for 12 and 13, default analog delay value set to 125 ps
            (0x0136, 0x08), # set local SYSREF SCLK digital delay to 8, but it is not used here
            (0x0137, 0x55), # set CLK output 12 and 13 to LVPECL 2000 mV
        ))

        # PLL Config
        self.pokes8((
            (0x0138, (self.int_vco & 0x1) << 5), # VCO_MUX, choose VCO0 or VCO1, power down OSCout
            (0x0139, 0x00), # Set SysRef source to 'Normal SYNC' (SYSREF_MUX=0) as we initially use the sync signal to synchronize dividers, disable SYSREF_REQ pin, normal SYSREF polarity, normal SYNC
            (0x013A, (cfg.sysref_div & 0x1F00) >> 8), # SYSREF Divide [12:8], set global SYSREF divider
            (0x013B, (cfg.sysref_div & 0x00FF) >> 0), # SYSREF Divide [7:0], set global SYSREF divider
            (0x013C, (sysref_delay & 0x1F00) >> 8), # SYSREF DDLY [12:8], set global SYSREF digital delay
            (0x013D, (sysref_delay & 0x00FF) >> 0), # shift SYSREF according to LMK data sheet
            (0x013E, 0x03), # set number of SYSREF pulse to 8(Default)
            (0x013F, 0x0F), # PLL2_RCLK_MUX = OSCin, PLL2_NCLK_MUX = PLL2 Prescaler, PLL1_NCLK_MUX = Feedback mux, FB_MUX = External, FB_MUX_EN = enabled
            (0x0140, 0x00), # All power down controls set to false, enabled PLL1, VCO_LDO, VCO, OSCin, SYSREF, SYSREF Digital Delay, SYSREF Pulser
            (0x0141, 0x00), # Disable dynamic digital delay.
            (0x0142, 0x00), # Set dynamic digital delay step count to 0.
            # Initial SYNC configuration:
            # SYSREF_CLR=1 (reset SYSREF digital delay),
            # SYNC_1SHOT_EN=0 (SYNC is level sensitive, not edge sensitive),
            # SYNC_POL=0, SYNC_EN=0 (disable sync), SYNC_PLL{1,2}_DLD=0,
            # SYNC_MODE=1 (SYNC event generated from SYNC pin).
            # When initial configuration is done, we do the final SYNC config further down.
            (0x0143, 0x81), # See above ^^^
            (0x0144, 0x00), # Allow SYNC to synchronize all SysRef and clock output dividers
            (0x0145, 0x10), # Disable PLL1 R divider SYNC, use SYNC pin for PLL1 R divider SYNC, disable PLL2 R divider SYNC
            (0x0146, 0x00), # disable CLKin_SEL_PIN, set CLKin_SEL_PIN to normal polarity, disable CLKIn0/1/2 auto-switching mode, CLKIN0/1/2 type = Bipolar (differential)
            (0x0147, 0x06), # CLKin_SEL_MANUAL= CLKin0, ClkIn0_Demux = PLL1, CLKIn1-Demux = Feedback mux (need for 0-delay mode)
            (0x0148, 0x33), # CLKIn_SEL0_MUX = SPI readback with CLKin_SEL0_TYPE output set to push-pull
            (0x0149, 0x02), # Set SPI readback ouput to push-pull
            (0x014A, 0x00), # Set RESET pin as input
            (0x014B, 0x02), # Default
            (0x014C, 0x00), # Default
            (0x014D, 0x00), # Default
            (0x014E, 0xC0), # Default
            (0x014F, 0x7F), # Default
            (0x0150, 0x00), # Default and disable holdover
            (0x0151, 0x02), # Default
            (0x0152, 0x00), # Default
            (0x0153, (cfg.clkin0_r_div & 0x3F00) >> 8), # CLKin0_R divider [13:8], default = 0
            (0x0154, (cfg.clkin0_r_div & 0x00FF) >> 0), # CLKin0_R divider [7:0], default = d120
            (0x0155, 0x00), # Set CLKin1 R divider to 1
            (0x0156, 0x01), # Set CLKin1 R divider to 1
            (0x0157, 0x00), # Set CLKin2 R divider to 1
            (0x0158, 0x01), # Set CLKin2 R divider to 1
            (0x0159, (cfg.pll1_n_div & 0x3F00) >> 8), # PLL1 N divider [13:8], default = 0
            (0x015A, (cfg.pll1_n_div & 0x00FF) >> 0), # PLL1 N divider [7:0], default = d120
            (0x015B, 0xCF), # Set PLL1 window size to 43ns, PLL1 CP ON, negative polarity, CP gain is 1.55 mA.
            (0x015C, 0x20), # Pll1 lock detect count is 8192 cycles (default)
            (0x015D, 0x00), # Pll1 lock detect count is 8192 cycles (default)
            (0x015E, 0x1E), # Default holdover relative time between PLL1 R and PLL1 N divider of "30"
            (0x015F, 0x1B), # PLL1 and PLL2 locked status in Status_LD1 pin. Status_LD1 pin is output (push-pull)
            (0x0160, 0x00), # PLL2 R divider is 1
            (0x0161, 0x01), # PLL2 R divider is 1
            (0x0162, prescaler), # PLL2 prescaler; OSCin freq; Lower nibble must be 0x4!!!
            (0x0163, (cfg.pll2_n_cal_div & 0x030000) >> 16), # PLL2 N Cal [17:16]
            (0x0164, (cfg.pll2_n_cal_div & 0x00FF00) >> 8), # PLL2 N Cal [15:8]
            (0x0165, (cfg.pll2_n_cal_div & 0x0000FF) >> 0), # PLL2 N Cal [7:0]
            (0x0169, 0x59), # Write this val after x165. PLL2 CP gain is 3.2 mA, PLL2 window is 1.8 ns, negative CP polarity, PLL2_DLD is forced on
            (0x016A, 0x20), # PLL2 lock detect count is 8192 cycles (default)
            (0x016B, 0x00), # PLL2 lock detect count is 8192 cycles (default)
            (0x016E, 0x13), # Status_LD2 pin not used. Don't care about this register, select PLL2_DLD as output (push-pull)
            (0x0173, 0x10), # PLL2 prescaler and PLL2 are enabled.
            (0x0177, 0x00), # PLL1 R divider not in reset
            # programming the register 0x168 last as it triggers the VCO calibration routine
            (0x0166, (cfg.pll2_n_div & 0x030000) >> 16), # PLL2 N[17:16]
            (0x0167, (cfg.pll2_n_div & 0x00FF00) >> 8), # PLL2 N[15:8]
            (0x0168, (cfg.pll2_n_div & 0x0000FF) >> 0), # PLL2 N[7:0]
        ))

        # Synchronize Output and SYSREF Dividers. This is similar to the config example in the datasheet, section 8.3.3.1.
        self.pokes8((
            (0x0143, 0x91), # Set SYNC_EN=1
            (0x0143, 0xB1), # Toggle SYNC_POL on...
            (0x0143, 0x91), # ...and off again. This will sync dividers.
            (0x0144, 0xFF), # Prevent sysref and other clock outputs from being synchronized or interrupted by a SYNC event.
                            # Note that this will synchronize clock outputs on a single LMK, but not between devices.
                            # To do that, we still need to synchronize the PLL1 R-divider to the PPS.
            (0x0143, 0x11), # Now set SYNC_EN=1, SYNC_MODE=1, and clear SYSREF_CLR
            (0x0139, 0x03), # SYSREF_REQ_EN=0, SYSREF_MUX=3 (continuous SYSREF)
        ))
        # pylint: enable=line-too-long
        # fmt: on

        # Check for Lock
        # PLL2 should lock first and be relatively fast (300 us)
        if self.wait_for_pll_lock("PLL2", timeout=5):
            self.log.trace("PLL2 is locked after SPLL config.")
        else:
            self.log.error("Sample Clock PLL2 failed to lock!")
            raise RuntimeError("Sample Clock PLL2 failed to lock! " "Check the logs for details.")
        # PLL1 may take up to 2 seconds to lock
        if self.wait_for_pll_lock("PLL1", timeout=2000):
            self.log.trace("PLL1 is locked after SPLL config.")
        else:
            self.log.error("Sample Clock PLL1 failed to lock!")
            raise RuntimeError("Sample Clock PLL1 failed to lock! " "Check the logs for details.")
