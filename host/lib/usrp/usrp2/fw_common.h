//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_USRP2_FW_COMMON_H
#define INCLUDED_USRP2_FW_COMMON_H

/*!
 * Structs and constants for usrp2 communication.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
#include <boost/cstdint.hpp>
#define _SINS_ boost:://stdint namespace when in c++
extern "C" {
#else
#include <stdint.h>
#endif

// size of the vrt header and trailer to the host
#define USRP2_HOST_RX_VRT_HEADER_WORDS32 5
#define USRP2_HOST_RX_VRT_TRAILER_WORDS32 1 //FIXME fpga sets wrong header size when no trailer present

// udp ports for the usrp2 communication
// Dynamic and/or private ports: 49152-65535
#define USRP2_UDP_CTRL_PORT 49152
#define USRP2_UDP_DATA_PORT 49153

typedef enum{
    USRP2_CTRL_ID_HUH_WHAT,
    //USRP2_CTRL_ID_FOR_SURE, //TODO error condition enums
    //USRP2_CTRL_ID_SUX_MAN,

    USRP2_CTRL_ID_GIVE_ME_YOUR_IP_ADDR_BRO,
    USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE,
    USRP2_CTRL_ID_HERE_IS_A_NEW_IP_ADDR_BRO,

    USRP2_CTRL_ID_GIVE_ME_YOUR_MAC_ADDR_BRO,
    USRP2_CTRL_ID_THIS_IS_MY_MAC_ADDR_DUDE,
    USRP2_CTRL_ID_HERE_IS_A_NEW_MAC_ADDR_BRO,

    USRP2_CTRL_ID_GIVE_ME_YOUR_DBOARD_IDS_BRO,
    USRP2_CTRL_ID_THESE_ARE_MY_DBOARD_IDS_DUDE,

    USRP2_CTRL_ID_HERES_A_NEW_CLOCK_CONFIG_BRO,
    USRP2_CTRL_ID_GOT_THE_NEW_CLOCK_CONFIG_DUDE,

    USRP2_CTRL_ID_USE_THESE_GPIO_DDR_SETTINGS_BRO,
    USRP2_CTRL_ID_GOT_THE_GPIO_DDR_SETTINGS_DUDE,

    USRP2_CTRL_ID_SET_YOUR_GPIO_PIN_OUTS_BRO,
    USRP2_CTRL_ID_I_SET_THE_GPIO_PIN_OUTS_DUDE,

    USRP2_CTRL_ID_GIVE_ME_YOUR_GPIO_PIN_VALS_BRO,
    USRP2_CTRL_ID_HERE_IS_YOUR_GPIO_PIN_VALS_DUDE,

    USRP2_CTRL_ID_USE_THESE_ATR_SETTINGS_BRO,
    USRP2_CTRL_ID_GOT_THE_ATR_SETTINGS_DUDE,

    USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO,
    USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE,

    USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO,
    USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE,

    USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO,
    USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE,

    USRP2_CTRL_ID_WRITE_THIS_TO_THE_AUX_DAC_BRO,
    USRP2_CTRL_ID_DONE_WITH_THAT_AUX_DAC_DUDE,

    USRP2_CTRL_ID_READ_FROM_THIS_AUX_ADC_BRO,
    USRP2_CTRL_ID_DONE_WITH_THAT_AUX_ADC_DUDE,

    USRP2_CTRL_ID_SETUP_THIS_DDC_FOR_ME_BRO,
    USRP2_CTRL_ID_TOTALLY_SETUP_THE_DDC_DUDE,

    USRP2_CTRL_ID_CONFIGURE_STREAMING_FOR_ME_BRO,
    USRP2_CTRL_ID_CONFIGURED_THAT_STREAMING_DUDE,

    USRP2_CTRL_ID_SETUP_THIS_DUC_FOR_ME_BRO,
    USRP2_CTRL_ID_TOTALLY_SETUP_THE_DUC_DUDE,

    USRP2_CTRL_ID_GOT_A_NEW_TIME_FOR_YOU_BRO,
    USRP2_CTRL_ID_SWEET_I_GOT_THAT_TIME_DUDE,

    USRP2_CTRL_ID_UPDATE_THOSE_MUX_SETTINGS_BRO,
    USRP2_CTRL_ID_UPDATED_THE_MUX_SETTINGS_DUDE,

    USRP2_CTRL_ID_PEACE_OUT

} usrp2_ctrl_id_t;

typedef enum{
    USRP2_PPS_SOURCE_SMA,
    USRP2_PPS_SOURCE_MIMO
} usrp2_pps_source_t;

typedef enum{
    USRP2_PPS_POLARITY_POS,
    USRP2_PPS_POLARITY_NEG
} usrp2_pps_polarity_t;

typedef enum{
    USRP2_REF_SOURCE_INT,
    USRP2_REF_SOURCE_SMA,
    USRP2_REF_SOURCE_MIMO
} usrp2_ref_source_t;

typedef enum{
    USRP2_DIR_RX,
    USRP2_DIR_TX
} usrp2_dir_which_t;

typedef enum{
    USRP2_CLK_EDGE_RISE,
    USRP2_CLK_EDGE_FALL
} usrp2_clk_edge_t;

typedef struct{
    _SINS_ uint32_t id;
    _SINS_ uint32_t seq;
    union{
        _SINS_ uint32_t ip_addr;
        _SINS_ uint8_t mac_addr[6];
        struct {
            _SINS_ uint16_t rx_id;
            _SINS_ uint16_t tx_id;
        } dboard_ids;
        struct {
            _SINS_ uint8_t pps_source;
            _SINS_ uint8_t pps_polarity;
            _SINS_ uint8_t ref_source;
            _SINS_ uint8_t _pad;
        } clock_config;
        struct {
            _SINS_ uint8_t bank;
            _SINS_ uint8_t _pad[3];
            _SINS_ uint16_t value;
            _SINS_ uint16_t mask;
        } gpio_config;
        struct {
            _SINS_ uint8_t bank;
            _SINS_ uint8_t _pad[3];
            _SINS_ uint16_t tx_value;
            _SINS_ uint16_t rx_value;
            _SINS_ uint16_t mask;
        } atr_config;
        struct {
            _SINS_ uint8_t dev;
            _SINS_ uint8_t latch;
            _SINS_ uint8_t push;
            _SINS_ uint8_t readback;
            _SINS_ uint8_t bytes;
            _SINS_ uint8_t data[sizeof(_SINS_ uint32_t)];
        } spi_args;
        struct {
            _SINS_ uint8_t addr;
            _SINS_ uint8_t bytes;
            _SINS_ uint8_t data[sizeof(_SINS_ uint32_t)];
        } i2c_args;
        struct {
            _SINS_ uint8_t dir;
            _SINS_ uint8_t which;
            _SINS_ uint8_t _pad[2];
            _SINS_ uint32_t value;
        } aux_args;
        struct {
            _SINS_ uint32_t freq_word;
            _SINS_ uint32_t decim;
            _SINS_ uint32_t scale_iq;
        } ddc_args;
        struct {
            _SINS_ uint8_t enabled;
            _SINS_ uint8_t _pad[3];
            _SINS_ uint32_t secs;
            _SINS_ uint32_t ticks;
            _SINS_ uint32_t samples;
        } streaming;
        struct {
            _SINS_ uint32_t freq_word;
            _SINS_ uint32_t interp;
            _SINS_ uint32_t scale_iq;
        } duc_args;
        struct {
            _SINS_ uint32_t secs;
            _SINS_ uint32_t ticks;
            _SINS_ uint8_t now;
        } time_args;
        struct {
            _SINS_ uint32_t rx_mux;
            _SINS_ uint32_t tx_mux;
        } mux_args;
    } data;
} usrp2_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
