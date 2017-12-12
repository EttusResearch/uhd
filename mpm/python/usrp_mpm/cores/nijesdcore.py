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
from usrp_mpm.mpmlog import get_logger

class NIMgJESDCore(object):
    """
    Provide interface for the FPGA JESD Core.
    Works with Magnesium/Mykonos daughterboards only.

    Arguments:
    regs -- regs class to use for peek/poke
    """

    DB_ID                      = 0x0630
    MGT_QPLL_CONTROL           = 0x2000
    MGT_PLL_POWER_DOWN_CONTROL = 0x200C
    MGT_TX_RESET_CONTROL       = 0x2020
    MGT_RX_RESET_CONTROL       = 0x2024
    MGT_RECEIVER_CONTROL       = 0x2040
    MGT_RX_DESCRAMBLER_CONTROL = 0x2050
    MGT_TRANSMITTER_CONTROL    = 0x2060
    MGT_TX_TRANSCEIVER_CONTROL = 0x2064
    MGT_TX_SCRAMBLER_CONTROL   = 0x2068
    LMK_SYNC_CONTROL           = 0x206C
    JESD_MGT_DRP_CONTROL       = 0x2070
    SYSREF_CAPTURE_CONTROL     = 0x2078
    JESD_SIGNATURE_REG         = 0x2100
    JESD_REVISION_REG          = 0x2104


    def __init__(self, regs, slot_idx=0):
        self.regs = regs
        self.log = get_logger("NIMgJESDCore-{}".format(slot_idx))
        assert hasattr(self.regs, 'peek32')
        assert hasattr(self.regs, 'poke32')
        # FutureWork: The following are constants for the Magnesium board. These need
        # to change to variables to support other interfaces.
        self.qplls_used = 1
        self.cplls_used = 0
        # Number of FPGA clock cycles per LMFC period.
        self.lmfc_divider = 20

    def unreset_qpll(self):
        # new_val = self.regs.peek32(0x0) & ~0x8
        # self.log.trace("Unresetting MMCM, writing value {:X}".format(new_val))
        self.regs.poke32(0x0, 0x7)

    def check_core(self):
        """
        Verify JESD core returns correct ID
        """
        self.log.trace("Checking JESD Core...")
        if self.regs.peek32(self.JESD_SIGNATURE_REG) != 0x4A455344:
            raise Exception('JESD Core signature mismatch! Check that core is mapped correctly')
        #if self.regs.peek32(JESD_REVISION_REG) != 0xFF
        #error here for date revision mismatch
        self.log.trace("JESD Core build code: {0}".format(hex(self.regs.peek32(self.JESD_REVISION_REG))))
        self.log.trace("DB Slot #: {}".format( (self.regs.peek32(self.DB_ID) & 0x10000) >> 16  ))
        self.log.trace("DB PID: {:X}" .format(  self.regs.peek32(self.DB_ID) & 0xFFFF ))
        return True

    def reset(self):
        """
        Reset to the core. Places the PLLs, TX MGTs and RX MGTs (along with the glue
        logic) in reset. Also disables the SYSREF sampler.
        """
        self.log.trace("Resetting the JESD204B FPGA core(s)...")
        self._gt_reset('tx', reset_only=True)
        self._gt_reset('rx', reset_only=True)
        self._gt_pll_lock_control(self.qplls_used, self.cplls_used, reset_only=True)
        # Disable SYSREF Sampler
        self.enable_lmfc(False)

    def init_deframer(self):
        " Initialize deframer "
        self.log.trace("Initializing deframer...")
        self.regs.poke32(self.MGT_RECEIVER_CONTROL, 0x2)
        self.regs.poke32(self.MGT_RX_DESCRAMBLER_CONTROL, 0x0)
        self._gt_reset('rx', reset_only=False)
        self.regs.poke32(self.MGT_RECEIVER_CONTROL, 0x0)

    def init_framer(self, bypass_scrambler = True):
        " Initialize framer "
        self.log.trace("Initializing framer...")
        # Disable DAC Sync from requesting CGS & Stop Deframer
        self.regs.poke32(self.MGT_TRANSMITTER_CONTROL, (0b1 << 13) | (0b1 << 1))
        # Reset, unreset, and check the GTs
        self._gt_reset('tx', reset_only=False)
        # MGT phy control... enable TX Driver Swing
        self.regs.poke32(self.MGT_TX_TRANSCEIVER_CONTROL, 0xF0000)
        time.sleep(0.001)
        # Bypass scrambler and char replacement. If the scrambler is bypassed,
        # then the char replacement is also disabled.
        reg_val = {True: 0x01, False: 0x10}[bypass_scrambler]
        self.regs.poke32(self.MGT_TX_SCRAMBLER_CONTROL, reg_val)
        # Check for Framer in Idle state
        rb = self.regs.peek32(self.MGT_TRANSMITTER_CONTROL)
        if rb & 0x100 != 0x100:
            raise Exception('TX Framer is not idle after reset')
        # Enable incoming DAC Sync
        self.regs.poke32(self.MGT_TRANSMITTER_CONTROL, 0b1 << 12)
        # Enable the framer
        self.regs.poke32(self.MGT_TRANSMITTER_CONTROL, 0b1 <<  0)

    def get_framer_status(self):
        " Return True if framer is in good status "
        rb = self.regs.peek32(self.MGT_TRANSMITTER_CONTROL)
        self.log.trace("FPGA Framer status: {0}".format(hex(rb & 0xFF0)))
        if rb & (0b1 << 8) == 0b1 << 8:
            self.log.warning("Framer warning: Framer is Idle!")
        elif rb & (0b1 << 6) == 0b0 << 6:
            self.log.warning("Framer warning: Code Group Sync failed to complete!")
        elif rb & (0b1 << 7) == 0b0 << 7:
            self.log.warning("Framer warning: Lane Alignment failed to complete!")
        return rb & 0xFF0 == 0x6C0

    def get_deframer_status(self):
        " Return True if deframer is in good status "
        rb = self.regs.peek32(self.MGT_RECEIVER_CONTROL)
        self.log.trace("FPGA Deframer status: {0}".format(hex(rb & 0xFFFFFFFF)))
        if rb & (0b1 << 2) == 0b0 << 2:
            self.log.warning("Deframer warning: Code Group Sync failed to complete!")
        elif rb & (0b1 <<  3) == 0b0 << 3:
            self.log.warning("Deframer warning: Channel Bonding failed to complete!")
        elif rb & (0b1 << 21) == 0b1 << 21:
            self.log.warning("Deframer warning: Misc link error!")
        return rb & 0xFFFFFFFF == 0xF000001C

    def init(self):
        """
        Initializes the core. Must happen after the reference clock is stable.
        """
        self.log.trace("Initializing JESD204B FPGA core(s)...")
        self._gt_pll_power_control(self.qplls_used, self.cplls_used)
        self._gt_reset('tx', reset_only=True)
        self._gt_reset('rx', reset_only=True)
        self._gt_pll_lock_control(self.qplls_used, self.cplls_used, reset_only=False)
        # Disable SYSREF Sampler
        self.enable_lmfc(False)

    def enable_lmfc(self, enable=False):
        """
        Enable/disable LMFC generator in FPGA.
        """
        self.log.trace("%s FPGA SYSREF Receiver..." % {True: 'Enabling', False: 'Disabling'}[enable])
        disable_bit = 0b1
        if enable:
           disable_bit = 0b0
        reg_val = ((self.lmfc_divider-1) << 23) | (disable_bit << 6)
        self.log.trace("Setting SYSREF Capture reg: 0x{:08X}".format(reg_val))
        self.regs.poke32(self.SYSREF_CAPTURE_CONTROL, reg_val)

    def send_sysref_pulse(self):
        """
        Toggles the LMK pin that triggers a SYSREF pulse.
        Note: SYSREFs must be enabled on LMK separately beforehand.
        """
        self.log.trace("Sending SYSREF pulse...")
        self.regs.poke32(self.LMK_SYNC_CONTROL, 0b1 << 30) # Bit 30. Self-clears.

    def _gt_reset(self, tx_or_rx, reset_only=False):
        " Put MGTs into reset. Optionally unresets and enables them "
        assert tx_or_rx.lower() in ('rx', 'tx')
        mgt_reg = {'tx': self.MGT_TX_RESET_CONTROL, 'rx': self.MGT_RX_RESET_CONTROL}[tx_or_rx]
        self.log.trace("Resetting %s MGTs..." % tx_or_rx.upper())
        self.regs.poke32(mgt_reg, 0x10)
        if not reset_only:
            self.regs.poke32(mgt_reg, 0x20)
            rb = -1
            for _ in range(20):
                rb = self.regs.peek32(mgt_reg)
                if rb & 0xFFFF0000 == 0x000F0000:
                    self.log.trace("%s MGT Reset Cleared!" % tx_or_rx.upper())
                    return True
                time.sleep(0.001)
            raise Exception('Timeout in GT {trx} Reset (Readback: 0x{rb:X})'.format(
                trx=tx_or_rx.upper(),
                rb=(rb & 0xFFFF0000),
            ))
        return True

    def _gt_pll_power_control(self, qplls = 0, cplls = 0):
        " Power down unused CPLLs and QPLLs "
        assert qplls in range(4+1) # valid is 0 - 4
        assert cplls in range(8+1) # valid is 0 - 8
        self.log.trace("Powering up {} CPLLs and {} QPLLs".format(cplls, qplls))
        reg_val = 0xFFFF000F
        reg_val_on = 0x0
        # Power down state is when the corresponding bit is set. For the PLLs we wish to
        # use, clear those bits.
        for x in range(qplls):
           reg_val_on = reg_val_on | 0x1 << x # QPLL bits are 0-3
        for y in range(16, 16 + cplls):
           reg_val_on = reg_val_on | 0x1 << y # CPLL bits are 16-23, others are reserved
        reg_val = reg_val ^ reg_val_on
        self.regs.poke32(self.MGT_PLL_POWER_DOWN_CONTROL, reg_val)

    def _gt_pll_lock_control(self, qplls = 0, cplls = 0, reset_only=False):
        """
        Turn on the PLLs we're using, and make sure lock bits are set.
        QPLL bitfield mapping: the following nibble is repeated for each QPLL. For
        example, QPLL0 get bits 0-3, QPLL1 get bits 4-7, etc.
        [0] = reset
        [1] = locked
        [2] = unlocked sticky
        [3] = ref clock lost sticky
        ...
        [16] = sticky reset (strobe)
        """
        # FutureWork: CPLLs are NOT supported yet!!!
        assert cplls == 0
        assert qplls in range(4+1) # valid is 0 - 4

        # Reset QPLLs.
        reg_val = 0x1111 # by default assert all resets
        self.regs.poke32(self.MGT_QPLL_CONTROL, reg_val)
        self.log.trace("Resetting QPLL(s)...")

        # Unreset the PLLs in use and check for lock.
        if not reset_only:
           if qplls > 0:
              # Unreset only the QPLLs we are using.
              reg_val_on = 0x0
              for nibble in range(qplls):
                 reg_val_on = reg_val_on | 0x1 << nibble*4
              reg_val = reg_val ^ reg_val_on
              self.regs.poke32(self.MGT_QPLL_CONTROL, reg_val)
              self.log.trace("Clearing QPLL reset...")

              # Check for lock a short time later.
              time.sleep(0.010)
              # Clear all QPLL sticky bits
              self.regs.poke32(self.MGT_QPLL_CONTROL, 0b1 << 16)
              # Check for lock on active quads only.
              rb = self.regs.peek32(self.MGT_QPLL_CONTROL)
              rb_mask = 0x0
              locked_val = 0x0
              for nibble in range(qplls):
                 if (rb & (0xF << nibble*4)) != (0x2 << nibble*4):
                    self.log.warning("GT QPLL {} failed to lock!".format(nibble))
                 locked_val = locked_val | 0x2 << nibble*4
                 rb_mask    = rb_mask    | 0xF << nibble*4
              if (rb & rb_mask) != locked_val:
                  raise Exception("One or more GT QPLLs failed to lock!")
              self.log.trace("QPLL(s) reporting locked!")

    def set_drp_target(self, mgt_or_qpll, dev_num):
        """
        Sets up access to the specified MGT or QPLL. This must be called
        prior to drp_access(). It may be called repeatedly to change DRP targets
        without calling the disable function first.
        """
        MAX_MGTS = 4
        MAX_QPLLs = 1
        DRP_ENABLE_VAL = 0b1
        assert mgt_or_qpll.lower() in ('mgt', 'qpll')

        self.log.trace("Enabling DRP access to %s #%d...",mgt_or_qpll.upper(), dev_num)

        # Enable access to the DRP ports and select the correct channel. Channels are
        # one-hot encoded with the MGT ports in bit locations [0, (MAX_MGTS-1)] and the
        # QPLL in [MAX_MGTS, MAX_MGTs+MAX_QPLLs-1].
        drp_ch_sel = {'mgt': dev_num, 'qpll': dev_num + MAX_MGTS}[mgt_or_qpll.lower()]
        assert drp_ch_sel in range(MAX_MGTS + MAX_QPLLs)
        reg_val = (0b1 << drp_ch_sel) | (DRP_ENABLE_VAL << 16)
        self.log.trace("Writing DRP Control Register (offset 0x{:04X}) with 0x{:08X}"
            .format(self.JESD_MGT_DRP_CONTROL, reg_val))
        self.regs.poke32(self.JESD_MGT_DRP_CONTROL, reg_val)

    def disable_drp_target(self):
        """
        Tears down access to the DRP ports. This must be called after drp_access().
        """
        self.regs.poke32(self.JESD_MGT_DRP_CONTROL, 0x0)
        self.log.trace("DRP accesses disabled!")

    def drp_access(self, rd = True, addr = 0, wr_data = 0):
        """
        Provides register access to the DRP ports on the MGTs or QPLLs buried inside
        the JESD204b logic. Reads will return the DRP data directly. Writes will return
        zeros.
        """
        # Check the DRP port is not busy.
        if (self.regs.peek32(self.JESD_MGT_DRP_CONTROL) & (0b1 << 20)) != 0:
            self.log.error("MGT/QPLL DRP Port is reporting busy during an attempted access.")
            raise Exception("MGT/QPLL DRP Port is reporting busy during an attempted access.")

        # Access the DRP registers...
        rd_data = 0x0
        core_offset = 0x2800 + (addr << 2)
        if rd:
            rd_data = self.regs.peek32(core_offset)
            rd_data_valid = rd_data & 0xFFFF
            self.log.trace("Reading DRP register 0x{:04X} at DB Core offset 0x{:04X}... "
                           "0x{:04X}"
                           .format(addr, core_offset, rd_data))
        else:
            self.log.trace("Writing DRP register 0x{:04X} with 0x{:04X}...".format(addr, wr_data))
            self.regs.poke32(core_offset, wr_data)
            if self.regs.peek32(core_offset) != wr_data:
                self.log.error("DRP read after write failed to match!")

        return rd_data

