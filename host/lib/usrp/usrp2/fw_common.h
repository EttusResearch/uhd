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
    #define __stdint(type) boost::type
extern "C" {
#else
    #include <stdint.h>
    #define __stdint(type) type
#endif

//defines the protocol version in this shared header
//increment this value when the protocol is changed
#define USRP2_PROTO_VERSION 5

//used to differentiate control packets over data port
#define USRP2_INVALID_VRT_HEADER 0

// udp ports for the usrp2 communication
// Dynamic and/or private ports: 49152-65535
#define USRP2_UDP_CTRL_PORT 49152
#define USRP2_UDP_DATA_PORT 49153

typedef enum{
    USRP2_CTRL_ID_HUH_WHAT = ' ',
    //USRP2_CTRL_ID_FOR_SURE, //TODO error condition enums
    //USRP2_CTRL_ID_SUX_MAN,

    USRP2_CTRL_ID_WAZZUP_BRO = 'a',
    USRP2_CTRL_ID_WAZZUP_DUDE = 'A',

    USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO = 's',
    USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE = 'S',

    USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO = 'i',
    USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE = 'I',

    USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO = 'h',
    USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE = 'H',

    USRP2_CTRL_ID_POKE_THIS_REGISTER_FOR_ME_BRO = 'p',
    USRP2_CTRL_ID_OMG_POKED_REGISTER_SO_BAD_DUDE = 'P',

    USRP2_CTRL_ID_PEEK_AT_THIS_REGISTER_FOR_ME_BRO = 'r',
    USRP2_CTRL_ID_WOAH_I_DEFINITELY_PEEKED_IT_DUDE = 'R',

    USRP2_CTRL_ID_PEACE_OUT = '~'

} usrp2_ctrl_id_t;

typedef enum{
    USRP2_DIR_RX = 'r',
    USRP2_DIR_TX = 't'
} usrp2_dir_which_t;

typedef enum{
    USRP2_CLK_EDGE_RISE = 'r',
    USRP2_CLK_EDGE_FALL = 'f'
} usrp2_clk_edge_t;

typedef struct{
    __stdint(uint32_t) proto_ver;
    __stdint(uint32_t) id;
    __stdint(uint32_t) seq;
    union{
        __stdint(uint32_t) ip_addr;
        struct {
            __stdint(uint8_t) dev;
            __stdint(uint8_t) miso_edge;
            __stdint(uint8_t) mosi_edge;
            __stdint(uint8_t) readback;
            __stdint(uint32_t) data;
            __stdint(uint8_t) num_bits;
        } spi_args;
        struct {
            __stdint(uint8_t) addr;
            __stdint(uint8_t) bytes;
            __stdint(uint8_t) data[20];
        } i2c_args;
        struct {
            __stdint(uint32_t) addr;
            __stdint(uint32_t) data;
            __stdint(uint32_t) addrhi;
            __stdint(uint32_t) datahi;
            __stdint(uint8_t) num_bytes; //1, 2, 4, 8
        } poke_args;
    } data;
} usrp2_ctrl_data_t;

#undef __stdint
#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
