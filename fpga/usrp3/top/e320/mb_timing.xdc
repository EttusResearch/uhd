#
# Copyright 2018 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Description: Timing constraints for the USRP E320
#


###############################################################################
# Input Clocks
###############################################################################

# External Reference Clock
set REF_CLK_PERIOD 50.00
create_clock -name ref_clk -period $REF_CLK_PERIOD  [get_ports CLK_REF_RAW]

# Radio clock from AD9361
set rx_clk_period 8.138
create_clock -name rx_clk -period $rx_clk_period [get_ports RX_CLK_P]

# 1 Gigabit Ethernet Reference Clock
create_clock -name ge_clk  -period 8.000 [get_ports CLK_MGT_125M_P]

# 10 Gigabit and Aurora Reference Clock
create_clock -name xge_clk -period 6.400 [get_ports CLK_MGT_156_25M_P]

# Derived radio clocks (two mutually-exclusive clocks using a BUFGMUX)
create_generated_clock -name radio_clk_1x \
                       -divide_by 4 \
                       -add \
                       -master_clock rx_clk \
                       -source [get_pins cat_io_lvds_dual_mode_i0/cat_io_lvds_i0/cat_input_lvds_i0/sdr_clk_2x_bufr/O] \
                       [get_pins cat_io_lvds_dual_mode_i0/BUFGCTRL_radio_clk/O]
create_generated_clock -name radio_clk_2x \
                       -divide_by 2 \
                       -add \
                       -master_clock rx_clk \
                       -source [get_pins cat_io_lvds_dual_mode_i0/cat_io_lvds_i0/cat_input_lvds_i0/sdr_clk_bufr/O] \
                       [get_pins cat_io_lvds_dual_mode_i0/BUFGCTRL_radio_clk/O]
set_clock_groups -physically_exclusive -group radio_clk_1x -group radio_clk_2x



###############################################################################
# Rename Clocks
###############################################################################

create_clock -name clk100 \
             -period   [get_property PERIOD      [get_clocks clk_fpga_0]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_0]]]
set_input_jitter clk100 0.3

create_clock -name clk40 \
             -period   [get_property PERIOD      [get_clocks clk_fpga_1]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_1]]]
set_input_jitter clk40 0.75

create_clock -name meas_clk_ref \
             -period   [get_property PERIOD      [get_clocks clk_fpga_2]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_2]]]
set_input_jitter meas_clk_ref 0.18

create_clock -name bus_clk \
             -period   [get_property PERIOD      [get_clocks clk_fpga_3]] \
             [get_pins [get_property SOURCE_PINS [get_clocks clk_fpga_3]]]
set_input_jitter bus_clk 0.15

# DMA Clock
create_generated_clock -name ddr3_dma_clk \
  [get_pins {e320_clocking_i/mmcm_adv_inst/CLKOUT0}]



###############################################################################
# Clock Crossings
###############################################################################

set DDR3_UI_CLK_PERIOD  [get_property period [get_clocks ddr3_ui_clk]]
set RADIO_CLK_1X_PERIOD [get_property period [get_clocks radio_clk_1x]]
set RADIO_CLK_2X_PERIOD [get_property period [get_clocks radio_clk_2x]]
set XGE_CLK_PERIOD      [get_property period [get_clocks xge_clk]]

# XADC temperature
set_max_delay -from [get_pins tempmon_i/device_temp_r_reg[*]/C] -to [get_clocks ddr3_ui_clk] $DDR3_UI_CLK_PERIOD -datapath_only

# GPIO muxing
set_max_delay -from [get_pins e320_core_i/fp_gpio_src_reg_reg[*]/C] -to [get_clocks radio_clk_1x] $RADIO_CLK_1X_PERIOD -datapath_only
set_max_delay -from [get_pins e320_core_i/fp_gpio_src_reg_reg[*]/C] -to [get_clocks radio_clk_2x] $RADIO_CLK_2X_PERIOD -datapath_only

# Codec reset
set_max_delay -from [get_pins e320_core_i/dboard_ctrl_reg[2]/C] -to [get_clocks radio_clk_1x_1] $RADIO_CLK_1X_PERIOD -datapath_only

# Power-on reset
set_max_delay -from [get_pins por_gen/por_rst_reg/C] -to [get_clocks xge_clk] $XGE_CLK_PERIOD -datapath_only

# SFP MDIO data and clock signal crossings. These are double synchronized in
# the Xilinx MDIO IP.
set_max_delay -from [get_pins sfp_wrapper_i/mgt_io_i/mdio_master_i/mdio_out_reg/C] \
              -to [get_clocks xge_clk] $XGE_CLK_PERIOD -datapath_only
