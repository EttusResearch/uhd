#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description: X410 variant of the X4xx GPIO SPI simulation shell.
#

VLOG_ARGS += +define+X410
SVLOG_ARGS += +define+X410

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
BASE_DIR = $(abspath ../../../../../top)
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

include ../x4xx/Makefile.common

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
include $(BASE_DIR)/../tools/make/viv_simulator.mak
