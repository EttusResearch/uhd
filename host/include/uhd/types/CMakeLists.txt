#
# Copyright 2010-2011,2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    byte_vector.hpp
    device_addr.hpp
    dict.ipp
    dict.hpp
    direction.hpp
    eeprom.hpp
    endianness.hpp
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
    DESTINATION ${INCLUDE_DIR}/uhd/types
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
        DESTINATION ${INCLUDE_DIR}/uhd/types
        COMPONENT headers
    )
endif(ENABLE_C_API)
