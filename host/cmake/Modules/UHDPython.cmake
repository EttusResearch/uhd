#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Note: This file gets installed into lib/cmake/uhd, and used by RFNoC OOT
# modules, so keep that in mind when editing this file!

if (POLICY CMP0094)
  # See https://cmake.org/cmake/help/v3.15/policy/CMP0094.html
  # set Python3_FIND_STRATEGY to LOCATION - this ensures that Python from
  # sysroot is used first when cross-compiling
  # note: policy CMP0094 is available starting with CMake 3.15
  cmake_policy(SET CMP0094 NEW)
endif()

if(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
set(INCLUDED_UHD_PYTHON_CMAKE TRUE)

if(NOT DEFINED UHD_PYTHON_MIN_VERSION OR NOT DEFINED UHD_PYBIND11_MIN_VERSION)
  include(UHDMinDepVersions)
endif()

########################################################################
# Setup Python Part 1: Find the interpreters
########################################################################
message(STATUS "")
message(STATUS "Configuring the Python interpreter...")
#this allows the user to override PYTHON_EXECUTABLE
if(PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND TRUE)
endif(PYTHON_EXECUTABLE)

if(NOT PYTHONINTERP_FOUND)
    find_package(Python3 ${UHD_PYTHON_MIN_VERSION} QUIET)
    if(Python3_Interpreter_FOUND)
        set(PYTHON_VERSION ${Python3_VERSION})
        set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
        set(PYTHONINTERP_FOUND TRUE)
    endif(Python3_Interpreter_FOUND)
endif(NOT PYTHONINTERP_FOUND)

# If that fails, try using the build-in find program routine.
if(NOT PYTHONINTERP_FOUND)
    message(STATUS "Attempting to find Python without CMake...")
    find_program(PYTHON_EXECUTABLE NAMES python3 python3.7 python3.8 python3.9 python3.10)
    if(PYTHON_EXECUTABLE)
        set(PYTHONINTERP_FOUND TRUE)
    endif(PYTHON_EXECUTABLE)
endif(NOT PYTHONINTERP_FOUND)

if(NOT PYTHON_VERSION)
    message(STATUS "Manually determining build Python version...")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "
import sys
print('{}.{}.{}'.format(
    sys.version_info.major,
    sys.version_info.minor,
    sys.version_info.micro))"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT PYTHON_VERSION)

string(REPLACE "." ";" PYTHON_VERSION_LIST "${PYTHON_VERSION}")
list(LENGTH PYTHON_VERSION_LIST PYTHON_VERSION_LIST_LEN)
if(PYTHON_VERSION_LIST_LEN GREATER 1)
    list(GET PYTHON_VERSION_LIST 0 Python_VERSION_MAJOR)
    list(GET PYTHON_VERSION_LIST 1 Python_VERSION_MINOR)
else(PYTHON_VERSION_LIST_LEN GREATER 1)
    set(Python_VERSION_MAJOR "UNKNOWN")
    set(Python_VERSION_MINOR "UNKNOWN")
endif(PYTHON_VERSION_LIST_LEN GREATER 1)

# If we still haven't found a Python interpreter, then we're done.
if(NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Error: Python interpreter required by the build system.")
endif(NOT PYTHONINTERP_FOUND)
if(NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Error: Python interpreter required by the build system.")
endif(NOT PYTHON_EXECUTABLE)

#make the path to the executable appear in the cmake gui
set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH
    "python buildtime interpreter")

message(STATUS "Python interpreter: ${PYTHON_EXECUTABLE} Version: ${PYTHON_VERSION}")
message(STATUS "Override with: -DPYTHON_EXECUTABLE=<path-to-python>")

#this allows the user to override RUNTIME_PYTHON_EXECUTABLE
if(NOT RUNTIME_PYTHON_EXECUTABLE)
    if(CMAKE_CROSSCOMPILING)
        message(STATUS "Cross compiling, setting python runtime to /usr/bin/python3")
        message(STATUS "and interpreter to min. required version ${UHD_PYTHON_MIN_VERSION}")
        message(STATUS "If this is not what you want, please set RUNTIME_PYTHON_EXECUTABLE")
        message(STATUS "and RUNTIME_PYTHON_VERSION manually")
        set(RUNTIME_PYTHON_EXECUTABLE "/usr/bin/python3")
        set(RUNTIME_PYTHON_VERSION ${UHD_PYTHON_MIN_VERSION})
        set(EXACT_ARGUMENT "")
    else(CMAKE_CROSSCOMPILING)
        #default to the buildtime interpreter
        set(RUNTIME_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
        set(RUNTIME_PYTHON_VERSION ${PYTHON_VERSION})
        set(EXACT_ARGUMENT "EXACT")
    endif(CMAKE_CROSSCOMPILING)
else(NOT RUNTIME_PYTHON_EXECUTABLE)
    set(EXACT_ARGUMENT "EXACT")
endif(NOT RUNTIME_PYTHON_EXECUTABLE)

if(NOT RUNTIME_PYTHON_VERSION)
    message(STATUS "Manually determining runtime Python version...")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "
from __future__ import print_function
import sys
print('{}.{}.{}'.format(
    sys.version_info.major,
    sys.version_info.minor,
    sys.version_info.micro))"
        OUTPUT_VARIABLE RUNTIME_PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT RUNTIME_PYTHON_VERSION)

#make the path to the executable appear in the cmake gui
set(RUNTIME_PYTHON_EXECUTABLE ${RUNTIME_PYTHON_EXECUTABLE} CACHE FILEPATH
    "python runtime interpreter")

message(STATUS "Python runtime interpreter: ${RUNTIME_PYTHON_EXECUTABLE} Version: ${RUNTIME_PYTHON_VERSION}")
message(STATUS "Override with: -DRUNTIME_PYTHON_EXECUTABLE=<path-to-python>")

###############################################################################
# Determine if a Python module is installed, or, more generally, determine
# if some condition that Python can report through a Boolean expression is
# met. This macro allows one or more modules to be imported and a Python
# Boolean expression to be evaluated.
#
# - desc:
#    Description of what's being checked (for user feedback)
# - module:
#    The module(s) to be passed to the `import` command
# - bool_expr:
#    A Python expression to be evaluated that returns True or False based on
#    the presence or absence of the module (or in the general case, the
#    condition being checked)
# - have_ver:
#    The variable name to be set to TRUE if the Python expression returns True,
#    or FALSE otherwise
macro(UHD_PYTHON_CHECK_MODULE desc module bool_expr have_var)
    message(STATUS "")
    message(STATUS "Python checking for ${desc}")
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "
#########################################
try:
    import ${module}
except:
    exit(1)
try:
    assert ${bool_expr}
except:
    exit(2)
exit(0)
#########################################"
        RESULT_VARIABLE python_result
    )
    if(python_result EQUAL 0)
        message(STATUS "Python checking for ${desc} - found")
        set(${have_var} TRUE)
    elseif(python_result EQUAL 1)
        message(STATUS "Python checking for ${desc} - \"import ${module}\" failed (is it installed?)")
        set(${have_var} FALSE)
    elseif(python_result EQUAL 2)
        message(STATUS "Python checking for ${desc} - \"assert ${bool_expr}\" failed")
        set(${have_var} FALSE)
    else()
        message(STATUS "Python checking for ${desc} - unknown error")
        set(${have_var} FALSE)
    endif()
endmacro()


###############################################################################
# Determine if a Python module is installed and if it meets a minimum required
# version.
#
# - desc:
#    Description of what's being checked (for user feedback)
# - module:
#    The module to be `import`ed
# - module_version_expr:
#    A Python expression to be evaluated that returns the module version string
#    (usually "module_name.__version__", but may be tailored for non-conformant
#    modules, or other custom use cases)
# - min_module_version:
#    The minimum version required of the module as a canonical Python version
#    string ("major.minor.micro") as defined in PEP 440
# - have_ver:
#    The variable name to be set to TRUE if the module is present and meets
#    the minimum version requirement or FALSE otherwise
macro(UHD_PYTHON_CHECK_MODULE_VERSION desc module module_version_expr min_module_version have_var)
    message(STATUS "")
    message(STATUS "Python checking for ${desc}")
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "
#########################################
try:
    import ${module}
except:
    exit(1)
try:
    version = ${module_version_expr}
    print(version)
except:
    exit(2)
exit(0)
#########################################"
        RESULT_VARIABLE python_result
        OUTPUT_VARIABLE version_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(python_result EQUAL 0)
        if(${version_output} VERSION_GREATER_EQUAL ${min_module_version})
            message(STATUS "Python checking for ${desc} - ${version_output} satisfies minimum required version ${min_module_version}")
            set(${have_var} TRUE)
        else()
            message(STATUS "Python checking for ${desc} - ${version_output} does not satisfy minimum required version ${min_module_version}")
            set(${have_var} FALSE)
        endif()
    elseif(python_result EQUAL 1)
        message(STATUS "Python checking for ${desc} - \"import ${module}\" failed (is it installed?)")
        set(${have_var} FALSE)
    elseif(python_result EQUAL 2)
        message(STATUS "Python checking for ${desc} - evaluation of \"${module_version_expr}\" failed")
        set(${have_var} FALSE)
    else()
        message(STATUS "Python checking for ${desc} - unknown error")
        set(${have_var} FALSE)
    endif()
endmacro()


###############################################################################
# Install a Python module into a system/prefix/virtualenv location.
#
# - LIBTARGET: Is there a library target included in this module (e.g., pyuhd)?
#              If so, state its name here. It will update RPATH on Linux/Unix
#              systems.
# - MODULE: Name of module (e.g., 'uhd')
macro(UHD_PYTHON_INSTALL_MODULE)
    if (NOT WIN32)
        cmake_parse_arguments(
            _py_install_mod
            "" "LIBTARGET;MODULE" ""
            ${ARGN}
        )

        # Check if we're in a virtual environment -- the rules are a bit different
        # there.
        UHD_PYTHON_CHECK_MODULE(
            "virtual environment"
            "sys"
            "sys.prefix != sys.base_prefix"
            HAVE_PYTHON_VIRTUALENV
        )

        if(HAVE_PYTHON_VIRTUALENV)
            message(
            STATUS
            "Python virtual environment detected -- Ignoring UHD_PYTHON_DIR.")
            # In virtualenvs, let setuptools do its thing
            install(CODE "message(\"Installing ${_py_install_mod_MODULE} Python module into venv via pip.\")")
            install(CODE
                "execute_process(COMMAND pip3 install . --force-reinstall WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
        else()
            # Otherwise, use sysconfig to determine the correct relative path for Python
            # packages, and install to our prefix
            if(NOT DEFINED UHD_PYTHON_DIR)
                execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                    # Avoid the posix_local install scheme
                    "import os,sysconfig;\
                    install_scheme = 'posix_user';\
                    platlib = sysconfig.get_path('platlib', scheme=install_scheme);\
                    prefix = sysconfig.get_config_var('prefix');\
                    print(os.path.relpath(platlib, prefix));"
                    OUTPUT_VARIABLE UHD_PYTHON_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
            endif(NOT DEFINED UHD_PYTHON_DIR)
            file(TO_CMAKE_PATH ${UHD_PYTHON_DIR} UHD_PYTHON_DIR)

            message(
                STATUS
                "Installing '${_py_install_mod_MODULE}' Python module to: "
                "${CMAKE_INSTALL_PREFIX}/${UHD_PYTHON_DIR}")
            # We use sysconfig (above) to figure out the destination path, and then
            # we simply copy this module recursively into its final destination.
            install(DIRECTORY
                ${CMAKE_CURRENT_BINARY_DIR}/${_py_install_mod_MODULE}
                DESTINATION ${UHD_PYTHON_DIR}
                COMPONENT pythonapi
            )
            # On Linux/Unix systems, we must properly install the library file.
            # install(DIRECTORY) will treat the .so file like any other file, which
            # means it won't update its RPATH, and thus the RPATH would be stuck to the
            # build directory.
            if(UNIX AND _py_install_mod_LIBTARGET)
                install(TARGETS ${_py_install_mod_LIBTARGET}
                    DESTINATION ${UHD_PYTHON_DIR}/${_py_install_mod_MODULE}
                )
            endif()
        endif(HAVE_PYTHON_VIRTUALENV)
    endif(NOT WIN32)
endmacro()

###############################################################################
# Part 2: Python Libraries
###############################################################################
# The libraries must match the RUNTIME_PYTHON_EXECUTABLE's version.
# - Figure out version
# - See if Python3_LIBRARIES is already set
if(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
    message(STATUS "Finding Python Libraries...")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14 AND DEFINED UHD_NUMPY_MIN_VERSION)
      set(_findpython_numpy_component NumPy)
    endif()
    find_package(Python3 ${RUNTIME_PYTHON_VERSION}
        ${EXACT_ARGUMENT}
        QUIET
        COMPONENTS Interpreter Development ${_findpython_numpy_component})
    if(Python3_Development_FOUND)
        set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
        if(Python3_NumPy_FOUND)
            set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS})
        else()
            set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
        endif()
    endif(Python3_Development_FOUND)
    if(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
        message(STATUS "Could not find Python Libraries.")
    endif(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
endif(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)

if(PYTHON_LIBRARIES AND PYTHON_INCLUDE_DIRS)
    set(HAVE_PYTHON_LIBS TRUE)
    message(STATUS "Python Libraries: ${PYTHON_LIBRARIES}")
    message(STATUS "Python include directories: ${PYTHON_INCLUDE_DIRS}")
else(PYTHON_LIBRARIES AND PYTHON_INCLUDE_DIRS)
    set(HAVE_PYTHON_LIBS FALSE)
endif(PYTHON_LIBRARIES AND PYTHON_INCLUDE_DIRS)

if(NOT PYTHON_LIBRARY)
    set(PYTHON_LIBRARIES ${PYTHON_LIBRARIES} CACHE FILEPATH
        "Python libraries")
    mark_as_advanced(PYTHON_LIBRARIES)
endif(NOT PYTHON_LIBRARY)
if(NOT PYTHON_INCLUDE_DIR)
    set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS} CACHE FILEPATH
        "Python include dirs")
    mark_as_advanced(PYTHON_INCLUDE_DIRS)
