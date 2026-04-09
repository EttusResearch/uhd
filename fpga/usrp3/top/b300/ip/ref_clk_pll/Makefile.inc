#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_REF_CLK_PLL_SRCS = $(IP_BUILD_DIR)/ref_clk_pll/ref_clk_pll.xci

IP_REF_CLK_PLL_OUTS = $(addprefix $(IP_BUILD_DIR)/ref_clk_pll/, \
ref_clk_pll.xci.out \
ref_clk_pll.v \
)

.INTERMEDIATE: IP_REF_CLK_PLL_TRGT
$(IP_REF_CLK_PLL_SRCS) $(IP_REF_CLK_PLL_OUTS): IP_REF_CLK_PLL_TRGT
	@:

IP_REF_CLK_PLL_TRGT: $(IP_DIR)/ref_clk_pll/ref_clk_pll.xci
	$(call BUILD_VIVADO_IP,ref_clk_pll,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
