#
# Copyright 2014 Ettus Research LLC
#

#*******************************************************************************
## Primary clock definitions

# Define clocks
create_clock -name FPGA_CLK            -period   5.000 -waveform {0.000 2.500}   [get_ports FPGA_CLK_p]
create_clock -name FPGA_REFCLK         -period  32.563 -waveform {0.000 16.281}  [get_ports FPGA_REFCLK_10MHz_p]
create_clock -name FPGA_125MHz_CLK     -period   8.000 -waveform {0.000 4.000}   [get_ports FPGA_125MHz_CLK]
create_clock -name DB0_ADC_DCLK        -period   5.000 -waveform {0.000 2.500}   [get_ports DB0_ADC_DCLK_P]
create_clock -name DB1_ADC_DCLK        -period   5.000 -waveform {0.000 2.500}   [get_ports DB1_ADC_DCLK_P]
create_clock -name IoRxClock           -period   4.000 -waveform {0.000 2.000}   [get_ports IoRxClock]
# Create virtual clock aligned with FPGA_CLK that is twice the frequency for DAC IO Timing. 
create_clock -name VIRT_DAC_CLK        -period   2.500 -waveform {0.000 1.250}

# Set clock properties
set_input_jitter [get_clocks FPGA_CLK] 0.05

set var_fpga_clk_delay  1.545    ;# LMK_Delay=0.900ns, LMK->FPGA=0.645ns
set var_fpga_clk_skew   0.100
set_clock_latency -source -early [expr $var_fpga_clk_delay - $var_fpga_clk_skew/2] [get_clocks FPGA_CLK]
set_clock_latency -source -late  [expr $var_fpga_clk_delay + $var_fpga_clk_skew/2] [get_clocks FPGA_CLK]

set var_adc_clk_delay   8.440    ;# LMK->ADC=1.04ns, ADC->FPGA=0.750ns, ADC=6.65ns=(0.69*5ns)+5.7-2.5 
set var_adc_clk_skew    0.500    ;# The real skew is ~3.5ns with which we will not meet static timing. Ignore skew by temp variations.
set_clock_latency -source -early [expr $var_adc_clk_delay - $var_adc_clk_skew/2] [get_clocks DB0_ADC_DCLK]
set_clock_latency -source -late  [expr $var_adc_clk_delay + $var_adc_clk_skew/2] [get_clocks DB0_ADC_DCLK]
set_clock_latency -source -early [expr $var_adc_clk_delay - $var_adc_clk_skew/2] [get_clocks DB1_ADC_DCLK]
set_clock_latency -source -late  [expr $var_adc_clk_delay + $var_adc_clk_skew/2] [get_clocks DB1_ADC_DCLK]

# FPGA_CLK_p/n is externally phase shifted to allow for crossing from the ADC clock domain
# to the radio_clk (aka FPGA_CLK_p/n) clock domain. To ensure this timing is consistent,
# lock the locations of the MMCM and BUFG to generate radio_clk.
set_property LOC MMCME2_ADV_X0Y0 [get_cells -hierarchical -filter {NAME =~ "*radio_clk_gen/*mmcm_adv_inst"}]
set_property LOC BUFGCTRL_X0Y8   [get_cells -hierarchical -filter {NAME =~ "*radio_clk_gen/*clkout1_buf"}]

# The PCIe specific 40MHz and 200MHz clocks are only active in clock regious X0Y0 and X1Y0 so we use BUFHs
# to distribute them. To do so, we have to use a PLL because the MMCM in that region is used by radio_clk_gen
# Since that MMCM is LOC constrained, we must LOC constrain this PLL as well.
set_property LOC PLLE2_ADV_X0Y0 [get_cells -hierarchical -filter {NAME =~ "*pcie_clk_gen/*plle2_adv_inst"}]


#*******************************************************************************
## Generated clock definitions

