#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# unit test support
########################################################################
include(UHDUnitTest)

########################################################################
# unit test suite
########################################################################
set(test_sources
    gain_math_test.cpp
    gain_block_test.cpp
)
########################################################################
# Setup the include paths
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    ${UHD_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
)

# Note: Python-based tests cannot have the same name as a C++-based test (i.e.,
# only differ in the cpp/py file extension). If in doubt, prepend 'py'.
#set(pytest_sources
#)

#turn each test cpp file into an executable with an int main() function
add_definitions(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)

#for each source: build an executable, register it as a test
foreach(test_source ${test_sources})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} uhd rfnoc-gain ${Boost_LIBRARIES})
    UHD_ADD_TEST(${test_name} ${test_name})
    install(TARGETS ${test_name} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/tests COMPONENT tests)
endforeach(test_source)

if(ENABLE_PYTHON_API)
    foreach(test_source ${pytest_sources})
        get_filename_component(test_name ${test_source} NAME_WE)
        UHD_ADD_PYTEST(${test_name})
    endforeach(test_source)
    install(FILES ${pytest_sources}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/tests
        COMPONENT tests
    )
endif(ENABLE_PYTHON_API)

