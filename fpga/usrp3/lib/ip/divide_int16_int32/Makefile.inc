#
# Copyright 2015 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_DIVIDE_INT16_INT32_SRCS = $(IP_BUILD_DIR)/divide_int16_int32/divide_int16_int32.xci

LIB_IP_DIVIDE_INT16_INT32_OUTS = $(addprefix $(IP_BUILD_DIR)/divide_int16_int32/, \
divide_int16_int32.xci.out \
synth/divide_int16_int32.vhd \
)

.INTERMEDIATE: LIB_IP_DIVIDE_INT16_INT32_TRGT
$(LIB_IP_DIVIDE_INT16_INT32_SRCS) $(LIB_IP_DIVIDE_INT16_INT32_OUTS): LIB_IP_DIVIDE_INT16_INT32_TRGT
	@:

LIB_IP_DIVIDE_INT16_INT32_TRGT: $(LIB_IP_DIR)/divide_int16_int32/divide_int16_int32.xci
	$(call BUILD_VIVADO_IP,divide_int16_int32,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
