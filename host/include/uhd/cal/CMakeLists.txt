#
# Copyright 2016 Ettus Research
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

UHD_INSTALL(FILES
    container.hpp
    power_container.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/cal
    COMPONENT headers
)
