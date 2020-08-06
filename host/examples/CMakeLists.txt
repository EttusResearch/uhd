#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# example applications
########################################################################
set(example_sources
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
    rfnoc_nullsource_ce_rx.cpp
    rfnoc_rx_to_file.cpp
    rfnoc_radio_loopback.cpp
    rfnoc_replay_samples_from_file.cpp
    #benchmark_streamer.cpp
)

if(ENABLE_OCTOCLOCK)
    list(APPEND example_sources test_clock_synch.cpp)
endif(ENABLE_OCTOCLOCK)

#for each source: build an executable and install
foreach(example_source ${example_sources})
    get_filename_component(example_name ${example_source} NAME_WE)
    add_executable(${example_name} ${example_source})
    target_link_libraries(${example_name} uhd ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${example_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)
endforeach(example_source)

########################################################################
# ASCII Art DFT - requires curses, so this part is optional
########################################################################
find_package(Curses)

if(CURSES_FOUND)
    include_directories(${CURSES_INCLUDE_DIR})
    add_executable(rx_ascii_art_dft rx_ascii_art_dft.cpp)
    target_link_libraries(rx_ascii_art_dft uhd ${CURSES_LIBRARIES} ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS rx_ascii_art_dft RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)

    add_executable(twinrx_freq_hopping twinrx_freq_hopping.cpp)
    target_link_libraries(twinrx_freq_hopping uhd ${CURSES_LIBRARIES} ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS twinrx_freq_hopping RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)
endif(CURSES_FOUND)

########################################################################
# Examples using C API
########################################################################
if(ENABLE_C_API)
    #
    # Check if this particular C99 feature is available with this compiler
    #
    include(CheckCSourceCompiles)
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

    if(HAVE_C99_STRUCTDECL)
        add_subdirectory(getopt)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/getopt)

        set(C_API_EXAMPLES
            rx_samples_c
            tx_samples_c
        )

        foreach(example ${C_API_EXAMPLES})
            add_executable(${example} ${example}.c)
            target_link_libraries(${example} uhd getopt)
            if(UNIX)
                target_link_libraries(${example} m)
            endif(UNIX)
            UHD_INSTALL(TARGETS ${example} RUNTIME DESTINATION ${PKG_LIB_DIR}/examples COMPONENT examples)
        endforeach(example ${C_API_EXAMPLES})
    endif(HAVE_C99_STRUCTDECL)
endif(ENABLE_C_API)

if(ENABLE_PYTHON_API)
    add_subdirectory(python)
endif(ENABLE_PYTHON_API)
