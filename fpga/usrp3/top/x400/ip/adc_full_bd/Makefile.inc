#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_ADC_FULL_ORIG_SRCS = $(addprefix $(IP_DIR)/adc_full_bd/, \
adc_full_bd.tcl \
)

IP_ADC_FULL_HDL_SRCS = $(addprefix $(BASE_DIR)/x400/rf/, \
common/adc_iq_repacker.v \
)

IP_ADC_FULL_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/adc_full_bd/, \
adc_full_bd.tcl \
)

IP_ADC_FULL_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/adc_full_bd/, \
adc_full_bd/adc_full_bd.bd \
)

BD_ADC_FULL_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/adc_full_bd/, \
adc_full_bd.bd.out \
adc_full_bd/adc_full_bd_ooc.xdc \
adc_full_bd/synth/adc_full_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_ADC_FULL_BD_TRGT
$(IP_ADC_FULL_BD_SRCS) $(BD_ADC_FULL_BD_OUTS) $(IP_ADC_FULL_BDTCL_SRCS): IP_ADC_FULL_BD_TRGT
	@:

IP_ADC_FULL_BD_TRGT: $(IP_ADC_FULL_ORIG_SRCS) $(IP_ADC_FULL_HDL_SRCS)
	$(call BUILD_VIVADO_BDTCL,adc_full_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS),$(IP_ADC_FULL_HDL_SRCS),)
