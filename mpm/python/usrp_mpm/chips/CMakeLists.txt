#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(USRP_MPM_FILES ${USRP_MPM_FILES})
set(USRP_MPM_CHIP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/adf400x.py
    ${CMAKE_CURRENT_SOURCE_DIR}/ds125df410.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk04828.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk04832.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk03328.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk05318.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmx2572.py
    ${CMAKE_CURRENT_SOURCE_DIR}/max10_cpld_flash_ctrl.py
)
list(APPEND USRP_MPM_FILES ${USRP_MPM_CHIP_FILES})
add_subdirectory(ic_reg_maps)
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)
