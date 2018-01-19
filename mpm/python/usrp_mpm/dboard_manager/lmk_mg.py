#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK04828 driver for use with Magnesium
"""

import time
import math
from usrp_mpm.chips import LMK04828

class LMK04828Mg(LMK04828):
    """
    Magnesium-specific subclass of the LMK04828 controls.
    """
    def __init__(self, regs_iface, spi_lock, ref_clock_freq, master_clock_freq, log=None):
        LMK04828.__init__(self, regs_iface, log)
        self.log.debug("Using reference clock frequency: {} MHz".format(ref_clock_freq/1e6))
        self.log.debug("Using master clock frequency: {} MHz".format(master_clock_freq/1e6))
        self.spi_lock = spi_lock
        assert hasattr(self.spi_lock, 'lock')
        assert hasattr(self.spi_lock, 'unlock')
        self.ref_clock_freq = ref_clock_freq
        self.master_clock_freq = master_clock_freq
        # VCXO on Mg runs at 96 MHz
        self.vcxo_freq = 96e6
        # Feedback clock divider is constant for Mg regardless of the master_clock_freq.
        self.clkfb_divider = 24
        # PLL2 R value is also constant across all sample clock combinations.
        self.pll2_r_divider = 4
        # PLL1 PFD is 1 MHz. Divide the ref_clock_freq by 1 MHz to set the R divider.
        self.clkin_r_divider = int(math.floor(self.ref_clock_freq/1e6))
        self.clkout_divider = {122.88e6:   25, 125e6:   24, 153.6e6:   20}[self.master_clock_freq]
        self.pll1_n_divider = {122.88e6:  128, 125e6:  125, 153.6e6:  128}[self.master_clock_freq]
        self.sysref_divider = {122.88e6:  500, 125e6:  480, 153.6e6:  400}[self.master_clock_freq]
        self.pll2_prescaler = {122.88e6:    2, 125e6:    5, 153.6e6:    2}[self.master_clock_freq]
        self.pll2_n_divider = {122.88e6:   64, 125e6:   25, 153.6e6:   64}[self.master_clock_freq]
        self.pll2_vco_freq  = (self.vcxo_freq/self.pll2_r_divider)*self.pll2_prescaler*self.pll2_n_divider
        self.log.trace("Variable Configuration Report: "
                       "clkin1_r = 0d{}, clkout_div = 0d{}, pll1_n = 0d{}"
                       .format(self.clkin_r_divider, self.clkout_divider, self.pll1_n_divider))
        self.log.trace("Variable Configuration Report: "
                       "sysref_divider = 0d{}, pll2_pre = 0d{}, pll2_n = 0d{}"
                       .format(self.sysref_divider, self.pll2_prescaler, self.pll2_n_divider))
        self.log.trace("Variable Configuration Report: "
                       "pll2_vco_freq = 0d{}"
                       .format(self.pll2_vco_freq))
        # Run .init() and .config() right off the bat. Save clock shifty-ness for later.
        self.init()
        self.config()

    def get_vco_freq(self):
        """
        Return the calculated VCO frequency in the LMK PLL2.
        """
        return self.pll2_vco_freq

    def init(self):
        """
        Basic init. Turns it on. Let's read SPI.
        """
        self.log.trace("Reset and Verify Chip ID")
        self.pokes8((
            (0x000, 0x90), # Assert reset
            (0x000, 0x10), # De-assert reset
            (0x002, 0x00), # De-assert power down
            (0x148, 0x33), # Clock Select as SDO
        ))
        if not self.verify_chip_id():
            raise Exception("Unable to locate LMK04828!")

    def config(self):
        """
        Write lots of config foo.
        """
        clkout_div_val = self.divide_to_reg(self.clkout_divider)
        clkout_cnt_val = self.divide_to_cnth_cntl_reg(self.clkout_divider)

        self.log.trace("Register Initialization Commencing...")
        self.pokes8((
            (0x100, clkout_div_val), # CLKout Config
            (0x101, clkout_cnt_val), # CLKout Config
            (0x102, 0xCC), # CLKout Config
            (0x103, 0x00), # CLKout Config
            (0x104, 0x20), # CLKout Config
            (0x105, 0x00), # CLKout Config
            (0x106, 0x72), # CLKout Config MYK: (0xAB where A = SYSREF, B = CLK)
            (0x107, 0x15), # CLKout Config 0x15 = LVDS, 0x55 = LVPECL
            (0x108, clkout_div_val), # CLKout Config
            (0x109, clkout_cnt_val), # CLKout Config
            (0x10A, 0xFF), # CLKout Config
            (0x10B, 0x00), # CLKout Config
            (0x10C, 0x00), # CLKout Config
            (0x10D, 0x00), # CLKout Config
            (0x10E, 0x70), # CLKout Config
            (0x10F, 0x55), # CLKout Config
            (0x110, clkout_div_val), # CLKout Config
            (0x111, clkout_cnt_val), # CLKout Config
            (0x112, 0xCC), # CLKout Config
            (0x113, 0x00), # CLKout Config
            (0x114, 0x00), # CLKout Config
            (0x115, 0x00), # CLKout Config
            (0x116, 0xF9), # CLKout Config
            (0x117, 0x00), # CLKout Config
            (0x118, self.divide_to_reg(self.clkfb_divider)), # CLKout Config
            (0x119, self.divide_to_cnth_cntl_reg(self.clkfb_divider)), # CLKout Config
            (0x11A, 0xCC), # CLKout Config
            (0x11B, 0x00), # CLKout Config
            (0x11C, 0x20), # CLKout Config
            (0x11D, 0x00), # CLKout Config
            (0x11E, 0xF1), # CLKout Config
            (0x11F, 0x00), # CLKout Config
            (0x120, clkout_div_val), # CLKout Config
            (0x121, clkout_cnt_val), # CLKout Config
            (0x122, 0xCC), # CLKout Config
            (0x123, 0x00), # CLKout Config
            (0x124, 0x20), # CLKout Config 0x20 = SYSREF output, 0x00 = DEVCLK
            (0x125, 0x00), # CLKout Config
            (0x126, 0x72), # CLKout Config FPGA: (0xAB where A = SYSREF, B = CLK)
            (0x127, 0x55), # CLKout Config 0x1 = LVDS, 0x5 = LVPECL
            (0x128, clkout_div_val), # CLKout Config
            (0x129, clkout_cnt_val), # CLKout Config
            (0x12A, 0xCC), # CLKout Config
            (0x12B, 0x00), # CLKout Config
            (0x12C, 0x00), # CLKout Config
            (0x12D, 0x00), # CLKout Config
            (0x12E, 0x72), # CLKout Config
            (0x12F, 0xD0), # CLKout Config
            (0x130, clkout_div_val), # CLKout Config
            (0x131, clkout_cnt_val), # CLKout Config
            (0x132, 0xCC), # CLKout Config
            (0x133, 0x00), # CLKout Config
            (0x134, 0x20), # CLKout Config
            (0x135, 0x00), # CLKout Config
            (0x136, 0xF1), # CLKout Config
            (0x137, 0x05), # CLKout Config
            (0x138, 0x30), # VCO_MUX to VCO 1; OSCout off
            (0x139, 0x00), # SYSREF Source = MUX; SYSREF MUX = Normal SYNC
            (0x13A, (self.sysref_divider & 0x1F00) >> 8), # SYSREF Divide [12:8]
            (0x13B, (self.sysref_divider & 0x00FF) >> 0), # SYSREF Divide [7:0]
            (0x13C, 0x00), # SYSREF DDLY [12:8]
            (0x13D, 0x08), # SYSREF DDLY [7:0] ... 8 is default, <8 is reserved
            (0x13E, 0x00), # SYSREF Pulse Count = 1 pulse/request
            (0x13F, 0x09), # Feedback Mux: Enabled, DCLKout6, drives PLL1N divider
            (0x140, 0x00), # POWERDOWN options
            (0x141, 0x00), # Dynamic digital delay enable
            (0x142, 0x00), # Dynamic digital delay step
            (0x143, 0xD1), # SYNC edge sensitive; SYSREF_CLR; SYNC Enabled; SYNC from pin no pulser
            (0x144, 0x00), # Enable SYNC on all outputs including sysref
            (0x145, 0x7F), # Always program to d127
            (0x146, 0x10), # CLKin Type & En
            (0x147, 0x1A), # CLKin_SEL = CLKin1 manual; CLKin1 to PLL1
            # (0x148, 0x01), # CLKin_SEL0 = input with pullup: previously written above!
            (0x149, 0x01), # CLKin_SEL1 = input with pulldown
            (0x14A, 0x02), # RESET type
            (0x14B, 0x01), # Holdover & DAC Manual Mode
            (0x14C, 0xF6), # DAC Manual Mode
            (0x14D, 0x00), # DAC Settings (defaults)
            (0x14E, 0x00), # DAC Settings (defaults)
            (0x14F, 0x7F), # DAC Settings (defaults)
            (0x150, 0x00), # Holdover Settings; bits 0/1 = '0' per long PLL1 lock time debug
            (0x151, 0x02), # Holdover Settings (defaults)
            (0x152, 0x00), # Holdover Settings (defaults)
            (0x153, 0x00), # CLKin0_R divider [13:8], default = 0
            (0x154, 0x78), # CLKin0_R divider [7:0], default = d120
            (0x155, (self.clkin_r_divider & 0x3F00) >> 8), # CLKin1_R divider [13:8], default = 0
            (0x156, (self.clkin_r_divider & 0x00FF) >> 0), # CLKin1_R divider [7:0], default = d120
            (0x157, 0x00), # CLKin2_R divider [13:8], default = 0
            (0x158, 0x01), # CLKin2_R divider [7:0], default = d120
            (0x159, (self.pll1_n_divider & 0x3F00) >> 8), # PLL1 N divider [13:8], default = 0
            (0x15A, (self.pll1_n_divider & 0x00FF) >> 0), # PLL1 N divider [7:0], default = d120
            (0x15B, 0xCF), # PLL1 PFD
            (0x15C, 0x27), # PLL1 DLD Count [13:8]
            (0x15D, 0x10), # PLL1 DLD Count [7:0]
            (0x15E, 0x00), # PLL1 R/N delay, defaults = 0
            (0x15F, 0x0B), # Status LD1 pin = PLL1 LD, push-pull output
            (0x160, 0x00), # PLL2 R divider [11:8];
            (0x161, 0x04), # PLL2 R divider [7:0]
            (0x162, self.pll2_pre_to_reg(self.pll2_prescaler)), # PLL2 prescaler; OSCin freq 0xA4
            (0x163, 0x00), # PLL2 Cal = PLL2 normal val
            (0x164, 0x00), # PLL2 Cal = PLL2 normal val
            (0x165, 0x19), # PLL2 Cal = PLL2 normal val
            (0x171, 0xAA), # Write this val after x165
            (0x172, 0x02), # Write this val after x165
            (0x17C, 0x15), # VCo1 Cal; write before x168
            (0x17D, 0x33), # VCo1 Cal; write before x168
            (0x166, (self.pll2_n_divider & 0x030000) >> 16), # PLL2 N[17:16]
            (0x167, (self.pll2_n_divider & 0x00FF00) >> 8), # PLL2 N[15:8]
            (0x168, (self.pll2_n_divider & 0x0000FF) >> 0), # PLL2 N[7:0]
            (0x169, 0x51), # PLL2 PFD
            (0x16A, 0x27), # PLL2 DLD Count [13:8] = default d32
            (0x16B, 0x10), # PLL2 DLD Count [7:0] = default d0
            (0x16C, 0x00), # PLL2 Loop filter r = 200 ohm
            (0x16D, 0x00), # PLL2 loop filter c = 10 pF
            (0x16E, 0x13), # Status LD2 pin = Output push-pull, PLL2 DLD
            (0x173, 0x00), # Do not power down PLL2 or prescaler
        ))

        # Poll for PLL1/2 lock. Total time = 6 * 50 ms = 300 ms
        self.log.trace("Polling for PLL lock...")
        locked = False
        for _ in range(6):
            time.sleep(0.050)
            # Clear stickies
            self.pokes8((
                (0x182, 0x1), # Clear Lock Detect Sticky
                (0x182, 0x0), # Clear Lock Detect Sticky
                (0x183, 0x1), # Clear Lock Detect Sticky
                (0x183, 0x0), # Clear Lock Detect Sticky
            ))
            if self.check_plls_locked():
                locked = True
                self.log.trace("PLLs are Locked!")
                break
        if not locked:
            raise RuntimeError("At least one PLL did not lock! Check the logs for details.")

        self.log.trace("Synchronizing output dividers...")
        self.pokes8((
            (0x143, 0xF1), # toggle SYNC polarity to trigger SYNC event
            (0x143, 0xD1), # toggle SYNC polarity to trigger SYNC event
            (0x139, 0x02), # SYSREF Source = MUX; SYSREF MUX = pulser
            (0x144, 0xFF), # Disable SYNC on all outputs including sysref
            (0x143, 0x52), # Pulser selected; SYNC enabled; 1 shot enabled
        ))
        self.log.debug("Clocks Initialized and PLLs Locked!")

    def lmk_shift(self, num_shifts=0):
        """
        Apply time shift using the dynamic digital delays inside the LMK.
        """
        self.log.trace("Clock Shifting Commencing using Dynamic Digital Delay...")
        ddly_value = self.divide_to_cnth_cntl_reg(self.clkout_divider+1) \
                if num_shifts >= 0 else self.divide_to_cnth_cntl_reg(self.clkout_divider-1)
        ddly_value_sysref = self.sysref_divider+1 if num_shifts >= 0 else self.sysref_divider-1
        self.pokes8((
            (0x141, 0xB1), # Dynamic digital delay enable on outputs 0, 8, 10
            (0x143, 0x53), # SYSREF_CLR; SYNC Enabled; SYNC from pulser @ regwrite
            (0x139, 0x02), # SYSREF_MUX = Pulser
            (0x101, ddly_value), # Set DDLY values for DCLKout0 +/-1 on low cnt.
            (0x102, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x121, ddly_value), # Set DDLY values for DCLKout8 +/-1 on low cnt
            (0x122, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x129, ddly_value), # Set DDLY values for DCLKout10 +/-1 on low cnt
            (0x12A, ddly_value), # Hidden register. Write the same as previous based on inc/dec.
            (0x13C, (ddly_value_sysref & 0x1F00) >> 8), # SYSREF DDLY value [12:8]
            (0x13D, (ddly_value_sysref & 0x00FF) >> 0), # SYSREF DDLY value [7:0]
            (0x144, 0x4E), # Enable SYNC on outputs 0, 8, 10
        ))
        for _ in range(abs(num_shifts)):
            self.poke8(0x142, 0x1)
        # Put everything back the way it was before shifting.
        self.poke8(0x144, 0xFF) # Disable SYNC on all outputs including SYSREF
        self.poke8(0x143, 0x52) # Pulser selected; SYNC enabled; 1 shot enabled
