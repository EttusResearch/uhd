#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
SET(TYPES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lockable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log_buf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mmap_regs_iface.cpp
)

USRP_PERIPHS_ADD_OBJECT(types ${TYPES_SOURCES})
