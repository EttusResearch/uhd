#
# Copyright 2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

macro(LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM lvbitx binfile)
    get_filename_component(lvbitxprefix ${lvbitx} NAME_WE)

    if( ${binfile} STREQUAL "OFF" )
        set( GEN_OPTIONS )
        message( STATUS "  Using ${lvbitx} for codegen" )
    else( ${binfile} STREQUAL "OFF" )
        set( GEN_OPTIONS --merge-bin=${UHD_SOURCE_DIR}/../binaries/${binfile} --output-lvbitx-path=${UHD_SOURCE_DIR}/../binaries )
        message( STATUS "  Merging ${lvbitx} with ${binfile} for codegen" )
    endif( ${binfile} STREQUAL "OFF" )

    set(OUTPUT_PATH_OPT --output-src-path=${CMAKE_CURRENT_BINARY_DIR})
    set(IMAGES_PATH_OPT --uhd-images-path=${UHD_IMAGES_DIR})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.cpp
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.hpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/process-lvbitx.py
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/template_lvbitx.hpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/template_lvbitx.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${lvbitx}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/process-lvbitx.py ${OUTPUT_PATH_OPT} ${IMAGES_PATH_OPT} ${GEN_OPTIONS} ${CMAKE_CURRENT_SOURCE_DIR}/${lvbitx}
        COMMENT "Generating ${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.cpp"
    )

    #make libuhd depend on the output file
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.hpp)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_BINARY_DIR}/${lvbitxprefix}_lvbitx.cpp)
endmacro(LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

########################################################################
# Generation code
########################################################################

message(STATUS "")
message(STATUS "Processing NI-RIO FPGA LVBITX Bitstreams...")

# X300 Stuff
LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM(x300.lvbitx_base OFF)

# X310 Stuff
LIBUHD_LVBITX_GEN_SOURCE_AND_BITSTREAM(x310.lvbitx_base OFF)
