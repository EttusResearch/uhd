# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Copyright 2019 Ettus Research, A National Instruments Company
#
# Timing constraints for Rhodium's MAX 10 board controller

set_time_format -unit ns -decimal_places 3

# Some constants for constraining the design with the FPGA-centric method:
#     Maximum trace propagation delay is assumed to be 0.6 ns on any traces
#     to on-dboard slaves
set board_delay 0.600
set clk_uncertainty 0.150

###############################################################################
# Clocks
###############################################################################

# The PS SPI clock is maximum 10 MHz. It is driven from another source and
# provided with the data.
# CPLD_PS_SPI_CLK_25: 8 MHz
set sclk_ps_period 125.000

# Create clock for the PS's SPI interface
create_clock -name sclk_ps -period $sclk_ps_period \
    [get_ports CPLD_PS_SPI_CLK_25]

# The PL SPI clock is split into two pieces. For the normal case, the clock
# frequency is 10 MHz. This is for any read operations.
#
# CPLD_PL_SPI_SCLK_18 pass through / read back ONLY: 10 MHz
set sclk_pl_period 100.000
create_clock -name sclk_pl -period $sclk_pl_period \
    [get_ports CPLD_PL_SPI_SCLK_18]

# We can go faster for the PL writes to the internal registers and LOs.
# This rate is not supported for readback, but it helps with getting the DSA
# settings and LO settings in faster.
# CPLD_PL_SPI_SCLK_18 internal ONLY: 25 MHz
set sclk_pl_wr_period 40.000
create_clock -name sclk_pl_wr -period $sclk_pl_wr_period \
    [get_ports CPLD_PL_SPI_SCLK_18] -add

# Output clocks for the MAX 10's SPI master interfaces (1 for each slave IC)
create_generated_clock -source [get_ports CPLD_PS_SPI_CLK_25] \
    -name clkdist_clk [get_ports CLKDIST_SPI_SCLK]
create_generated_clock -source [get_ports CPLD_PS_SPI_CLK_25] \
    -name adc_clk [get_ports ADC_SPI_SCLK_18]
create_generated_clock -source [get_ports CPLD_PS_SPI_CLK_25] \
    -name dac_clk [get_ports DAC_SPI_SCLK_18]
create_generated_clock -source [get_ports CPLD_PS_SPI_CLK_25] \
    -name phdac_clk [get_ports PHDAC_SPI_SCLK]

create_generated_clock -source [get_ports CPLD_PL_SPI_SCLK_18] \
    -master_clock [get_clocks sclk_pl] \
    -name lo_clk [get_ports LO_SPI_SCLK]
create_generated_clock -source [get_ports CPLD_PL_SPI_SCLK_18] \
    -master_clock [get_clocks sclk_pl_wr] \
    -name lo_wr_clk [get_ports LO_SPI_SCLK] -add
create_generated_clock -source [get_ports CPLD_PL_SPI_SCLK_18] \
    -master_clock [get_clocks sclk_pl] \
    -name lodist_clk [get_ports LODIST_Bd_SPI_SCLK]

# Virtual clock for DSA writes for skew calculations
#create_generated_clock -source [get_pins lo_gain_table\|dsa1_le\|clk]
create_generated_clock -source [get_ports CPLD_PL_SPI_SCLK_18] \
    -master_clock [get_clocks sclk_pl_wr] \
    -name dsa_reg_clk [get_pins lo_gain_table\|dsa1_le\|q]

create_generated_clock -source [get_pins lo_gain_table\|dsa1_le\|q] \
    -name dsa_clk [get_ports RxLO_DSA_LE]

# PL's pass through clock doesn't interact with internal clock
set_clock_groups -physically_exclusive \
    -group [get_clocks {sclk_pl_wr lo_wr_clk dsa_reg_clk dsa_clk}] \
    -group [get_clocks {sclk_pl lo_clk lodist_clk}]

set_clock_groups -asynchronous \
    -group [get_clocks {sclk_ps clkdist_clk adc_clk dac_clk phdac_clk}] \
    -group [get_clocks {sclk_pl sclk_pl_wr lo_clk lodist_clk}]

set_clock_uncertainty -to [get_clocks {sclk_ps sclk_pl sclk_pl_wr clkdist_clk
    adc_clk dac_clk phdac_clk lo_clk lo_wr_clk lodist_clk dsa_reg_clk dsa_clk}] \
    $clk_uncertainty

