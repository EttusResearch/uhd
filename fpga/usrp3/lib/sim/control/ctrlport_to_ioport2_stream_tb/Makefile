#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
BASE_DIR = $(abspath ../../../../top)
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS) \
$(CTRLPORT_TO_IOPORT2_STREAM_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = ctrlport_to_ioport2_stream_all_tb

SIM_SRCS = $(abspath \
ctrlport_to_ioport2_stream_tb.sv \
ctrlport_to_ioport2_stream_all_tb.sv \
)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
include $(BASE_DIR)/../tools/make/viv_simulator.mak
