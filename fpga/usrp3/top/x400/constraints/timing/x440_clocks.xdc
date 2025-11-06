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
# those clocks, and so the MMCM has to be configured for the highest frequency.
###############################################################################

# PLL Reference Clock (PRC) input. This goes from the LMK straight to the MMCM
# input, and is used to generate the internal PRC as well as RFDC/data clocks.
# Set to maximum frequency of 64 MHz. Actual frequency depends on the LMK
# configuration.
set pll_ref_clk_period 15.625
create_clock -name pll_ref_clk -period $pll_ref_clk_period [get_ports PLL_REFCLK_FPGA_P]
