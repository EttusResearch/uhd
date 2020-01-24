#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_INTERCONNECT_SRCS = $(IP_BUILD_DIR)/axi_interconnect/axi_interconnect.xci

IP_AXI_INTERCONNECT = $(addprefix $(IP_BUILD_DIR)/axi_interconnect/, \
axi_interconnect.xci.out \
synth/axi_interconnect.v \
)

$(IP_AXI_INTERCONNECT_SRCS) $(IP_AXI_INTERCONNECT_OUTS) : $(IP_DIR)/axi_interconnect/axi_interconnect.xci
	$(call BUILD_VIVADO_IP,axi_interconnect,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)

