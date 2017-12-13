#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################


USRP_PERIPHS_ADD_OBJECT(tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests_spi_iface.cpp)
