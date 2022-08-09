#
# Copyright 2010-2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(CURSES_NEED_NCURSES 1)
find_package(Curses)

if(CURSES_FOUND)
    include_directories(${CURSES_INCLUDE_DIR})
    set(latency_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
    include_directories(${latency_include_dir})
    set(latency_lib_path ${CMAKE_CURRENT_SOURCE_DIR}/lib/Responder.cpp)

    set(sources
        responder.cpp
    )

    set(latency_comp_name utilities)
    set(latency_comp_dest ${PKG_LIB_DIR}/utils/latency)

    #for each source: build an executable and install
    foreach(source ${sources})
        get_filename_component(name ${source} NAME_WE)
        add_executable(${name} ${source} ${latency_lib_path})
    	LIBUHD_APPEND_SOURCES(${name})
        target_link_libraries(${name} uhd ${Boost_LIBRARIES} ${CURSES_LIBRARIES})
    	UHD_INSTALL(TARGETS ${name} RUNTIME DESTINATION ${latency_comp_dest} COMPONENT ${latency_comp_name})
    endforeach(source)

    UHD_INSTALL(PROGRAMS run_tests.py graph.py
                DESTINATION ${latency_comp_dest}
                COMPONENT ${latency_comp_name}
    )
endif(CURSES_FOUND)
