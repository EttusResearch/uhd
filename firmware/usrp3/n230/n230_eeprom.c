//
// Copyright 2014 Ettus Research LLC
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

#include "../../../host/lib/usrp/n230/n230_eeprom.h"

#include <trace.h>
#include <stddef.h>
#include <flash/spi_flash.h>
#include <flash/spif_spsn_s25flxx.h>
#include <string.h> //memcpy

#include "../../../host/lib/usrp/n230/n230_fw_defs.h"
#include "../../../host/lib/usrp/n230/n230_fw_host_iface.h"

static const wb_spi_slave_t flash_spi_slave = {
    .base      = (void*) 0xB000,
    .slave_sel = 0x0001,
    .clk_div   = 4, //80MHz/4 = 20MHz
    .mosi_edge = RISING,
    .miso_edge = FALLING,
    .lsb_first = false
};

static const spi_flash_dev_t spi_flash_device = {
    .page_size   = 256,
    .sector_size = 65536,
    .num_sectors = 254,
    .bus         = &flash_spi_slave
};

/***********************************************************************
 * Non-volatile device data
 **********************************************************************/
#define N230_FLASH_NV_DATA_OFFSET     0x800000

//Default values in case the EEPROM is not read, corrupt
const n230_eeprom_map_t default_eeprom = {
    .data_version_major = N230_EEPROM_VER_MAJOR,
    .data_version_minor = N230_EEPROM_VER_MINOR,
    .hw_revision = 0,
    .hw_product = 0x01,
    .gateway = N230_DEFAULT_GATEWAY,
    .eth_info = {
        {   //eth0
            .mac_addr = N230_DEFAULT_ETH0_MAC,
            .subnet   = N230_DEFAULT_ETH0_MASK,
            .ip_addr  = N230_DEFAULT_ETH0_IP
        },
        {   //eth1
            .mac_addr = N230_DEFAULT_ETH1_MAC,
            .subnet   = N230_DEFAULT_ETH1_MASK,
            .ip_addr  = N230_DEFAULT_ETH1_IP
        }
    }
};

//EEPROM cache
static spi_flash_session_t flash_session = {.device = NULL};
static n230_eeprom_map_t eeprom_cache;
static bool cache_dirty = true;

bool read_n230_eeprom()
{
    bool status = false;
    if (flash_session.device == NULL) { //Initialize flash session structure for the first time
        wb_spi_init(spi_flash_device.bus);
        spif_init(&flash_session, &spi_flash_device, spif_spsn_s25flxx_operations());
    }
    spif_read_sync(&flash_session, N230_FLASH_NV_DATA_OFFSET, &eeprom_cache, sizeof(n230_eeprom_map_t));

    //Verify data format
    status = (eeprom_cache.data_version_major == default_eeprom.data_version_major);
    //Sanity communication info
    if (eeprom_cache.eth_info[0].ip_addr == 0xFFFFFFFF)
        eeprom_cache.eth_info[0].ip_addr = default_eeprom.eth_info[0].ip_addr;
    if (eeprom_cache.eth_info[1].ip_addr == 0xFFFFFFFF)
        eeprom_cache.eth_info[1].ip_addr = default_eeprom.eth_info[1].ip_addr;
    if (eeprom_cache.eth_info[0].subnet == 0xFFFFFFFF)
        eeprom_cache.eth_info[0].subnet = default_eeprom.eth_info[0].subnet;
    if (eeprom_cache.eth_info[1].subnet == 0xFFFFFFFF)
        eeprom_cache.eth_info[1].subnet = default_eeprom.eth_info[1].subnet;

    if (!status)  {
        UHD_FW_TRACE(WARN, "read_n230_eeprom: Initialized cache to the default map.");
        memcpy(&eeprom_cache, &default_eeprom, sizeof(n230_eeprom_map_t));
    }
    cache_dirty = !status;
    return status;
}

