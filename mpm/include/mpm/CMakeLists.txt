#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
install(FILES
    exception.hpp
    DESTINATION ${INCLUDE_DIR}/mpm
)

if(ENABLE_MYKONOS)
    add_subdirectory(ad937x)
elseif(ENABLE_E320 OR ENABLE_E300)
    add_subdirectory(ad9361)
endif(ENABLE_MYKONOS)

add_subdirectory(chips)
add_subdirectory(dboards)
add_subdirectory(spi)
add_subdirectory(types)

if(ENABLE_X400)
  add_subdirectory(rfdc)
endif(ENABLE_X400)
