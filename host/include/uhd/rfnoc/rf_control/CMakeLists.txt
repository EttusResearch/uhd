#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    core_iface.hpp
    power_reference_iface.hpp

    DESTINATION ${INCLUDE_DIR}/uhd/rfnoc/rf_control
    COMPONENT headers
)
