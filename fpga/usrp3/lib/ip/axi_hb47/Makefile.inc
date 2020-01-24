#
# Copyright 2016 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_AXI_HB47_SRCS = $(IP_BUILD_DIR)/axi_hb47/axi_hb47.xci

LIB_IP_AXI_HB47_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_hb47/, \
axi_hb47.xci.out \
) 

$(LIB_IP_AXI_HB47_SRCS) $(LIB_IP_AXI_HB47_OUTS) : $(LIB_IP_DIR)/axi_hb47/axi_hb47.xci
	$(call BUILD_VIVADO_IP,axi_hb47,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
