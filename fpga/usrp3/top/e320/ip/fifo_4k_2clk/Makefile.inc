#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_FIFO_4K_2CLK_SRCS = $(IP_BUILD_DIR)/fifo_4k_2clk/fifo_4k_2clk.xci

IP_FIFO_4K_2CLK_OUTS = $(addprefix $(IP_BUILD_DIR)/fifo_4k_2clk/, \
fifo_4k_2clk.xci.out \
synth/fifo_4k_2clk.vhd \
) 

$(IP_FIFO_4K_2CLK_SRCS) $(IP_FIFO_4K_2CLK_OUTS) : $(IP_DIR)/fifo_4k_2clk/fifo_4k_2clk.xci
	$(call BUILD_VIVADO_IP,fifo_4k_2clk,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
