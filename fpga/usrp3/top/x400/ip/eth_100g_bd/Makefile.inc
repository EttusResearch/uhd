#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_100G_HDL_SRCS = $(addprefix $(IP_DIR)/eth_100g_bd/, \
PkgEth100gLbus.sv \
eth_100g.sv \
eth_100g_axis2lbus.sv \
eth_100g_lbus2axis.sv \
)

IP_100G_HDL_SIM_SRCS = $(addprefix $(IP_DIR)/eth_100g_bd/, \
model_100gbe.sv \
) \
$(wildcard $(addprefix $(IP_BUILD_DIR)/eth_100g_bd/eth_100g_bd/, \
sim/eth_100g_bd.v\
ip/*/ip_0/sim/*.v\
ip/*/sim/*.h\
ip/*/sim/*.v\
ip/eth_100g_bd_cmac_usplus_0_0/cmac_usplus_v3_1_4/*.v\
ip/eth_100g_bd_cmac_usplus_0_0/eth_100g_bd_cmac_usplus_0_0/example_design/*.v\
ip/eth_100g_bd_cmac_usplus_0_0/eth_100g_bd_cmac_usplus_0_0/header_files/*.h\
ip/eth_100g_bd_cmac_usplus_0_0/eth_100g_bd_cmac_usplus_0_0.v\
ipshared/*/hdl/*.v\
ipshared/*/hdl/*.sv\
))

IP_100G_ORIG_SRCS = $(addprefix $(IP_DIR)/eth_100g_bd/, \
eth_100g_bd.tcl \
)

IP_100G_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/eth_100g_bd/, \
eth_100g_bd.tcl \
)

IP_100G_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/eth_100g_bd/, \
eth_100g_bd/eth_100g_bd.bd \
)

BD_100G_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/eth_100g_bd/, \
eth_100g_bd.bd.out \
eth_100g_bd/eth_100g_bd_ooc.xdc \
eth_100g_bd/synth/eth_100g_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_100G_BD_TRGT
$(IP_100G_BD_SRCS) $(BD_100G_BD_OUTS) $(IP_100G_BDTCL_SRCS): IP_100G_BD_TRGT
	@:

IP_100G_BD_TRGT: $(IP_100G_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,eth_100g_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(EMPTY_IP_SRCS))