###############################################################################
# Timing Budget Calculations
###############################################################################
# Here we carve out some timing budget for the master's SPI interfaces.
# The master will use these values to time its SPI interface.
# The PL's write-only values are smaller because there are no external chip
# dependencies.
set setup_ps 25
set hold_ps 30

# PL SPI is constrained on the master with an allowed skew value of +/- 3 ns
# relative to the launch clock
# Increase to 5 ns here for more margin
set pl_skew 5

# Clocks are nominally a 50% duty cycle, so difference between latch and
# launch is half a period, so subtract allowed skew from that for setup/hold
# specification. The half period is due to launch being the falling edge and
# latch being the rising edge.
set setup_pl [expr {$sclk_pl_period / 2 - $pl_skew}]
set hold_pl [expr {$sclk_pl_period / 2 - $pl_skew}]
set setup_pl_wr [expr {$sclk_pl_wr_period / 2 - $pl_skew}]
set hold_pl_wr [expr {$sclk_pl_wr_period / 2 - $pl_skew}]

# Calculate input delays relative to falling edge (launch)
# Min is hold time after previous rising edge (previous latch)
# Max is setup time before next rising edge (next latch)
set input_delay_min_ps [expr {-$sclk_ps_period / 2 + $hold_ps}]
set input_delay_max_ps [expr {$sclk_ps_period / 2 - $setup_ps}]
set input_delay_min_pl [expr {-$sclk_pl_period / 2 + $hold_pl}]
set input_delay_max_pl [expr {$sclk_pl_period / 2 - $setup_pl}]
set input_delay_min_pl_wr [expr {-$sclk_pl_wr_period / 2 + $hold_pl_wr}]
set input_delay_max_pl_wr [expr {$sclk_pl_wr_period / 2 - $setup_pl_wr}]

# Again, carve out timing budget for master's SPI interface
# Readback on the master will depend on clk-to-q of our slave.
# These values will need to be at least as large as the worst slave.
# Clock arrival at the slave will be delayed by propagation through the MAX 10
# Data to the MAX 10's input port will be further delayed by slave's clk-to-q
# On top of that, we then need budget for the data to cross the MAX 10, head
# out the I/O, and propagate to the master's pin. Then the master will need
# some budget for setup time.
#
# Here is what we'll budget:
# Clk propagation to I/O: 7 ns
# Clk trace delay: 1 ns
# Worst-case chip clk-to-q: 10 ns
# Data trace delay: 1 ns
# Data propagation delay from input pin to output pin: 8 ns
# Total clk-to-q from MAX 10 sclk input to MAX 10 output: 27 ns
#
# Then master's budget is 23 ns for clock delay + data delay + setup time
set clk_q_max_ps 27.000

# For the PL, the worst-case chip changes to 2 ns, so there is more budget
set clk_q_max_pl 20.000

# clk-to-q determines one side of the data invalid window
# The maximum output delay is simply latch edge - clk-to-q
# Launch is falling edge, and latch is rising edge, so...
set output_delay_max_ps [expr {$sclk_ps_period / 2 - $clk_q_max_ps}]
set output_delay_max_pl [expr {$sclk_pl_period / 2 - $clk_q_max_pl}]

# The minimum output delay represents the other edge of the data invalid
# window. Our clock is likely quite delayed already, but add a little more
# margin for hold time.
set output_delay_min_ps -5.000
set output_delay_min_pl -5.000


###############################################################################
# I/O groups (for reference later)
###############################################################################

# Chip selects
set ps_csb [get_ports {
    CPLD_PS_ADDR0_25
    CPLD_PS_ADDR1_25
    CPLD_PS_SPI_LE_25
    usrp_io[12]
    usrp_io[13]
}]
set pl_csb [get_ports {
    CPLD_PL_SPI_ADDR0_18
    CPLD_PL_SPI_ADDR1_18
    CPLD_PL_SPI_ADDR2_18
    CPLD_PL_SPI_LE_18
}]

# Data for internal PL SPI
set pl_src [get_ports {
    CPLD_PL_SPI_SDI_18
}]

# Passthrough inputs (forward direction)
# CPLD_PS_SPI_CLK_25 and CPLD_PL_SPI_SCLK_18 are special
set ps_pt_src [get_ports {CPLD_PS_SPI_LE_25
                          usrp_io[12]
                          usrp_io[13]
                          CPLD_PS_ADDR1_25
                          CPLD_PS_SPI_SDI_25
              }]

