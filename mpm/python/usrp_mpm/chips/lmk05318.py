#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK05318 parent driver class
"""

import time
import datetime
from usrp_mpm.mpmlog import get_logger

class LMK05318:
    """
    Generic driver class for LMK05318 access.
    """

    LMK_VENDOR_ID = 0x100B
    LMK_PROD_ID = 0x35

    LMK_EEPROM_REG_COMMIT   = "register_commit"
    LMK_EEPROM_DIRECT_WRITE = "direct_write"

    def __init__(self, regs_iface, parent_log=None):
        self.log = \
            parent_log.getChild("LMK05318") if parent_log is not None \
            else get_logger("LMK05318")
        self.regs_iface = regs_iface
        assert hasattr(self.regs_iface, 'peek8')
        assert hasattr(self.regs_iface, 'poke8')
        self.peek8 = regs_iface.peek8

    def poke8(self, addr, val, overwrite_mask=False):
        """
        Write val to addr via register interface
        """
        # TI LMK UserGuide chapter 9.5.5 states that some register require bit masks
        # to be applied to bits to avoid writing to them
        # mask is in the form that a 1 means that the bit shall not be modified
        # in order to write to the address without the mask applied
        # the overwrite_mask parameter can be set to True
        if not overwrite_mask:
            mask = None
            if addr == 0x0C:
                mask = 0xA7
            elif addr == 0x9D:
                mask = 0xFF
            elif addr == 0xA4:
                mask = 0xFF
            elif addr in range(0x161, 0x1B3):
                mask = 0xFF

            if mask is not None:
                current_val = self.peek8(addr)
                val = val & ~mask
                val = val | current_val
                self.log.trace(
                    "Attention: writing to register {:02x} with masked bits, "
                    "mask 0x{:02x} was applied, resulting in value {:02x}"
                    .format(addr, mask, val))
        self.regs_iface.poke8(addr, val)

    def pokes8(self, addr_vals, overwrite_mask=False):
        """
        Apply a series of pokes.
        pokes8((0,1),(0,2)) is the same as calling poke8(0,1), poke8(0,2).
        """
        for addr, val in addr_vals:
            self.poke8(addr, val, overwrite_mask)

    def get_vendor_id(self):
        """ Read back the chip's vendor ID"""
        vendor_id_high = self.peek8(0x00)
        vendor_id_low = self.peek8(0x01)
        vendor_id = (vendor_id_high << 8) \
                  | vendor_id_low
        self.log.trace("Vendor ID Readback: 0x{:X}".format(vendor_id))
        return vendor_id

    def get_product_id(self):
        """
        Read back the chip's product ID
        """
        prod_id = self.peek8(0x02)
        self.log.trace("Product ID Readback: 0x{:X}".format(prod_id))
        return prod_id

    def is_chip_id_valid(self):
        """
        Returns True if the chip ID and product ID matches what we expect,
        False otherwise.
        """
        vendor_id = self.get_vendor_id()
        prod_id = self.get_product_id()
        if vendor_id != self.LMK_VENDOR_ID:
            self.log.error("Wrong Vendor ID 0x{:X}".format(vendor_id))
            return False
        if prod_id != self.LMK_PROD_ID:
            self.log.error("Wrong Product ID 0x{:X}".format(prod_id))
            return False
        return True

    def soft_reset(self, value=True):
        """
        Performs a soft reset of the LMK05318 by setting or unsetting
        the reset register
        """
        reset_addr = 0xC #DEV_CTL
        if value: # Reset
            reset_byte = 0x80
        else: # Clear Reset
            reset_byte = 0x7F & self.peek8(reset_addr)
        self.poke8(reset_addr, reset_byte, overwrite_mask=True)

    def write_cfg_regs_to_eeprom(self, method, eeprom_data=None):
        """
        program the current register config to LMK eeprom
        """
        def _wait_for_busy(self, value):
            wait_until = datetime.datetime.now() + datetime.timedelta(seconds=2)
            while datetime.datetime.now() < wait_until:
                self.log.trace("wait till busy bit becomes {}".format(value))
                busy = (self.peek8(0x9D) >> 2) & 1 # check if busy bit is cleared
                if busy == value:
                    return True
                time.sleep(0.01)
            return False

        if method == self.LMK_EEPROM_REG_COMMIT:
            self.log.trace("write current device register content to EEPROM")
            #store current cfg to SRAM
            self.poke8(0x9D, 0x40, overwrite_mask=True)
            time.sleep(0.01)
            #unlock EEPROM
            self.poke8(0xA4, 0xEA, overwrite_mask=True)
            time.sleep(0.01)
            #store SRAM into EEPROM
            self.poke8(0x9D, 0x03, overwrite_mask=True)
            #the actual programming takes about 230ms, poll the busy bit to see when it's done
            if not _wait_for_busy(self, 1):
                self.log.error("EEPROM does not start programming, something went wrong")
            if not _wait_for_busy(self, 0):
                self.log.error("EEPROM is still busy after programming, something went wrong")

        elif method == self.LMK_EEPROM_DIRECT_WRITE:
            raise RuntimeError("direct LMK05318 EEPROM programming not implemented")
        else:
            raise RuntimeError("Invalid method for LMK05318 EEPROM programming")

        #lock EEPROM
        self.poke8(0xA4, 0x00, overwrite_mask=True)
        self.log.trace("programming EEPROM done, power-cycle or hard-reset to take effect")

    def write_eeprom_to_cfg_regs(self):
        """
        read register cfg from eeprom and store it into registers
        """
        self.poke8(0x9D, 0x08, overwrite_mask=True)

    def get_eeprom_prog_cycles(self):
        """
        returns the number of eeprom programming cycles
        note:
        the actual counter only increases after programming AND power-cycle/hard-reset
        so multiple programming cycles without power cycle will lead to wrong counter values
        """
        return self.peek8(0x9C)

    def get_status_dpll(self):
        """
        returns the status register of the DPLL as human readable string
        """
        status = self.peek8(0x0E)
        return f"""
        Loss of phase lock: {status>>7 & 1}
        Loss of freq. lock: {status>>6 & 1}
        Tuning word update: {status>>5 & 1}
        Holdover Event: {status>>4 & 1}
        Reference Switch Event: {status>>3 & 1}
        Active ref. missing clk: {status>>2 & 1}
        Active ref. loss freq.: {status>>1 & 1}
        Active ref. loss ampl.: {status & 1}
        """

    def get_status_pll_xo(self):
        """
        returns the status register of the PLLs and XO as human readable string
        """
        status = self.peek8(0x0D)
        return f"""
        Loss of freq. detection XO: {status>>4 & 1}
        Loss of lock APLL2: {status>>3 & 1}
        Loss of lock APLL1: {status>>2 & 1}
        Loss of source XO: {status & 1}
        """
