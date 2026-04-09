#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include $(IP_DIR)/fifo_4k_2clk/Makefile.inc
include $(IP_DIR)/bus_clk_gen/Makefile.inc
include $(IP_DIR)/ref_clk_pll/Makefile.inc
include $(IP_DIR)/radio_clk_gen/Makefile.inc
include $(IP_DIR)/axi_intercon_2x64_256_bd/Makefile.inc
include $(IP_DIR)/axi_intercon_4x64_256_bd/Makefile.inc
include $(IP_DIR)/ddr3_32bit/Makefile.inc
include $(IP_DIR)/fifo_short_2clk/Makefile.inc
include $(IP_DIR)/fifo_32bit_4k/Makefile.inc

IP_XCI_SRCS = \
$(IP_BUS_CLK_GEN_SRCS) \
$(IP_REF_CLK_PLL_SRCS) \
$(IP_RADIO_CLK_GEN_SRCS) \
$(IP_FIFO_4K_2CLK_SRCS) \
$(IP_FIFO_SHORT_2CLK_SRCS) \
$(IP_DDR3_32BIT_SRCS) \
$(IP_FIFO_32BIT_4K_SRCS)

BD_SRCS = \
$(IP_AXI_INTERCON_2X64_256_BD_SRCS) \
$(IP_AXI_INTERCON_4X64_256_BD_SRCS) \

IP_SYNTH_OUTPUTS = \
$(IP_BUS_CLK_GEN_OUTS) \
$(IP_REF_CLK_PLL_OUTS) \
$(IP_RADIO_CLK_GEN_OUTS) \
$(IP_FIFO_4K_2CLK_OUTS) \
$(IP_FIFO_SHORT_2CLK_OUTS) \
$(IP_DDR3_32BIT_OUTS) \
$(IP_FIFO_32BIT_4K_OUTS)

BD_OUTPUTS = \
$(IP_AXI_INTERCON_2X64_256_BD_OUTS) \
$(IP_AXI_INTERCON_4X64_256_BD_OUTS) \

ip: $(IP_SYNTH_OUTPUTS) $(BD_OUTPUTS)

.PHONY: ip

