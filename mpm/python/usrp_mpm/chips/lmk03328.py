#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK03328 parent driver class
"""

from usrp_mpm.mpmlog import get_logger

class LMK03328():
    """
    Generic driver class for LMK03328 access.
    """
    LMK_CHIP_ID = 0x32

    def __init__(self, regs_iface, parent_log=None):
        self.log = \
            parent_log.getChild("LMK03328") if parent_log is not None \
            else get_logger("LMK03328")
        self.regs_iface = regs_iface
        assert hasattr(self.regs_iface, 'peek8')
        assert hasattr(self.regs_iface, 'poke8')
        self.poke8 = regs_iface.poke8
        self.peek8 = regs_iface.peek8

    def pokes8(self, addr_vals):
        """
        Apply a series of pokes.
        pokes8([(0,1),(0,2)]) is the same as calling poke8(0,1), poke8(0,2).
        """
        for addr, val in addr_vals:
            self.poke8(addr, val)

    def get_chip_id(self):
        """
        Read back the chip ID
        """
        chip_id = self.peek8(0x02)
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

    def check_pll_locked(self, pll_num):
        """
        Returns True if the specified PLL is locked, False otherwise.
        """
        if pll_num not in (1, 2):
            self.log.warning("PLL{} is not a valid PLL"
                             .format(pll_num))
            return False
        pll_lock_status = self.peek8(13)

        # Lock status for PLL1 is bits [7:6] and PLL2 is bits [4:3]
        pll_lock_mask = 0xC0 if pll_num == 1 else 0x18

        if (pll_lock_status & pll_lock_mask) != 0x0:
            self.log.debug("PLL{} reporting unlocked... Status: 0x{:x}"
                           .format(pll_num, pll_lock_status))
            return False
        return True

    def soft_reset(self, value=True):
        """
        Performs a soft reset of the LMK03328 by setting or unsetting the software
        reset register. The adjacent contents of the register are preserved by
        reading their value prior to re-writing the corresponding byte.
        """
        reset_bit = 0x80
        reset_addr = 12
        old_reg_val = self.peek8(reset_addr)
        # The reset register is active low so if value is True, write 0. Otherwise, write 1
        new_reg_val = (old_reg_val & (~reset_bit)) if value else (old_reg_val | reset_bit)
        self.poke8(reset_addr, new_reg_val)
