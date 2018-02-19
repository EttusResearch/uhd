#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_basic_funcs.py
    ${CMAKE_CURRENT_BINARY_DIR}/basic_functions.hpp
)

IF(ENABLE_MANUAL)
    LIBUHD_PYTHON_GEN_SOURCE(
        ${CMAKE_CURRENT_SOURCE_DIR}/gen_basic_funcs.py
        ${CMAKE_BINARY_DIR}/docs/nocscript_functions.dox
    )
ENDIF(ENABLE_MANUAL)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/function_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_iface.cpp
)
