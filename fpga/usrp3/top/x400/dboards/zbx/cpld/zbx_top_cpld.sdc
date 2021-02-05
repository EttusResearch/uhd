#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Timing constraints for the ZBX daughterboard CPLD.
#

set_time_format -unit ns -decimal_places 3

#####################################################################
# Main Clocks
#####################################################################

## Input clocks.
#    Reliable clock: 50.0 MHz
set reliable_clock_period 20.000
create_clock -name ctrlport_clk -period $reliable_clock_period [get_ports CTRL_REG_CLK]

# PLL reference clock: 64 MHz (maximum)
# Rounded down from 15.625 as this number divided by 2 has 4 decimal digits
# which produces a warning.
set prc_clock_period 15.62
create_clock -name pll_ref_clk -period $prc_clock_period [get_ports CPLD_REFCLK]

# Create clock for the ControlPort SPI interface.
# SPI clock is divided further down but only 3 clock cycles for processing are
# available for this SPI slave
set ctrl_sclk_period [expr {3 * $prc_clock_period}]
create_clock -name mb_ctrl_sck -period $ctrl_sclk_period [get_ports MB_CTRL_SCK]

# Oscillator clock regenerated as the IP constraint for the "int_osc_clk" is not
# yet available when processing this file.
create_generated_clock -name osc_clk \
  -source [get_pins -compatibility_mode {*oscillator_dut|clkout}] \
  [get_pins {int_osc_clk_ctrl_i|altclkctrl_0|clkctrl_altclkctrl_0_sub_component|clkctrl1|outclk}]

# required to get rid of Warning (332056): PLL cross checking found inconsistent
# PLL clock settings
derive_pll_clocks

#####################################################################
# Synthesizer sync interfaces
#####################################################################
# From MB FPGA
#
# ADClk_min and ADClk_max come from the ADCLK944 datasheet.
# The CPLD receives it's clock from ADCLK944 U40. U40 also supplies a
# clock to ADCLK944 U67, which drives clocks to the LO's.  Therefore,
# the clock arrives at the LO slightly later than it arrives at the
# CPLD.
#
# The clock input to the CPLD is delayed by one ADCLK944 (U40) compared
# to the input clock to the FPGA.
#
# ADClk_skew is also from the ADCLK944 datasheet,and is the maximum
# difference between outputs in a single ADCLK944, and represents the
# difference in arrival times of the clock at the
# CPLD input and the input to U67.
set ADClk_skew 0.015
set ADClk_min 0.070
set ADClk_max 0.130

# The longest SYNTH_SYNC trace going into a DB corresponds to DBO
#   Longest trace | Trace length | Trace delay
# (multiple paths)|   7.909 in   |   1.36 ns
# MB_SYNTH_SYNC, DB0_SYNTH_SYNC_fs, DB0_SYNTH_SYNC
# - for maximum propagation delay, this number will be rounded up
set synth_board_delay 1.4
# - for minimum propagation delay, we will consider a time of 0.

# Constrain the sync inputs to the CPLD driven from the MB FPGA.
# synth_sync min/max output delays are defined in
# fpga/usrp3/top/x400/constraints/timing/shared_constants.sdc

# Assume worst case as data being generated late and receiving an early clock:
#  - Max FPGA TCO
#  - Max FPGA clock propagation delay and minimum CPLD clock propagation delay
#  - Max data propagation delay
#  - Minimum delay on MC100EPT23 clock buffer
set_input_delay -clock [get_clocks pll_ref_clk] \
  -max [expr {$prc_clock_period - $synth_sync_setup_requirement         \
              + $fpga_prc_clock_prop_max - $db_cpld_prc_clock_prop_min  \
              + $synth_board_delay                                      \
              - $clock_translate_min }] \
  [get_ports MB_SYNTH_SYNC]

# Assume worst case as data being generated early and receiving an late clock:
#  - Min FPGA TCO
#  - Min data propagation delay (0)
#  - Min FPGA clock propagation delay and maximum CPLD clock propagation delay
#  - Maximum delay on MC100EPT23 clock buffer
set_input_delay -clock [get_clocks pll_ref_clk] \
  -min [expr {$synth_sync_hold_requirement                              \
              - 0                                                       \
              - $fpga_prc_clock_prop_min + $db_cpld_prc_clock_prop_max  \
              + $clock_translate_max}] \
  [get_ports MB_SYNTH_SYNC]

