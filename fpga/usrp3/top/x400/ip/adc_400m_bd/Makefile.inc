#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_ADC_400M_ORIG_SRCS = $(addprefix $(IP_DIR)/adc_400m_bd/, \
adc_400m_bd.tcl \
)

IP_ADC_400M_HDL_SRCS = $(addprefix $(BASE_DIR)/x400/rf/, \
400m/ddc_400m_saturate.vhd \
400m/adc_gearbox_8x4.v \
400m/adc_gearbox_2x4.vhd \
common/PkgRf.vhd \
common/scale_2x.vhd \
)

IP_ADC_400M_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/adc_400m_bd/, \
adc_400m_bd.tcl \
)

IP_ADC_400M_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/adc_400m_bd/, \
adc_400m_bd/adc_400m_bd.bd \
)

BD_ADC_400M_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/adc_400m_bd/, \
adc_400m_bd.bd.out \
adc_400m_bd/adc_400m_bd_ooc.xdc \
adc_400m_bd/synth/adc_400m_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_ADC_400M_BD_TRGT
$(IP_ADC_400M_BD_SRCS) $(BD_ADC_400M_BD_OUTS) $(IP_ADC_400M_BDTCL_SRCS): IP_ADC_400M_BD_TRGT
	@:

IP_ADC_400M_BD_TRGT: $(IP_ADC_400M_ORIG_SRCS) $(IP_ADC_400M_HDL_SRCS)
	$(call BUILD_VIVADO_BDTCL,adc_400m_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS),$(IP_ADC_400M_HDL_SRCS),)
