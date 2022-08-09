#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

# Check installation status of Python modules required by MPM unit tests
set(GEVENT_MIN_VERSION "1.4.0")
set(PYUDEV_MIN_VERSION "0.21.0")

PYTHON_CHECK_MODULE_VERSION(
    "gevent module"
    "gevent"
    "gevent.__version__"
    ${GEVENT_MIN_VERSION}
    HAVE_PYTHON_MODULE_GEVENT
)

# mprpc does not expose a __version__ attribute, so merely check for its
# presence
PYTHON_CHECK_MODULE(
    "mprpc module"
    "mprpc"
    "True"
    HAVE_PYTHON_MODULE_MPRPC
)

PYTHON_CHECK_MODULE_VERSION(
    "pyudev module"
    "pyudev"
    "pyudev.__version__"
    ${PYUDEV_MIN_VERSION}
    HAVE_PYTHON_MODULE_PYUDEV
)

# Older versions of pyroute2 (e.g. 0.5.2) do not expose a __version__
# attribute, so merely check for its presence.
PYTHON_CHECK_MODULE(
    "pyroute2 module"
    "pyroute2"
    "True"
    HAVE_PYTHON_MODULE_PYROUTE2
)

if (HAVE_PYTHON_MODULE_GEVENT AND HAVE_PYTHON_MODULE_MPRPC AND HAVE_PYTHON_MODULE_PYUDEV AND HAVE_PYTHON_MODULE_PYROUTE2)
    set(HAVE_MPM_TEST_PREREQS TRUE)
    MESSAGE(STATUS "MPM unit test Python package prerequisites met!")
else()
    set(HAVE_MPM_TEST_PREREQS FALSE)
    MESSAGE(STATUS "MPM unit test Python package prerequisites not met; skipping")
endif()
MESSAGE(STATUS "")

if(MPM_DEVICE STREQUAL "n3xx")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/n3xx/pyusrp_periphs.cpp)
elseif(MPM_DEVICE STREQUAL "e320")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/e320/pyusrp_periphs.cpp)
elseif(MPM_DEVICE STREQUAL "e31x")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/e31x/pyusrp_periphs.cpp)
elseif(MPM_DEVICE STREQUAL "x4xx")
    add_library(pyusrp_periphs SHARED pyusrp_periphs/x4xx/pyusrp_periphs.cpp)
endif(MPM_DEVICE STREQUAL "n3xx")

if(NOT ENABLE_SIM)
    target_include_directories(pyusrp_periphs PUBLIC
        ${PYTHON_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/lib/
        ${UHD_SOURCE_DIR}/lib/usrp/common
        ${UHD_SOURCE_DIR}/lib/usrp/common/ad9361_driver
        ${UHD_SOURCE_DIR}/lib/deps/pybind11/include
    )
    target_link_libraries(pyusrp_periphs ${Boost_LIBRARIES} usrp-periphs)
    add_custom_command(TARGET pyusrp_periphs POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libpyusrp_periphs.so ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/libpyusrp_periphs.so)
endif(NOT ENABLE_SIM)

set(USRP_MPM_FILES "")
add_subdirectory(usrp_mpm)

set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(PERIPH_MGR_INIT_IN "${CMAKE_CURRENT_SOURCE_DIR}/usrp_mpm/periph_manager/__init__.py.in")
set(PERIPH_MGR_INIT "${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/periph_manager/__init__.py")
set(USRP_MPM_INIT_IN "${CMAKE_CURRENT_SOURCE_DIR}/usrp_mpm/__init__.py.in")
set(USRP_MPM_INIT "${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/__init__.py")

configure_file(${SETUP_PY_IN} ${SETUP_PY})
configure_file(${PERIPH_MGR_INIT_IN} ${PERIPH_MGR_INIT})
configure_file(${USRP_MPM_INIT_IN} ${USRP_MPM_INIT})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/usrp_hwd.py" "${CMAKE_CURRENT_BINARY_DIR}/usrp_hwd.py" COPYONLY)

add_custom_command(OUTPUT ${OUTPUT}
     COMMAND ${CMAKE_COMMAND}  -DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DBINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"  -P ${CMAKE_CURRENT_SOURCE_DIR}/copy_python_module.cmake
     COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
     COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
     DEPENDS ${USRP_MPM_FILES} pyusrp_periphs)
add_custom_target(usrp_mpm ALL DEPENDS ${OUTPUT} pyusrp_periphs)

if(NOT DEFINED USRP_MPM_PYTHON_DIR)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
        "import os,sysconfig;\
        platlib = sysconfig.get_path(name='platlib');\
        prefix = sysconfig.get_config_var('prefix');\
        print(os.path.relpath(platlib, prefix));"
        OUTPUT_VARIABLE USRP_MPM_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif(NOT DEFINED USRP_MPM_PYTHON_DIR)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/lib/usrp_mpm DESTINATION ${USRP_MPM_PYTHON_DIR})
install(PROGRAMS
    usrp_hwd.py
    DESTINATION ${RUNTIME_DIR}
)

if(NOT ENABLE_SIM)
    install(PROGRAMS
        aurora_bist_test.py
        usrp_update_fs
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
    elseif (ENABLE_X400)
        install(PROGRAMS
            x4xx_bist
            DESTINATION ${RUNTIME_DIR}
        )
        install(PROGRAMS
            usrp_mpm/dboard_manager/zbx_update_cpld.py
            DESTINATION ${RUNTIME_DIR}
            RENAME zbx_update_cpld
        )
        install(PROGRAMS
            usrp_mpm/periph_manager/x4xx_update_cpld.py
            DESTINATION ${RUNTIME_DIR}
            RENAME x4xx_update_cpld
        )
    endif (ENABLE_MYKONOS)
endif()

if (HAVE_MPM_TEST_PREREQS)
    add_subdirectory(tests)
endif (HAVE_MPM_TEST_PREREQS)

