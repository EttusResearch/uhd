#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Timing analysis is performed in "usrp3/top/n3xx/dboards/rh/doc/rh_timing.xlsx".
# See this spreadsheet for more details and explanations.

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

# The SPI readback and write clocks cannot be active at the same time, as they
# originate from the same pin.
set_clock_groups -physically_exclusive \
    -group [get_clocks pl_spi_rb_clk_a] \
    -group [get_clocks pl_spi_clk_a]
set_clock_groups -physically_exclusive \
    -group [get_clocks pl_spi_rb_clk_b] \
    -group [get_clocks pl_spi_clk_b]

#*******************************************************************************
## PS SPI: since these lines all come from the PS and I don't have access to the
# driving clock (or anything for that matter), I'm left with constraining the maximum
# and minimum delay on these lines, per a Xilinx AR:
# https://www.xilinx.com/support/answers/62122.html
set CPLD_SPI_OUTS [get_ports {DB*_CPLD_PS_SPI_SCLK \
                              DB*_CPLD_PS_SPI_MOSI \
                              DB*_CPLD_PS_SPI_CS_B \
                              DB*_CLKDIS_SPI_CS_B \
                              DB*_PHDAC_SPI_CS_B \
                              DB*_ADC_SPI_CS_B \
                              DB*_DAC_SPI_CS_B}]

# The actual min and max path delays before applying constraints were (from report_timing):
#    3.332 ns (Min at Fast Process Corner)
#   10.596 ns (Max at Slow Process Corner)
# Therefore, we round those number to their immediate succesor respectively.
# After implementation, the tools were unable to meet timing when leaving a 11 ns max
# delay value, so it was incremented.
set MIN_OUT_DELAY  3.0
set MAX_OUT_DELAY 12.0

set_max_delay $MAX_OUT_DELAY -to $CPLD_SPI_OUTS
set_min_delay $MIN_OUT_DELAY -to $CPLD_SPI_OUTS

# report_timing -to $CPLD_SPI_OUTS -max_paths 20 -delay_type min_max -name CpldSpiOutTiming

# The actual min and max path delays before applying constraints were (from report_timing):
#   2.733 ns (Min at Fast Process Corner)
#   6.071 ns (Max at Slow Process Corner)
# Therefore, we round those number to their immediate succesor respectively.
set MIN_IN_DELAY   2.0
set MAX_IN_DELAY   10.0

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
# calculations assume the FPGA has less than 6 ns of skew between the SCK and
# SDI/CS_n. Pretty easy constraint to write! See above for the clock definition.
# Do this for DBA and DBB independently.
set MAX_SKEW 6.0
set SETUP_SKEW [expr {$MAX_SKEW / 2}]
set HOLD_SKEW  [expr {$MAX_SKEW / 2}]
# Do not set the output delay constraint on the clock line!
set PORT_LIST_A [get_ports {DBA_CPLD_PL_SPI_CS_B \
                            DBA_CPLD_PL_SPI_MOSI \
                            DBA_TXLO_SPI_CS_B    \
                            DBA_RXLO_SPI_CS_B    \
                            DBA_LODIS_SPI_CS_B  }]
set PORT_LIST_B [get_ports {DBB_CPLD_PL_SPI_CS_B \
                            DBB_CPLD_PL_SPI_MOSI \
                            DBB_TXLO_SPI_CS_B    \
                            DBB_RXLO_SPI_CS_B    \
                            DBB_LODIS_SPI_CS_B  }]
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
# CPLD clk-to-q is 20 ns, then add 1.2 ns for board delay (once for clock, once for data)
# For hold time, assume zero delay (likely overconstraining here, due to board delays)
set MISO_INPUT_A [get_ports DBA_CPLD_PL_SPI_MISO]
set MISO_INPUT_B [get_ports DBB_CPLD_PL_SPI_MISO]
set_input_delay -clock [get_clocks pl_spi_rb_clk_a] -clock_fall -max  22.400  $MISO_INPUT_A
set_input_delay -clock [get_clocks pl_spi_rb_clk_a] -clock_fall -min   0.000  $MISO_INPUT_A
set_input_delay -clock [get_clocks pl_spi_rb_clk_b] -clock_fall -max  22.400  $MISO_INPUT_B
set_input_delay -clock [get_clocks pl_spi_rb_clk_b] -clock_fall -min   0.000  $MISO_INPUT_B

