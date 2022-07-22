#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_FIFO_4K_2CLK_SRCS = $(IP_BUILD_DIR)/fifo_4k_2clk/fifo_4k_2clk.xci

IP_FIFO_4K_2CLK_HDL_SIM_SRCS = $(addprefix $(IP_BUILD_DIR)/fifo_4k_2clk/, \
fifo_4k_2clk_sim_netlist.v \
)

IP_FIFO_4K_2CLK_OUTS = $(addprefix $(IP_BUILD_DIR)/fifo_4k_2clk/, \
fifo_4k_2clk.xci.out \
synth/fifo_4k_2clk.vhd \
)

.INTERMEDIATE: IP_FIFO_4K_2CLK_TRGT
$(IP_FIFO_4K_2CLK_SRCS) $(IP_FIFO_4K_2CLK_OUTS): IP_FIFO_4K_2CLK_TRGT
	@:

IP_FIFO_4K_2CLK_TRGT: $(IP_DIR)/fifo_4k_2clk/fifo_4k_2clk.xci
	$(call BUILD_VIVADO_IP,fifo_4k_2clk,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
