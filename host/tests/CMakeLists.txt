
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
    cal_database_test.cpp
    cal_data_iq_test.cpp
    cal_data_gain_pwr_test.cpp
    chdr_parse_test.cpp
    cal_data_dsa_test.cpp
    chdr_test.cpp
    constrained_device_args_test.cpp
    convert_test.cpp
    dict_test.cpp
    eeprom_utils_test.cpp
    error_test.cpp
    fp_compare_delta_test.cpp
    fp_compare_epsilon_test.cpp
    gain_group_test.cpp
    interpolation_test.cpp
    isatty_test.cpp
    log_test.cpp
    math_test.cpp
    narrow_cast_test.cpp
    property_test.cpp
    ranges_test.cpp
    rfnoc_node_test.cpp
    scope_exit_test.cpp
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
    link_test.cpp
    rx_streamer_test.cpp
    tx_streamer_test.cpp
    block_id_test.cpp
    rfnoc_property_test.cpp
    multichan_register_iface_test.cpp
)

# Note: Python-based tests cannot have the same name as a C++-based test (i.e.,
# only differ in the cpp/py file extension). If in doubt, prepend 'py'
set(pytest_sources
    pyranges_test.py
    verify_fbs_test.py
    pychdr_parse_test.py
    uhd_image_downloader_test.py
)

#turn each test cpp file into an executable with an int main() function
add_definitions(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)

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

include_directories("${UHD_SOURCE_DIR}/lib/include")
include_directories("${UHD_BINARY_DIR}/lib/include")
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

if(ENABLE_PYTHON_API)
    foreach(test_source ${pytest_sources})
        get_filename_component(test_name ${test_source} NAME_WE)
        UHD_ADD_PYTEST(${test_name})
    endforeach(test_source)
    UHD_INSTALL(FILES ${pytest_sources}
        DESTINATION ${PKG_LIB_DIR}/tests
        COMPONENT tests
    )
endif(ENABLE_PYTHON_API)

###############################################################################
# Add a unit test that requires linkage to internal parts of UHD which are not
# API
###############################################################################
macro(UHD_ADD_NONAPI_TEST)
    cmake_parse_arguments(test "NOAUTORUN" "TARGET" "INCLUDE_DIRS;EXTRA_SOURCES;EXTRA_LIBS" ${ARGN})
    get_filename_component(test_name ${test_TARGET} NAME_WE)
    include_directories(${test_INCLUDE_DIRS})
    add_executable(${test_name} ${test_TARGET} ${test_EXTRA_SOURCES})
    target_link_libraries(${test_name} uhd uhd_test ${Boost_LIBRARIES})
    if(NOT ${test_NOAUTORUN})
        UHD_ADD_TEST(${test_name} ${test_name})
    endif(NOT ${test_NOAUTORUN})
    UHD_INSTALL(TARGETS ${test_name}
        RUNTIME
        DESTINATION ${PKG_LIB_DIR}/tests
        COMPONENT tests)
endmacro(UHD_ADD_NONAPI_TEST)

###############################################################################
# Add a unit test for an RFNoC block controller
###############################################################################
macro(UHD_ADD_RFNOC_BLOCK_TEST)
    cmake_parse_arguments(test "NOAUTORUN" "TARGET" "INCLUDE_DIRS;EXTRA_SOURCES;EXTRA_LIBS" ${ARGN})
    UHD_ADD_NONAPI_TEST(
        TARGET rfnoc_block_tests/${test_TARGET}
        EXTRA_SOURCES
        ${test_EXTRA_SOURCES}
        ${UHD_SOURCE_DIR}/lib/rfnoc/graph.cpp
        INCLUDE_DIRS
        ${test_INCLUDE_DIRS}
    )
endmacro(UHD_ADD_RFNOC_BLOCK_TEST)

