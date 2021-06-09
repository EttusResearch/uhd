#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(IP_DIR)/xge_pcs_pma/Makefile.inc
include $(IP_DIR)/x4xx_ps_rfdc_bd/Makefile.inc
include $(IP_DIR)/axi_interconnect_app_bd/Makefile.inc
include $(IP_DIR)/axi_interconnect_eth_bd/Makefile.inc
include $(IP_DIR)/axi_interconnect_dma_bd/Makefile.inc
include $(IP_DIR)/ddr4_64bits/Makefile.inc
include $(IP_DIR)/adc_100m_bd/Makefile.inc
include $(IP_DIR)/adc_400m_bd/Makefile.inc
include $(IP_DIR)/dac_100m_bd/Makefile.inc
include $(IP_DIR)/dac_400m_bd/Makefile.inc
include $(IP_DIR)/axi64_4k_2clk_fifo/Makefile.inc
include $(IP_DIR)/fifo_short_2clk/Makefile.inc
include $(IP_DIR)/fifo_4k_2clk/Makefile.inc
include $(IP_DIR)/eth_100g_bd/Makefile.inc
include $(IP_DIR)/axi_eth_dma_bd/Makefile.inc
include $(IP_DIR)/hb47_1to2/Makefile.inc
include $(IP_DIR)/hb47_2to1/Makefile.inc

BD_SRCS = \
$(IP_X4XX_PS_RFDC_BD_SRCS) \
$(IP_X4XX_PS_RFDC_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_APP_BD_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_BD_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_BD_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_HDL_SRCS) \
$(IP_ADC_100M_BD_SRCS) \
$(IP_ADC_100M_HDL_SRCS) \
$(IP_DAC_100M_BD_SRCS) \
$(IP_DAC_100M_HDL_SRCS) \
$(IP_ADC_400M_BD_SRCS) \
$(IP_ADC_400M_HDL_SRCS) \
$(IP_DAC_400M_BD_SRCS) \
$(IP_DAC_400M_HDL_SRCS) \
$(IP_100G_BD_SRCS) \
$(IP_100G_HDL_SRCS) \
$(IP_AXI_ETH_DMA_BD_SRCS) \
$(IP_AXI_ETH_DMA_BD_HDL_SRCS)

IP_XCI_SRCS = \
$(IP_DDR4_64BITS_SRCS) \
$(IP_XGE_PCS_PMA_SRCS) \
$(IP_AXI64_4K_2CLK_FIFO_SRCS) \
$(IP_FIFO_SHORT_2CLK_SRCS) \
$(IP_FIFO_4K_2CLK_SRCS) \
$(IP_HB47_1TO2_SRCS) \
$(IP_HB47_2TO1_SRCS) \

BD_OUTPUTS = \
$(BD_X4XX_PS_RFDC_BD_OUTS) \
$(BD_AXI_INTERCONNECT_APP_BD_OUTS) \
$(BD_AXI_INTERCONNECT_ETH_BD_OUTS) \
$(BD_AXI_INTERCONNECT_DMA_BD_OUTS) \
$(BD_ADC_100M_BD_OUTS) \
$(BD_ADC_400M_BD_OUTS)  \
$(BD_DAC_100M_BD_OUTS) \
$(BD_DAC_400M_BD_OUTS)  \
$(BD_100G_BD_OUTS) \
$(BD_AXI_ETH_DMA_BD_OUTS)

IP_SYNTH_OUTPUTS = \
$(IP_DDR4_64BITS_OUTS) \
$(IP_XGE_PCS_PMA_OUTS) \
$(IP_AXI64_4K_2CLK_FIFO_OUTS) \
$(IP_FIFO_SHORT_2CLK_OUTS) \
$(IP_FIFO_4K_2CLK_OUTS) \
$(IP_HB47_1TO2_OUTS) \
$(IP_HB47_2TO1_OUTS) \

IP_HDL_SIM_SRCS = \
$(IP_AXI64_4K_2CLK_FIFO_HDL_SIM_SRCS) \
$(IP_FIFO_4K_2CLK_HDL_SIM_SRCS) \
$(IP_FIFO_SHORT_2CLK_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_HDL_SIM_SRCS) \
$(IP_100G_HDL_SIM_SRCS) \
$(IP_XGE_PCS_PMA_HDL_SIM_SRCS)

ip: $(IP_SYNTH_OUTPUTS) $(BD_OUTPUTS)

.PHONY: ip
