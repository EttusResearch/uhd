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
#define _SINS_
#endif

//used to differentiate control packets over data port
#define USRP2_INVALID_VRT_HEADER 0

// size of the vrt header and trailer to the host
#define USRP2_HOST_RX_VRT_HEADER_WORDS32 5
#define USRP2_HOST_RX_VRT_TRAILER_WORDS32 1 //FIXME fpga sets wrong header size when no trailer present

// udp ports for the usrp2 communication
// Dynamic and/or private ports: 49152-65535
#define USRP2_UDP_CTRL_PORT 49152
#define USRP2_UDP_DATA_PORT 49153

typedef enum{
    USRP2_CTRL_ID_HUH_WHAT = ' ',
    //USRP2_CTRL_ID_FOR_SURE, //TODO error condition enums
    //USRP2_CTRL_ID_SUX_MAN,

    USRP2_CTRL_ID_GIVE_ME_YOUR_IP_ADDR_BRO = 'a',
    USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE = 'A',
    USRP2_CTRL_ID_HERE_IS_A_NEW_IP_ADDR_BRO = 'b',

    USRP2_CTRL_ID_GIVE_ME_YOUR_MAC_ADDR_BRO = 'm',
    USRP2_CTRL_ID_THIS_IS_MY_MAC_ADDR_DUDE = 'M',
    USRP2_CTRL_ID_HERE_IS_A_NEW_MAC_ADDR_BRO = 'n',

    USRP2_CTRL_ID_GIVE_ME_YOUR_DBOARD_IDS_BRO = 'd',
    USRP2_CTRL_ID_THESE_ARE_MY_DBOARD_IDS_DUDE = 'D',

    USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO = 's',
    USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE = 'S',

    USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO = 'i',
    USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE = 'I',

    USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO = 'h',
    USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE = 'H',

    USRP2_CTRL_ID_WRITE_THIS_TO_THE_AUX_DAC_BRO = 'x',
    USRP2_CTRL_ID_DONE_WITH_THAT_AUX_DAC_DUDE = 'X',

    USRP2_CTRL_ID_READ_FROM_THIS_AUX_ADC_BRO = 'y',
    USRP2_CTRL_ID_DONE_WITH_THAT_AUX_ADC_DUDE = 'Y',

    USRP2_CTRL_ID_SEND_STREAM_COMMAND_FOR_ME_BRO = '{',
    USRP2_CTRL_ID_GOT_THAT_STREAM_COMMAND_DUDE = '}',

    USRP2_CTRL_ID_POKE_THIS_REGISTER_FOR_ME_BRO = 'p',
    USRP2_CTRL_ID_OMG_POKED_REGISTER_SO_BAD_DUDE = 'P',

    USRP2_CTRL_ID_PEEK_AT_THIS_REGISTER_FOR_ME_BRO = 'r',
    USRP2_CTRL_ID_WOAH_I_DEFINITELY_PEEKED_IT_DUDE = 'R',

    USRP2_CTRL_ID_PEACE_OUT = '~'

} usrp2_ctrl_id_t;

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
            _SINS_ uint8_t dev;
            _SINS_ uint8_t edge;
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
            _SINS_ uint8_t now; //stream now?
            _SINS_ uint8_t continuous; //auto-reload commmands?
            _SINS_ uint8_t chain;
            _SINS_ uint8_t _pad[1];
            _SINS_ uint32_t secs;
            _SINS_ uint32_t ticks;
            _SINS_ uint32_t num_samps;
        } stream_cmd;
        struct {
            _SINS_ uint32_t addr;
            _SINS_ uint32_t data;
            _SINS_ uint8_t num_bytes; //1, 2, 4
        } poke_args;
    } data;
} usrp2_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
