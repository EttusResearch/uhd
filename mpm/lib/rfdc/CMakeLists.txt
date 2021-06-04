#
# Copyright 2019 Ettus Research, National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0
#
set(RFDC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/rfdc_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rfdc_throw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_clock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_g.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_intr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_mb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_mts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc_sinit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xrfdc.c
)

USRP_PERIPHS_ADD_OBJECT(rfdc ${RFDC_SOURCES})
