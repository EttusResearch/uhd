#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

PYTHON_CHECK_MODULE(
    "virtualenv"
    "sys" "hasattr(sys, 'real_prefix')"
    HAVE_PYTHON_VIRTUALENV
)

# Get include dirs
include_directories(${PYTHON_INCLUDE_DIRS})
set(PYBIND11_INCLUDE_DIR
    "${UHD_SOURCE_DIR}/lib/deps/pybind11/include"
    CACHE
    STRING
    "Location of PyBind11 includes"
)
include_directories(${PYBIND11_INCLUDE_DIR})
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c
    "from __future__ import print_function\ntry:\n import numpy\n import os\n inc_path = numpy.get_include()\n if os.path.exists(os.path.join(inc_path, 'numpy', 'arrayobject.h')):\n  print(inc_path, end='')\nexcept:\n pass"
    OUTPUT_VARIABLE PYTHON_NUMPY_INCLUDE_DIR)

# Build pyuhd library
add_library(pyuhd SHARED pyuhd.cpp)
# python expects extension modules with a particular suffix
if(WIN32)
    set_target_properties(pyuhd PROPERTIES PREFIX "lib" SUFFIX ".pyd")
else()
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" -c
        "from distutils.sysconfig import get_config_var; print(get_config_var('EXT_SUFFIX'))"
        OUTPUT_VARIABLE PYTHON_EXTENSION_SUFFIX
    )
    string(STRIP ${PYTHON_EXTENSION_SUFFIX} PYTHON_EXTENSION_SUFFIX)
    if(${PYTHON_EXTENSION_SUFFIX} STREQUAL "None")
        set(PYTHON_EXTENSION_SUFFIX ${CMAKE_SHARED_MODULE_SUFFIX})
    endif()
    set_target_properties(pyuhd PROPERTIES PREFIX "lib" SUFFIX ${PYTHON_EXTENSION_SUFFIX})
endif(WIN32)
target_include_directories(pyuhd PUBLIC
    ${PYTHON_NUMPY_INCLUDE_DIR}
    ${UHD_SOURCE_DIR}/lib
    ${PYBIND11_INCLUDE_DIR}
)

if(WIN32)
    target_link_libraries(pyuhd ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} uhd)
else()
    # for extension module, proper to NOT link against python library and instead
    # add dynamic lookup link option on OSX
    target_link_libraries(pyuhd ${Boost_LIBRARIES} uhd)
    if(APPLE)
        target_link_options(pyuhd PRIVATE "LINKER:-undefined,dynamic_lookup")
    endif(APPLE)
endif(WIN32)
# Copy pyuhd library to the staging directory
add_custom_command(TARGET pyuhd
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pyuhd> ${CMAKE_CURRENT_BINARY_DIR}/uhd/$<TARGET_FILE_NAME:pyuhd>)

set(PYUHD_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/types.py
  ${CMAKE_CURRENT_SOURCE_DIR}/usrp.py
  ${CMAKE_CURRENT_SOURCE_DIR}/filters.py
)

set(SETUP_PY_IN    "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY       "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
# convert binary directory to native format to use in SETUP_PY file.
file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} NATIVE_CURRENT_BINARY_DIR)
configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT ${TIMESTAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E copy ${PYUHD_FILES} ${CMAKE_CURRENT_BINARY_DIR}/uhd
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
  COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
  DEPENDS ${PYUHD_FILES})

add_custom_target(pyuhd_library ALL DEPENDS ${TIMESTAMP_FILE} pyuhd)
if(HAVE_PYTHON_VIRTUALENV)
    # In virtualenvs, let setuptools do its thing
    install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q install --force)")
else()
    # Otherwise, use distutils to determine the correct relative path for Python
    # packages, and install to our prefix
    if(NOT DEFINED UHD_PYTHON_DIR)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
            "from __future__ import print_function;\
            from distutils import sysconfig;\
            print(sysconfig.get_python_lib(plat_specific=True, prefix='${CMAKE_INSTALL_PREFIX}'));"
            OUTPUT_VARIABLE UHD_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif(NOT DEFINED UHD_PYTHON_DIR)
    file(TO_CMAKE_PATH ${UHD_PYTHON_DIR} UHD_PYTHON_DIR)

    message(STATUS "Utilizing the python install directory: ${UHD_PYTHON_DIR}")
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/uhd DESTINATION ${UHD_PYTHON_DIR} COMPONENT pythonapi)
endif(HAVE_PYTHON_VIRTUALENV)
