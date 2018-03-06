#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK04828 parent driver class
"""

import math
from builtins import object
from usrp_mpm.mpmlog import get_logger

class LMK04828(object):
    """
    Generic driver class for LMK04828 access.
    """
    LMK_CHIP_ID = 6

    def __init__(self, regs_iface, parent_log=None):
        self.log = \
            parent_log.getChild("LMK04828") if parent_log is not None \
            else get_logger("LMK04828")
        self.regs_iface = regs_iface
        assert hasattr(self.regs_iface, 'peek8')
        assert hasattr(self.regs_iface, 'poke8')
        self.poke8 = regs_iface.poke8
        self.peek8 = regs_iface.peek8

    def pokes8(self, addr_vals):
        """
        Apply a series of pokes.
        pokes8((0,1),(0,2)) is the same as calling poke8(0,1), poke8(0,2).
        """
        for addr, val in addr_vals:
            self.poke8(addr, val)

    def get_chip_id(self):
        """
        Read back the chip ID
        """
        chip_id = self.peek8(0x03)
        self.log.trace("Chip ID Readback: {}".format(chip_id))
        return chip_id

    def verify_chip_id(self):
        """
        Returns True if the chip ID matches what we expect, False otherwise.
        """
        chip_id = self.get_chip_id()
        if chip_id != self.LMK_CHIP_ID:
            self.log.error("Wrong Chip ID 0x{:X}".format(chip_id))
            return False
        return True

    def check_plls_locked(self):
        """
        Returns True if both PLLs are locked, False otherwise.
        """
        def check_pll_lock(pll_id, addr):
            """
            pll_id -- A string defining the PLL (e.g. 'PLL1')
            addr -- The address to peek to see if it's locked
            """
            pll_lock_status = self.regs_iface.peek8(addr)
            if (pll_lock_status & 0x7) != 0x02:
                self.log.debug("{} reporting unlocked... Status: 0x{:x}"
                               .format(pll_id, pll_lock_status))
                return False
            return True
        lock_status = \
                check_pll_lock("PLL1", 0x182) and \
                check_pll_lock("PLL2", 0x183)
        return lock_status


## Register bitfield definitions ##

    def divide_to_cnth_cntl_reg(self, divide_val):
        """
        From the divider value, returns the CNTL and CNTH register value.
        Split divider value in half. If odd, round up for the CNTL and down
        for the CNTH based on the datasheet recommendation.
        """
        cntl = int(math.ceil( divide_val/2.0))
        cnth = int(math.floor(divide_val/2.0))
        reg_val = ((cnth & 0xF) << 4) | (cntl & 0xF)
        self.log.trace("From divider value 0d{}, writing CNTH/L as 0x{:02X}."
                       .format(divide_val, reg_val))
        return reg_val

    def divide_to_reg(self, divide_val, in_drive=0x1, out_drive=0x1):
        """
        From the divider value, returns the register value combined with the other
        register fields.
        """
        reg_val = (divide_val & 0x1F) | ((in_drive & 0x1) << 5) | ((out_drive & 0x1) << 6)
        self.log.trace("From divider value 0d{}, writing divider register as 0x{:02X}."
                       .format(divide_val, reg_val))
        return reg_val

    def pll2_pre_to_reg(self, prescaler, osc_field=0x01, xtal_en=0x0, ref_2x_en=0x0):
        """
        From the prescaler value, returns the register value combined with the other
        register fields.
        """
        # valid prescaler values are 2-8, where 8 is represented as 0x00.
        assert prescaler in range(2, 8+1)
        reg_val = ((prescaler & 0x07) << 5) \
                  | ((osc_field & 0x7) << 2) \
                  | ((xtal_en & 0x1) << 1) \
                  | ((ref_2x_en & 0x1) << 0)
        self.log.trace("From prescaler value 0d{}, writing register as 0x{:02X}."
                       .format(prescaler, reg_val))
        return reg_val

