#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Timing analysis is performed in "/n3xx/dboards/mg/doc/mg_timing.xlsx". See
# the spreadsheet for more details and explanations.

#*******************************************************************************
## Daughterboard Clocks

# 122.88, 125, and 153.6 MHz Sample Clocks are allowable. Constrain the paths to the max
# rate in order to support all rates in a single FPGA image.
set SAMPLE_CLK_PERIOD 6.510
create_clock -name fpga_clk_a  -period $SAMPLE_CLK_PERIOD  [get_ports DBA_FPGA_CLK_P]
create_clock -name fpga_clk_b  -period $SAMPLE_CLK_PERIOD  [get_ports DBB_FPGA_CLK_P]
create_clock -name mgt_clk_dba -period $SAMPLE_CLK_PERIOD  [get_ports USRPIO_A_MGTCLK_P]
create_clock -name mgt_clk_dbb -period $SAMPLE_CLK_PERIOD  [get_ports USRPIO_B_MGTCLK_P]

# The Radio Clocks coming from the DBs are synchronized together (at the ADCs) to a
# typical value of less than 100ps. To give ourselves and Vivado some margin, we claim
# here that the DB-B Radio Clock can arrive 500ps before or after the DB-A clock at
# the FPGA (note that the trace lengths of the Radio Clocks coming from the DBs to the
# FPGA are about 0.5" different, thereby incurring ~80ps of additional skew at the FPGA).
# There is one spot in the FPGA where we cross domains between the DB-A and
# DB-B clock, so we must ensure that Vivado can analyze that path safely.
set FPGA_CLK_EARLY -0.5
set FPGA_CLK_LATE   0.5
set_clock_latency  -source -early $FPGA_CLK_EARLY [get_clocks fpga_clk_b]
set_clock_latency  -source -late  $FPGA_CLK_LATE  [get_clocks fpga_clk_b]

# Virtual clocks for constraining I/O (used below)
create_clock -name fpga_clk_a_v -period $SAMPLE_CLK_PERIOD
create_clock -name fpga_clk_b_v -period $SAMPLE_CLK_PERIOD

# The set_clock_latency constraints set on fpga_clk_b are problematic when used with
# I/O timing, since the analyzer gives us a double-hit on the latency. One workaround
# (used here) is to simply swap the early and late times for the virtual clock so that
# it cancels out the source latency during analysis. I tested this by setting the
# early and late numbers to zero and then their actual value, running timing reports
# on each. The slack report matches for both cases, showing that the reversed early/late
# numbers on the virtual clock zero out the latency effects on the actual clock.
#
# Note this is not a problem for the fpga_clk_a, since no latency is added. So only apply
# it to fpga_clk_b_v.
set_clock_latency  -source -early $FPGA_CLK_LATE  [get_clocks fpga_clk_b_v]
set_clock_latency  -source -late  $FPGA_CLK_EARLY [get_clocks fpga_clk_b_v]



#*******************************************************************************
## Aliases for auto-generated clocks

create_generated_clock -name radio_clk_fb   [get_pins {dba_core/RadioClockingx/RadioClkMmcm/CLKFBOUT}]
create_generated_clock -name radio_clk      [get_pins {dba_core/RadioClockingx/RadioClkMmcm/CLKOUT0}]
create_generated_clock -name radio_clk_2x   [get_pins {dba_core/RadioClockingx/RadioClkMmcm/CLKOUT1}]

create_generated_clock -name radio_clk_b_fb [get_pins {dbb_core/RadioClockingx/RadioClkMmcm/CLKFBOUT}]
create_generated_clock -name radio_clk_b    [get_pins {dbb_core/RadioClockingx/RadioClkMmcm/CLKOUT0}]
create_generated_clock -name radio_clk_b_2x [get_pins {dbb_core/RadioClockingx/RadioClkMmcm/CLKOUT1}]



#*******************************************************************************
## Generated clocks for output busses to the daughterboard
#
# These clock definitions need to come above the set_clock_groups commands below to work!

# Define clocks on the PL SPI clock output pins for both DBs. Actual divider values are
# set by SW at run-time. Divider values are 123, 125, or 154 based on what radio clock
# rate is set. To be ultra-conservative (which still provides 10s of ns of slack), we
# set an over-constrained divider value of 50.
set PL_SPI_DIVIDE_VAL 50
set PL_SPI_CLK_A [get_ports DBA_CPLD_PL_SPI_SCLK]
create_generated_clock -name pl_spi_clk_a \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $PL_SPI_CLK_A]/C] \
  -divide_by $PL_SPI_DIVIDE_VAL $PL_SPI_CLK_A
