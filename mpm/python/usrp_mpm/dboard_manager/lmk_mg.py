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
LMK04828 driver for use with Magnesium
"""

import time
from builtins import zip
from builtins import hex
from ..mpmlog import get_logger
from ..chips import LMK04828

class LMK04828Mg(LMK04828):
    def __init__(self, regs_iface, spi_lock, slot=None):
        LMK04828.__init__(self, regs_iface, slot)
        self.spi_lock = spi_lock
        self.log = get_logger("LMK04828")
        assert hasattr(self.spi_lock, 'lock')
        assert hasattr(self.spi_lock, 'unlock')

    def init(self):
        """
        Basic init. Turns it on. Let's read SPI.
        """
        self.log.info("Reset LMK & Verify")
        self.pokes8((
            (0x000, 0x90), # Assert reset
            (0x000, 0x10), # De-assert reset
            (0x002, 0x00), # De-assert power down
            (0x148, 0x33), # Clock Select as SDO
        ))
        if not self.verify_chip_id():
            raise Exception("Unable to locate LMK04828")


    def config(self):
        """
        Write lots of config foo.
        """
        self.log.trace("LMK Initialization")
        clkin0_r_divider = {10e6: 0x0A, 20e6: 0x14}[10e6] # FIXME: hard coded to use 10 MHz
        self.pokes8((
            (0x100, 0x78), # CLKout Config
            (0x101, 0x55), # CLKout Config
            (0x102, 0x55), # CLKout Config
            (0x103, 0x00), # CLKout Config
            (0x104, 0x20), # CLKout Config
            (0x105, 0x00), # CLKout Config
            (0x106, 0xF2), # CLKout Config MYK: (0xAB where A = SYSREF, B = CLK)
            (0x107, 0x15), # CLKout Config 0x15 = LVDS, 0x55 = LVPECL
            (0x108, 0x7E), # CLKout Config
            (0x109, 0x55), # CLKout Config
            (0x10A, 0x55), # CLKout Config
            (0x10B, 0x00), # CLKout Config
            (0x10C, 0x00), # CLKout Config
            (0x10D, 0x00), # CLKout Config
            (0x10E, 0xF0), # CLKout Config
            (0x10F, 0x55), # CLKout Config
            (0x110, 0x61), # CLKout Config
            (0x111, 0x55), # CLKout Config
            (0x112, 0x55), # CLKout Config
            (0x113, 0x00), # CLKout Config
            (0x114, 0x00), # CLKout Config
            (0x115, 0x00), # CLKout Config
            (0x116, 0xF9), # CLKout Config
            (0x117, 0x00), # CLKout Config
            (0x118, 0x78), # CLKout Config
            (0x119, 0x55), # CLKout Config
            (0x11A, 0x55), # CLKout Config
            (0x11B, 0x00), # CLKout Config
            (0x11C, 0x20), # CLKout Config
            (0x11D, 0x00), # CLKout Config
            (0x11E, 0xF1), # CLKout Config
            (0x11F, 0x00), # CLKout Config
            (0x120, 0x78), # CLKout Config
            (0x121, 0x55), # CLKout Config
            (0x122, 0x55), # CLKout Config
            (0x123, 0x00), # CLKout Config
            (0x124, 0x20), # CLKout Config
            (0x125, 0x00), # CLKout Config
            (0x126, 0xF2), # CLKout Config FPGA: (0xAB where A = SYSREF, B = CLK)
            (0x127, 0x55), # CLKout Config 0x15 = LVDS, 0x55 = LVPECL
            (0x128, 0x78), # CLKout Config
            (0x129, 0x55), # CLKout Config
            (0x12A, 0x55), # CLKout Config
            (0x12B, 0x00), # CLKout Config
            (0x12C, 0x00), # CLKout Config
            (0x12D, 0x00), # CLKout Config
            (0x12E, 0xF0), # CLKout Config
            (0x12F, 0x50), # CLKout Config
            (0x130, 0x78), # CLKout Config
            (0x131, 0x55), # CLKout Config
            (0x132, 0x55), # CLKout Config
            (0x133, 0x00), # CLKout Config
            (0x134, 0x20), # CLKout Config
            (0x135, 0x00), # CLKout Config
            (0x136, 0xF1), # CLKout Config
            (0x137, 0x05), # CLKout Config
            (0x138, 0x30), # VCO_MUX to VCO 1; OSCout off
            (0x139, 0x00), # SYSREF Source = MUX; SYSREF MUX = Normal SYNC
            (0x13A, 0x01), # SYSREF Divide [12:8]
            (0x13B, 0xE0), # SYSREF Divide [7:0]
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
            (0x14B, 0x01), # Holdover & DAC Manual Mode
            (0x14C, 0xF6), # DAC Manual Mode
            (0x14D, 0x00), # DAC Settings (defaults)
            (0x14E, 0x00), # DAC Settings (defaults)
            (0x14F, 0x7F), # DAC Settings (defaults)
            (0x150, 0x03), # Holdover Settings (defaults)
            (0x151, 0x02), # Holdover Settings (defaults)
            (0x152, 0x00), # Holdover Settings (defaults)
            (0x153, 0x00), # CLKin0_R divider [13:8], default = 0
            (0x154, clkin0_r_divider), # CLKin0_R divider [7:0], default = d120
            (0x155, 0x00), # CLKin1_R divider [13:8], default = 0
            (0x156, clkin0_r_divider), # CLKin1_R divider [7:0], default = d120
            (0x157, 0x00), # CLKin2_R divider [13:8], default = 0
            (0x158, 0x01), # CLKin2_R divider [7:0], default = d120
            (0x159, 0x00), # PLL1 N divider [13:8], default = 0
            (0x15A, 0x7D), # PLL1 N divider [7:0], default = d120
            (0x15B, 0xCF), # PLL1 PFD
            (0x15C, 0x27), # PLL1 DLD Count [13:8]
            (0x15D, 0x10), # PLL1 DLD Count [7:0]
            (0x15E, 0x00), # PLL1 R/N delay, defaults = 0
            (0x15F, 0x0B), # Status LD1 pin = PLL1 LD, push-pull output
            (0x160, 0x00), # PLL2 R divider [11:8];
            (0x161, 0x04), # PLL2 R divider [7:0]
            (0x162, 0xA4), # PLL2 prescaler; OSCin freq
            (0x163, 0x00), # PLL2 Cal = PLL2 normal val
            (0x164, 0x00), # PLL2 Cal = PLL2 normal val
            (0x165, 0x19), # PLL2 Cal = PLL2 normal val
            (0x171, 0xAA), # Write this val after x165
            (0x172, 0x02), # Write this val after x165
            (0x17C, 0x15), # VCo1 Cal; write before x168
            (0x17D, 0x33), # VCo1 Cal; write before x168
            (0x166, 0x00), # PLL2 N[17:16]
            (0x167, 0x00), # PLL2 N[15:8]
            (0x168, 0x19), # PLL2 N[7:0]
            (0x169, 0x51), # PLL2 PFD
            (0x16A, 0x27), # PLL2 DLD Count [13:8] = default d32
            (0x16B, 0x10), # PLL2 DLD Count [7:0] = default d0
            (0x16C, 0x00), # PLL2 Loop filter r = 200 ohm
            (0x16D, 0x00), # PLL2 loop filter c = 10 pF
            (0x16E, 0x13), # Status LD2 pin = Output push-pull, PLL2 DLD
            (0x173, 0x00), # Do not power down PLL2 or prescaler
        ))
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
        self.log.info("LMK init'd and locked!")

