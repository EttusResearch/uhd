#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include(CMakeRC)
cmrc_add_resource_library(uhd-resources
    ALIAS uhd_rc
    NAMESPACE rc
# calibration data test file
    cal/test.cal
# ZBX revB/C TX calibration data
    cal/x4xx_pwr_zbx_tx_0_tx+rx0.cal
    cal/x4xx_pwr_zbx_tx_1_tx+rx0.cal
# ZBX revB/C RX calibration data
    cal/x4xx_pwr_zbx_rx_0_tx+rx0.cal
    cal/x4xx_pwr_zbx_rx_1_tx+rx0.cal
    cal/x4xx_pwr_zbx_rx_0_rx1.cal
    cal/x4xx_pwr_zbx_rx_1_rx1.cal
# ZBX DSA data
    cal/zbx_dsa_tx.cal
    cal/zbx_dsa_rx.cal
)
set_property(TARGET uhd-resources PROPERTY POSITION_INDEPENDENT_CODE ON)