set PL_SPI_CLK_B [get_ports DBB_CPLD_PL_SPI_SCLK]
create_generated_clock -name pl_spi_clk_b \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $PL_SPI_CLK_B]/C] \
  -divide_by $PL_SPI_DIVIDE_VAL $PL_SPI_CLK_B

# Define one of the outputs of each bus as a clock (even though it isn't a clock). This
# allows us to constrain the overall bus skew with respect to one of the bus outputs.
# See the remainder of this constraint below for more details.
set DSA_CLK [get_ports {DBA_CH1_RX_DSA_DATA[0]}]
create_generated_clock -name dsa_bus_clk \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $DSA_CLK]/C] \
  -divide_by 2 $DSA_CLK

set ATR_CLK [get_ports DBA_ATR_RX_1]
create_generated_clock -name atr_bus_clk \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $ATR_CLK]/C] \
  -divide_by 2 $ATR_CLK

# Interface Unused
# set MGPIO_CLK [get_ports DBA_MYK_GPIO_0]
# create_generated_clock -name myk_gpio_bus_clk \
  # -source [get_pins [all_fanin -flat -only_cells -startpoints_only [get_ports $MGPIO_CLK]]/C] \
  # -divide_by 2 [get_ports $MGPIO_CLK]



#*******************************************************************************
## Asynchronous clock groups

# MGT reference clocks are also async to everything.
set_clock_groups -asynchronous -group [get_clocks mgt_clk_dba -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks mgt_clk_dbb -include_generated_clocks]

# fpga_clk_a and fpga_clk_b are related to one another after synchronization.
# However, we do need to declare that these clocks (both a and b) and their children
# are async to the remainder of the design. Use the wildcard at the end to grab the
# virtual clock as well as the real ones.
set_clock_groups -asynchronous -group [get_clocks {fpga_clk_a* fpga_clk_b*} -include_generated_clocks]



#*******************************************************************************
## PS SPI: since these lines all come from the PS and I don't have access to the
# driving clock (or anything for that matter), I'm left with constraining the maximum
# and minimum delay on these lines, per a Xilinx AR:
# https://www.xilinx.com/support/answers/62122.html
set CPLD_SPI_OUTS [get_ports {DB*_CPLD_PS_SPI_SCLK \
                              DB*_CPLD_PS_SPI_SDI \
                              DB*_CPLD_PS_SPI_LE \
                              DB*_CPLD_PS_SPI_ADDR[0] \
                              DB*_CPLD_PS_SPI_ADDR[1]}]

set_max_delay 12.0 -to $CPLD_SPI_OUTS
set_min_delay  3.0 -to $CPLD_SPI_OUTS

set MYK_SPI_OUTS  [get_ports {DB*_MYK_SPI_SCLK \
                              DB*_MYK_SPI_SDIO \
                              DB*_MYK_SPI_CS_n}]

set_max_delay 14.0 -to $MYK_SPI_OUTS
set_min_delay  3.0 -to $MYK_SPI_OUTS

# report_timing -to $CPLD_SPI_OUTS -max_paths 20 -delay_type min_max -name CpldSpiOutTiming
# report_timing -to $MYK_SPI_OUTS  -max_paths 20 -delay_type min_max -name MykSpiOutTiming

set MIN_IN_DELAY   2.0
set MAX_IN_DELAY  10.0

set PS_SPI_INPUTS_0 [get_pins -hierarchical -filter {NAME =~ "*/PS7_i/EMIOSPI0MI"}]
set PS_SPI_INPUTS_1 [get_pins -hierarchical -filter {NAME =~ "*/PS7_i/EMIOSPI1MI"}]

set_max_delay $MAX_IN_DELAY -to $PS_SPI_INPUTS_0
set_min_delay $MIN_IN_DELAY -to $PS_SPI_INPUTS_0
set_max_delay $MAX_IN_DELAY -to $PS_SPI_INPUTS_1
set_min_delay $MIN_IN_DELAY -to $PS_SPI_INPUTS_1

