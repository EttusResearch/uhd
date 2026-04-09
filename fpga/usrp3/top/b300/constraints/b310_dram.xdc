#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

create_generated_clock -name ddr3_axi_clk    [get_pins -hierarchical -filter {NAME =~ "*u_ddr3_infrastructure/gen_ui_extra_clocks.mmcm_i/CLKFBOUT"}]
create_generated_clock -name ddr3_axi_clk_x2 [get_pins -hierarchical -filter {NAME =~ "*u_ddr3_infrastructure/gen_ui_extra_clocks.mmcm_i/CLKOUT0"}]

set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks mmcm_ps_clk_bufg_in]
set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks ddr3_axi_clk]
set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks ddr3_axi_clk_x2]
