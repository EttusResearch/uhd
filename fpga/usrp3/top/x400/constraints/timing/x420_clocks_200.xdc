#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Clock definition constraints for X420 200 MHz BW type images
#
# See README.md in x420_ps_rfdc_bd for details on the clock configuration and
# the rationale behind it.
# The listed clocks are assuming an MMCM input frequency of 64 MHz.
#  CLKOUT0 = pll_ref_clk    = 64 MHz
#  CLKOUT1 = data_clk       = 64 MHz
#  CLKOUT2 = r0_rfdc_clk    = 192 MHz
#  CLKOUT3 = data_clk_2x    = 128 MHz
#  CLKOUT4 = r0_rfdc_clk_2x = 384 MHz
#  CLKOUT5 = r1_rfdc_clk    = 192 MHz
#  CLKOUT6 = r1_rfdc_clk_2x = 384 MHz

set data_clock_mmcm [get_cells -hierarchical -filter { PRIMITIVE_TYPE == CLOCK.PLL.MMCME4_ADV && NAME =~  "*data_clock_mmcm*" }]

set_property -dict [ list \
   CLKFBOUT_MULT_F  {24.0} \
   CLKOUT0_DIVIDE_F {24.0} \
   CLKOUT1_DIVIDE   {24} \
   CLKOUT2_DIVIDE   {8} \
   CLKOUT3_DIVIDE   {12} \
   CLKOUT4_DIVIDE   {4} \
   CLKOUT5_DIVIDE   {8} \
   CLKOUT6_DIVIDE   {4} \
] $data_clock_mmcm
