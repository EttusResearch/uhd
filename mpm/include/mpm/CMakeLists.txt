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

ADD_SUBDIRECTORY(ad937x)
ADD_SUBDIRECTORY(chips)
ADD_SUBDIRECTORY(dboards)
ADD_SUBDIRECTORY(spi)
ADD_SUBDIRECTORY(types)
