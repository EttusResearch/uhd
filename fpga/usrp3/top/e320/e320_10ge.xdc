#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0-or-later
#


# No need for any asynchronous clock groups between bus_clk and the recovered clocks,
# because bus_clk already has a blanket asynchronous constraint from the top level XDC.

# Remove analysis between the xge_clk and the recovered clocks from the MGT PHYs,
# since they cannot be related to one another with any known phase or period.
set_clock_groups -asynchronous -group [get_clocks -filter {NAME =~ sfp_wrapper_*/mgt_io_i/ten_gige_phy_i/ten_gig_eth_pcs_pma_i/*/gtxe2_i/RXOUTCLK}] -group [get_clocks xge_clk]
set_clock_groups -asynchronous -group [get_clocks -filter {NAME =~ sfp_wrapper_*/mgt_io_i/ten_gige_phy_i/ten_gig_eth_pcs_pma_i/*/gtxe2_i/TXOUTCLK}] -group [get_clocks xge_clk]

set_false_path -to [get_pins -of_objects [get_cells -hier -filter {NAME =~ sfp_wrapper_*/mgt_io_i/ten_gige_phy_i/*sync1_r_reg*}] -filter {NAME =~ *PRE}]
set_false_path -to [get_pins -of_objects [get_cells -hier -filter {NAME =~ sfp_wrapper_*/mgt_io_i/ten_gige_phy_i/*sync1_r_reg*}] -filter {NAME =~ *CLR}]
