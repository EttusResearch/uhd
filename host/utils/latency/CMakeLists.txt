#
# Copyright 2010-2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

FIND_PACKAGE(Curses)

IF(CURSES_FOUND)
    INCLUDE_DIRECTORIES(${CURSES_INCLUDE_DIR})
    SET(latency_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
    INCLUDE_DIRECTORIES(${latency_include_dir})
    SET(latency_lib_path ${CMAKE_CURRENT_SOURCE_DIR}/lib/Responder.cpp)

    SET(sources
        responder.cpp
    )

    SET(latency_comp_name utilities)
    SET(latency_comp_dest ${PKG_LIB_DIR}/utils/latency)

    #for each source: build an executable and install
    FOREACH(source ${sources})
        GET_FILENAME_COMPONENT(name ${source} NAME_WE)
        ADD_EXECUTABLE(${name} ${source} ${latency_lib_path})
    	LIBUHD_APPEND_SOURCES(${name})
        TARGET_LINK_LIBRARIES(${name} uhd ${Boost_LIBRARIES} ${CURSES_LIBRARIES})
    	UHD_INSTALL(TARGETS ${name} RUNTIME DESTINATION ${latency_comp_dest} COMPONENT ${latency_comp_name})
    ENDFOREACH(source)

    UHD_INSTALL(PROGRAMS run_tests.py graph.py
                DESTINATION ${latency_comp_dest}
                COMPONENT ${latency_comp_name}
    )
ENDIF(CURSES_FOUND)
