#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#


# No need for any asynchronous clock groups between clk100 and the recovered clocks,
# because clk100 already has a blanket asynchronous constraint from the top level XDC.
set_clock_groups -asynchronous -group [get_clocks clk100] -group [get_clocks ge_clk]
set_clock_groups -asynchronous -group [get_clocks clk100] -group [get_clocks -of_objects [get_pins sfp_wrapper_*/mgt_io_i/one_gige_phy_i/*/core_clocking_i/mmcm_*/CLKOUT0]]
set_clock_groups -asynchronous -group [get_clocks clk100] -group [get_clocks -of_objects [get_pins sfp_wrapper_*/mgt_io_i/one_gige_phy_i/*/core_clocking_i/mmcm_*/CLKOUT1]]

set_false_path -to [get_pins -hier -filter {NAME =~ sfp_wrapper_*/mgt_io_i/one_gige_phy_i/*reset_sync*/PRE}]
set_false_path -to [get_pins -hier -filter {NAME =~ sfp_wrapper_*/mgt_io_i/one_gige_phy_i/*/pma_reset_pipe_reg*/PRE}]
set_false_path -to [get_pins -hier -filter {NAME =~ sfp_wrapper_*/mgt_io_i/one_gige_phy_i/*/pma_reset_pipe*[0]/D}]