set pl_pt_src [get_ports {CPLD_PL_SPI_LE_18
                          CPLD_PL_SPI_ADDR1_18
                          CPLD_PL_SPI_ADDR2_18
                          CPLD_PL_SPI_SDI_18
              }]

# Passthrough outputs (forward direction)
# And inputs from the SPI slaves (readback direction)
set clkdist_spi_out [get_ports {
    CLKDIST_SPI_CS_L
    CLKDIST_SPI_SDIO
}]

set clkdist_spi_in [get_ports {
    CLKDIST_SPI_SDIO
}]

set phdac_spi [get_ports {
    PHDAC_SPI_CS_L
    PHDAC_SPI_SDI
}]

set dac_spi_out [get_ports {
    DAC_SPI_CS_L_18
    DAC_SPI_SDIO_18
}]

set dac_spi_in [get_ports {
    DAC_SPI_SDIO_18
}]

set adc_spi_out [get_ports {
    ADC_SPI_CS_L_18
    ADC_SPI_SDIO_18
}]

set adc_spi_in [get_ports {
    ADC_SPI_SDIO_18
}]

set lo_spi_out [get_ports {
    LO_TX_CS_L
    LO_RX_CS_L
    LO_SPI_SDI
}]

set lo_spi_in [get_ports {
    LOSYNTH_RX_MUXOUT
    LOSYNTH_TX_MUXOUT
}]

set lodist_spi_out [get_ports {
    LODIST_Bd_SPI_CS_L
    LODIST_Bd_SPI_SDI
}]

# Readback outputs
set ps_rb_out [get_ports CPLD_PS_SPI_SDO_25]
set pl_rb_out [get_ports CPLD_PL_SPI_SDO_18]

##############################################################################
# Chip-selects provide async resets
##############################################################################
set_false_path -from $ps_csb -to [get_pins *|clrn]
set_false_path -from $pl_csb -to [get_pins *|clrn]

# Also ignore setup/hold analysis for chip-selects affecting readback path
# These are available many cycles before readback begins and have
# combinatorial paths to the output
set_false_path -from $ps_csb -to $ps_rb_out
set_false_path -from $pl_csb -to $pl_rb_out

set_input_delay -clock sclk_ps -clock_fall -max $input_delay_max_ps \
    [get_ports CPLD_PS_ADDR0_25]
set_input_delay -clock sclk_ps -clock_fall -min $input_delay_min_ps \
    [get_ports CPLD_PS_ADDR0_25]

set_input_delay -clock sclk_pl -clock_fall -max $input_delay_max_pl \
    [get_ports CPLD_PL_SPI_ADDR0_18]
set_input_delay -clock sclk_pl -clock_fall -min $input_delay_min_pl \
    [get_ports CPLD_PL_SPI_ADDR0_18]

set_input_delay -clock sclk_pl_wr -clock_fall -max $input_delay_max_pl_wr \
    [get_ports CPLD_PL_SPI_ADDR0_18] -add
set_input_delay -clock sclk_pl_wr -clock_fall -min $input_delay_min_pl_wr \
    [get_ports CPLD_PL_SPI_ADDR0_18] -add


##############################################################################
# Input delays from SPI master
##############################################################################
set_input_delay -clock sclk_ps -clock_fall -max $input_delay_max_ps $ps_pt_src
set_input_delay -clock sclk_ps -clock_fall -min $input_delay_min_ps $ps_pt_src

set_input_delay -clock sclk_pl -clock_fall -max $input_delay_max_pl $pl_pt_src
set_input_delay -clock sclk_pl -clock_fall -min $input_delay_min_pl $pl_pt_src

set_input_delay -clock sclk_pl_wr -clock_fall -max $input_delay_max_pl_wr $pl_src -add
set_input_delay -clock sclk_pl_wr -clock_fall -min $input_delay_min_pl_wr $pl_src -add

##############################################################################
# Output delays to each SPI slave (uses setup/hold times from data sheet)
##############################################################################
set adc_setup 4
set adc_hold 2
set_output_delay -clock adc_clk -max [expr {$adc_setup + $board_delay}] \
    $adc_spi_out
set_output_delay -clock adc_clk -min [expr {-$adc_hold - $board_delay}] \
    $adc_spi_out

set dac_setup 10
set dac_hold 5
set_output_delay -clock dac_clk -max [expr {$dac_setup + $board_delay}] \
    $dac_spi_out
set_output_delay -clock dac_clk -min [expr {-$dac_hold - $board_delay}] \
    $dac_spi_out