endif(NOT PYTHON_INCLUDE_DIR)

########################################################################
# Setup Python Part 3: Pybind11
#
# Note that find_package(pybind11) breaks all sorts of stuff because it
# uses its own Python-finding CMake code. We therefore limit its scope.
# When we switch to CMake 3.12, we can probably simplify this by calling
# find_package(Python ...) above.
########################################################################
function(UHD_FIND_PYBIND11)
  set(PYBIND11_FINDPYTHON ON)
  find_package(pybind11 ${UHD_PYBIND11_MIN_VERSION} QUIET)
  if(DEFINED pybind11_INCLUDE_DIR)
    set(pybind11_INCLUDE_DIR ${pybind11_INCLUDE_DIR} PARENT_SCOPE)
    set(pybind11_INCLUDE_DIRS ${pybind11_INCLUDE_DIRS} PARENT_SCOPE)
    set(pybind11_FOUND ${pybind11_FOUND} PARENT_SCOPE)
    set(pybind11_DEFINITIONS ${pybind11_DEFINITIONS} PARENT_SCOPE)
    set(pybind11_LIBRARIES ${pybind11_LIBRARIES} PARENT_SCOPE)
    set(pybind11_LIBRARY ${pybind11_LIBRARY} PARENT_SCOPE)
  endif()
endfunction()

UHD_FIND_PYBIND11()

endif(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
