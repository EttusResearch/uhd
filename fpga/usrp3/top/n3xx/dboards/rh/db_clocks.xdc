#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Timing analysis is performed in "usrp3/top/n3xx/dboards/rh/doc/rh_timing.xlsx".
# See this spreadsheet for more details and explanations.

#*******************************************************************************
## Daughterboard Clocks
#
# 122.88, 200, 245.76 and 250 MHz Sample Rates are allowable with 2:1/1:2 DSP and
# 2 samples/cycle arriving at the FPGA:
#
#            <-- 2:1/1:2 -->
#     | Supported   | Sample rate  | FPGA Clk  |
#     |sample rates | at JESD core | Frequency |
#     |   (MSPS)    |    (MSPS)    |   (MHz)   |
#     |-------------|--------------|-----------|
#     |   122.88    |    491.52    |  245.76   | (uses DUC/DDC)
#     |   200.00    |    400.00    |  200.00   |
#     |   245.76    |    491.52    |  245.76   |
#     |   250.00    |    500.00    |  250.00   |
#
# Therefore, supported sample clocks are: 122.88, 200, 245.76 and 250 MHz.
# Constrain the paths to the max rate to support all rates in a single FPGA image.
set SAMPLE_CLK_PERIOD 4.00
create_clock -name fpga_clk_a  -period $SAMPLE_CLK_PERIOD  [get_ports DBA_FPGA_CLK_P]
create_clock -name fpga_clk_b  -period $SAMPLE_CLK_PERIOD  [get_ports DBB_FPGA_CLK_P]
create_clock -name mgt_clk_dba -period $SAMPLE_CLK_PERIOD  [get_ports DBA_MGTCLK_P]
create_clock -name mgt_clk_dbb -period $SAMPLE_CLK_PERIOD  [get_ports DBB_MGTCLK_P]

# The Radio Clocks coming from the DBs are synchronized together (at the converters) to
# a typical value of less than 100ps. To give ourselves and Vivado some margin, we claim
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
# it cancels out the source latency during analysis. D. Jepson tested this by setting
# the early and late numbers to zero and then their actual value, running timing reports
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
# set by SW at run-time. Current divider value is 125 based on what radio clock
# rate is set.
# For the CPLD SPI endpoint alone, we need it to run at ~25 MHz (writes only), this means
# that at times, the PL SPI will have its divider set to 10 (radio_clock = 250 MHz) or 8
# (radio_clock = 200 MHz).
# The readback clock is lower (~10 MHz), so create a separate clock for it.
# Use readback divide value of 24 for an even divider (and some overconstraining).
set PL_SPI_DIVIDE_VAL 10
set PL_SPI_RB_DIVIDE_VAL 24
set PL_SPI_CLK_A [get_ports DBA_CPLD_PL_SPI_SCLK]
create_generated_clock -name pl_spi_clk_a \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $PL_SPI_CLK_A]/C] \
  -divide_by $PL_SPI_DIVIDE_VAL $PL_SPI_CLK_A
create_generated_clock -name pl_spi_rb_clk_a \
  -master_clock [get_clocks radio_clk] \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $PL_SPI_CLK_A]/C] \
  -divide_by $PL_SPI_RB_DIVIDE_VAL -add $PL_SPI_CLK_A
set PL_SPI_CLK_B [get_ports DBB_CPLD_PL_SPI_SCLK]
create_generated_clock -name pl_spi_clk_b \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $PL_SPI_CLK_B]/C] \
  -divide_by $PL_SPI_DIVIDE_VAL $PL_SPI_CLK_B
create_generated_clock -name pl_spi_rb_clk_b \
  -master_clock [get_clocks radio_clk] \
  -source [get_pins [all_fanin -flat -only_cells -startpoints_only $PL_SPI_CLK_B]/C] \
  -divide_by $PL_SPI_RB_DIVIDE_VAL -add $PL_SPI_CLK_B

#*******************************************************************************
## JTAG
set DB_JTAG_DIVISOR 4
create_generated_clock -name dba_jtag_tck -divide_by $DB_JTAG_DIVISOR \
    -source [get_pins {inst_n310_ps/jtag_0/U0/bitq_ctrl/bitq_state_reg[1]/C}] \
    [get_ports DBA_CPLD_JTAG_TCK]

create_generated_clock -name dbb_jtag_tck -divide_by $DB_JTAG_DIVISOR \
    -source [get_pins {inst_n310_ps/jtag_1/U0/bitq_ctrl/bitq_state_reg[1]/C}] \
    [get_ports DBB_CPLD_JTAG_TCK]

