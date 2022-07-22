#
# Copyright 2016 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_AXI_HB31_SRCS = $(IP_BUILD_DIR)/axi_hb31/axi_hb31.xci

LIB_IP_AXI_HB31_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_hb31/, \
axi_hb31.xci.out \
)

.INTERMEDIATE: LIB_IP_AXI_HB31_TRGT
$(LIB_IP_AXI_HB31_SRCS) $(LIB_IP_AXI_HB31_OUTS): LIB_IP_AXI_HB31_TRGT
	@:

LIB_IP_AXI_HB31_TRGT: $(LIB_IP_DIR)/axi_hb31/axi_hb31.xci
	$(call BUILD_VIVADO_IP,axi_hb31,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
