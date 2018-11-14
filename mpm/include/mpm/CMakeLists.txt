#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
install(FILES
    xbar_iface.hpp
    exception.hpp
    DESTINATION ${INCLUDE_DIR}/mpm
)

if(ENABLE_MYKONOS)
    add_subdirectory(ad937x)
elseif(ENABLE_E320)
    add_subdirectory(ad9361)
endif(ENABLE_MYKONOS)

add_subdirectory(chips)
add_subdirectory(dboards)
add_subdirectory(spi)
add_subdirectory(types)