create_generated_clock -name DB0_DAC_DCI  -source [get_pins gen_db0/oddr_clk/C] -divide_by 1 [get_ports DB0_DAC_DCI_P]
create_generated_clock -name DB1_DAC_DCI  -source [get_pins gen_db1/oddr_clk/C] -divide_by 1 [get_ports DB1_DAC_DCI_P]
create_generated_clock -name IoTxClock -multiply_by 1                                                               \
                       -source [get_pins -hier -filter {NAME =~ lvfpga_chinch_inst/*/TxClockGenx/TxUseMmcm.TxMmcm/CLKOUT0}] \
                       [get_ports {IoTxClock}]


#*******************************************************************************
## Aliases for auto-generated clocks

create_generated_clock -name radio_clk                [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen/*/CLKOUT0"}]
create_generated_clock -name radio_clk_2x             [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen/*/CLKOUT1"}]
#create_generated_clock -name dac_dci_clk              [get_pins -hierarchical -filter {NAME =~ "*radio_clk_gen/*/CLKOUT2"}]
create_generated_clock -name bus_clk                  [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen/*/CLKOUT0"}]
create_generated_clock -name bus_clk_div2             [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen/*/CLKOUT2"}]
create_generated_clock -name ce_clk                   [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen/*/CLKOUT3"}]
create_generated_clock -name ioport2_clk              [get_pins -hierarchical -filter {NAME =~ "*bus_clk_gen/*/CLKFBOUT"}]
create_generated_clock -name rio40_clk                [get_pins -hierarchical -filter {NAME =~ "*pcie_clk_gen/*/CLKOUT0"}]
create_generated_clock -name ioport2_idelay_ref_clk   [get_pins -hierarchical -filter {NAME =~ "*pcie_clk_gen/*/CLKOUT1"}]


#*******************************************************************************
## Asynchronous clock groups

set_clock_groups -asynchronous -group [get_clocks bus_clk]      -group [get_clocks ioport2_clk]
set_clock_groups -asynchronous -group [get_clocks ioport2_clk]  -group [get_clocks rio40_clk]
set_clock_groups -asynchronous -group [get_clocks bus_clk]      -group [get_clocks radio_clk]
set_clock_groups -asynchronous -group [get_clocks bus_clk_div2] -group [get_clocks radio_clk]
set_clock_groups -asynchronous -group [get_clocks ioport2_clk]  -group [get_clocks IoPort2Wrapperx/RxLowSpeedClk]
set_clock_groups -asynchronous -group [get_clocks bus_clk]      -group [get_clocks FPGA_REFCLK]
set_clock_groups -asynchronous -group [get_clocks ce_clk]       -group [get_clocks bus_clk]
set_clock_groups -asynchronous -group [get_clocks ce_clk]       -group [get_clocks radio_clk]


#*******************************************************************************
## ADC Interface

# At 200 MHz, static timing cannot be closed so we tune data delays on the capture
# interface from software at device creation time.
# The data is center aligned wrt to the SS Clock when it is launched from the ADC
# So we tune the data IDELAYS to half the range (16) so we have slack in both directions
# In the constraints we capture this by padding the dv_before and dv_after by half the
# tuning range of the IDELAY.

# Using typical values for ADC
set adc_clk_delay_wrt_center    0.000      ;# Possible {-1.340, -0.769, 0, 0.769}
set adc_in_dv_before_clk_edge   0.550      ;# Typical: 0.90ns
set adc_in_dv_after_clk_edge    0.550      ;# Typical: 0.95ns
set idelay_tune_range           2.500      ;# Refclk for IDELAY is 200MHz. Range of idelay is 0.5*period

set adc_in_delay_max [expr 2.500 - $adc_in_dv_before_clk_edge - $idelay_tune_range + $adc_clk_delay_wrt_center]
set adc_in_delay_min [expr $adc_in_dv_after_clk_edge + $idelay_tune_range - $adc_clk_delay_wrt_center]

set_input_delay -clock DB0_ADC_DCLK -max $adc_in_delay_max                         [get_ports {DB0_ADC_DA*}]
set_input_delay -clock DB0_ADC_DCLK -min $adc_in_delay_min                         [get_ports {DB0_ADC_DA*}]
set_input_delay -clock DB0_ADC_DCLK -max $adc_in_delay_max -clock_fall -add_delay  [get_ports {DB0_ADC_DA*}]
set_input_delay -clock DB0_ADC_DCLK -min $adc_in_delay_min -clock_fall -add_delay  [get_ports {DB0_ADC_DA*}]

set_input_delay -clock DB0_ADC_DCLK -max $adc_in_delay_max                         [get_ports {DB0_ADC_DB*}]
set_input_delay -clock DB0_ADC_DCLK -min $adc_in_delay_min                         [get_ports {DB0_ADC_DB*}]
set_input_delay -clock DB0_ADC_DCLK -max $adc_in_delay_max -clock_fall -add_delay  [get_ports {DB0_ADC_DB*}]
set_input_delay -clock DB0_ADC_DCLK -min $adc_in_delay_min -clock_fall -add_delay  [get_ports {DB0_ADC_DB*}]

set_input_delay -clock DB1_ADC_DCLK -max $adc_in_delay_max                         [get_ports {DB1_ADC_DA*}]
set_input_delay -clock DB1_ADC_DCLK -min $adc_in_delay_min                         [get_ports {DB1_ADC_DA*}]
set_input_delay -clock DB1_ADC_DCLK -max $adc_in_delay_max -clock_fall -add_delay  [get_ports {DB1_ADC_DA*}]
set_input_delay -clock DB1_ADC_DCLK -min $adc_in_delay_min -clock_fall -add_delay  [get_ports {DB1_ADC_DA*}]

set_input_delay -clock DB1_ADC_DCLK -max $adc_in_delay_max                         [get_ports {DB1_ADC_DB*}]
set_input_delay -clock DB1_ADC_DCLK -min $adc_in_delay_min                         [get_ports {DB1_ADC_DB*}]
set_input_delay -clock DB1_ADC_DCLK -max $adc_in_delay_max -clock_fall -add_delay  [get_ports {DB1_ADC_DB*}]
set_input_delay -clock DB1_ADC_DCLK -min $adc_in_delay_min -clock_fall -add_delay  [get_ports {DB1_ADC_DB*}]

# We use a simple synchronizer to cross ADC data over from the ADC_CLK domain to the radio_clk domain
# Use max delay constraints to ensure that the transition happens safely
set_min_delay                0.700 -from [get_cells {cap_db0/gen_lvds_pins[*].iddr_i}] \
                                   -to   [get_cells {cap_db0/adc_data_rclk_reg*[*]}]
set_min_delay                0.700 -from [get_cells {cap_db1/gen_lvds_pins[*].iddr_i}] \
                                   -to   [get_cells {cap_db1/adc_data_rclk_reg*[*]}]
set_max_delay -datapath_only 0.950 -from [get_cells {cap_db0/gen_lvds_pins[*].iddr_i}] \
                                   -to   [get_cells {cap_db0/adc_data_rclk_reg*[*]}]
set_max_delay -datapath_only 0.950 -from [get_cells {cap_db1/gen_lvds_pins[*].iddr_i}] \
                                   -to   [get_cells {cap_db1/adc_data_rclk_reg*[*]}]

# We also need to location constrain the first flops in the synchronizer to help the tools
# meet timing reliably

# ADC0
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[0]}]
set_property LOC SLICE_X1Y192   [get_cells {cap_db0/adc_data_rclk_reg*[0]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[1]}]
set_property LOC SLICE_X1Y192   [get_cells {cap_db0/adc_data_rclk_reg*[1]}]
set_property BEL AFF            [get_cells {cap_db0/adc_data_rclk_reg*[2]}]
set_property LOC SLICE_X1Y190   [get_cells {cap_db0/adc_data_rclk_reg*[2]}]
set_property BEL BFF            [get_cells {cap_db0/adc_data_rclk_reg*[3]}]
set_property LOC SLICE_X1Y190   [get_cells {cap_db0/adc_data_rclk_reg*[3]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[4]}]
set_property LOC SLICE_X1Y188   [get_cells {cap_db0/adc_data_rclk_reg*[4]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[5]}]
set_property LOC SLICE_X1Y188   [get_cells {cap_db0/adc_data_rclk_reg*[5]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[6]}]
set_property LOC SLICE_X1Y186   [get_cells {cap_db0/adc_data_rclk_reg*[6]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[7]}]
set_property LOC SLICE_X1Y186   [get_cells {cap_db0/adc_data_rclk_reg*[7]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[8]}]
set_property LOC SLICE_X1Y184   [get_cells {cap_db0/adc_data_rclk_reg*[8]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[9]}]
set_property LOC SLICE_X1Y184   [get_cells {cap_db0/adc_data_rclk_reg*[9]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[10]}]
set_property LOC SLICE_X1Y182   [get_cells {cap_db0/adc_data_rclk_reg*[10]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[11]}]
set_property LOC SLICE_X1Y182   [get_cells {cap_db0/adc_data_rclk_reg*[11]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[12]}]
set_property LOC SLICE_X1Y180   [get_cells {cap_db0/adc_data_rclk_reg*[12]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[13]}]
set_property LOC SLICE_X1Y180   [get_cells {cap_db0/adc_data_rclk_reg*[13]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[14]}]
set_property LOC SLICE_X1Y178   [get_cells {cap_db0/adc_data_rclk_reg*[14]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[15]}]
set_property LOC SLICE_X1Y178   [get_cells {cap_db0/adc_data_rclk_reg*[15]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[16]}]
set_property LOC SLICE_X1Y174   [get_cells {cap_db0/adc_data_rclk_reg*[16]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[17]}]
set_property LOC SLICE_X1Y174   [get_cells {cap_db0/adc_data_rclk_reg*[17]}]
set_property BEL AFF            [get_cells {cap_db0/adc_data_rclk_reg*[18]}]
set_property LOC SLICE_X1Y172   [get_cells {cap_db0/adc_data_rclk_reg*[18]}]
set_property BEL BFF            [get_cells {cap_db0/adc_data_rclk_reg*[19]}]
set_property LOC SLICE_X1Y172   [get_cells {cap_db0/adc_data_rclk_reg*[19]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[20]}]
set_property LOC SLICE_X1Y218   [get_cells {cap_db0/adc_data_rclk_reg*[20]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[21]}]
set_property LOC SLICE_X1Y218   [get_cells {cap_db0/adc_data_rclk_reg*[21]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[22]}]
set_property LOC SLICE_X1Y198   [get_cells {cap_db0/adc_data_rclk_reg*[22]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[23]}]
set_property LOC SLICE_X1Y198   [get_cells {cap_db0/adc_data_rclk_reg*[23]}]
set_property BEL AFF            [get_cells {cap_db0/adc_data_rclk_reg*[24]}]
set_property LOC SLICE_X1Y196   [get_cells {cap_db0/adc_data_rclk_reg*[24]}]
set_property BEL BFF            [get_cells {cap_db0/adc_data_rclk_reg*[25]}]
set_property LOC SLICE_X1Y196   [get_cells {cap_db0/adc_data_rclk_reg*[25]}]
set_property BEL A5FF           [get_cells {cap_db0/adc_data_rclk_reg*[26]}]
set_property LOC SLICE_X1Y194   [get_cells {cap_db0/adc_data_rclk_reg*[26]}]
set_property BEL B5FF           [get_cells {cap_db0/adc_data_rclk_reg*[27]}]
set_property LOC SLICE_X1Y194   [get_cells {cap_db0/adc_data_rclk_reg*[27]}]

# ADC1
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[0]}]
set_property LOC SLICE_X1Y298   [get_cells {cap_db1/adc_data_rclk_reg*[0]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[1]}]
set_property LOC SLICE_X1Y298   [get_cells {cap_db1/adc_data_rclk_reg*[1]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[2]}]
set_property LOC SLICE_X1Y284   [get_cells {cap_db1/adc_data_rclk_reg*[2]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[3]}]
set_property LOC SLICE_X1Y284   [get_cells {cap_db1/adc_data_rclk_reg*[3]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[4]}]
set_property LOC SLICE_X1Y288   [get_cells {cap_db1/adc_data_rclk_reg*[4]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[5]}]
set_property LOC SLICE_X1Y288   [get_cells {cap_db1/adc_data_rclk_reg*[5]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[6]}]
set_property LOC SLICE_X1Y282   [get_cells {cap_db1/adc_data_rclk_reg*[6]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[7]}]
set_property LOC SLICE_X1Y282   [get_cells {cap_db1/adc_data_rclk_reg*[7]}]
set_property BEL AFF            [get_cells {cap_db1/adc_data_rclk_reg*[8]}]
set_property LOC SLICE_X1Y296   [get_cells {cap_db1/adc_data_rclk_reg*[8]}]
set_property BEL BFF            [get_cells {cap_db1/adc_data_rclk_reg*[9]}]
set_property LOC SLICE_X1Y296   [get_cells {cap_db1/adc_data_rclk_reg*[9]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[10]}]
set_property LOC SLICE_X1Y280   [get_cells {cap_db1/adc_data_rclk_reg*[10]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[11]}]
set_property LOC SLICE_X1Y280   [get_cells {cap_db1/adc_data_rclk_reg*[11]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[12]}]
set_property LOC SLICE_X1Y286   [get_cells {cap_db1/adc_data_rclk_reg*[12]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[13]}]
set_property LOC SLICE_X1Y286   [get_cells {cap_db1/adc_data_rclk_reg*[13]}]
set_property BEL AFF            [get_cells {cap_db1/adc_data_rclk_reg*[14]}]
set_property LOC SLICE_X1Y274   [get_cells {cap_db1/adc_data_rclk_reg*[14]}]
set_property BEL BFF            [get_cells {cap_db1/adc_data_rclk_reg*[15]}]
set_property LOC SLICE_X1Y274   [get_cells {cap_db1/adc_data_rclk_reg*[15]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[16]}]
set_property LOC SLICE_X1Y272   [get_cells {cap_db1/adc_data_rclk_reg*[16]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[17]}]
set_property LOC SLICE_X1Y272   [get_cells {cap_db1/adc_data_rclk_reg*[17]}]
set_property BEL AFF            [get_cells {cap_db1/adc_data_rclk_reg*[18]}]
set_property LOC SLICE_X1Y290   [get_cells {cap_db1/adc_data_rclk_reg*[18]}]
set_property BEL BFF            [get_cells {cap_db1/adc_data_rclk_reg*[19]}]
set_property LOC SLICE_X1Y290   [get_cells {cap_db1/adc_data_rclk_reg*[19]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[20]}]
set_property LOC SLICE_X1Y342   [get_cells {cap_db1/adc_data_rclk_reg*[20]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[21]}]
set_property LOC SLICE_X1Y342   [get_cells {cap_db1/adc_data_rclk_reg*[21]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[22]}]
set_property LOC SLICE_X1Y294   [get_cells {cap_db1/adc_data_rclk_reg*[22]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[23]}]
set_property LOC SLICE_X1Y294   [get_cells {cap_db1/adc_data_rclk_reg*[23]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[24]}]
set_property LOC SLICE_X1Y268   [get_cells {cap_db1/adc_data_rclk_reg*[24]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[25]}]
set_property LOC SLICE_X1Y268   [get_cells {cap_db1/adc_data_rclk_reg*[25]}]
set_property BEL A5FF           [get_cells {cap_db1/adc_data_rclk_reg*[26]}]
set_property LOC SLICE_X1Y292   [get_cells {cap_db1/adc_data_rclk_reg*[26]}]
set_property BEL B5FF           [get_cells {cap_db1/adc_data_rclk_reg*[27]}]
set_property LOC SLICE_X1Y292   [get_cells {cap_db1/adc_data_rclk_reg*[27]}]

# IODELAY constraints
set_property IODELAY_GROUP ADC_CAP_IODELAY_GRP [get_cells adc_cap_idelayctrl_i]
set_property IODELAY_GROUP ADC_CAP_IODELAY_GRP [get_cells {cap_db0/gen_lvds_pins[*].idelay_i}]
set_property IODELAY_GROUP ADC_CAP_IODELAY_GRP [get_cells {cap_db1/gen_lvds_pins[*].idelay_i}]


#*******************************************************************************
## DAC Interface

# DCI System-Sync Timing

# The DCI clock driven to the DACs must obey setup and hold timing with respect to
# the reference clock driven to the DACs (same as the FPGA_CLK, driven by the LMK).
# Define the minimum and maximum clock propagation delays through the FPGA in order to
# meet this system-wide timing.
set dac0_clk_offset_out_max  1.350
set dac0_clk_offset_out_min  0.225
set dac1_clk_offset_out_max  1.350
set dac1_clk_offset_out_min  0.225

# The absolute latest the DCI clock should change is the sum of the maximum delay through
# the FPGA and the latest the sourcing clock (FPGA_CLK) can arrive at the FPGA. This is an
# artifact of the set_clock_latency constraints and doing system-wide timing. Typically,
# these Early/Late delays are automatically compensated for by the analyzer. However this
# is only the case for signals that start and end in the same PRIMARY clock domain. In
# our case, VIRT_DAC_CLK and radio_clk are not the same clock domain and
# therefore we have to manually remove the added Early/Late values from analysis.
set dac0_dci_out_delay_max [expr $dac0_clk_offset_out_max + $var_fpga_clk_delay + $var_fpga_clk_skew/2]
set dac0_dci_out_delay_min [expr $dac0_clk_offset_out_min + $var_fpga_clk_delay - $var_fpga_clk_skew/2]
set dac1_dci_out_delay_max [expr $dac1_clk_offset_out_max + $var_fpga_clk_delay + $var_fpga_clk_skew/2]
set dac1_dci_out_delay_min [expr $dac1_clk_offset_out_min + $var_fpga_clk_delay - $var_fpga_clk_skew/2]

# The min set_output_delay is the earliest the DCI clock should change BEFORE the current
# edge of interest. Here it is inverted (negated) because the earliest the clock should
# change is dac0_dci_out_delay_min AFTER the launch edge of the virtual clock.
set_output_delay -clock VIRT_DAC_CLK -min [expr - $dac0_dci_out_delay_min]                        [get_ports {DB0_DAC_DCI_*}]
set_output_delay -clock VIRT_DAC_CLK -min [expr - $dac0_dci_out_delay_min] -clock_fall -add_delay [get_ports {DB0_DAC_DCI_*}] 
set_output_delay -clock VIRT_DAC_CLK -min [expr - $dac1_dci_out_delay_min]                        [get_ports {DB1_DAC_DCI_*}]
set_output_delay -clock VIRT_DAC_CLK -min [expr - $dac1_dci_out_delay_min] -clock_fall -add_delay [get_ports {DB1_DAC_DCI_*}] 

# The max set_output_delay is the time the data should be stable before the next
# edge of interest. Since we are DDR, this is the falling edge. Hence we subtract
# latest time the data should change, dac0_dci_out_delay_max, from the falling edge
# time, dci_period/2 = 1.25ns.
set_output_delay -clock VIRT_DAC_CLK -max [expr 1.25 - $dac0_dci_out_delay_max]                         [get_ports {DB0_DAC_DCI_*}]
set_output_delay -clock VIRT_DAC_CLK -max [expr 1.25 - $dac0_dci_out_delay_max] -clock_fall -add_delay  [get_ports {DB0_DAC_DCI_*}] 
set_output_delay -clock VIRT_DAC_CLK -max [expr 1.25 - $dac1_dci_out_delay_max]                         [get_ports {DB1_DAC_DCI_*}]
set_output_delay -clock VIRT_DAC_CLK -max [expr 1.25 - $dac1_dci_out_delay_max] -clock_fall -add_delay  [get_ports {DB1_DAC_DCI_*}]


# Data to DCI Source-Sync Timing

# The data setup and hold values must be modified in order to pass timing in
# the FPGA. The correct values are 0.270 and 0.090 for setup and hold, respectively.
# The interface fails by around 390 ps in both directions, so we subtract the failing
# amount from the actual amount to get a passing constraint.
# NOTE: Any changes to the adjustment margin below would need to be validated over
#       multiple builds, process and temperature. Try not to change it!
set dac_data_setup      0.270
set dac_data_hold       0.090
set dac_setup_adj       0.390
set dac_hold_adj        0.390

# These are real trace delays from the timing spreadsheet. Note that we are assuming
# no variability in our clock delay.
set dac0_data_delay_max 1.036
set dac0_data_delay_min 0.898
set dac0_clk_delay_max  0.974
set dac0_clk_delay_min  0.974

set dac1_data_delay_max 0.941
set dac1_data_delay_min 0.833
set dac1_clk_delay_max  0.930
set dac1_clk_delay_min  0.930

set dac0_out_delay_max [expr $dac0_data_delay_max - $dac0_clk_delay_min + $dac_data_setup - $dac_setup_adj]
set dac0_out_delay_min [expr $dac0_data_delay_min - $dac0_clk_delay_max - $dac_data_hold  + $dac_hold_adj]
set dac1_out_delay_max [expr $dac1_data_delay_max - $dac1_clk_delay_min + $dac_data_setup - $dac_setup_adj]
set dac1_out_delay_min [expr $dac1_data_delay_min - $dac1_clk_delay_max - $dac_data_hold  + $dac_hold_adj]

set_output_delay -clock [get_clocks DB0_DAC_DCI] -max $dac0_out_delay_max                        [get_ports -regexp {DB0_DAC_D._. DB0_DAC_FRAME_.}]
set_output_delay -clock [get_clocks DB0_DAC_DCI] -max $dac0_out_delay_max -clock_fall -add_delay [get_ports -regexp {DB0_DAC_D._. DB0_DAC_FRAME_.}]
set_output_delay -clock [get_clocks DB0_DAC_DCI] -min $dac0_out_delay_min                        [get_ports -regexp {DB0_DAC_D._. DB0_DAC_FRAME_.}]
set_output_delay -clock [get_clocks DB0_DAC_DCI] -min $dac0_out_delay_min -clock_fall -add_delay [get_ports -regexp {DB0_DAC_D._. DB0_DAC_FRAME_.}]

set_output_delay -clock [get_clocks DB1_DAC_DCI] -max $dac1_out_delay_max                        [get_ports -regexp {DB1_DAC_D._. DB1_DAC_FRAME_.}]
set_output_delay -clock [get_clocks DB1_DAC_DCI] -max $dac1_out_delay_max -clock_fall -add_delay [get_ports -regexp {DB1_DAC_D._. DB1_DAC_FRAME_.}]
set_output_delay -clock [get_clocks DB1_DAC_DCI] -min $dac1_out_delay_min                        [get_ports -regexp {DB1_DAC_D._. DB1_DAC_FRAME_.}]
set_output_delay -clock [get_clocks DB1_DAC_DCI] -min $dac1_out_delay_min -clock_fall -add_delay [get_ports -regexp {DB1_DAC_D._. DB1_DAC_FRAME_.}]


#*******************************************************************************
## IoPort2

# Constrain the location of the IDELAYCTERL associated with the interface trainer IDELAYs
set_property LOC IDELAYCTRL_X1Y0 [get_cells lvfpga_chinch_inst/IDELAYCTRLx]

# RX Pad Input constraints
set_input_delay -clock [get_clocks IoRxClock] -max 2.580                        [get_ports {irIoRx*}]
set_input_delay -clock [get_clocks IoRxClock] -min 2.280                        [get_ports {irIoRx*}]
set_input_delay -clock [get_clocks IoRxClock] -max 2.580 -clock_fall -add_delay [get_ports {irIoRx*}]
set_input_delay -clock [get_clocks IoRxClock] -min 2.280 -clock_fall -add_delay [get_ports {irIoRx*}]

# Note: The input clock N-Side ISERDES is not constrained for IO timing since
# adding an input delay does not work as the clock and data are the same.
# Since the architecture requires dedicated routes, the build-to-build
# variablilty will be zero and therefore, no separate timing constraint
# is necessary for the N-Side pin. The RxClock delay is constrained because
# of the input delay constraints on the rest of the bus. This path does, however,
# require a max delay constraint in order to override the default analysis:
set_max_delay -from [get_ports {IoRxClock*}]                                                       \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Wrapperx/RxClockGenx/RxClockSerdes*}] \
              2.0 -datapath_only

