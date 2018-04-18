#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
INSTALL(FILES
    xbar_iface.hpp
    exception.hpp
    DESTINATION ${INCLUDE_DIR}/mpm
)

IF(ENABLE_MYKONOS)
    ADD_SUBDIRECTORY(ad937x)
ELSEIF(ENABLE_E320)
    ADD_SUBDIRECTORY(ad9361)
ENDIF(ENABLE_MYKONOS)

ADD_SUBDIRECTORY(chips)
ADD_SUBDIRECTORY(dboards)
ADD_SUBDIRECTORY(spi)
ADD_SUBDIRECTORY(types)