# Since the input delay span is clearly more than a period of the radio_clk, we need to
# add a multicycle path here as well to define the clock divider ratio. The MISO data
# is driven on the falling edge of the SPI clock and captured on the rising edge, so we
# only have one half of a SPI clock cycle for our setup. Hold is left alone and is OK
# as-is due to the delays in the CPLD and board.
set SETUP_CYCLES [expr {$PL_SPI_RB_DIVIDE_VAL / 2}]
set HOLD_CYCLES 0
set_multicycle_path -setup -from [get_clocks pl_spi_rb_clk_a] -through $MISO_INPUT_A \
  $SETUP_CYCLES
set_multicycle_path -hold  -from [get_clocks pl_spi_rb_clk_a] -through $MISO_INPUT_A -end \
  [expr {$SETUP_CYCLES + $HOLD_CYCLES - 1}]
set_multicycle_path -setup -from [get_clocks pl_spi_rb_clk_b] -through $MISO_INPUT_B \
  $SETUP_CYCLES
set_multicycle_path -hold  -from [get_clocks pl_spi_rb_clk_b] -through $MISO_INPUT_B -end \
  [expr {$SETUP_CYCLES + $HOLD_CYCLES - 1}]

#*******************************************************************************
## SYSREF/SYNC JESD Timing
#
# SYNC is async, SYSREF is tightly timed.

# The SYNC output (to ADC) for both DBs is governed by the JESD cores, which are solely
# driven by DB-A clock... but it is an asynchronous signal so we use the async_out_clk.
set_output_delay -clock [get_clocks async_out_clk] 0.000 [get_ports DB*_ADC_SYNCB_P]
set_max_delay -to [get_ports DB*_ADC_SYNCB_P] 50.000
set_min_delay -to [get_ports DB*_ADC_SYNCB_P] 0.000

# The SYNC input (from DAC) for both DBs is received by the DB-A clock inside the JESD
# cores... but again, it is asynchronous and therefore uses the async_in_clk.
set_input_delay -clock [get_clocks async_in_clk] 0.000 [get_ports DB*_DAC_SYNCB_P]
set_max_delay -from [get_ports DB*_DAC_SYNCB_P] 50.000
set_min_delay -from [get_ports DB*_DAC_SYNCB_P] 0.000

# SYSREF is driven by the LMK directly to the FPGA. Timing analysis was performed once
# for the worst-case numbers across both DBs to produce one set of numbers for both DBs.
# Since we easily meet setup and hold in Vivado, then this is an acceptable approach.
# SYSREF is captured by the local clock from each DB, so we have two sets of constraints.
set_input_delay -clock fpga_clk_a_v -min -0.479 [get_ports DBA_FPGA_SYSREF_*]
set_input_delay -clock fpga_clk_a_v -max  0.661 [get_ports DBA_FPGA_SYSREF_*]

set_input_delay -clock fpga_clk_b_v -min -0.479 [get_ports DBB_FPGA_SYSREF_*]
set_input_delay -clock fpga_clk_b_v -max  0.661 [get_ports DBB_FPGA_SYSREF_*]


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

#*******************************************************************************
### Async I/Os
set DB_ASYNC_OUTPUTS [get_ports {
    DB*_MODULE_PWR_ENABLE
    DB*_RF_PWR_ENABLE
    DB*_CLKDIST_SYNC
    DB*_ATR_TX
    DB*_ATR_RX
    DB*_TXRX_SW_CTRL_1
    DB*_TXRX_SW_CTRL_2
    DB*_LED_RX
    DB*_LED_RX2
    DB*_LED_TX
    QSFP_I2C_*
}]
set_output_delay -clock [get_clocks async_out_clk] 0.000 $DB_ASYNC_OUTPUTS
set_max_delay -to $DB_ASYNC_OUTPUTS 50.000
set_min_delay -to $DB_ASYNC_OUTPUTS 0.000

set_input_delay -clock [get_clocks async_in_clk] 0.000 [get_ports QSFP_I2C_*]
set_max_delay -from [get_ports QSFP_I2C_*] 50.000
set_min_delay -from [get_ports QSFP_I2C_*] 0.000

#*******************************************************************************
## JTAG

