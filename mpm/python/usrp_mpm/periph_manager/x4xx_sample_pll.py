#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK04832 driver for use with X4xx
"""

from usrp_mpm.chips import LMK04832
from usrp_mpm.sys_utils.gpio import Gpio

class LMK04832X4xx(LMK04832):
    """
    X4xx-specific subclass of the Sample Clock PLL LMK04832 controls.
    """
    def __init__(self, pll_regs_iface, log=None):
        LMK04832.__init__(self, pll_regs_iface, log)
        self._output_freq = None
        self._is_legacy_mode = None

        self._sclk_pll_reset = Gpio('SAMPLE-CLOCK-PLL-RESET', Gpio.OUTPUT, 0)
        self._sclk_pll_select = Gpio('SAMPLE-CLOCK-PLL-VCXO-SELECT', Gpio.OUTPUT, 0)

    @property
    def is_legacy_mode(self):
        if self._is_legacy_mode is None:
            self.log.error('The Sample PLL was never configured before '
                           'checking for legacy mode!')
            raise RuntimeError('The Sample PLL was never configured before '
                               'checking for legacy mode!')
        return self._is_legacy_mode

    @property
    def output_freq(self):
        if self._output_freq is None:
            self.log.error('The Sample PLL was never configured before '
                           'checking the output frequency!')
            raise RuntimeError('The Sample PLL was never configured before '
                               'checking the output frequency!')
        return self._output_freq

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

    def enable_4wire_spi(self):
        """ Enable 4-wire SPI readback from the CLKin_SEL0 pin """
        self.poke8(0x148, 0x33)
        self.enable_3wire_spi = False

    def set_vcxo(self, source_freq):
        """
        Selects either the 100e6 MHz or 122.88e6 MHz VCXO for the PLL1 loop of the LMK04832.
        """
        if source_freq == 100e6:
            source_index = 0
        elif source_freq == 122.88e6:
            source_index = 1
        else:
            self.log.warning(
                'Selected VCXO source of {:g} is not a valid selection'
                .format(source_freq))
            return
        self.log.trace(
            'Selected VCXO source of {:g}'
            .format(source_freq))
        self._sclk_pll_select.set(source_index)

    def get_status(self):
        """
        Returns PLL lock status
        """
        pll1_status = self.check_plls_locked(pll='PLL1')
        pll2_status = self.check_plls_locked(pll='PLL2')
        return {'PLL1 lock': pll1_status,
                'PLL2 lock': pll2_status}

    def config(self, output_freq, brc_freq, is_legacy_mode=False):
        """
        Configures the LMK04832 to generate the desired output_freq
        """
        def calculate_vcxo_freq(output_freq):
            """
            Returns the vcxo frequency based on the desired output frequency
            """
            return {2.94912e9: 122.88e6, 3e9: 100e6, 3.072e9: 122.88e6}[output_freq]
        def calculate_pll1_n_div(output_freq):
            """
            Returns the PLL1 N divider value based on the desired output frequency
            """
            return {2.94912e9: 64, 3e9: 50, 3.072e9: 64}[output_freq]
        def calculate_pll2_n_div(output_freq):
            """
            Returns the PLL2 N divider value based on the desired output frequency
            """
            return {2.94912e9: 12, 3e9: 10, 3.072e9: 5}[output_freq]
        def calculate_pll2_pre(output_freq):
            """
            Returns the PLL2 prescaler value based on the desired output frequency
            """
            return {2.94912e9: 2, 3e9: 3, 3.072e9: 5}[output_freq]
        def calculate_n_cal_div(output_freq):
            """
            Returns the PLL2 N cal value based on the desired output frequency
            """
            return {2.94912e9: 12, 3e9: 10, 3.072e9: 5}[output_freq]
        def calculate_sysref_div(output_freq):
            """
            Returns the SYSREF divider value based on the desired output frequency
            """
            return {2.94912e9: 1152, 3e9: 1200, 3.072e9: 1200}[output_freq]
        def calculate_clk_in_0_r_div(output_freq, brc_freq):
            """
            Returns the CLKin0 R divider value based on the desired output frequency
            and current base reference clock frequency
            """
            pfd1 = {2.94912e9: 40e3, 3e9: 50e3, 3.072e9: 40e3}[output_freq]
            return int(brc_freq / pfd1)

        if output_freq not in (2.94912e9, 3e9, 3.072e9):
            # A failure to config the SPLL could lead to an invalid state for
            # downstream clocks, so throw here to alert the caller.
            raise RuntimeError(
                'Selected output_freq of {:g} is not a valid selection'
                .format(output_freq))

        self._is_legacy_mode = is_legacy_mode
        self._output_freq = output_freq

        self.log.trace(
            f"Configuring SPLL to output frequency of {output_freq} Hz, used "
            f"BRC frquency is {brc_freq} Hz, legacy mode is {is_legacy_mode}")

        self.set_vcxo(calculate_vcxo_freq(output_freq))

        # Clear hard reset and trigger soft reset
        self.reset(False, hard=True)
        self.reset(True, hard=False)
        self.reset(False, hard=False)

        prc_divider = 0x3C if is_legacy_mode else 0x30

        # CLKout Config
        self.pokes8((
            (0x0100, 0x01),
            (0x0101, 0x0A),
            (0x0102, 0x70),
            (0x0103, 0x44),
            (0x0104, 0x10),
            (0x0105, 0x00),
            (0x0106, 0x00),
            (0x0107, 0x55),
            (0x0108, 0x01),
            (0x0109, 0x0A),
            (0x010A, 0x70),
            (0x010B, 0x44),
            (0x010C, 0x10),
            (0x010D, 0x00),
            (0x010E, 0x00),
            (0x010F, 0x55),
            (0x0110, prc_divider),
            (0x0111, 0x0A),
            (0x0112, 0x60),
            (0x0113, 0x40),
            (0x0114, 0x10),
            (0x0115, 0x00),
            (0x0116, 0x00),
            (0x0117, 0x44),
            (0x0118, prc_divider),
            (0x0119, 0x0A),
            (0x011A, 0x60),
            (0x011B, 0x40),
            (0x011C, 0x10),
            (0x011D, 0x00),
            (0x011E, 0x00),
            (0x011F, 0x44),
            (0x0120, prc_divider),
            (0x0121, 0x0A),
            (0x0122, 0x60),
            (0x0123, 0x40),
            (0x0124, 0x20),
            (0x0125, 0x00),
            (0x0126, 0x00),
            (0x0127, 0x44),
            (0x0128, 0x01),
            (0x0129, 0x0A),
            (0x012A, 0x60),
            (0x012B, 0x60),
            (0x012C, 0x20),
            (0x012D, 0x00),
            (0x012E, 0x00),
            (0x012F, 0x44),
            (0x0130, 0x01),
            (0x0131, 0x0A),
            (0x0132, 0x70),
            (0x0133, 0x44),
            (0x0134, 0x10),
            (0x0135, 0x00),
            (0x0136, 0x00),
            (0x0137, 0x55),
        ))

        # PLL Config
        sysref_div = calculate_sysref_div(output_freq)
        clk_in_0_r_div = calculate_clk_in_0_r_div(output_freq, brc_freq)
        pll1_n_div = calculate_pll1_n_div(output_freq)
        prescaler = self.pll2_pre_to_reg(calculate_pll2_pre(output_freq))
        pll2_n_cal_div = calculate_n_cal_div(output_freq)
        pll2_n_div = calculate_pll2_n_div(output_freq)
        self.pokes8((
            (0x0138, 0x20),
            (0x0139, 0x00), # Set SysRef source to 'Normal SYNC' as we initially use the sync signal to synchronize dividers
            (0x013A, (sysref_div & 0x1F00) >> 8), # SYSREF Divide [12:8]
            (0x013B, (sysref_div & 0x00FF) >> 0), # SYSREF Divide [7:0]
            (0x013C, 0x00), # set sysref delay value
            (0x013D, 0x20), # shift SYSREF with respect to falling edge of data clock
            (0x013E, 0x03), # set number of SYSREF pulse to 8(Default)
            (0x013F, 0x0F), # PLL1_NCLK_MUX = Feedback mux, FB_MUX = External, FB_MUX_EN = enabled
            (0x0140, 0x00), # All power down controls set to false.
            (0x0141, 0x00), # Disable dynamic digital delay.
            (0x0142, 0x00), # Set dynamic digtial delay step count to 0.
            (0x0143, 0x81), # Enable SYNC pin, disable sync functionality, SYSREF_CLR='0, SYNC is level sensitive.
            (0x0144, 0x00), # Allow SYNC to synchronize all SysRef and clock outputs
            (0x0145, 0x10), # Disable PLL1 R divider SYNC, use SYNC pin for PLL1 R divider SYNC, disable PLL2 R divider SYNC
            (0x0146, 0x00), # CLKIN0/1 type = Bipolar, disable CLKin_sel pin, disable both CLKIn source for auto-switching.
            (0x0147, 0x06), # ClkIn0_Demux= PLL1, CLKIn1-Demux=Feedback mux (need for 0-delay mode)
            (0x0148, 0x33), # CLKIn_Sel0 = SPI readback with output set to push-pull
            (0x0149, 0x02), # Set SPI readback ouput to open drain (needed for 4-wire)
            (0x014A, 0x00), # Set RESET pin as input
            (0x014B, 0x02), # Default
            (0x014C, 0x00), # Default
            (0x014D, 0x00), # Default
            (0x014E, 0xC0), # Default
            (0x014F, 0x7F), # Default
            (0x0150, 0x00), # Default and disable holdover
            (0x0151, 0x02), # Default
            (0x0152, 0x00), # Default
            (0x0153, (clk_in_0_r_div & 0x3F00) >> 8), # CLKin0_R divider [13:8], default = 0
            (0x0154, (clk_in_0_r_div & 0x00FF) >> 0), # CLKin0_R divider [7:0], default = d120
            (0x0155, 0x00), # Set CLKin1 R divider to 1
            (0x0156, 0x01), # Set CLKin1 R divider to 1
            (0x0157, 0x00), # Set CLKin2 R divider to 1
            (0x0158, 0x01), # Set CLKin2 R divider to 1
            (0x0159, (pll1_n_div & 0x3F00) >> 8), # PLL1 N divider [13:8], default = 0
            (0x015A, (pll1_n_div & 0x00FF) >> 0), # PLL1 N divider [7:0], default = d120
            (0x015B, 0xCF), # Set PLL1 window size to 43ns, PLL1 CP ON, negative polarity, CP gain is 1.55 mA.
            (0x015C, 0x20), # Pll1 lock detect count is 8192 cycles (default)
            (0x015D, 0x00), # Pll1 lock detect count is 8192 cycles (default)
            (0x015E, 0x1E), # Default holdover relative time between PLL1 R and PLL1 N divider
            (0x015F, 0x1B), # PLL1 and PLL2 locked status in Status_LD1 pin. Status_LD1 pin is ouput (push-pull)
            (0x0160, 0x00), # PLL2 R divider is 1
            (0x0161, 0x01), # PLL2 R divider is 1
            (0x0162, prescaler), # PLL2 prescaler; OSCin freq; Lower nibble must be 0x4!!!
            (0x0163, (pll2_n_cal_div & 0x030000) >> 16), # PLL2 N Cal [17:16]
            (0x0164, (pll2_n_cal_div & 0x00FF00) >> 8), # PLL2 N Cal [15:8]
            (0x0165, (pll2_n_cal_div & 0x0000FF) >> 0), # PLL2 N Cal [7:0]
            (0x0169, 0x59), # Write this val after x165. PLL2 CP gain is 3.2 mA, PLL2 window is 1.8 ns
            (0x016A, 0x20), # PLL2 lock detect count is 8192 cycles (default)
            (0x016B, 0x00), # PLL2 lock detect count is 8192 cycles (default)
            (0x016E, 0x13), # Stautus_LD2 pin not used. Don't care about this register
            (0x0173, 0x10), # PLL2 prescaler and PLL2 are enabled.
            (0x0177, 0x00), # PLL1 R divider not in reset
            (0x0166, (pll2_n_div & 0x030000) >> 16), # PLL2 N[17:16]
            (0x0167, (pll2_n_div & 0x00FF00) >> 8), # PLL2 N[15:8]
            (0x0168, (pll2_n_div & 0x0000FF) >> 0), # PLL2 N[7:0]
        ))

        # Synchronize Output and SYSREF Dividers
        self.pokes8((
            (0x0143, 0x91),
            (0x0143, 0xB1),
            (0x0143, 0x91),
            (0x0144, 0xFF),
            (0x0143, 0x11),
            (0x0139, 0x12),
            (0x0143, 0x31),
        ))

        # Check for Lock
        # PLL2 should lock first and be relatively fast (300 us)
        if self.wait_for_pll_lock('PLL2', timeout=5):
            self.log.trace("PLL2 is locked after SPLL config.")
        else:
            self.log.error('Sample Clock PLL2 failed to lock!')
            raise RuntimeError('Sample Clock PLL2 failed to lock! '
                               'Check the logs for details.')
        # PLL1 may take up to 2 seconds to lock
        if self.wait_for_pll_lock('PLL1', timeout=2000):
            self.log.trace("PLL1 is locked after SPLL config.")
        else:
            self.log.error('Sample Clock PLL1 failed to lock!')
            raise RuntimeError('Sample Clock PLL1 failed to lock! '
                               'Check the logs for details.')
