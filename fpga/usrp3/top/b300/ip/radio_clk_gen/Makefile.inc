#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_RADIO_CLK_GEN_SRCS = $(IP_BUILD_DIR)/radio_clk_gen/radio_clk_gen.xci

IP_RADIO_CLK_GEN_OUTS = $(addprefix $(IP_BUILD_DIR)/radio_clk_gen/, \
radio_clk_gen.xci.out \
radio_clk_gen.v \
)

.INTERMEDIATE: IP_RADIO_CLK_GEN_TRGT
$(IP_RADIO_CLK_GEN_SRCS) $(IP_RADIO_CLK_GEN_OUTS): IP_RADIO_CLK_GEN_TRGT
	@:

IP_RADIO_CLK_GEN_TRGT: $(IP_DIR)/radio_clk_gen/radio_clk_gen.xci
	$(call BUILD_VIVADO_IP,radio_clk_gen,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
