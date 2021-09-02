#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# Formatting
message(STATUS "")

# All devtest files get installed:
file(GLOB py_devtest_files "*.py")
UHD_INSTALL(PROGRAMS
    ${py_devtest_files}
    DESTINATION ${PKG_LIB_DIR}/tests/devtest
    COMPONENT tests
)

# Arguments:
# - pattern: This will be used to identify which devtest_*.py is to be executed.
# - filter: Will be used in args strings as "type=<filter>".
# - devtype: A descriptive string. Is only used for CMake output.
macro(ADD_DEVTEST pattern filter devtype)
    message(STATUS "Adding ${devtype} device test target")
    add_custom_target("test_${pattern}"
        ${RUNTIME_PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_testsuite.py
        "--src-dir" "${CMAKE_CURRENT_SOURCE_DIR}"
        "--devtest-pattern" "${pattern}"
        "--args" "$$EXTRA_DEV_ARGS,type=${filter}"
        "--build-type" "${CMAKE_BUILD_TYPE}"
        "--build-dir" "${UHD_BINARY_DIR}"
        "--python-interp" "${RUNTIME_PYTHON_EXECUTABLE}"
        COMMENT "Running device test on all connected ${devtype} devices:"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endmacro(ADD_DEVTEST)

if(ENABLE_B200)
    ADD_DEVTEST("b2xx" "b200" "B2XX")
endif(ENABLE_B200)
if(ENABLE_X300)
    ADD_DEVTEST("x3x0" "x300" "X3x0")
endif(ENABLE_X300)
if(ENABLE_E300)
    ADD_DEVTEST("e3xx" "e3xx" "E3XX")
endif(ENABLE_E300)
if(ENABLE_N300)
    ADD_DEVTEST("n3x0" "n3xx" "N3XX")
endif(ENABLE_N300)
if(ENABLE_E320)
    ADD_DEVTEST("e320" "e3xx" "E32x")
endif(ENABLE_E320)
if(ENABLE_X400)
    ADD_DEVTEST("x4x0" "x4xx" "X4x0")
endif(ENABLE_X400)
# Formatting
message(STATUS "")
