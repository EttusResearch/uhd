#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI_INTERCONNECT_ETH_HDL_SRCS = $(addprefix $(IP_DIR)/axi_interconnect_eth_bd/, \
axi_interconnect_eth.sv \
)

IP_AXI_INTERCONNECT_ETH_HDL_SIM_SRCS = $(wildcard $(addprefix $(IP_BUILD_DIR)/axi_interconnect_eth_bd/axi_interconnect_eth_bd/, \
sim/axi_interconnect_eth_bd.v\
ip/axi_interconnect_eth_bd_xbar_0/sim/axi_interconnect_eth_bd_xbar_0.v\
ipshared/*/simulation/fifo_generator_vlog_beh.v\
ipshared/*/hdl/*.v\
))

IP_AXI_INTERCONNECT_ETH_ORIG_SRCS = $(addprefix $(IP_DIR)/axi_interconnect_eth_bd/, \
axi_interconnect_eth_bd.tcl \
)

IP_AXI_INTERCONNECT_ETH_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_interconnect_eth_bd/, \
axi_interconnect_eth_bd.tcl \
)

IP_AXI_INTERCONNECT_ETH_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/axi_interconnect_eth_bd/, \
axi_interconnect_eth_bd/axi_interconnect_eth_bd.bd \
)

BD_AXI_INTERCONNECT_ETH_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_interconnect_eth_bd/, \
axi_interconnect_eth_bd.bd.out \
axi_interconnect_eth_bd/synth/axi_interconnect_eth_bd.v \
axi_interconnect_eth_bd/axi_interconnect_eth_bd_ooc.xdc \
)

.INTERMEDIATE: IP_AXI_INTERCONNECT_ETH_BD_TRGT
$(IP_AXI_INTERCONNECT_ETH_BD_SRCS) $(BD_AXI_INTERCONNECT_ETH_BD_OUTS) $(IP_AXI_INTERCONNECT_ETH_BDTCL_SRCS): IP_AXI_INTERCONNECT_ETH_BD_TRGT
	@:

IP_AXI_INTERCONNECT_ETH_BD_TRGT: $(IP_AXI_INTERCONNECT_ETH_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,axi_interconnect_eth_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi)
