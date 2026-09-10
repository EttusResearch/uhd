#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(IP_DIR)/xge_pcs_pma/Makefile.inc
include $(IP_DIR)/axi_interconnect_app_bd/Makefile.inc
include $(IP_DIR)/axi_interconnect_eth_bd/Makefile.inc
include $(IP_DIR)/axi_interconnect_dma_bd/Makefile.inc
include $(IP_DIR)/axi_inter_1x64_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_2x64_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_4x64_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_1x128_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_2x128_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_4x128_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_1x512_512_bd/Makefile.inc
include $(IP_DIR)/axi_inter_2x512_512_bd/Makefile.inc
include $(IP_DIR)/axi64_4k_2clk_fifo/Makefile.inc
include $(IP_DIR)/eth_100g_bd/Makefile.inc
include $(IP_DIR)/axi_eth_dma_bd/Makefile.inc
include $(IP_DIR)/hb47_1to2/Makefile.inc
include $(IP_DIR)/hb47_2to1/Makefile.inc

ifdef X410
include $(IP_DIR)/ddr4_64bits/Makefile.inc
include $(IP_DIR)/adc_100m_bd/Makefile.inc
include $(IP_DIR)/adc_400m_bd/Makefile.inc
include $(IP_DIR)/dac_100m_bd/Makefile.inc
include $(IP_DIR)/dac_400m_bd/Makefile.inc
include $(IP_DIR)/x4xx_ps_rfdc_bd/x410_ps_rfdc_bd/Makefile.inc
endif

ifdef X420
include $(IP_DIR)/ddr4_64bits/Makefile.inc
include $(IP_DIR)/x4xx_ps_rfdc_bd/x420_ps_rfdc_bd/Makefile.inc
include $(IP_DIR)/fir_dec3_4spc/Makefile.inc
include $(IP_DIR)/fir_inp3_2spc/Makefile.inc
endif

ifdef X440
include $(IP_DIR)/adc_100m_bd/Makefile.inc
include $(IP_DIR)/dac_100m_bd/Makefile.inc
include $(IP_DIR)/ddr4_64bits_x440/Makefile.inc
include $(IP_DIR)/x4xx_ps_rfdc_bd/x440_ps_rfdc_bd/Makefile.inc
include $(IP_DIR)/x4xx_ps_rfdc_bd/x440_2spc_ps_rfdc_bd/Makefile.inc
include $(IP_DIR)/fir_dec3_2spc/Makefile.inc
include $(IP_DIR)/fir_inp3_1spc/Makefile.inc
endif

BD_SRCS = \
$(IP_X4XX_PS_RFDC_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_APP_BD_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_BD_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_BD_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_HDL_SRCS) \
$(IP_AXI_INTER_1X64_512_BD_SRCS) \
$(IP_AXI_INTER_2X64_512_BD_SRCS) \
$(IP_AXI_INTER_4X64_512_BD_SRCS) \
$(IP_AXI_INTER_1X128_512_BD_SRCS) \
$(IP_AXI_INTER_2X128_512_BD_SRCS) \
$(IP_AXI_INTER_4X128_512_BD_SRCS) \
$(IP_AXI_INTER_1X512_512_BD_SRCS) \
$(IP_AXI_INTER_2X512_512_BD_SRCS) \
$(IP_100G_BD_SRCS) \
$(IP_100G_HDL_SRCS) \
$(IP_AXI_ETH_DMA_BD_SRCS) \
$(IP_AXI_ETH_DMA_BD_HDL_SRCS)

ifdef X410
BD_SRCS += \
$(IP_ADC_100M_BD_SRCS)  \
$(IP_ADC_100M_HDL_SRCS) \
$(IP_DAC_100M_BD_SRCS)  \
$(IP_DAC_100M_HDL_SRCS) \
$(IP_ADC_400M_BD_SRCS)  \
$(IP_ADC_400M_HDL_SRCS) \
$(IP_DAC_400M_BD_SRCS)  \
$(IP_DAC_400M_HDL_SRCS)
endif

ifdef X440
	# Both X440 RFDC block-diagram Makefiles assign the same source and output
	# variables. For a binary target, select only the RFDC variant required by
	# the image core so the shared variables identify the correct sources. For
	# the IP target, BUILD_ALL_X440_RFDC selects both variants so all generated
	# IP is available.
	ifeq ($(BUILD_ALL_X440_RFDC),1)
		BD_SRCS += $(IP_X4XX_PS_RFDC_BD_SRCS) $(IP_X4XX_2SPC_PS_RFDC_BD_SRCS)
	else ifneq (,$(findstring X440_2SPC_PS_RFDC_BD=1,$(RFNOC_IMAGE_CORE_DEFS)))
		BD_SRCS += $(IP_X4XX_2SPC_PS_RFDC_BD_SRCS)
	else
		BD_SRCS += $(IP_X4XX_PS_RFDC_BD_SRCS)
	endif
