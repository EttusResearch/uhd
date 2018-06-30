#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
MYKONOS_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/mykonos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mykonos_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mykonos_user.c
)

ADD_SUBDIRECTORY(mykonos_debug)
