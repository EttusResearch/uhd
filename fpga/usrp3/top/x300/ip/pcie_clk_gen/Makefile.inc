#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_PCIE_CLK_GEN_SRCS = $(IP_BUILD_DIR)/pcie_clk_gen/pcie_clk_gen.xci

IP_PCIE_CLK_GEN_OUTS = $(addprefix $(IP_BUILD_DIR)/pcie_clk_gen/, \
pcie_clk_gen.xci.out \
pcie_clk_gen.v \
) 

$(IP_PCIE_CLK_GEN_SRCS) $(IP_PCIE_CLK_GEN_OUTS) : $(IP_DIR)/pcie_clk_gen/pcie_clk_gen.xci
	$(call BUILD_VIVADO_IP,pcie_clk_gen,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
