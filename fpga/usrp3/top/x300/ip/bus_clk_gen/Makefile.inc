#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_BUS_CLK_GEN_SRCS = $(IP_BUILD_DIR)/bus_clk_gen/bus_clk_gen.xci

IP_BUS_CLK_GEN_OUTS = $(addprefix $(IP_BUILD_DIR)/bus_clk_gen/, \
bus_clk_gen.xci.out \
bus_clk_gen.v \
) 

$(IP_BUS_CLK_GEN_SRCS) $(IP_BUS_CLK_GEN_OUTS) : $(IP_DIR)/bus_clk_gen/bus_clk_gen.xci
	$(call BUILD_VIVADO_IP,bus_clk_gen,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
