#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_VIO_0_SRCS = $(IP_BUILD_DIR)/vio_0/vio_0.xci

LIB_IP_VIO_0_OUTS = $(addprefix $(IP_BUILD_DIR)/vio_0/, \
vio_0.xci.out \
synth/vio_0.vhd \
) 

$(LIB_IP_VIO_0_SRCS) $(LIB_IP_VIO_0_OUTS) : $(OOT_FPGA_DIR)/ip/vio_0/vio_0.xci
	$(call BUILD_VIVADO_IP,vio_0,$(ARCH),$(PART_ID),$(OOT_FPGA_DIR)/ip,$(IP_BUILD_DIR),0)
