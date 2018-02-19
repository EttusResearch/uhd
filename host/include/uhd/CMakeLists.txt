#
# Copyright 2010-2011,2013-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

ADD_SUBDIRECTORY(rfnoc)
ADD_SUBDIRECTORY(transport)
ADD_SUBDIRECTORY(types)
ADD_SUBDIRECTORY(cal)
ADD_SUBDIRECTORY(usrp)
ADD_SUBDIRECTORY(usrp_clock)
ADD_SUBDIRECTORY(utils)

CONFIGURE_FILE(
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

IF(ENABLE_RFNOC)
    UHD_INSTALL(FILES
        device3.hpp
        DESTINATION ${INCLUDE_DIR}/uhd
        COMPONENT headers
    )
ENDIF(ENABLE_RFNOC)

IF(ENABLE_C_API)
    UHD_INSTALL(FILES
        config.h
        error.h
        DESTINATION ${INCLUDE_DIR}/uhd
        COMPONENT headers
    )
ENDIF(ENABLE_C_API)
