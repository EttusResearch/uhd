/* USRP E310 Firmware Atmel AVR SPI driver
 * Copyright (C) 2014 Ettus Research
 * This file is part of the USRP E310 Firmware
 * The USRP E310 Firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * The USRP E310 Firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with the USRP E310 Firmware. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file spi.h
 * \brief Atmel AVR SPI driver
 */

#ifndef SPI_H
#define SPI_H

#include <stdlib.h>

typedef enum spi_type {
	SPI_TYPE_MASTER = 0x00,
	SPI_TYPE_SLAVE = 0x01
} spi_type_t;

typedef enum spi_mode {
	SPI_MODE_0	= 0x00, /** CPOL=0, CPHA=0 */
	SPI_MODE_1	= 0x01, /** CPOL=0, CPHA=1 */
	SPI_MODE_2	= 0x02, /** CPOL=1, CPHA=0 */
	SPI_MODE_3	= 0x03, /** CPOL=1, CPHA=1 */
} spi_mode_t;

/* note that this assumes a clock speed of 8MHz */
typedef enum spi_speed {
	SPI_SPEED_2M	= 0x00,
	SPI_SPEED_500K	= 0x01,
	SPI_SPEED_125K	= 0x02,
	SPI_SPEED_62_5K	= 0x03,
} spi_speed_t;

typedef enum spi_order {
	SPI_MSB_FIRST	=	0x00,
	SPI_LSB_FIRST	=	0x01,
} spi_order_t;

/**
 * \brief Initialize the AVR's SPI module
 * \param[in] master Operate as master
 * \param[in] order Whether to send the LSB first
 * \param[in] mode Which clock / phase configuration to use
 * \param[in] speed Which speed to use
 */
void spi_init(spi_type_t type, spi_order_t order, spi_mode_t mode, spi_speed_t speed);

/**
 * \brief Transact one byte to the slave and receive one byte
 * \param[in] mode Whether we're using the SPI in MSB or LSB first mode
 * \param[in] speed Which speed to use, note that these speeds are based on a prescaler,
		and the values are correct for a FCLK of 8 MHz
*/
uint8_t spi_transact(uint8_t data);

void spi_transact_buf(uint8_t *in, uint8_t *out, uint8_t size);
/**
 * \brief Wait till transaction is done
 */
void spi_wait_till_done(void);

#endif /* SPI_H */
