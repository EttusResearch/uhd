#
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
LMK04828 parent driver class
"""

from builtins import object
from ..mpmlog import get_logger

class LMK04828(object):
    """
    Generic driver class for LMK04828 access.
    """
    LMK_CHIP_ID = 6

    def __init__(self, regs_iface, postfix=None):
        postfix = postfix or ""
        self.log = get_logger("LMK04828-{}".format(postfix))
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
        self.log.trace("Read chip ID: {}".format(chip_id))
        return chip_id

    def verify_chip_id(self):
        """
        Returns True if the chip ID matches what we expect, False otherwise.
        """
        chip_id = self.get_chip_id()
        if chip_id != self.LMK_CHIP_ID:
            self.log.error("Wrong chip id 0x{:X}".format(chip_id))
            return False
        return True

    def check_plls_locked(self):
        """
        Checks both PLLs are locked. Will throw an exception otherwise.
        Returns True if both PLLs are locked, False otherwise.
        """
        def check_pll_lock(pll_id, addr):
            """
            pll_id -- A string defining the PLL (e.g. 'PLL1')
            addr -- The address to peek to see if it's locked
            """
            pll_lock_status = self.regs_iface.peek8(addr)
            if (pll_lock_status & 0x7) != 0x02:
                self.log.warning("LMK {} reporting unlocked... Status: 0x{:x}".format(pll_id, pll_lock_status))
                return False
            return True
        lock_status = \
                check_pll_lock("PLL1", 0x182) and \
                check_pll_lock("PLL2", 0x183)
        return lock_status

