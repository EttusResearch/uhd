#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Clock definition constraints for X440
#

###############################################################################
# MMCM-Related Clocks
#
# Note: Vivado will create generated clocks based on the default MMCM settings
# configured in the clocking wizard, but we are going to dynamically change
# those clocks, and so we need to provide worst-case values in here.
###############################################################################

# PLL Reference Clock (PRC) input. This goes from the LMK straight to the MMCM
# input, and is used to generate the internal PRC as well as RFDC/data clocks.
set pll_ref_clk_in_period 15.625
create_clock -name pll_ref_in_clk -period $pll_ref_clk_in_period [get_ports PLL_REFCLK_FPGA_P]

# Internal PLL Reference Clock (PRC). Used to derive data clocks. Constrain to
# the fastest possible clock rate supported in the driver.
set pll_ref_clk_period 15.625
create_clock -name pll_ref_clk \
             -period $pll_ref_clk_period \
             [get_pins x440_ps_rfdc_bd_i/rfdc/data_clock_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]

###############################################################################
# Generated Clocks
###############################################################################

create_generated_clock -name sync_ref_clk \
                       -source [get_ports BASE_REFCLK_FPGA_P] \
                       -divide_by 2 [get_pins clock_div_5mhz/clk_out_reg/Q]