set_max_delay -from [get_pins sfp_wrapper_i/mgt_io_i/mdio_master_i/mdc_reg/C] \
              -to [get_clocks xge_clk] $XGE_CLK_PERIOD -datapath_only



###############################################################################
# False Paths
###############################################################################

# Synchronizer core false paths
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/S}]


# MIG core reset
# According to Xilinx AR 61112, it is safe to make sys_rst a false path.
set_false_path -from [get_pins bus_reset_gen/reset_out_reg/C] \
               -to   [get_clocks ddr3_ui_clk]
set_false_path -from [get_pins bus_reset_gen/reset_out_reg/C] \
               -to   [get_clocks ddr3_ui_clk_2x]

# USR_ACCESS build date
set_false_path -through [get_pins usr_access_i/DATA[*]]



###############################################################################
# PPS Input Timing
###############################################################################

# The external PPS is synchronous to the external reference clock. We want to
# allow for 5 ns of setup and 5 ns of hold at the external connectors of the
# device.
set t_ext_setup 5.0
set t_ext_hold  5.0

# Board delays for external REF/PPS
set t_ext_pps_to_fpga(min) 1.673 ; # Delay from external pin of PPS to FPGA
set t_ext_pps_to_fpga(max) 5.011
set t_ext_ref_to_fpga(min) 1.452 ; # Delay from external pin of reference clock to FPGA
set t_ext_ref_to_fpga(max) 4.000

# Calculate the needed setup and hold at FPGA for external PPS, taking into
# account worst-case clock and data path skew.
set t_ext_fpga_setup [expr $t_ext_setup + ($t_ext_ref_to_fpga(min) - $t_ext_pps_to_fpga(max))]
set t_ext_fpga_hold  [expr $t_ext_hold  + ($t_ext_pps_to_fpga(min) - $t_ext_ref_to_fpga(max))]

set_input_delay -clock ref_clk -max [expr $REF_CLK_PERIOD - $t_ext_fpga_setup] [get_ports CLK_SYNC_EXT]
set_input_delay -clock ref_clk -min $t_ext_fpga_hold                           [get_ports CLK_SYNC_EXT]


# The GPS provides 2 ns setup and 2 ns of hold around the rising clock edge
set t_int_setup 2.0
set t_int_hold  2.0

# Board delays for internal REF/PPS
set t_int_pps_to_fpga(min) 0.359 ; # Delay from PPS output of GPS to FPGA
set t_int_pps_to_fpga(max) 0.438
set t_int_ref_to_fpga(min) 1.699 ; # Delay from reference clock output of GPS to FPGA
set t_int_ref_to_fpga(max) 3.149

# Calculate the needed setup and hold at FPGA for internal PPS, taking into
# account worst-case clock and data path skew.
set t_int_fpga_setup [expr $t_int_setup + ($t_int_ref_to_fpga(min) - $t_int_pps_to_fpga(max))]
set t_int_fpga_hold  [expr $t_int_hold  + ($t_int_pps_to_fpga(min) - $t_int_ref_to_fpga(max))]

set_input_delay -clock ref_clk -max [expr $REF_CLK_PERIOD - $t_int_fpga_setup] [get_ports CLK_SYNC_INT]
set_input_delay -clock ref_clk -min $t_int_fpga_hold                           [get_ports CLK_SYNC_INT]



###############################################################################
# LVDS Interface
###############################################################################

# LVDS interface is source synchronous DDR. tPCB numbers are taken from
# HyperLynx for the Rev B PCB. 10 ps was added to each PCB delay for additional
# margin.

# From the AD9361 data sheet
set tDDRX(min) 0.25
set tDDRX(max) 1.25
set tSTX(min)  1.0
set tHTX(min)  0.0

# Other timing parameters
set tCP2X(min) [expr 0.45 * $rx_clk_period]  ; # Worst-case bit period
set tTrns(max) 0.220   ; # Amount of time it takes an input to transition

# Input timing parameters
set tPCB_RX(max)  0.058   ; # Max delay by which the clock trace is longer than the data trace
set tPCB_RX(min) -0.059   ; # Min delay by which the clock trace is longer than the data trace
set tSetupIn  [expr $tCP2X(min) - $tDDRX(max) + $tPCB_RX(min)]
set tHoldIn   [expr $tDDRX(min) - $tTrns(max) - $tPCB_RX(max)]

# Input Setup/Hold (Rising Clock Edge)
set_input_delay -clock [get_clocks rx_clk] -max [expr $tCP2X(min) - $tSetupIn] [get_ports {RX_DATA_*[*] RX_FRAME_*}]
set_input_delay -clock [get_clocks rx_clk] -min $tHoldIn [get_ports {RX_DATA_*[*] RX_FRAME_*}]

