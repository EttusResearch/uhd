#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Timing analysis is performed in "/n3xx/doc/mb_timing.xlsx". See
# the spreadsheet for more details and explanations.

#*******************************************************************************
## Motherboard Clocks

# 10/20/25 MHz reference clock from rear panel connector. Constrain to the fastest
# possible clock rate.
set REF_CLK_PERIOD 40.00
create_clock -name ref_clk       -period $REF_CLK_PERIOD  [get_ports FPGA_REFCLK_P]
# 125 MHz RJ45 Ethernet clock
create_clock -name ge_phy_clk    -period 8.000            [get_ports ENET0_CLK125]
# 156.25 MHz oscillator to MGT bank 110
create_clock -name xge_clk       -period 6.400            [get_ports MGT156MHZ_CLK1_P]
# 125 MHz PLL for MG bank 109
create_clock -name net_clk       -period 8.000            [get_ports NETCLK_P]

# Virtual clocks for constraining I/O (used below)
create_clock -name async_in_clk  -period 50.00
create_clock -name async_out_clk -period 50.00



#*******************************************************************************
## Aliases for auto-generated clocks

# Rename the PS clocks. These are originally declared in the PS7 IP block, but do not
# have super descriptive names. We rename them here for additional clarity, and to match
# the rest of the design.

# First save off the input jitter setting for each, before we nuke the original clocks.
set clk100_jitter       [get_property INPUT_JITTER [get_clocks clk_fpga_0]]
set clk40_jitter        [get_property INPUT_JITTER [get_clocks clk_fpga_1]]
set meas_clk_ref_jitter [get_property INPUT_JITTER [get_clocks clk_fpga_2]]
set bus_clk_jitter      [get_property INPUT_JITTER [get_clocks clk_fpga_3]]

# Create the new clocks based on the old ones. This generates critical warnings that
# we are completely rewriting the old clock definition... this is OK.
create_clock -name clk100 \
             -period   [get_property PERIOD      [get_clocks clk_fpga_0]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_0]]]
create_clock -name clk40 \
             -period   [get_property PERIOD      [get_clocks clk_fpga_1]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_1]]]
create_clock -name meas_clk_ref \
             -period   [get_property PERIOD      [get_clocks clk_fpga_2]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_2]]]
create_clock -name bus_clk \
             -period   [get_property PERIOD      [get_clocks clk_fpga_3]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_3]]]

# Apply the jitter setting from the original clocks.
set_input_jitter [get_clocks clk100]       $clk100_jitter
set_input_jitter [get_clocks clk40]        $clk40_jitter
set_input_jitter [get_clocks meas_clk_ref] $meas_clk_ref_jitter
set_input_jitter [get_clocks bus_clk]      $bus_clk_jitter


# TDC Measurement Clock
create_generated_clock -name meas_clk_fb [get_pins {n3xx_clocking_i/misc_clock_gen_i/inst/mmcm_adv_inst/CLKFBOUT}]
create_generated_clock -name meas_clk    [get_pins {n3xx_clocking_i/misc_clock_gen_i/inst/mmcm_adv_inst/CLKOUT0}]

#*******************************************************************************
## White Rabbit DAC
# Constrain the DIN and NSYNC bits around the clock output. No readback.

set WR_OUT_CLK [get_ports {WB_DAC_SCLK}]
create_generated_clock -name wr_bus_clk \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $WR_OUT_CLK]/C] \
  -divide_by 2 $WR_OUT_CLK

#*******************************************************************************
## Front Panel GPIO
# These bits are driven from the DB-A radio clock. Although they are received async in
# the outside world, they should be constrained in the FPGA to avoid any race
# conditions. The best way to do this is a skew constraint across all the bits.

set FP_GPIO_CLK [get_ports {FPGA_GPIO[0]}]
create_generated_clock -name fp_gpio_bus_clk \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $FP_GPIO_CLK]/C] \
  -divide_by 2 $FP_GPIO_CLK

