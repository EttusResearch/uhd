#
# Copyright 2010-2011,2013-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

add_subdirectory(cal)
add_subdirectory(experts)
add_subdirectory(extension)
add_subdirectory(features)
add_subdirectory(rfnoc)
add_subdirectory(transport)
add_subdirectory(types)
add_subdirectory(usrp)
add_subdirectory(usrp_clock)
add_subdirectory(utils)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
)

UHD_INSTALL(FILES
    build_info.hpp
    config.hpp
    convert.hpp
    device.hpp
    exception.hpp
    property_tree.ipp
    property_tree.hpp
    rfnoc_graph.hpp
    stream.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
    DESTINATION ${INCLUDE_DIR}/uhd
    COMPONENT headers
)

if(ENABLE_C_API)
    UHD_INSTALL(FILES
        config.h
        error.h
        version.h
        DESTINATION ${INCLUDE_DIR}/uhd
        COMPONENT headers
    )
endif(ENABLE_C_API)