###############################################################################
# Now add all unit tests that require special linkage
###############################################################################
if(ENABLE_DPDK)
    find_package(DPDK)
    UHD_ADD_NONAPI_TEST(
        TARGET "dpdk_test.cpp"
        EXTRA_SOURCES
        ${UHD_SOURCE_DIR}/lib/utils/config_parser.cpp
        ${UHD_SOURCE_DIR}/lib/utils/paths.cpp
        ${UHD_SOURCE_DIR}/lib/utils/pathslib.cpp
        ${UHD_SOURCE_DIR}/lib/utils/prefs.cpp
        ${UHD_SOURCE_DIR}/lib/transport/adapter.cpp
        ${UHD_SOURCE_DIR}/lib/transport/uhd-dpdk/dpdk_common.cpp
        ${UHD_SOURCE_DIR}/lib/transport/uhd-dpdk/dpdk_io_service.cpp
        ${UHD_SOURCE_DIR}/lib/transport/udp_dpdk_link.cpp
        INCLUDE_DIRS
        ${DPDK_INCLUDE_DIRS}
        EXTRA_LIBS ${DPDK_LIBRARIES}
        NOAUTORUN # Don't register for auto-run, it requires special config
    )
    UHD_ADD_NONAPI_TEST(
        TARGET "dpdk_port_test.cpp"
        EXTRA_SOURCES
        ${UHD_SOURCE_DIR}/lib/utils/config_parser.cpp
        ${UHD_SOURCE_DIR}/lib/utils/paths.cpp
        ${UHD_SOURCE_DIR}/lib/utils/pathslib.cpp
        ${UHD_SOURCE_DIR}/lib/utils/prefs.cpp
        ${UHD_SOURCE_DIR}/lib/transport/adapter.cpp
        ${UHD_SOURCE_DIR}/lib/transport/uhd-dpdk/dpdk_common.cpp
        ${UHD_SOURCE_DIR}/lib/transport/uhd-dpdk/dpdk_io_service.cpp
        ${UHD_SOURCE_DIR}/lib/transport/udp_dpdk_link.cpp
        INCLUDE_DIRS
        ${DPDK_INCLUDE_DIRS}
        EXTRA_LIBS ${DPDK_LIBRARIES}
        NOAUTORUN # Don't register for auto-run, it requires special config
    )
    set_source_files_properties(
        ${UHD_SOURCE_DIR}/lib/transport/uhd-dpdk/dpdk_common.cpp
        ${UHD_SOURCE_DIR}/lib/transport/uhd-dpdk/dpdk_io_service.cpp
        ${UHD_SOURCE_DIR}/lib/transport/udp_dpdk_link.cpp
        PROPERTIES COMPILE_FLAGS "-march=native -D_GNU_SOURCE"
    )
ENDIF(ENABLE_DPDK)

UHD_ADD_NONAPI_TEST(
    TARGET "system_time_test.cpp"
    EXTRA_SOURCES
    "${UHD_SOURCE_DIR}/lib/utils/system_time.cpp"
)

UHD_ADD_NONAPI_TEST(
    TARGET "streamer_benchmark.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/chdr_packet_writer.cpp
    ${UHD_SOURCE_DIR}/lib/rfnoc/chdr_ctrl_xport.cpp
    ${UHD_SOURCE_DIR}/lib/rfnoc/chdr_rx_data_xport.cpp
    ${UHD_SOURCE_DIR}/lib/rfnoc/chdr_tx_data_xport.cpp
    ${UHD_SOURCE_DIR}/lib/transport/inline_io_service.cpp
    NOAUTORUN # Don't register for auto-run
)

UHD_ADD_NONAPI_TEST(
    TARGET "packet_handler_benchmark.cpp"
    NOAUTORUN
)

UHD_ADD_NONAPI_TEST(
    TARGET "config_parser_test.cpp"
    EXTRA_SOURCES ${UHD_SOURCE_DIR}/lib/utils/config_parser.cpp
)

