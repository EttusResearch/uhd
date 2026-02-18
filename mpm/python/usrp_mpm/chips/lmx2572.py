#
# Copyright 2019-2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMX2572 parent driver class
"""

import math
from builtins import object

from usrp_mpm.chips.ic_reg_maps.lmx2572_regs import lmx2572_regs_t

# Highest LO / output frequency
MAX_OUT_FREQ = 6.4e9  # Hz
# Lowest LO / output frequency
MIN_OUT_FREQ = 12.5e6  # Hz
NUMBER_OF_LMX2572_REGISTERS = 126
# Target loop bandwidth
TARGET_LOOP_BANDWIDTH = 75e3  # Hz
# Loop Filter gain setting resistor
LOOP_GAIN_SETTING_RESISTANCE = 150  # ohm


class LMX2572(object):
    """
    Generic driver class for LMX2572 access.
    """

    READ_ONLY_REGISTERS = [107, 108, 109, 110, 111, 112, 113]

    def __init__(self, poke_fnct, peek_fnct, parent_log=None):
        self.log = parent_log

        self._poke16 = poke_fnct
        self._peek16 = peek_fnct

        self._lmx2572_regs = lmx2572_regs_t()

        self._need_recalculation = False
        self._enabled = False

    @property
    def enabled(self):
        return self._enabled

    @enabled.setter
    def enabled(self, enable):
        self._set_chip_enable(bool(enable))
        self._enabled = bool(enable)

    def reset(self):
        """
        Performs a reset of the LMX2572 by using the software reset register.
        """
        self._lmx2572_regs = lmx2572_regs_t()
        self._lmx2572_regs.reset = lmx2572_regs_t.reset_t.RESET_RESET
        self._poke16(0, self._lmx2572_regs.get_reg(0))
        self._lmx2572_regs.reset = lmx2572_regs_t.reset_t.RESET_NORMAL_OPERATION
        self._set_default_values()
        self._power_up_sequence()
        self._lmx2572_regs.save_state()

    def commit(self):
        """
        Calculates the settings when needed and writes the settings to the device
        """
        if self._need_recalculation:
            self._calculate_settings()
            self._need_recalculation = False
        self._write_registers_reference_chain()
        self._write_registers_frequency_tuning()

    def check_pll_locked(self):
        """
        Returns True if the PLL is locked, False otherwise.
        """
        # SPI MISO is multiplexed to lock detect and register readback. Reading any
        # register when the mux is set to the lock detect will return just the lock detect signal
        self._lmx2572_regs.muxout_ld_sel = lmx2572_regs_t.muxout_ld_sel_t.MUXOUT_LD_SEL_LOCK_DETECT
        self._poke16(0, self._lmx2572_regs.get_reg(0))

        # If the PLL is locked we expect to read 0xFFFF from any read, else 0x0000
        value_read = self._peek16(0)

        return value_read == 0xFFFF

    def set_synchronization(self, enable_synchronization):
        """
        Enables and disables the phase synchronization
        """
        vco_phase_sync = (
            lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE
            if enable_synchronization
            else lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_NORMAL_OPERATION
        )
        self._lmx2572_regs.vco_phase_sync_en = vco_phase_sync
        self._need_recalculation = True

    def get_synchronization(self):
        """
        Returns the enabled/disabled state of the phase synchronization
        """
        return (
            self._lmx2572_regs.vco_phase_sync_en
            == lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE
        )

    def set_output_enable_all(self, enable_output):
        """
        Enables or disables the output on both ports
        """
        self._set_output_a_enable(enable_output)
        self._set_output_b_enable(enable_output)

    def set_output_power(self, output, power):
        """
        Sets the output power for the specified output
        """
        if output == "RF_OUTPUT_A":
            self._set_output_a_power(power)
        elif output == "RF_OUTPUT_B":
            self._set_output_b_power(power)
        else:
            raise ValueError("Invalid output specified")

    def set_frequency(self, target_freq, fOSC, spur_dodging):
        """
        Sets the frequency of the LMX2572.
        """
        # Sanity check
        if target_freq > MAX_OUT_FREQ or target_freq < MIN_OUT_FREQ:
            raise ValueError("Invalid LMX2572 target frequency!")

        if fOSC not in {61.44e6, 64e6, 62.5e6, 50e6, 46.08e6}:
            raise ValueError("Invalid fOSC value!")

        # Create an integer version of fOSC for some of the following calculations
        fOSC_int = int(fOSC)

        # 1. Set up output/channel divider value and the output mux
        out_D = self._set_output_divider(target_freq)
        fVCO = target_freq * out_D
        assert 3200e6 <= fVCO <= 6400e6

        # 2. Configure the reference dividers/multipliers
        self._set_pll_div_and_mult(target_freq, fVCO, fOSC_int)

        # Calculate phase detector frequency
        fPD = (
            fOSC
            * (self._lmx2572_regs.osc_2x.value + 1)
            * self._lmx2572_regs.mult
            / (self._lmx2572_regs.pll_r_pre * self._lmx2572_regs.pll_r)
        )

        # pre-3. Identify SYNC category.
        p = self._set_phase_sync(
            self._get_sync_cat(self._lmx2572_regs.mult, fOSC, target_freq, out_D)
        )

        # 3. Calculate N, PLL_NUM and PLL_DEN
        delta_fVCO = 2e6 if spur_dodging else 1.0
        PLL_DEN = max(1, min(math.ceil(fPD * p / delta_fVCO), 0xFFFFFFFF))
        assert PLL_DEN > 0

        N_real = fVCO / (fPD * p)
        N = int(math.floor(N_real))
        PLL_NUM = round((N_real - N) * PLL_DEN)

        fVCO_actual = fPD * p * (N + (PLL_NUM / PLL_DEN))
        assert 3200e6 <= fVCO_actual <= 6400e6
        actual_freq = fVCO_actual / out_D

        # 4. Set frequency dependent registers
        self._compute_and_set_mult_hi(fOSC)
        self._set_pll_n(N)
        self._set_pll_num(PLL_NUM)
        self._set_pll_den(PLL_DEN)
        self._set_fcal_hpfd_adj(fPD)
        self._set_fcal_lpfd_adj(fPD)
        self._set_pfd_dly(fVCO_actual)
        self._set_mash_seed(spur_dodging, PLL_NUM, fPD)

        if self.get_synchronization():
            period = (1 if self._lmx2572_regs.cal_clk_div == 0 else 2) / fOSC
            mash_rst_count = math.ceil(4 * 200e-6 / period)
            self._set_mash_rst_count(mash_rst_count)

        # 5. Calculate charge pump gain
        self._compute_and_set_charge_pump_gain(fVCO_actual, N_real)

        # 6. Calculate VCO calibration values
        self._compute_and_set_vco_cal(fVCO_actual)

        # 7. Set amplitude on enabled outputs
        if self._get_output_enabled("RF_OUTPUT_A"):
            self._find_and_set_lo_power(actual_freq, "RF_OUTPUT_A")
        if self._get_output_enabled("RF_OUTPUT_B"):
            self._find_and_set_lo_power(actual_freq, "RF_OUTPUT_B")

        return actual_freq

    def _set_output_divider(self, freq):
        """
        Sets the output divider registers.
        """
        out_div_map = {
            25e6: (256, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_256),
            50e6: (128, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_128),
            100e6: (64, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_64),
            200e6: (32, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_32),
            400e6: (16, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_16),
            800e6: (8, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_8),
            1.6e9: (4, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_4),
            3.2e9: (2, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_2),
            6.4e9 + 1: (1, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_2),
        }

        out_D, chdiv = next(
            (v for k, v in out_div_map.items() if freq <= k),
            (1, lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_2),
        )
        self._lmx2572_regs.chdiv = chdiv

        input_mux = (
            lmx2572_regs_t.outa_mux_t.OUTA_MUX_CHANNEL_DIVIDER
            if out_D > 1
            else lmx2572_regs_t.outa_mux_t.OUTA_MUX_VCO
        )
        if self._get_output_enabled("RF_OUTPUT_A"):
            self._lmx2572_regs.outa_mux = input_mux
        if self._get_output_enabled("RF_OUTPUT_B"):
            self._lmx2572_regs.outb_mux = input_mux

        return out_D

    def _set_pll_div_and_mult(self, target_freq, fVCO, fOSC_int):
        """
        Sets the PLL divider and multiplier values.
        """
        self._lmx2572_regs.pll_r_pre = 1
        self._lmx2572_regs.mult = 1
        self._lmx2572_regs.osc_2x = lmx2572_regs_t.osc_2x_t.OSC_2X_DISABLED

        pll_r = 1
        if self.get_synchronization() and target_freq < 3200e6:
            if fOSC_int == 61440000:
                pll_r = 2 if 3200e6 <= fVCO < 3950e6 else 1
            elif fOSC_int == 64000000:
                pll_r = 2 if 3200e6 <= fVCO < 4100e6 else 1
            elif fOSC_int == 62500000:
                pll_r = 2 if 3200e6 <= fVCO < 4000e6 else 1
            elif fOSC_int == 50000000:
                pll_r = 1

        self._lmx2572_regs.pll_r = pll_r

    def _set_phase_sync(self, cat):
        """
        Sets the phase synchronization mode.
        """
        P = 1
        self._lmx2572_regs.vco_phase_sync_en = (
            lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_NORMAL_OPERATION
        )

        if cat == "CAT1A":
            pass
        elif cat == "CAT1B":
            self._lmx2572_regs.vco_phase_sync_en = (
                lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE
            )
            P = 2
        elif cat == "CAT2":
            pass
        elif cat == "CAT3":
            if (
                self._lmx2572_regs.outa_mux == lmx2572_regs_t.outa_mux_t.OUTA_MUX_CHANNEL_DIVIDER
                or self._lmx2572_regs.outb_mux == lmx2572_regs_t.outb_mux_t.OUTB_MUX_CHANNEL_DIVIDER
            ):
                P = 2
            self._lmx2572_regs.vco_phase_sync_en = (
                lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE
            )
        elif cat == "CAT4":
            raise ValueError("PLL programming does not allow reliable phase synchronization!")
        elif cat == "NONE":
            pass
        else:
            raise ValueError("Invalid sync category")

        return P

    def _get_sync_cat(self, M, fOSC, fOUT, CHDIV):
        """
        Identify sync category according to Section 8.1.6 of the datasheet.
        """
        if not self.get_synchronization():
            return "NONE"

        if M > 1:
            if CHDIV > 2:
                return "CAT4"
            if fOUT % (fOSC * M) != 0:
                return "CAT4"

        if M == 1 and fOUT % fOSC != 0:
            return "CAT3"
        if M == 1 and CHDIV > 2:
            return "CAT2"
        if CHDIV == 2:
            return "CAT1B"
        return "CAT1A"

    def _set_chip_enable(self, chip_enable):
        """
        Enables or disables the LMX2572 using the powerdown register
            All other registers are maintained during powerdown
        """
        powerdown = (
            lmx2572_regs_t.powerdown_t.POWERDOWN_NORMAL_OPERATION
            if chip_enable
            else lmx2572_regs_t.powerdown_t.POWERDOWN_POWER_DOWN
        )
        self._lmx2572_regs.powerdown = powerdown
        self._poke16(0, self._lmx2572_regs.get_reg(0))

    def peek16(self, address):
        """
        Wraps _peek16 to account for mux_ld_sel
        """
        # SPI MISO is multiplexed to lock detect and register readback. Set the mux to register
        # readback before trying to read the register.
        self._lmx2572_regs.muxout_ld_sel = (
            lmx2572_regs_t.muxout_ld_sel_t.MUXOUT_LD_SEL_REGISTER_READBACK
        )
        self._poke16(0, self._lmx2572_regs.get_reg(0))

        value_read = self._peek16(address)

        self._lmx2572_regs.muxout_ld_sel = lmx2572_regs_t.muxout_ld_sel_t.MUXOUT_LD_SEL_LOCK_DETECT
        self._poke16(0, self._lmx2572_regs.get_reg(0))

        return value_read

    def _calculate_settings(self):
        """
        This function is intended to be called for calculating the register settings,
            however it is implementation, not chip specific, so it is defined but not implemented
        """
        raise NotImplementedError("This function is meant to be overriden by a child class.")

    def _set_default_values(self):
        """
        The register map has all base class defaults.
        Subclasses can override this function to have the values populated on reset.
        """
        self._lmx2572_regs.ramp_en = lmx2572_regs_t.ramp_en_t.RAMP_EN_NORMAL_OPERATION
        self._lmx2572_regs.vco_phase_sync_en = (
            lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_NORMAL_OPERATION
        )
        self._lmx2572_regs.add_hold = 0
        self._lmx2572_regs.out_mute = lmx2572_regs_t.out_mute_t.OUT_MUTE_MUTED
        self._lmx2572_regs.fcal_hpfd_adj = 1
        self._lmx2572_regs.fcal_lpfd_adj = 0
        self._lmx2572_regs.fcal_en = lmx2572_regs_t.fcal_en_t.FCAL_EN_ENABLE
        self._lmx2572_regs.muxout_ld_sel = lmx2572_regs_t.muxout_ld_sel_t.MUXOUT_LD_SEL_LOCK_DETECT
        self._lmx2572_regs.reset = lmx2572_regs_t.reset_t.RESET_NORMAL_OPERATION
        self._lmx2572_regs.powerdown = lmx2572_regs_t.powerdown_t.POWERDOWN_NORMAL_OPERATION

        self._lmx2572_regs.cal_clk_div = 0

        self._lmx2572_regs.ipbuf_type = lmx2572_regs_t.ipbuf_type_t.IPBUF_TYPE_DIFFERENTIAL
        self._lmx2572_regs.ipbuf_term = lmx2572_regs_t.ipbuf_term_t.IPBUF_TERM_INTERNALLY_TERMINATED

        self._lmx2572_regs.out_force = lmx2572_regs_t.out_force_t.OUT_FORCE_USE_OUT_MUTE

        self._lmx2572_regs.vco_daciset_force = (
            lmx2572_regs_t.vco_daciset_force_t.VCO_DACISET_FORCE_NORMAL_OPERATION
        )
        self._lmx2572_regs.vco_capctrl_force = (
            lmx2572_regs_t.vco_capctrl_force_t.VCO_CAPCTRL_FORCE_NORMAL_OPERATION
        )
        self._lmx2572_regs.vco_sel_force = lmx2572_regs_t.vco_sel_force_t.VCO_SEL_FORCE_DISABLED
        self._lmx2572_regs.vco_daciset_strt = 0x096
        self._lmx2572_regs.vco_sel = 0x6
        self._lmx2572_regs.vco_capctrl_strt = 0

        self._lmx2572_regs.mult_hi = lmx2572_regs_t.mult_hi_t.MULT_HI_LESS_THAN_EQUAL_TO_100M
        self._lmx2572_regs.osc_2x = lmx2572_regs_t.osc_2x_t.OSC_2X_DISABLED

        self._lmx2572_regs.mult = 1

        self._lmx2572_regs.pll_r = 1

        self._lmx2572_regs.pll_r_pre = 1

        self._lmx2572_regs.cpg = 7

        self._lmx2572_regs.pll_n_upper_3_bits = 0
        self._lmx2572_regs.pll_n_lower_16_bits = 0x28

        self._lmx2572_regs.mash_seed_en = lmx2572_regs_t.mash_seed_en_t.MASH_SEED_EN_ENABLED
        self._lmx2572_regs.pfd_dly_sel = 0x2

        self._lmx2572_regs.pll_den_upper = 0
        self._lmx2572_regs.pll_den_lower = 0

        self._lmx2572_regs.mash_seed_upper = 0
        self._lmx2572_regs.mash_seed_lower = 0

        self._lmx2572_regs.pll_num_upper = 0
        self._lmx2572_regs.pll_num_lower = 0

        self._lmx2572_regs.outa_pwr = 0
        self._lmx2572_regs.outb_pd = lmx2572_regs_t.outb_pd_t.OUTB_PD_POWER_DOWN
        self._lmx2572_regs.outa_pd = lmx2572_regs_t.outa_pd_t.OUTA_PD_POWER_DOWN
        self._lmx2572_regs.mash_reset_n = (
            lmx2572_regs_t.mash_reset_n_t.MASH_RESET_N_NORMAL_OPERATION
        )
        self._lmx2572_regs.mash_order = lmx2572_regs_t.mash_order_t.MASH_ORDER_THIRD_ORDER

        self._lmx2572_regs.outa_mux = lmx2572_regs_t.outa_mux_t.OUTA_MUX_VCO
        self._lmx2572_regs.outb_pwr = 0

        self._lmx2572_regs.outb_mux = lmx2572_regs_t.outb_mux_t.OUTB_MUX_VCO

        self._lmx2572_regs.inpin_ignore = 0
        self._lmx2572_regs.inpin_hyst = lmx2572_regs_t.inpin_hyst_t.INPIN_HYST_DISABLED
        self._lmx2572_regs.inpin_lvl = lmx2572_regs_t.inpin_lvl_t.INPIN_LVL_INVALID
        self._lmx2572_regs.inpin_fmt = (
            lmx2572_regs_t.inpin_fmt_t.INPIN_FMT_SYNC_EQUALS_SYSREFREQ_EQUALS_CMOS2
        )

        self._lmx2572_regs.ld_type = lmx2572_regs_t.ld_type_t.LD_TYPE_VTUNE_AND_VCOCAL

        self._lmx2572_regs.ld_dly = 100
        self._lmx2572_regs.ldo_dly = 3

        self._lmx2572_regs.dblbuf_en_0 = lmx2572_regs_t.dblbuf_en_0_t.DBLBUF_EN_0_ENABLED
        self._lmx2572_regs.dblbuf_en_1 = lmx2572_regs_t.dblbuf_en_1_t.DBLBUF_EN_1_ENABLED
        self._lmx2572_regs.dblbuf_en_2 = lmx2572_regs_t.dblbuf_en_2_t.DBLBUF_EN_2_ENABLED
        self._lmx2572_regs.dblbuf_en_3 = lmx2572_regs_t.dblbuf_en_3_t.DBLBUF_EN_3_ENABLED
        self._lmx2572_regs.dblbuf_en_4 = lmx2572_regs_t.dblbuf_en_4_t.DBLBUF_EN_4_ENABLED
        self._lmx2572_regs.dblbuf_en_5 = lmx2572_regs_t.dblbuf_en_5_t.DBLBUF_EN_5_ENABLED

        self._lmx2572_regs.mash_rst_count_upper = 0
        self._lmx2572_regs.mash_rst_count_lower = 0

        self._lmx2572_regs.chdiv = lmx2572_regs_t.chdiv_t.CHDIV_DIVIDE_BY_2

        self._lmx2572_regs.ramp_thresh_33rd = 0
        self._lmx2572_regs.quick_recal_en = 0

        self._lmx2572_regs.ramp_thresh_upper = 0
        self._lmx2572_regs.ramp_thresh_lower = 0

        self._lmx2572_regs.ramp_limit_high_33rd = 0
        self._lmx2572_regs.ramp_limit_high_upper = 0
        self._lmx2572_regs.ramp_limit_high_lower = 0

        self._lmx2572_regs.ramp_limit_low_33rd = 0
        self._lmx2572_regs.ramp_limit_low_upper = 0
        self._lmx2572_regs.ramp_limit_low_lower = 0

        self._lmx2572_regs.reg29_reserved0 = 0
        self._lmx2572_regs.reg30_reserved0 = 0x18A6
        self._lmx2572_regs.reg52_reserved0 = 0x421
        self._lmx2572_regs.reg57_reserved0 = 0x20
        self._lmx2572_regs.reg78_reserved0 = 1

    def _pokes16(self, addr_vals):
        """
        Apply a series of pokes.
        pokes16((0,1),(0,2)) is the same as calling poke16(0,1), poke16(0,2).
        """
        for addr, val in addr_vals:
            self._poke16(addr, val)

    def _set_output_a_enable(self, enable_output):
        """
        Sets output A (OUTA_PD)
        """
        new_value = (
            lmx2572_regs_t.outa_pd_t.OUTA_PD_NORMAL_OPERATION
            if enable_output
            else lmx2572_regs_t.outa_pd_t.OUTA_PD_POWER_DOWN
        )
        self._lmx2572_regs.outa_pd = new_value

    def _set_output_b_enable(self, enable_output):
        """
        Sets output B (OUTB_PD)
        """
        new_value = (
            lmx2572_regs_t.outb_pd_t.OUTB_PD_NORMAL_OPERATION
            if enable_output
            else lmx2572_regs_t.outb_pd_t.OUTB_PD_POWER_DOWN
        )
        self._lmx2572_regs.outb_pd = new_value

    def _set_output_a_power(self, power):
        """
        Sets the output A power
        """
        self._lmx2572_regs.outa_pwr = power & self._lmx2572_regs.outa_pwr_mask

    def _set_output_b_power(self, power):
        """
        Sets the output B power
        """
        self._lmx2572_regs.outb_pwr = power & self._lmx2572_regs.outb_pwr_mask

    def _set_fcal_hpfd_adj(self, phase_detector_frequency):
        """
        Sets the FCAL_HPFD_ADJ value based on the Fpfd
        """
        # These magic number frequency constants are from the data sheet
        if phase_detector_frequency <= 37.5e6:
            self._lmx2572_regs.fcal_hpfd_adj = 0x0
        elif 37.5e6 < phase_detector_frequency <= 75e6:
            self._lmx2572_regs.fcal_hpfd_adj = 0x1
        elif 75e6 < phase_detector_frequency <= 100e6:
            self._lmx2572_regs.fcal_hpfd_adj = 0x2
        else:  # 100MHz < phase_detector_frequency
            self._lmx2572_regs.fcal_hpfd_adj = 0x3

    def _set_fcal_lpfd_adj(self, phase_detector_frequency):
        """
        Sets the FCAL_LPFD_ADJ value based on the Fpfd
        """
        # These magic number frequency constants are from the data sheet
        if phase_detector_frequency >= 10e6:
            self._lmx2572_regs.fcal_lpfd_adj = 0x0
        elif 10e6 > phase_detector_frequency >= 5e6:
            self._lmx2572_regs.fcal_lpfd_adj = 0x1
        elif 5e6 > phase_detector_frequency >= 2.5e6:
            self._lmx2572_regs.fcal_lpfd_adj = 0x2
        else:  # phase_detector_frequency < 2.5MHz
            self._lmx2572_regs.fcal_lpfd_adj = 0x3

    def _set_pll_n(self, n):
        """
        Sets the pll_n registers
        """
        self._lmx2572_regs.pll_n_upper_3_bits = (
            n >> 16
        ) & self._lmx2572_regs.pll_n_upper_3_bits_mask
        self._lmx2572_regs.pll_n_lower_16_bits = n & self._lmx2572_regs.pll_n_lower_16_bits_mask

    def _set_pll_den(self, den):
        """
        Sets the pll_den registers
        """
        self._lmx2572_regs.pll_den_upper = (den >> 16) & self._lmx2572_regs.pll_den_upper_mask
        self._lmx2572_regs.pll_den_lower = den & self._lmx2572_regs.pll_den_lower_mask

    def _set_mash_seed(self, spur_dodging, PLL_NUM, fPD):
        """
        Sets the mash seed value based on fPD and whether spur dodging is enabled
        """
        mash_seed = 0
        if not spur_dodging and PLL_NUM != 0:
            seed_map = {
                25e6: 4999,
                30.72e6: 5531,
                31.25e6: 5591,
                32e6: 5657,
                50e6: 7096,
                61.44e6: 7841,
                62.5e6: 7907,
                64e6: 7993,
            }
            mash_seed = seed_map[min(seed_map, key=lambda x: abs(x - fPD))]

        self._lmx2572_regs.mash_seed_upper = (
            mash_seed >> 16
        ) & self._lmx2572_regs.mash_seed_upper_mask
        self._lmx2572_regs.mash_seed_lower = mash_seed & self._lmx2572_regs.mash_seed_lower_mask

    def _set_pll_num(self, num):
        """
        Sets the pll_num registers
        """
        self._lmx2572_regs.pll_num_upper = (num >> 16) & self._lmx2572_regs.pll_num_upper_mask
        self._lmx2572_regs.pll_num_lower = num & self._lmx2572_regs.pll_num_lower_mask

    def _set_mash_rst_count(self, mash_rst_count):
        """
        Sets the mash_rst_count registers
        """
        self._lmx2572_regs.mash_rst_count_upper = (
            mash_rst_count >> 16
        ) & self._lmx2572_regs.mash_rst_count_upper_mask
        self._lmx2572_regs.mash_rst_count_lower = (
            mash_rst_count & self._lmx2572_regs.mash_rst_count_lower_mask
        )

    def _compute_and_set_charge_pump_gain(self, fVCO_actual, N_real):
        """
        Compute and set charge pump gain register
        """
        # Table 135 (VCO Gain)
        vco_gain_map = {
            3.65e9: (3.2e9, 3.65e9, 1, 32, 47),
            4.2e9: (3.65e9, 4.2e9, 2, 35, 54),
            4.65e9: (4.2e9, 4.65e9, 3, 47, 64),
            5.2e9: (4.65e9, 5.2e9, 4, 50, 73),
            5.75e9: (5.2e9, 5.75e9, 5, 61, 82),
            6.4e9: (5.75e9, 6.4e9, 6, 57, 79),
        }

        fmin, fmax, _, KvcoMin, KvcoMax = vco_gain_map[
            min(k for k in vco_gain_map if k >= fVCO_actual)
        ]
        Kvco = ((KvcoMax - KvcoMin) / (fmax - fmin)) * (fVCO_actual - fmin) + KvcoMin

        # Calculate the optimal charge pump current (uA)
        icp = 2 * math.pi * TARGET_LOOP_BANDWIDTH * N_real / (Kvco * LOOP_GAIN_SETTING_RESISTANCE)

        # Table 2 (Charge Pump Gain)
        cpg_map = {
            0: 0,
            625: 1,
            1250: 2,
            1875: 3,
            2500: 4,
            3125: 5,
            3750: 6,
            4375: 7,
            5000: 12,
            5625: 13,
            6250: 14,
            6875: 15,
        }

        cpg = cpg_map.get(min(cpg_map, key=lambda x: abs(x - icp)))
        self._lmx2572_regs.cpg = cpg

    def _compute_and_set_vco_cal(self, fVCO_actual):
        """
        Compute and set VCO calibration values
        """
        # Table 136
        vco_partial_assist_map = {
            3.65e9: (3.2e9, 3.65e9, 1, 131, 19, 138, 137),
            4.2e9: (3.65e9, 4.2e9, 2, 143, 25, 162, 142),
            4.65e9: (4.2e9, 4.65e9, 3, 135, 34, 126, 114),
            5.2e9: (4.65e9, 5.2e9, 4, 136, 25, 195, 172),
            5.75e9: (5.2e9, 5.75e9, 5, 133, 20, 190, 163),
            6.4e9: (5.75e9, 6.4e9, 6, 151, 27, 256, 204),
        }

        fmin, fmax, VCO_CORE, Cmin, Cmax, Amin, Amax = vco_partial_assist_map[
            min(k for k in vco_partial_assist_map if k >= fVCO_actual)
        ]

        VCO_CAPCTRL_STRT = round(Cmin - (fVCO_actual - fmin) * (Cmin - Cmax) / (fmax - fmin))
        VCO_CAPCTRL_STRT = min(VCO_CAPCTRL_STRT, 183)  # VCO_CAPCTRL_STRT_MAX

        VCO_DACISET_STRT = round(Amin - (fVCO_actual - fmin) * (Amin - Amax) / (fmax - fmin))
        VCO_DACISET_STRT = min(VCO_DACISET_STRT, 511)  # VCO_DACISET_STRT_MAX

        self._lmx2572_regs.vco_sel = VCO_CORE
        self._lmx2572_regs.vco_capctrl_strt = VCO_CAPCTRL_STRT
        self._lmx2572_regs.vco_daciset_strt = VCO_DACISET_STRT

    def _get_output_enabled(self, output):
        """
        Returns the output enabled status of the specified output
        """
        if output == "RF_OUTPUT_A":
            return self._lmx2572_regs.outa_pd == lmx2572_regs_t.outa_pd_t.OUTA_PD_NORMAL_OPERATION
        elif output == "RF_OUTPUT_B":
            return self._lmx2572_regs.outb_pd == lmx2572_regs_t.outb_pd_t.OUTB_PD_NORMAL_OPERATION
        else:
            raise ValueError("Invalid output specified")

    def _find_and_set_lo_power(self, freq, output):
        if freq < 3e9:
            self.set_output_power(output, 25)
        elif 3e9 <= freq < 4e9:
            slope = 5.0
            segment_range = 1e9
            power_base = 25
            offset = freq - 3e9
            power = round(power_base + ((offset / segment_range) * slope))
            self.set_output_power(output, power)
        elif 4e9 <= freq < 5e9:
            slope = 10.0
            segment_range = 1e9
            power_base = 30
            offset = freq - 4e9
            power = round(power_base + ((offset / segment_range) * slope))
            self.set_output_power(output, power)
        elif 5e9 <= freq < 6.4e9:
            slope = 5 / 1.4
            segment_range = 1.4e9
            power_base = 40
            offset = freq - 5e9
            power = round(power_base + ((offset / segment_range) * slope))
            self.set_output_power(output, power)
        elif freq >= 6.4e9:
            self.set_output_power(output, 45)
        else:
            raise ValueError("Invalid frequency value")

    def _compute_and_set_mult_hi(self, reference_frequency):
        multiplier_output_frequency = (
            reference_frequency
            * (int(self._lmx2572_regs.osc_2x.value) + 1)
            * self._lmx2572_regs.mult
        ) / self._lmx2572_regs.pll_r_pre
        new_mult_hi = (
            lmx2572_regs_t.mult_hi_t.MULT_HI_GREATER_THAN_100M
            if self._lmx2572_regs.mult > 1 and multiplier_output_frequency > 100e6
            else lmx2572_regs_t.mult_hi_t.MULT_HI_LESS_THAN_EQUAL_TO_100M
        )
        self._lmx2572_regs.mult_hi = new_mult_hi

    def _set_pfd_dly(self, fVCO_actual):
        """
        Sets the PFD Delay value based on fVCO
        """
        assert (
            self._lmx2572_regs.mash_order == self._lmx2572_regs.mash_order_t.MASH_ORDER_THIRD_ORDER
        )
        # These constants / values come from the data sheet (Table 3)
        if 3.2e9 <= fVCO_actual < 4e9:
            self._lmx2572_regs.pfd_dly_sel = 2
        elif 4e9 <= fVCO_actual < 4.9e9:
            self._lmx2572_regs.pfd_dly_sel = 2
        elif 4.9e9 <= fVCO_actual <= 6.4e9:
            self._lmx2572_regs.pfd_dly_sel = 3
        else:
            raise ValueError("Invalid fVCO_actual value")

    def _power_up_sequence(self):
        """
        Performs the intial register writes for the LMX2572
        """
        for register in reversed(range(NUMBER_OF_LMX2572_REGISTERS)):
            if register in LMX2572.READ_ONLY_REGISTERS:
                continue
            self._poke16(register, self._lmx2572_regs.get_reg(register))

    def _write_registers_frequency_tuning(self):
        """
        This function writes just the registers for frequency tuning
        """
        # Write PLL_N to registers R34 and R36
        self._poke16(34, self._lmx2572_regs.get_reg(34))
        self._poke16(36, self._lmx2572_regs.get_reg(36))
        # Write PLL_DEN to registers R38 and R39
        self._poke16(38, self._lmx2572_regs.get_reg(38))
        self._poke16(39, self._lmx2572_regs.get_reg(39))
        # Write PLL_NUM to registers R42 and R43
        self._poke16(42, self._lmx2572_regs.get_reg(42))
        self._poke16(43, self._lmx2572_regs.get_reg(43))

        # MASH_SEED to registers R40 and R41
        self._poke16(40, self._lmx2572_regs.get_reg(40))
        self._poke16(41, self._lmx2572_regs.get_reg(41))

        # Write OUTA_PWR to register R44 or OUTB_PWR to register R45
        # Write OUTA_MUX to register R45 and/or OUTB_MUX to register R46
        self._poke16(44, self._lmx2572_regs.get_reg(44))
        self._poke16(45, self._lmx2572_regs.get_reg(45))
        self._poke16(46, self._lmx2572_regs.get_reg(46))

        # Write CHDIV to register R75
        self._poke16(75, self._lmx2572_regs.get_reg(75))

        # Write CPG to register R14
        self._poke16(14, self._lmx2572_regs.get_reg(14))

        # Write PFD_DLY_SEL to register R37
        self._poke16(37, self._lmx2572_regs.get_reg(37))

        # Write VCO_SEL to register R20
        self._poke16(20, self._lmx2572_regs.get_reg(20))

        # Write VCO_DACISET_STRT to register R17
        self._poke16(17, self._lmx2572_regs.get_reg(17))

        # Write VCO_CALCTRL_STRT to register R78
        self._poke16(78, self._lmx2572_regs.get_reg(78))

        # Write R0 to latch double buffered registers
        self._poke16(0, self._lmx2572_regs.get_reg(0))
        self._poke16(0, self._lmx2572_regs.get_reg(0))

    def _write_registers_reference_chain(self):
        """
        This function writes the registers that are used for setting the reference chain
        """
        # Write FCAL_HPFD_ADJ to register R0
        # Write FCAL_LPFD_ADJ to register R0
        self._poke16(0, self._lmx2572_regs.get_reg(0))

        # Write MULT_HI to register R9
        # Write OSC_2X to register R9
        self._poke16(9, self._lmx2572_regs.get_reg(9))

        # Write MULT to register R10
        self._poke16(10, self._lmx2572_regs.get_reg(10))

        # Write PLL_R to register R11
        self._poke16(11, self._lmx2572_regs.get_reg(11))
        # Write PLL_R_PRE to register R12
        self._poke16(12, self._lmx2572_regs.get_reg(12))

        # if Phase SYNC being used:
        # Write MASH_RST_COUNT to registers R69 and 70
        if self.get_synchronization():
            self._poke16(70, self._lmx2572_regs.get_reg(70))
            self._poke16(69, self._lmx2572_regs.get_reg(69))
