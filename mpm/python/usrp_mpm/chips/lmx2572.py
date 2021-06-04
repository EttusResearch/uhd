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
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.chips.ic_reg_maps import lmx2572_regs_t

NUMBER_OF_LMX2572_REGISTERS = 126

class LMX2572(object):
    """
    Generic driver class for LMX2572 access.
    """

    READ_ONLY_REGISTERS = [107, 108, 109, 110, 111, 112, 113]

    def __init__(self, regs_iface, parent_log = None):
        self.log = parent_log

        self.regs_iface = regs_iface
        assert hasattr(self.regs_iface, 'peek16')
        assert hasattr(self.regs_iface, 'poke16')
        self._poke16 = regs_iface.poke16
        self._peek16 = regs_iface.peek16

        self._lmx2572_regs = lmx2572_regs_t()

        self._need_recalculation = True
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
        vco_phase_sync = lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE if \
            enable_synchronization else \
            lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_NORMAL_OPERATION
        self._lmx2572_regs.vco_phase_sync_en = vco_phase_sync
        self._need_recalculation = True

    def get_synchronization(self):
        """
        Returns the enabled/disabled state of the phase synchronization
        """
        return self._lmx2572_regs.vco_phase_sync_en == \
            lmx2572_regs_t.vco_phase_sync_en_t.VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE

    def set_output_enable_all(self, enable_output):
        """
        Enables or disables the output on both ports
        """
        self._set_output_a_enable(enable_output)
        self._set_output_b_enable(enable_output)

    def _set_chip_enable(self, chip_enable):
        """
        Enables or disables the LMX2572 using the powerdown register
            All other registers are maintained during powerdown
        """
        powerdown = lmx2572_regs_t.powerdown_t.POWERDOWN_NORMAL_OPERATION if chip_enable else \
            lmx2572_regs_t.powerdown_t.POWERDOWN_POWER_DOWN
        self._lmx2572_regs.powerdown = powerdown
        self._poke16(0, self._lmx2572_regs.get_reg(0))

    def peek16(self, address):
        """
        Wraps _peek16 to account for mux_ld_sel
        """
        # SPI MISO is multiplexed to lock detect and register readback. Set the mux to register 
        # readback before trying to read the register.
        self._lmx2572_regs.muxout_ld_sel = \
            lmx2572_regs_t.muxout_ld_sel_t.MUXOUT_LD_SEL_REGISTER_READBACK
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
        pass

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
        new_value = lmx2572_regs_t.outa_pd_t.OUTA_PD_NORMAL_OPERATION if enable_output \
            else lmx2572_regs_t.outa_pd_t.OUTA_PD_POWER_DOWN
        self._lmx2572_regs.outa_pd = new_value

    def _set_output_b_enable(self, enable_output):
        """
        Sets output B (OUTB_PD)
        """
        new_value = lmx2572_regs_t.outb_pd_t.OUTB_PD_NORMAL_OPERATION if enable_output \
            else lmx2572_regs_t.outb_pd_t.OUTB_PD_POWER_DOWN
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
        else: # 100MHz < phase_detector_frequency
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
        else: # phase_detector_frequency < 2.5MHz
            self._lmx2572_regs.fcal_lpfd_adj = 0x3

    def _set_pll_n(self, n):
        """
        Sets the pll_n registers
        """
        self._lmx2572_regs.pll_n_upper_3_bits = (n >> 16) & \
            self._lmx2572_regs.pll_n_upper_3_bits_mask
        self._lmx2572_regs.pll_n_lower_16_bits = n & self._lmx2572_regs.pll_n_lower_16_bits_mask

    def _set_pll_den(self, den):
        """
        Sets the pll_den registers
        """
        self._lmx2572_regs.pll_den_upper = (den >> 16) & self._lmx2572_regs.pll_den_upper_mask
        self._lmx2572_regs.pll_den_lower = den & self._lmx2572_regs.pll_den_lower_mask

    def _set_mash_seed(self, mash_seed):
        """
        Sets the mash seed register
        """
        self._lmx2572_regs.mash_seed_upper = (mash_seed >> 16) & \
            self._lmx2572_regs.mash_seed_upper_mask
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
        self._lmx2572_regs.mash_rst_count_upper = (mash_rst_count >> 16) & \
            self._lmx2572_regs.mash_rst_count_upper_mask
        self._lmx2572_regs.mash_rst_count_lower = mash_rst_count & \
            self._lmx2572_regs.mash_rst_count_lower_mask

    def _compute_and_set_mult_hi(self, reference_frequency):
        multiplier_output_frequency = (reference_frequency*(int(self._lmx2572_regs.osc_2x.value)\
            +1)*self._lmx2572_regs.mult) / self._lmx2572_regs.pll_r_pre
        new_mult_hi = lmx2572_regs_t.mult_hi_t.MULT_HI_GREATER_THAN_100M \
            if self._lmx2572_regs.mult > 1 and multiplier_output_frequency > 100e6 else \
            lmx2572_regs_t.mult_hi_t.MULT_HI_LESS_THAN_EQUAL_TO_100M
        self._lmx2572_regs.mult_hi = new_mult_hi

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
