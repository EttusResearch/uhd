//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_KINTEX7SDR_FW_COMMON_H
#define INCLUDED_KINTEX7SDR_FW_COMMON_H

#include <stdint.h>

/*!
 * Structs and constants for kintex7sdr communication.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

//fpga and firmware compatibility numbers
#define KINTEX7SDR_FPGA_COMPAT_NUM 10
#define N200_FPGA_COMPAT_NUM 11
#define KINTEX7SDR_FW_COMPAT_NUM 12
#define KINTEX7SDR_FW_VER_MINOR 4

//used to differentiate control packets over data port
#define KINTEX7SDR_INVALID_VRT_HEADER 0

typedef struct{
    uint32_t sequence;
    uint32_t vrt_hdr;
    uint32_t ip_addr;
    uint32_t udp_port;
} kintex7sdr_stream_ctrl_t;

// udp ports for the kintex7sdr communication
// Dynamic and/or private ports: 49152-65535
#define KINTEX7SDR_UDP_CTRL_PORT 49152
//#define KINTEX7SDR_UDP_UPDATE_PORT 49154
#define KINTEX7SDR_UDP_RX_DSP0_PORT 49156
#define KINTEX7SDR_UDP_TX_DSP0_PORT 49157
#define KINTEX7SDR_UDP_RX_DSP1_PORT 49158
#define KINTEX7SDR_UDP_FIFO_CRTL_PORT 49159
#define KINTEX7SDR_UDP_UART_BASE_PORT 49170
#define KINTEX7SDR_UDP_UART_GPS_PORT 49172

// Map for virtual firmware regs (not very big so we can keep it here for now)
#define U2_FW_REG_LOCK_TIME 0
#define U2_FW_REG_LOCK_GPID 1
#define U2_FW_REG_HAS_GPSDO 3
#define U2_FW_REG_VER_MINOR 7

////////////////////////////////////////////////////////////////////////
// I2C addresses
////////////////////////////////////////////////////////////////////////
#define KINTEX7SDR_I2C_DEV_EEPROM  0x50 // 24LC02[45]:  7-bits 1010xxx
#define	KINTEX7SDR_I2C_ADDR_MBOARD (KINTEX7SDR_I2C_DEV_EEPROM | 0x0)
#define	KINTEX7SDR_I2C_ADDR_TX_DB  (KINTEX7SDR_I2C_DEV_EEPROM | 0x4)
#define	KINTEX7SDR_I2C_ADDR_RX_DB  (KINTEX7SDR_I2C_DEV_EEPROM | 0x5)

////////////////////////////////////////////////////////////////////////
// EEPROM Layout
////////////////////////////////////////////////////////////////////////
#define KINTEX7SDR_EE_MBOARD_REV      0x00 //2 bytes, little-endian (historic, don't blame me)
#define KINTEX7SDR_EE_MBOARD_MAC_ADDR 0x02 //6 bytes
#define KINTEX7SDR_EE_MBOARD_GATEWAY  0x38 //uint32, big-endian
#define KINTEX7SDR_EE_MBOARD_SUBNET   0x08 //uint32, big-endian
#define KINTEX7SDR_EE_MBOARD_IP_ADDR  0x0C //uint32, big-endian
#define KINTEX7SDR_EE_MBOARD_BOOTLOADER_FLAGS 0xF7

typedef enum{
    KINTEX7SDR_CTRL_ID_HUH_WHAT = ' ',
    //KINTEX7SDR_CTRL_ID_FOR_SURE, //TODO error condition enums
    //KINTEX7SDR_CTRL_ID_SUX_MAN,

    KINTEX7SDR_CTRL_ID_WAZZUP_BRO = 'w',
    KINTEX7SDR_CTRL_ID_WAZZUP_DUDE = 'W',

    KINTEX7SDR_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO = 's',
    KINTEX7SDR_CTRL_ID_OMG_TRANSACTED_SPI_DUDE = 'S',

    KINTEX7SDR_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO = 'i',
    KINTEX7SDR_CTRL_ID_HERES_THE_I2C_DATA_DUDE = 'I',

    KINTEX7SDR_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO = 'h',
    KINTEX7SDR_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE = 'H',

    KINTEX7SDR_CTRL_ID_GET_THIS_REGISTER_FOR_ME_BRO = 'r',
    KINTEX7SDR_CTRL_ID_OMG_GOT_REGISTER_SO_BAD_DUDE = 'R',

    KINTEX7SDR_CTRL_ID_HOLLER_AT_ME_BRO = 'l',
    KINTEX7SDR_CTRL_ID_HOLLER_BACK_DUDE = 'L',

    KINTEX7SDR_CTRL_ID_PEACE_OUT = '~'

} kintex7sdr_ctrl_id_t;

typedef enum{
    KINTEX7SDR_DIR_RX = 'r',
    KINTEX7SDR_DIR_TX = 't'
} kintex7sdr_dir_which_t;

typedef enum{
    KINTEX7SDR_CLK_EDGE_RISE = 'r',
    KINTEX7SDR_CLK_EDGE_FALL = 'f'
} kintex7sdr_clk_edge_t;

typedef enum{
    KINTEX7SDR_REG_ACTION_FPGA_PEEK32 = 1,
    KINTEX7SDR_REG_ACTION_FPGA_PEEK16 = 2,
    KINTEX7SDR_REG_ACTION_FPGA_POKE32 = 3,
    KINTEX7SDR_REG_ACTION_FPGA_POKE16 = 4,
    KINTEX7SDR_REG_ACTION_FW_PEEK32   = 5,
    KINTEX7SDR_REG_ACTION_FW_POKE32   = 6
} kintex7sdr_reg_action_t;

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
} kintex7sdr_ctrl_data_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_KINTEX7SDR_FW_COMMON_H */
