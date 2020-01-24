#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI64_8K_2CLK_FIFO_SRCS = $(IP_BUILD_DIR)/axi64_8k_2clk_fifo/axi64_8k_2clk_fifo.xci

IP_AXI64_8K_2CLK_FIFO_OUTS = $(addprefix $(IP_BUILD_DIR)/axi64_8k_2clk_fifo/, \
axi64_8k_2clk_fifo.xci.out \
synth/axi64_8k_2clk_fifo.vhd \
) 

$(IP_AXI64_8K_2CLK_FIFO_SRCS) $(IP_AXI64_8K_2CLK_FIFO_OUTS) : $(IP_DIR)/axi64_8k_2clk_fifo/axi64_8k_2clk_fifo.xci
	$(call BUILD_VIVADO_IP,axi64_8k_2clk_fifo,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