BD_SRCS += \
$(IP_ADC_100M_BD_SRCS)   \
$(IP_ADC_100M_HDL_SRCS)  \
$(IP_DAC_100M_BD_SRCS)   \
$(IP_DAC_100M_HDL_SRCS)
else # default case for non-X440 devices
 	BD_SRCS += $(IP_X4XX_PS_RFDC_BD_SRCS)
endif

IP_XCI_SRCS = \
$(IP_XGE_PCS_PMA_SRCS) \
$(IP_AXI64_4K_2CLK_FIFO_SRCS) \
$(IP_HB47_1TO2_SRCS) \
$(IP_HB47_2TO1_SRCS) \

ifdef X410
IP_XCI_SRCS += \
$(IP_DDR4_64BITS_SRCS)
endif

ifdef X420
IP_XCI_SRCS += \
$(IP_DDR4_64BITS_SRCS) \
$(IP_FIR_DEC3_4SPC_SRCS) \
$(IP_FIR_INP3_2SPC_SRCS)
endif

ifdef X440
IP_XCI_SRCS += \
$(IP_DDR4_64BITS_X440_SRCS) \
$(IP_FIR_DEC3_2SPC_SRCS) \
$(IP_FIR_INP3_1SPC_SRCS)
endif

BD_OUTPUTS = \
$(BD_AXI_INTERCONNECT_APP_BD_OUTS) \
$(BD_AXI_INTERCONNECT_ETH_BD_OUTS) \
$(BD_AXI_INTERCONNECT_DMA_BD_OUTS) \
$(BD_AXI_INTER_1x64_512_BD_OUTS) \
$(BD_AXI_INTER_2x64_512_BD_OUTS) \
$(BD_AXI_INTER_4x64_512_BD_OUTS) \
$(BD_AXI_INTER_1X128_512_BD_OUTS) \
$(BD_AXI_INTER_2X128_512_BD_OUTS) \
$(BD_AXI_INTER_4x128_512_BD_OUTS) \
$(BD_AXI_INTER_1X512_512_BD_OUTS) \
$(BD_AXI_INTER_2X512_512_BD_OUTS) \
$(BD_100G_BD_OUTS) \
$(BD_AXI_ETH_DMA_BD_OUTS)

# See the X440 BD_SRCS selection above for why these RFDC outputs are selected.
ifeq ($(BUILD_ALL_X440_RFDC),1)
	BD_OUTPUTS += $(BD_X4XX_PS_RFDC_BD_OUTS) $(BD_X4XX_2SPC_PS_RFDC_BD_OUTS)
else ifneq (,$(findstring X440_2SPC_PS_RFDC_BD=1,$(RFNOC_IMAGE_CORE_DEFS)))
	BD_OUTPUTS += $(BD_X4XX_2SPC_PS_RFDC_BD_OUTS)
else
	BD_OUTPUTS += $(BD_X4XX_PS_RFDC_BD_OUTS)
endif

ifdef X410
BD_OUTPUTS += \
$(BD_ADC_100M_BD_OUTS) \
$(BD_ADC_400M_BD_OUTS) \
$(BD_DAC_100M_BD_OUTS) \
$(BD_DAC_400M_BD_OUTS)
endif

ifdef X440
BD_OUTPUTS += \
$(BD_ADC_100M_BD_OUTS) \
$(BD_DAC_100M_BD_OUTS)
endif

IP_SYNTH_OUTPUTS = \
$(IP_XGE_PCS_PMA_OUTS) \
$(IP_AXI64_4K_2CLK_FIFO_OUTS) \
$(IP_HB47_1TO2_OUTS) \
$(IP_HB47_2TO1_OUTS) \

ifdef X410
IP_SYNTH_OUTPUTS += \
$(IP_DDR4_64BITS_OUTS)
endif

ifdef X420
IP_SYNTH_OUTPUTS += \
$(IP_DDR4_64BITS_OUTS)
endif

ifdef X440
IP_SYNTH_OUTPUTS += \
$(IP_DDR4_64BITS_X440_OUTS)
endif

IP_HDL_SIM_SRCS = \
$(IP_AXI64_4K_2CLK_FIFO_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_HDL_SIM_SRCS) \
$(IP_100G_HDL_SIM_SRCS) \
$(IP_XGE_PCS_PMA_HDL_SIM_SRCS)

ip: $(IP_SYNTH_OUTPUTS) $(BD_OUTPUTS)

.PHONY: ip
