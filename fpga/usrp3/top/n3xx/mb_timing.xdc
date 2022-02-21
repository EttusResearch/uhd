#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Timing analysis is performed in "/n3xx/doc/mb_timing.xlsx". See
# the spreadsheet for more details and explanations.

#*******************************************************************************
## Asynchronous clock groups

# All the clocks from the PS are async to everything else except clocks generated
# from themselves.
set_clock_groups -asynchronous -group [get_clocks clk100       -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks clk40        -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks bus_clk      -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks meas_clk_ref -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks async_in_clk]
set_clock_groups -asynchronous -group [get_clocks async_out_clk]



#*******************************************************************************
## PPS Input Timing

# The external PPS is synchronous to the external reference clock, which is expected to
# be at 10 MHz. Given [setup, hold] of [5ns, 5ns] at the rear panel inputs of the N310,
# we have an adequate data valid window at the FPGA. However, since we overconstrain the
# reference clock to 25 MHz, we use the alternative period here for setup analysis.
set_input_delay -clock ref_clk -min  4.651                           [get_ports REF_1PPS_IN]
set_input_delay -clock ref_clk -max [expr {$REF_CLK_PERIOD - 0.235}] [get_ports REF_1PPS_IN]

# The GPS PPS is also synchronous to the external reference clock (since there is a
# switch on the clock input outside the FPGA). Again, use the overconstrained period.
set_input_delay -clock ref_clk -min  1.234                           [get_ports GPS_1PPS]
set_input_delay -clock ref_clk -max [expr {$REF_CLK_PERIOD - 2.111}] [get_ports GPS_1PPS]



#*******************************************************************************
## White Rabbit DAC
# Constrain the DIN and NSYNC bits around the clock output. No readback.

set MAX_SKEW 5
set SETUP_SKEW [expr {($MAX_SKEW / 2)-0.5}]
set HOLD_SKEW  [expr {($MAX_SKEW / 2)+0.5}]
set PORT_LIST [get_ports {WB_DAC_DIN WB_DAC_NCLR WB_DAC_NSYNC WB_DAC_NLDAC}]
# Then add the output delay on each of the ports.
set_output_delay                        -clock [get_clocks wr_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks wr_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay                        -clock [get_clocks wr_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks wr_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
# Finally, make both the setup and hold checks use the same launching and latching edges.
set_multicycle_path -setup -to [get_clocks wr_bus_clk] -start 0
set_multicycle_path -hold  -to [get_clocks wr_bus_clk] -1
# Remove analysis from the output "clock" pin. There are ways to do this using TCL, but
# they aren't supported in XDC files... so we do it the old fashioned way.
set_output_delay -clock [get_clocks async_out_clk] 0.000 $WR_OUT_CLK
set_max_delay -to $WR_OUT_CLK 50.000
set_min_delay -to $WR_OUT_CLK 0.000



#*******************************************************************************
## MB Async Ins/Outs

set ASYNC_MB_INPUTS [get_ports {SFP_*_LOS SFP_*_TXFAULT UNUSED_PIN_TDC*}]

set_input_delay -clock [get_clocks async_in_clk] 0.000 $ASYNC_MB_INPUTS
set_max_delay -from $ASYNC_MB_INPUTS 50.000
set_min_delay -from $ASYNC_MB_INPUTS 0.000


set ASYNC_MB_OUTPUTS [get_ports {*LED* SFP_*TXDISABLE UNUSED_PIN_TDC* \
                               FPGA_TEST[*]}]

set_output_delay -clock [get_clocks async_out_clk] 0.000 $ASYNC_MB_OUTPUTS
set_max_delay -to $ASYNC_MB_OUTPUTS 50.000
set_min_delay -to $ASYNC_MB_OUTPUTS 0.000



#*******************************************************************************
## Front Panel GPIO
# These bits are driven from the DB-A radio clock. Although they are received async in
# the outside world, they should be constrained in the FPGA to avoid any race
# conditions. The best way to do this is a skew constraint across all the bits.

set MAX_SKEW 10
set SETUP_SKEW [expr {($MAX_SKEW / 2)-0.5}]
set HOLD_SKEW  [expr {($MAX_SKEW / 2)+0.5}]
set PORT_LIST [get_ports {FPGA_GPIO[*]}]
# Then add the output delay on each of the ports.
set_output_delay                        -clock [get_clocks fp_gpio_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks fp_gpio_bus_clk] -max -$SETUP_SKEW $PORT_LIST
set_output_delay                        -clock [get_clocks fp_gpio_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
set_output_delay -add_delay -clock_fall -clock [get_clocks fp_gpio_bus_clk] -min  $HOLD_SKEW  $PORT_LIST
# Finally, make both the setup and hold checks use the same launching and latching edges.
set_multicycle_path -setup -to [get_clocks fp_gpio_bus_clk] -start 0
set_multicycle_path -hold  -to [get_clocks fp_gpio_bus_clk] -1
# Remove analysis from the output "clock" pin. There are ways to do this using TCL, but
# they aren't supported in XDC files... so we do it the old fashioned way.
set_output_delay -clock [get_clocks async_out_clk] 0.000 $FP_GPIO_CLK
set_max_delay -to $FP_GPIO_CLK 50.000
set_min_delay -to $FP_GPIO_CLK 0.000
# All inputs on this interface are async.
set_input_delay -clock [get_clocks async_in_clk] 0.000 $PORT_LIST
set_max_delay -from $PORT_LIST 50.000
set_min_delay -from $PORT_LIST 0.000

#******************************************************************************
## Synchronizer false paths
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
