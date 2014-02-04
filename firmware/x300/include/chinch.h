//
// Copyright 2013 Ettus Research LLC
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
#ifndef INCLUDED_CHINCH_H
#define INCLUDED_CHINCH_H

#include <wb_utils.h>
#include <stdbool.h>
#include <printf.h>

#define STATUS_CHAIN(x, status) if (status) status = (x)
#define STATUS_MERGE(x, status) status &= (x)
#define STATUS_CHAIN_DBG(x, status) STATUS_CHAIN(x, status); printf("%s: %s\n", #x, status?"succeeded":"failed!")

//The unit for this timeout is somewhat arbitrary. We could use the counter reg to enforce this in
//terms of clock cycles but that is not worth the extra code
static const uint32_t CHINCH_DEFAULT_XACT_TIMEOUT   = 32768;
static const uint32_t CHINCH_FLASH_MAX_BUF_WRITES   = 32;

//-----------------------------------------------------
// Peek-Poke interface for the Chinch
//-----------------------------------------------------
bool chinch_poke(const uint32_t addr, const uint32_t data, bool half_word, uint32_t timeout);
bool chinch_peek(const uint32_t addr, uint32_t* data, bool half_word, uint32_t timeout);

static inline bool chinch_poke32(const uint32_t addr, const uint32_t data) {
    return chinch_poke(addr, data, false /*half word*/, CHINCH_DEFAULT_XACT_TIMEOUT);
}
static inline bool chinch_poke16(const uint32_t addr, const uint32_t data) {
    return chinch_poke(addr, data, true /*half word*/, CHINCH_DEFAULT_XACT_TIMEOUT);
}
static inline bool chinch_peek32(const uint32_t addr, uint32_t* data) {
    return chinch_peek(addr, data, false /*half word*/, CHINCH_DEFAULT_XACT_TIMEOUT);
}
static inline bool chinch_peek16(const uint32_t addr, uint32_t* data) {
    return chinch_peek(addr, data, true /*half word*/, CHINCH_DEFAULT_XACT_TIMEOUT);
}

//-----------------------------------------------------
// Flash access
//-----------------------------------------------------
bool chinch_flash_init();
void chinch_flash_cleanup();
bool chinch_flash_select_sector(uint32_t sector);
bool chinch_flash_erase_sector();
bool chinch_flash_read_buf(uint32_t offset, uint16_t* buf, uint32_t size);
bool chinch_flash_write_buf(uint32_t offset, uint16_t* buf, uint32_t size);

static inline bool chinch_flash_read(uint32_t offset, uint16_t* data) {
    return chinch_flash_read_buf(offset, data, 1);
}
static inline bool chinch_flash_write(uint32_t offset, uint16_t data) {
    return chinch_flash_write_buf(offset, &data, 1);
}

//-----------------------------------------------------
// FPGA Configuration
//-----------------------------------------------------
typedef uint8_t config_status_t;
static const config_status_t CHINCH_CONFIG_RUNNING   = 0;
static const config_status_t CHINCH_CONFIG_DISABLED  = 1;
static const config_status_t CHINCH_CONFIG_COMPLETED = 2;
static const config_status_t CHINCH_CONFIG_ERROR     = 3;

void chinch_start_config();   //Caution: This operation will make the ZPU self-destruct!
config_status_t chinch_get_config_status();

//-----------------------------------------------------
// Read-back interface for user initiated
// PCIe register transactions
//-----------------------------------------------------
typedef uint8_t pcie_xact_size_t;
static const pcie_xact_size_t PCIE_XACT_32_BIT = 0;
static const pcie_xact_size_t PCIE_XACT_16_BIT = 1;

typedef uint8_t pcie_xact_t;
static const pcie_xact_t PCIE_XACT_ERROR = 0;
static const pcie_xact_t PCIE_XACT_READ  = 1;
static const pcie_xact_t PCIE_XACT_WRITE = 2;

typedef bool (*pcie_register_xact_responder_t)(uint32_t response, uint32_t timeout);

typedef struct
{
    pcie_xact_t                     type;
    uint32_t                        addr;
    uint32_t                        data;
    pcie_xact_size_t                size;
    pcie_register_xact_responder_t  respond;
} pcie_register_xact_t;

bool check_pcie_user_regport(pcie_register_xact_t** xact_info_hdl);
bool forward_pcie_user_xact_to_wb();


#endif /* INCLUDED_CHINCH_H */
