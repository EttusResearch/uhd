#
# Copyright 2018 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Description: Timing constraints for the USRP E31X
#


###############################################################################
# Input Clocks
###############################################################################

# 10MHz / PPS References
create_clock -period 100.000 -name pps_ext [get_nets PPS_EXT_IN]

create_clock -period 100.000 -name gps_pps [get_nets GPS_PPS]

# TCXO clock 40 MHz
create_clock -period 25.000 -name TCXO_CLK [get_nets TCXO_CLK]
set_input_jitter TCXO_CLK 0.100

###############################################################################
# Rename Clocks
###############################################################################

create_clock -period 10.000 \
             -name bus_clk [get_pins {e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/FCLKCLK[0]}]
set_input_jitter bus_clk 0.300

create_clock -period 25.000 \
             -name clk40 [get_pins {e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/FCLKCLK[1]}]
set_input_jitter clk40 0.750

#create_clock -period 5.000 \
#             -name bus_clk [get_pins {e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/FCLKCLK[3]}]
#set_input_jitter bus_clk 0.150

###############################################################################
# Timing Constraints for E310 daughter board signals
###############################################################################
# CAT_DATA_CLK is the data clock from AD9361, sample rate dependent with a max rate of 61.44 MHz
set cat_data_clk_period             16.276;
set cat_data_clk_duty_cycle_var     [expr $cat_data_clk_period * (0.55 - 0.45)];
set tcxo_jitter                     0.0005;     # Calculated from datasheet phase noise
create_clock -period $cat_data_clk_period -name CAT_DATA_CLK [get_ports CAT_DATA_CLK]
# Model variable duty cycle as jitter.
set_input_jitter CAT_DATA_CLK [expr $cat_data_clk_duty_cycle_var + $tcxo_jitter]

# Generate DAC output clock
create_generated_clock -name CAT_FB_CLK -multiply_by 1 -source [get_pins e310_io/oddr_clk/C] [get_ports CAT_FB_CLK]

# Asynchronous clock domains
set_clock_groups -asynchronous \
    -group [get_clocks -include_generated_clocks CAT_DATA_CLK] \
    -group [get_clocks -include_generated_clocks bus_clk] \
    -group [get_clocks -include_generated_clocks TCXO_CLK]

set_clock_groups -asynchronous \
  -group [get_clocks -include_generated_clocks *clk_200M_o] \
  -group [get_clocks -include_generated_clocks pps_ext] \
  -group [get_clocks -include_generated_clocks gps_pps]


#TODO: I don't think this was getting used on E310
# Logically exclusive clocks in catcodec capture interface. These two clocks are the input to a BUFG mux that
# drives radio_clk, meaning only one of the two can drive radio_clk at a time.
#set_clock_groups -logically_exclusive #  -group [get_clocks -include_generated_clocks {clk0}] #  -group [get_clocks -include_generated_clocks {clkdv}]

# Setup ADC (AD9361) interface constraints.
set cat_data_prog_dly               4.5;  # Programmable skew in AD9361 set to delay RX data by 4.5 ns
set cat_data_clk_to_data_out_min    0;
set cat_data_clk_to_data_out_max    1.2;

set_input_delay -clock [get_clocks CAT_DATA_CLK] -max [expr $cat_data_prog_dly + $cat_data_clk_to_data_out_max] [get_ports {CAT_P0_D* CAT_RX_FRAME}]
set_input_delay -clock [get_clocks CAT_DATA_CLK] -min [expr $cat_data_prog_dly + $cat_data_clk_to_data_out_min] [get_ports {CAT_P0_D* CAT_RX_FRAME}]
set_input_delay -clock [get_clocks CAT_DATA_CLK] -max [expr $cat_data_prog_dly + $cat_data_clk_to_data_out_max] [get_ports {CAT_P0_D* CAT_RX_FRAME}] -clock_fall -add_delay
set_input_delay -clock [get_clocks CAT_DATA_CLK] -min [expr $cat_data_prog_dly + $cat_data_clk_to_data_out_min] [get_ports {CAT_P0_D* CAT_RX_FRAME}] -clock_fall -add_delay

set cat_fb_data_prog_dly            4.5;  # Programmable skew in AD9361 set to delay TX data by 4.5 ns
set cat_fb_data_setup               1.0;
set cat_fb_data_hold                0;

