#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

IF(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
SET(INCLUDED_UHD_PYTHON_CMAKE TRUE)

########################################################################
# Setup Python
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring the python interpreter...")
#this allows the user to override PYTHON_EXECUTABLE
IF(PYTHON_EXECUTABLE)

    SET(PYTHONINTERP_FOUND TRUE)

#otherwise if not set, try to automatically find it
ELSE(PYTHON_EXECUTABLE)

    #use the built-in find script
    IF(ENABLE_PYTHON3)
        FIND_PACKAGE(PythonInterp 3.0)
    ELSE(ENABLE_PYTHON3)
        FIND_PACKAGE(PythonInterp 2.0)
    ENDIF(ENABLE_PYTHON3)

    #and if that fails use the find program routine
    IF(NOT PYTHONINTERP_FOUND)
        IF(ENABLE_PYTHON3)
            FIND_PROGRAM(PYTHON_EXECUTABLE NAMES python3 python3.5 python3.6)
        ELSE(ENABLE_PYTHON3)
            FIND_PROGRAM(PYTHON_EXECUTABLE NAMES python2 python2.7)
        ENDIF(ENABLE_PYTHON3)

        IF(PYTHON_EXECUTABLE)
            SET(PYTHONINTERP_FOUND TRUE)
        ENDIF(PYTHON_EXECUTABLE)
    ENDIF(NOT PYTHONINTERP_FOUND)

ENDIF(PYTHON_EXECUTABLE)

#make the path to the executable appear in the cmake gui
SET(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH "python interpreter")

MESSAGE(STATUS "Python interpreter: ${PYTHON_EXECUTABLE}")
MESSAGE(STATUS "Override with: -DPYTHON_EXECUTABLE=<path-to-python>")

IF(NOT PYTHONINTERP_FOUND)
    MESSAGE(FATAL_ERROR "Error: Python interpreter required by the build system.")
ENDIF(NOT PYTHONINTERP_FOUND)

MACRO(PYTHON_CHECK_MODULE desc mod cmd have)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "Python checking for ${desc}")
    EXECUTE_PROCESS(
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
    IF(${have} EQUAL 0)
        MESSAGE(STATUS "Python checking for ${desc} - found")
        SET(${have} TRUE)
    ELSEIF(${have} EQUAL 1)
        MESSAGE(STATUS "Python checking for ${desc} - \"import ${mod}\" failed")
        SET(${have} FALSE)
    ELSEIF(${have} EQUAL 2)
        MESSAGE(STATUS "Python checking for ${desc} - \"assert ${cmd}\" failed")
        SET(${have} FALSE)
    ELSE()
        MESSAGE(STATUS "Python checking for ${desc} - unknown error")
        SET(${have} FALSE)
    ENDIF()
ENDMACRO(PYTHON_CHECK_MODULE)

ENDIF(NOT DEFINED INCLUDED_UHD_PYTHON_CMAKE)
