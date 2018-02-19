//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_FW_HOST_IFACE_H
#define INCLUDED_N230_FW_HOST_IFACE_H

#include <stdint.h>

/*!
 * Structs and constants for N230 communication between firmware and host.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------
// Ethernet related
//
#define N230_DEFAULT_ETH0_MAC    {0x00, 0x50, 0xC2, 0x85, 0x3f, 0xff}
#define N230_DEFAULT_ETH1_MAC    {0x00, 0x50, 0xC2, 0x85, 0x3f, 0x33}
#define N230_DEFAULT_ETH0_IP     (192 << 24 | 168 << 16 | 10  << 8  | 2 << 0)
#define N230_DEFAULT_ETH1_IP     (192 << 24 | 168 << 16 | 20  << 8  | 2 << 0)
#define N230_DEFAULT_ETH0_MASK   (255 << 24 | 255 << 16 | 255 << 8  | 0 << 0)
#define N230_DEFAULT_ETH1_MASK   (255 << 24 | 255 << 16 | 255 << 8  | 0 << 0)
#define N230_DEFAULT_GATEWAY     (192 << 24 | 168 << 16 | 10  << 8  | 1 << 0)

#define N230_FW_COMMS_UDP_PORT        49152
#define N230_FW_COMMS_CVITA_PORT      49153
#define N230_FW_COMMS_FLASH_PROG_PORT 49154
//
//--------------------------------------------------

//--------------------------------------------------
// Memory shared with host
//
#define N230_FW_HOST_SHMEM_BASE_ADDR      0x10000
#define N230_FW_HOST_SHMEM_RW_BASE_ADDR   0x1000C
#define N230_FW_HOST_SHMEM_NUM_WORDS      (sizeof(n230_host_shared_mem_data_t)/sizeof(uint32_t))

#define N230_FW_HOST_SHMEM_MAX_ADDR  \
    (N230_FW_HOST_SHMEM_BASE_ADDR + ((N230_FW_HOST_SHMEM_NUM_WORDS - 1) * sizeof(uint32_t)))

#define N230_FW_HOST_SHMEM_OFFSET(member) \
    (N230_FW_HOST_SHMEM_BASE_ADDR + ((uint32_t)offsetof(n230_host_shared_mem_data_t, member)))

//The shared memory block can only be accessed on 32-bit boundaries
typedef struct {    //All fields must be 32-bit wide to avoid packing directives
    //Read-Only fields (N230_FW_HOST_SHMEM_BASE_ADDR)
    uint32_t fw_compat_num;     //Compat number must be at offset 0
    uint32_t fw_version_hash;
    uint32_t claim_status;

    //Read-Write fields (N230_FW_HOST_SHMEM_RW_BASE_ADDR)
    uint32_t scratch;
    uint32_t claim_time;
    uint32_t claim_src;
} n230_host_shared_mem_data_t;

typedef union
{
    uint32_t                    buff[N230_FW_HOST_SHMEM_NUM_WORDS];
    n230_host_shared_mem_data_t   data;
} n230_host_shared_mem_t;

#define N230_FW_PRODUCT_ID        1
#define N230_FW_COMPAT_NUM_MAJOR  32
#define N230_FW_COMPAT_NUM_MINOR  0
#define N230_FW_COMPAT_NUM        (((N230_FW_COMPAT_NUM_MAJOR & 0xFF) << 16) | (N230_FW_COMPAT_NUM_MINOR & 0xFFFF))
//
//--------------------------------------------------

//--------------------------------------------------
// Flash read-write interface for host
//
#define N230_FLASH_COMM_FLAGS_ACK           0x00000001
#define N230_FLASH_COMM_FLAGS_CMD_MASK      0x00000FF0
#define N230_FLASH_COMM_FLAGS_ERROR_MASK    0xFF000000

#define N230_FLASH_COMM_CMD_READ_NV_DATA    0x00000010
#define N230_FLASH_COMM_CMD_WRITE_NV_DATA   0x00000020
#define N230_FLASH_COMM_CMD_READ_FPGA       0x00000030
#define N230_FLASH_COMM_CMD_WRITE_FPGA      0x00000040
#define N230_FLASH_COMM_CMD_ERASE_FPGA      0x00000050

#define N230_FLASH_COMM_ERR_PKT_ERROR       0x80000000
#define N230_FLASH_COMM_ERR_CMD_ERROR       0x40000000
#define N230_FLASH_COMM_ERR_SIZE_ERROR      0x20000000

#define N230_FLASH_COMM_MAX_PAYLOAD_SIZE    128

typedef struct
{
    uint32_t flags;
    uint32_t seq;
    uint32_t offset;
    uint32_t size;
    uint8_t data[N230_FLASH_COMM_MAX_PAYLOAD_SIZE];
} n230_flash_prog_t;
//
//--------------------------------------------------

#define N230_HW_REVISION_COMPAT 1
#define N230_HW_REVISION_MIN    1


#define N230_CLAIMER_TIMEOUT_IN_MS        2000

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_N230_FW_HOST_IFACE_H */