set_output_delay -clock CAT_FB_CLK -max [expr $cat_fb_data_prog_dly + $cat_fb_data_setup] [get_ports {CAT_P1_D* CAT_TX_FRAME}]
set_output_delay -clock CAT_FB_CLK -min [expr $cat_fb_data_prog_dly - $cat_fb_data_hold]  [get_ports {CAT_P1_D* CAT_TX_FRAME}]
set_output_delay -clock CAT_FB_CLK -max [expr $cat_fb_data_prog_dly + $cat_fb_data_setup] [get_ports {CAT_P1_D* CAT_TX_FRAME}] -clock_fall -add_delay;
set_output_delay -clock CAT_FB_CLK -min [expr $cat_fb_data_prog_dly - $cat_fb_data_hold]  [get_ports {CAT_P1_D* CAT_TX_FRAME}] -clock_fall -add_delay;

# TODO: CAT SPI
# Xilinx doesn't allow you to fully constrain EMIO because the internal SPI
# clock is not accessible. So delay constraints are used to limit the delays to
# compatible values.

# Transceiver SPI
set_max_delay -from [get_pins e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/EMIOSPI0MO] \
              -to [get_ports CAT_MOSI] 10.000 -datapath_only
set_min_delay -to [get_ports CAT_MOSI] 1.000
#
set_max_delay -from [get_pins e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/EMIOSPI0SCLKO] \
              -to [get_ports CAT_SCLK] 10.000 -datapath_only
set_min_delay -to [get_ports CAT_SCLK] 1.000
#
set_max_delay -from [get_pins {e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/EMIOSPI0SSON[0]}] \
              -to [get_ports CAT_CS] 10.000 -datapath_only
set_min_delay -to [get_ports CAT_CS] 1.000
#
set_max_delay -from [get_ports CAT_MISO] \
              -to [get_pins e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/EMIOSPI0MI] 10.000 -datapath_only
set_min_delay -from [get_ports CAT_MISO] \
              -to [get_pins e31x_ps_bd_inst/processing_system7_0/inst/PS7_i/EMIOSPI0MI] 1.000

###############################################################################
# PPS and Ref Clk Input Timing
###############################################################################

# Asynchronous clock domains
set_clock_groups -asynchronous \
    -group [get_clocks -include_generated_clocks bus_clk] \
    -group [get_clocks -include_generated_clocks pps_ext] \
    -group [get_clocks -include_generated_clocks gps_pps]

# TCXO DAC SPI
# 12 MHz SPI clock rate
set_max_delay -datapath_only -from [all_ffs] -to [get_ports TCXO_DAC*] 40.000
set_min_delay -from [all_ffs] -to [get_ports TCXO_DAC*] 1.000

# User GPIO
set_max_delay -datapath_only -to   [get_ports PL_GPIO*] -from [all_ffs] [expr 15.0]
set_min_delay                -to   [get_ports PL_GPIO*] -from [all_ffs] 5.0
set_max_delay -datapath_only -from [get_ports PL_GPIO*] -to   [all_ffs] [expr 15.0]
set_min_delay                -from [get_ports PL_GPIO*] -to   [all_ffs] 5.0

# GPIO muxing
set_max_delay -from [get_pins e31x_core_inst/fp_gpio_src_reg_reg[*]/C] -to [get_clocks CAT_DATA_CLK] $cat_data_clk_period -datapath_only

###############################################################################
# False Paths
###############################################################################

# Synchronizer core false paths
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/S}]

# USR_ACCESS build date
set_false_path -through [get_pins {usr_access_i/DATA[*]}]

###############################################################################
## Asynchronous paths
###############################################################################
set_false_path -from [get_ports CAT_CTRL_OUT]
set_false_path -to [get_ports CAT_RESET]
set_false_path -to [get_ports RX*_BANDSEL*]
set_false_path -to [get_ports TX_BANDSEL*]
set_false_path -to [get_ports TX_ENABLE*]
set_false_path -to [get_ports LED_*]
set_false_path -to [get_ports VCRX*]
set_false_path -to [get_ports VCTX*]
set_false_path -from [get_ports ONSWITCH_DB]
