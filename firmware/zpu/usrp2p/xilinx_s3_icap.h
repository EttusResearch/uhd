/* -*- c -*- */
/*
 * Copyright 2009-2011 Ettus Research LLC
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

#ifndef INCLUDED_XILINX_S3_ICAP_H
#define INCLUDED_XILINX_S3_ICAP_H

#include <stdint.h>


void wr_icap(uint8_t x);
uint8_t rd_icap(void);

//int icap_read_config_reg(int regno);

/*
 * Attempt to reload the fpga from \p flash_address.
 * Shouldn't return, but might.
 */
void icap_reload_fpga(uint32_t flash_address);


#endif /* INCLUDED_XILINX_S3_ICAP_H */