set phdac_setup 5
set phdac_hold 5
set_output_delay -clock phdac_clk -max [expr {$phdac_setup + $board_delay}] \
    $phdac_spi
set_output_delay -clock phdac_clk -min [expr {-$phdac_hold - $board_delay}] \
    $phdac_spi

set clkdist_setup 10
set clkdist_hold 10
set_output_delay -clock clkdist_clk -max [expr {$clkdist_setup + $board_delay}] \
    $clkdist_spi_out
set_output_delay -clock clkdist_clk -min [expr {-$clkdist_hold - $board_delay}] \
    $clkdist_spi_out

set lo_setup 2
set lo_hold 2
set_output_delay -clock lo_wr_clk -max [expr {$lo_setup + $board_delay}] \
    $lo_spi_out
set_output_delay -clock lo_wr_clk -min [expr {-$lo_hold - $board_delay}] \
    $lo_spi_out

##############################################################################
# Input delays from each SPI slave (uses clk-to-q times from data sheet)
# One board delay for clock, another for data
##############################################################################
set lo_clk_q 2
set_input_delay -clock lo_clk -clock_fall -max [expr {$lo_clk_q + $board_delay + $board_delay}] \
    $lo_spi_in
set_input_delay -clock lo_clk -clock_fall -min 0 \
    $lo_spi_in

set adc_clk_q 10
set dac_clk_q 10
set clkdist_clk_q 10
set_input_delay -clock adc_clk -clock_fall -max [expr {$adc_clk_q + $board_delay + $board_delay}] \
    $adc_spi_in
set_input_delay -clock adc_clk -clock_fall -min 0 \
    $adc_spi_in
set_input_delay -clock dac_clk -clock_fall -max [expr {$dac_clk_q + $board_delay + $board_delay}] \
    $dac_spi_in
set_input_delay -clock dac_clk -clock_fall -min 0 \
    $dac_spi_in
set_input_delay -clock clkdist_clk -clock_fall  -max [expr {$clkdist_clk_q + $board_delay + $board_delay}] \
    $clkdist_spi_in
set_input_delay -clock clkdist_clk -clock_fall  -min 0 \
    $clkdist_spi_in


##############################################################################
# Output delays for readback path
##############################################################################
set_output_delay -clock sclk_ps -max $output_delay_max_ps $ps_rb_out
set_output_delay -clock sclk_ps -min $output_delay_min_ps $ps_rb_out

set_output_delay -clock sclk_pl -max $output_delay_max_pl $pl_rb_out
set_output_delay -clock sclk_pl -min $output_delay_min_pl $pl_rb_out

##############################################################################
# GPIOs and DSAs
# Outputs that aren't timing-critical
##############################################################################
set gpos [get_ports {Tx_Sw*
                     Rx_LO_*
                     Tx_LO_*
                     Rx_Sw*
                     Rx_Demod_*
                     Tx_HB_LB_Select
                     Rx_HB_LB_Select
                     Cal_iso_Sw_Ctrl
                     LODIST_Bd_IO1
         }]

# Inputs that aren't timing-critical
set gpis [get_ports {LO_SYNC
                     CPLD_ATR_TX_18
                     CPLD_ATR_RX_18
                     DAC_Alarm_18
                     CLKDIST_Status*
                     LODIST_Bd_IO1
         }]

# DSAs (special skew needs)
# RxLO_DSA_LE used for skew basis
set dsas [get_ports {Tx_DSA*
                     Rx_DSA*
                     TxLO_DSA*
                     LO_DSA*
         }]

# Just do false paths for gpios
set_false_path -to $gpos
set_false_path -from $gpis

# Unused
set_false_path -to $lodist_spi_out

# DSA skew timing
# Earlier, we created a "clock" for one of the DSA latch enable signals
# Use set_output_delay to constrain skew around the latch enable
#     set_multicycle_path is used to make latch clock = launch clock for setup
# Constrain skew to 8 ns -- controller nominally does 120 ns minimum between
# edges, and 100 ns is the DSA's requirement for setup/hold
set dsa_skew 8.0
set_output_delay -clock dsa_clk -max -$dsa_skew $dsas
set_output_delay -clock dsa_clk -min $dsa_skew $dsas
set_multicycle_path -start -setup 0 -to $dsas

set_max_delay -from [get_ports CPLD_ATR_TX_18] \
    -to [get_ports {Tx_Sw1_Ctrl_1 Tx_Sw1_Ctrl_2}] 10.0

