//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/usrp_info.h>

uhd_error uhd_usrp_rx_info_free(uhd_usrp_rx_info_t *rx_info){
    free(rx_info->mboard_id);
    free(rx_info->mboard_name);
    free(rx_info->mboard_serial);
    free(rx_info->rx_id);
    free(rx_info->rx_subdev_name);
    free(rx_info->rx_subdev_spec);
    free(rx_info->rx_serial);
    free(rx_info->rx_antenna);

    return UHD_ERROR_NONE;
}

uhd_error uhd_usrp_tx_info_free(uhd_usrp_tx_info_t *tx_info){
    free(tx_info->mboard_id);
    free(tx_info->mboard_name);
    free(tx_info->mboard_serial);
    free(tx_info->tx_id);
    free(tx_info->tx_subdev_name);
    free(tx_info->tx_subdev_spec);
    free(tx_info->tx_serial);
    free(tx_info->tx_antenna);

    return UHD_ERROR_NONE;
}
