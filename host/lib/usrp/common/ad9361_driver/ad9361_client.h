//
// Copyright 2014 Ettus Research LLC
//

#ifndef INCLUDED_AD9361_CLIENT_SETTINGS_H
#define INCLUDED_AD9361_CLIENT_SETTINGS_H

#include <stdint.h>
#include <ad9361_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Frequency band settings
 */
typedef enum {
    AD9361_RX_BAND0,
    AD9361_RX_BAND1,
    AD9361_TX_BAND0
} frequency_band_t;

double ad9361_client_get_band_edge(ad9361_product_t product, frequency_band_t band);

/*!
 * Clocking mode
 */
typedef enum {
    AD9361_XTAL_P_CLK_PATH,
    AD9361_XTAL_N_CLK_PATH
} clocking_mode_t;

clocking_mode_t ad9361_client_get_clocking_mode(ad9361_product_t product);

/*!
 * Digital interface specific
 */
typedef enum {
    AD9361_DDR_FDD_LVCMOS,
    AD9361_DDR_FDD_LVDS
} digital_interface_mode_t;

digital_interface_mode_t ad9361_client_get_digital_interface_mode(ad9361_product_t product);

typedef struct {
    uint8_t rx_clk_delay;
    uint8_t rx_data_delay;
    uint8_t tx_clk_delay;
    uint8_t tx_data_delay;
} digital_interface_delays_t;

digital_interface_delays_t ad9361_client_get_digital_interface_timing(ad9361_product_t product);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_AD9361_CLIENT_SETTINGS_H */
