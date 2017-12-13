#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

MACRO(MYKONOS_APPEND_SOURCES)
    SET(mykonos_sources ${mykonos_sources})
    LIST(APPEND mykonos_sources ${ARGV})
    SET(mykonos_sources ${mykonos_sources} PARENT_SCOPE)
ENDMACRO(MYKONOS_APPEND_SOURCES)

SET(mykonos_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_spi_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adi_ctrl.cpp
    ${UHD_HOST_ROOT}/lib/types/ranges.cpp
)

ADD_SUBDIRECTORY(adi)
ADD_SUBDIRECTORY(config)

USRP_PERIPHS_ADD_OBJECT(mykonos ${mykonos_sources})
