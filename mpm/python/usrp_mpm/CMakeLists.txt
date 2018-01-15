#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
SET(USRP_MPM_FILES ${USRP_MPM_FILES})
SET(USRP_MPM_TOP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/aurora_control.py
    ${CMAKE_CURRENT_SOURCE_DIR}/bfrfs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/discovery.py
    ${CMAKE_CURRENT_SOURCE_DIR}/eeprom.py
    ${CMAKE_CURRENT_SOURCE_DIR}/ethtable.py
    ${CMAKE_CURRENT_SOURCE_DIR}/gpsd_iface.py
    ${CMAKE_CURRENT_SOURCE_DIR}/liberiotable.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmlog.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmtypes.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmutils.py
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_server.py
)
LIST(APPEND USRP_MPM_FILES ${USRP_MPM_TOP_FILES})
ADD_SUBDIRECTORY(chips)
ADD_SUBDIRECTORY(cores)
ADD_SUBDIRECTORY(dboard_manager)
ADD_SUBDIRECTORY(periph_manager)
ADD_SUBDIRECTORY(sys_utils)
ADD_SUBDIRECTORY(xports)
SET(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)
