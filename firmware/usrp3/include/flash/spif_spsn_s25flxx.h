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

#ifndef INCLUDED_SPIF_SPSN_S25FLXX_H
#define INCLUDED_SPIF_SPSN_S25FLXX_H

#include <flash/spi_flash.h>

const spi_flash_ops_t* spif_spsn_s25flxx_operations();

uint16_t spif_spsn_s25flxx_read_id(const spi_flash_dev_t* flash);

void spif_spsn_s25flxx_read(const spi_flash_dev_t* flash, uint32_t offset, void *buf, uint32_t num_bytes);

bool spif_spsn_s25flxx_erase_sector_dispatch(const spi_flash_dev_t* flash, uint32_t offset);

bool spif_spsn_s25flxx_erase_sector_commit(const spi_flash_dev_t* flash, uint32_t offset);

bool spif_spsn_s25flxx_write_page_dispatch(const spi_flash_dev_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes);

bool spif_spsn_s25flxx_write_page_commit(const spi_flash_dev_t* flash, uint32_t offset, const void *buf, uint32_t num_bytes);

bool spif_spsn_s25flxx_device_busy(const spi_flash_dev_t* flash);

#endif /* INCLUDED_SPIF_SPSN_S25FLXX_H */
