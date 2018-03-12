//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
#define USRP2_FPGA_COMPAT_NUM 10
#define N200_FPGA_COMPAT_NUM 11
#define USRP2_FW_COMPAT_NUM 12
#define USRP2_FW_VER_MINOR 4

//used to differentiate control packets over data port
#define USRP2_INVALID_VRT_HEADER 0

typedef struct{
    uint32_t sequence;
    uint32_t vrt_hdr;
    uint32_t ip_addr;
    uint32_t udp_port;
} usrp2_stream_ctrl_t;

// udp ports for the usrp2 communication
// Dynamic and/or private ports: 49152-65535
#define USRP2_UDP_CTRL_PORT 49152
//#define USRP2_UDP_UPDATE_PORT 49154
#define USRP2_UDP_RX_DSP0_PORT 49156
#define USRP2_UDP_TX_DSP0_PORT 49157
#define USRP2_UDP_RX_DSP1_PORT 49158
#define USRP2_UDP_FIFO_CRTL_PORT 49159
#define USRP2_UDP_UART_BASE_PORT 49170
#define USRP2_UDP_UART_GPS_PORT 49172

// Map for virtual firmware regs (not very big so we can keep it here for now)
#define U2_FW_REG_LOCK_TIME 0
#define U2_FW_REG_LOCK_GPID 1
#define U2_FW_REG_HAS_GPSDO 3
#define U2_FW_REG_VER_MINOR 7

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
#define USRP2_EE_MBOARD_GATEWAY  0x38 //uint32, big-endian
#define USRP2_EE_MBOARD_SUBNET   0x08 //uint32, big-endian
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

    USRP2_CTRL_ID_GET_THIS_REGISTER_FOR_ME_BRO = 'r',
    USRP2_CTRL_ID_OMG_GOT_REGISTER_SO_BAD_DUDE = 'R',

    USRP2_CTRL_ID_HOLLER_AT_ME_BRO = 'l',
    USRP2_CTRL_ID_HOLLER_BACK_DUDE = 'L',

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

typedef enum{
    USRP2_REG_ACTION_FPGA_PEEK32 = 1,
    USRP2_REG_ACTION_FPGA_PEEK16 = 2,
    USRP2_REG_ACTION_FPGA_POKE32 = 3,
    USRP2_REG_ACTION_FPGA_POKE16 = 4,
    USRP2_REG_ACTION_FW_PEEK32   = 5,
    USRP2_REG_ACTION_FW_POKE32   = 6
} usrp2_reg_action_t;

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
            uint8_t action;
        } reg_args;
        struct {
            uint32_t len;
        } echo_args;
    } data;
} usrp2_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_USRP2_FW_COMMON_H */