# TX Pad Output constraints
set_output_delay -clock [get_clocks IoTxClock] -max 1.600                        [get_ports {itIoTx*}]
set_output_delay -clock [get_clocks IoTxClock] -min 0.400                        [get_ports {itIoTx*}]
set_output_delay -clock [get_clocks IoTxClock] -max 1.600 -clock_fall -add_delay [get_ports {itIoTx*}]
set_output_delay -clock [get_clocks IoTxClock] -min 0.400 -clock_fall -add_delay [get_ports {itIoTx*}]

# These signals are all treated as async signals so no stringent timing requirements are needed.
set_max_delay -to [get_ports aIrq*]              10.000
set_max_delay -from [get_ports aIoResetIn_n]     10.000
set_max_delay -from [get_ports aIoReadyIn]       10.000
set_max_delay -to [get_ports aIoReadyOut]        10.000
set_max_delay -to [get_ports aIoPort2Restart]    10.000
set_false_path -from [get_ports aStc3Gpio7]

# Async reset
set_false_path -from [get_cells -hier -filter {NAME =~ lvfpga_chinch_inst/*StartupFsmx/aResetLcl*}]

# Double Sync
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Wrapperx/tIoResetSync/DoubleSyncBasex/iDlySig*}]                            \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Wrapperx/tIoResetSync/DoubleSyncBasex/DoubleSyncAsyncInBasex/oSig_ms*}]     \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Wrapperx/bIoResetAckSync/DoubleSyncBasex/iDlySig*}]                         \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Wrapperx/bIoResetAckSync/DoubleSyncBasex/DoubleSyncAsyncInBasex/oSig_ms*}]  \
              6.0 -datapath_only

# Constrains HandshakeSLVx and IClkToPushClkHs in ControlIoDelayClockCross
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/*iLclStoredData*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/*ODataFlop*}]      \
              8.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/iPushToggle}]           \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/BlkOut.oPushToggle0_ms*}] \
              4.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/BlkOut.oPushToggle0_ms*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/BlkOut.oPushToggle1*}]    \
              4.0

set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/*oPushToggleToReady*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/*iRdyPushToggle_ms*}]  \
              4.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/*iRdyPushToggle_ms*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/*ControlIoDelayClockCrossx/*/HBx/*iRdyPushToggle*}]    \
              4.0

# SamplerResultsHandshake and SamplerControlHandshake
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/*iLclStoredData*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/*ODataFlop*}]      \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/iPushToggle}]           \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/BlkOut.oPushToggle0_ms*}] \
              4.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/BlkOut.oPushToggle0_ms*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/BlkOut.oPushToggle1*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/*oPushToggleToReady*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/*iRdyPushToggle_ms*}]   \
              4.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/*iRdyPushToggle_ms*}]   \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/ClockSamplerBlock.Sampler*Handshake/HBx/*iRdyPushToggle*}]      \
              4.0

# Constrain PhyResetSync PulseSync
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/iHoldSigInx*}]     \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/oHoldSigIn_msx*}]  \
              4.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/oHoldSigIn_msx*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/oLocalSigOutCEx*}] \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/oLocalSigOutCEx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/iSigOut_msx*}]     \
              4.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/iSigOut_msx*}]     \
              -to   [get_cells -hier -filter {NAME =~ *IoPortClkDelayTrainerx/TrainerBlock.PhyResetSync/PulseSyncBasex/iSigOutx*}]        \
              4.0

# IoPort2 Core Clock Crossings
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoPort2Receiverx/PacketReceivedDoublesync*iDlySigx*}]               \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoPort2Receiverx/PacketReceivedDoublesync*DoubleSyncAsyncInBasex*}] \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoPort2Receiverx/PacketReceivedDoublesync*DoubleSyncAsyncInBasex/oSig_msx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoPort2Receiverx/PacketReceivedDoublesync*DoubleSyncAsyncInBasex/oSigx*}]    \
              6.0

# Handshake
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/*iLclStoredData*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/*ODataFlop*}]      \
              10.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/iPushToggle}]           \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/BlkOut.oPushToggle0_ms*}] \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/BlkOut.oPushToggle0_ms*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/BlkOut.oPushToggle1*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/*oPushToggleToReady*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/*iRdyPushToggle_ms*}]  \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/*iRdyPushToggle_ms*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/CreditManager*/HBx/*iRdyPushToggle*}]     \
              4.0

# FIFO Clock Crossings
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/FifoFlags/ieInputCountGrayx*}]    \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/FifoFlags/oInputCountGray_msx*}]  \
              5.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/FifoFlags/oInputCountGray_msx*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/FifoFlags/oInputCountGrayx*}]     \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/PacketFullyReceived/ieInputCountGrayx*}]   \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/PacketFullyReceived/oInputCountGray_msx*}] \
              5.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/PacketFullyReceived/oInputCountGray_msx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/ReceiveSide.IoReceiveFifoBasex/PacketFullyReceived/oInputCountGrayx*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/ieInputCountGrayx*}]    \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/oInputCountGray_msx*}]  \
              5.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/oInputCountGray_msx*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/oInputCountGrayx*}]     \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/oeOutputCountGrayx*}]   \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/iOutputCountGray_msx*}] \
              5.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/iOutputCountGray_msx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.PacketFullyReceived/iOutputCountGrayx*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/ieInputCountGrayx*}]    \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/oInputCountGray_msx*}]  \
              5.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/oInputCountGray_msx*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/oInputCountGrayx*}]     \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/oeOutputCountGrayx*}]   \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/iOutputCountGray_msx*}] \
              5.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/iOutputCountGray_msx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/iOutputCountGrayx*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo.InputFifo.FifoFlags/oeOutputCountGrayx*}]   \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/TransmitFifo*DualPortRAMx*oDlyAddr*}]                    \
              5.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/*iLclStoredData*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/*ODataFlop*}]      \
              10.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/iPushToggle}]           \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/BlkOut.oPushToggle0_ms*}] \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/BlkOut.oPushToggle0_ms*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/BlkOut.oPushToggle1*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/*oPushToggleToReady*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/*iRdyPushToggle_ms*}]  \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/*iRdyPushToggle_ms*}]  \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/TransmitSide.IoTransmitFifox/CreditManager.HandshakeCredits/HBx/*iRdyPushToggle*}]     \
              4.0

# Double Sync
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/Startup.DoubleSyncEnableTransmit/iDlySigx*}]                         \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/Startup.DoubleSyncEnableTransmit/*DoubleSyncAsyncInBasex/oSig_msx*}] \
              6.0 -datapath_only
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/Startup.DoubleSyncEnableTransmit/*DoubleSyncAsyncInBasex/oSig_msx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2x/IoPort2Basex/Startup.DoubleSyncEnableTransmit/*DoubleSyncAsyncInBasex/oSigx*}]    \
              4.0
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/DoubleSyncWidePortMode.DoubleSync*WidePortMode/iDlySigx*}]               \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/DoubleSyncWidePortMode.DoubleSync*WidePortMode/DoubleSyncAsyncInBasex*}] \
              6.0 -datapath_only -quiet
set_max_delay -from [get_cells -hier -filter {NAME =~ *IoPort2Basex/DoubleSyncWidePortMode.DoubleSync*WidePortMode/DoubleSyncAsyncInBasex/oSig_msx*}] \
              -to   [get_cells -hier -filter {NAME =~ *IoPort2Basex/DoubleSyncWidePortMode.DoubleSync*WidePortMode/DoubleSyncAsyncInBasex/oSigx*}]    \
              5.0 -quiet


#*******************************************************************************
## PPS Timing

# Constrain delay from PPS input pins to the first stage synchronizer flip-flop
set_max_delay 5.000 -from [get_ports EXT_PPS_IN] \
                    -to [get_pins -hier -filter {NAME =~ */pps_sync_refclk_inst/synchronizer_constrained/stages[0].value_reg[*]/D}] \
                    -datapath_only