bool write_n230_eeprom()
{
    //Assumption: sizeof(n230_eeprom_map_t) <= flash_page_size
    //This function would need to be reimplemented if this assumption is no longer true
    if (sizeof(n230_eeprom_map_t) > flash_session.device->page_size) {
        UHD_FW_TRACE(ERROR, "write_n230_eeprom: sizeof(n230_eeprom_map_t) > flash_page_size");
        return false;
    }

    bool status = true;
    if (cache_dirty) {
        n230_eeprom_map_t device_eeprom;
        spif_read_sync(&flash_session, N230_FLASH_NV_DATA_OFFSET, &device_eeprom, sizeof(n230_eeprom_map_t));
        if (memcmp(&eeprom_cache, &device_eeprom, sizeof(n230_eeprom_map_t)) != 0) {
            //Cache does not match read state. Write.
            UHD_FW_TRACE(DEBUG, "write_n230_eeprom: Writing data to flash...");
            status = spif_erase_sector_sync(&flash_session, N230_FLASH_NV_DATA_OFFSET);
            if (status) {
                status = spif_write_page_sync(
                    &flash_session, N230_FLASH_NV_DATA_OFFSET, &eeprom_cache, sizeof(n230_eeprom_map_t));
            }
            if (!status) {
                UHD_FW_TRACE(ERROR, "write_n230_eeprom: Operation failed!");
            }
            cache_dirty = !status;
        } else {
            UHD_FW_TRACE(DEBUG, "write_n230_eeprom: No new data. Write skipped.");
            //Cache matches read state. So mark as clean
            cache_dirty = false;
        }
    }
    return status;
}

bool is_n230_eeprom_cache_dirty()
{
    return cache_dirty;
}

n230_eeprom_map_t* get_n230_eeprom_map()
{
    cache_dirty = true;
    return &eeprom_cache;
}

const n230_eeprom_map_t* get_n230_const_eeprom_map()
{
    return &eeprom_cache;
}

const n230_eth_eeprom_map_t* get_n230_ethernet_info(uint32_t iface) {
    if (iface >= N230_NUM_ETH_PORTS) {
        UHD_FW_TRACE_FSTR(ERROR,
            "get_n230_ethernet_info called with iface=%d when there are only %d ports!!!",
            iface, N230_NUM_ETH_PORTS);
    }
    return &(get_n230_const_eeprom_map()->eth_info[iface]);
}


/***********************************************************************
 * Storage for bootstrap FPGA Image
 **********************************************************************/
#define N230_FLASH_FPGA_IMAGE_OFFSET  0x000000
#define N230_FLASH_FPGA_IMAGE_SIZE    0x400000
#define N230_FLASH_NUM_FPGA_IMAGES    2

void read_n230_fpga_image_page(uint32_t offset, void *buf, uint32_t num_bytes)
{
    if (offset >= (N230_FLASH_NUM_FPGA_IMAGES * N230_FLASH_FPGA_IMAGE_SIZE)) {
        UHD_FW_TRACE_FSTR(ERROR, "read_n230_fpga_image_page: Offset 0x%x out of bounds", offset);
    }
    spif_read_sync(&flash_session, N230_FLASH_FPGA_IMAGE_OFFSET + offset, buf, num_bytes);
}

bool write_n230_fpga_image_page(uint32_t offset, const void *buf, uint32_t num_bytes)
{
    if (offset >= (N230_FLASH_NUM_FPGA_IMAGES * N230_FLASH_FPGA_IMAGE_SIZE)) {
        UHD_FW_TRACE_FSTR(ERROR, "write_n230_fpga_image_page: Offset 0x%x out of bounds", offset);
        return false;
    }
    return spif_write_page_sync(&flash_session, N230_FLASH_FPGA_IMAGE_OFFSET + offset, buf, num_bytes);
}

bool erase_n230_fpga_image_sector(uint32_t offset)
{
    if (offset >= (N230_FLASH_NUM_FPGA_IMAGES * N230_FLASH_FPGA_IMAGE_SIZE)) {
        UHD_FW_TRACE_FSTR(ERROR, "erase_n230_fpga_image_sector: Offset 0x%x out of bounds", offset);
        return false;
    }
    return spif_erase_sector_sync(&flash_session, N230_FLASH_FPGA_IMAGE_OFFSET + offset);
}
