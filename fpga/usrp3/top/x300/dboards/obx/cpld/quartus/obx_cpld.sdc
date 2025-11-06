#
# Copyright 2025 Ettus Research, A National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Timing constraints for the OBX daughterboard CPLD.
#

set_time_format -unit ns -decimal_places 3

#####################################################################
# Main Clocks
#####################################################################

# The period is derived from the radio_clock frequency (5 ns period), using a
# divider value of 30 to generate the SPI clock.
set mb_ctrl_sck_period 150
create_clock -name mb_ctrl_sck -period $mb_ctrl_sck_period [get_ports MCTRL_SCLK]
create_clock -name mb_ctrl_sen -period $mb_ctrl_sck_period [get_ports mMCTRL_SEN]


#####################################################################
# Asynchronous IO
#####################################################################

# Although controlled by a flip-flop, these ports are asynchronous
# outside of the CPLD, and can be constrained by creating a generic
# flip-flop that will interface to the device.

# For asynchronous outputs
set generic_ext_flop_tsu 1
set generic_ext_flop_th  0

# For asynchronous inputs
set generic_ext_flop_max_tco 2
set generic_ext_flop_min_tco 0

set async_outputs_sclk {aFE_SEL_*          \
                        aPWREN_*           \
                        aPWEN_*            \
                        a*XLO*_NPDRF       \
                        a*XLO*_FSEL*       \
                        a*X*B_SEL*         \
                        aLED_*             \
                        aLOCKD_*           \
                       }

set_output_delay -clock mb_ctrl_sck -max [expr $generic_ext_flop_tsu] \
                 [get_ports $async_outputs_sclk]
set_output_delay -clock mb_ctrl_sck -min [expr 0 - $generic_ext_flop_th] \
                 [get_ports $async_outputs_sclk]

# The asynchronous inputs will only toggle when the SPI clock is
# idle.
set async_inputs {aMCTRL_RESET_N     \
                  aMCTRL_ATR*        \
                  aMCTRL_RX2_EN      \
                  aMCTRL_ADDR*       \
                  a*XLO*_LD}

set_input_delay -clock mb_ctrl_sck -max [expr $generic_ext_flop_max_tco] \
                 [get_ports $async_inputs]
set_input_delay -clock mb_ctrl_sck -min [expr $generic_ext_flop_min_tco] \
                 [get_ports $async_inputs]


#####################################################################
# Synchronous IO
#####################################################################

# FPGA -> CPLD SPI path
#--------------------------------

# Keep conservative values for the SPI propagation between
# FPGA and CPLD to account for variant compatibility.

# TCO jitter between SPI lines driven by the FPGA
set max_fpga_gpio_jitter 2.5

# Conservative routing mismatch compensation (used for clock vs data arrival times)
set max_fpga_gpio_trace_diff_dly 1.0

# Incoming SPI lines from FPGA.

# SDI is toggled on the falling edge of the SPI clock.
# Assume worst case as data being generated late and receiving an early clock:
#  - Max FPGA TCO for data and Min FPGA TCO for clock (max_fpga_gpio_jitter)
#  - Max data propagation delay and minimum clock propagation delay (max_fpga_gpio_trace_diff_dly)
set_input_delay -clock [get_clocks mb_ctrl_sck]   \
  -max [expr  $mb_ctrl_sck_period/2               \
          +   $max_fpga_gpio_jitter               \
          +   $max_fpga_gpio_trace_diff_dly ]     \
  [get_ports mMCTRL_SDI]

# Assume worst case as data being generated early and receiving a late clock:
#  - Min FPGA TCO for data and Max FPGA TCO for clock (max_fpga_gpio_jitter)
#  - Min data trace delay and Max clock trace delay (max_fpga_gpio_trace_diff_dly)
set_input_delay -clock  [get_clocks mb_ctrl_sck]  \
  -min [expr  $mb_ctrl_sck_period/2               \
          -   $max_fpga_gpio_jitter               \
          -   $max_fpga_gpio_trace_diff_dly ]     \
  [get_ports mMCTRL_SDI]

# SEN is deasserted one mb_ctrl_sck clock period before the SPI transaction starts,
# and is asserted one mb_ctrl_sck clock period after the SPI transaction ends.
# Assume worst case as data being generated late and receiving an early clock:
#  - Max FPGA TCO for data and Min FPGA TCO for clock (max_fpga_gpio_jitter)
#  - Max data propagation delay and minimum clock propagation delay (max_fpga_gpio_trace_diff_dly)
set_input_delay -clock [get_clocks mb_ctrl_sck]   \
  -max [expr  $max_fpga_gpio_jitter               \
          +   $max_fpga_gpio_trace_diff_dly ]     \
  [get_ports mMCTRL_SEN]

# Assume worst case as data being generated early and receiving a late clock:
#  - Min FPGA TCO for data and Max FPGA TCO for clock (max_fpga_gpio_jitter)
#  - Min data trace delay and Max clock trace delay (max_fpga_gpio_trace_diff_dly)
set_input_delay -clock  [get_clocks mb_ctrl_sck]  \
  -min [expr  0                                   \
          -   $max_fpga_gpio_jitter               \
          -   $max_fpga_gpio_trace_diff_dly ]     \
  [get_ports mMCTRL_SEN]

# SPI passthrough path
#--------------------------------

# Specify max delay on SPI clock from CPLD input to output pin
# This is used to derive SPI passthrough timing constraints, and
# is designed to leave about a second of slack on the paths it covers.
set max_internal_clk_prop 14.0
set min_internal_clk_prop 5.0

