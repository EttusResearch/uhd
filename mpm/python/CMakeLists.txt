#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

if(MPM_DEVICE STREQUAL "n3xx")
    ADD_LIBRARY(pyusrp_periphs SHARED pyusrp_periphs/n3xx/pyusrp_periphs.cpp)
endif(MPM_DEVICE STREQUAL "n3xx")

TARGET_INCLUDE_DIRECTORIES(pyusrp_periphs PUBLIC
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/
    ${UHD_HOST_ROOT}/lib/usrp/common
)
TARGET_LINK_LIBRARIES(pyusrp_periphs ${Boost_PYTHON_LIBRARY} ${Boost_LIBRARIES} usrp-periphs)
ADD_CUSTOM_COMMAND(TARGET pyusrp_periphs POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libpyusrp_periphs.so ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/libpyusrp_periphs.so)

SET(USRP_MPM_FILES "")
ADD_SUBDIRECTORY(usrp_mpm)

SET(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
SET(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
SET(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
SET(PERIPH_MGR_INIT_IN "${CMAKE_CURRENT_SOURCE_DIR}/usrp_mpm/periph_manager/__init__.py.in")
SET(PERIPH_MGR_INIT "${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/periph_manager/__init__.py")

CONFIGURE_FILE(${SETUP_PY_IN} ${SETUP_PY})
CONFIGURE_FILE(${PERIPH_MGR_INIT_IN} ${PERIPH_MGR_INIT})
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/usrp_hwd.py" "${CMAKE_CURRENT_BINARY_DIR}/usrp_hwd.py" COPYONLY)

ADD_CUSTOM_COMMAND(OUTPUT ${OUTPUT}
     COMMAND ${CMAKE_COMMAND}  -DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DBINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"  -P ${CMAKE_CURRENT_SOURCE_DIR}/copy_python_module.cmake
     COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
     COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
     DEPENDS ${USRP_MPM_FILES})
ADD_CUSTOM_TARGET(usrp_mpm ALL DEPENDS ${OUTPUT} pyusrp_periphs)

EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
    "from __future__ import print_function; from distutils import sysconfig; print(sysconfig.get_python_lib(plat_specific=True, prefix=''))"
    OUTPUT_VARIABLE USRP_MPM_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
)
INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/lib/usrp_mpm DESTINATION ${CMAKE_INSTALL_PREFIX}/${USRP_MPM_PYTHON_DIR})
INSTALL(PROGRAMS
    aurora_bist_test.py
    n3xx_bist
    usrp_hwd.py
    DESTINATION ${RUNTIME_DIR}
)
