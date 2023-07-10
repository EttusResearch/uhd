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


/* Changes required to work for the Spartan-3A series:
 * The ICAP interface on the 3A is 8 bits wide, instead of 32.
 * Everything is Xilinx standard LSBit-first.
 * The operations are all different.
 * Commands are 16 bits long, presented to the ICAP interface 8 bits at a time.
*/

#include <xilinx_icape2.h>
#include <memory_map.h>

void
warmboot(uint32_t address)
{
	uint32_t *icape = (volatile uint32_t *)ICAP_BASE;
	icape[16] = address;
	icape[4] = 15;
	// FPGA is now reconfiguring itself from the new address
	// If executed on an FPGA, this routine will never return.
}
