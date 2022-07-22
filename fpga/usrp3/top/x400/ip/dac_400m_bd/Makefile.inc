#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_DAC_400M_ORIG_SRCS = $(addprefix $(IP_DIR)/dac_400m_bd/, \
dac_400m_bd.tcl \
)

IP_DAC_400M_HDL_SRCS = $(addprefix $(BASE_DIR)/x400/rf/400m/, \
duc_400m_saturate.vhd \
dac_gearbox_6x8.vhd \
dac_gearbox_6x12.vhd \
dac_gearbox_12x8.vhd \
dac_gearbox_4x2.v \
)

IP_DAC_400M_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/dac_400m_bd/, \
dac_400m_bd.tcl \
)

IP_DAC_400M_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/dac_400m_bd/, \
dac_400m_bd/dac_400m_bd.bd \
)

BD_DAC_400M_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/dac_400m_bd/, \
dac_400m_bd.bd.out \
dac_400m_bd/dac_400m_bd_ooc.xdc \
dac_400m_bd/synth/dac_400m_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_DAC_400M_BD_TRGT
$(IP_DAC_400M_BD_SRCS) $(BD_DAC_400M_BD_OUTS) $(IP_DAC_400M_BDTCL_SRCS): IP_DAC_400M_BD_TRGT
	@:

IP_DAC_400M_BD_TRGT: $(IP_DAC_400M_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,dac_400m_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS),$(IP_DAC_400M_HDL_SRCS),)
