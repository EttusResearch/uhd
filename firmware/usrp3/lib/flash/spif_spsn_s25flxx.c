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

#include <wb_spi.h>
#include <flash/spif_spsn_s25flxx.h>
#include <cron.h>
#include <trace.h>
#include <string.h> //for memset, memcpy

#define S25FLXX_CMD_WIDTH       8
#define S25FLXX_ADDR_WIDTH      24

/* S25FLxx-specific commands */
#define S25FLXX_CMD_READID      0x90    /* Read Manufacturer and Device Identification */
#define S25FLXX_CMD_READSIG     0xAB    /* Read Electronic Signature (Will release from Deep PD) */
#define S25FLXX_CMD_READ        0x03    /* Read Data Bytes */
#define S25FLXX_CMD_FAST_READ   0x0B    /* Read Data Bytes at Higher Speed */

#define S25FLXX_CMD_WREN        0x06    /* Write Enable */
#define S25FLXX_CMD_WRDI        0x04    /* Write Disable */

#define S25FLXX_CMD_PP          0x02    /* Page Program */
#define S25FLXX_CMD_SE          0xD8    /* Sector Erase */
#define S25FLXX_CMD_BE          0xC7    /* Bulk Erase */
#define S25FLXX_CMD_DP          0xB9    /* Deep Power-down */

#define S25FLXX_CMD_RDSR        0x05    /* Read Status Register */
#define S25FLXX_CMD_WRSR        0x01    /* Write Status Register */

#define S25FLXX_STATUS_WIP      0x01    /* Write in Progress */
#define S25FLXX_STATUS_E_ERR    0x20    /* Erase Error Occured */
#define S25FLXX_STATUS_P_ERR    0x40    /* Programming Error Occured */

#define S25FLXX_SECTOR_ERASE_TIME_MS    750     //Spec: 650ms
#define S25FLXX_PAGE_WRITE_TIME_MS      1       //Spec: 750us

#define S25FLXX_SMALL_SECTORS_PER_LOGICAL   16      //16 4-kB physical sectors per logical sector
#define S25FLXX_LARGE_SECTOR_BASE           0x20000 //Large physical sectors start at logical sector 2

inline static uint8_t _spif_read_status(const spi_flash_dev_t* flash)
{
    uint16_t cmd = S25FLXX_CMD_RDSR << 8, status = 0xFFFF;
    wb_spi_transact(flash->bus, WRITE_READ, &cmd, &status, S25FLXX_CMD_WIDTH + 8 /* 8 bits of status */);
    return status;
}

inline static bool _spif_wait_ready(const spi_flash_dev_t* flash, uint32_t timeout_ms)
{
    uint32_t start_ticks = cron_get_ticks();
    do {
        if ((_spif_read_status(flash) & S25FLXX_STATUS_WIP) == 0) {
            return true;
        }
    } while (get_elapsed_time(start_ticks, cron_get_ticks(), MILLISEC) < timeout_ms);

    return false;  // Timed out
}

inline static void _spi_flash_set_write_enabled(const spi_flash_dev_t* flash, bool enabled)
{
    uint8_t cmd = enabled ? S25FLXX_CMD_WREN : S25FLXX_CMD_WRDI;
    wb_spi_transact(flash->bus, WRITE, &cmd, NULL, S25FLXX_CMD_WIDTH);
}

const spi_flash_ops_t spif_spsn_s25flxx_ops =
{
    .read_id = spif_spsn_s25flxx_read_id,
    .read = spif_spsn_s25flxx_read,
    .erase_sector_dispatch = spif_spsn_s25flxx_erase_sector_dispatch,
    .erase_sector_commit = spif_spsn_s25flxx_erase_sector_commit,
    .erase_sector_busy = spif_spsn_s25flxx_device_busy,
    .write_page_dispatch = spif_spsn_s25flxx_write_page_dispatch,
    .write_page_commit = spif_spsn_s25flxx_write_page_commit,
    .write_page_busy = spif_spsn_s25flxx_device_busy
};

const spi_flash_ops_t* spif_spsn_s25flxx_operations()
{
    return &spif_spsn_s25flxx_ops;
}

uint16_t spif_spsn_s25flxx_read_id(const spi_flash_dev_t* flash)
{
    wb_spi_slave_select(flash->bus);
    uint32_t command = S25FLXX_CMD_READID << 24;
    wb_spi_transact_man_ss(flash->bus, WRITE, &command, NULL, 32);
    uint16_t id = 0;
    wb_spi_transact_man_ss(flash->bus, WRITE_READ, NULL, &id, 16);
    wb_spi_slave_deselect(flash->bus);
    return id;
}

void spif_spsn_s25flxx_read(const spi_flash_dev_t* flash, uint32_t offset, void *buf, uint32_t num_bytes)
{
    //We explicitly control the slave select here, so that we can
    //do the entire read operation as a single transaction from
    //device's point of view. (The most our SPI peripheral can transfer
    //in a single shot is 16 bytes.)

    //Do the 5 byte instruction tranfer:
    //FAST_READ_CMD, ADDR2, ADDR1, ADDR0, DUMMY (0)
    uint8_t read_cmd[5];
    read_cmd[4] = S25FLXX_CMD_FAST_READ;
    *((uint32_t*)(read_cmd + 3)) = (offset << 8);

    wb_spi_slave_select(flash->bus);
    wb_spi_transact_man_ss(flash->bus, WRITE_READ, read_cmd, NULL, 5*8);

    //Read up to 4 bytes at a time until done
    uint8_t data_sw[16], data[16];
    size_t xact_size = 16;
    unsigned char *bytes = (unsigned char *) buf;
    for (size_t i = 0; i < num_bytes; i += 16) {
        if (xact_size > num_bytes - i) xact_size = num_bytes - i;
        wb_spi_transact_man_ss(flash->bus, WRITE_READ, NULL, data_sw, xact_size*8);
        for (size_t k = 0; k < 4; k++) {    //Fix word level significance
            ((uint32_t*)data)[k] = ((uint32_t*)data_sw)[3-k];
        }
        for (size_t j = 0; j < xact_size; j++) {
            *bytes = data[j];
            bytes++;
        }
    }
    wb_spi_slave_deselect(flash->bus);
}

