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
JESD FPGA Core Interface
"""

import time
from builtins import hex
from builtins import object
from .mpmlog import get_logger

class NIMgJESDCore(object):
    """
    Provide interface for the FPGA JESD Core.
    Works with Magnesium/Mykonos daughterboards only.

    Arguments:
    regs -- regs class to use for peek/poke
    """
    def __init__(self, regs, side=None):
        side = side or "-0"
        self.regs = regs
        self.log = get_logger('NIMgJESDCore'+side)
        assert hasattr(self.regs, 'peek32')
        assert hasattr(self.regs, 'poke32')

    def unreset_qpll(self):
        # new_val = self.regs.peek32(0x0) & ~0x8
        # self.log.trace("Unresetting MMCM, writing value {:X}".format(new_val))
        self.regs.poke32(0x0, 0x7)

    def check_core(self):
        """
        Verify JESD core returns correct ID
        """
        self.log.trace("Checking JESD Core...")
        if self.regs.peek32(0x2100) != 0x4A455344:
            raise Exception('JESD Core signature mismatch! Check that core is mapped correctly')
        #if self.regs.peek32(0x2104) != 0xFF
        #error here for date revision mismatch
        self.log.trace("JESD Core build code: {0}".format(hex(self.regs.peek32(0x2104))))
        return True

    def init_deframer(self):
        " Initialize deframer "
        self.log.trace("Initializing deframer...")
        self.regs.poke32(0x2040, 0x2)
        self.regs.poke32(0x2050, 0x0)
        self._gt_reset('rx', reset_only=False)
        self.regs.poke32(0x2040, 0x0)

    def init_framer(self):
        " Initialize framer "
        self.log.trace("Initializing framer...")
        self.regs.poke32(0x2060, 0x2002)
        self._gt_reset('tx', reset_only=False)
        self.regs.poke32(0x2064, 0xF0000)
        time.sleep(0.001)
        self.regs.poke32(0x2068, 0x1)
        rb = self.regs.peek32(0x2060)
        if rb & 0x100 != 0x100:
            raise Exception('TX core is not idle after reset')
        self.regs.poke32(0x2060, 0x1001)

    def get_framer_status(self):
        " Return True if framer is in good status "
        rb = self.regs.peek32(0x2060)
        self.log.trace("Returning framer status: {0}".format(hex(rb & 0xFF0)))
        return rb & 0xFF0 == 0x6C0

    def get_deframer_status(self):
        " Return True if deframer is in good status "
        rb = self.regs.peek32(0x2040)
        self.log.trace("Returning deframer status: {0}".format(hex(rb & 0xFFFFFFFF)))
        if rb & 0b100 == 0b0:
            self.log.warning("Deframer warning: Code Group Sync failed to complete!")
        elif rb & 0b1000 == 0b0:
            self.log.warning("Deframer warning: Channel Bonding failed to complete!")
        elif rb & 0x200000 == 0b1:
            self.log.warning("Deframer warning: Misc error!")
        return rb & 0xFFFFFFFF == 0xF000001C

    def init(self):
        """
        Initializes to the core. Needs to happen after the clock signal is ready.
        """
        self.log.trace("Initializing core...")
        self._gt_pll_power_control()
        self._gt_reset('tx', reset_only=True)
        self._gt_reset('rx', reset_only=True)
        self._gt_pll_lock_control()
        self.regs.poke32(0x2078, 0x40)

    def enable_lmfc(self):
        """
        Enable LMFC generator in FPGA. This step is woefully incomplete, but this call will work for now.
        """
        self.regs.poke32(0x2078, 0)

    def send_sysref_pulse(self):
        """
        Toggles the LMK pin that triggers a SYSREF pulse.
        Note: SYSREFs must be enabled on LMK separately beforehand.
        """
        self.log.trace("Sending SYSREF pulse...")
        self.regs.poke32(0x206C, 0x40000000) # Bit 30. Self-clears.

    def _gt_reset(self, tx_or_rx, reset_only=False):
        " Put MGTs into reset. Optionally unresets and enables them "
        assert tx_or_rx.lower() in ('rx', 'tx')
        mgt_reg = {'tx': 0x2020, 'rx': 0x2024}[tx_or_rx]
        self.log.trace("Resetting %s MGTs..." % tx_or_rx.upper())
        self.regs.poke32(mgt_reg, 0x10)
        if not reset_only:
            self.regs.poke32(mgt_reg, 0x20)
            rb = -1
            for _ in range(20):
                rb = self.regs.peek32(mgt_reg)
                if rb & 0xFFFF0000 == 0x000F0000:
                    return True
                time.sleep(0.001)
            raise Exception('Timeout in GT {trx} Reset (Readback: 0x{rb:X})'.format(
                trx=tx_or_rx.upper(),
                rb=(rb & 0xFFFF0000),
            ))
        return True

    def _gt_pll_power_control(self):
        " Power down unused CPLLs and QPLLs "
        self.log.trace("Powering down unused CPLLs and QPLLs...")
        self.regs.poke32(0x200C, 0xFFF000E)

    def _gt_pll_lock_control(self):
        """
        Turn on the PLLs we're using, and make sure lock bits are set.
        """
        self.regs.poke32(0x2000, 0x1111) # Reset QPLLs
        self.regs.poke32(0x2000, 0x1110) # Unreset the ones we're using
        time.sleep(0.002) # alternatively, poll on the locked bit below
        self.regs.poke32(0x2000, 0x10000) # Clear all QPLL sticky bits
        rb = self.regs.peek32(0x2000) # Read QPLL locked and no unlocked stickies.
        self.log.trace("Reading QPLL lock bit: {0}".format(hex(rb & 0xF)))
        # Error: GT PLL failed to lock.
        if rb & 0xF != 0x2:
            raise Exception("GT PLL failed to lock!")

    def reset_mykonos(self):
        " Toggle reset line on Mykonos "
        self.regs.poke32(0x0008, 0) # Active low reset
        time.sleep(0.001)
        self.regs.poke32(0x0008, 1) # No longer in reset