set rx_sync_ports {RX0_LO1_SYNC RX0_LO2_SYNC RX1_LO1_SYNC RX1_LO2_SYNC}
set tx_sync_ports {TX0_LO1_SYNC TX0_LO2_SYNC TX1_LO1_SYNC TX1_LO2_SYNC}
set sync_ports [get_ports [concat $rx_sync_ports $tx_sync_ports]]

# lo_setup and lo_hold are the constraints from the lmx2572 datasheet
# for the LO's SYNC input.
set lo_setup 2.5
set lo_hold -2.0
# The delay through an extra ADCLk944 reduces the maximum output delay,
# but the skew of the root ADCLK944 (U40) might work against you, so the
# skew increases the output delay.
set_output_delay -clock [get_clocks pll_ref_clk]\
  -max [expr {$lo_setup - $ADClk_min + $ADClk_skew}] \
  $sync_ports

# The LO's hold requirement is modeled as a negative output delay. The
# extra ADClk delay and the ADCLK skew can both work against you, so
# they are both added to increase the minimum output delay.
set_output_delay -clock [get_clocks pll_ref_clk]\
  -min [expr {-$lo_hold + $ADClk_max + $ADClk_skew}] \
  $sync_ports

#####################################################################
# Timing exceptions
#####################################################################
## synchronizers
set_false_path -to [get_registers *synchronizer_false_path\|value\[0\]\[*\]]

## PS SPI slave
# sclk data to ctrlport_clk
set_false_path -from [get_registers *spi_slave_async\|received_word\[*\]] \
              -to [get_registers *spi_slave_async\|data_out\[*\]]

# PLL driven data to sclk
set_false_path -from [get_registers *spi_slave_async\|transmit_word\[*\]] \
               -to [get_registers *spi_slave_async\|transmit_bits\[*\]]

#####################################################################
# MB CPLD <-> DP CPLD CTRL SPI interface
#####################################################################
# The timing constants of the MB CPLD are defined in
# fpga/usrp3/top/x400/cpld/db_spi_shared_constants.sdc

# The longest trace on the PL SPI interface is (assuming 170.0 ps/in)
#   Longest trace | Trace length | Trace delay
#      DB0_SCK    |   6.669 in   |   1.134 ns
set ctrl_spi_board_delay 1.134

# MB an dB CPLD both use PLL reference clock from a common clock chip.
# The traces from that clock chip to the ICs are not length match
# Assume a worst case clock difference of 2.5 ns at the IC inputs.
# There is no direction defined. The clock can arrive faster or slower
# on one IC.
set ctrl_clock_diff 2.500

set ctrl_slave_inputs [get_ports {MB_CTRL_MOSI MB_CTRL_CS}]
# calculate output delays back from capturing edge, add board delay and clock difference
set_input_delay -clock mb_ctrl_sck -clock_fall \
  -max [expr {$prc_clock_period - $db_cpld_spi_max_out - $ctrl_spi_board_delay - $ctrl_clock_diff}] \
  $ctrl_slave_inputs
# Assuming data is going without any delay, clock is arriving early at CPLD.
# Negate minimum output delay as it is defined from the change to the start clock edge.
set_input_delay -clock mb_ctrl_sck -clock_fall \
  -min [expr {- $db_cpld_spi_min_out - $ctrl_clock_diff}] \
  $ctrl_slave_inputs

set ctrl_slave_outputs [get_ports {MB_CTRL_MISO}]
# Calculate remaining time of clock period based on MB CPLD maximum input delay.
# Add board delay and clock difference.
set_output_delay -clock mb_ctrl_sck \
  -max [expr {$prc_clock_period - $db_cpld_spi_max_in + $ctrl_spi_board_delay + $ctrl_clock_diff}] \
  $ctrl_slave_outputs
# Assume no board delay just clock difference with rising edge occurring early at
# DB CPLD and MB CPLD input constraint
set_output_delay -clock mb_ctrl_sck \
  -min [expr {- $db_cpld_spi_min_in - $ctrl_clock_diff}] \
  $ctrl_slave_outputs

#####################################################################
# LO SPI interface
#####################################################################
set lo_spi_clks         [get_ports *X*_LO*_SCK]
set lo_spi_output_ports [get_ports {*X*_LO*_SDI *X*_LO*_CSB}]
set lo_spi_input_ports  [get_ports {*X*_LO*_MUXOUT}]

# Use the worst-case board propagation delay.
# Assuming 170.0 ps/in.
#   Longest trace | Trace length | Trace delay
#    RX0_LO1_SDI  |  8.333 in    |  1.416 ns
#  --------------------------------------------
# Since lines are not managed individually, and since we should
# have plenty of slack in this interface, we will conservatively use
# twice the propagation time of the longest trace for all our
# max delay calculations.
set lo_spi_max_sclk_delay     3.000
set lo_spi_min_sclk_delay     0.000
set lo_spi_max_signal_delay   3.000
set lo_spi_min_signal_delay   0.000

