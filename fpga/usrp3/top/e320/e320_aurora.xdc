#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#


create_generated_clock -name aurora_init_clk [get_pins -hierarchical -filter {NAME =~ "*aurora_clk_gen_i/dclk_divide_by_2_buf/O"}]

set_clock_groups -asynchronous -group [get_clocks bus_clk] -group [get_clocks aurora_init_clk]

set_false_path -to [get_pins -hierarchical -filter {NAME =~ "sfp_wrapper_*/mgt_io_i/aurora_phy*/aurora_64b66b_pcs_pma*/*/gt_reset_sync/stg1_*_cdc_to_reg/D"}]

set_false_path -to [get_pins -hierarchical -filter {NAME =~ "*npio*/aurora_phy*/aurora_64b66b_pcs_pma*/*/gt_reset_sync/stg1_*_cdc_to_reg/D"}]