# report_timing -to $PS_SPI_INPUTS_0 -max_paths 30 -delay_type min_max -nworst 30 -name Spi0InTiming
# report_timing -to $PS_SPI_INPUTS_1 -max_paths 30 -delay_type min_max -nworst 30 -name Spi1InTiming



#*******************************************************************************
## PL SPI to the CPLD
#
# All of these lines are driven or received from flops in simple_spi_core. The CPLD
# calculations assume the FPGA has less than 20 ns of skew between the SCK and
# SDI/CS_n. Pretty easy constraint to write! See above for the clock definition.
# Do this for DBA and DBB independently.
set MAX_SKEW 20.0
set SETUP_SKEW [expr {$MAX_SKEW / 2}]
set HOLD_SKEW  [expr {$MAX_SKEW / 2}]
# Do not set the output delay constraint on the clock line!
set PORT_LIST_A [get_ports {DBA_CPLD_PL_SPI_LE \
                            DBA_CPLD_PL_SPI_SDI \
                            DBA_CPLD_PL_SPI_ADDR[0] \
                            DBA_CPLD_PL_SPI_ADDR[1]}]
set PORT_LIST_B [get_ports {DBB_CPLD_PL_SPI_LE \
                            DBB_CPLD_PL_SPI_SDI \
                            DBB_CPLD_PL_SPI_ADDR[0] \
                            DBB_CPLD_PL_SPI_ADDR[1]}]
# Then add the output delay on each of the ports.
set_output_delay                        -clock [get_clocks pl_spi_clk_a] -max -$SETUP_SKEW $PORT_LIST_A
set_output_delay -add_delay -clock_fall -clock [get_clocks pl_spi_clk_a] -max -$SETUP_SKEW $PORT_LIST_A
set_output_delay                        -clock [get_clocks pl_spi_clk_a] -min  $HOLD_SKEW  $PORT_LIST_A
set_output_delay -add_delay -clock_fall -clock [get_clocks pl_spi_clk_a] -min  $HOLD_SKEW  $PORT_LIST_A
set_output_delay                        -clock [get_clocks pl_spi_clk_b] -max -$SETUP_SKEW $PORT_LIST_B
set_output_delay -add_delay -clock_fall -clock [get_clocks pl_spi_clk_b] -max -$SETUP_SKEW $PORT_LIST_B
set_output_delay                        -clock [get_clocks pl_spi_clk_b] -min  $HOLD_SKEW  $PORT_LIST_B
set_output_delay -add_delay -clock_fall -clock [get_clocks pl_spi_clk_b] -min  $HOLD_SKEW  $PORT_LIST_B
# Finally, make both the setup and hold checks use the same launching and latching edges.
set_multicycle_path -setup -from [get_clocks radio_clk] -to [get_clocks pl_spi_clk_a] -start 0
set_multicycle_path -hold  -from [get_clocks radio_clk] -to [get_clocks pl_spi_clk_a] -1
set_multicycle_path -setup -from [get_clocks radio_clk] -to [get_clocks pl_spi_clk_b] -start 0
set_multicycle_path -hold  -from [get_clocks radio_clk] -to [get_clocks pl_spi_clk_b] -1

# For SDO input timing (MISO), we need to look at the CPLD's constraints on turnaround
# time plus any board propagation delay.
set MISO_INPUT_A [get_ports DBA_CPLD_PL_SPI_SDO]
set MISO_INPUT_B [get_ports DBB_CPLD_PL_SPI_SDO]
set_input_delay -clock [get_clocks pl_spi_clk_a] -clock_fall -max  68.041 $MISO_INPUT_A
set_input_delay -clock [get_clocks pl_spi_clk_a] -clock_fall -min  12.218 $MISO_INPUT_A
set_input_delay -clock [get_clocks pl_spi_clk_b] -clock_fall -max  68.041 $MISO_INPUT_B
set_input_delay -clock [get_clocks pl_spi_clk_b] -clock_fall -min  12.218 $MISO_INPUT_B
# Since the input delay span is clearly more than a period of the radio_clk, we need to
# add a multicycle path here as well to define the clock divider ratio. The MISO data
# is driven on the falling edge of the SPI clock and captured on the rising edge, so we
# only have one half of a SPI clock cycle for our setup. Hold is left alone and is OK
# as-is due to the delays in the CPLD and board.
set SETUP_CYCLES [expr {$PL_SPI_DIVIDE_VAL / 2}]
set HOLD_CYCLES 0
set_multicycle_path -setup -from [get_clocks pl_spi_clk_a] -through $MISO_INPUT_A \
  $SETUP_CYCLES
