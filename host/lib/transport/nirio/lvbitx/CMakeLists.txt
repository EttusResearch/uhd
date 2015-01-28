#
# Copyright 2013 Ettus Research LLC
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

MACRO(LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM lvbitx binfile)
    GET_FILENAME_COMPONENT(lvbitxprefix ${lvbitx} NAME_WE)

    IF( ${binfile} STREQUAL "OFF" )
        SET( GEN_OPTIONS )
        MESSAGE( STATUS "  Using ${lvbitx} for codegen" )
    ELSE( ${binfile} STREQUAL "OFF" )
        SET( GEN_OPTIONS --merge-bin=${CMAKE_SOURCE_DIR}/../binaries/${binfile} --output-lvbitx-path=${CMAKE_SOURCE_DIR}/../binaries )
        MESSAGE( STATUS "  Merging ${lvbitx} with ${binfile} for codegen" )
    ENDIF( ${binfile} STREQUAL "OFF" )

    SET(OUTPUT_PATH_OPT --output-src-path=${CMAKE_CURRENT_BINARY_DIR})
    SET(IMAGES_PATH_OPT --uhd-images-path=${UHD_IMAGES_DIR})

    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.hpp 
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/process-lvbitx.py
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/template_lvbitx.hpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/template_lvbitx.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${lvbitx}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/process-lvbitx.py ${OUTPUT_PATH_OPT} ${IMAGES_PATH_OPT} ${GEN_OPTIONS} ${CMAKE_CURRENT_SOURCE_DIR}/${lvbitx}
        COMMENT "Generating ${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.cpp"
    )

    #make libuhd depend on the output file
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.cpp)
ENDMACRO(LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

########################################################################
# Generation code
########################################################################

MESSAGE(STATUS "")
MESSAGE(STATUS "Processing NI-RIO FPGA LVBITX Bitstreams...")

# X300 Stuff
LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM(x300.lvbitx_base OFF)

# X310 Stuff
LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM(x310.lvbitx_base OFF)
