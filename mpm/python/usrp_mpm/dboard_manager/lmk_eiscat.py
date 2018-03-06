#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK04828 driver for use with Magnesium
"""

import time
from usrp_mpm.chips import LMK04828

class LMK04828EISCAT(LMK04828):
    """
    LMK04828 controls for EISCAT daughterboard
    """
    def __init__(self, regs_iface, ref_clock_freq, slot=None):
        LMK04828.__init__(self, regs_iface, slot)
        self.log.trace("Using reference clock frequency {} MHz".format(ref_clock_freq/1e6))
        if ref_clock_freq != 10e6:
            error_msg = "Invalid reference clock frequency: {} MHz. " \
                        "Must be 10 MHz.".format(ref_clock_freq)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        self.ref_clock_freq = ref_clock_freq
        self.init()
        self.config()

    def init(self):
        """
        Basic init. Turns it on. Let's us read SPI.
        """
        self.log.debug("Reset LMK & Verify")
        self.pokes8((
            (0x000, 0x90), # Assert reset
            (0x000, 0x10), # De-assert reset
            (0x002, 0x00), # De-assert power down
            (0x16E, 0x3B), # PLL2 Lock Detect Config as SDO
        ))
        if not self.verify_chip_id():
            raise Exception("Unable to locate LMK04828")


    def config(self):
        """
        Write lots of config foo.
        """
        self.log.trace("LMK Initialization")
        clkin0_r_divider = {10e6: 0x0A, 20e6: 0x14}[self.ref_clock_freq]
        self.pokes8((
            (0x100, 0x6C), # CLKout Config
            (0x101, 0x66), # CLKout Config
            (0x102, 0x66), # CLKout Config
            (0x103, 0x00), # CLKout Config
            (0x104, 0x20), # CLKout Config
            (0x105, 0x00), # CLKout Config
            (0x106, 0xF3), # CLKout Config
            (0x107, 0x05), # CLKout Config
            (0x108, 0x6C), # CLKout Config
            (0x109, 0x67), # CLKout Config
            (0x10A, 0x67), # CLKout Config
            (0x10B, 0x00), # CLKout Config
            (0x10C, 0x20), # CLKout Config
            (0x10D, 0x00), # CLKout Config
            (0x10E, 0x71), # CLKout Config
            (0x10F, 0x05), # CLKout Config
            (0x110, 0x6C), # CLKout Config
            (0x111, 0x67), # CLKout Config
            (0x112, 0x67), # CLKout Config
            (0x113, 0x00), # CLKout Config
            (0x114, 0x20), # CLKout Config
            (0x115, 0x00), # CLKout Config
            (0x116, 0x71), # CLKout Config
            (0x117, 0x05), # CLKout Config
            (0x118, 0x6C), # CLKout Config
            (0x119, 0x67), # CLKout Config
            (0x11A, 0x67), # CLKout Config
            (0x11B, 0x00), # CLKout Config
            (0x11C, 0x20), # CLKout Config
            (0x11D, 0x00), # CLKout Config
            (0x11E, 0x71), # CLKout Config
            (0x11F, 0x05), # CLKout Config
            (0x120, 0x78), # CLKout Config
            (0x121, 0x66), # CLKout Config
            (0x122, 0x66), # CLKout Config
            (0x123, 0x00), # CLKout Config
            (0x124, 0x20), # CLKout Config
            (0x125, 0x00), # CLKout Config
            (0x126, 0xF3), # CLKout Config
            (0x127, 0x00), # CLKout Config
            (0x128, 0x6C), # CLKout Config
            (0x129, 0x55), # CLKout Config
            (0x12A, 0x55), # CLKout Config
            (0x12B, 0x00), # CLKout Config
            (0x12C, 0x20), # CLKout Config
            (0x12D, 0x00), # CLKout Config
            (0x12E, 0xF9), # CLKout Config
            (0x12F, 0x00), # CLKout Config
            (0x130, 0x6C), # CLKout Config
            (0x131, 0x67), # CLKout Config
            (0x132, 0x67), # CLKout Config
            (0x133, 0x00), # CLKout Config
            (0x134, 0x20), # CLKout Config
            (0x135, 0x00), # CLKout Config
            (0x136, 0x71), # CLKout Config
            (0x137, 0x01), # CLKout Config
            (0x138, 0x10), # VCO_MUX to VCO 1; OSCout off
            (0x139, 0x00), # SYSREF Source = MUX; SYSREF MUX = Normal SYNC
            (0x13A, 0x01), # SYSREF Divide [12:8]
            (0x13B, 0xE0), # SYSREF Divide [7:0]
            (0x13C, 0x00), # SYSREF DDLY [12:8]
            (0x13D, 0x08), # SYSREF DDLY [7:0] ... 8 is default, <8 is reserved
            (0x13E, 0x00), # SYSREF Pulse Count = 1 pulse/request
            (0x13F, 0x0B), # Feedback Mux: Enabled, DCLKout6, drives PLL1N divider
            (0x140, 0x00), # POWERDOWN options
            (0x141, 0x08), # Dynamic digital delay enable
            (0x142, 0x00), # Dynamic digital delay step
            (0x143, 0xD1), # SYNC edge sensitive; SYSREF_CLR; SYNC Enabled; SYNC fro
            (0x144, 0x00), # Enable SYNC on all outputs including sysref
            (0x145, 0x7F), # Always program to d127
            (0x146, 0x08), # CLKin Type & En
            (0x147, 0x0E), # CLKin_SEL = CLKin1 manual; CLKin1 to PLL1
            (0x148, 0x01), # CLKin_SEL0 = input with pullup
            (0x149, 0x01), # CLKin_SEL1 = input with pulldown
            (0x14A, 0x02), # RESET type as input w/pulldown
            (0x14B, 0x01), # Holdover & DAC Manual Mode
            (0x14C, 0xF6), # DAC Manual Mode
            (0x14D, 0x00), # DAC Settings (defaults)
            (0x14E, 0x00), # DAC Settings (defaults)
            (0x14F, 0x7F), # DAC Settings (defaults)
            (0x150, 0x00), # Holdover Settings; bits 0/1 = '0' per long PLL1 lock time debug
            (0x151, 0x02), # Holdover Settings (defaults)
            (0x152, 0x00), # Holdover Settings (defaults)
            (0x153, 0x00), # CLKin0_R divider [13:8], default = 0
            (0x154, clkin0_r_divider), # CLKin0_R divider [7:0], default = d120
            (0x155, 0x00), # CLKin1_R divider [13:8], default = 0
            (0x156, 0x01), # CLKin1_R divider [7:0], default = d120
            (0x157, 0x00), # CLKin2_R divider [13:8], default = 0
            (0x158, 0x01), # CLKin2_R divider [7:0], default = d120
            (0x159, 0x00), # PLL1 N divider [13:8], default = 0
            (0x15A, 0x68), # PLL1 N divider [7:0], default = d120
            (0x15B, 0xCF), # PLL1 PFD
            (0x15C, 0x27), # PLL1 DLD Count [13:8]
            (0x15D, 0x10), # PLL1 DLD Count [7:0]
            (0x15E, 0x00), # PLL1 R/N delay, defaults = 0
            (0x15F, 0x13), # Status LD1 pin = PLL2 LD, push-pull output
            (0x160, 0x00), # PLL2 R divider [11:8];
            (0x161, 0x01), # PLL2 R divider [7:0]
            (0x162, 0x24), # PLL2 prescaler; OSCin freq
            (0x163, 0x00), # PLL2 Cal = PLL2 normal val
            (0x164, 0x00), # PLL2 Cal = PLL2 normal val
            (0x165, 0x0C), # PLL2 Cal = PLL2 normal val
            (0x171, 0xAA), # Write this val after x165
            (0x172, 0x02), # Write this val after x165
            (0x17C, 0x15), # VCo1 Cal; write before x168
            (0x17D, 0x33), # VCo1 Cal; write before x168
            (0x166, 0x00), # PLL2 N[17:16]
            (0x167, 0x00), # PLL2 N[15:8]
            (0x168, 0x0C), # PLL2 N[7:0]
            (0x169, 0x51), # PLL2 PFD
            (0x16A, 0x27), # PLL2 DLD Count [13:8] = default d32
            (0x16B, 0x10), # PLL2 DLD Count [7:0] = default d0
            (0x16C, 0x00), # PLL2 Loop filter r = 200 ohm
            (0x16D, 0x00), # PLL2 loop filter c = 10 pF
            (0x173, 0x00), # Do not power down PLL2 or prescaler
        ))
        # TODO: change to Polling.
        time.sleep(1.0) # Increased time to wait for DAC and VCXO to settle.
        self.pokes8((
            (0x182, 0x1), # Clear Lock Detect Sticky
            (0x182, 0x0), # Clear Lock Detect Sticky
            (0x183, 0x1), # Clear Lock Detect Sticky
            (0x183, 0x0), # Clear Lock Detect Sticky
        ))
        time.sleep(0.1)
        if not self.check_plls_locked():
            raise RuntimeError("At least one LMK PLL did not lock! Check the logs for details.")
        self.log.trace("Setting SYNC and SYSREF config...")
        self.pokes8((
            (0x143, 0xF1), # toggle SYNC polarity to trigger SYNC event
            (0x143, 0xD1), # toggle SYNC polarity to trigger SYNC event
            (0x139, 0x02), # SYSREF Source = MUX; SYSREF MUX = pulser
            (0x144, 0xFF), # Disable SYNC on all outputs including sysref
            (0x143, 0x52), # Pulser selected; SYNC enabled; 1 shot enabled
        ))
        self.log.debug("LMK init'd and locked!")

    def lmk_shift(self, num_shifts=0):
        """
        Apply time shift

        TODO: See if we can move this up to parent class
        """
        ddly_value = 0x67 if num_shifts >= 0 else 0x65
        self.pokes8((
            (0x141, 0x4E), # Dynamic digital delay enable
            (0x143, 0x53), # SYSREF_CLR; SYNC Enabled; SYNC from pulser @ regwrite
            (0x139, 0x02), # SYSREF_MUX = Pulser
            (0x109, ddly_value), # Set DDLY values for DCLKout2 +/-1 on low cnt.
                                 # To Increment phase, write 0x65. Decrement = 0x67
            (0x10A, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x111, ddly_value), # Set DDLY values for DCLKout4 +/-1 on low cnt
            (0x112, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x119, ddly_value), # Set DDLY values for DCLKout6 +/-1 on low cnt
            (0x11A, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x131, ddly_value), # Set DDLY values for DCLKout12 +/-1 on low cnt
            (0x132, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x144, 0xB1), # Enable SYNC on outputs 2,4,6,12
        ))
        for x in range(abs(num_shifts)):
            self.poke8(0x142, 0x1)
        self.poke8(0x144, 0xFF) # Disable SYNC on all outputs

