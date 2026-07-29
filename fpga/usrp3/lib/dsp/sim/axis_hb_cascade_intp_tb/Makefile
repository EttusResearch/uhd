#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# -------------------------------------------------
# Top-of-Makefile
# -------------------------------------------------
BASE_DIR = $(abspath ../../../../top)
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

# -------------------------------------------------
# Design specific
# -------------------------------------------------
include $(BASE_DIR)/../lib/dsp/hb_filter/Makefile.srcs

# -------------------------------------------------
# Testbench specific
# -------------------------------------------------
SIM_TOP = axis_hb_cascade_intp_all_tb

SIM_SRCS = \
$(abspath ../axis_hb_cascade_decim_tb/axis_hb_decim_fir_tb/axis_hb_test_pkg.sv) \
$(abspath axis_hb_cascade_intp_tb.sv) \
$(abspath axis_hb_cascade_intp_all_tb.sv) \

# -------------------------------------------------
# Bottom-of-Makefile
# -------------------------------------------------
include $(BASE_DIR)/../tools/make/viv_simulator.mak

# -------------------------------------------------
# Debugging of Makefile
# -------------------------------------------------
print-%: ; @echo $* is $($*)
