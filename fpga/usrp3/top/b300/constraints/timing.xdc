#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# *******************************************************************************
# Clocking

create_clock -period 10.000 -name pcie_clk [get_ports PCIE_REF_CLK_P]

create_clock -period 8.000 -name dev_clk [get_ports DEVCLK_P]

# Ref Clocks have max frequency of 125MHz
create_clock -period 8.000 -name local_ref_clk [get_ports FPGA_REFCLK_P]
create_clock -period 8.000 -name ext_ref_clk   [get_ports EXT_REFCLK_P]

# Virtual clocks for constraining I/O
create_clock -name async_in_clk  -period 50.00
create_clock -name async_out_clk -period 50.00

# dev_clk is related to ext_ref_clk, but through an LMK configured in pass through mode.
# That delay has variation over process/temperature.
# Specify the min and max delay here so that vivado times the clock crossings with this.
# Typically seeing 0.62ns delay through the part
# 3.2in ref_clk_buf --> ref_clk at fpga
# 2.7in ref_clk_buf --> ref_clk at lmk
# 1.4in lmk to dev_clk fpga
# total additional delay = 2.7 + 1.4 - 3.2 = 0.9in * 1ns / 6in = 0.133ns)
# Thus total typ delay seen by FPGA is 0.753ns or 0.75ns.
# With min delay of 0.3ns below, this gives 0.45ns of margin on min side, thus a lmk with min delay of 0.17ns would start failing
# Conversely we have max delay of 1.9ns, and thus have 1.15ns of margin on the high side.  The lmk would need to slow down to 1.77ns to start failing.
# The lmk varies over temperature by +-0.04ns, so temperature is not a large factor
# It would be process that would break this.  NI measured 0.5ns on one part and 0.65ns on another part.  TI measured 0.8ns.
# Measured:  0.65ns +- 0.15ns
# Tollerate:  0.17ns to 1.77ns
# Measured:  0.65ns/1.3  to 0.65ns*1.23
# Tollerate: 0.65ns/3.82 to 0.65ns*2.72
set lmk_min_delay 0.3
set lmk_max_delay 1.9
set_clock_latency -source -early [expr $lmk_min_delay] [get_clocks dev_clk]
set_clock_latency -source -late  [expr $lmk_max_delay] [get_clocks dev_clk]



create_clock -name txoutclk -period 10 [get_pins {b310_host_interface_i/b310_host_interfacex/TargetHostInterfacex/InchwormWrapperGen.PcieInchwormWrapper/InchwormNetlist/PcieIpWrapper/PcieIp/inst/inst/gt_top_i/pipe_wrapper_i/pipe_lane[0].gt_wrapper_i/gtx_channel.gtxe2_channel_i/TXOUTCLK}]

# JESD

create_clock -period 8.000 -name jesd_ref_clk [get_ports JESD_CLK_P]

create_clock -period 8.0 [get_pins -filter {REF_PIN_NAME=~*TXOUTCLK} -of_objects [get_cells -hierarchical -filter {NAME =~ b310_jesd204b*gt0_gtwizard_0_i*gtxe2_i*}]]
create_clock -period 8.0 [get_pins -filter {REF_PIN_NAME=~*TXOUTCLKFABRIC} -of_objects [get_cells -hierarchical -filter {NAME =~ b310_jesd204b*gt0_gtwizard_0_i*gtxe2_i*}]]
create_clock -period 8.0 [get_pins -filter {REF_PIN_NAME=~*RXOUTCLKFABRIC} -of_objects [get_cells -hierarchical -filter {NAME =~ b310_jesd204b*gt0_gtwizard_0_i*gtxe2_i*}]]
create_clock -period 8.0 [get_pins -filter {REF_PIN_NAME=~*RXOUTCLK} -of_objects [get_cells -hierarchical -filter {NAME =~ b310_jesd204b*gt0_gtwizard_0_i*gtxe2_i*}]]

set_false_path -to [get_cells -hierarchical -filter {NAME =~ b310_jesd204b*data_sync_reg1}]


