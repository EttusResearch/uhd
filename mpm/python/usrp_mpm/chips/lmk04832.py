#
# Copyright 2019-2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This class also has a C++ counterpart in host/lib/usrp/common/lmk04832.cpp and
# host/lib/include/uhdlib/usrp/common/lmk04832.hpp
# If any changes are needed in this file, consider if making the change in the C++ counterpart is
# also necessary.
"""LMK04832 parent driver class."""

import time

from usrp_mpm.mpmlog import get_logger


class LMK04832:
    """Generic driver class for LMK04832 access.

    This is subclassed for the X4xx-specific use case.
    """

    LMK_CHIP_ID = 6
    LMK_PROD_ID = 0xD163

    VCXO_FREQUENCIES = [122.88e6, 100.00e6]
    LMK_VCO0_RANGE_MIN = 2440e6
    LMK_VCO0_RANGE_MAX = 2580e6
    LMK_VCO1_RANGE_MIN = 2945e6
    LMK_VCO1_RANGE_MAX = 3255e6

    # PLL2 Prescaler is in range from 2, 8
    PLL2_PRESCALER = range(2, 9)

    # fmt: off
    RB_PLL1_LD_LOST_MASK = 0b1000
    RB_PLL1_LD_MASK      = 0b0100
    RB_PLL2_LD_LOST_MASK = 0b0010
    RB_PLL2_LD_MASK      = 0b0001
    # fmt: on

    def __init__(self, regs_iface, parent_log=None):
        """Initialize class."""
        self.log = (
            parent_log.getChild("LMK04832") if parent_log is not None else get_logger("LMK04832")
        )
        self.regs_iface = regs_iface
        assert hasattr(self.regs_iface, "peek8")
        assert hasattr(self.regs_iface, "poke8")
        self.poke8 = regs_iface.poke8
        self.peek8 = regs_iface.peek8
        self.enable_3wire_spi = False

    def pokes8(self, addr_vals):
        """Apply a series of pokes.

        pokes8([(0,1),(0,2)]) is the same as calling poke8(0,1), poke8(0,2).
        """
        for addr, val in addr_vals:
            self.poke8(addr, val)

    def get_chip_id(self):
        """Read back the chip ID."""
        chip_id = self.peek8(0x03)
        self.log.trace("Chip ID Readback: {}".format(chip_id))
        return chip_id

    def get_product_id(self):
        """Read back the product ID."""
        prod_id_0 = self.peek8(0x04)
        prod_id_1 = self.peek8(0x05)
        prod_id = (prod_id_1 << 8) | prod_id_0
        self.log.trace("Product ID Readback: 0x{:X}".format(prod_id))
        return prod_id

    def verify_chip_id(self):
        """Return True if the chip ID and product ID matches what we expect.

        Returns False otherwise.
        """
        chip_id = self.get_chip_id()
        prod_id = self.get_product_id()
        if chip_id != self.LMK_CHIP_ID:
            self.log.error("Wrong Chip ID 0x{:X}".format(chip_id))
            return False
        if prod_id != self.LMK_PROD_ID:
            self.log.error("Wrong Product ID 0x{:X}".format(prod_id))
            return False
        return True

    def enable_4wire_spi(self):
        """Enable 4-wire SPI readback from the CLKin_SEL0 pin."""
        self.poke8(0x148, 0x33)
        self.enable_3wire_spi = False

    def get_status(self):
        """Returns PLL lock status as a dictionary."""
        pll1_status = self.check_plls_locked(pll="PLL1")
        pll2_status = self.check_plls_locked(pll="PLL2")
        return {"PLL1 lock": pll1_status, "PLL2 lock": pll2_status}

    def clear_ldl(self, pll="BOTH"):
        """Clear the LD_LOST flags for both PLLs.

        Set `pll` to "PLL1", "PLL2", or "BOTH" to specify which PLL's LD_LOST
        flags to clear.
        """
        pll = pll.upper()
        assert pll in ("BOTH", "PLL1", "PLL2"), "Invalid PLL specified"
        clear_val1 = 0b10 if pll in ("BOTH", "PLL1") else 0
        clear_val2 = 0b01 if pll in ("BOTH", "PLL2") else 0
        self.poke8(0x182, clear_val1 | clear_val2)
        self.poke8(0x182, 0x00)

    def check_plls_locked(self, pll="BOTH", sticky=False):
        """Return True if the specified PLLs are locked, False otherwise.

        Note: By default, this checks the current status only, it does not check
        the sticky lock-detect bits (i.e., we only check RB_PLL1_LD and/or
        RB_PLL2_LD). If sticky is set to True, this will also check the sticky
        bits, and will clear them afterwards.

        From the datasheet (Section 8.6.2.9.3), register 0x183 bits:
        3: RB_PLL1_LD_LOST. This is set when PLL1 DLD edge falls. Does not set
           if cleared while PLL1 DLD
        2: RB_PLL1_LD. Read back 0: PLL1 DLD is low.  Read back 1: PLL1 DLD is
           high.
        1: RB_PLL2_LD_LOST. This is set when PLL2 DLD edge falls. Does not set
           if cleared while PLL2 DLD
        0: RB_PLL2_LD. Read back 0: PLL2 DLD is low.  Read back 1: PLL2 DLD is
           high.

        The *_LD_LOST bits get cleared by writing to register 0x182 (see
        self.clear_ldl()).
        """
        pll = pll.upper()
        assert pll in ("BOTH", "PLL1", "PLL2"), "Invalid PLL specified"
        result = True
        clear_ldl = False
        pll_lock_status = self.peek8(0x183)
        pll1_ld = bool(pll_lock_status & self.RB_PLL1_LD_MASK)
        pll1_ld_lost = bool(pll_lock_status & self.RB_PLL1_LD_LOST_MASK)
        pll2_ld = bool(pll_lock_status & self.RB_PLL2_LD_MASK)
        pll2_ld_lost = bool(pll_lock_status & self.RB_PLL2_LD_LOST_MASK)

        if pll in ("BOTH", "PLL1"):
            result = result and pll1_ld and (not sticky or not pll1_ld_lost)
            clear_ldl = clear_ldl or (sticky and pll1_ld_lost)
        if pll in ("BOTH", "PLL2"):
            result = result and pll2_ld and (not sticky or not pll2_ld_lost)
            clear_ldl = clear_ldl or (sticky and pll2_ld_lost)
        if not result:
            self.log.debug(
                f"Lock loss reported: RB_PLL1_LD_LOST={int(pll1_ld_lost)},"
                f"RB_PLL1_LD={int(pll1_ld)},RB_PLL2_LD_LOST={int(pll2_ld_lost)},"
                f"RB_PLL2_LD={int(pll2_ld)}"
            )
        if clear_ldl:
            self.clear_ldl(pll)
        return result

    def wait_for_pll_lock(self, pll="BOTH", timeout=2000):
        """Wait for the PLL(s) to lock.

        Returns False if the PLL(s) do not lock before the timeout (in ms)
        """
        self.clear_ldl()
        # Now poll lock status until timeout
        end_time = time.monotonic() + (timeout / 1000)
        while time.monotonic() < end_time:
            time.sleep(0.1)
            if self.check_plls_locked(pll):
                return True
        pll = "PLL1 or PLL2" if pll.upper() == "BOTH" else pll
        self.log.debug("{} not reporting locked after {} ms wait".format(pll, timeout))
        return False

    def soft_reset(self, value=True):
        """Perform soft reset of the LMK04832.

        This is done by setting or unsetting the reset register.
        """
        reset_addr = 0
        if value:  # Reset
            reset_byte = 0x80
        else:  # Clear Reset
            reset_byte = 0x00
        if not self.enable_3wire_spi:
            reset_byte |= 0x10
        self.poke8(reset_addr, reset_byte)

    def pll1_r_divider_sync(self, sync_pin_callback):
        """Run PLL1 R Divider sync.

        Follows the sequence in
        http://www.ti.com/lit/ds/snas688c/snas688c.pdf chapter 8.3.1.1

        Rising edge on sync pin is done by an callback which has to return its
        success state. If the sync callback was successful, returns PLL1 lock
        state as overall success otherwise the method fails.
        """
        # 1) Setup device for synchronizing PLL1 R
        # PLL1R_SYNC_EN    (6) = 1
        # PLL1R_SYNC_SRC (5,4) = Sync pin
        # PLL2R_SYNC_EN    (3) = 0
        self.poke8(0x145, 0x50)

        # Do NOT change clkin0_TYPE and Clkin[0,1]_DEMUX.
        # Both are set in initialization and remain static.

        # 2) Arm PLL1 R divider for synchronization
        self.poke8(0x177, 0x20)
        self.poke8(0x177, 0)
        # 3) wait for the writes to complete by triggering a read
        self.get_chip_id()

        # 4) Send rising edge on SYNC pin
        result = sync_pin_callback()

        # 5) reset 0x145 to safe value (no sync enable set, sync src invalidated)
        self.poke8(0x145, 0)

        # 6) wait for PLL1 to lock
        if result:
            return self.wait_for_pll_lock("PLL1")
        return False

    ## Register bitfield definitions ##

    def pll2_pre_to_reg(self, prescaler, osc_field=0x01, xtal_en=0x0, ref_2x_en=0x0):
        """Return prescaler value.

        From the prescaler value, returns the register value combined with the other
        register fields.
        """
        # valid prescaler values are 2-8, where 8 is represented as 0x00.
        assert prescaler in range(2, 8 + 1)
        reg_val = (
            ((prescaler & 0x07) << 5)
            | ((osc_field & 0x3) << 2)
            | ((xtal_en & 0x1) << 1)
            | ((ref_2x_en & 0x1) << 0)
        )
        self.log.trace(
            "From prescaler value 0d{}, writing register as 0x{:02X}.".format(prescaler, reg_val)
        )
        return reg_val
