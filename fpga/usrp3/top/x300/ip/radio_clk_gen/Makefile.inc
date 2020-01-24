#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_RADIO_CLK_GEN_SRCS = $(IP_BUILD_DIR)/radio_clk_gen/radio_clk_gen.xci

IP_RADIO_CLK_GEN_OUTS = $(addprefix $(IP_BUILD_DIR)/radio_clk_gen/, \
radio_clk_gen.xci.out \
radio_clk_gen.v \
) 

# We have to patch the XDC file to remove constraints on the source clock for the module
# All timing constraints are handled in one place (timing.xdc)
$(IP_RADIO_CLK_GEN_SRCS) $(IP_RADIO_CLK_GEN_OUTS) : $(IP_DIR)/radio_clk_gen/radio_clk_gen.xci
	$(call BUILD_VIVADO_IP,radio_clk_gen,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
	patch $(IP_BUILD_DIR)/radio_clk_gen/radio_clk_gen.xdc $(IP_DIR)/radio_clk_gen/radio_clk_gen.xdc.patch

