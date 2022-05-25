#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = ../../../../top
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its
# dependencies.
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include $(BASE_DIR)/../lib/control/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/xport_sv/Makefile.srcs
include $(BASE_DIR)/../lib/axi4s_sv/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/crossbar/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS)     \
$(RFNOC_UTIL_SRCS)     \
$(RFNOC_OOT_SRCS)      \
$(CONTROL_LIB_SRCS)    \
$(RFNOC_XPORT_SV_SRCS) \
$(AXI4S_SV_SRCS)       \
$(RFNOC_XBAR_SRCS)     \
)

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------

# Suppressing the following warning:
#
# ** Warning: axi4s_remove_bytes.sv(228): (vlog-2583) [SVCHK] - Extra checking
#    for conflicts with always_comb and always_latch variables is done at vopt
#    time
SVLOG_ARGS += -suppress 2583

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = eth_ipv4_interface_all_tb glbl
SIM_SRCS = \
$(abspath eth_ipv4_interface_tb.sv) \
$(abspath eth_ipv4_interface_all_tb.sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
