#
# Copyright 2014 Ettus Research LLC
#

set_property PACKAGE_PIN   L8       [get_ports ETH_CLK_p]
set_property PACKAGE_PIN   L7       [get_ports ETH_CLK_n]

#IOSTANDARD not required because this is a GT terminal
#set_property IOSTANDARD    LVDS_25  [get_ports {ETH_CLK_*}]

create_clock -name ETH_CLK -period 8.000 -waveform {0.000 4.000} [get_ports ETH_CLK_p]

set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks ETH_CLK]
set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks -of_objects [get_pins *sfpp_io_*/one_gige_phy_i/*/core_clocking_i/mmcm_*/CLKOUT0]]
set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks -of_objects [get_pins *sfpp_io_*/one_gige_phy_i/*/core_clocking_i/mmcm_*/CLKOUT1]]

set_false_path -to [get_pins -hier -filter {NAME =~ *sfpp_io_*/one_gige_phy_i/*reset_sync*/PRE}]
set_false_path -to [get_pins -hier -filter {NAME =~ *sfpp_io_*/one_gige_phy_i/*/pma_reset_pipe_reg*/PRE}]
set_false_path -to [get_pins -hier -filter {NAME =~ *sfpp_io_*/one_gige_phy_i/*/pma_reset_pipe*[0]/D}]