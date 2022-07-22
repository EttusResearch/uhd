#
# Copyright 2015 Ettus Research
# Copyright 2016 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_DIVIDE_INT16_SRCS = $(IP_BUILD_DIR)/divide_int16/divide_int16.xci

LIB_IP_DIVIDE_INT16_OUTS = $(addprefix $(IP_BUILD_DIR)/divide_int16/, \
divide_int16.xci.out \
synth/divide_int16.vhd \
)

.INTERMEDIATE: LIB_IP_DIVIDE_INT16_TRGT
$(LIB_IP_DIVIDE_INT16_SRCS) $(LIB_IP_DIVIDE_INT16_OUTS): LIB_IP_DIVIDE_INT16_TRGT
	@:

LIB_IP_DIVIDE_INT16_TRGT: $(LIB_IP_DIR)/divide_int16/divide_int16.xci
	$(call BUILD_VIVADO_IP,divide_int16,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
