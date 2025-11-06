#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    antenna_iface.hpp    
    core_iface.hpp
    nameless_gain_mixin.hpp
    power_reference_iface.hpp

    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/uhd/rfnoc/rf_control
    COMPONENT headers
)
