#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

SET(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../host)

ADD_SUBDIRECTORY(dboards)
ADD_SUBDIRECTORY(chips)
ADD_SUBDIRECTORY(spi)
ADD_SUBDIRECTORY(types)

if(ENABLE_MYKONOS)
    ADD_SUBDIRECTORY(mykonos)
endif(ENABLE_MYKONOS)

USRP_PERIPHS_ADD_OBJECT(periphs
    exception.cpp
    xbar_iface.cpp
    ${UHD_HOST_ROOT}/lib/exception.cpp
)

# vim: set sw=4 et:
