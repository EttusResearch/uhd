#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

# Global Python API constants
set(SETUP_PY_IN    "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY       "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
# convert binary directory to native format to use in SETUP_PY file.
file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} NATIVE_CURRENT_BINARY_DIR)
configure_file(${SETUP_PY_IN} ${SETUP_PY})

# If we're not in a virtual environment, then we need to figure out where to
# install the Python module. Do this here so we can use the value for UHD_PYTHON_DIR
# in the simulator.
if(NOT DEFINED UHD_PYTHON_DIR)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
        # Avoid the posix_local install scheme
        "import os,sysconfig;\
        install_scheme = 'posix_prefix';\
        platlib = sysconfig.get_path('platlib', scheme=install_scheme);\
        prefix = sysconfig.get_config_var('prefix');\
        print(os.path.relpath(platlib, prefix));"
        OUTPUT_VARIABLE UHD_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif(NOT DEFINED UHD_PYTHON_DIR)

# If ENABLE_PYTHON_API is OFF, then we can skip most of the work and will just
# install a bunch of Python files.
if(NOT ENABLE_PYTHON_API AND ENABLE_PYMOD_UTILS)
    # List of Python files that are part of the module but don't get
    # generated during build time and don't require libpyuhd.
    # Note: When adding Python files into uhd/, they don't get added to the
    # dependency list until CMake is re-run.
    file(GLOB_RECURSE PYUHD_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/uhd/dsp/*.py
        ${CMAKE_CURRENT_SOURCE_DIR}/uhd/imgbuilder/*.py
        ${CMAKE_CURRENT_SOURCE_DIR}/uhd/imgbuilder/*.mako
    )
    # This copies the contents of host/python/uhd into the build directory. We will
    # use that as a staging ground for installing the final module to the system.
    # We make sure that we always have an up-to-date copy in here.
    add_custom_command(OUTPUT ${TIMESTAMP_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/uhd/
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/uhd/__init__.py
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/uhd/imgbuilder ${CMAKE_CURRENT_BINARY_DIR}/uhd/imgbuilder
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/uhd/dsp ${CMAKE_CURRENT_BINARY_DIR}/uhd/dsp
        COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
        COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
        DEPENDS ${PYUHD_FILES})
    add_custom_target(pyuhd_library ALL DEPENDS ${TIMESTAMP_FILE})
    PYTHON_INSTALL_MODULE(
        MODULE "uhd"
    )
    return()
endif()


if(pybind11_FOUND)
  message(STATUS "Using Pybind11 from: ${pybind11_INCLUDE_DIR}")
else()
  set(pybind11_INCLUDE_DIR
      "${UHD_SOURCE_DIR}/lib/deps/pybind11/include"
      CACHE STRING
      "Location of PyBind11 includes"
  )
  message(STATUS "Using in-tree Pybind11.")
endif()

# Get include dirs
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c
    "try:\n import numpy\n import os\n inc_path = numpy.get_include()\n if os.path.exists(os.path.join(inc_path, 'numpy', 'arrayobject.h')):\n  print(inc_path, end='')\nexcept:\n pass"
    OUTPUT_VARIABLE PYTHON_NUMPY_INCLUDE_DIR)

# Build pyuhd library
add_library(pyuhd SHARED
    pyuhd.cpp
    ${UHD_SOURCE_DIR}/lib/property_tree_python.cpp
    ${UHD_SOURCE_DIR}/lib/device_python.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/multi_usrp_python.cpp
)

# python expects extension modules with a particular suffix
if(WIN32)
    set_target_properties(pyuhd PROPERTIES PREFIX "lib" SUFFIX ".pyd")
else()
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" -c
        "from sysconfig import get_config_var; print(get_config_var('EXT_SUFFIX'))"
        OUTPUT_VARIABLE PYTHON_EXTENSION_SUFFIX
    )
    string(STRIP ${PYTHON_EXTENSION_SUFFIX} PYTHON_EXTENSION_SUFFIX)
    if(${PYTHON_EXTENSION_SUFFIX} STREQUAL "None")
        set(PYTHON_EXTENSION_SUFFIX ${CMAKE_SHARED_MODULE_SUFFIX})
    endif()
    set_target_properties(pyuhd
        PROPERTIES
        PREFIX "lib"
        SUFFIX ${PYTHON_EXTENSION_SUFFIX}
    )
endif(WIN32)
target_include_directories(pyuhd PUBLIC
    ${PYTHON_INCLUDE_DIRS}
    ${PYTHON_NUMPY_INCLUDE_DIR}
    ${UHD_SOURCE_DIR}/lib
    ${pybind11_INCLUDE_DIR}
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

# List of Python files that are part of the module but don't get
# generated during build time.
# Note: When adding Python files into uhd/, they don't get added to the
# dependency list until CMake is re-run.
file(GLOB_RECURSE PYUHD_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/uhd/*.py
)

if(ENABLE_SIM)
    set(MPM_DEVICE "sim")
    # Whereever UHD Python gets installed, also install that from MPM.
    set(USRP_MPM_PYTHON_DIR ${UHD_PYTHON_DIR})
    add_subdirectory(${UHD_SOURCE_DIR}/../mpm/python simulator)
    # simulator/usrp_mpm needs to be copied to usrp_mpm because setuptools only detects import packages in the working directory
    add_custom_target(copy_mpm_packages ALL DEPENDS usrp_mpm)
    add_custom_command(TARGET copy_mpm_packages
                       PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/dboard_manager
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/periph_manager
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/simulator
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/sys_utils
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/xports

                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/dboard_manager/base.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/dboard_manager/__init__.py
                       ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/dboard_manager

                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/periph_manager/base.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/periph_manager/common.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/periph_manager/sim.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/periph_manager/__init__.py
                       ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/periph_manager

                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/simulator
                       ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/simulator

                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/sys_utils
                       ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/sys_utils

                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/xports/xportmgr_udp.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/xports/__init__.py
                       ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/xports

                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/compat_num.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/discovery.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/eeprom.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/ethdispatch.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/gpsd_iface.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/mpmlog.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/mpmtypes.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/mpmutils.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/prefs.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/tlv_eeprom.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/rpc_server.py
                       ${CMAKE_CURRENT_BINARY_DIR}/simulator/usrp_mpm/__init__.py
                       ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm)
    # Move usrp_hwd.py into usrp_mpm so that it is included in the package
    configure_file(${UHD_SOURCE_DIR}/../mpm/python/usrp_hwd.py
                   ${CMAKE_CURRENT_BINARY_DIR}/usrp_mpm/usrp_hwd.py COPYONLY)
    set(PYUHD_FILES ${PYUHD_FILES} copy_mpm_packages)
endif(ENABLE_SIM)

# This copies the contents of host/python/uhd into the build directory. We will
# use that as a staging ground for installing the final module to the system.
# We make sure that we always have an up-to-date copy in here.
add_custom_command(OUTPUT ${TIMESTAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/uhd ${CMAKE_CURRENT_BINARY_DIR}/uhd
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
  COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
  DEPENDS ${PYUHD_FILES})

add_custom_target(pyuhd_library ALL DEPENDS ${TIMESTAMP_FILE} pyuhd)

# Now install the Python module from the build directory to the final destination.
PYTHON_INSTALL_MODULE(
    MODULE "uhd"
    LIBTARGET pyuhd
)
