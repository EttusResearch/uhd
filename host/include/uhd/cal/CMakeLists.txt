#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    database.hpp
    container.hpp
    interpolation.hpp
    iq_cal.hpp
    iq_cal_generated.h
    DESTINATION ${INCLUDE_DIR}/uhd/cal
    COMPONENT headers
)

UHD_INSTALL(FILES
    iq_cal.fbs
    DESTINATION ${PKG_DATA_DIR}/cal
    COMPONENT headers
)

