#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_DAC_100M_ORIG_SRCS = $(addprefix $(IP_DIR)/dac_100m_bd/, \
dac_100m_bd.tcl \
)

IP_DAC_100M_HDL_SRCS = $(addprefix $(BASE_DIR)/x400/rf/100m/, \
duc_saturate.vhd \
dac_1_3_clk_converter.vhd \
dac_2_1_clk_converter.vhd \
)

IP_DAC_100M_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/dac_100m_bd/, \
dac_100m_bd.tcl \
)

IP_DAC_100M_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/dac_100m_bd/, \
dac_100m_bd/dac_100m_bd.bd \
)

BD_DAC_100M_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/dac_100m_bd/, \
dac_100m_bd.bd.out \
dac_100m_bd/dac_100m_bd_ooc.xdc \
dac_100m_bd/synth/dac_100m_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_DAC_100M_BD_TRGT
$(IP_DAC_100M_BD_SRCS) $(BD_DAC_100M_BD_OUTS) $(IP_DAC_100M_BDTCL_SRCS): IP_DAC_100M_BD_TRGT
	@:

IP_DAC_100M_BD_TRGT: $(IP_DAC_100M_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,dac_100m_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS),$(IP_DAC_100M_HDL_SRCS),)
