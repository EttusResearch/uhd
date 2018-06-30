#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# example applications
########################################################################
SET(example_sources
    benchmark_rate.cpp
    network_relay.cpp
    rx_multi_samples.cpp
    rx_samples_to_file.cpp
    rx_samples_to_udp.cpp
    rx_timed_samples.cpp
    test_dboard_coercion.cpp
    test_messages.cpp
    test_pps_input.cpp
    test_timed_commands.cpp
    tx_bursts.cpp
    tx_samples_from_file.cpp
    tx_timed_samples.cpp
    tx_waveforms.cpp
    txrx_loopback_to_file.cpp
    usrp_list_sensors.cpp
    latency_test.cpp
    gpio.cpp
    sync_to_gps.cpp
)

IF(ENABLE_RFNOC)
    LIST(APPEND
        example_sources
        rfnoc_nullsource_ce_rx.cpp
        rfnoc_rx_to_file.cpp
    )
ENDIF(ENABLE_RFNOC)

IF(ENABLE_OCTOCLOCK)
    LIST(APPEND example_sources test_clock_synch.cpp)
ENDIF(ENABLE_OCTOCLOCK)

#for each source: build an executable and install
FOREACH(example_source ${example_sources})
    GET_FILENAME_COMPONENT(example_name ${example_source} NAME_WE)
    ADD_EXECUTABLE(${example_name} ${example_source})
    TARGET_LINK_LIBRARIES(${example_name} uhd ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${example_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)
ENDFOREACH(example_source)

########################################################################
# ASCII Art DFT - requires curses, so this part is optional
########################################################################
FIND_PACKAGE(Curses)

IF(CURSES_FOUND)
    INCLUDE_DIRECTORIES(${CURSES_INCLUDE_DIR})
    ADD_EXECUTABLE(rx_ascii_art_dft rx_ascii_art_dft.cpp)
    TARGET_LINK_LIBRARIES(rx_ascii_art_dft uhd ${CURSES_LIBRARIES} ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS rx_ascii_art_dft RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)

    ADD_EXECUTABLE(twinrx_freq_hopping twinrx_freq_hopping.cpp)
    TARGET_LINK_LIBRARIES(twinrx_freq_hopping uhd ${CURSES_LIBRARIES} ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS twinrx_freq_hopping RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)
ENDIF(CURSES_FOUND)

########################################################################
# Examples using C API
########################################################################
IF(ENABLE_C_API)
    #
    # Check if this particular C99 feature is available with this compiler
    #
    INCLUDE(CheckCSourceCompiles)
    CHECK_C_SOURCE_COMPILES("
    typedef struct {
        int bar;
        int baz;
    } foo;

    int main()
    {
        foo wat = {
            .bar = 1,
            .baz = 2
        };

        return 0;
    }
    " HAVE_C99_STRUCTDECL)

    IF(HAVE_C99_STRUCTDECL)
        ADD_SUBDIRECTORY(getopt)
        INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/getopt)

        SET(C_API_EXAMPLES
            rx_samples_c
            tx_samples_c
        )

        FOREACH(example ${C_API_EXAMPLES})
            ADD_EXECUTABLE(${example} ${example}.c)
            TARGET_LINK_LIBRARIES(${example} uhd getopt)
            IF(UNIX)
                TARGET_LINK_LIBRARIES(${example} m)
            ENDIF(UNIX)
            UHD_INSTALL(TARGETS ${example} RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)
        ENDFOREACH(example ${C_API_EXAMPLES})
    ENDIF(HAVE_C99_STRUCTDECL)
ENDIF(ENABLE_C_API)

IF(ENABLE_PYTHON_API)
    ADD_SUBDIRECTORY(python)
ENDIF(ENABLE_PYTHON_API)
