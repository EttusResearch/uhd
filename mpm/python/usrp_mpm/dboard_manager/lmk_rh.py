#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
LMK04828 driver for use with Rhodium
"""

import time
from ..mpmlog import get_logger
from ..chips import LMK04828

class LMK04828Rh(LMK04828):
    """
    This class provides an interface to configure the LMK04828 IC through SPI.
    """

    def __init__(self, slot_idx, regs_iface, ref_clock_freq, sampling_clock_freq, parent_log=None):
        self.log = \
            parent_log.getChild("LMK04828-{}".format(slot_idx)) if parent_log is not None \
            else get_logger("LMK04828-{}".format(slot_idx))
        LMK04828.__init__(self, regs_iface, parent_log)
        self.log.debug("Using reference clock frequency {:.2f} MHz".format(ref_clock_freq/1e6))
        self.log.debug("Using sampling clock frequency: {:.2f} MHz"
                       .format(sampling_clock_freq/1e6))
        self.ref_clock_freq = ref_clock_freq
        self.sampling_clock_freq = sampling_clock_freq
        # VCXO on Rh runs at 122.88 MHz
        self.vcxo_freq       = 122.88e6
        self.clkin_r_divider = { 10e6:  250,     20e6:  500,  25e6:  625, 30.72e6:  750}[self.ref_clock_freq]
        self.pll1_n_divider  = { 10e6: 3072,     20e6: 3072,  25e6: 3072, 30.72e6: 3000}[self.ref_clock_freq]
        self.pll2_r_divider  = {400e6:   32, 491.52e6:   32, 500e6:  128}[self.sampling_clock_freq]
        self.pll2_prescaler  = {400e6:    5, 491.52e6:    2, 500e6:    5}[self.sampling_clock_freq]
        self.pll2_n_divider  = {400e6:  125, 491.52e6:  320, 500e6:  625}[self.sampling_clock_freq]
        self.pll2_vco_freq   = (self.vcxo_freq/self.pll2_r_divider)*self.pll2_prescaler*self.pll2_n_divider
        self.vco_selection   = self.get_vco_selection()
        self.sysref_divider  = {400e6:  144, 491.52e6:  120, 500e6:  144}[self.sampling_clock_freq]
        self.fpga_sysref_dly = {400e6:   11, 491.52e6:    8, 500e6:   10}[self.sampling_clock_freq]
        self.clkout_divider  = {400e6:    6, 491.52e6:    5, 500e6:    6}[self.sampling_clock_freq]
        self.lb_lo_divider   = {400e6:    2, 491.52e6:    2, 500e6:    2}[self.sampling_clock_freq]
        self.log.trace("Variable Configuration Report: "
                       "clkin0/1_r = 0d{}, clkout_div = 0d{}, pll1_n = 0d{}"
                       .format(self.clkin_r_divider, self.clkout_divider, self.pll1_n_divider))
        self.log.trace("Variable Configuration Report: "
                       "sysref_divider = 0d{}, fpga_sysref_dly = 0d{},"
                       .format(self.sysref_divider, self.fpga_sysref_dly))
        self.log.trace("Variable Configuration Report: "
                       "pll2_pre = 0d{}, pll2_n = 0d{}, pll2_vco_freq = 0d{}"
                       .format(self.pll2_prescaler, self.pll2_n_divider, self.pll2_vco_freq))

        self.init()
        self.config()

    def get_vco_freq(self):
        """
        Return the calculated VCO frequency in the LMK PLL2.
        """
        return self.pll2_vco_freq


    def get_vco_selection(self):
        """
        The internal VCO (0/1) is selected depending on the pll2_vco_freq value.
        According to the datasheet:
            VCO0 Frequency -> 2370 to 2630 MHz (Default)
            VCO1 Frequency -> 2920 to 3080 MHz
        The VCO selection is configured with bits 6:5 (VCO_MUX) in register 0x138:
               0x00      |     VCO 0
               0x01      |     VCO 1
               0x01      |  CLKin1 (ext)
               0x03      |    Reserved
        This function returns the full register value with ONLY the VCO selected
        (i.e. the returned value must be or-ed with the rest of the configuration).
        """
        if   (self.pll2_vco_freq >= 2370e6) and (self.pll2_vco_freq <= 2630e6):
            self.log.trace("VCO0 selected for PLL2.")
            return 0x00 << 5
        elif (self.pll2_vco_freq >= 2920e6) and (self.pll2_vco_freq <= 3080e6):
            self.log.trace("Internal VCO1 selected for PLL2.")
            return 0x01 << 5
        else:
            self.log.error("The calculated PLL2 VCO frequency ({}) is not supported \
                            by neither internal VCO".format(self.pll2_vco_freq))
            raise Exception("PLL2 VCO frequency not supported. Check log for details.")


    def init(self):
        """
        Basic init. Turns it on. Enables SPI reads.
        """
        self.log.debug("Reset LMK & Verify")
        self.pokes8((
            (0x000, 0x80), # Assert reset
            (0x000, 0x00), # De-assert reset
            (0x002, 0x00), # De-assert power down
        ))
        if not self.verify_chip_id():
            raise Exception("Unable to locate LMK04828")


    def config(self):
        """
        Writes the entire configuration necessary for Rhodium operation.
        """
        self.log.trace("LMK Initialization")

        # The sampling clocks going to the converters must be set at the sampling_clock_freq
        # rate. But, the JESD204B IP at the FPGA expects the clocks to be half that rate;
        # therefore, the actual divider for these clocks must be twice the normal.
        convclk_div_val = self.divide_to_reg(self.clkout_divider)
        convclk_cnt_val = self.divide_to_cnth_cntl_reg(self.clkout_divider)
        fpgaclk_div_val = self.divide_to_reg(self.clkout_divider*2)
        fpgaclk_cnt_val = self.divide_to_cnth_cntl_reg(self.clkout_divider*2)
        # The LMK provides both the TX and RX low-band references, which are configured
        # with the same divider values.
        lb_lo_div_val = self.divide_to_reg(self.lb_lo_divider)
        lb_lo_cnt_val = self.divide_to_cnth_cntl_reg(self.lb_lo_divider)

        # Determine one of the SDCLkOut configuration registers (0x104) for the SYSREF
        # signal going to the FPGA.
        #                    SYSREF out   VCO cycles to delay SYSREF
        fpga_sysref_config = (0b1 << 5) | ((self.fpga_sysref_dly - 1) << 1)
        self.log.trace("FPGA SYSREF delay register (0x104): 0x{:02X}".format(fpga_sysref_config))

        self.pokes8((
            (0x100, fpgaclk_div_val), # CLKout Config (FPGA Clock)
            (0x101, fpgaclk_cnt_val), # CLKout Config
            (0x102, 0x88), # CLKout Config
            (0x103, 0x00), # CLKout Config
            (0x104, fpga_sysref_config), # CLKout Config
            (0x105, 0x00), # CLKout Config
            (0x106, 0x72), # CLKout Config
            (0x107, 0x11), # CLKout Config
            (0x108, fpgaclk_div_val), # CLKout Config (MGT Reference Clock)
            (0x109, fpgaclk_cnt_val), # CLKout Config
            (0x10A, 0x88), # CLKout Config
            (0x10B, 0x00), # CLKout Config
            (0x10C, 0x00), # CLKout Config
            (0x10D, 0x00), # CLKout Config
            (0x10E, 0xF1), # CLKout Config
            (0x10F, 0x05), # CLKout Config
            (0x110, convclk_div_val), # CLKout Config (DAC Clock)
            (0x111, convclk_cnt_val), # CLKout Config
            (0x112, 0x22), # CLKout Config
            (0x113, 0x00), # CLKout Config
            (0x114, 0x20), # CLKout Config
            (0x115, 0x00), # CLKout Config
            (0x116, 0x72), # CLKout Config
            (0x117, 0x75), # CLKout Config
            (0x118, lb_lo_div_val), # CLKout Config (TX LB LO)
            (0x119, lb_lo_cnt_val), # CLKout Config
            (0x11A, 0x11), # CLKout Config
            (0x11B, 0x00), # CLKout Config
            (0x11C, 0x00), # CLKout Config
            (0x11D, 0x00), # CLKout Config
            (0x11E, 0x71), # CLKout Config
            (0x11F, 0x05), # CLKout Config
            (0x120, fpgaclk_div_val), # CLKout Config (Test Point Clock)
            (0x121, fpgaclk_cnt_val), # CLKout Config
            (0x122, 0x22), # CLKout Config
            (0x123, 0x00), # CLKout Config
            (0x124, 0x20), # CLKout Config
            (0x125, 0x00), # CLKout Config
            (0x126, 0x72), # CLKout Config
            (0x127, 0x10), # CLKout Config
            (0x128, lb_lo_div_val), # CLKout Config (RX LB LO)
            (0x129, lb_lo_cnt_val), # CLKout Config
            (0x12A, 0x22), # CLKout Config
            (0x12B, 0x00), # CLKout Config
            (0x12C, 0x00), # CLKout Config
            (0x12D, 0x00), # CLKout Config
            (0x12E, 0x71), # CLKout Config
            (0x12F, 0x05), # CLKout Config
            (0x130, convclk_div_val), # CLKout Config (ADC Clock)
            (0x131, convclk_cnt_val), # CLKout Config
            (0x132, 0x22), # CLKout Config
            (0x133, 0x00), # CLKout Config
            (0x134, 0x20), # CLKout Config
            (0x135, 0x00), # CLKout Config
            (0x136, 0x72), # CLKout Config
            (0x137, 0x55), # CLKout Config
            (0x138, (0x04 | self.vco_selection)), # VCO_MUX to VCO 0; OSCin->OSCout @ LVPECL 1600 mV
            (0x139, 0x00), # SYSREF Source = MUX; SYSREF MUX = Normal SYNC
            (0x13A, (self.sysref_divider & 0x1F00) >> 8), # SYSREF Divide [12:8]
            (0x13B, (self.sysref_divider & 0x00FF) >> 0), # SYSREF Divide [7:0]
            (0x13C, 0x00), # SYSREF DDLY [12:8]
            (0x13D, 0x0A), # SYSREF DDLY [7:0] ... 8 is default, <8 is reserved
            (0x13E, 0x00), # SYSREF Pulse Count = 1 pulse/request
            (0x13F, 0x00), # Feedback Mux: Disabled. OSCin, drives PLL1N divider (Dual PLL non 0-delay). PLL2_P drives PLL2N divider.
            (0x140, 0x00), # POWERDOWN options
            (0x141, 0x00), # Dynamic digital delay enable
            (0x142, 0x00), # Dynamic digital delay step
            (0x143, 0xD1), # SYNC edge sensitive; SYSREF_CLR; SYNC Enabled; SYNC from pin no pulser
            (0x144, 0x00), # Enable SYNC on all outputs including sysref
            (0x145, 0x7F), # Always program to d127
            (0x146, 0x00), # CLKin Type & En
            (0x147, 0x0A), # CLKin_SEL = CLKin0 manual (10/20/25 MHz) / CLKin1 manual (30.72 MHz); CLKin0/1 to PLL1
            (0x148, 0x02), # CLKin_SEL0 = input /w pulldown (default)
            (0x149, 0x02), # CLKin_SEL1 = input w/ pulldown +SDIO RDBK (push-pull)
            (0x14A, 0x02), # RESET type: in. w/ pulldown (default)
            (0x14B, 0x02), # Holdover & DAC Manual Mode
            (0x14C, 0x00), # DAC Manual Mode
            (0x14D, 0x00), # DAC Settings (defaults)
            (0x14E, 0x00), # DAC Settings (defaults)
            (0x14F, 0x7F), # DAC Settings (defaults)
            (0x150, 0x00), # Holdover Settings; bit 1 = '0' per long PLL1 lock time debug
            (0x151, 0x02), # Holdover Settings (defaults)
            (0x152, 0x00), # Holdover Settings (defaults)
            (0x153, (self.clkin_r_divider & 0x3F00) >> 8), # CLKin0_R divider [13:8], default = 0
            (0x154, (self.clkin_r_divider & 0x00FF) >> 0), # CLKin0_R divider [7:0], default = d120
            (0x155, (self.clkin_r_divider & 0x3F00) >> 8), # CLKin1_R divider [13:8], default = 0
            (0x156, (self.clkin_r_divider & 0x00FF) >> 0), # CLKin1_R divider [7:0], default = d150
            (0x157, 0x00), # CLKin2_R divider [13:8], default = 0 (Not used)
            (0x158, 0x01), # CLKin2_R divider [7:0], default = d1 (Not used)
            (0x159, (self.pll1_n_divider & 0x3F00) >> 8), # PLL1 N divider [13:8], default = d6
            (0x15A, (self.pll1_n_divider & 0x00FF) >> 0), # PLL1 N divider [7:0], default = d0
            (0x15B, 0xC7), # PLL1 PFD: negative slope for active filter / CP = 750 uA
            (0x15C, 0x27), # PLL1 DLD Count [13:8]
            (0x15D, 0x10), # PLL1 DLD Count [7:0]
            (0x15E, 0x00), # PLL1 R/N delay, defaults = 0
            (0x15F, 0x0B), # Status LD1 pin = PLL1 LD, push-pull output
            (0x160, (self.pll2_r_divider & 0x0F00) >> 8), # PLL2 R divider [11:8];
            (0x161, (self.pll2_r_divider & 0x00FF) >> 0), # PLL2 R divider [7:0]
            (0x162, self.pll2_pre_to_reg(self.pll2_prescaler)), # PLL2 prescaler; OSCin freq
            (0x163, 0x00), # PLL2 Cal = PLL2 normal val
            (0x164, 0x00), # PLL2 Cal = PLL2 normal val
            (0x165, 0x0A), # PLL2 Cal = PLL2 normal val
            (0x171, 0xAA), # Write this val after x165
            (0x172, 0x02), # Write this val after x165
            (0x17C, 0x15), # VCo1 Cal; write before x168
            (0x17D, 0x33), # VCo1 Cal; write before x168
            (0x166, (self.pll2_n_divider & 0x030000) >> 16), # PLL2 N[17:16]
            (0x167, (self.pll2_n_divider & 0x00FF00) >> 8),  # PLL2 N[15:8]
            (0x168, (self.pll2_n_divider & 0x0000FF) >> 0),  # PLL2 N[7:0]
            (0x169, 0x59), # PLL2 PFD
            (0x16A, 0x27), # PLL2 DLD Count [13:8] = default d39; SYSREF_REQ_EN disabled.
            (0x16B, 0x10), # PLL2 DLD Count [7:0] = default d16
            (0x16C, 0x00), # PLL2 Loop filter R3/4 = 200 ohm
            (0x16D, 0x00), # PLL2 loop filter C3/4 = 10 pF
            (0x16E, 0x13), # Status LD2 pin = Output push-pull, PLL2 DLD
            (0x173, 0x00), # Do not power down PLL2 or prescaler
        ))

        # Poll for PLL1/2 lock. Total time = 100 * 10 ms = 1000 ms max.
        self.log.trace("Polling for PLL lock...")
        locked = False
        for _ in range(100):
            time.sleep(0.01)
            # Clear stickies
            self.pokes8((
                (0x182, 0x1), # Clear Lock Detect Sticky
                (0x182, 0x0), # Clear Lock Detect Sticky
                (0x183, 0x1), # Clear Lock Detect Sticky
                (0x183, 0x0), # Clear Lock Detect Sticky
            ))
            if self.check_plls_locked():
                locked = True
                self.log.trace("LMK PLLs Locked!")
                break
        if not locked:
            raise RuntimeError("At least one LMK PLL did not lock! Check the logs for details.")

        self.log.trace("Setting SYNC and SYSREF config...")
        self.pokes8((
            (0x143, 0xF1), # toggle SYNC polarity to trigger SYNC event
            (0x143, 0xD1), # toggle SYNC polarity to trigger SYNC event
            (0x139, 0x02), # SYSREF Source = MUX; SYSREF MUX = pulser
            (0x144, 0xFF), # Disable SYNC on all outputs including sysref
            (0x143, 0x52), # Pulser selected; SYNC enabled; 1 shot enabled
        ))
        self.log.info("LMK initialized and locked!")

    def lmk_shift(self, num_shifts=0):
        """
        Apply time shift
        """
        self.log.trace("Clock Shifting Commencing using Dynamic Digital Delay...")
        # The sampling clocks going to the converters are set at the sampling_clock_freq
        # rate. But, the JESD204B IP at the FPGA expects the clocks to be half that rate;
        # therefore, the actual divider for the FPGA clocks is twice the normal.
        ddly_value_conv = self.divide_to_cnth_cntl_reg(self.clkout_divider+1) \
                if num_shifts >= 0 else self.divide_to_cnth_cntl_reg(self.clkout_divider-1)
        ddly_value_fpga = self.divide_to_cnth_cntl_reg(self.clkout_divider*2+1) \
                if num_shifts >= 0 else self.divide_to_cnth_cntl_reg(self.clkout_divider*2-1)
        ddly_value_sysref = self.sysref_divider+1 if num_shifts >= 0 else self.sysref_divider-1
        # Since the LMK provides the low-band LO references for RX/TX, these need to be
        # also shifted along with the sampling clocks to achieve the best possible
        # phase alignment perfomance at this band.
        ddly_value_lb_lo = self.divide_to_cnth_cntl_reg(self.lb_lo_divider+1) \
                if num_shifts >= 0 else self.divide_to_cnth_cntl_reg(self.lb_lo_divider-1)
        self.pokes8((
            # Clocks to shift: 0(FPGA CLK), 4(DAC), 6(TX LB-LO), 8(Test), 10(RX LB-LO), 12(ADC)
            (0x141, 0xFD),             # Dynamic digital delay enable on outputs.
            (0x143, 0x53),             # SYSREF_CLR; SYNC Enabled; SYNC from pulser @ regwrite
            (0x139, 0x02),             # SYSREF_MUX = Pulser
            (0x101, ddly_value_fpga),  # Set DDLY values for DCLKout0 +/-1 on low cnt.
            (0x102, ddly_value_fpga),  # Hidden register. Write the same as previous based on inc/dec.
            (0x111, ddly_value_conv),  # Set DDLY values for DCLKout4 +/-1 on low cnt
            (0x112, ddly_value_conv),  # Hidden register. Write the same as previous based on inc/dec.
            (0x119, ddly_value_lb_lo), # Set DDLY values for DCLKout6 +/-1 on low cnt
            (0x11A, ddly_value_lb_lo), # Hidden register. Write the same as previous based on inc/dec.
            (0x121, ddly_value_fpga),  # Set DDLY values for DCLKout8 +/-1 on low cnt
            (0x122, ddly_value_fpga),  # Hidden register. Write the same as previous based on inc/dec.
            (0x129, ddly_value_lb_lo), # Set DDLY values for DCLKout10 +/-1 on low cnt
            (0x12A, ddly_value_lb_lo), # Hidden register. Write the same as previous based on inc/dec.
            (0x131, ddly_value_conv),  # Set DDLY values for DCLKout12 +/-1 on low cnt
            (0x132, ddly_value_conv),  # Hidden register. Write the same as previous based on inc/dec.
            (0x13C, (ddly_value_sysref & 0x1F00) >> 8), # SYSREF DDLY value
            (0x13D, (ddly_value_sysref & 0x00FF) >> 0), # SYSREF DDLY value
            (0x144, 0x02),             # Enable SYNC on outputs 0, 4, 6, 8, 10, 12
        ))
        for _ in range(abs(num_shifts)):
            self.poke8(0x142, 0x1)
        # Put everything back the way it was before shifting.
        self.poke8(0x144, 0xFF) # Disable SYNC on all outputs including SYSREF
        self.poke8(0x143, 0x52) # Pulser selected; SYNC enabled; 1 shot enabled

    def enable_tx_lb_lo(self, enb):
        self.poke8(0x11F, 0x05 if enb else 0x00)

    def enable_rx_lb_lo(self, enb):
        self.poke8(0x12F, 0x05 if enb else 0x00)
