#
# Copyright 2015-2017 Ettus Research LLC
#

create_generated_clock -name ddr3_axi_clk    [get_pins -hierarchical -filter {NAME =~ "*u_ddr3_infrastructure/gen_ui_extra_clocks.mmcm_i/CLKFBOUT"}]
create_generated_clock -name ddr3_axi_clk_x2 [get_pins -hierarchical -filter {NAME =~ "*u_ddr3_infrastructure/gen_ui_extra_clocks.mmcm_i/CLKOUT0"}]

set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks mmcm_ps_clk_bufg_in]
set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks ddr3_axi_clk]
set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks ddr3_axi_clk_x2]

# Floorplan the MIG and the primary AXI4 interconnect
# create_pblock pblock_dram_iface
# resize_pblock pblock_dram_iface -add {SLICE_X104Y0:SLICE_X153Y149 DSP48_X3Y0:DSP48_X5Y59 RAMB18_X3Y0:RAMB18_X6Y59 RAMB36_X3Y0:RAMB36_X6Y29}
# add_cells_to_pblock pblock_dram_iface [get_cells [list u_ddr3_32bit]] -clear_locs
# add_cells_to_pblock pblock_dram_iface [get_cells [list x300_core/axi_intercon_2x64_128_bd_i]] -clear_locs
