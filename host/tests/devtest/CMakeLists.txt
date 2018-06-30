#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

# Formatting
MESSAGE(STATUS "")

# All devtest files get installed:
FILE(GLOB py_devtest_files "*.py")
UHD_INSTALL(PROGRAMS
    ${py_devtest_files}
    DESTINATION ${PKG_LIB_DIR}/tests/devtest
    COMPONENT tests
)

# Arguments:
# - pattern: This will be used to identify which devtest_*.py is to be executed.
# - filter: Will be used in args strings as "type=<filter>".
# - devtype: A descriptive string. Is only used for CMake output.
MACRO(ADD_DEVTEST pattern filter devtype)
    MESSAGE(STATUS "Adding ${devtype} device test target")
    ADD_CUSTOM_TARGET("test_${pattern}"
        ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_testsuite.py
        "--src-dir" "${CMAKE_CURRENT_SOURCE_DIR}"
        "--devtest-pattern" "${pattern}"
        "--device-filter" "${filter}"
        "--build-type" "${CMAKE_BUILD_TYPE}"
        "--build-dir" "${CMAKE_BINARY_DIR}"
        COMMENT "Running device test on all connected ${devtype} devices:"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
ENDMACRO(ADD_DEVTEST)

IF(ENABLE_B200)
    ADD_DEVTEST("b2xx" "b200" "B2XX")
ENDIF(ENABLE_B200)
IF(ENABLE_X300)
    ADD_DEVTEST("x3x0" "x300" "X3x0")
ENDIF(ENABLE_X300)
IF(ENABLE_E300)
    ADD_DEVTEST("e3xx" "e3x0" "E3XX")
ENDIF(ENABLE_E300)
IF(ENABLE_MPMD)
    ADD_DEVTEST("n3x0" "n3xx" "N3XX")
ENDIF(ENABLE_MPMD)
# Formatting
MESSAGE(STATUS "")
