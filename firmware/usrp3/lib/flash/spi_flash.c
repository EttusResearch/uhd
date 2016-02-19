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

#include <flash/spi_flash.h>

void spif_init(spi_flash_session_t* flash, const spi_flash_dev_t* device, const spi_flash_ops_t* ops)
{
    flash->device = device;
    flash->ops = ops;
    flash->state = IDLE;
    flash->last_offset = 0;
    flash->id = ops->read_id(device);
}

void spif_read_sync(const spi_flash_session_t* flash, uint32_t offset, void *buf, uint32_t num_bytes)
{
    flash->ops->read(flash->device, offset, buf, num_bytes);
}

bool spif_erase_sector_sync(const spi_flash_session_t* flash, uint32_t offset)
{
    if (flash->ops->erase_sector_dispatch(flash->device, offset)) {
        return flash->ops->erase_sector_commit(flash->device, offset);
    } else {
        return false;
    }
}

bool spif_write_page_sync(const spi_flash_session_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes)
{
    if (flash->ops->write_page_dispatch(flash->device, offset, buf, num_bytes)) {
        return flash->ops->write_page_commit(flash->device, offset, buf, num_bytes);
    } else {
        return false;
    }
}
