#
# Copyright 2015 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_DIVIDE_UINT32_SRCS = $(IP_BUILD_DIR)/divide_uint32/divide_uint32.xci

LIB_IP_DIVIDE_UINT32_OUTS = $(addprefix $(IP_BUILD_DIR)/divide_uint32/, \
divide_uint32.xci.out \
synth/divide_uint32.vhd \
)

.INTERMEDIATE: LIB_IP_DIVIDE_UINT32_TRGT
$(LIB_IP_DIVIDE_UINT32_SRCS) $(LIB_IP_DIVIDE_UINT32_OUTS): LIB_IP_DIVIDE_UINT32_TRGT
	@:

LIB_IP_DIVIDE_UINT32_TRGT: $(LIB_IP_DIR)/divide_uint32/divide_uint32.xci
	$(call BUILD_VIVADO_IP,divide_uint32,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