## MAX 10 JTAG TDI setup: 2 ns
## MAX 10 JTAG TMS setup: 3 ns
## MAX 10 JTAG hold: 10 ns
## MAX 10 JTAG clk-to-q: 18 ns
## Board delay: < 1.5 ns
##
## Setup time = Board delay + TMS setup = 3 ns + 1.5 ns = 4.5 ns
## Hold time = Board delay + TMS hold = 1.5 ns + 10 ns = 11.5 ns
## Overconstrain output delay and keep skew to +/- 8 ns
##
## Input delay = 2x Board delay + clk-to-q = 3 ns + 18 ns = 21 ns

# Constrain outputs for skew, with same latch/launch edge:
set_output_delay                        -clock [get_clocks dba_jtag_tck] -max -4.0 \
    [get_ports {DBA_CPLD_JTAG_TDI DBA_CPLD_JTAG_TMS}]
set_output_delay -add_delay -clock_fall -clock [get_clocks dba_jtag_tck] -max -4.0 \
    [get_ports {DBA_CPLD_JTAG_TDI DBA_CPLD_JTAG_TMS}]
set_output_delay                        -clock [get_clocks dba_jtag_tck] -min  4.0 \
    [get_ports {DBA_CPLD_JTAG_TDI DBA_CPLD_JTAG_TMS}]
set_output_delay -add_delay -clock_fall -clock [get_clocks dba_jtag_tck] -min  4.0 \
    [get_ports {DBA_CPLD_JTAG_TDI DBA_CPLD_JTAG_TMS}]
set_output_delay                        -clock [get_clocks dbb_jtag_tck] -max -4.0 \
    [get_ports {DBB_CPLD_JTAG_TDI DBB_CPLD_JTAG_TMS}]
set_output_delay -add_delay -clock_fall -clock [get_clocks dbb_jtag_tck] -max -4.0 \
    [get_ports {DBB_CPLD_JTAG_TDI DBB_CPLD_JTAG_TMS}]
set_output_delay                        -clock [get_clocks dbb_jtag_tck] -min  4.0 \
    [get_ports {DBB_CPLD_JTAG_TDI DBB_CPLD_JTAG_TMS}]
set_output_delay -add_delay -clock_fall -clock [get_clocks dbb_jtag_tck] -min  4.0 \
    [get_ports {DBB_CPLD_JTAG_TDI DBB_CPLD_JTAG_TMS}]
# Finally, make both the setup and hold checks use the same launching and latching edges.
set_multicycle_path -setup -from [get_clocks clk40] -to [get_clocks dba_jtag_tck] -start 0
set_multicycle_path -hold  -from [get_clocks clk40] -to [get_clocks dba_jtag_tck] -1
set_multicycle_path -setup -from [get_clocks clk40] -to [get_clocks dbb_jtag_tck] -start 0
set_multicycle_path -hold  -from [get_clocks clk40] -to [get_clocks dbb_jtag_tck] -1

set_input_delay -clock [get_clocks dba_jtag_tck] -clock_fall -max 21 \
    [get_ports DBA_CPLD_JTAG_TDO]
set_input_delay -clock [get_clocks dba_jtag_tck] -clock_fall -min 0 \
    [get_ports DBA_CPLD_JTAG_TDO]
set_input_delay -clock [get_clocks dbb_jtag_tck] -clock_fall -max 21 \
    [get_ports DBB_CPLD_JTAG_TDO]
set_input_delay -clock [get_clocks dbb_jtag_tck] -clock_fall -min 0 \
    [get_ports DBB_CPLD_JTAG_TDO]
# Inputs have setup checks relative to half a period of TCK (launch on fall,
# latch on rise). Actual latch clock is faster, so push back setup and hold
# checks to match.
set_multicycle_path -setup -from [get_clocks dba_jtag_tck] \
    -through [get_ports DBA_CPLD_JTAG_TDO] \
    [expr {$DB_JTAG_DIVISOR / 2}]
set_multicycle_path -end -hold -from [get_clocks dba_jtag_tck] \
    -through [get_ports DBA_CPLD_JTAG_TDO] \
    [expr {$DB_JTAG_DIVISOR - 1}]
set_multicycle_path -setup -from [get_clocks dbb_jtag_tck] \
    -through [get_ports DBB_CPLD_JTAG_TDO] \
    [expr {$DB_JTAG_DIVISOR / 2}]
set_multicycle_path -end -hold -from [get_clocks dbb_jtag_tck] \
    -through [get_ports DBB_CPLD_JTAG_TDO] \
    [expr {$DB_JTAG_DIVISOR - 1}]
