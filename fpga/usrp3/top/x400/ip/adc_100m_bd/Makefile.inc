#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_ADC_100M_ORIG_SRCS = $(addprefix $(IP_DIR)/adc_100m_bd/, \
adc_100m_bd.tcl \
)

IP_ADC_100M_HDL_SRCS = $(addprefix $(BASE_DIR)/x400/rf/, \
100m/ddc_saturate.vhd \
100m/adc_3_1_clk_converter.vhd \
100m/adc_gearbox_2x1.v \
common/PkgRf.vhd \
common/scale_2x.vhd \
)

IP_ADC_100M_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/adc_100m_bd/, \
adc_100m_bd.tcl \
)

IP_ADC_100M_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/adc_100m_bd/, \
adc_100m_bd/adc_100m_bd.bd \
)

BD_ADC_100M_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/adc_100m_bd/, \
adc_100m_bd.bd.out \
adc_100m_bd/adc_100m_bd_ooc.xdc \
adc_100m_bd/synth/adc_100m_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_ADC_100M_BD_TRGT
$(IP_ADC_100M_BD_SRCS) $(BD_ADC_100M_BD_OUTS) $(IP_ADC_100M_BDTCL_SRCS): IP_ADC_100M_BD_TRGT
	@:

IP_ADC_100M_BD_TRGT: $(IP_ADC_100M_ORIG_SRCS) $(IP_ADC_100M_HDL_SRCS)
	$(call BUILD_VIVADO_BDTCL,adc_100m_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS),$(IP_ADC_100M_HDL_SRCS),)
