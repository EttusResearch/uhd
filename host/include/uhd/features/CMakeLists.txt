#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

UHD_INSTALL(FILES
    adc_self_calibration_iface.hpp
    discoverable_feature_getter_iface.hpp
    discoverable_feature.hpp
    gpio_power_iface.hpp
    ref_clk_calibration_iface.hpp
    spi_getter_iface.hpp
    trig_io_mode_iface.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/features
    COMPONENT headers
)
