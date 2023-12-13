#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Common timing constraints for X410.
#

###############################################################################
# Motherboard Clocks
###############################################################################

# 10/25 MHz reference clock from rear panel connector.
# Constrain to the fastest possible clock rate.
set ref_clk_period 40.00
create_clock -name ref_clk     -period $ref_clk_period     [get_ports BASE_REFCLK_FPGA_P]

# MGT Clocks
#    Clock Reference   |   Frequency      |   Purpose
#   MGT_REFCLK_LMK0    |  156.25/125 MHz  |  10 GbE
#   MGT_REFCLK_LMK1    |  100.00 MHz      |  Reserved
#   MGT_REFCLK_LMK2    |  100.00 MHz      |  Reserved
#   MGT_REFCLK_LMK3    |  156.25/125 MHz  |  10 GbE
create_clock -name mgt_ref_0 -period  6.400 [get_ports MGT_REFCLK_LMK0_P]
create_clock -name mgt_ref_1 -period 10.000 [get_ports MGT_REFCLK_LMK1_P]
create_clock -name mgt_ref_2 -period 10.400 [get_ports MGT_REFCLK_LMK2_P]
create_clock -name mgt_ref_3 -period  6.400 [get_ports MGT_REFCLK_LMK3_P]

# Virtual clocks for constraining misc. I/Os.
create_clock -name async_in_clk  -period 50.00
create_clock -name async_out_clk -period 50.00


###############################################################################
# Aliases for auto-generated clocks
###############################################################################

# Name the PS clocks. These are originally declared in the PS8 IP block.
# Create the clocks based on the PS PLCLK pins.
# This generates critical warnings in the OSS flow because the clocks were already
# define and we are completely rewriting the old clock definition... this is OK.
create_clock -name clk100 -period 10.000 \
  [get_pins -of_objects [get_cells -hierarchical {*PS8_i}] -filter {NAME =~ *PLCLK[0]}]

create_clock -name clk40  -period 25.000 \
  [get_pins -of_objects [get_cells -hierarchical {*PS8_i}] -filter {NAME =~ *PLCLK[1]}]

create_clock -name clk166 -period  6.000 \
  [get_pins -of_objects [get_cells -hierarchical {*PS8_i}] -filter {NAME =~ *PLCLK[2]}]

create_clock -name clk200 -period  5.000 \
  [get_pins -of_objects [get_cells -hierarchical {*PS8_i}] -filter {NAME =~ *PLCLK[3]}]

create_generated_clock -name ce_clk [get_pins {ce_clk_gen_i/CLKOUT0}]

###############################################################################
# Sync to DB synthesizer sync CPLD input
###############################################################################

# synth_sync_hold_requirement and synth_sync_setup_requirement are shared
# between the FPGA and DB CPLD. The values are set in shared_constants.sdc
set synth_sync_ports [get_ports {DB0_SYNTH_SYNC DB1_SYNTH_SYNC}]
set_output_delay -clock [get_clocks pll_ref_clk] -min -$synth_sync_hold_requirement $synth_sync_ports
set_output_delay -clock [get_clocks pll_ref_clk] -max $synth_sync_setup_requirement $synth_sync_ports

###############################################################################
# SPI to MB CPLD (PL)
#   This interface is defined as system synchronous to pll_ref_clk.
###############################################################################

# Both CPLD and FPGA use PLL reference clock from a common clock chip.
# The traces from that clock chip to the ICs are not length matched. Assume a
# worst case clock difference of 0.5 ns at the IC inputs. There is no direction
# defined. The clock can arrive faster or slower at one IC.
set pl_clock_diff 0.500

# The longest trace on the PL SPI interface is (assuming 170.0 ps/in)
#   Longest trace | Trace length | Trace delay
#   PL_CPLD_MISO  |   3.863 in   |   0.657 ns
set pl_spi_board_delay 0.657

# Output delay timings of the MB CPLD design, which still meet timing
set pl_spi_cpld_min_out -1.000
set pl_spi_cpld_max_out  8.000

set spi_in_port [get_ports {PL_CPLD_MISO}]
set_input_delay -clock [get_clocks pll_ref_clk] \
  -min [expr {- $pl_spi_cpld_min_out - $pl_clock_diff}] \
  $spi_in_port
set_input_delay -clock [get_clocks pll_ref_clk] \
  -max [expr {$pll_ref_clk_period - $pl_spi_cpld_max_out + $pl_spi_board_delay + $pl_clock_diff}] \
  $spi_in_port


###############################################################################
# 10 GbE
###############################################################################

