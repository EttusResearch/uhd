#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(USRP_MPM_FILES ${USRP_MPM_FILES})
set(USRP_MPM_SYSUTILS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/dtoverlay.py
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c_dev.py
    ${CMAKE_CURRENT_SOURCE_DIR}/net.py
    ${CMAKE_CURRENT_SOURCE_DIR}/sysfs_gpio.py
    ${CMAKE_CURRENT_SOURCE_DIR}/sysfs_thermal.py
    ${CMAKE_CURRENT_SOURCE_DIR}/udev.py
    ${CMAKE_CURRENT_SOURCE_DIR}/uio.py
    ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.py
    ${CMAKE_CURRENT_SOURCE_DIR}/ectool.py
)
list(APPEND USRP_MPM_FILES ${USRP_MPM_SYSUTILS_FILES})
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

