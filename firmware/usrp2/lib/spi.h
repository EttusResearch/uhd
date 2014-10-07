/*
 * Copyright 2012 Ettus Research LLC
 * Copyright 2006,2007 Free Software Foundation, Inc.
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

#ifndef INCLUDED_SPI_H
#define INCLUDED_SPI_H

#include <stdbool.h>
#include <stdint.h>

/*!
 * \brief One time call to initialize SPI
 */
void spi_init(void);

/*! 
 * \brief Wait for last SPI transaction to complete.
 * Unless you need to know it completed, it's not necessary to call this.
 */
void spi_wait(void);

#define SPI_TXONLY false	// readback: no
#define SPI_TXRX   true		// readback: yes

/*
 * Flags for spi_transact
 */
#define SPI_PUSH_RISE   (1 << 0)		// push tx data on rising edge of SCLK
#define SPI_PUSH_FALL   (1 << 1)	// push tx data on falling edge of SCLK
#define SPI_LATCH_RISE  (1 << 2)		// latch rx data on rising edge of SCLK
#define SPI_LATCH_FALL  (1 << 3)	// latch rx data on falling edge of SCLK

uint32_t spi_transact(bool readback, int slave, uint32_t data, int length, uint32_t flags);

#endif /* INCLUDED_SPI_H */
