#
# Copyright 2024 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

###############################################################################
# Build Python wrapper module for C++ -> Python bindings
###############################################################################
# Global Python API constants
set(PYMODULE_NAME rfnoc_gain)
set(SETUP_PY_IN    "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY       "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
# convert binary directory to native format to use in SETUP_PY file.
file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} NATIVE_CURRENT_BINARY_DIR)
configure_file(${SETUP_PY_IN} ${SETUP_PY})


###############################################################################
# Build Python wrapper module for C++ -> Python bindings
###############################################################################
pybind11_add_module(${PYMODULE_NAME}_python
  MODULE
  pyrfnoc-gain.cpp
)

target_include_directories(${PYMODULE_NAME}_python
  PUBLIC
  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_SOURCE_DIR}/include
  ${Python3_INCLUDE_DIRS}
  ${Python3_NumPy_INCLUDE_DIRS}
)
target_link_libraries(
  ${PYMODULE_NAME}_python
  PRIVATE
  pybind11::pybind11
  ${UHD_LIBRARIES}
)

# Copy pybind bindings library to the staging directory (it will get copied to
# its final destination further down)
add_custom_command(TARGET ${PYMODULE_NAME}_python
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PYMODULE_NAME}_python> ${CMAKE_CURRENT_BINARY_DIR}/${PYMODULE_NAME}/$<TARGET_FILE_NAME:${PYMODULE_NAME}_python>)


# List of Python files that are part of the module but don't get
# generated during build time.
# Note: When adding Python files into the module, they don't get added to the
# dependency list until CMake is re-run.
file(GLOB_RECURSE PYMODULE_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/${PYMODULE_NAME}/*.py
)

# If we're not in a virtual environment, then we need to figure out where to
# install the Python module.
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

# This copies the contents of the Python module into the build directory. We will
# use that as a staging ground for installing the final module to the system.
# We make sure that we always have an up-to-date copy in here.
add_custom_command(OUTPUT ${TIMESTAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${PYMODULE_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${PYMODULE_NAME}
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} -q build
  COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
  DEPENDS ${PYMODULE_FILE})

add_custom_target(pymodule_library
    ALL DEPENDS ${TIMESTAMP_FILE} ${PYMODULE_NAME}_python)

# Now install the Python module from the build directory to the final destination.
UHD_PYTHON_INSTALL_MODULE(
    MODULE "${PYMODULE_NAME}"
    LIBTARGET "${PYMODULE_NAME}_python"
)
