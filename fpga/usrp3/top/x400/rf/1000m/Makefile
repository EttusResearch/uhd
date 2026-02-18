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

DESIGN_SRCS +=  \
$(RFNOC_FRAMEWORK_SRCS) \
$(RF_FULL_SRCS) \
$(RF_COMMON_SRCS) \
$(abspath XmlSvPkgIQ_IMPAIRMENT_REGMAP.sv) \
$(abspath impairment_correction_dsp.sv) \
$(abspath impairment_correction.sv) \
$(abspath XmlSvPkgDC_OFFSET_REGMAP.sv) \
$(abspath dc_offset.sv) \
$(abspath XmlSvPkgRF_CORE_REGMAP.sv) \
$(abspath rf_core_1000m.sv) \

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = rf_core_1000m_all_tb

SIM_SRCS = \
$(abspath impairment_correction_test_pkg.sv) \
$(abspath impairment_correction_tb.sv) \
$(abspath rf_core_1000m_tb.sv) \
$(abspath dc_offset_tb.sv) \
$(abspath rf_core_1000m_all_tb.sv) \


#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
