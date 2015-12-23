/*
 * Copyright 2014 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SPI_FLASH
#define INCLUDED_SPI_FLASH

#include <wb_spi.h>

//Device class that encapsulates the geometry and control
//interface for the flash chip
typedef struct {
    uint32_t page_size;     //in bytes
    uint32_t sector_size;   //in bytes
    uint32_t num_sectors;
    const wb_spi_slave_t* bus;
} spi_flash_dev_t;

//Low level device specific operations
typedef uint16_t (*spif_read_id_fn_t)(const spi_flash_dev_t* flash);
typedef void (*spif_read_fn_t)(const spi_flash_dev_t* flash, uint32_t offset, void *buf, uint32_t num_bytes);
typedef bool (*spif_erase_sector_dispatch_fn_t)(const spi_flash_dev_t* flash, uint32_t offset);
typedef bool (*spif_erase_sector_commit_fn_t)(const spi_flash_dev_t* flash, uint32_t offset);
typedef bool (*spif_erase_sector_busy_fn_t)(const spi_flash_dev_t* flash);
typedef bool (*spif_write_page_dispatch_fn_t)(const spi_flash_dev_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes);
typedef bool (*spif_write_page_commit_fn_t)(const spi_flash_dev_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes);
typedef bool (*spif_write_page_busy_fn_t)(const spi_flash_dev_t* flash);

//Interface struct for all low level device operations
typedef struct {
    spif_read_id_fn_t               read_id;
    spif_read_fn_t                  read;
    spif_erase_sector_dispatch_fn_t erase_sector_dispatch;
    spif_erase_sector_commit_fn_t   erase_sector_commit;
    spif_erase_sector_busy_fn_t     erase_sector_busy;
    spif_write_page_dispatch_fn_t   write_page_dispatch;
    spif_write_page_commit_fn_t     write_page_commit;
    spif_write_page_busy_fn_t       write_page_busy;
} spi_flash_ops_t;

typedef enum {
    IDLE, WRITE_IN_PROGRESS, ERASE_IN_PROGRESS
} spi_flash_state_t;

//A session struct that encapsulates everything about the flash
//in a device agnostic way
typedef struct {
    const spi_flash_dev_t*  device;
    const spi_flash_ops_t*  ops;
    spi_flash_state_t       state;
    uint32_t                last_offset;
    uint16_t                id;
} spi_flash_session_t;

/*!
 * Initialize the spi_flash_session_t object
 */
void spif_init(spi_flash_session_t* flash, const spi_flash_dev_t* device, const spi_flash_ops_t* ops);

/*!
 * Read "num_bytes" from "offset" in the flash into the buffer "buf".
 * This call will block until all data is available.
 */
void spif_read_sync(const spi_flash_session_t* flash, uint32_t offset, void *buf, uint32_t num_bytes);

/*!
 * Erase sector at "offset" in the flash.
 * This call will block until the erase is complete.
 */
bool spif_erase_sector_sync(const spi_flash_session_t* flash, uint32_t offset);

/*!
 * Write "num_bytes" from buffer "buf" at "offset" in the flash.
 * This call will block until the write is complete.
 */
bool spif_write_page_sync(const spi_flash_session_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes);


#endif /* INCLUDED_SPI_FLASH */
