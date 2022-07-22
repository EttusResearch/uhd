#
# Copyright 2016 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_INTERCON_4X64_256_BD_SRCS = $(IP_BUILD_DIR)/axi_intercon_4x64_256_bd/axi_intercon_4x64_256_bd.bd \
$(IP_BUILD_DIR)/axi_intercon_4x64_256_bd/axi_intercon_4x64_256_bd.bxml \
$(IP_BUILD_DIR)/axi_intercon_4x64_256_bd/axi_intercon_4x64_256_bd_wrapper.v 

BD_AXI_INTERCON_4X64_256_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_intercon_4x64_256_bd/, \
axi_intercon_4x64_256_bd.bd.out \
axi_intercon_4x64_256_bd_ooc.xdc \
)

.INTERMEDIATE: IP_AXI_INTERCON_4X64_256_BD_TRGT
$(IP_AXI_INTERCON_4X64_256_BD_SRCS) $(IP_AXI_INTERCON_4X64_256_BD_OUTS) : IP_AXI_INTERCON_4X64_256_BD_TRGT
	@:

IP_AXI_INTERCON_4X64_256_BD_TRGT: $(IP_DIR)/axi_intercon_4x64_256_bd/axi_intercon_4x64_256_bd.bd
	$(call BUILD_VIVADO_BD,axi_intercon_4x64_256_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR))