# These are the exceptions from "xge_pcs_pma_exceptions.xdc" which are to be
# used when not using the example design.
#
# "clk100" used here is the clock that's connected to the "dclk" input in the core.
set_max_delay -from [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/RXOUTCLK}]] -to [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/TXOUTCLK}]] -datapath_only 6.40
set_max_delay -from [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/TXOUTCLK}]] -to [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/RXOUTCLK}]] -datapath_only 6.40
set_max_delay -from [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/RXOUTCLK}]] -to [get_clocks clk100] -datapath_only 6.40
set_max_delay -from [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/TXOUTCLK}]] -to [get_clocks clk100] -datapath_only 6.40
set_max_delay -from [get_clocks clk100] -to [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/TXOUTCLK}]] -datapath_only 10.000
set_max_delay -from [get_clocks clk100] -to [get_clocks -of_objects [get_pins -hierarchical -filter {NAME =~ */channel_inst/*_CHANNEL_PRIM_INST/RXOUTCLK}]] -datapath_only 10.000


###############################################################################
# DIO
#   Those GPIO pins are considered asynchronous paths. The user has to add
#   constraints in case required. Therefore not setting false_paths from / to
#   user logic to allow user generated timing constraints to be applied.
###############################################################################

# Ignore paths from "slow" PS interface to not interfere with user constraints.
set dio_ports     [get_ports {DIOA_FPGA[*] DIOB_FPGA[*]}]
set dio_registers [get_cells -hierarchical -filter {NAME =~ *x4xx_dio_i* && IS_SEQUENTIAL && IS_PRIMITIVE}]
set_false_path -from $dio_registers -to $dio_ports
set_false_path -from $dio_ports     -to $dio_registers


###############################################################################
# PPS
###############################################################################

# The TRIG_IO port may be driven by either the PPS in BRC domain to
# enable direct sync between 2 devices, or by any other user logic.
# When PPS is exported through Trigger I/O, timing must be analyzed
# to ensure determinism in the PPS exporting.
# But, when other user logic drives TRIG_IO, then the port should be
# treated as asynchronous (or close to async. at least).
# To achieve this conditional timing analysis, the following trick is
# used:
#   1. A virtual copy of ref_clk is created for I/O timing - virtual_ref_clk
#   2. Set output_delay constraints to assign a clock to the TRIG_IO port.
#   3. A set_max_delay constraint is used to time the output path to TRIG_IO
#        set_max_delay makes the timing constraint driver agnostic, and as long
#        as the critical output delay is met for driving PPS through TRIG_IO,
#        we should be fine as this requirement is relatively loose.

# 1) Creating copy of ref_clk to only analyze timing to TRIG_IO port (output)
# when output is driven by ref_clk (PPS generation in ref_clk domain).
create_clock -name virtual_ref_clk -period $ref_clk_period

# Trigger IO port is used as output for the PPS signal
# TRIG_IO_1V8 trace length MB = 4.050 + 1.190 inch = 5.240 inch
# TRIG_IO_1V8 trace length DB = 2.401 + 0.120 + 0.457 + 0.261 inch = 3.239 inch
# TRIG_IO buffer max switching time = 3.3
set trig_max_out_delay [expr {8.479 * 0.17 + 3.3}]

# Set minimum output delay hold time to a small amount to grant external
# devices some hold time. Delay should be simple to achieve as there is no PLL
# in the clocking path and some combinatorial logic.
set trig_min_out_delay 2.000

# 2) set_output_delay for assigning clocks to TRIG_IO. Use zero for delay to
# avoid adding extra delay requirements on top of the set_max|min_delay
# constraints below.
set_output_delay -clock [get_clocks virtual_ref_clk] 0.0 [get_ports {TRIG_IO}]

# 3) Min and max delays make constraining driver agnostic. We just make sure
# the critical timing for PPS export is met though.
set_max_delay -through [get_port {TRIG_IO}] -to [get_clocks {virtual_ref_clk}] \
  [expr {$ref_clk_period - $trig_max_out_delay}]
set_min_delay -through [get_port {TRIG_IO}] -to [get_clocks {virtual_ref_clk}] \
  $trig_min_out_delay

# Treat TRIG_IO input as asynchronous.
set_false_path -from [get_ports {TRIG_IO}]
# For documentation purposes, these are the input max/min delays for TRIG_IO:
#   - Input delay assuming zero trace delay and TRIG_IO buffer min switching
#     time (B->A) = 0.1 ns.
#   - TRIG_IO buffer max switching time (B->A = input) = 3.7 ns + same trace
#     length as for output (8.479).

# Assuming no delay on external clock distribution.

# Account for the PPS min output delay only (for the case two X410 are directly
# connected to each other).
set pps_min_in_delay $trig_min_out_delay

# PPS_IN trace length DB = 0.535 + 0.133 + 0.117 + 0.061 + 2.745 inch = 3.591 inch
# PPS_IN trace length MB = 5.726 inch
# PPS switch max propagation delay = 3.6

# Assume 50% of the clock period is used for external PPS clock distribution as
# the PPS out is used to synchronize one X410 (master) with another X410
# (slave) the PPS out (trig_io) delay is added to the PPS input.
set pps_max_in_delay [expr {9.317 * 0.17 + 3.6 + 0.5 * $ref_clk_period + $trig_max_out_delay}]

# Apply PPS input constraints.
set_input_delay -clock [get_clocks ref_clk] -min $pps_min_in_delay [get_ports {PPS_IN}]
set_input_delay -clock [get_clocks ref_clk] -max $pps_max_in_delay [get_ports {PPS_IN}]

# PPS clock domain crossing BRC -> PRC on the aligned edge.
# Use a data path of half PLL reference clock period to make sure the value is
# captured without metastability.
set_max_delay -from [get_cells -hierarchical pps_delayed_brc_reg] \
  -to [get_clocks pll_ref_clk*] [expr {$pll_ref_clk_period/2}]


###############################################################################
# LMK sync
###############################################################################

# The timings are derived from simulation.

# Clock Buffer ADCLK944 -> FPGA.
set buffer_to_fpga_min_clk_delay 0.997
set buffer_to_fpga_max_clk_delay 1.154

# Clock Buffer ADCLK944 -> Sample clock PLL (LMK04832).
set buffer_to_spll_min_clk_delay 0.000
set buffer_to_spll_max_clk_delay 0.014

# FPGA -> Sample clock PLL SYNC input.
set fpga_to_spll_min_clk_delay   0.381
set fpga_to_spll_max_clk_delay   0.460

# Sample clock PLL requirements.
set lmk_sync_input_hold          4.000
set lmk_sync_input_setup         4.000

set lmk_sync_output_max_delay [expr {$fpga_to_spll_max_clk_delay + $buffer_to_fpga_max_clk_delay + \
                                $lmk_sync_input_setup - $buffer_to_spll_min_clk_delay}]
set lmk_sync_output_min_delay [expr {$fpga_to_spll_min_clk_delay + $buffer_to_fpga_min_clk_delay - \
                                $buffer_to_spll_max_clk_delay - $lmk_sync_input_hold}]
set_output_delay -clock ref_clk -max $lmk_sync_output_max_delay [get_ports {LMK_SYNC}]
set_output_delay -clock ref_clk -min $lmk_sync_output_min_delay [get_ports {LMK_SYNC}]


###############################################################################
# x4xx_ps_rfdc_bd
###############################################################################

# The calibration_muxes component contains a clock crossing from some GPIO
# component instances that are synchronous to a configuration clock and ending
# in some AXI registers synchronous to data clock. The GPIO registers are
# essentially constant. When they are changing (due to a register write), the
# latching registers can definitely become metastable, so the software must
# ensure that the corrupted data appears at a safe time.
set gpio_regs [get_pins -of [get_cells -filter {IS_SEQUENTIAL && NAME =~ *rfdc/calibration_muxes/axi_gpio*} -hier] -filter {IS_CLOCK}]
set mux_regs [get_cells -hier -filter {IS_SEQUENTIAL && NAME =~ *rfdc/calibration_muxes/gpio_to_axis_mux*}]
set_false_path -from $gpio_regs -to $mux_regs


###############################################################################
# Misc Constraints
###############################################################################

# Double synchronizer false paths.
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */rf_reset_controller*/*_ms_reg/D}]
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */rfdc/rf_nco_reset_0/*_ms*/D}]

# GTY_RCV_CLK_* is driven by a OBUFDS_GTE4 buffer, which has an asynchronous
# clock-enable pin.
# By experimentation, it was observed that explicitly setting a false_path to
# this pin improved timing.
set gty_rcv_clk_buff_ceb [get_pins -of_objects [get_cells -of_objects [all_fanin -flat -startpoints_only [get_ports {GTY_RCV_CLK_P}]]] -filter {NAME=~ "*CEB"}]
set_false_path -from [get_clocks {clk40}] -through $gty_rcv_clk_buff_ceb


###############################################################################
# Asynchronous / misc. I/O constraints
#   Loosely constrain these to prevent warnings in Vivado.
#   Using set_input_delay associates the I/Os to a clock group, but
#   set_(min|max)_delay overwrites the setup/hold analysis values.
###############################################################################

set async_inputs [get_ports {FPGA_AUX_REF}]

set_input_delay -clock [get_clocks async_in_clk] 0.000 $async_inputs
set_max_delay -from $async_inputs 50.000
set_min_delay -from $async_inputs 0.000


set async_outputs [get_ports {FABRIC_CLK_OUT_P PPS_LED}]

set_output_delay -clock [get_clocks async_out_clk] 0.000 $async_outputs
set_max_delay -to $async_outputs 50.000
set_min_delay -to $async_outputs 0.000