set_min_delay 2.500 -from [get_ports EXT_PPS_IN] \
                    -to [get_pins -hier -filter {NAME =~ */pps_sync_refclk_inst/synchronizer_constrained/stages[0].value_reg[*]/D}]
set_max_delay 5.000 -from [get_ports GPS_PPS_OUT] \
                    -to [get_pins -hier -filter {NAME =~ */pps_sync_refclk_inst/synchronizer_constrained/stages[0].value_reg[*]/D}] \
                    -datapath_only
set_min_delay 2.500 -from [get_ports GPS_PPS_OUT] \
                    -to [get_pins -hier -filter {NAME =~ */pps_sync_refclk_inst/synchronizer_constrained/stages[0].value_reg[*]/D}]

# Constrain input-output delay for external PPS
set_max_delay 10.000 -from [get_ports EXT_PPS_IN] -to [get_ports {EXT_PPS_OUT}] -datapath_only
set_min_delay 5.000  -from [get_ports EXT_PPS_IN] -to [get_ports {EXT_PPS_OUT}]

# Constrain delay to the first flop in radio_clk with about 1ns of slack
set_max_delay 6.500 -to [get_pins -hier -filter {NAME =~ */pps_sync_tbclk_inst/synchronizer_constrained/stages[0].value_reg[*]/D}]
set_min_delay 0.500 -to [get_pins -hier -filter {NAME =~ */pps_sync_tbclk_inst/synchronizer_constrained/stages[0].value_reg[*]/D}]

