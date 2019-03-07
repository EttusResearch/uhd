#
# Copyright 2010,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

if(ENABLE_C_API)
    UHD_INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/uhd.h
        DESTINATION ${INCLUDE_DIR}
        COMPONENT headers
    )
endif(ENABLE_C_API)

add_subdirectory(uhd)