# Careful: This is to satisfy the out-of-library build of paths.cpp. This is
# duplicate code from lib/utils/CMakeLists.txt, and it's been simplified.
# TODO Figure out if this is even needed
set(UHD_LIB_DIR "lib")
file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" UHD_PKG_PATH)
string(REPLACE "\\" "\\\\" UHD_PKG_PATH "${UHD_PKG_PATH}")
set_source_files_properties(
    ${UHD_SOURCE_DIR}/lib/utils/paths.cpp
    PROPERTIES COMPILE_DEFINITIONS
    "UHD_PKG_PATH=\"${UHD_PKG_PATH}\";UHD_LIB_DIR=\"${UHD_LIB_DIR}\""
)
UHD_ADD_NONAPI_TEST(
    TARGET "paths_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/utils/pathslib.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET rfnoc_propprop_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/graph.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET rfnoc_detailgraph_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/graph.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET actions_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/graph.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET rfnoc_chdr_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/chdr_packet_writer.cpp
    INCLUDE_DIRS
    ${UHD_BINARY_DIR}/lib/rfnoc/
    ${UHD_SOURCE_DIR}/lib/rfnoc/
)

UHD_ADD_NONAPI_TEST(
    TARGET client_zero_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/client_zero.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET zbx_cpld_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_cpld_ctrl.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_lo_ctrl.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/common/lmx2572.cpp
    INCLUDE_DIRS
    ${UHD_BINARY_DIR}/lib/ic_reg_maps
)

UHD_ADD_NONAPI_TEST(
    TARGET lmx2572_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/usrp/common/lmx2572.cpp
    INCLUDE_DIRS
    ${UHD_BINARY_DIR}/lib/ic_reg_maps
)

set_source_files_properties(
    ${UHD_SOURCE_DIR}/lib/utils/system_time.cpp
    PROPERTIES COMPILE_DEFINITIONS
    "HAVE_MICROSEC_CLOCK"
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET addsub_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET ddc_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET duc_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET fft_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET fir_filter_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET fosphor_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET keep_one_in_n_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET logpwr_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET moving_average_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET null_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET replay_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET siggen_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET split_stream_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET switchboard_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET vector_iir_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET window_block_test.cpp
)

UHD_ADD_RFNOC_BLOCK_TEST(
    TARGET x4xx_radio_block_test.cpp
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/usrp/common/lmx2572.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/common/pwr_cal_mgr.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/x400/x400_gpio_control.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/x400/x400_radio_control.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/x400/x400_rfdc_control.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/x400/adc_self_calibration.cpp
    ${UHD_SOURCE_DIR}/lib/rfnoc/radio_control_impl.cpp
    ${UHD_SOURCE_DIR}/lib/rfnoc/rf_control/gain_profile.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/mpmd/mpmd_mb_controller.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_dboard.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_dboard_init.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_lo_ctrl.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_cpld_ctrl.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/dboard/zbx/zbx_expert.cpp
    ${UHD_SOURCE_DIR}/lib/utils/compat_check.cpp
    ${UHD_SOURCE_DIR}/lib/features/discoverable_feature_registry.cpp
    ${UHD_SOURCE_DIR}/lib/usrp/cores/gpio_atr_3000.cpp
    $<TARGET_OBJECTS:uhd_rpclib>
    INCLUDE_DIRS ${UHD_SOURCE_DIR}/lib/deps/rpclib/include
    INCLUDE_DIRS ${UHD_SOURCE_DIR}/lib/deps/flatbuffers/include
)

UHD_ADD_NONAPI_TEST(
    TARGET "mb_controller_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/features/discoverable_feature_registry.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "transport_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/transport/inline_io_service.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "offload_io_srv_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/transport/offload_io_service.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "serial_number_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/utils/serial_number.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "pwr_cal_mgr_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/usrp/common/pwr_cal_mgr.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "discoverable_feature_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/features/discoverable_feature_registry.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "rf_control_gain_profile_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/rfnoc/rf_control/gain_profile.cpp
)

UHD_ADD_NONAPI_TEST(
    TARGET "x400_rfdc_control_test.cpp"
    EXTRA_SOURCES
    ${UHD_SOURCE_DIR}/lib/usrp/x400/x400_rfdc_control.cpp
)

########################################################################
# demo of a loadable module
########################################################################
if(MSVC OR APPLE OR LINUX)
    add_library(module_test MODULE module_test.cpp)
    target_link_libraries(module_test uhd)
endif()

add_subdirectory(devtest)
add_subdirectory(streaming_performance)
