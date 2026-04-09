#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_INTERCON_2X64_256_ORIG_SRCS = $(addprefix $(IP_DIR)/axi_intercon_2x64_256_bd/, \
axi_intercon_2x64_256_bd.tcl \
)

IP_AXI_INTERCON_2X64_256_HDL_SRCS = 

IP_AXI_INTERCON_2X64_256_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_intercon_2x64_256_bd/, \
axi_intercon_2x64_256_bd.tcl \
)

IP_AXI_INTERCON_2X64_256_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_intercon_2x64_256_bd/, \
axi_intercon_2x64_256_bd/axi_intercon_2x64_256_bd.bd \
)

BD_AXI_INTERCON_2X64_256_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_intercon_2x64_256_bd/, \
axi_intercon_2x64_256_bd.bd.out \
axi_intercon_2x64_256_bd/axi_intercon_2x64_256_bd_ooc.xdc \
axi_intercon_2x64_256_bd/synth/axi_intercon_2x64_256_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_AXI_INTERCON_2X64_256_BD_TRGT
$(IP_AXI_INTERCON_2X64_256_BD_SRCS) $(BD_AXI_INTERCON_2X64_256_BD_OUTS) $(IP_AXI_INTERCON_2X64_256_BDTCL_SRCS): IP_AXI_INTERCON_2X64_256_BD_TRGT
	@:

IP_AXI_INTERCON_2X64_256_BD_TRGT: $(IP_AXI_INTERCON_2X64_256_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,axi_intercon_2x64_256_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS),$(IP_AXI_INTERCON_2X64_256_HDL_SRCS),)
