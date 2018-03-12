//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_FW_COMMON_H
#define INCLUDED_X300_FW_COMMON_H

#include <stdint.h>

/*!
 * Structs and constants for x300 communication.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

#define X300_REVISION_COMPAT 7
#define X300_REVISION_MIN    2
#define X300_FW_COMPAT_MAJOR 6
#define X300_FW_COMPAT_MINOR 0
#define X300_FPGA_COMPAT_MAJOR 0x23

//shared memory sections - in between the stack and the program space
#define X300_FW_SHMEM_BASE 0x6000
#define X300_FW_SHMEM_COMPAT_NUM 0
#define X300_FW_SHMEM_GPSDO_STATUS 1
#define X300_FW_SHMEM_UART_RX_INDEX 2
#define X300_FW_SHMEM_UART_TX_INDEX 3
#define X300_FW_SHMEM_CLAIM_STATUS 5
#define X300_FW_SHMEM_CLAIM_TIME 6
#define X300_FW_SHMEM_CLAIM_SRC 7
#define X300_FW_SHMEM_UART_RX_ADDR 8
#define X300_FW_SHMEM_UART_TX_ADDR 9
#define X300_FW_SHMEM_UART_WORDS32 10
#define X300_FW_SHMEM_ROUTE_MAP_ADDR 11
#define X300_FW_SHMEM_ROUTE_MAP_LEN 12
#define X300_FW_SHMEM_IDENT 13 // (13-39) EEPROM values in use
#define X300_FW_SHMEM_DEBUG 128
#define X300_FW_SHMEM_ADDR(offset) X300_FW_SHMEM_BASE + (4 * (offset))

#define X300_FW_NUM_BYTES (1 << 15) //64k
#define X300_FW_COMMS_MTU (1 << 13) //8k
#define X300_FW_COMMS_UDP_PORT 49152

#define X300_VITA_UDP_PORT 49153
#define X300_GPSDO_UDP_PORT 49156
#define X300_FPGA_PROG_UDP_PORT 49157
#define X300_MTU_DETECT_UDP_PORT 49158
#define X300_FPGA_READ_UDP_PORT 49159

#define X300_DEFAULT_MAC_ADDR_0         {0x00, 0x50, 0xC2, 0x85, 0x3f, 0xff}
#define X300_DEFAULT_MAC_ADDR_1         {0x00, 0x50, 0xC2, 0x85, 0x3f, 0x33}

#define X300_DEFAULT_GATEWAY            (192 << 24 | 168 << 16 | 10  << 8  | 1 << 0)

#define X300_DEFAULT_IP_ETH0_1G         (192 << 24 | 168 << 16 | 10  << 8  | 2 << 0)
#define X300_DEFAULT_IP_ETH1_1G         (192 << 24 | 168 << 16 | 20  << 8  | 2 << 0)
#define X300_DEFAULT_IP_ETH0_10G        (192 << 24 | 168 << 16 | 30  << 8  | 2 << 0)
#define X300_DEFAULT_IP_ETH1_10G        (192 << 24 | 168 << 16 | 40  << 8  | 2 << 0)

#define X300_DEFAULT_NETMASK_ETH0_1G    (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)
#define X300_DEFAULT_NETMASK_ETH1_1G    (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)
#define X300_DEFAULT_NETMASK_ETH0_10G   (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)
#define X300_DEFAULT_NETMASK_ETH1_10G   (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)

#define X300_FW_COMMS_FLAGS_ACK        (1 << 0)
#define X300_FW_COMMS_FLAGS_ERROR      (1 << 1)
#define X300_FW_COMMS_FLAGS_POKE32     (1 << 2)
#define X300_FW_COMMS_FLAGS_PEEK32     (1 << 3)

#define X300_FPGA_PROG_FLAGS_ACK       (1 << 0)
#define X300_FPGA_PROG_FLAGS_ERROR     (1 << 1)
#define X300_FPGA_PROG_FLAGS_INIT      (1 << 2)
#define X300_FPGA_PROG_FLAGS_CLEANUP   (1 << 3)
#define X300_FPGA_PROG_FLAGS_ERASE     (1 << 4)
#define X300_FPGA_PROG_FLAGS_VERIFY    (1 << 5)
#define X300_FPGA_PROG_CONFIGURE       (1 << 6)
#define X300_FPGA_PROG_CONFIG_STATUS   (1 << 7)

#define X300_FPGA_READ_FLAGS_ACK       (1 << 0)
#define X300_FPGA_READ_FLAGS_ERROR     (1 << 1)
#define X300_FPGA_READ_FLAGS_INIT      (1 << 2)
#define X300_FPGA_READ_FLAGS_CLEANUP   (1 << 3)

#define X300_MTU_DETECT_ECHO_REQUEST (1 << 0)
#define X300_MTU_DETECT_ECHO_REPLY (1 << 1)
#define X300_MTU_DETECT_ERROR (1 << 2)

typedef struct
{
    //indentifying numbers
    unsigned char revision[2];
    unsigned char product[2];
    uint8_t _pad0[4];

    //all the mac addrs
    uint8_t mac_addr0[6];
    uint8_t _pad1[2];
    uint8_t mac_addr1[6];
    uint8_t _pad2[2];

    //all the IP addrs
    uint32_t gateway;
    uint32_t subnet[4];
    uint32_t ip_addr[4];
    uint8_t _pad3[16];

    //names and serials
    unsigned char name[23];
    unsigned char serial[9];
} x300_eeprom_map_t;

typedef struct
{
    uint32_t flags;
    uint32_t sequence;
    uint32_t addr;
    uint32_t data;
} x300_fw_comms_t;

typedef struct
{
    uint32_t flags;
    uint32_t sector;
    uint32_t index;
    uint32_t size;
    uint16_t data[128];
} x300_fpga_prog_t;

typedef struct
{
    uint32_t flags;
} x300_fpga_prog_flags_t;

typedef struct
{
    uint32_t flags;
    uint32_t sector;
    uint32_t index;
    uint32_t size;
} x300_fpga_read_t;

typedef x300_fpga_prog_t x300_fpga_read_reply_t;

typedef struct
{
    uint32_t flags;
    uint32_t size;
} x300_mtu_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_X300_FW_COMMON_H */
