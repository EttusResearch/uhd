/* -*- c -*- */
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
#ifndef INCLUDED_SPI_FLASH_H
#define INCLUDED_SPI_FLASH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SPI_FLASH_PAGE_SIZE	256
#define SPI_SS_FLASH 1

#define SPIF_PUSH_RISE   0             // push tx data on rising edge of SCLK
#define SPIF_PUSH_FALL   SPI_CTRL_TXNEG        // push tx data on falling edge of SCLK
#define SPIF_LATCH_RISE  0             // latch rx data on rising edge of SCLK
#define SPIF_LATCH_FALL  SPI_CTRL_RXNEG        // latch rx data on falling edge of SCLK

void spif_init(void);
void spif_wait(void);

uint32_t spif_transact(bool readback, int slave, uint32_t data, int length, uint32_t flags);

uint32_t spi_flash_rdid(void);	/* Read ID */
uint32_t spi_flash_rdsr(void);	/* Read Status Register */

size_t spi_flash_log2_memory_size(void);
size_t spi_flash_log2_sector_size(void);
size_t spi_flash_sector_size(void);
size_t spi_flash_memory_size(void);

void spi_flash_read(uint32_t flash_addr,  size_t nbytes, void *buf);

/*
 * Erase all sectors that fall within the interval [flash_addr, flash_addr + nbytes).
 * Erasing sets the memory to ones.
 */
void spi_flash_erase(uint32_t flash_addr, size_t nbytes);

/*
 * Program the flash.
 * The area must have been erased prior to programming.
 */
bool spi_flash_program(uint32_t flash_addr, size_t nbytes, const void *buf);

/*
 * --- asynchronous routines ---
 */

/*
 * Is the erasing or programming done?
 */
bool spi_flash_done_p(void);

/*
 * Wait for erasing or programming to complete
 */
void spi_flash_wait(void);

/*
 * Start the erase process on a single sector.
 * (It takes between 1 and 3 seconds to erase a 64KB sector)
 */
void spi_flash_erase_sector_start(uint32_t flash_addr);

/*
 * Start the programming process within a single page.
 * nbytes must be between 1 and 256.
 * (It takes between 1.4 and 5 ms to program a page -> 640 ms for 64KB)
 */
bool spi_flash_page_program_start(uint32_t flash_addr, size_t nbytes, const void *buf);


/*
 * --- high-level async erase ---
 */

typedef struct {
  uint32_t	first;
  uint32_t	last;
  uint32_t	current;
} spi_flash_async_state_t;

/*
 * Start to erase all sectors that fall within the interval [flash_addr, flash_addr + nbytes).
 * Erasing sets the memory to ones.
 *
 * Initializes s and begins the process.  Call spi_flash_async_erase_poll
 * to test for completion and advance state machine.
 */
void spi_flash_async_erase_start(spi_flash_async_state_t *s,
				 uint32_t flash_addr, size_t nbytes);

/*
 * Poll for aysnc flash erase completion.
 * Returns true when the erase has completed.
 * (This should be called at something >= 4 Hz.  It takes 1 to 3 seconds to
 * erase each 64KB sector).
 */
bool spi_flash_async_erase_poll(spi_flash_async_state_t *s);


#endif /* INCLUDED_SPI_FLASH_H */