#*******************************************************************************
## Miscellaneous Interfaces

# Dboard and Front-Panel GPIO Interfaces 
# We force the registers closest to the PADs into the IOB to achieve lowest skew between individual bits
# in the parallel bus. However, as a sanity check we add the following constraints that will fail if the 
# registers don't get placed in the IOB for whatever reason.
set_max_delay 6.000 -to   [get_ports * -filter {(DIRECTION == OUT || DIRECTION == INOUT) && NAME =~ "DB*_*X_IO*"}]
set_max_delay 3.000 -from [get_ports * -filter {(DIRECTION == IN  || DIRECTION == INOUT) && NAME =~ "DB*_*X_IO*"}]
set_max_delay 6.000 -to   [get_ports * -filter {(DIRECTION == OUT || DIRECTION == INOUT) && NAME =~ "FrontPanelGpio[*]"}]
set_max_delay 3.000 -from [get_ports * -filter {(DIRECTION == IN  || DIRECTION == INOUT) && NAME =~ "FrontPanelGpio[*]"}]

# SPI Lines
set_max_delay 10.000 -datapath_only \
                     -from [get_ports {DB*_*X*MISO*}] 
set_max_delay 10.000 -to   [get_ports {DB*_*SCLK DB*_*SEN DB*_*MOSI}]
set_max_delay 10.000 -to   [get_ports {DB_SCL DB_SDA DB0_DAC_ENABLE DB1_DAC_ENABLE DB_ADC_RESET DB_DAC_RESET}]
set_max_delay 10.000 -from [get_ports {DB_SCL DB_SDA DB_DAC_MOSI}]

