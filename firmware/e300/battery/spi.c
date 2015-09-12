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

#include "mcu_settings.h"
#include "io.h"
#include "spi.h"
#include "utils.h"

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

static io_pin_t AVR_CS = IO_PB(2);
static io_pin_t AVR_MOSI = IO_PB(3);
static io_pin_t AVR_MISO = IO_PB(4);
static io_pin_t AVR_SCK = IO_PB(5);

void spi_init(spi_type_t type, spi_order_t order, spi_mode_t mode, spi_speed_t speed)
{
	uint8_t val;

	io_output_pin(AVR_CS);
	io_output_pin(AVR_MOSI);
	io_input_pin(AVR_MISO);
	io_output_pin(AVR_SCK);

	/* slave select is low active */
	io_set_pin(AVR_CS);

	/* SPCR looks like this: [SPIE | SPE | DORD | MSTR | CPOL | CPHA | SPR1 | SPR0] */
	val = BIT(SPE);
	val |= (order == SPI_LSB_FIRST) ? BIT(DORD) : 0;
	val |= (type == SPI_TYPE_MASTER) ? BIT(MSTR) : 0;
	val |= mode << CPHA;
	val |= speed << SPR0;

	SPCR = val;
}

uint8_t spi_transact(uint8_t data)
{
	uint8_t ret;

	io_clear_pin(AVR_CS);
	SPDR = data;
	ret = SPDR;
	io_set_pin(AVR_CS);
	return ret;
}

void spi_transact_buf(uint8_t *in, uint8_t *out, uint8_t size)
{
	uint8_t i;

	io_clear_pin(AVR_CS);

	for (i = 0; i < size; i++) {
		SPDR = in[i];
		spi_wait_till_done();
		out[i] = SPDR;
	}

	io_set_pin(AVR_CS);
}

void spi_wait_till_done(void)
{
	uint8_t timeout = 100;

	do {
		_delay_us(10);
		timeout--;
	} while (timeout && !(SPSR & BIT(SPIF)));
}
