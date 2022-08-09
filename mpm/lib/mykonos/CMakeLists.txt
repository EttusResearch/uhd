#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

macro(MYKONOS_APPEND_SOURCES)
    set(mykonos_sources ${mykonos_sources})
    list(APPEND mykonos_sources ${ARGV})
    set(mykonos_sources ${mykonos_sources} PARENT_SCOPE)
endmacro(MYKONOS_APPEND_SOURCES)

set(mykonos_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_spi_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adi_ctrl.cpp
    ${UHD_SOURCE_DIR}/lib/types/ranges.cpp
)

add_subdirectory(adi)
add_subdirectory(config)

USRP_PERIPHS_ADD_OBJECT(mykonos ${mykonos_sources})
