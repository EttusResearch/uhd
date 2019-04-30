#
# Copyright 2010-2011,2013-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

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
    deprecated.hpp
    device.hpp
    exception.hpp
    property_tree.ipp
    property_tree.hpp
    stream.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
    DESTINATION ${INCLUDE_DIR}/uhd
    COMPONENT headers
)

if(ENABLE_RFNOC)
    UHD_INSTALL(FILES
        device3.hpp
        DESTINATION ${INCLUDE_DIR}/uhd
        COMPONENT headers
    )
endif(ENABLE_RFNOC)

if(ENABLE_C_API)
    UHD_INSTALL(FILES
        config.h
        error.h
        version.h
        DESTINATION ${INCLUDE_DIR}/uhd
        COMPONENT headers
    )
endif(ENABLE_C_API)
