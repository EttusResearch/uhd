#
# Copyright 2017 Ettus Research, National Instruments Company
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
ENDIF(ENABLE_MYKONOS)

ADD_SUBDIRECTORY(chips)
ADD_SUBDIRECTORY(dboards)
ADD_SUBDIRECTORY(spi)
ADD_SUBDIRECTORY(types)