# Clock distribution chip control
set_max_delay -from   [get_ports {LMK_Status[*] LMK_Holdover LMK_Lock LMK_Sync}] 10.000
set_max_delay -to     [get_ports {LMK_SEN LMK_MOSI LMK_SCLK}]                    10.000
set_max_delay -to     [get_ports {ClockRefSelect*}]                              10.000
set_max_delay -to     [get_ports {TCXO_ENA}]                                     10.000

# GPS UART
set_max_delay -from   [get_ports {GPS_SER_OUT}] 6.000
set_max_delay -to     [get_ports {GPS_SER_IN}]  6.000
set_max_delay -from   [get_ports {GPS_LOCK_OK}] 25.000

# Reset paths
# All asynchronous resets must be held for at least 20ns
# which is 2+2 radio_clk cycles @200MHz or 2+2 bus_clk cycles @166MHz
set_max_delay -to [get_pins {int_reset_sync/reset_int*/PRE}]        12.000
set_max_delay -to [get_pins {int_div2_reset_sync/reset_int*/PRE}]   12.000
set_max_delay -to [get_pins {ce_reset_sync/reset_int*/PRE}]         12.000
set_max_delay -to [get_pins {radio_reset_sync/reset_int*/PRE}]      10.000

#*******************************************************************************
## Asynchronous paths

set_false_path -to   [get_pins -hierarchical -filter {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
set_false_path -to   [get_ports LED_*]
set_false_path -to   [get_ports {SFPP*_RS0 SFPP*_RS1 SFPP*_SCL SFPP*_SDA SFPP*_TxDisable}]
set_false_path -from [get_ports {SFPP*_ModAbs SFPP*_RxLOS SFPP*_SCL SFPP*_SDA SFPP*_TxFault}]
set_false_path -to   [get_ports GPSDO_PWR_ENA]