set_max_delay -from [get_ports MCTRL_SCLK] -to [get_ports m*XLO*_DCLK] \
              [expr $max_internal_clk_prop]
set_min_delay -from [get_ports MCTRL_SCLK] -to [get_ports m*XLO*_DCLK] \
              [expr $min_internal_clk_prop]

# SPI Passthrough
# Longest trace in the DB goes into RXLO2 (3.5 in)
set max_db_sdata_propagation_dly 0.581
set min_db_sdata_propagation_dly 0.0
# Longest trace in the DB goes into RXLO2 (3.6 in)
set max_db_sclk_propagation_dly 0.6
set min_db_sclk_propagation_dly 0.0

# LO timing requirements
set lmx2871_data_setup_time 25.0
set lmx2871_data_hold_time  25.0
set lmx2871_le_setup_time   20.0
set lmx2871_le_hold_time    10.0


# Assume worst case as data being generated late and receiving an early clock:
#  - Min clock propagation through CPLD
#  - Max data propagation delay on DB
#  - Min clock propagation delay on DB
set_output_delay -clock [get_clocks mb_ctrl_sck]  \
  -max [expr  $lmx2871_data_setup_time            \
          -   $min_internal_clk_prop              \
          +   $max_db_sdata_propagation_dly       \
          -   $min_db_sclk_propagation_dly ]      \
  [get_ports m*XLO*_DATA]

set_output_delay -clock [get_clocks mb_ctrl_sck]  \
  -max [expr  $lmx2871_le_setup_time              \
          -   $min_internal_clk_prop              \
          +   $max_db_sdata_propagation_dly       \
          -   $min_db_sclk_propagation_dly ]      \
  [get_ports m*XLO*_DLE]

# Assume worst case as data being generated early and receiving a late clock:
#  - Max clock propagation through CPLD
#  - Min data propagation delay on DB
#  - Max clock propagation delay on DB
set_output_delay -clock [get_clocks mb_ctrl_sck]  \
   -min [expr 0                                   \
          -   $lmx2871_data_hold_time             \
          -   $max_internal_clk_prop              \
          +   $min_db_sdata_propagation_dly       \
          -   $max_db_sclk_propagation_dly ]      \
  [get_ports m*XLO*_DATA]

set_output_delay -clock [get_clocks mb_ctrl_sck]  \
  -min [expr  0                                   \
          -   $lmx2871_le_hold_time               \
          -   $max_internal_clk_prop              \
          +   $min_db_sdata_propagation_dly       \
          -   $max_db_sclk_propagation_dly ]      \
  [get_ports m*XLO*_DLE]

# -- Multi-cycle path for SPI DLE --
# ----------------------------------------
# The DLE lines are defined in reference to the rising edge. The CPLD struggles to
# meet the hold timing when using conservative values for the propagation delays/margins
# This is really not a concern by design, since the DLE line will only assert one
# clock period after the SPI clock is done toggling. For this reason, a multi-cycle path
# exception can be used to relax the hold requirement
#
# edge #   30          31          32          33          34
#  sclk  __/-----\_____/-----\_____/-----\_____________________
#  DLE   _______________________________________/--------------
#                                               |           | Setup edge
#                                               |
#                                               | launch edge (for timing analysis)
#           |           |           |           | sclk is idle after 32 bits
#           3           2           1           0 --(Setup -1)
#                                    \____ Edge used for hold = 1
set_multicycle_path -hold  -start -to [get_ports {m*XLO*_DLE}] 1

# CPLD -> FPGA SPI path
#--------------------------------

# Maximum propagation delay from CPLD to FPGA (conservative)
set max_fpga_gpio_trace_dly 6.0
set min_fpga_gpio_trace_dly 0.0

# Assume worst case as data being generated late and receiving an early clock:
#  - Data latched on the falling edge of the SPI clock
#  - Max data propagation delay back to FPGA (max_fpga_gpio_trace_dly)
set_output_delay -clock [get_clocks mb_ctrl_sck]  \
  -max [expr  $mb_ctrl_sck_period/2               \
          +   $max_fpga_gpio_trace_dly   ]        \
  [get_ports mMCTRL_SDO]

# Assume worst case as data being generated early and receiving an late clock:
#  - Data latched on the falling edge of the SPI clock
#  - Min data propagation delay (min_fpga_gpio_trace_dly)
set_output_delay -clock [get_clocks mb_ctrl_sck]  \
  -min [expr  $mb_ctrl_sck_period/2               \
          +   $min_fpga_gpio_trace_dly   ]        \
  [get_ports mMCTRL_SDO]

# SPI passthrough Address lines
#--------------------------------
# These paths are asynchronous, and will only toggle when the SPI
# interface is idle.

set_false_path -from [get_ports {aMCTRL_ADDR*}] -to [get_ports {m*XLO*_D* mMCTRL_SDO}]
set_false_path -from [get_ports {aMCTRL_RESET_N}] -to [get_registers {obx_register_endpoints*spi_buf*}]
set_false_path -from [get_ports {mMCTRL_SEN}] -to [get_registers {obx_register_endpoints*spi_buf*}]
set_false_path -from [get_ports {mMCTRL_SEN}] -to [get_registers {obx_register_endpoints*cpld_spi_data_out*}]
set_false_path -from [get_ports {mMCTRL_SEN}] -to [get_registers {obx_register_endpoints*address_complete_sr*}]

#####################################################################
# Clock uncertainty
#####################################################################
# Assign some uncertainty to all clocks
set clock_uncertainty 0.150
set_clock_uncertainty -to [get_clocks *] $clock_uncertainty
derive_clock_uncertainty
