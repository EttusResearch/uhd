#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# unit test support
########################################################################
include(UHDUnitTest)

########################################################################
# unit test suite
########################################################################
SET(test_sources
    addr_test.cpp
    buffer_test.cpp
    byteswap_test.cpp
    cast_test.cpp
    cal_container_test.cpp
    chdr_test.cpp
    constrained_device_args_test.cpp
    convert_test.cpp
    dict_test.cpp
    error_test.cpp
    fp_compare_delta_test.cpp
    fp_compare_epsilon_test.cpp
    gain_group_test.cpp
    log_test.cpp
    math_test.cpp
    narrow_cast_test.cpp
    property_test.cpp
    ranges_test.cpp
    sid_t_test.cpp
    sensors_test.cpp
    sph_recv_test.cpp
    sph_send_test.cpp
    subdev_spec_test.cpp
    time_spec_test.cpp
    tasks_test.cpp
    vrt_test.cpp
    expert_test.cpp
    fe_conn_test.cpp
)

#turn each test cpp file into an executable with an int main() function
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)

IF(ENABLE_RFNOC)
    LIST(APPEND test_sources
        block_id_test.cpp
        blockdef_test.cpp
        device3_test.cpp
        graph_search_test.cpp
        node_connect_test.cpp
        rate_node_test.cpp
        stream_sig_test.cpp
        tick_node_test.cpp
    )
ENDIF(ENABLE_RFNOC)

IF(ENABLE_C_API)
    LIST(APPEND test_sources
        eeprom_c_test.c
        error_c_test.cpp
        ranges_c_test.c
        sensors_c_test.c
        string_vector_c_test.c
        subdev_spec_c_test.c
    )
ENDIF(ENABLE_C_API)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/lib/include")

#for each source: build an executable, register it as a test
FOREACH(test_source ${test_sources})
    GET_FILENAME_COMPONENT(test_name ${test_source} NAME_WE)
    ADD_EXECUTABLE(${test_name} ${test_source})
    TARGET_LINK_LIBRARIES(${test_name} uhd ${Boost_LIBRARIES})
    UHD_ADD_TEST(${test_name} ${test_name})
    UHD_INSTALL(TARGETS ${test_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/tests COMPONENT tests)
ENDFOREACH(test_source)

# Other tests that don't directly link with libuhd: (TODO find a nicer way to do this)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/lib/rfnoc/nocscript/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/)
ADD_EXECUTABLE(nocscript_expr_test
    nocscript_expr_test.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/expression.cpp
)
TARGET_LINK_LIBRARIES(nocscript_expr_test uhd ${Boost_LIBRARIES})
UHD_ADD_TEST(nocscript_expr_test nocscript_expr_test)
UHD_INSTALL(TARGETS nocscript_expr_test RUNTIME DESTINATION ${PKG_LIB_DIR}/tests COMPONENT tests)

ADD_EXECUTABLE(nocscript_ftable_test
    nocscript_ftable_test.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/function_table.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/expression.cpp
)
TARGET_LINK_LIBRARIES(nocscript_ftable_test uhd ${Boost_LIBRARIES})
UHD_ADD_TEST(nocscript_ftable_test nocscript_ftable_test)
UHD_INSTALL(TARGETS nocscript_ftable_test RUNTIME DESTINATION ${PKG_LIB_DIR}/tests COMPONENT tests)

ADD_EXECUTABLE(nocscript_parser_test
    nocscript_parser_test.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/parser.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/function_table.cpp
    ${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/expression.cpp
)
TARGET_LINK_LIBRARIES(nocscript_parser_test uhd ${Boost_LIBRARIES})
UHD_ADD_TEST(nocscript_parser_test nocscript_parser_test)
UHD_INSTALL(TARGETS nocscript_parser_test RUNTIME DESTINATION ${PKG_LIB_DIR}/tests COMPONENT tests)

ADD_EXECUTABLE(config_parser_test
    config_parser_test.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/config_parser.cpp
)
TARGET_LINK_LIBRARIES(config_parser_test uhd ${Boost_LIBRARIES})
UHD_ADD_TEST(config_parser_test config_parser_test)
UHD_INSTALL(TARGETS
    config_parser_test
    RUNTIME
    DESTINATION ${PKG_LIB_DIR}/tests
    COMPONENT tests
)

ADD_EXECUTABLE(paths_test
    paths_test.cpp
    ${CMAKE_SOURCE_DIR}/lib/utils/pathslib.cpp
)
# Careful: This is to satisfy the out-of-library build of paths.cpp. This is
# duplicate code from lib/utils/CMakeLists.txt, and it's been simplified.
SET(UHD_LIB_DIR "lib")
FILE(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" UHD_PKG_PATH)
STRING(REPLACE "\\" "\\\\" UHD_PKG_PATH "${UHD_PKG_PATH}")
SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_SOURCE_DIR}/lib/utils/paths.cpp
    PROPERTIES COMPILE_DEFINITIONS
    "UHD_PKG_PATH=\"${UHD_PKG_PATH}\";UHD_LIB_DIR=\"${UHD_LIB_DIR}\""
)
TARGET_LINK_LIBRARIES(paths_test uhd ${Boost_LIBRARIES})
UHD_ADD_TEST(paths_test paths_test)
UHD_INSTALL(TARGETS
    paths_test
    RUNTIME
    DESTINATION ${PKG_LIB_DIR}/tests
    COMPONENT tests
)

########################################################################
# demo of a loadable module
########################################################################
IF(MSVC OR APPLE OR LINUX)
    ADD_LIBRARY(module_test MODULE module_test.cpp)
    TARGET_LINK_LIBRARIES(module_test uhd)
ENDIF()

ADD_SUBDIRECTORY(devtest)
