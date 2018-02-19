/*
 * Copyright 2014-2016 Ettus Research LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _OCTOCLOCK_COMMON_H_
#define _OCTOCLOCK_COMMON_H_

#include <stdint.h>

/*
 * C++ macros used for code cleanliness and extern "C" declaration.
 */
#ifdef __cplusplus

#define UHD_OCTOCLOCK_SEND_AND_RECV(xport, fw_version, pkt_code, pkt_out, len, data) do {\
                                                                            pkt_out.proto_ver = fw_version; \
                                                                            pkt_out.code = pkt_code; \
                                                                            xport->send(boost::asio::buffer(&pkt_out, sizeof(octoclock_packet_t))); \
                                                                            len = xport->recv(boost::asio::buffer(data), 2);\
                                                                         } while(0)

#define UHD_OCTOCLOCK_PACKET_MATCHES(pkt_code, pkt_out, pkt_in, len)    (len > offsetof(octoclock_packet_t, data) and \
                                                                            pkt_in->sequence == pkt_out.sequence and \
                                                                            pkt_in->code == pkt_code)

extern "C" {
#endif

/*
 * This code is used by both the C firmware and C++ host driver, so
 * only valid C code should go in this section.
 */

// These values are placed in the octoclock_packet_t.proto_ver field
#define OCTOCLOCK_BOOTLOADER_PROTO_VER 1234
#define OCTOCLOCK_FW_MIN_COMPAT_NUM       2
#define OCTOCLOCK_FW_COMPAT_NUM           4

// UDP ports assigned for different tasks
#define OCTOCLOCK_UDP_CTRL_PORT   50000
#define OCTOCLOCK_UDP_GPSDO_PORT  50001
#define OCTOCLOCK_UDP_FW_PORT     50002
#define OCTOCLOCK_UDP_EEPROM_PORT 50003

typedef enum {
    NO_CODE,

    OCTOCLOCK_QUERY_CMD,
    OCTOCLOCK_QUERY_ACK,

    SEND_EEPROM_CMD,
    SEND_EEPROM_ACK,
    BURN_EEPROM_CMD,
    BURN_EEPROM_SUCCESS_ACK,
    BURN_EEPROM_FAILURE_ACK,
    CLEAR_EEPROM_CMD,
    CLEAR_EEPROM_ACK,

    SEND_STATE_CMD,
    SEND_STATE_ACK,

    RESET_CMD,
    RESET_ACK,

    HOST_SEND_TO_GPSDO_CMD,
    HOST_SEND_TO_GPSDO_ACK,
    SEND_POOLSIZE_CMD,
    SEND_POOLSIZE_ACK,
    SEND_CACHE_STATE_CMD,
    SEND_CACHE_STATE_ACK,
    SEND_GPSDO_CACHE_CMD,
    SEND_GPSDO_CACHE_ACK,

    PREPARE_FW_BURN_CMD,
    FW_BURN_READY_ACK,
    FILE_TRANSFER_CMD,
    FILE_TRANSFER_ACK,
    READ_FW_CMD,
    READ_FW_ACK,
    FINALIZE_BURNING_CMD,
    FINALIZE_BURNING_ACK,
} packet_code_t;

typedef enum {
    NO_REF,
    INTERNAL,
    EXTERNAL
} ref_t;

typedef enum {
    PREFER_INTERNAL,
    PREFER_EXTERNAL
} switch_pos_t;

/*
 * Some versions of AVR-GCC ignore #pragma pack, so
 * if AVR-GCC is being used, use __attribute__
 * instead.
 */
#ifdef AVR
#define __AVR_ALIGNED__ __attribute__((aligned(1)))
#else
#define __AVR_ALIGNED__
#pragma pack(push,1)
#endif

// Structure of values in EEPROM, starting in location 0
typedef struct {
    uint8_t mac_addr[6];
    uint32_t ip_addr;
    uint32_t dr_addr;
    uint32_t netmask;
    uint8_t serial[10];
    uint8_t name[10];
    uint8_t revision;
} octoclock_fw_eeprom_t __AVR_ALIGNED__;

typedef struct {
    uint8_t external_detected;
    uint8_t gps_detected;
    uint8_t which_ref;
    uint8_t switch_pos;
} octoclock_state_t __AVR_ALIGNED__;

typedef struct {
    uint8_t num_wraps;
    uint8_t pos;
} gpsdo_cache_state_t __AVR_ALIGNED__;

typedef struct {
    uint32_t proto_ver;
    uint32_t sequence;
    uint8_t code;
    union {
        uint16_t crc;
        gpsdo_cache_state_t state;
        uint16_t poolsize;
        uint16_t addr;
    };
    uint8_t data[256];
    uint16_t len;
} octoclock_packet_t __AVR_ALIGNED__;

#ifndef AVR
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OCTOCLOCK_COMMON_H_ */
