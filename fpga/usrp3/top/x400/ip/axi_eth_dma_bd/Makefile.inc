#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_ETH_DMA_BD_HDL_SRCS = $(addprefix $(IP_DIR)/axi_eth_dma_bd/, \
axi_eth_dma.sv \
)

IP_AXI_ETH_DMA_BD_ORIG_SRCS = $(addprefix $(IP_DIR)/axi_eth_dma_bd/, \
axi_eth_dma_bd.tcl \
)

IP_AXI_ETH_DMA_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_eth_dma_bd/, \
axi_eth_dma_bd.tcl \
)

IP_AXI_ETH_DMA_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_eth_dma_bd/, \
axi_eth_dma_bd/axi_eth_dma_bd.bd \
)

BD_AXI_ETH_DMA_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_eth_dma_bd/, \
axi_eth_dma_bd.bd.out \
axi_eth_dma_bd/synth/axi_eth_dma_bd.v \
)

.INTERMEDIATE: IP_AXI_ETH_DMA_BD_TRGT
$(IP_AXI_ETH_DMA_BD_SRCS) $(BD_AXI_ETH_DMA_BD_OUTS) $(IP_AXI_ETH_DMA_BDTCL_SRCS): IP_AXI_ETH_DMA_BD_TRGT
	@:

IP_AXI_ETH_DMA_BD_TRGT: $(IP_AXI_ETH_DMA_BD_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,axi_eth_dma_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi)
