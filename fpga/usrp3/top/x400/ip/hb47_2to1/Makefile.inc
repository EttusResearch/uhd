#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_HB47_2TO1_SRCS = $(IP_BUILD_DIR)/hb47_2to1/hb47_2to1.xci

IP_HB47_2TO1_OUTS = $(addprefix $(IP_BUILD_DIR)/hb47_2to1/, \
hb47_2to1.xci.out \
synth/hb47_2to1.vhd \
)

.INTERMEDIATE: IP_HB47_2TO1_TRGT
$(IP_HB47_2TO1_SRCS) $(IP_HB47_2TO1_OUTS): IP_HB47_2TO1_TRGT
	@:

IP_HB47_2TO1_TRGT: $(IP_DIR)/hb47_2to1/hb47_2to1.xci
	$(call BUILD_VIVADO_IP,hb47_2to1,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
