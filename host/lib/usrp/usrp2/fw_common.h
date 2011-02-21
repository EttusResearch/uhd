//
// Copyright 2010-2011 Ettus Research LLC
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

#include <stdint.h>

/*!
 * Structs and constants for usrp2 communication.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

//fpga and firmware compatibility numbers
#define USRP2_FPGA_COMPAT_NUM 5
#define USRP2_FW_COMPAT_NUM 9

//used to differentiate control packets over data port
#define USRP2_INVALID_VRT_HEADER 0

// udp ports for the usrp2 communication
// Dynamic and/or private ports: 49152-65535
#define USRP2_UDP_CTRL_PORT 49152
//#define USRP2_UDP_UPDATE_PORT 49154
#define USRP2_UDP_DSP0_PORT 49156
#define USRP2_UDP_ERR0_PORT 49157
#define USRP2_UDP_DSP1_PORT 49158

////////////////////////////////////////////////////////////////////////
// I2C addresses
////////////////////////////////////////////////////////////////////////
#define USRP2_I2C_DEV_EEPROM  0x50 // 24LC02[45]:  7-bits 1010xxx
#define	USRP2_I2C_ADDR_MBOARD (USRP2_I2C_DEV_EEPROM | 0x0)
#define	USRP2_I2C_ADDR_TX_DB  (USRP2_I2C_DEV_EEPROM | 0x4)
#define	USRP2_I2C_ADDR_RX_DB  (USRP2_I2C_DEV_EEPROM | 0x5)

////////////////////////////////////////////////////////////////////////
// EEPROM Layout
////////////////////////////////////////////////////////////////////////
#define USRP2_EE_MBOARD_REV      0x00 //2 bytes, little-endian (historic, don't blame me)
#define USRP2_EE_MBOARD_MAC_ADDR 0x02 //6 bytes
#define USRP2_EE_MBOARD_IP_ADDR  0x0C //uint32, big-endian
#define USRP2_EE_MBOARD_BOOTLOADER_FLAGS 0xF7

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

    USRP2_CTRL_ID_HEY_WRITE_THIS_UART_FOR_ME_BRO = 'u',
    USRP2_CTRL_ID_MAN_I_TOTALLY_WROTE_THAT_UART_DUDE = 'U',

    USRP2_CTRL_ID_SO_LIKE_CAN_YOU_READ_THIS_UART_BRO = 'v',
    USRP2_CTRL_ID_I_HELLA_READ_THAT_UART_DUDE = 'V',

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
    uint32_t proto_ver;
    uint32_t id;
    uint32_t seq;
    union{
        uint32_t ip_addr;
        struct {
            uint32_t dev;
            uint32_t data;
            uint8_t miso_edge;
            uint8_t mosi_edge;
            uint8_t num_bits;
            uint8_t readback;
        } spi_args;
        struct {
            uint8_t addr;
            uint8_t bytes;
            uint8_t data[20];
        } i2c_args;
        struct {
            uint32_t addr;
            uint32_t data;
            uint32_t _pad[2];
            uint8_t num_bytes; //1, 2, 4
        } poke_args;
        struct {
            uint8_t dev;
            uint8_t bytes;
            uint8_t data[20];
        } uart_args;
    } data;
} usrp2_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