set lo_spi_min_tco   0.000
set lo_spi_max_tco  10.000

set lo_spi_setup    10.000
set lo_spi_hold     10.000

set lo_spi_clk_div 4

set lo_spi_clk_register [get_registers {zbx_cpld_core:zbx_cpld_core_i|lo_control:lo_control_i|spi_top:spi_top_i|spi_clgen:clgen|clk_out}]
create_generated_clock -source [get_ports {CPLD_REFCLK}] \
  -name lo_spi_clk $lo_spi_clk_register \
  -divide_by $lo_spi_clk_div

create_generated_clock \
  -source $lo_spi_clk_register \
  -name lo_spi_clk_out $lo_spi_clks

# ----------------------------------------
# -- Constraint for SPI CS and SDI --
# ----------------------------------------

set_output_delay -clock lo_spi_clk_out \
  -max [expr {$lo_spi_setup + $lo_spi_max_signal_delay - $lo_spi_min_sclk_delay}] \
  $lo_spi_output_ports
set_output_delay -clock lo_spi_clk_out \
  -min [expr {0 - $lo_spi_max_sclk_delay - $lo_spi_hold + $lo_spi_min_signal_delay }] \
  $lo_spi_output_ports

# -- Multi-cycle path for SPI CS and SDI --
# ----------------------------------------
# Both the CSB and SDI timing are defined in reference to the rising edge of SCLK, so we can merge the analysis.
#
# edge #   1           2           3           4           5
#  clk50 __/-----\_____/-----\_____/-----\_____/-----\_____/-----\_____/--
#   sclk __/-----------------------\_______________________/--------------
#                                         | launch edge (due to negedge reg)
#                                  |           |           |
#                                  0           1           2 -- Edge used for setup analysis N = 2
#
#          |           |           |           |
#          3           2           1           0 --(Setup -1) edge, in case of no hold multi-cycle path
#          |
#           \____ Edge used for hold = 3
#
# Analyzing this diagram, we can see that the setup edge of interest is located a sclk cycle
# after the launch edge($lo_spi_clk_div) and that the hold margin has setup-1 edges after the launch edge,
# or: lo_spi_clk_div - 1.
set_multicycle_path -setup -start -to $lo_spi_output_ports [expr ($lo_spi_clk_div/2)]
set_multicycle_path -hold  -start -to $lo_spi_output_ports [expr ($lo_spi_clk_div-1)]


# ----------------------------------------
# -- Constraint for SPI MUXOUT --
# ----------------------------------------

set_input_delay -clock lo_spi_clk_out -clock_fall \
  -max [expr {$lo_spi_max_sclk_delay + $lo_spi_max_tco + $lo_spi_max_signal_delay}] \
  $lo_spi_input_ports

set_input_delay -clock lo_spi_clk_out -clock_fall \
  -min [expr {$lo_spi_min_sclk_delay + $lo_spi_min_tco + $lo_spi_min_signal_delay}] \
  $lo_spi_input_ports

# -- Multi-cycle path for SPI MUXOUT --
# ----------------------------------------
# edge #   1           2           3           4           5           6           7           8
#  clk50 __/-----\_____/-----\_____/-----\_____/-----\_____/-----\_____/-----\_____/-----\_____/-----\_____/--
#   sclk __/-----------------------\_______________________/-----------------------\_______________________/--
# muxout              MSB          |                     MSB-1
#                             launch edge                  |
#                                  |           |           |
#                                  0           1           2
#                                                           \_ Edge used for setup analysis N = 2
#          |           |           |           |
#          3           2           1           0  - N-1 edge(if no hold is given)
#          |
#           \____ Edge used for hold = 3
#
# Analyzing this diagram, we can see that the setup edge of interest is located half a sclk cycle
# after the launch edge($lo_spi_clk_div/2) and that the hold margin has ($lo_spi_clk_div/2)
# of margin before the launch edge and setup-1 edges after the launch edge, to simplify:
# ($lo_spi_clk_div/2+$lo_spi_clk_div/2-1) = lo_spi_clk_div - 1.
set_multicycle_path -setup -end -from $lo_spi_input_ports [expr ($lo_spi_clk_div/2)]
set_multicycle_path -hold  -end -from $lo_spi_input_ports [expr ($lo_spi_clk_div-1)]

