#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
add_test(
  NAME mpm_unit_tests
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CMAKE_BINARY_DIR}/python
  ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_unit_tests.py ${MPM_DEVICE}
)

if(APPLE)
    set_tests_properties(mpm_unit_tests PROPERTIES
        ENVIRONMENT "DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$DYLD_LIBRARY_PATH")
else()
    set_tests_properties(mpm_unit_tests PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$LD_LIBRARY_PATH")
endif()