# Input Setup/Hold (Falling Clock Edge)
set_input_delay -clock [get_clocks rx_clk] -max [expr $tCP2X(min) - $tSetupIn] [get_ports {RX_DATA_*[*] RX_FRAME_*}] -clock_fall -add_delay
set_input_delay -clock [get_clocks rx_clk] -min $tHoldIn [get_ports {RX_DATA_*[*] RX_FRAME_*}] -clock_fall -add_delay


# Output timing parameters
set tPCB_TX(max)  0.066   ; # Max delay by which the clock trace is longer than the data trace
set tPCB_TX(min) -0.049   ; # Min delay by which the clock trace is longer than the data trace
set tSetupOut  [expr $tSTX(min) - $tPCB_TX(min)]
set tHoldOut   [expr $tHTX(min) + $tPCB_TX(max)]

# Create tx_clk (FB_CLK)
create_generated_clock \
  -name tx_clk \
  -multiply_by 1 \
  -source [get_pins cat_io_lvds_dual_mode_i0/cat_io_lvds_i0/cat_output_lvds_i0/ddr_clk_oserdese2/CLK] \
  [get_ports TX_CLK_P]

# Output Setup
set_output_delay -clock [get_clocks tx_clk] -max $tSetupOut [get_ports {TX_DATA_*[*] TX_FRAME_*}]
set_output_delay -clock [get_clocks tx_clk] -max $tSetupOut [get_ports {TX_DATA_*[*] TX_FRAME_*}] -clock_fall -add_delay

# Output Hold
set_output_delay -clock [get_clocks tx_clk] -min [expr -$tHoldOut] [get_ports {TX_DATA_*[*] TX_FRAME_*}]
set_output_delay -clock [get_clocks tx_clk] -min [expr -$tHoldOut] [get_ports {TX_DATA_*[*] TX_FRAME_*}] -clock_fall -add_delay



###############################################################################
# SPI
###############################################################################

# Xilinx doesn't allow you to fully constrain EMIO because the internal SPI
# clock is not accessible. So delay constraints are used to limit the delays to
# compatible values.

# Transceiver SPI
set_max_delay -from [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI0MO] \
              -to [get_ports XCVR_SPI_MOSI] 6.0 -datapath_only
set_min_delay -to [get_ports XCVR_SPI_MOSI] 0.0
#
set_max_delay -from [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI0SCLKO] \
              -to [get_ports XCVR_SPI_CLK] 6.0 -datapath_only
set_min_delay -to [get_ports XCVR_SPI_CLK] 0.0
#
set_max_delay -from [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI0SSON[0]] \
              -to [get_ports XCVR_SPI_CS_N] 6.0 -datapath_only
set_min_delay -to [get_ports XCVR_SPI_CS_N] 0.0
#
set_max_delay -from [get_ports XCVR_SPI_MISO] \
              -to [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI0MI] 4.0 -datapath_only
set_min_delay -from [get_ports XCVR_SPI_MISO] -to [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI0MI] 0.0

# Clock synthesizer SPI
set_max_delay -from [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI1MO] \
              -to [get_ports CLK_PLL_SDATA] 9.0 -datapath_only
set_min_delay -to [get_ports XCVR_SPI_MOSI] 0.0
#
set_max_delay -from [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI1SCLKO] \
              -to [get_ports CLK_PLL_SCLK] 9.0 -datapath_only
set_min_delay -to [get_ports XCVR_SPI_CLK] 0.0
#
set_max_delay -from [get_pins e320_ps_bd_i/processing_system7_0/inst/PS7_i/EMIOSPI1SSON[0]] \
              -to [get_ports CLK_PLL_SLE] 9.0 -datapath_only
set_min_delay -to [get_ports XCVR_SPI_CS_N] 0.0



###############################################################################
# Miscellaneous I/O Constraints
###############################################################################

# Transceiver
set_max_delay -to [get_ports XCVR_RESET_N] 50.0
set_min_delay -to [get_ports XCVR_RESET_N] 0.0
#
set_max_delay -from [get_ports XCVR_CTRL_OUT[*]] 5.0 -datapath_only
set_min_delay -from [get_ports XCVR_CTRL_OUT[*]] 0.0

# GPIO
set_max_delay -from [get_ports GPIO_PREBUFF[*]] 5.0 -datapath_only
set_min_delay -from [get_ports GPIO_PREBUFF[*]] 0.0
#
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports GPIO_DIR[*]]] \
              -to   [get_ports GPIO_DIR[*]] 8.0 -datapath_only
