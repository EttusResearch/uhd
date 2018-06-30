/*
 * Copyright 2015 Ettus Research LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_UHD_TYPES_USRP_INFO_H
#define INCLUDED_UHD_TYPES_USRP_INFO_H

#include <uhd/config.h>
#include <uhd/error.h>

//! USRP RX info
/*!
 * This struct is populated by uhd_usrp_get_rx_info().
 */
typedef struct {
    //! Motherboard ID
    char* mboard_id;
    //! Motherboard name
    char* mboard_name;
    //! Motherboard serial
    char* mboard_serial;
    //! RX daughterboard ID
    char* rx_id;
    //! RX subdev name
    char* rx_subdev_name;
    //! RX subdev spec
    char* rx_subdev_spec;
    //! RX daughterboard serial
    char* rx_serial;
    //! RX daughterboard antenna
    char* rx_antenna;
} uhd_usrp_rx_info_t;

//! USRP TX info
/*!
 * This struct is populated by uhd_usrp_get_tx_info().
 */
typedef struct {
    //! Motherboard ID
    char* mboard_id;
    //! Motherboard name
    char* mboard_name;
    //! Motherboard serial
    char* mboard_serial;
    //! TX daughterboard ID
    char* tx_id;
    //! TX subdev name
    char* tx_subdev_name;
    //! TX subdev spec
    char* tx_subdev_spec;
    //! TX daughterboard serial
    char* tx_serial;
    //! TX daughterboard antenna
    char* tx_antenna;
} uhd_usrp_tx_info_t;

#ifdef __cplusplus
extern "C" {
#endif

//! Clean up a uhd_usrp_rx_info_t populated by uhd_usrp_get_rx_info().
/*!
 * NOTE: If this function is passed a uhd_usrp_rx_info_t that has not
 * been populated by uhd_usrp_get_rx_info(), it will produce a double-free
 * error.
 */
UHD_API uhd_error uhd_usrp_rx_info_free(uhd_usrp_rx_info_t *rx_info);

//! Clean up a uhd_usrp_tx_info_t populated by uhd_usrp_get_tx_info().
/*!
 * NOTE: If this function is passed a uhd_usrp_tx_info_t that has not
 * been populated by uhd_usrp_get_tx_info(), it will produce a double-free
 * error.
 */
UHD_API uhd_error uhd_usrp_tx_info_free(uhd_usrp_tx_info_t *tx_info);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_TYPES_USRP_INFO_H */
