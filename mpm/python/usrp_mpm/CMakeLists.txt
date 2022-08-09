#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
set(USRP_MPM_FILES ${USRP_MPM_FILES})
set(USRP_MPM_TOP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in
    ${CMAKE_CURRENT_SOURCE_DIR}/aurora_control.py
    ${CMAKE_CURRENT_SOURCE_DIR}/bfrfs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/bist.py
    ${CMAKE_CURRENT_SOURCE_DIR}/components.py
    ${CMAKE_CURRENT_SOURCE_DIR}/compat_num.py
    ${CMAKE_CURRENT_SOURCE_DIR}/discovery.py
    ${CMAKE_CURRENT_SOURCE_DIR}/eeprom.py
    ${CMAKE_CURRENT_SOURCE_DIR}/e31x_legacy_eeprom.py
    ${CMAKE_CURRENT_SOURCE_DIR}/ethdispatch.py
    ${CMAKE_CURRENT_SOURCE_DIR}/fpga_bit_to_bin.py
    ${CMAKE_CURRENT_SOURCE_DIR}/gpsd_iface.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmlog.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmtypes.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmutils.py
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/process_manager.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_server.py
    ${CMAKE_CURRENT_SOURCE_DIR}/tlv_eeprom.py
    ${CMAKE_CURRENT_SOURCE_DIR}/user_eeprom.py
)
list(APPEND USRP_MPM_FILES ${USRP_MPM_TOP_FILES})
add_subdirectory(chips)
add_subdirectory(cores)
add_subdirectory(dboard_manager)
add_subdirectory(periph_manager)
add_subdirectory(sys_utils)
add_subdirectory(xports)
add_subdirectory(simulator)
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)
