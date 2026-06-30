#
# Copyright 2026 Ettus Research, a National Instruments Brand
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

include $(BASE_DIR)/../lib/rfnoc/Makefile.srcs
include ../full/Makefile.srcs
include ../common/Makefile.srcs
include ../x420/Makefile.srcs

DESIGN_SRCS +=  \
$(RFNOC_FRAMEWORK_SRCS) \
$(RF_FULL_SRCS) \
$(RF_COMMON_SRCS) \
$(RF_X420_COMMON_SRCS) \
$(abspath rf_core_1000m.sv) \

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = rf_core_1000m_all_tb

SIM_SRCS = \
$(RF_X420_COMMON_TB_SRCS) \
$(abspath rf_core_1000m_tb.sv) \
$(abspath rf_core_1000m_all_tb.sv) \


#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
