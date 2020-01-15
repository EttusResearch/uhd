#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
set(I2C_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/i2cdev_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c_regs_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/i2cdev.c
)
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/i2cdev.c
    PROPERTIES COMPILE_FLAGS "-D_GNU_SOURCE"
)

USRP_PERIPHS_ADD_OBJECT(i2c ${I2C_SOURCES})
