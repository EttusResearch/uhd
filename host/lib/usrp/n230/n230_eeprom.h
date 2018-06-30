//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_EEPROM_H
#define INCLUDED_N230_EEPROM_H

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define N230_NUM_ETH_PORTS 2
#define N230_MAX_NUM_ETH_PORTS 2

#if (N230_NUM_ETH_PORTS > N230_MAX_NUM_ETH_PORTS)
#error
#endif

#define N230_EEPROM_VER_MAJOR     1
#define N230_EEPROM_VER_MINOR     1
#define N230_EEPROM_SERIAL_LEN    9
#define N230_EEPROM_NAME_LEN      32

typedef struct
{
    uint8_t  mac_addr[6];
    uint8_t  _pad[2];
    uint32_t subnet;
    uint32_t ip_addr;
} n230_eth_eeprom_map_t;

typedef struct
{
    //Data format version
    uint16_t data_version_major;
    uint16_t data_version_minor;

    //HW identification info
    uint16_t hw_revision;
    uint16_t hw_product;
    uint8_t serial[N230_EEPROM_SERIAL_LEN];
    uint8_t _pad_serial;
    uint16_t hw_revision_compat;
    uint8_t _pad0[18 - (N230_EEPROM_SERIAL_LEN + 1)];

    //Ethernet specific
    uint32_t gateway;
    n230_eth_eeprom_map_t eth_info[N230_MAX_NUM_ETH_PORTS];

    //User specific
    uint8_t user_name[N230_EEPROM_NAME_LEN];
} n230_eeprom_map_t;

#ifdef __cplusplus
} //extern "C"
#endif

// The following definitions are only useful in firmware. Exclude in host code.
#ifndef __cplusplus

/*!
 * Read the eeprom and update caches.
 * Returns true if read was successful.
 * If the read was not successful then the cache is initialized with
 * default values and marked as dirty.
 */
bool read_n230_eeprom();

/*!
 * Write the contents of the cache to the eeprom.
 * Returns true if write was successful.
 */
bool write_n230_eeprom();

/*!
 * Returns the dirty state of the cache.
 */
bool is_n230_eeprom_cache_dirty();

/*!
 * Returns a const pointer to the EEPROM map.
 */
const n230_eeprom_map_t* get_n230_const_eeprom_map();

/*!
 * Returns the settings for the the 'iface'th ethernet interface
 */
const n230_eth_eeprom_map_t* get_n230_ethernet_info(uint32_t iface);

/*!
 * Returns a non-const pointer to the EEPROM map. Will mark the cache as dirty.
 */
n230_eeprom_map_t* get_n230_eeprom_map();

/*!
 * FPGA Image operations
 */
inline void read_n230_fpga_image_page(uint32_t offset, void *buf, uint32_t num_bytes);

inline bool write_n230_fpga_image_page(uint32_t offset, const void *buf, uint32_t num_bytes);

inline bool erase_n230_fpga_image_sector(uint32_t offset);

#endif  //ifdef __cplusplus

#endif /* INCLUDED_N230_EEPROM_H */
