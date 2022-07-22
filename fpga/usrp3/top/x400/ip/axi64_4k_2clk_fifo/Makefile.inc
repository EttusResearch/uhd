#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI64_4K_2CLK_FIFO_SRCS = $(IP_BUILD_DIR)/axi64_4k_2clk_fifo/axi64_4k_2clk_fifo.xci

IP_AXI64_4K_2CLK_FIFO_HDL_SIM_SRCS = $(addprefix $(IP_BUILD_DIR)/axi64_4k_2clk_fifo/, \
axi64_4k_2clk_fifo_sim_netlist.v \
)

IP_AXI64_4K_2CLK_FIFO_OUTS = $(addprefix $(IP_BUILD_DIR)/axi64_4k_2clk_fifo/, \
axi64_4k_2clk_fifo.xci.out \
synth/axi64_4k_2clk_fifo.vhd \
)

.INTERMEDIATE: IP_AXI64_4K_2CLK_FIFO_TRGT
$(IP_AXI64_4K_2CLK_FIFO_SRCS) $(IP_AXI64_4K_2CLK_FIFO_OUTS): IP_AXI64_4K_2CLK_FIFO_TRGT
	@:

IP_AXI64_4K_2CLK_FIFO_TRGT: $(IP_DIR)/axi64_4k_2clk_fifo/axi64_4k_2clk_fifo.xci
	$(call BUILD_VIVADO_IP,axi64_4k_2clk_fifo,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
