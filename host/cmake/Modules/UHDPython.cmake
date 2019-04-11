#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
set(INCLUDED_UHD_PYTHON_CMAKE TRUE)

########################################################################
# Setup Python Part 1: Find the interpreters
########################################################################
message(STATUS "")
message(STATUS "Configuring the Python interpreter...")
#this allows the user to override PYTHON_EXECUTABLE
if(PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND TRUE)
endif(PYTHON_EXECUTABLE)

# We always try to find Py3k first. Once we only support Py3k we can remove
# most of this.
if(NOT PYTHONINTERP_FOUND)
    find_package(Python3 ${PYTHON3_MIN_VERSION} QUIET)
    if(Python3_Interpreter_FOUND)
        set(PYTHON_MIN_VERSION ${PYTHON3_MIN_VERSION})
        set(PYTHON_VERSION ${Python3_VERSION})
        set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
        set(PYTHONINTERP_FOUND TRUE)
    endif(Python3_Interpreter_FOUND)
endif(NOT PYTHONINTERP_FOUND)

if(NOT PYTHONINTERP_FOUND)
    find_package(PythonInterp ${PYTHON3_MIN_VERSION} QUIET)
    if(PYTHONINTERP_FOUND)
        set(PYTHON_MIN_VERSION ${PYTHON3_MIN_VERSION})
        set(PYTHON_VERSION ${PYTHON_VERSION_STRING})
    endif(PYTHONINTERP_FOUND)
endif(NOT PYTHONINTERP_FOUND)

# Next, try and find Py2k.
if(NOT PYTHONINTERP_FOUND)
    find_package(Python2 ${PYTHON_MIN_VERSION} QUIET)
    if(Python2_Interpreter_FOUND)
        set(PYTHON_VERSION ${Python2_VERSION})
        set(PYTHON_EXECUTABLE ${Python2_EXECUTABLE})
        set(PYTHONINTERP_FOUND TRUE)
    endif(Python2_Interpreter_FOUND)
endif(NOT PYTHONINTERP_FOUND)

if(NOT PYTHONINTERP_FOUND)
    find_package(PythonInterp ${PYTHON_MIN_VERSION} QUIET)
    set(PYTHON_VERSION ${PYTHON_VERSION_STRING})
endif(NOT PYTHONINTERP_FOUND)

# If that fails, try using the build-in find program routine.
if(NOT PYTHONINTERP_FOUND)
    message(STATUS "Attempting to find Python without CMake...")
    find_program(PYTHON_EXECUTABLE NAMES python3 python3.5 python3.6 python3.7 python3.8)
    if(PYTHON_EXECUTABLE)
        set(PYTHONINTERP_FOUND TRUE)
        set(PYTHON_MIN_VERSION ${PYTHON3_MIN_VERSION})
    endif(PYTHON_EXECUTABLE)
endif(NOT PYTHONINTERP_FOUND)

if(NOT PYTHONINTERP_FOUND)
    message(STATUS "Attempting to find Python without CMake...")
    find_program(PYTHON_EXECUTABLE NAMES python2 python2.7)
    if(PYTHON_EXECUTABLE)
        set(PYTHONINTERP_FOUND TRUE)
    endif(PYTHON_EXECUTABLE)
endif(NOT PYTHONINTERP_FOUND)

if(NOT PYTHON_VERSION)
    message(STATUS "Manually determining build Python version...")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "
from __future__ import print_function
import sys
print('{}.{}.{}'.format(
    sys.version_info.major,
    sys.version_info.minor,
    sys.version_info.micro))"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT PYTHON_VERSION)

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
    #default to the buildtime interpreter
    set(RUNTIME_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
    set(RUNTIME_PYTHON_VERSION ${PYTHON_VERSION})
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

macro(PYTHON_CHECK_MODULE desc mod cmd have)
    message(STATUS "")
    message(STATUS "Python checking for ${desc}")
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "
#########################################
from distutils.version import LooseVersion
try: import ${mod}
except: exit(1)
try: assert ${cmd}
except: exit(2)
exit(0)
#########################################"
        RESULT_VARIABLE ${have}
    )
    if(${have} EQUAL 0)
        message(STATUS "Python checking for ${desc} - found")
        set(${have} TRUE)
    elseif(${have} EQUAL 1)
        message(STATUS "Python checking for ${desc} - \"import ${mod}\" failed")
        set(${have} FALSE)
    elseif(${have} EQUAL 2)
        message(STATUS "Python checking for ${desc} - \"assert ${cmd}\" failed")
        set(${have} FALSE)
    else()
        message(STATUS "Python checking for ${desc} - unknown error")
        set(${have} FALSE)
    endif()
endmacro(PYTHON_CHECK_MODULE)

###############################################################################
# Part 2: Python Libraries
###############################################################################
# The libraries must match the RUNTIME_PYTHON_EXECUTABLE's version.
# - Figure out version
# - See if Python3_LIBRARIES is already set (or Python2_LIBRARIES)
if(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
    message(STATUS "Finding Python Libraries...")
    find_package(PythonLibs ${RUNTIME_PYTHON_VERSION} EXACT QUIET)
    if(RUNTIME_PYTHON_VERSION VERSION_LESS 3)
        if(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
            find_package(Python3 ${RUNTIME_PYTHON_VERSION}
                EXACT
                QUIET
                COMPONENTS Interpreter Development)
            if(Python3_Development_FOUND)
                set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
                set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
            endif(Python3_Development_FOUND)
        endif(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
    else(RUNTIME_PYTHON_VERSION VERSION_LESS 3)
        if(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
            find_package(Python2 ${RUNTIME_PYTHON_VERSION}
                EXACT
                QUIET
                COMPONENTS Interpreter Development)
            if(Python2_Development_FOUND)
                set(PYTHON_LIBRARIES ${Python2_LIBRARIES})
                set(PYTHON_INCLUDE_DIRS ${Python2_INCLUDE_DIRS})
            endif(Python2_Development_FOUND)
        endif(NOT PYTHON_LIBRARIES OR NOT PYTHON_INCLUDE_DIRS)
    endif(RUNTIME_PYTHON_VERSION VERSION_LESS 3)
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

endif(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
