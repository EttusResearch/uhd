#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_ILA_0_SRCS = $(IP_BUILD_DIR)/ila_0/ila_0.xci

LIB_IP_ILA_0_OUTS = $(addprefix $(IP_BUILD_DIR)/ila_0/, \
ila_0.xci.out \
synth/ila_0.vhd \
) 

$(LIB_IP_ILA_0_SRCS) $(LIB_IP_ILA_0_OUTS) : $(OOT_FPGA_DIR)/ip/ila_0/ila_0.xci
	$(call BUILD_VIVADO_IP,ila_0,$(ARCH),$(PART_ID),$(OOT_FPGA_DIR)/ip,$(IP_BUILD_DIR),0)
