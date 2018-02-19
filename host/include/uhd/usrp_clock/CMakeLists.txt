#
# Copyright 2014-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

UHD_INSTALL(FILES
    octoclock_eeprom.hpp
    multi_usrp_clock.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/usrp_clock
    COMPONENT headers
)

IF(ENABLE_C_API)
    UHD_INSTALL(FILES
    usrp_clock.h
    DESTINATION ${INCLUDE_DIR}/uhd/usrp_clock
    COMPONENT headers
    )
ENDIF(ENABLE_C_API)
