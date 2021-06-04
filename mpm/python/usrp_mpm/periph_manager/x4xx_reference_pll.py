#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK03328 driver for use with X4xx
"""

from time import sleep
from usrp_mpm.chips import LMK03328
from usrp_mpm.sys_utils.gpio import Gpio

class LMK03328X4xx(LMK03328):
    """
    X4xx-specific subclass of the Reference Clock PLL LMK03328 controls.
    """
    def __init__(self, pll_regs_iface, log=None):
        LMK03328.__init__(self, pll_regs_iface, log)

        self._pll_status_0 = Gpio('REFERENCE-CLOCK-PLL-STATUS-0', Gpio.INPUT)
        self._pll_status_1 = Gpio('REFERENCE-CLOCK-PLL-STATUS-1', Gpio.INPUT)
        self._pll_pwrdown_n = Gpio('REFERENCE-CLOCK-PLL-PWRDOWN', Gpio.OUTPUT, 1)

        self._reference_rates = None

    @property
    def reference_rates(self):
        """
        Gets a list of reference source rates indexed by [primary, secondary]
        """
        return self._reference_rates

    @reference_rates.setter
    def reference_rates(self, reference_rates):
        """
        Sets a list of reference source rates indexed by [primary, secondary]
        """
        assert len(reference_rates) == 2, 'Invalid number of reference rates'
        self._reference_rates = reference_rates

    def init(self):
        """
        Perform a soft reset and verify chip ID
        """
        # Clear hard reset
        self.reset(False, hard=True)

        # Enable sync mute
        self.poke8(0x0C, 0xC8)

        # Trigger soft reset
        self.reset(True, hard=False)
        self.reset(False, hard=False)

        if not self.verify_chip_id():
            raise Exception("unable to locate LMK03328!")

    def reset(self, value=True, hard=False):
        """
        Perform a hard reset from the GPIO pins or a soft reset from the LMK register
        """
        if hard:
            # The powerdown pin is active low
            self._pll_pwrdown_n.set(not value)
        else:
            self.soft_reset(value)

    def get_status(self):
        """
        Returns PLL lock and outgoing status indicators for the LMK03328
        """
        status_indicator_0 = self._pll_status_0.get()
        status_indicator_1 = self._pll_status_1.get()
        status_indicator = (status_indicator_1 << 1) | status_indicator_0
        return {'PLL1 lock': self.check_pll_locked(1),
                'PLL2 lock': self.check_pll_locked(2),
                'status indicator': status_indicator}

    def config(self, ref_select=2, brc_rate=25e6, usr_clk_rate=156.25e6, brc_select='PLL'):
        """
        Configure the RPLL to generate the desired MGT Reference clock sources
        using the specified internal BRC.
        ref_select - the reference source to use (primary=1, secondary=2)
        brc_rate   - specifies the desired rate of the output BRC
        usr_clk_rate - specifies the desired rate to configure PLL1
        brc_select - specifies whether the BRC out should be from the PLL ('PLL') or
                     a passthrough of the primary reference signal ('bypass')
        """
        def calculate_out7_mux(brc_select):
            """
            Returns the OUT7 Mux select register value based on the chosen BRC source.
            Note that OUT7 is wired to the InternalRef clock which is used as the default
            reference clock source.
            """
            return {'bypass': 0x98, 'PLL': 0x58}[brc_select]
        def calculate_out7_div(brc_rate):
            """ Returns the OUT7 Divider register value based on the chosen BRC rate """
            return {25e6: 0x31, 125e6: 0x09}[brc_rate]
        def calculate_pll2_input_select(ref_select):
            """ Returns the reference mux register value based on which reference should be used """
            assert ref_select in (1, 2)
            return {1: 0x5B, 2: 0x7F}[ref_select]
        def calculate_pll2_n_div(ref_rate):
            """ Returns the PLL2 N div value based on the rate of the reference source """
            return {25e6: 0x00C8, 100e6: 0x0032}[ref_rate]
        def calculate_pll1_post_div(usr_clk_rate):
            """ Returns the PLL1 post div value based the usr_clk_rate """
            assert usr_clk_rate in (156.25e6, 125e6, 312.5e6, 161.1328125e6)
            return {
                156.25e6:       0x0E,
                125e6:          0x0E,
                312.5e6:        0x0E,
                161.1328125e6:  0x1E,
            }[usr_clk_rate]
        def calculate_pll1_n_div(usr_clk_rate):
            """ Returns the PLL1 N div value based the usr_clk_rate """
            assert usr_clk_rate in (156.25e6, 125e6, 312.5e6, 161.1328125e6)
            return {
                156.25e6:       0x0339,
                125e6:          0x0339,
                312.5e6:        0x0032,
                161.1328125e6:  0x0339,
            }[usr_clk_rate]
        def calculate_pll1_m_div(usr_clk_rate):
            """ Returns the PLL1 M div value based the usr_clk_rate """
            assert usr_clk_rate in (156.25e6, 125e6, 312.5e6, 161.1328125e6)
            return {
                156.25e6:       0x0F,
                125e6:          0x0F,
                312.5e6:        0x00,
                161.1328125e6:  0x0F,
            }[usr_clk_rate]
        def calculate_pll_select(usr_clk_rate):
            """ Returns the PLL selection based on the usr_clk_rate """
            assert usr_clk_rate in (156.25e6, 125e6)
            return {156.25e6: 2, 125e6: 2}[usr_clk_rate]
        def get_register_from_pll(pll_selection, addr):
            """ Returns the value to write to a specified register given
            the desired PLL selection. """
            assert pll_selection in (1, 2)
            assert addr in (0x22, 0x29)
            return {0x22: [0x00, 0x80], 0x29: [0x10, 0x50]}[addr][pll_selection-1]
        def calculate_out_div(usr_clk_rate):
            """ Returns the output divider for a given clock rate """
            assert usr_clk_rate in (156.25e6, 125e6)
            return {156.25e6: 0x07, 125e6: 0x09}[usr_clk_rate]

        if self._reference_rates is None:
            self.log.error('Cannot config reference PLL until the reference sources are set.')
            raise RuntimeError('Cannot config reference PLL until the reference sources are set.')
        if ref_select not in (1, 2):
            raise RuntimeError('Selected reference source {} is invalid'.format(ref_select))
        ref_rate = self._reference_rates[ref_select-1]
        if ref_rate not in (25e6, 100e6):
            raise RuntimeError('Selected reference rate {} Hz is invalid'.format(ref_rate))
        if brc_select not in ('bypass', 'PLL'):
            raise RuntimeError('Selected BRC source {} is invalid'.format(brc_select))
        if brc_rate not in (25e6, 125e6):
            raise RuntimeError('Selected BRC rate {} Hz is invalid'.format(brc_rate))
        if brc_select == 'bypass':
            # 'bypass' sends the primary reference directly to out7
            actual_brc_rate = self._reference_rates[0]
            if actual_brc_rate != brc_rate:
                self.log.error('The specified BRC rate does not match the actual '
                               'rate of the primary ref in bypass mode.')
                raise RuntimeError('The specified BRC rate does not match the actual '
                                   'rate of the primary ref in bypass mode.')
        if usr_clk_rate not in (156.25e6, 125e6):
            raise RuntimeError('Selected RPLL clock rate {} Hz is not supported'.format(usr_clk_rate))

        self.log.trace("Configuring RPLL to ref:{}, brc:{} {} Hz, clock rate:{}"
                       .format(ref_select, brc_select, brc_rate, usr_clk_rate))
        # Config
        pll2_input_mux = calculate_pll2_input_select(ref_select)
        pll2_n_div = calculate_pll2_n_div(ref_rate)
        pll1_post_div = calculate_pll1_post_div(usr_clk_rate)
        pll1_n_div = calculate_pll1_n_div(usr_clk_rate)
        pll1_m_div = calculate_pll1_m_div(usr_clk_rate)
        pll_select = calculate_pll_select(usr_clk_rate)
        out_div = calculate_out_div(usr_clk_rate)
        out7_mux = calculate_out7_mux(brc_select)
        out7_div = calculate_out7_div(brc_rate)

        self.pokes8((
            (0x0C, 0xDF),
            (0x0D, 0x00),
            (0x0E, 0x00),
            (0x0F, 0x00),
            (0x10, 0x00),
            (0x11, 0x00),
            (0x12, 0x00),
            (0x13, 0x00),
            (0x14, 0xFF),
            (0x15, 0xFF),
            (0x16, 0xFF),
            (0x17, 0x00), # Status 0/1 mute control is disabled. Both status always ON.
            (0x18, 0x00),
            (0x19, 0x55),
            (0x1A, 0x00),
            (0x1B, 0x58),
            (0x1C, 0x58),
            (0x1D, 0x8F),
            (0x1E, 0x01),
            (0x1F, 0x00),
            (0x20, 0x00),
            (0x21, 0x00),
            (0x22, get_register_from_pll(pll_select, 0x22)),
            (0x23, 0x20),
            (0x24, out_div),
            (0x25, 0xD0),
            (0x26, 0x00),
            (0x27, 0xD0),
            (0x28, 0x09),
            (0x29, get_register_from_pll(pll_select, 0x29)),
            (0x2A, out_div),
            (0x2B, out7_mux),
            (0x2C, out7_div),
            (0x2D, 0x0A), # Disable all PLL divider status outputs. Both status pins are set to normal operation.
            (0x2E, 0x00), # Disable all PLL divider status outputs.
            (0x2F, 0x00), # Disable all PLL divider status outputs.
            (0x30, 0xFF), # Hidden register. Value from TICS software.
            (0x31, 0x0A), # set both status slew rate to slow (2.1 ns)
            (0x32, pll2_input_mux),
            (0x33, 0x03),
            (0x34, 0x00),
            (0x35, pll1_m_div),
            (0x36, 0x00),
            (0x37, 0x00),
            (0x38, pll1_post_div), # PLL1 enabled, PLL1 output reset sync enable
            (0x39, 0x08),
            (0x3A, (pll1_n_div & 0x0F00) >> 8), # PLL1 N Divider [11:8]
            (0x3B, (pll1_n_div & 0x00FF) >> 0), # PLL1 N Divider [7:0]
            (0x3C, 0x00),
            (0x3D, 0x00),
            (0x3E, 0x00),
            (0x3F, 0x00),
            (0x40, 0x00),
            (0x41, 0x01),
            (0x42, 0x0C),
            (0x43, 0x08), # PLL1 loop filter R2 = 735 ohms
            (0x44, 0x00), # PLL1 loop filter C1 = 5 pF
            (0x45, 0x00), # PLL1 loop filter R3 = 18 ohms
            (0x46, 0x00), # PLL1 loop filter C3 = 0 pF
            (0x47, 0x0E),
            (0x48, 0x08),
            (0x49, (pll2_n_div & 0x0F00) >> 8), # PLL2 N Divider [11:8]
            (0x4A, (pll2_n_div & 0x00FF) >> 0), # PLL2 N Divider [7:0]
            (0x4B, 0x00),
            (0x4C, 0x00),
            (0x4D, 0x00),
            (0x4E, 0x00),
            (0x4F, 0x00),
            (0x50, 0x01),
            (0x51, 0x0C),
            (0x52, 0x08),
            (0x53, 0x00),
            (0x54, 0x00),
            (0x55, 0x00),
            (0x56, 0x08),
            (0x57, 0x00),
            (0x58, 0x00),
            (0x59, 0xDE),
            (0x5A, 0x01),
            (0x5B, 0x18),
            (0x5C, 0x01),
            (0x5D, 0x4B),
            (0x5E, 0x01),
            (0x5F, 0x86),
            (0x60, 0x01),
            (0x61, 0xBE),
            (0x62, 0x01),
            (0x63, 0xFE),
            (0x64, 0x02),
            (0x65, 0x47),
            (0x66, 0x02),
            (0x67, 0x9E),
            (0x68, 0x00),
            (0x69, 0x00),
            (0x6A, 0x05),
            (0x6B, 0x0F),
            (0x6C, 0x0F),
            (0x6D, 0x0F),
            (0x6E, 0x0F),
            (0x6F, 0x00),
            (0x70, 0x00),
            (0x71, 0x00),
            (0x72, 0x00),
            (0x73, 0x08),
            (0x74, 0x19),
            (0x75, 0x00),
            (0x76, 0x03), # PLL1 uses 2nd order loop filter recommended for integer PLL mode.
            (0x77, 0x01),
            (0x78, 0x00),
            (0x79, 0x0F),
            (0x7A, 0x0F),
            (0x7B, 0x0F),
            (0x7C, 0x0F),
            (0x7D, 0x00),
            (0x7E, 0x00),
            (0x7F, 0x00),
            (0x80, 0x00),
            (0x81, 0x08),
            (0x82, 0x19),
            (0x83, 0x00),
            (0x84, 0x03), # PLL2 uses 2nd order loop filter recommended for integer PLL mode.
            (0x85, 0x01),
            (0x86, 0x00),
            (0x87, 0x00),
            (0x88, 0x00),
            (0x89, 0x10),
            (0x8A, 0x00),
            (0x8B, 0x00),
            (0x8C, 0x00),
            (0x8D, 0x00),
            (0x8E, 0x00),
            (0x8F, 0x00),
            (0x90, 0x00),
            (0x91, 0x00),
            (0xA9, 0x40),
            (0xAC, 0x24),
            (0xAD, 0x00),
            (0x0C, 0x5F), # Initiate VCO calibration
            (0x0C, 0xDF),
        ))
        # wait for VCO calibration to be done and PLL to lock
        sleep(0.5)
        # Reset all output and PLL post dividers
        self.pokes8((
            (0x0C, 0x9F),
            (0x0C, 0xDF)
        ))

        # Check for Lock
        if not self.check_pll_locked(1):
            raise RuntimeError('PLL1 did not lock!')
        if not self.check_pll_locked(2):
            raise RuntimeError('PLL2 did not lock!')
        self.log.trace("PLLs are locked!")
