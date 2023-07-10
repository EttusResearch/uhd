/* -*- c++ -*- */
/*
 * Copyright 2009 Free Software Foundation, Inc.
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

#ifndef INCLUDED_SPI_FLASH_PRIVATE_H
#define INCLUDED_SPI_FLASH_PRIVATE_H

#include "spi_flash.h"
#include "spi.h"
#include "memory_map.h"
#include <string.h>


/* M25P64 et al. */

#define	WREN_CMD	0x06	// write enable
#define	WRDI_CMD	0x04	// write disable
#define	RDID_CMD	0x9f	// read identification
#define	RDSR_CMD	0x05	// read status register
#define WRSR_CMD	0x01	// write status register
#define	READ_CMD	0x03
#define	FAST_READ_CMD	0x0b
#define	PP_CMD		0x02	// page program (256 bytes)
#define	SE_CMD		0xd8	// sector erase (64KB)
#define	BE_CMD		0xc7	// bulk erase (all)
#define	RES_CMD		0xab	// read electronic sig (deprecated)

/* Status register bits */

#define	SR_SRWD		0x80
#define	SR_BP2		0x10	// block protect bit 2
#define	SR_BP1		0x08	// block protect bit 1
#define	SR_BP0		0x04	// block protect bit 0
#define	SR_WEL		0x02	// Write Enable Latch
#define	SR_WIP		0x01	// Write in Progress.  Set if busy w/ program or erase cycle.


#define	FLAGS (SPIF_PUSH_FALL | SPIF_LATCH_RISE)

#define LEN(x) ((x) & SPI_CTRL_CHAR_LEN_MASK)


static inline uint32_t
min(uint32_t a, uint32_t b)
{
  return a < b ? a : b;
}

static inline uint32_t
round_down(uint32_t x, uint32_t power_of_2)
{
  return x & -power_of_2;
}

#endif /* INCLUDED_SPI_FLASH_PRIVATE_H */
