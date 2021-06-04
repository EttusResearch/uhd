#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    discoverable_feature.hpp
    discoverable_feature_getter_iface.hpp
    ref_clk_calibration_iface.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/features
    COMPONENT headers
)
