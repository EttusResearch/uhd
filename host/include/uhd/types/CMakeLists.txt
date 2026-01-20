#
# Copyright 2010-2011,2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# Detect host endianness
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
    # Use CMAKE_CXX_BYTE_ORDER for CMake 3.20 and later
    if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
        set(UHD_ENDIANNESS_MACRO "UHD_BIG_ENDIAN")
    elseif(CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
        set(UHD_ENDIANNESS_MACRO "UHD_LITTLE_ENDIAN")
    else()
        message(FATAL_ERROR "Unknown byte order: ${CMAKE_CXX_BYTE_ORDER}")
    endif()
else()
    # Use test_big_endian for older CMake versions
    include(TestBigEndian)
    test_big_endian(__IS_BIG_ENDIAN)
    if(__IS_BIG_ENDIAN)
        set(UHD_ENDIANNESS_MACRO "UHD_BIG_ENDIAN")
    else()
        set(UHD_ENDIANNESS_MACRO "UHD_LITTLE_ENDIAN")
    endif()
    if(NOT DEFINED __IS_BIG_ENDIAN)
        message(FATAL_ERROR "Could not determine target endianness!")
    endif()
endif()
message(STATUS "Detected target endianness: ${UHD_ENDIANNESS_MACRO}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/endianness.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/endianness.hpp
)

UHD_INSTALL(FILES
    byte_vector.hpp
    device_addr.hpp
    dict.ipp
    dict.hpp
    direction.hpp
    eeprom.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/endianness.hpp
    mac_addr.hpp
    memmap_iface.hpp
    metadata.hpp
    ranges.hpp
    ref_vector.hpp
    sensors.hpp
    serial.hpp
    stream_cmd.hpp
    time_spec.hpp
    tune_request.hpp
    tune_result.hpp
    wb_iface.hpp
    filters.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/uhd/types
    COMPONENT headers
)

if(ENABLE_C_API)
    UHD_INSTALL(FILES
        metadata.h
        ranges.h
        sensors.h
        string_vector.h
        tune_request.h
        tune_result.h
        usrp_info.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/uhd/types
        COMPONENT headers
    )
endif(ENABLE_C_API)
