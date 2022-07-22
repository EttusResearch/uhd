#
# Copyright 2015 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_DIVIDE_INT24_SRCS = $(IP_BUILD_DIR)/divide_int24/divide_int24.xci

LIB_IP_DIVIDE_INT24_OUTS = $(addprefix $(IP_BUILD_DIR)/divide_int24/, \
divide_int24.xci.out \
synth/divide_int24.vhd \
)

.INTERMEDIATE: LIB_IP_DIVIDE_INT24_TRGT
$(LIB_IP_DIVIDE_INT24_SRCS) $(LIB_IP_DIVIDE_INT24_OUTS): LIB_IP_DIVIDE_INT24_TRGT
	@:

LIB_IP_DIVIDE_INT24_TRGT: $(LIB_IP_DIR)/divide_int24/divide_int24.xci
	$(call BUILD_VIVADO_IP,divide_int24,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
