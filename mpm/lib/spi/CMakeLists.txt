#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
SET(SPI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/spidev_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spi_regs_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spidev.c
)

USRP_PERIPHS_ADD_OBJECT(spi ${SPI_SOURCES})
