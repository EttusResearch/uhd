#
# Copyright 2017 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_ETH_DMA_SRCS = $(IP_BUILD_DIR)/axi_eth_dma/axi_eth_dma.xci

IP_AXI_ETH_DMA_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_eth_dma/, \
axi_eth_dma.xci.out \
)

.INTERMEDIATE: IP_AXI_ETH_DMA_TRGT
$(IP_AXI_ETH_DMA_SRCS) $(IP_AXI_ETH_DMA_OUTS) : IP_AXI_ETH_DMA_TRGT
	@:

IP_AXI_ETH_DMA_TRGT: $(IP_DIR)/axi_eth_dma/axi_eth_dma.xci
	$(call BUILD_VIVADO_IP,axi_eth_dma,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
