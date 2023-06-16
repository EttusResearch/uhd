#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Clock definition constraints for X410
#

###############################################################################
# Motherboard Clocks
###############################################################################

# PLL Reference Clock. Used to derive data clocks.
# Constrain to the fastest possible clock rate supported in the driver.
# MPM supports 61.44 / 62.5 / 64.0 MHz.
set pll_ref_clk_period 15.625
create_clock -name pll_ref_clk -period $pll_ref_clk_period [get_ports PLL_REFCLK_FPGA_P]
