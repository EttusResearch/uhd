#
# Copyright 2014 Ettus Research LLC
#

set_property PACKAGE_PIN   R8       [get_ports XG_CLK_p]
set_property PACKAGE_PIN   R7       [get_ports XG_CLK_n]

#IOSTANDARD not required because this is a GT terminal
#set_property IOSTANDARD    LVDS_25  [get_ports {XG_CLK_*}]

create_clock -name XG_CLK -period 6.400 -waveform {0.000 3.200} [get_ports XG_CLK_p]

set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks XG_CLK]
set_clock_groups -asynchronous -group [get_clocks bus_clk_div2] -group [get_clocks XG_CLK]
set_clock_groups -asynchronous -group [get_clocks -filter {NAME =~ *sfpp_io_*/ten_gige_phy_i/ten_gig_eth_pcs_pma_i/*/gtxe2_i/RXOUTCLK}] -group [get_clocks XG_CLK]
set_clock_groups -asynchronous -group [get_clocks -filter {NAME =~ *sfpp_io_*/ten_gige_phy_i/ten_gig_eth_pcs_pma_i/*/gtxe2_i/TXOUTCLK}] -group [get_clocks XG_CLK]

set_false_path -to [get_pins -of_objects [get_cells -hier -filter {NAME =~ *sfpp_io_*/ten_gige_phy_i/*sync1_r_reg*}] -filter {NAME =~ *PRE}]
set_false_path -to [get_pins -of_objects [get_cells -hier -filter {NAME =~ *sfpp_io_*/ten_gige_phy_i/*sync1_r_reg*}] -filter {NAME =~ *CLR}]
