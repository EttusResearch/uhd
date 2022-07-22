#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_INTER_2X128_512_ORIG_SRCS = $(addprefix $(IP_DIR)/axi_inter_2x128_512_bd/, \
axi_inter_2x128_512_bd.tcl \
)

IP_AXI_INTER_2X128_512_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_inter_2x128_512_bd/, \
axi_inter_2x128_512_bd.tcl \
)

IP_AXI_INTER_2X128_512_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_inter_2x128_512_bd/, \
axi_inter_2x128_512_bd/axi_inter_2x128_512_bd.bd \
)

BD_AXI_INTER_2X128_512_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_inter_2x128_512_bd/, \
axi_inter_2x128_512_bd.bd.out \
axi_inter_2x128_512_bd/axi_inter_2x128_512_bd_ooc.xdc \
axi_inter_2x128_512_bd/synth/axi_inter_2x128_512_bd.v \
)

.INTERMEDIATE: IP_AXI_INTER_2X128_512_BD_TRGT
$(IP_AXI_INTER_2X128_512_BD_SRCS) $(BD_AXI_INTER_2X128_512_BD_OUTS) $(IP_AXI_INTER_2X128_512_BDTCL_SRCS): IP_AXI_INTER_2X128_512_BD_TRGT
	@:

IP_AXI_INTER_2X128_512_BD_TRGT: $(IP_AXI_INTER_2X128_512_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,axi_inter_2x128_512_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi)