#####################################################################
# Asynchronous IO
#####################################################################
# For general I/O that don't have tight timing constraints, we can constrain
# these paths by creating a generic flip-flop that will interface to the
# device.
# For asynchronous outputs
set generic_ext_flop_tsu 1
set generic_ext_flop_th  0

# For asynchronous inputs
set generic_ext_flop_max_tco 2
set generic_ext_flop_min_tco 0

set async_outputs_prc {CH*_*X*_LED \
                   RX*_DSA*_*[*] \
                   TX*_DSA*[*] \
                   *X*_SW*}
set_output_delay -clock pll_ref_clk -max [expr $generic_ext_flop_tsu] \
                 [get_ports $async_outputs_prc]
set_output_delay -clock pll_ref_clk -min [expr 0 - $generic_ext_flop_th] \
                 [get_ports $async_outputs_prc]

set_output_delay -clock osc_clk -max [expr $generic_ext_flop_tsu] \
                 [get_ports {P*_ENABLE*}]
set_output_delay -clock osc_clk -min [expr 0 - $generic_ext_flop_th] \
                 [get_ports {P*_ENABLE*}]

set async_inputs {CTRL_REG_ARST}
set_input_delay -clock ctrlport_clk -max [expr $generic_ext_flop_max_tco] \
                 [get_ports $async_inputs]
set_input_delay -clock ctrlport_clk -min [expr $generic_ext_flop_min_tco] \
                 [get_ports $async_inputs]

set_input_delay -clock osc_clk -max [expr $generic_ext_flop_max_tco] \
                 [get_ports {P7V_PG_*}]
set_input_delay -clock osc_clk -min [expr $generic_ext_flop_min_tco] \
                 [get_ports {P7V_PG_*}]

#####################################################################
# MB FPGA GPIO
#####################################################################
# Some timing constants in this section are declared in
# fpga/usrp3/top/x400/constraints/timing/shared_constants.sdc

set db_gpio_inputs [get_ports {MB_FPGA_GPIO[*]}]
# Assume worst case as data being generated late and receiving an early clock:
#  - Max FPGA TCO
#  - Max data propagation delay
#  - Max FPGA clock propagation delay and minimum CPLD clock propagation delay
#  - Minimum delay on MC100EPT23 clock buffer
set_input_delay -clock pll_ref_clk -clock_fall \
  -max [expr {  $prc_clock_period/2 - $db_gpio_fpga_max_out             \
              + $db_gpio_board_max_delay                                \
              + $fpga_prc_clock_prop_max - $db_cpld_prc_clock_prop_min  \
              - $clock_translate_min}] \
  $db_gpio_inputs
# Assume worst case as data being generated early and receiving an late clock:
#  - Min FPGA TCO
#  - Min data propagation delay (0)
#  - Min FPGA clock propagation delay and maximum CPLD clock propagation delay
#  - Maximum delay on MC100EPT23 clock buffer
set_input_delay -clock pll_ref_clk -clock_fall \
  -min [expr {- $db_gpio_fpga_min_out                                   \
              - $db_gpio_board_min_delay                                \
              - $fpga_prc_clock_prop_min + $db_cpld_prc_clock_prop_max  \
              + $clock_translate_max}] \
  $db_gpio_inputs

# output delay
# maximum integer delays with slack of around 1ns
set_output_delay -clock pll_ref_clk -max $db_gpio_cpld_max_out $db_gpio_inputs
set_output_delay -clock pll_ref_clk -min $db_gpio_cpld_min_out $db_gpio_inputs

#####################################################################
# Known Issue of On-Chip Flash
#####################################################################
# see https://www.intel.com/content/www/us/en/programmable/support/support-resources/knowledge-base/tools/2016/warning--332060---node---alteraonchipflash-onchipflash-alteraonc.html
create_generated_clock -name flash_se_neg_reg \
 -source [get_pins { on_chip_flash:flash_i|altera_onchip_flash:onchip_flash_0|altera_onchip_flash_avmm_data_controller:avmm_data_controller|flash_se_neg_reg|clk }] \
 -divide_by 2 [get_pins { on_chip_flash:flash_i|altera_onchip_flash:onchip_flash_0|altera_onchip_flash_avmm_data_controller:avmm_data_controller|flash_se_neg_reg|q } ]

#####################################################################
# Clock uncertainty
#####################################################################
# Assign some uncertainty to all clocks
set clock_uncertainty 0.150
set_clock_uncertainty -to [get_clocks *] $clock_uncertainty
derive_clock_uncertainty

