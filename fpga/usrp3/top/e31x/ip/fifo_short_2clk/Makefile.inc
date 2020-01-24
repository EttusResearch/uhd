#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_FIFO_SHORT_2CLK_SRCS = $(IP_BUILD_DIR)/fifo_short_2clk/fifo_short_2clk.xci

IP_FIFO_SHORT_2CLK_OUTS = $(addprefix $(IP_BUILD_DIR)/fifo_short_2clk/, \
fifo_short_2clk.xci.out \
synth/fifo_short_2clk.vhd \
) 

$(IP_FIFO_SHORT_2CLK_SRCS) $(IP_FIFO_SHORT_2CLK_OUTS) : $(IP_DIR)/fifo_short_2clk/fifo_short_2clk.xci
	$(call BUILD_VIVADO_IP,fifo_short_2clk,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
