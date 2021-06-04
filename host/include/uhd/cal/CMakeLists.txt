#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(FLATBUFFER_SCHEMA_FILES
    "cal_metadata.fbs"
    "iq_cal.fbs"
    "pwr_cal.fbs"
    "dsa_cal.fbs"
)

set(FLATBUFFER_GEN_HEADER_FILES
    "cal_metadata_generated.h"
    "iq_cal_generated.h"
    "pwr_cal_generated.h"
    "dsa_cal_generated.h"
)

UHD_INSTALL(FILES
    container.hpp
    database.hpp
    iq_cal.hpp
    pwr_cal.hpp
    dsa_cal.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/cal
    COMPONENT headers
)

UHD_INSTALL(FILES
    ${FLATBUFFER_SCHEMA_FILES}
    DESTINATION ${PKG_DATA_DIR}/cal
    COMPONENT headers
)

UHD_INSTALL(FILES
        ${FLATBUFFER_GEN_HEADER_FILES}
        DESTINATION ${INCLUDE_DIR}/uhd/cal
        COMPONENT headers
    )