bool spif_spsn_s25flxx_erase_sector_dispatch(const spi_flash_dev_t* flash, uint32_t offset)
{
    //Sanity check sector size
    if (offset % flash->sector_size) {
        UHD_FW_TRACE(ERROR, "spif_spsn_s25flxx_erase_sector: Erase offset not a multiple of sector size.");
        return false;
    }

    if (!_spif_wait_ready(flash, S25FLXX_SECTOR_ERASE_TIME_MS)) {
        UHD_FW_TRACE_FSTR(ERROR, "spif_spsn_s25flxx_erase_sector: Timeout. Sector at 0x%X was not ready for erase.", offset);
        return false;
    }
    _spi_flash_set_write_enabled(flash, true);

    //Send sector erase command
    uint32_t command = (S25FLXX_CMD_SE << 24) | (offset & 0x00FFFFFF);
    wb_spi_transact(flash->bus, WRITE_READ, &command, NULL, 32);

    return true;
}

bool spif_spsn_s25flxx_erase_sector_commit(const spi_flash_dev_t* flash, uint32_t offset)
{
    //Poll status until write done
    uint8_t phy_sector_count = (offset < S25FLXX_LARGE_SECTOR_BASE) ? S25FLXX_SMALL_SECTORS_PER_LOGICAL : 1;
    bool status = false;
    for (uint8_t i = 0; i < phy_sector_count && !status; i++) {
        status = _spif_wait_ready(flash, S25FLXX_SECTOR_ERASE_TIME_MS);
    }
    if (!status) {
        UHD_FW_TRACE_FSTR(ERROR, "spif_spsn_s25flxx_erase_sector_commit: Timeout. Sector at 0x%X did not finish erasing in time.", offset);
    }
    _spi_flash_set_write_enabled(flash, false);
    return status;
}

bool spif_spsn_s25flxx_write_page_dispatch(const spi_flash_dev_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes)
{
    if (num_bytes == 0 || num_bytes > flash->page_size) {
        UHD_FW_TRACE(ERROR, "spif_spsn_s25flxx_write_page: Invalid size. Must be > 0 and <= Page Size.");
        return false;
    }
    if (num_bytes > (flash->sector_size * flash->num_sectors)) {
        UHD_FW_TRACE(ERROR, "spif_spsn_s25flxx_write_page: Cannot write past flash boundary.");
        return false;
    }

    //Wait until ready and enable write enabled
    if (!_spif_wait_ready(flash, S25FLXX_PAGE_WRITE_TIME_MS)) {
        UHD_FW_TRACE_FSTR(ERROR, "spif_spsn_s25flxx_write_page: Timeout. Page at 0x%X was not ready for write.", offset);
        return false;
    }
    _spi_flash_set_write_enabled(flash, true);

    //We explicitly control the slave select here, so that we can
    //do the entire read operation as a single transaction from
    //device's point of view. (The most our SPI peripheral can transfer
    //in a single shot is 16 bytes.)

    //Do the 4 byte instruction tranfer:
    //PP_CMD, ADDR2, ADDR1, ADDR0
    uint32_t write_cmd = (S25FLXX_CMD_PP << 24) | (offset & 0x00FFFFFF);

    wb_spi_slave_select(flash->bus);
    wb_spi_transact_man_ss(flash->bus, WRITE, &write_cmd, NULL, 32);

    //Write the page 16 bytes at a time.
    uint8_t bytes_sw[16];
    uint8_t* bytes = (uint8_t*) buf;
    for (int32_t bytes_left = num_bytes; bytes_left > 0; bytes_left -= 16) {
        const uint32_t xact_size = (bytes_left < 16) ? bytes_left : 16;
        for (size_t k = 0; k < 4; k++) {    //Fix word level significance
            ((uint32_t*)bytes_sw)[k] = ((uint32_t*)bytes)[3-k];
        }
        wb_spi_transact_man_ss(flash->bus, WRITE, bytes_sw, NULL, xact_size * 8);
        bytes += xact_size;
    }
    wb_spi_slave_deselect(flash->bus);

    return true;
}

bool spif_spsn_s25flxx_write_page_commit(const spi_flash_dev_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes)
{
    //Wait until write done
    if (!_spif_wait_ready(flash, S25FLXX_PAGE_WRITE_TIME_MS)) {
        UHD_FW_TRACE(ERROR, "spif_spsn_s25flxx_commit_write: Timeout. Page did not finish writing in time.");
        return false;
    }
    _spi_flash_set_write_enabled(flash, false);
    return true;
}

bool spif_spsn_s25flxx_device_busy(const spi_flash_dev_t* flash)
{
    return (_spif_read_status(flash) & S25FLXX_STATUS_WIP);
}

