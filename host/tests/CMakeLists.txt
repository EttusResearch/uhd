
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# unit test support
########################################################################
include(UHDUnitTest)

########################################################################
# build test common
########################################################################
set(test_common_SOURCEDIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
add_subdirectory(common)
link_directories(${test_common_SOURCEDIR})

########################################################################
# unit test suite
########################################################################
set(test_sources
    addr_test.cpp
    buffer_test.cpp
    byteswap_test.cpp
    cast_test.cpp
    chdr_test.cpp
    constrained_device_args_test.cpp
    convert_test.cpp
    dict_test.cpp
    eeprom_utils_test.cpp
    error_test.cpp
    fp_compare_delta_test.cpp
    fp_compare_epsilon_test.cpp
    gain_group_test.cpp
    isatty_test.cpp
    log_test.cpp
    math_test.cpp
    narrow_cast_test.cpp
    property_test.cpp
    ranges_test.cpp
    scope_exit_test.cpp
    sid_t_test.cpp
    sensors_test.cpp
    soft_reg_test.cpp
    sph_recv_test.cpp
    sph_send_test.cpp
    subdev_spec_test.cpp
    time_spec_test.cpp
    tasks_test.cpp
    vrt_test.cpp
    expert_test.cpp
    fe_conn_test.cpp
)

set(benchmark_sources
    packet_handler_benchmark.cpp
)

#turn each test cpp file into an executable with an int main() function
add_definitions(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)

if(ENABLE_RFNOC)
    list(APPEND test_sources
        block_id_test.cpp
        blockdef_test.cpp
        device3_test.cpp
        graph_search_test.cpp
        node_connect_test.cpp
        rate_node_test.cpp
        stream_sig_test.cpp
        tick_node_test.cpp
    )
endif(ENABLE_RFNOC)

if(ENABLE_C_API)
    list(APPEND test_sources
        eeprom_c_test.c
        error_c_test.cpp
        ranges_c_test.c
        sensors_c_test.c
        string_vector_c_test.c
        subdev_spec_c_test.c
    )
endif(ENABLE_C_API)

include_directories("${CMAKE_SOURCE_DIR}/lib/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common")

#for each source: build an executable, register it as a test
foreach(test_source ${test_sources})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} uhd uhd_test ${Boost_LIBRARIES})
    UHD_ADD_TEST(${test_name} ${test_name})
    UHD_INSTALL(TARGETS ${test_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/tests COMPONENT tests)
endforeach(test_source)

#for benchmarks, build executable but do not register
foreach(benchmark_source ${benchmark_sources})
    get_filename_component(benchmark_name ${benchmark_source} NAME_WE)
    add_executable(${benchmark_name} ${benchmark_source})
    target_link_libraries(${benchmark_name} uhd uhd_test ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${benchmark_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/tests COMPONENT tests)
endforeach(benchmark_source)


###############################################################################
# Add a unit test that requires linkage to internal parts of UHD which are not
# API
###############################################################################
macro(UHD_ADD_NONAPI_TEST)
    cmake_parse_arguments(test "NOAUTORUN" "TARGET" "INCLUDE_DIRS;EXTRA_SOURCES;EXTRA_LIBS" ${ARGN})
    get_filename_component(test_name ${test_TARGET} NAME_WE)
    include_directories(${test_INCLUDE_DIRS})
    add_executable(${test_name} ${test_TARGET} ${test_EXTRA_SOURCES})
    target_link_libraries(${test_name} uhd ${Boost_LIBRARIES})
    if(NOT ${test_NOAUTORUN})
        UHD_ADD_TEST(${test_name} ${test_name})
    endif(NOT ${test_NOAUTORUN})
    UHD_INSTALL(TARGETS ${test_name}
        RUNTIME
        DESTINATION ${PKG_LIB_DIR}/tests
        COMPONENT tests)
endmacro(UHD_ADD_NONAPI_TEST)

###############################################################################
# Now add all unit tests that require special linkage
###############################################################################
if(ENABLE_DPDK)
    find_package(DPDK)
    UHD_ADD_NONAPI_TEST(
        TARGET "dpdk_test.cpp"
        EXTRA_SOURCES
        ${CMAKE_SOURCE_DIR}/lib/utils/config_parser.cpp
        ${CMAKE_SOURCE_DIR}/lib/utils/paths.cpp
        ${CMAKE_SOURCE_DIR}/lib/utils/pathslib.cpp
        ${CMAKE_SOURCE_DIR}/lib/utils/prefs.cpp
        ${CMAKE_SOURCE_DIR}/lib/transport/dpdk_zero_copy.cpp
        INCLUDE_DIRS
        ${DPDK_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/lib/transport/
        ${CMAKE_SOURCE_DIR}/lib/transport/
        EXTRA_LIBS ${DPDK_LIBRARIES}
        NOAUTORUN # Don't register for auto-run, it requires special config
    )
ENDIF(ENABLE_DPDK)

UHD_ADD_NONAPI_TEST(
    TARGET "nocscript_expr_test.cpp"
    EXTRA_SOURCES
    "${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/expression.cpp"
    INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/lib/rfnoc/nocscript/
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/
)

UHD_ADD_NONAPI_TEST(
    TARGET "nocscript_ftable_test.cpp"
    EXTRA_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/function_table.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/expression.cpp
    INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/lib/rfnoc/nocscript/
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/
)

UHD_ADD_NONAPI_TEST(
    TARGET "nocscript_parser_test.cpp"
    EXTRA_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/parser.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/function_table.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/expression.cpp
    INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/lib/rfnoc/nocscript/
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/
)

UHD_ADD_NONAPI_TEST(
    TARGET "config_parser_test.cpp"
    EXTRA_SOURCES ${CMAKE_SOURCE_DIR}/lib/utils/config_parser.cpp
)

# Careful: This is to satisfy the out-of-library build of paths.cpp. This is
# duplicate code from lib/utils/CMakeLists.txt, and it's been simplified.
# TODO Figure out if this is even needed
set(UHD_LIB_DIR "lib")
file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" UHD_PKG_PATH)
string(REPLACE "\\" "\\\\" UHD_PKG_PATH "${UHD_PKG_PATH}")
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/lib/utils/paths.cpp
    PROPERTIES COMPILE_DEFINITIONS
    "UHD_PKG_PATH=\"${UHD_PKG_PATH}\";UHD_LIB_DIR=\"${UHD_LIB_DIR}\""
)
UHD_ADD_NONAPI_TEST(
    TARGET "paths_test.cpp"
    EXTRA_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/utils/pathslib.cpp
)

########################################################################
# demo of a loadable module
########################################################################
if(MSVC OR APPLE OR LINUX)
    add_library(module_test MODULE module_test.cpp)
    target_link_libraries(module_test uhd)
endif()

add_subdirectory(devtest)
