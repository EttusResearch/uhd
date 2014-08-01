//
// Copyright 2014 Ettus Research LLC
//

#include <ad9361_client.h>

double ad9361_client_get_band_edge(ad9361_product_t product, frequency_band_t band)
{
    switch (product) {
    default:
        switch (band) {
        case AD9361_RX_BAND0:   return 2.2e9;
        case AD9361_RX_BAND1:   return 4.0e9;
        case AD9361_TX_BAND0:   return 2.5e9;
        default:                return 0;
        }
    }
}

clocking_mode_t ad9361_client_get_clocking_mode(ad9361_product_t product)
{
    switch (product) {
    case AD9361_B200:
        return AD9361_XTAL_N_CLK_PATH;
    default:
        return AD9361_XTAL_N_CLK_PATH;
    }
}

digital_interface_mode_t ad9361_client_get_digital_interface_mode(ad9361_product_t product)
{
    switch (product) {
        case AD9361_B200:   return AD9361_DDR_FDD_LVCMOS;
        default:            return AD9361_DDR_FDD_LVCMOS;
    }
}

digital_interface_delays_t ad9361_client_get_digital_interface_timing(ad9361_product_t product)
{
    digital_interface_delays_t delays;
    switch (product) {
        case AD9361_B200:
            delays.rx_clk_delay = 0;
            delays.rx_data_delay = 0xF;
            delays.tx_clk_delay = 0;
            delays.tx_data_delay = 0xF;
            break;
        default:
            delays.rx_clk_delay = 0;
            delays.rx_data_delay = 0;
            delays.tx_clk_delay = 0;
            delays.tx_data_delay = 0;
            break;
    }
    return delays;
}
