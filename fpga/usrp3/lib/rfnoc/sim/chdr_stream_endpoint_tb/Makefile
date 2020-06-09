#
# Copyright 2019 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs

include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/crossbar/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS) \
$(RFNOC_XBAR_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = chdr_stream_endpoint_all_tb

SIM_SRCS = \
$(abspath lossy_xport_model.v) \
$(abspath chdr_stream_endpoint_tb.sv) \
$(abspath chdr_stream_endpoint_all_tb.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
