#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Setup library
########################################################################
#include(GrPlatform) #define LIB_SUFFIX

# List any C++ sources here. If there are no sources (e.g., because there
# is no block controller), then this directory will be skipped.
list(APPEND rfnoc_example_sources
    gain_block_control.cpp
)
if(NOT rfnoc_example_sources)
    MESSAGE(STATUS "No C++ sources... skipping lib/")
    return()
endif()

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    ${UHD_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
)

add_library(rfnoc-example SHARED
    ${rfnoc_example_sources}
)
target_link_libraries(rfnoc-example
    ${UHD_LIBRARIES}
    ${Boost_LIBRARIES}
    ${GNURADIO_ALL_LIBRARIES}
    ${ETTUS_LIBRARIES}
)
set_target_properties(rfnoc-example
    PROPERTIES DEFINE_SYMBOL "rfnoc_example_EXPORTS")

########################################################################
# Install built library files
########################################################################
install(TARGETS rfnoc-example
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
#include(GrTest)
#
#include_directories(${CPPUNIT_INCLUDE_DIRS})
#
#list(APPEND test_gain_sources
#    ${CMAKE_CURRENT_SOURCE_DIR}/test_gain.cc
#    ${CMAKE_CURRENT_SOURCE_DIR}/qa_gain.cc
#)
#
#add_executable(test-gain ${test_gain_sources})
#
#target_link_libraries(
#  test-gain
#  ${GNURADIO_RUNTIME_LIBRARIES}
#  ${Boost_LIBRARIES}
#  ${CPPUNIT_LIBRARIES}
#  ${ETTUS_LIBRARIES}
#  ${PC_ETTUS_LIBDIR}
#  gnuradio-gain
#)
#
#GR_ADD_TEST(test_gain test-gain)
