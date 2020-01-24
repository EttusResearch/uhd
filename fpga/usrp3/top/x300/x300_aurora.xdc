#
# Copyright 2016 Ettus Research LLC
#

set_property PACKAGE_PIN   R8       [get_ports XG_CLK_p]
set_property PACKAGE_PIN   R7       [get_ports XG_CLK_n]

#IOSTANDARD not required because this is a GT terminal
#set_property IOSTANDARD    LVDS_25  [get_ports {XG_CLK_*}]

create_clock -name AUR_CLK -period 6.400 -waveform {0.000 3.200} [get_ports XG_CLK_p]
create_generated_clock -name aurora_init_clk [get_pins -hierarchical -filter {NAME =~ "*aurora_clk_gen_i/dclk_divide_by_2_buf/O"}]

set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks aurora_init_clk]

set_false_path -to [get_pins -hierarchical -filter {NAME =~ "*sfpp_io_*/*/rst_sync_sys_rst_i/*aurora_64b66b_pcs_pma_cdc_to_reg/D"}]