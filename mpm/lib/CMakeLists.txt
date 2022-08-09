#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(UHD_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../host)

add_subdirectory(dboards)
add_subdirectory(chips)
add_subdirectory(spi)
add_subdirectory(i2c)
add_subdirectory(types)

if(ENABLE_MYKONOS)
    add_subdirectory(mykonos)
elseif(ENABLE_E320 OR ENABLE_E300)
    add_subdirectory(catalina)
endif(ENABLE_MYKONOS)

if(ENABLE_X400)
  add_subdirectory(rfdc)
endif(ENABLE_X400)

USRP_PERIPHS_ADD_OBJECT(periphs
    exception.cpp
    ${UHD_SOURCE_DIR}/lib/exception.cpp
)

# vim: set sw=4 et:
