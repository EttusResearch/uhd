# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT rfnoc_gain_sources)
    message(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT rfnoc_gain_sources)

########################################################################
# Check for pygccxml
########################################################################
gr_python_check_module_raw("pygccxml" "import pygccxml" PYGCCXML_FOUND)

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND rfnoc_gain_python_files
    rfnoc_gain_python.cc python_bindings.cc)

gr_pybind_make_oot(rfnoc_gain ../../.. gr::rfnoc_gain "${rfnoc_gain_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(
    TARGET rfnoc_gain_python
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:rfnoc_gain_python>
            ${PROJECT_BINARY_DIR}/test_modules/gnuradio/rfnoc_gain/)

install(
    TARGETS rfnoc_gain_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/rfnoc_gain
    COMPONENT pythonapi)
