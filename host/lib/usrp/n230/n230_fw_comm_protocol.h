//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_FW_COMM_PROTOCOL
#define INCLUDED_N230_FW_COMM_PROTOCOL

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

/*!
 * Structs and constants for communication between firmware and host.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

#define FW_COMM_PROTOCOL_SIGNATURE  0xACE3
#define FW_COMM_PROTOCOL_VERSION    0
#define FW_COMM_MAX_DATA_WORDS      16
#define FW_COMM_PROTOCOL_MTU        256

#define FW_COMM_FLAGS_ACK           0x00000001
#define FW_COMM_FLAGS_CMD_MASK      0x00000FF0
#define FW_COMM_FLAGS_ERROR_MASK    0xFF000000

#define FW_COMM_CMD_ECHO            0x00000000
#define FW_COMM_CMD_POKE32          0x00000010
#define FW_COMM_CMD_PEEK32          0x00000020
#define FW_COMM_CMD_BLOCK_POKE32    0x00000030
#define FW_COMM_CMD_BLOCK_PEEK32    0x00000040

#define FW_COMM_ERR_PKT_ERROR       0x80000000
#define FW_COMM_ERR_CMD_ERROR       0x40000000
#define FW_COMM_ERR_SIZE_ERROR      0x20000000

#define FW_COMM_GENERATE_ID(prod)   ((((uint32_t) FW_COMM_PROTOCOL_SIGNATURE) << 0)  | \
                                     (((uint32_t) prod)                       << 16) | \
                                     (((uint32_t) FW_COMM_PROTOCOL_VERSION)   << 24))

#define FW_COMM_GET_PROTOCOL_SIG(id) ((uint16_t)(id & 0xFFFF))
#define FW_COMM_GET_PRODUCT_ID(id)   ((uint8_t)(id >> 16))
#define FW_COMM_GET_PROTOCOL_VER(id) ((uint8_t)(id >> 24))

typedef struct
{
    uint32_t id;            //Protocol and device identifier
    uint32_t flags;         //Holds commands and ack messages
    uint32_t sequence;      //Sequence number (specific to FW communication transactions)
    uint32_t data_words;    //Number of data words in payload
    uint32_t addr;          //Address field for the command in flags
    uint32_t data[FW_COMM_MAX_DATA_WORDS];  //Data field for the command in flags
} fw_comm_pkt_t;

#ifdef __cplusplus
} //extern "C"
#endif

// The following definitions are only useful in firmware. Exclude in host code.
#ifndef __cplusplus

typedef void (*poke32_func)(const uint32_t addr, const uint32_t data);
typedef uint32_t (*peek32_func)(const uint32_t addr);

/*!
 * Process a firmware communication packet and compute a response.
 * Args:
 * - (in) request: Pointer to the request struct
 * - (out) response: Pointer to the response struct
 * - (in) product_id: The 8-bit usrp3 specific product ID (for request filtering)
 * - (func) poke_callback, peek_callback: Callback functions for a single peek/poke
 * - return value: Send a response packet
 */
bool process_fw_comm_protocol_pkt(
    const fw_comm_pkt_t* request,
    fw_comm_pkt_t* response,
    uint8_t product_id,
    uint32_t iface_id,
    poke32_func poke_callback,
    peek32_func peek_callback
);

#endif  //ifdef __cplusplus

#endif /* INCLUDED_N230_FW_COMM_PROTOCOL */