set_min_delay -to   [get_ports GPIO_DIR[*]] 0.0
#
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports GPIO_PREBUFF[*]]] \
              -to   [get_ports GPIO_PREBUFF[*]] 8.0 -datapath_only
set_min_delay -to   [get_ports GPIO_PREBUFF[*]] 0.0
#
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports GPIO_OE_N]] \
              -to   [get_ports GPIO_OE_N] 8.0 -datapath_only
set_min_delay -to   [get_ports GPIO_OE_N] 0.0
#
set_max_delay -to   [get_ports {EN_GPIO_2V5 EN_GPIO_3V3 EN_GPIO_VAR_SUPPLY}] 50.0
set_min_delay -to   [get_ports {EN_GPIO_2V5 EN_GPIO_3V3 EN_GPIO_VAR_SUPPLY}] 0.0

# GPS
set_max_delay -from [get_ports {GPS_ALARM GPS_LOCK GPS_PHASELOCK GPS_SURVEY GPS_WARMUP}] 10.0 -datapath_only
set_min_delay -from [get_ports {GPS_ALARM GPS_LOCK GPS_PHASELOCK GPS_SURVEY GPS_WARMUP}] 0.0
#
set_max_delay -to [get_ports GPS_INITSURV_N] 50.0
set_min_delay -to [get_ports GPS_INITSURV_N] 0.0
set_max_delay -to [get_ports GPS_RST_N] 50.0
set_min_delay -to [get_ports GPS_RST_N] 0.0
#
set_max_delay -to [get_ports CLK_GPS_PWR_EN] 50.0
set_min_delay -to [get_ports CLK_GPS_PWR_EN] 0.0

# Clock Control
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports CLK_REF_SEL]] \
              -to   [get_ports CLK_REF_SEL] 8.0 -datapath_only
set_min_delay -to   [get_ports CLK_REF_SEL] 0.0
#
set_max_delay -from [get_ports CLK_MUX_OUT] 5.0 -datapath_only
set_min_delay -from [get_ports CLK_MUX_OUT] 0.0

# DDR3
set_max_delay -to [get_ports ddr3_reset_n] 50.0
set_min_delay -to [get_ports ddr3_reset_n] 0.0

# LEDs
set_max_delay -to [get_ports RX1_GRN_ENA]   50.0
set_min_delay -to [get_ports RX1_GRN_ENA]   0.0
set_max_delay -to [get_ports TX1_RED_ENA]   50.0
set_min_delay -to [get_ports TX1_RED_ENA]   0.0
set_max_delay -to [get_ports TXRX1_GRN_ENA] 50.0
set_min_delay -to [get_ports TXRX1_GRN_ENA] 0.0
set_max_delay -to [get_ports RX2_GRN_ENA]   50.0
set_min_delay -to [get_ports RX2_GRN_ENA]   0.0
set_max_delay -to [get_ports TX2_RED_ENA]   50.0
set_min_delay -to [get_ports TX2_RED_ENA]   0.0
set_max_delay -to [get_ports TXRX2_GRN_ENA] 50.0
set_min_delay -to [get_ports TXRX2_GRN_ENA] 0.0
#
set_max_delay -to [get_ports LED_ACT1]  50.0
set_min_delay -to [get_ports LED_ACT1]  0.0
set_max_delay -to [get_ports LED_LINK1] 50.0
set_min_delay -to [get_ports LED_LINK1] 0.0

# Control Filters
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports FE*_SEL[*]]] \
              -to   [get_ports FE*_SEL[*]] 10.0 -datapath_only
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports RX*_SEL[*]]] \
              -to   [get_ports RX*_SEL[*]] 10.0 -datapath_only
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports RX*_BSEL[*]]] \
              -to   [get_ports RX*_BSEL[*]] 10.0 -datapath_only
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports TX*_BSEL[*]]] \
              -to   [get_ports TX*_BSEL[*]] 10.0 -datapath_only

# PA Control
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports TX_HFAMP*_ENA]] \
              -to   [get_ports TX_HFAMP*_ENA] 10.0 -datapath_only
set_max_delay -from [all_fanin -only_cells -startpoints_only -flat [get_ports TX_LFAMP*_ENA]] \
              -to   [get_ports TX_LFAMP*_ENA] 10.0 -datapath_only

# SFP
set_max_delay -from [get_ports SFP1_RXLOS] 50.0
set_min_delay -from [get_ports SFP1_RXLOS] 0.0
set_max_delay -to   [get_ports SFP1_TXDISABLE] 50.0
set_min_delay -to   [get_ports SFP1_TXDISABLE] 0.0