set_multicycle_path -hold  -from [get_clocks pl_spi_clk_a] -through $MISO_INPUT_A -end \
  [expr {$SETUP_CYCLES + $HOLD_CYCLES - 1}]
set_multicycle_path -setup -from [get_clocks pl_spi_clk_b] -through $MISO_INPUT_B \
  $SETUP_CYCLES
set_multicycle_path -hold  -from [get_clocks pl_spi_clk_b] -through $MISO_INPUT_B -end \
  [expr {$SETUP_CYCLES + $HOLD_CYCLES - 1}]

# One of the PL_SPI_ADDR lines is used instead for the LMK SYNC strobe. This line is
# driven asynchronously.
set_output_delay -clock [get_clocks async_out_clk] 0.000 [get_ports DB*_CPLD_PL_SPI_ADDR[2]]
set_max_delay -to [get_ports DB*_CPLD_PL_SPI_ADDR[2]] 50.000
set_min_delay -to [get_ports DB*_CPLD_PL_SPI_ADDR[2]] 0.000



#*******************************************************************************
## DSA Bus
# The DSA controls are driven from the DB-A radio clock. Although they are received async
# at the DSAs, they should be tightly constrained in the FPGA to arrive as closely as
# possible. The best way to do this is a skew constraint across all the bits.
set MAX_SKEW 2.5
set SETUP_SKEW [expr {($MAX_SKEW / 2)+0.5}]
set HOLD_SKEW  [expr {($MAX_SKEW / 2)-0.5}]
set PORT_LIST [get_ports {DB*_CH*_*X_DSA_DATA[*]}]
# Then add the output delay on each of the ports.
set_output_delay                        -clock [get_clocks dsa_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks dsa_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay                        -clock [get_clocks dsa_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks dsa_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
# Finally, make both the setup and hold checks use the same launching and latching edges.
# The clock, which is essentially one of the data lines, should arrive at the pin
# +/- MAX_DELAY compared to the other data lines, so setup and hold checks need to be
# relative to the SAME edges for both the clock and the data.
set_multicycle_path -setup -from [get_clocks radio_clk] -to [get_clocks dsa_bus_clk] -start 0
set_multicycle_path -hold  -from [get_clocks radio_clk] -to [get_clocks dsa_bus_clk] -1
# Remove analysis from the output "clock" pin. There are ways to do this using TCL, but
# they aren't supported in XDC files... so we do it the old fashioned way.
set_output_delay -clock [get_clocks async_out_clk] 0.000 $DSA_CLK
set_max_delay -to $DSA_CLK 50.000
set_min_delay -to $DSA_CLK 0.000



#*******************************************************************************
## ATR Bus
# The ATR bits are driven from the DB-A radio clock. Although they are received async in
# the CPLD, they should be tightly constrained in the FPGA to avoid any race conditions.
# The best way to do this is a skew constraint across all the bits.
set MAX_SKEW 2.5
set SETUP_SKEW [expr {($MAX_SKEW / 2)+0.5}]
set HOLD_SKEW  [expr {($MAX_SKEW / 2)-0.5}]
set PORT_LIST [get_ports DB*_ATR_*X_*]
# Then add the output delay on each of the ports.
set_output_delay                        -clock [get_clocks atr_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks atr_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay                        -clock [get_clocks atr_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks atr_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
# Finally, make both the setup and hold checks use the same launching and latching edges.
set_multicycle_path -setup -to [get_clocks atr_bus_clk] -start 0
set_multicycle_path -hold  -to [get_clocks atr_bus_clk] -1
# Remove analysis from the output "clock" pin. There are ways to do this using TCL, but
# they aren't supported in XDC files... so we do it the old fashioned way.
set_output_delay -clock [get_clocks async_out_clk] 0.000 $ATR_CLK
set_max_delay -to $ATR_CLK 50.000
set_min_delay -to $ATR_CLK 0.000



#*******************************************************************************
## Mykonos Ports
# Mykonos GPIO is driven from the DB-A radio clock. Although they are received async in
# Mykonos, they should be tightly constrained in the FPGA to avoid any race conditions.
# The best way to do this is a skew constraint across all the bits.
# set MAX_SKEW 2.5
# set SETUP_SKEW [expr {($MAX_SKEW / 2)+0.5}]
# set HOLD_SKEW  [expr {($MAX_SKEW / 2)-0.5}]
# set PORT_LIST [get_ports DB*_ATR_*X_*]
# # Then add the output delay on each of the ports.
# set_output_delay                        -clock [get_clocks myk_gpio_bus_clk] -max -$SETUP_SKEW $PORT_LIST
# set_output_delay -add_delay -clock_fall -clock [get_clocks myk_gpio_bus_clk] -max -$SETUP_SKEW $PORT_LIST
# set_output_delay                        -clock [get_clocks myk_gpio_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
# set_output_delay -add_delay -clock_fall -clock [get_clocks myk_gpio_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
# # Finally, make both the setup and hold checks use the same launching and latching edges.
# set_multicycle_path -setup -to [get_clocks myk_gpio_bus_clk] -start 0
# set_multicycle_path -hold  -to [get_clocks myk_gpio_bus_clk] -1
# # Remove analysis from the output "clock" pin. There are ways to do this using TCL, but
# # they aren't supported in XDC files... so we do it the old fashioned way.
# set_output_delay -clock [get_clocks async_out_clk] 0.000 $MGPIO_CLK
# set_max_delay -to $MGPIO_CLK 50.000
# set_min_delay -to $MGPIO_CLK 0.000

# Mykonos Interrupt is received asynchronously, and driven directly to the PS.
set_input_delay -clock [get_clocks async_in_clk] 0.000 [get_ports DB*_MYK_INTRQ]
set_max_delay -from [get_ports DB*_MYK_INTRQ] 50.000
set_min_delay -from [get_ports DB*_MYK_INTRQ] 0.000



#*******************************************************************************
## SYSREF/SYNC JESD Timing
#
# SYNC is async, SYSREF is tightly timed.

# The SYNC output for both DBs is governed by the JESD cores, which are solely driven by
# DB-A clock... but it is an asynchronous signal so we use the async_out_clk.
set_output_delay -clock [get_clocks async_out_clk] 0.000 [get_ports DB*_MYK_SYNC_IN_n]
set_max_delay -to [get_ports DB*_MYK_SYNC_IN_n] 50.000
set_min_delay -to [get_ports DB*_MYK_SYNC_IN_n] 0.000

# The SYNC input for both DBs is received by the DB-A clock inside the JESD cores... but
# again, it is asynchronous and therefore uses the async_in_clk.
set_input_delay -clock [get_clocks async_in_clk] 0.000 [get_ports DB*_MYK_SYNC_OUT_n]
set_max_delay -from [get_ports DB*_MYK_SYNC_OUT_n] 50.000
set_min_delay -from [get_ports DB*_MYK_SYNC_OUT_n] 0.000

# SYSREF is driven by the LMK directly to the FPGA. Timing analysis was performed once
# for the worst-case numbers across both DBs to produce one set of numbers for both DBs.
# Since we easily meet setup and hold in Vivado, then this is an acceptable approach.
# SYSREF is captured by the local clock from each DB, so we have two sets of constraints.
set_input_delay -clock fpga_clk_a_v -min -0.906 [get_ports DBA_FPGA_SYSREF_*]
set_input_delay -clock fpga_clk_a_v -max  0.646 [get_ports DBA_FPGA_SYSREF_*]

set_input_delay -clock fpga_clk_b_v -min -0.906 [get_ports DBB_FPGA_SYSREF_*]
set_input_delay -clock fpga_clk_b_v -max  0.646 [get_ports DBB_FPGA_SYSREF_*]



#*******************************************************************************
## PPS Timing

# Due to the N3xx synchronization and clocking structure, the PPS output is driven from
# the Sample Clock domain instead of the input Reference Clock. Constrain the output as
# tightly as possible to accurately mimic the internal Sample Clock timing.
set SETUP_SKEW  2.0
set HOLD_SKEW  -0.5
set_output_delay -clock [get_clocks fpga_clk_a_v] -max -$SETUP_SKEW [get_ports REF_1PPS_OUT]
set_output_delay -clock [get_clocks fpga_clk_a_v] -min  $HOLD_SKEW  [get_ports REF_1PPS_OUT]
set_multicycle_path -setup -to [get_ports REF_1PPS_OUT] -start 0
set_multicycle_path -hold  -to [get_ports REF_1PPS_OUT] -1
