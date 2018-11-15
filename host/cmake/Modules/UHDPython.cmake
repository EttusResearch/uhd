#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
set(INCLUDED_UHD_PYTHON_CMAKE TRUE)

########################################################################
# Setup Python
########################################################################
message(STATUS "")
message(STATUS "Configuring the python interpreter...")
#this allows the user to override PYTHON_EXECUTABLE
if(PYTHON_EXECUTABLE)

    set(PYTHONINTERP_FOUND TRUE)

#otherwise if not set, try to automatically find it
else(PYTHON_EXECUTABLE)

    #use the built-in find script
    if(ENABLE_PYTHON3)
        find_package(PythonInterp 3.0)
    else(ENABLE_PYTHON3)
        find_package(PythonInterp 2.0)
    endif(ENABLE_PYTHON3)

    #and if that fails use the find program routine
    if(NOT PYTHONINTERP_FOUND)
        if(ENABLE_PYTHON3)
            find_program(PYTHON_EXECUTABLE NAMES python3 python3.5 python3.6)
        else(ENABLE_PYTHON3)
            find_program(PYTHON_EXECUTABLE NAMES python2 python2.7)
        endif(ENABLE_PYTHON3)

        if(PYTHON_EXECUTABLE)
            set(PYTHONINTERP_FOUND TRUE)
        endif(PYTHON_EXECUTABLE)
    endif(NOT PYTHONINTERP_FOUND)

endif(PYTHON_EXECUTABLE)

#make the path to the executable appear in the cmake gui
set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH
    "python buildtime interpreter")

message(STATUS "Python interpreter: ${PYTHON_EXECUTABLE}")
message(STATUS "Override with: -DPYTHON_EXECUTABLE=<path-to-python>")

if(NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Error: Python interpreter required by the build system.")
endif(NOT PYTHONINTERP_FOUND)

#this allows the user to override RUNTIME_PYTHON_EXECUTABLE
if(NOT RUNTIME_PYTHON_EXECUTABLE)
    #default to the buildtime interpreter
    set(RUNTIME_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
endif(NOT RUNTIME_PYTHON_EXECUTABLE)

#make the path to the executable appear in the cmake gui
set(RUNTIME_PYTHON_EXECUTABLE ${RUNTIME_PYTHON_EXECUTABLE} CACHE FILEPATH
    "python runtime interpreter")

message(STATUS "Python runtime interpreter: ${RUNTIME_PYTHON_EXECUTABLE}")
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

endif(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
