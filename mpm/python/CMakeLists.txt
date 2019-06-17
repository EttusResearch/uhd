#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

if(MPM_DEVICE STREQUAL "n3xx")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/n3xx/pyusrp_periphs.cpp)
elseif(MPM_DEVICE STREQUAL "e320")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/e320/pyusrp_periphs.cpp)
elseif(MPM_DEVICE STREQUAL "e31x")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/e31x/pyusrp_periphs.cpp)
endif(MPM_DEVICE STREQUAL "n3xx")

target_include_directories(pyusrp_periphs PUBLIC
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/
    ${UHD_HOST_ROOT}/lib/usrp/common
    ${UHD_HOST_ROOT}/lib/usrp/common/ad9361_driver
    ${UHD_HOST_ROOT}/lib/deps/pybind11/include
)
target_link_libraries(pyusrp_periphs ${Boost_LIBRARIES} usrp-periphs)
add_custom_command(TARGET pyusrp_periphs POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libpyusrp_periphs.so ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/libpyusrp_periphs.so)

set(USRP_MPM_FILES "")
add_subdirectory(usrp_mpm)

set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(PERIPH_MGR_INIT_IN "${CMAKE_CURRENT_SOURCE_DIR}/usrp_mpm/periph_manager/__init__.py.in")
set(PERIPH_MGR_INIT "${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/periph_manager/__init__.py")

configure_file(${SETUP_PY_IN} ${SETUP_PY})
configure_file(${PERIPH_MGR_INIT_IN} ${PERIPH_MGR_INIT})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/usrp_hwd.py" "${CMAKE_CURRENT_BINARY_DIR}/usrp_hwd.py" COPYONLY)

add_custom_command(OUTPUT ${OUTPUT}
     COMMAND ${CMAKE_COMMAND}  -DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DBINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"  -P ${CMAKE_CURRENT_SOURCE_DIR}/copy_python_module.cmake
     COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
     COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
     DEPENDS ${USRP_MPM_FILES})
add_custom_target(usrp_mpm ALL DEPENDS ${OUTPUT} pyusrp_periphs)

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
    "from __future__ import print_function; from distutils import sysconfig; print(sysconfig.get_python_lib(plat_specific=True, prefix=''))"
    OUTPUT_VARIABLE USRP_MPM_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/lib/usrp_mpm DESTINATION ${USRP_MPM_PYTHON_DIR})
install(PROGRAMS
    aurora_bist_test.py
    usrp_update_fs
    usrp_hwd.py
    DESTINATION ${RUNTIME_DIR}
)

if (ENABLE_MYKONOS)
    install(PROGRAMS
        n3xx_bist
        DESTINATION ${RUNTIME_DIR}
    )
elseif (ENABLE_E320)
    install(PROGRAMS
        e320_bist
        DESTINATION ${RUNTIME_DIR}
    )
endif (ENABLE_MYKONOS)

add_subdirectory(tests)
