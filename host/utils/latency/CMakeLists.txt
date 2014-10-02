#
# Copyright 2010-2013 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