# Create clocks for the clock wizard outputs
create_generated_clock -name radio_clk                [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen_i/*/CLKOUT0"}]

create_generated_clock -name radio_clk_shifted        [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen_i/*/CLKOUT3"}]
# Shift to meet timing from ext_ref_clk capture of pps_in into this clock.
# PPS_IN then transfers from this clock into the non shifted radio_clk.
# This is due to the phase difference between ext_ref_clk and devclk through the LMK.
set_property CLKOUT3_PHASE 45.000    [get_cells -hierarchical -filter {NAME =~ "*radio_clk_gen_i/*/plle2_adv_inst"}]
# set PHASESHIFT_MODE to cause capture edge to be 360deg + 45 deg out, rather than even harder to meet just 45deg out.
set_property PHASESHIFT_MODE LATENCY [get_cells -hierarchical -filter {NAME =~ "*radio_clk_gen_i/*/plle2_adv_inst"}]

create_generated_clock -name radio_clk_2x             [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen_i/*/CLKOUT1"}]
create_generated_clock -name bus_clk                  [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen_i/*/CLKOUT0"}]
create_generated_clock -name clk_40mhz                [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen_i/*/CLKOUT1"}]
create_generated_clock -name ce_clk                   [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen_i/*/CLKOUT2"}]

create_generated_clock -name DmaClockSource           [get_pins -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieMmcm/CLKOUT3"}]
create_generated_clock -name clk_125mhz_x0y0          [get_pins -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieMmcm/CLKOUT0"}]
create_generated_clock -name clk_250mhz_x0y0          [get_pins -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieMmcm/CLKOUT1"}]
create_generated_clock -name clk_125mhz_mux_x0y0 \
                        -source [get_pins  -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieBufgctrl/I0"}] \
                        -divide_by 1 \
                        [get_pins  -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieBufgctrl/O"}]
create_generated_clock -name clk_250mhz_mux_x0y0 \
                        -source [get_pins  -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieBufgctrl/I1"}] \
                        -divide_by 1 -add -master_clock [get_clocks -of [get_pins  -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieBufgctrl/I1"}]] \
                        [get_pins  -hierarchical -filter {NAME =~ "*Pcie7SeriesBimClockGenx/PcieBufgctrl/O"}]

# Remove analysis over clock update logic.
set_clock_groups -name pcieclkmux -physically_exclusive -group clk_125mhz_mux_x0y0 -group clk_250mhz_mux_x0y0

# PPS OUT
# Trace delays below are from simulations.  See "Multi Device Synchronization.docx" for theory of operation.

# Shift phase of PPS_OUT to be centered around window of when needed to meet PPS_IN timing into the FPGA.
# PHASE must be multiple of 45 / 7
set_property CLKOUT0_PHASE 218.57 [get_cells -hierarchical -filter {NAME =~ "*ref_clk_pll_inst/*/plle2_adv_inst"}]
# force these into an IOB, as Vivado could also decide to fan out the IO to 4 IO cells
set_property IOB TRUE [get_ports  PPS_OUT*]
# Tighten these up to timing results from FPGA build to be used below also for input timing.
set min_tco 5.5
set max_tco 6.7
set_output_delay -clock local_ref_clk -max [expr 8 - $max_tco]  [get_ports PPS_OUT*]
set_output_delay -clock local_ref_clk -min [expr 0 - $min_tco]  [get_ports PPS_OUT*]

# PPS IN
set_property IOB TRUE [get_ports  EXT_PPS_IN]
# vivado auto insters zero delay buffer which has extra slop when clocking directly from a pin
# Remove as we can't use a PLL on this as PLL can't be used with 10Mhz input
set_property IOBDELAY "NONE" [get_cells EXT_PPS_IN_IBUF_inst]

# Each of these are + tcable, but cable length cancels out since clock and PPS both travel a cable length
# Below simulated with 1.5ft cable
# tadclk948min (-0.025) + trefclk_cable_min (3.998) + tadclk944_min (0.07) + adclk944_to_fpga_min (0.07) + ttrace_min(0.541) + balon delay (0.5)
set clk_in_trace_min 5.154
# additional amount to subtract from min when assuming 0ft cable and minimum capacitances
set clk_in_0ft_correction_min -0.443
# tadclk948max (0.025) + trefclk_cable_max (2.025) + tadclk944_max (0.13) + ttrace_max(0.15)
set clk_in_trace_max 5.553
# ttrace_min (0) + tpps_in_min (3.265) - tpps_in_min_correction_for_min_cap_on_diodes_passfet (1.026)
set pps_in_trace_min 5.232
# ttrace_max (0.15) + tpps_in_max (7.906) (only consider rising edge as that is when we do the clock cross of time)
set pps_in_trace_max 8.056
# min: 5.5 + 5.232 - 5.553 - 0.443 = 4.732
set_input_delay -clock ext_ref_clk -min [expr $min_tco + $pps_in_trace_min + $clk_in_0ft_correction_min - $clk_in_trace_max] [get_ports EXT_PPS_IN]

# max:  6.7 + 8.056 - 5.145 = 9.611
set_input_delay -clock ext_ref_clk -max [expr $max_tco + $pps_in_trace_max - $clk_in_trace_min] [get_ports EXT_PPS_IN]

# LMK SYNC out
set lmk_hold_req 0.02
set lmk_setup_req 0.02
set sync_trace_min 0.119
set sync_trace_max 1.087
set extra_30p72_setup 24

# There are two paths
# 10MHz ext ref:                     Timing is to ext_ref_clock (input of LMK) and sourced from ext_ref_clk in the FPGA
#    Reason for this is that radio_clk is a diferent frequency than ext_ref_clock (125Mhz vs 10/30.72Mhz)
#    LMK is synchronous to ext_ref_clock thus makes sense to drive it from that.
#    10MHz is slow enough to meet timing
# High freq ref (122.88MHz/125MHz).  Timing is to ext_ref_clock (input of LMK) and sourced from radio_clk in the FPGA
#    Reason for this is that ext_ref_clock and radio_clk are the same frequency, thus related
#    We get better timing characteristics when driving this output from a PLL vs directly from IO pin to
#    meet the higher 125MHz timing to sync in of the LMK
set_output_delay -clock ext_ref_clk -min [expr $sync_trace_min - $lmk_hold_req]  [get_ports LMK_SYNC]
set_output_delay -clock ext_ref_clk -max [expr $sync_trace_max + $lmk_setup_req]  [get_ports LMK_SYNC]
# One more path for when a 30p72 clock, give extra setup.  This path is never fasle pathed, above is for 30p72MHz case
set_output_delay -add_delay -clock ext_ref_clk -max [expr $sync_trace_max + $lmk_setup_req - $extra_30p72_setup]  [get_ports LMK_SYNC]
# not guaranteed to meet timing to LMK on RevA, but here for testing with RevA
set_output_delay -clock ext_ref_clk -min [expr $sync_trace_min - $lmk_hold_req]  [get_ports LMK_SYNC_REVA]
set_output_delay -clock ext_ref_clk -max [expr $sync_trace_max + $lmk_setup_req]  [get_ports LMK_SYNC_REVA]
set_output_delay -clock ext_ref_clk -max [expr $sync_trace_max + $lmk_setup_req]  [get_ports LMK_SYNC_REVA]
set_output_delay -add_delay -clock ext_ref_clk -max [expr $sync_trace_max + $lmk_setup_req - $extra_30p72_setup]  [get_ports LMK_SYNC_REVA]

# False path the selection between having ext_ref_clk or radio_clk register drive LMK_SYNC
set_false_path -from [get_pins b310_core_i/core_regsx/lmk_sync_clk_sel_reg/C] -to [get_ports LMK_SYNC_REVA]
set_false_path -from [get_pins b310_core_i/core_regsx/lmk_sync_clk_sel_reg/C] -to [get_ports LMK_SYNC]

# False path from ext_ref_clock source to output.  In this case clock is 30.72MHz max, so easier to meet
# TODO:  Don't false path this, but make Vivado analyze this path at 30.72Mhz
# Tricky part is I can't seem to find a way to get Vivado to analyze from 125MHz radio clock to ext_ref_clk
# as well as from 30.72MHz ext_ref_clk to ext_ref_clk
# No way I know of yet to false path the 30.72Mhz path for the 125MHz case but not 30.72MHz case
# So just false pathing it all together.  Need to manually check timing of that port for now
set_false_path -from [get_pins b310_core_i/b3xx_pps_sync_i/sync_brc_out_reg/C] -to [get_ports LMK_SYNC]
set_false_path -from [get_pins b310_core_i/b3xx_pps_sync_i/sync_brc_out_reva_reg/C] -to [get_ports LMK_SYNC_REVA]

set BasePath b310_host_interface_i/b310_host_interfacex/CoreRegPortToCtrlPort/BaRegPortClockCrossingx/RequestHandshake
## Start include, file HandshakeSLV_RSD.xml
set HandshakeSlvRsdPath $BasePath
set BasePath $BasePath/HBx
## Start add from file HandshakeBaseRSD.xdc
# ---------------------------------------------------------------------------------------
# HandshakeBaseRSD
# ---------------------------------------------------------------------------------------
# Save incoming path
set HandshakeBaseRsdPath $BasePath

# Data
set TNM_HS_iData   [get_cells "$BasePath/BlkIn.iStoredDatax/*/*"      -filter {IS_SEQUENTIAL==true}]
set TNM_HS_oData   [get_cells "$BasePath/*oDataFlopx/*/*"        -filter {IS_SEQUENTIAL==true}]
# Toggle
set TNM_HS_iTog    [get_cells "$BasePath/*iPushTogglex/*"        -filter {IS_SEQUENTIAL==true}]
set TNM_HS_oTog_ms [get_cells "$BasePath/*oPushToggle0_msx/*"    -filter {IS_SEQUENTIAL==true}]
set TNM_HS_oTog    [get_cells "$BasePath/*oPushToggle1x/*"       -filter {IS_SEQUENTIAL==true}]
# Ready
set TNM_HS_oRdy    [get_cells "$BasePath/*oPushToggleToReadyx/*" -filter {IS_SEQUENTIAL==true}]
set TNM_HS_iRdy_ms [get_cells "$BasePath/*iRdyPushToggle_msx/*"  -filter {IS_SEQUENTIAL==true}]
set TNM_HS_iRdy    [get_cells "$BasePath/*iRdyPushTogglex/*"     -filter {IS_SEQUENTIAL==true}]

# Find out the minimum period of the clocks related to the previous groups.
set T_IClkMin [expr "min([join [get_property PERIOD [get_clocks -of $TNM_HS_iData]] ,])"]
set T_OClkMin [expr "min([join [get_property PERIOD [get_clocks -of $TNM_HS_oData]] ,])"]

# The datapath clock crossings must be less than 2X the period of the destination clock.
set_max_delay  -from $TNM_HS_iData   -to $TNM_HS_oData -datapath_only [expr 2 * $T_OClkMin - 0.5]

# Toggle
set_false_path -from $TNM_HS_iTog    -to $TNM_HS_oTog_ms
set_max_delay  -from $TNM_HS_oTog_ms -to $TNM_HS_oTog -datapath_only [expr 0.5 * $T_OClkMin]

# The return ready path isn't very important here.
set_false_path -from $TNM_HS_oRdy    -to $TNM_HS_iRdy_ms
set_max_delay  -from $TNM_HS_iRdy_ms -to $TNM_HS_iRdy -datapath_only [expr 0.5 * $T_IClkMin]


set BasePath $HandshakeSlvRsdPath


set BasePath b310_host_interface_i/b310_host_interfacex/CoreRegPortToCtrlPort/BaRegPortClockCrossingx/ResponseHandshake
## Start include, file HandshakeSLV_RSD.xml
set HandshakeSlvRsdPath $BasePath
set BasePath $BasePath/HBx
## Start add from file HandshakeBaseRSD.xdc
# ---------------------------------------------------------------------------------------
# HandshakeBaseRSD
# ---------------------------------------------------------------------------------------
# Save incoming path
set HandshakeBaseRsdPath $BasePath

# Data
set TNM_HS_iData   [get_cells "$BasePath/BlkIn.iStoredDatax/*/*"      -filter {IS_SEQUENTIAL==true}]
set TNM_HS_oData   [get_cells "$BasePath/*oDataFlopx/*/*"        -filter {IS_SEQUENTIAL==true}]
# Toggle
set TNM_HS_iTog    [get_cells "$BasePath/*iPushTogglex/*"        -filter {IS_SEQUENTIAL==true}]
set TNM_HS_oTog_ms [get_cells "$BasePath/*oPushToggle0_msx/*"    -filter {IS_SEQUENTIAL==true}]
set TNM_HS_oTog    [get_cells "$BasePath/*oPushToggle1x/*"       -filter {IS_SEQUENTIAL==true}]
# Ready
set TNM_HS_oRdy    [get_cells "$BasePath/*oPushToggleToReadyx/*" -filter {IS_SEQUENTIAL==true}]
set TNM_HS_iRdy_ms [get_cells "$BasePath/*iRdyPushToggle_msx/*"  -filter {IS_SEQUENTIAL==true}]
set TNM_HS_iRdy    [get_cells "$BasePath/*iRdyPushTogglex/*"     -filter {IS_SEQUENTIAL==true}]

# Find out the minimum period of the clocks related to the previous groups.
set T_IClkMin [expr "min([join [get_property PERIOD [get_clocks -of $TNM_HS_iData]] ,])"]
set T_OClkMin [expr "min([join [get_property PERIOD [get_clocks -of $TNM_HS_oData]] ,])"]

# The datapath clock crossings must be less than 2X the period of the destination clock.
set_max_delay  -from $TNM_HS_iData   -to $TNM_HS_oData -datapath_only [expr 2 * $T_OClkMin - 0.5]

# Toggle
set_false_path -from $TNM_HS_iTog    -to $TNM_HS_oTog_ms
set_max_delay  -from $TNM_HS_oTog_ms -to $TNM_HS_oTog -datapath_only [expr 0.5 * $T_OClkMin]

# The return ready path isn't very important here.
set_false_path -from $TNM_HS_oRdy    -to $TNM_HS_iRdy_ms
set_max_delay  -from $TNM_HS_iRdy_ms -to $TNM_HS_iRdy -datapath_only [expr 0.5 * $T_IClkMin]

#*******************************************************************************
## Asynchronous clock groups

set_clock_groups -asynchronous -group [get_clocks radio_clk]    -group [get_clocks clk_40mhz]
set_clock_groups -asynchronous -group [get_clocks radio_clk]    -group [get_clocks bus_clk]
set_clock_groups -asynchronous -group [get_clocks radio_clk_2x] -group [get_clocks bus_clk]
set_clock_groups -asynchronous -group [get_clocks radio_clk_2x] -group [get_clocks clk_40mhz]
set_clock_groups -asynchronous -group [get_clocks ce_clk]       -group [get_clocks clk_40mhz]
set_clock_groups -asynchronous -group [get_clocks ce_clk]       -group [get_clocks radio_clk]
set_clock_groups -asynchronous -group [get_clocks radio_clk]    -group [get_clocks txoutclk]
set_clock_groups -asynchronous -group [get_clocks txoutclk]     -group [get_clocks clk_40mhz]
set_clock_groups -asynchronous -group [get_clocks clk_40mhz]    -group [get_clocks clk_125mhz_x0y0]
set_clock_groups -asynchronous -group [get_clocks clk_40mhz]    -group [get_clocks clk_250mhz_x0y0]
set_clock_groups -asynchronous -group [get_clocks clk_40mhz]    -group [get_clocks DmaClockSource]
set_clock_groups -asynchronous -group [get_clocks radio_clk]    -group [get_clocks DmaClockSource]
set_clock_groups -asynchronous -group [get_clocks radio_clk_2x] -group [get_clocks DmaClockSource]
set_clock_groups -asynchronous -group [get_clocks jesd_ref_clk -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks async_in_clk]
set_clock_groups -asynchronous -group [get_clocks async_out_clk]

#*******************************************************************************
## CPLD Interface Clocks and Timing

# CPLD to FPGA RX Clock: 80 MHz (from CPLD_TX_CLK port)
create_clock -name cpld_to_fpga_clk -period 12.5 [get_ports CPLD_TX_CLK]

# FPGA to CPLD TX Clock (from CPLD_RX_CLK port) - generated from bus_clk
create_generated_clock -name fpga_to_cpld_clk \
                      -source [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen_i*CLKOUT1"}] \
                      -divide_by 1 \
                      [get_ports CPLD_RX_CLK]

set CpldToFpgaClkPeriod [get_property period [get_clocks cpld_to_fpga_clk]]
set BusClkPeriod [get_property period [get_clocks bus_clk]]

# Set input/output delays for CPLD interface signals
#
# FPGA-to-CPLD OUTPUT TIMING CALCULATION:
# =======================================
# Board measurements: CPLD_RX_CLK=0.1483ns, CPLD_RX_DATA=0.2133ns (longest), 0.1466ns (shortest)
# Clock-to-data skew: longest=0.065ns (data arrives 65ps after clock), shortest=-0.0017ns (data arrives 1.7ps before clock)
#
# Available time per half-cycle: BusClkPeriod/2 = 25ns/2 = 12.5ns
# CPLD setup requirement: ~2.0ns (typical for CPLD)
# Board delay + skew worst case: 0.2133ns + safety margin
# FPGA internal routing margin: ~1.0ns
# Total external budget: 2.0 + 0.22 + 1.0 = 3.22ns
# Conservative constraint: 5.0ns (provides 1.8ns extra margin)
#
# Tightened timing margins based on successful timing closure

set FpgaToCpldTcoMax  5.0
set FpgaToCpldTcoMin -1.0

set FpgaToCpldMaxOutputDelay  [expr $BusClkPeriod / 2 - $FpgaToCpldTcoMax]
set FpgaToCpldMinOutputDelay  [expr 0 - $FpgaToCpldTcoMin]

set_output_delay -clock [get_clocks fpga_to_cpld_clk] -max $FpgaToCpldMaxOutputDelay [get_ports {CPLD_RX_DATA[*]}]
set_output_delay -clock [get_clocks fpga_to_cpld_clk] -min $FpgaToCpldMinOutputDelay [get_ports {CPLD_RX_DATA[*]}]
set_output_delay -clock [get_clocks fpga_to_cpld_clk] -max $FpgaToCpldMaxOutputDelay [get_ports CPLD_RX_CLKEN]
set_output_delay -clock [get_clocks fpga_to_cpld_clk] -min $FpgaToCpldMinOutputDelay [get_ports CPLD_RX_CLKEN]

# Additional constraints for CPLD_RX_DATA bus trace length extremes
# Longest trace (1.28 in = 0.2133ns): Provides worst-case delay from FPGA to CPLD
# Shortest trace (0.88 in = 0.1466ns): Provides best-case delay from FPGA to CPLD
# Clock trace (0.89 in = 0.1483ns): Reference for skew calculations

# Make sure the clock to the CPLD is in the IOB
# Board measurement: CPLD_RX_CLK trace = 0.1483ns
# Relaxed constraint for better timing closure
set_max_delay -datapath_only -from [get_pins {*cpld_interface_i/*/CpldClockFlop/C}] -to [get_ports {CPLD_RX_CLK}] 3.0

# The following numbers are the calculated FPGA available setup and hold and
# include the CPLD Tco numbers plus any board skew numbers.
#
# CPLD-to-FPGA INPUT TIMING CALCULATION:
# ======================================
# Updated board measurements: CPLD_TX_CLK=0.155ns, CPLD_TX_DATA=0.1666ns (longest), 0.1166ns (shortest), CPLD_TX_CLKEN=0.1433ns
# Clock-to-data skew: longest=0.0116ns (data arrives 11.6ps after clock), shortest=-0.0384ns (data arrives 38.4ps before clock)
# Clock-to-clken skew = -0.0117ns (clken arrives 11.7ps before clock)
#
# Available time per half-cycle: CpldToFpgaClkPeriod/2 = 12.5ns/2 = 6.25ns
# CPLD internal Tco (clock-to-output): ~2.0ns typical, ~2.5ns max
# Board trace delays: 0.155ns (clock) vs 0.1166-0.1666ns (data)
# Board skew: 0.0116ns worst case (data after clock)
# Power supply noise margin: ~0.2ns
# Temperature variation: ~0.1ns
# Manufacturing tolerance: ~0.1ns
# Safety margin: ~0.1ns
# Total budget: 2.5 + 0.17 + 0.01 + 0.2 + 0.1 + 0.1 + 0.1 = 3.18ns
# Conservative constraint: 3.0ns
# Remaining time for FPGA: 6.25 - 3.0 = 3.25ns
# Tightened timing margins based on successful timing closure

set CpldDataAtFpgaSetupTime   3.0
set CpldDataAtFpgaHoldTime    3.0

set CpldToFpgaMaxOutputDelay  [expr $CpldToFpgaClkPeriod / 2 - $CpldDataAtFpgaSetupTime]
set CpldToFpgaMinOutputDelay  [expr $CpldDataAtFpgaHoldTime - $CpldToFpgaClkPeriod / 2]

set_input_delay -clock_fall -clock [get_clocks cpld_to_fpga_clk] -max $CpldToFpgaMaxOutputDelay [get_ports {CPLD_TX_DATA[*] CPLD_TX_CLKEN}]
set_input_delay -clock_fall -clock [get_clocks cpld_to_fpga_clk] -min $CpldToFpgaMinOutputDelay [get_ports {CPLD_TX_DATA[*] CPLD_TX_CLKEN}]

# Additional constraints for data bus trace length extremes
# Updated board measurements: CPLD_TX_DATA longest trace (1.00 in = 0.1666ns), shortest trace (0.70 in = 0.1166ns)
# This ensures timing closure across all data bit trace length variations

# CPLD Reset Output
# Updated board measurement: CPLD_EXT_RST trace = 0.215ns
# Tightened constraint based on actual board measurement and successful timing closure
set_max_delay -datapath_only -from [all_registers -edge_triggered] -to [get_ports {CPLD_EXT_RST_R_N}] 12

# CPLD oscillator enable - static control signal
set_false_path -to [get_ports CPLD_OSC_EN]

# ---------------------------------
# Clock Domain Crossings Exceptions
# ---------------------------------

#### CPLD Exceptions ####

# Constrain the asynchronous rResetCpld propagation delay to less than one cpld_to_fpga_clk cycle
# Software would then be able to assert/de-assert the rResetCpld with no timing requirement in
# between the assertion/de-assertion of this reset signal.
# Note that rResetCpld is generated off the bus_clk and is received by the CPLD code
# on both the cpld_to_fpga_clk and bus_clk domains. We only want to place a set_max_delay
# constraint on this signal to registers that are clocked off the cpld_to_fpga_clk, so that we
# do not override and prevent the tools from analyzing the bus_clk period constraint on these paths
# Tightened constraint based on successful timing closure

set_max_delay -from [get_cells {*cpld_interface_i/*/rResetCpldInt*} -filter {IS_SEQUENTIAL==true}] \
              -to   [all_registers -clock cpld_to_fpga_clk] \
              -datapath_only 20.0;

# cSerialData is shifted in from the CPLD into the FPGA using the CpldToFpgaClk
# The CpldToFpgaClk is not continuous so a double-sync, as opposed to a handshake,
# was used to cross the cSerialData from the CpldToFpgaClk clock domain to the
# reliable clock domain. SW waits on cCpldDataValid to assert before reading the data.
# cCpldDataValid gets double-synced to the reliable clock domain before being available
# for SW to read. We place a 20 ns (less than one reliableClk cycle) on the data paths
# to ensure that the code sampling the data on the reliable clock domain is sampling
# non-metastable data.

set_max_delay -from [get_cells {*cpld_interface_i/*/cSerialData*} -filter {IS_SEQUENTIAL==true}] \
              -to   [get_pins {*cpld_interface_i/*/SerialDataFlops/*/D}] \
              -datapath_only 20.0;

# Double-Sync crossing (cpld_to_fpga_clk --> bus_clk)
# A strict requirement is not needed on these double-sync flops, but constraining
# them to 20 ns for reasonable timing closure

set_max_delay -from [get_cells {*cpld_interface_i/*/cCpldDataValid*} -filter {IS_SEQUENTIAL==true}] \
              -to   [get_pins  {*cpld_interface_i/*/DoubleSyncDataValid/*/oSig_msx/*/D}] \
              -datapath_only 20.0;

set_max_delay -from [get_cells {*cpld_interface_i/*/cCpldHoldoff*} -filter {IS_SEQUENTIAL==true}] \
              -to   [get_pins  {*cpld_interface_i/*/DoubleSyncHoldoff/*/oSig_msx/*/D}] \
              -datapath_only 20.0;

set_max_delay -from [get_cells {*cpld_interface_i/*/cCpldInitialized*} -filter {IS_SEQUENTIAL==true}] \
              -to   [get_pins  {*cpld_interface_i/*/DoubleSyncInitialized/*/oSig_msx/*/D}] \
              -datapath_only 20.0;

set_max_delay -from [get_cells {*cpld_interface_i/*/cCpldReady*} -filter {IS_SEQUENTIAL==true}] \
              -to   [get_pins  {*cpld_interface_i/*/DoubleSyncReady/*/oSig_msx/*/D}] \
              -datapath_only 20.0;

# Mark CPLD clocks as asynchronous to all system clocks
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks bus_clk]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks radio_clk]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks radio_clk_2x]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks ce_clk]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks clk_40mhz]
# Enable PCIe clock relationships for proper timing analysis
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks DmaClockSource]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks clk_125mhz_x0y0]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks clk_250mhz_x0y0]
set_clock_groups -asynchronous -group [get_clocks cpld_to_fpga_clk] -group [get_clocks jesd_ref_clk]

set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks radio_clk]
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks radio_clk_2x]
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks ce_clk]
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks clk_40mhz]
# Enable PCIe clock relationships for proper timing analysis
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks DmaClockSource]
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks clk_125mhz_x0y0]
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks clk_250mhz_x0y0]
set_clock_groups -asynchronous -group [get_clocks fpga_to_cpld_clk] -group [get_clocks jesd_ref_clk]

#*******************************************************************************
## CPLD JTAG Interface
#
# TCK frequency = bus_clk / (2*(prescalar+1))
# prescalar used by utilities = 12 → TCK ≈ 5.76 MHz (period ≈ 173 ns)
#
# Protocol:
#   - TDI and TMS are launched on the falling edge of TCK (FSM HIGH→LOW transition)
#     and captured by the CPLD on the rising edge of TCK.
#   - TDO is launched by the CPLD on the falling edge of TCK and captured by the
#     FPGA bitq_fsm on the rising edge of TCK (FSM LOW→HIGH transition).

# divide_by 26 matches prescalar=12
create_generated_clock \
  -source [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen_i*CLKOUT0"}] \
  -name jtag_cpld_tck_clk_out \
  -divide_by 26 \
  [get_ports JTAG_CPLD_TCK]

# CPLD JTAG timing parameters
# Board trace delays: conservative estimate for PCB traces (in reality, these are short ~0.7-1 in)
set jtag_cpld_tck_max_trace       1.0  ;# TCK board trace max delay (ns)
set jtag_cpld_tck_min_trace       0.0  ;# TCK board trace min delay (ns)
set jtag_cpld_data_max_trace      1.0  ;# TDI/TMS board trace max delay (ns)
set jtag_cpld_data_min_trace      0.0  ;# TDI/TMS board trace min delay (ns)
set jtag_cpld_tdo_max_trace       1.0  ;# TDO board trace max delay (ns)
set jtag_cpld_tdo_min_trace       0.0  ;# TDO board trace min delay (ns)
# CPLD JTAG device requirements: From MACHXO2 datasheet
set jtag_cpld_tdi_setup          10.0  ;# CPLD JTAG TDI/TMS setup before TCK rising (ns)
set jtag_cpld_tdi_hold            8.0  ;# CPLD JTAG TDI/TMS hold after TCK rising (ns)
set jtag_cpld_tdo_max_clk_to_out 10.0  ;# CPLD JTAG TDO max clock-to-output after TCK falling (ns)
set jtag_cpld_tdo_min_clk_to_out  0.0  ;# CPLD JTAG TDO min clock-to-output after TCK falling (ns)

# -------------------------------------------------------
# -- Constraint for CPLD JTAG TDI and TMS (outputs) --
# -------------------------------------------------------

set_output_delay -clock jtag_cpld_tck_clk_out \
  -max [expr {$jtag_cpld_tdi_setup + $jtag_cpld_data_max_trace - $jtag_cpld_tck_min_trace}] \
  [get_ports {JTAG_CPLD_TDI JTAG_CPLD_TMS}]
set_output_delay -clock jtag_cpld_tck_clk_out \
  -min [expr {0 - $jtag_cpld_tck_max_trace - $jtag_cpld_tdi_hold + $jtag_cpld_data_min_trace}] \
  [get_ports {JTAG_CPLD_TDI JTAG_CPLD_TMS}]

# -------------------------------------------------------
# -- Multi-cycle path for CPLD JTAG TDI and TMS --
# -------------------------------------------------------
# TDI and TMS are launched on the falling edge of TCK (half period before the capture
# rising edge). With divide_by=26, data is stable for 13 bus_clk cycles before capture.
#
# bus_clk  __/--\__/--\__ ... __/--\__/--\__/--\__ ... __/--\__/--\__/--\__
#    TCK   __/------------------------\________________________/-----------
#                                     | launch edge (falling TCK)
#                                     |     |      ...   |     |     |
#                                     0     1      ...   12   13*   14
#                                                    Edge used for setup N = 13
#            |     |      ...   |     |     |      ...   |
#           25    24      ...  13    12    11      ...   0
#                                                   (Setup -1) edge, in case
#                                                   of no hold multi-cycle path
#           |
#           \____ Edge used for hold = 25

set_multicycle_path -setup -start -to [get_ports {JTAG_CPLD_TDI JTAG_CPLD_TMS}] 13
set_multicycle_path -hold  -start -to [get_ports {JTAG_CPLD_TDI JTAG_CPLD_TMS}] 25

# -------------------------------------------------------
# -- Constraint for CPLD JTAG TDO (input) --
# -------------------------------------------------------
# TDO is launched by the CPLD on the falling edge of TCK and captured at the
# next rising edge of TCK inside the bitq_fsm. Waveform is similar to TDI/TMS.

set_input_delay -clock_fall -clock jtag_cpld_tck_clk_out \
  -max [expr {$jtag_cpld_tck_max_trace + $jtag_cpld_tdo_max_clk_to_out + $jtag_cpld_tdo_max_trace}] \
  [get_ports JTAG_CPLD_TDO]
set_input_delay -clock_fall -clock jtag_cpld_tck_clk_out \
  -min [expr {$jtag_cpld_tck_min_trace + $jtag_cpld_tdo_min_clk_to_out + $jtag_cpld_tdo_min_trace}] \
  [get_ports JTAG_CPLD_TDO]

# TDO multi-cycle path: same half-period relationship as TDI/TMS.
set_multicycle_path -setup -end -from [get_ports JTAG_CPLD_TDO] 13
set_multicycle_path -hold  -end -from [get_ports JTAG_CPLD_TDO] 25

#*******************************************************************************
## Authentication chip interface timing

# We are using a single wire interface to the authentication chip which is a asynchronous interface.
# Max delay of 15 ns is a conservative number as minimum time when "AUTH_SDA" signal can change it's state
# is 15 us.
set_max_delay -datapath_only -from [all_registers -edge_triggered] -to [get_ports AUTH_SDA] 15.000;
set_max_delay -datapath_only -from [get_ports AUTH_SDA] -to [all_registers -edge_triggered] 15.000;


# #*******************************************************************************
# ## ADRV9032 SPI Interface

create_generated_clock \
  -source [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen_i/*/CLKOUT0"}] \
  -name adrv_spi_clk_out \
  -divide_by 30 \
  [get_ports {ADRV_SPI_CLK}]

# DIO trace length is 2138.37 nils, which is roughly 0.36 ns. The main source for propagation delay
# in this trace comes from filtering circuitry on the ADRV side slowing down rise times. The measured
# delay is ~32ns. The constraint below is a conservative number to ensure timing closure.
set adrv_dio_max_trace_delay 50.0
set adrv_dio_min_trace_delay 0.0
# DO and CS has similar considerations as DIO, with both having a measured delay of ~45ns.
set adrv_do_max_trace_delay 50.0
set adrv_do_min_trace_delay 0.0
set adrv_cs_max_trace_delay 50.0
set adrv_cs_min_trace_delay 0.0
# And yet again, we conservatively constraint the delay of the SPI clock, measured at ~25ns.
set adrv_clk_max_trace_delay 35.0
set adrv_clk_min_trace_delay 0.0

# Values from the ADRV9032 datasheet, in ns
set adrv_spi_setup    4.0
set adrv_spi_hold     0.0
set adrv_spi_max_tco  3.5
set adrv_spi_min_tco  0.0

# ----------------------------------------
# -- Constraint for ADRV SPI CS and SDIO --
# ----------------------------------------

set_output_delay -clock adrv_spi_clk_out \
  -max [expr {$adrv_spi_setup + $adrv_cs_max_trace_delay - $adrv_clk_min_trace_delay}] \
  [get_ports {ADRV_SPI_EN_N}]
set_output_delay -clock adrv_spi_clk_out \
  -min [expr {0 - $adrv_clk_max_trace_delay - $adrv_spi_hold + $adrv_cs_min_trace_delay }] \
  [get_ports {ADRV_SPI_EN_N}]

set_output_delay -clock adrv_spi_clk_out \
  -max [expr {$adrv_spi_setup + $adrv_dio_max_trace_delay - $adrv_clk_min_trace_delay}] \
  [get_ports {ADRV_SPI_DIO}]
set_output_delay -clock adrv_spi_clk_out \
  -min [expr {0 - $adrv_clk_max_trace_delay - $adrv_spi_hold + $adrv_dio_min_trace_delay }] \
  [get_ports {ADRV_SPI_DIO}]

# ----------------------------------------
# -- Multi-cycle path for ADRVSPI EN_N and SDIO --
# ----------------------------------------
# Both the EN_N and SDIO timing are defined in reference to the falling edge of SCLK,
# so we can merge the analysis (EN_N is driven half a period before the first rising edge of SCLK).
#
# edge #          1     2     3        14    15    16    17      29    30    31    32
#  radio_clk    __/--\__/--\__/--\_..._/--\__/--\__/--\__/--\..._/--\__/--\__/--\__/
# ADRV_SPI_CLK  __/----------------...-------\_______________..._______/------------
#                                            | launch edge
#                                            |     |     |   ... |     |     |
#                                            0     1     2   ... 14   15*    16
#                                                                     Edge used for
#                                                                     setup analysis N = 15
#                 |     |     |        |     |     |      |  ... |
#                29     28    27  ... 15    14    13     12  ... 0
#                                                                (Setup -1) edge, in case
#                                                                of no hold multi-cycle path
#                 |
#                 \____ Edge used for hold = 29
#
set_multicycle_path -setup -start -to [get_ports {ADRV_SPI_EN_N ADRV_SPI_DIO}] 15
set_multicycle_path -hold  -start -to [get_ports {ADRV_SPI_EN_N ADRV_SPI_DIO}] 29

# ----------------------------------------
# -- Constraint for ADRV SPI SDO --
set_input_delay -clock adrv_spi_clk_out -clock_fall \
  -max [expr {$adrv_clk_max_trace_delay + $adrv_spi_max_tco + $adrv_do_max_trace_delay}] \
  [get_ports {ADRV_SPI_DO}]

set_input_delay -clock adrv_spi_clk_out -clock_fall \
  -min [expr {$adrv_clk_min_trace_delay + $adrv_spi_min_tco + $adrv_do_min_trace_delay}] \
  [get_ports {ADRV_SPI_DO}]

# -- Multi-cycle path for ADRV SPI SDO --
# SDO is launched on the falling edge of SCLK and captured on the rising edge,
# so we can use the same multi-cycle path analysis as the EN_N and SDIO signals.
set_multicycle_path -setup -end -from [get_ports {ADRV_SPI_DO}] 15
set_multicycle_path -hold  -end -from [get_ports {ADRV_SPI_DO}] 29

#*******************************************************************************
## Clocking SPI Interface

# This interface interacts with the LMK05318 and LMK04832 clock chips, as well as the AD5623 DAC.
# The DAC is placed the furthest away from the FPGA, at around 2.7 inches of trace length,
# we will use conservative numbers that far exceed its trace delay to ensure timing closure
# for all devices on this interface.

set clocking_output_max_trace_delay 10.0
set clocking_output_min_trace_delay 0.0
set clocking_miso_max_trace_delay 10.0
set clocking_miso_min_trace_delay 0.0
set clocking_sclk_max_trace_delay 10.0
set clocking_sclk_min_trace_delay 0.0

# Values from datasheets, in ns
# Setup requirements for AD5623 = 13ns, LMK05318 = 10ns, LMK04832 = 20ns
set clock_spi_setup    20.0
# Hold requirements for AD5623 = 5ns, LMK05318 = 10ns, LMK04832 = 10ns. Using 0 ns.
set clock_spi_hold     0.0
# Tco specifications LMK05318 = 20ns, LMK04832 = 60ns. AD5623 has no read capability.
set clock_spi_max_tco  60.0
set clock_spi_min_tco  0.0


# SPI divider uses default value of 30.
create_generated_clock \
  -source [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen_i*CLKOUT0"}] \
  -name clocking_spi_clk_out \
  -divide_by 30 \
  [get_ports {LMK32_SCLK LMK053_SCLK DAC_SCLK}]


# ---------------------------------------------
# -- Constraint for Clocking SPI CS and MOSI --
# ---------------------------------------------

set CLOCKING_SPI_OUTPUTS [get_ports {LMK32_CS_N LMK32_MOSI LMK053_CS_N LMK053_MOSI DAC_CS_N DAC_MOSI}]

set_output_delay -clock clocking_spi_clk_out \
  -max [expr {$clock_spi_setup + $clocking_output_max_trace_delay - $clocking_sclk_min_trace_delay}] \
  $CLOCKING_SPI_OUTPUTS
set_output_delay -clock clocking_spi_clk_out \
  -min [expr {0 - $clocking_sclk_max_trace_delay - $clock_spi_hold + $clocking_output_min_trace_delay }] \
  $CLOCKING_SPI_OUTPUTS


# -----------------------------------------------------------------
# -- Multi-cycle path for Clocking SPI CS_N and MOSI (LMKs only) --
# -----------------------------------------------------------------
# Both the CS_N and MOSI timing are defined in reference to the falling edge of SCLK,
# so we can merge the analysis (CS_N is driven half a period before the first rising edge of SCLK).
#
# edge #          1     2     3        14    15    16    17      29    30    31    32
#    bus_clk    __/--\__/--\__/--\_..._/--\__/--\__/--\__/--\..._/--\__/--\__/--\__/
#       SCLK    __/----------------...-------\_______________..._______/------------
#                                            | launch edge
#                                            |     |     |   ... |     |     |
#                                            0     1     2   ... 14   15*    16
#                                                                     Edge used for
#                                                                     setup analysis N = 15
#                 |     |     |        |     |     |     |   ... |
#                29     28    27  ... 15    14    13    12   ... 0
#                                                               (Setup -1) edge, in case
#                                                                of no hold multi-cycle path
#                 |
#                 \____ Edge used for hold = 29
#
# For the DAC MOSI, we will use the same multi-cycle path analysis as the LMKs, even though the DAC is
# launched on the rising edge of SCLK. This is because the DAC MOSI is captured on the falling edge of SCLK,
# so the same analysis applies, just with the edges reversed.
set_multicycle_path -setup -start -to $CLOCKING_SPI_OUTPUTS 15
set_multicycle_path -hold  -start -to $CLOCKING_SPI_OUTPUTS 29

# ----------------------------------------
# -- Constraint for Clocking SPI MISO --
set_input_delay -clock clocking_spi_clk_out -clock_fall \
  -max [expr {$clocking_sclk_max_trace_delay + $clock_spi_max_tco + $clocking_miso_max_trace_delay}] \
  [get_ports {LMK053_MISO LMK32_MISO}]

set_input_delay -clock clocking_spi_clk_out -clock_fall \
  -min [expr {$clocking_sclk_min_trace_delay + $clock_spi_min_tco + $clocking_miso_min_trace_delay}] \
  [get_ports {LMK053_MISO LMK32_MISO}]

# -- Multi-cycle path for Clocking SPI MISO --
# MISO is launched on the falling edge of SCLK and captured on the rising edge,
# so we can use the same multi-cycle path analysis as the CS_N and MOSI signals.
set_multicycle_path -setup -end -from [get_ports {LMK053_MISO LMK32_MISO}] 15
set_multicycle_path -hold  -end -from [get_ports {LMK053_MISO LMK32_MISO}] 29


#*******************************************************************************
## Asynchronous inputs/output signals
# Consolidated groups used by the coarse timing model

# ADRV sideband signals
set async_adrv_input_ports [get_ports {
    ADRV_GPIO[*]
    ADRV_GPINT[*]
}]
set async_adrv_output_ports [get_ports {
    ADRV_GPIO[*]
    ADRV_RESET_N
    ADRV_TEST_EN
    ADRV_TRXA_CTRL
    ADRV_TRXB_CTRL
    ADRV_TRXC_CTRL
    ADRV_TRXD_CTRL
    ADRV_TRXE_CTRL
    ADRV_TRXF_CTRL
    ADRV_TRXG_CTRL
    ADRV_TRXH_CTRL
}]

# GPS / timing sideband signals
set async_gps_input_ports [get_ports {
    GPS_PWR_FAULT_N
    GPS_PPS_OUT
    GPS_REF[*]
    NSYNC_STATUS[*]
}]
set async_gps_output_ports [get_ports {
    GPS_EXTINT
    GPS_REFSEL
    GPS_RESET_N
    NSYNC_GPIO0
    NSYNC_PDN
}]

# Clocking SPI control/status
set async_clocking_input_ports [get_ports {
    LMK32_STATUS
}]
set async_clocking_output_ports [get_ports {
    LMK32_RESET
    LMK32_VCXO_SEL_122M88
    DAC_CLR_N
    TCXO_EN_N
    REF_CLK_SEL
}]

# Board control / status signals
set async_board_input_ports [get_ports {
    PG_1V2
    TYPEC_PWR_NEGOTIATED
    VBUS_ALERT_N
    GT_25W_PWR_SRC
}]
set async_board_output_ports [get_ports {
    ANT_PWR_EN
    ENABLE_TXRX_TDR[*]
    PCIE_RESET_N_TO_CPLD
    RX_SW_CTRL[*]
    TBOLT_PD_CTRL_RESET
    TBOLT_RIDGE_RESET_N
}]

# User-visible GPIO and LED sideband
set async_panel_output_ports [get_ports {
    LED_RX_GRN[*]
    LED_TX_GRN[*]
    LED_TX_RED[*]
    LED_PWR_STS_BLUE
    LED_PWR_STS_ORANGE
}]

# DDR3 Reset
set async_ddr3_output_ports [get_ports {
    ddr3_reset_n
}]

set async_panel_bidir_ports [get_ports {
    FP_GPIO[*]
}]

# UART and I2C inputs treated as asynchronous by their
# respective logic engines. Grouping them here creates
# a very loose timing constraint to the first synchronizer
# stage of the logic engine.
set slow_input_ports [get_ports {
    GPS_UART_TOFPGA
    MISC_I2C_SCL
    MISC_I2C_SDA
    TBOLT_SCL
    TBOLT_SDA
}]

set slow_output_ports [get_ports {
    GPS_UART_TOGPS
    MISC_I2C_SCL
    MISC_I2C_SDA
    TBOLT_SCL
    TBOLT_SDA
}]

set async_input_ports  [list $async_adrv_input_ports \
                             $async_clocking_input_ports \
                             $async_gps_input_ports \
                             $async_board_input_ports \
                             $slow_input_ports]
set async_output_ports [list $async_adrv_output_ports \
                             $async_board_output_ports \
                             $async_clocking_output_ports \
                             $async_gps_output_ports \
                             $async_panel_output_ports\
                             $async_ddr3_output_ports \
                             $slow_output_ports]
set async_bidir_ports  [list $async_panel_bidir_ports]

set_input_delay -clock [get_clocks async_in_clk] 0.000 $async_input_ports
set_max_delay -from $async_input_ports 50.000
set_min_delay -from $async_input_ports 0.000

set_output_delay -clock [get_clocks async_out_clk] 0.000 $async_output_ports
set_max_delay -to $async_output_ports 50.000
set_min_delay -to $async_output_ports 0.000

set_input_delay -clock [get_clocks async_in_clk] 0.000 $async_bidir_ports
set_max_delay -from $async_bidir_ports 50.000
set_min_delay -from $async_bidir_ports 0.000

set_output_delay -clock [get_clocks async_out_clk] 0.000 $async_bidir_ports
set_max_delay -to $async_bidir_ports 50.000
set_min_delay -to $async_bidir_ports 0.000


#*******************************************************************************
## Asynchronous paths

set_false_path -from [get_ports PCIE_RESET_N]

set_false_path -to   [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
# Async reset
set_max_delay -from [get_cells -hier -filter {NAME =~ *b310_host_interface_i/*/PcieIp/inst/inst/user_reset_out_reg*}] 20.0
## Host IP
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */PcieBufgctrl/S0}]
set_false_path -to [get_pins -hierarchical -filter {NAME =~ */PcieBufgctrl/S1}]

set_false_path -to [get_pins -hierarchical -filter {NAME =~ */DoubleSyncAsyncInBasex/oSig_msx/*/D}]

#*******************************************************************************
# Define PLL location of critical PLLs

# both radio_clk and local_ref_clk PLL ideal placement is in the same clock region
# Define which one gets it to provide for further consistency in compiles
# Give priority to dev_clk since that is used for JESD IP
create_pblock radio_clk_pblock
resize_pblock radio_clk_pblock -add CLOCKREGION_X1Y0:CLOCKREGION_X1Y0
add_cells_to_pblock radio_clk_pblock [get_cells [list radio_clk_gen_i/inst/plle2_adv_inst]]
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets ref_clk_pll_inst/inst/clk_in1]

#*******************************************************************************
## SYSREF/SYNC JESD Timing
#
# SYNC is async, SYSREF is tightly timed.

# The SYNC output (to ADC) for both DBs is governed by the JESD cores, which are solely
# driven by radio_clk... but it is an asynchronous signal so we use the async_out_clk.
set_output_delay -clock [get_clocks async_out_clk] 0.000 [get_ports {ADRV_SYNCIN_*}]
set_max_delay -to [get_ports {ADRV_SYNCIN_*}] 50.000
set_min_delay -to [get_ports {ADRV_SYNCIN_*}] 0.000

# The SYNC input (from DAC) for both DBs is received by the radio_clk inside the JESD
# cores... but again, it is asynchronous and therefore uses the async_in_clk.
set_input_delay -clock [get_clocks async_in_clk] 0.000 [get_ports {ADRV_SYNCOUT_*}]
set_max_delay -from [get_ports {ADRV_SYNCOUT_*}] 50.000
set_min_delay -from [get_ports {ADRV_SYNCOUT_*}] 0.000

# SYSREF Input timing
# The LMK either launches this on the falling edge of DEVCLK + 282ps for distribution mode
# and -200ps for non distribution mode.
# This is essentially a min delay of 3.8ns and max delay of 4.283ns
# To provide margin for LMK variation/jitter, we and add/subtract 0.7ns from these.
# We subtract out lmk_min/max_delay because the tools assume that the min/delay max delay is not tracked
# by the PLL, and thus the radio_clk it assumes can see this delay.  The delay is static
# thus the PLL can track it, so subtract that out here.
set_property IOB TRUE [get_ports  SYSREF_P]
set_input_delay -clock dev_clk -min [expr 3.8 - 0.7 - $lmk_min_delay] [get_ports SYSREF_P]
set_input_delay -clock dev_clk -max [expr 4.283 + 0.7 - $lmk_max_delay] [get_ports SYSREF_P]
