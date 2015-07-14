/* USRP E310 Firmware Atmel AVR TWI driver
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

#include "i2c_twi.h"
#include "mcu_settings.h"
#include "utils.h"

#include <stdbool.h>
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>

static const uint8_t I2C_TIMEOUT = 10;

static inline uint8_t I2C_READ_ADDR(const uint8_t x)
{
	return (x << 1) | 0x1;
}

static inline uint8_t I2C_WRITE_ADDR(const uint8_t x)
{
	return (x << 1) & 0xfe;
}

void i2c_twi_init_calc(uint32_t rate)
{
	uint8_t twbr;
	twbr = ((F_CPU/rate)-16)/2;

	TWBR = twbr;

	PRR &= ~BIT(PRTWI);

	/* www.mikrocontroller.net/articles/AVR_TWI says this might help ... */
	TWCR &= ~(BIT(TWSTO) | BIT(TWEN));
	TWCR |=	BIT(TWEN);
}

void i2c_twi_init(i2c_speed_t speed)
{
	switch (speed) {
	case I2C_SPEED_400K:
		TWBR = 16;
		break;
	case I2C_SPEED_100K:
		TWBR = 32;
		break;
	default:
		TWBR = 32;
		break;
	}

	/* reset potential prescalers */
	TWSR = 0;

	/* www.mikrocontroller.net/articles/AVR_TWI says this might help ... */
	TWCR &= ~(BIT(TWSTO) | BIT(TWEN));
	TWCR |=	BIT(TWEN);
}

static void i2c_twi_wait_for_complete(void)
{
	uint8_t timeout = 100;

	do {
		_delay_us(10);
		timeout--;
	} while(timeout && !(TWCR & (1<<TWINT)));
}

static void i2c_twi_start(void)
{
	TWCR = BIT(TWINT) | BIT(TWEN) | BIT(TWSTA);
	i2c_twi_wait_for_complete();
}

static void i2c_twi_stop(void)
{
	TWCR = BIT(TWINT) | BIT(TWEN) | BIT(TWSTO);
}


static uint8_t i2c_twi_recv_byte(bool ack)
{
	TWCR = BIT(TWINT) | BIT(TWEN) | (ack ? BIT(TWEA) : 0);
	i2c_twi_wait_for_complete();
	return TWDR;
}

static void i2c_twi_send_byte(uint8_t data)
{
	TWDR = data;
	TWCR = BIT(TWINT) | BIT(TWEN);
	i2c_twi_wait_for_complete();
}

int8_t i2c_twi_read(uint8_t addr, uint8_t reg, uint8_t *value)
{
	/* start the write transaction to select the register */
	i2c_twi_start();
	i2c_twi_send_byte(I2C_WRITE_ADDR(addr));
	i2c_twi_send_byte(reg);

	/* (re)start for the actual read transaction to read back */
	i2c_twi_start();
	i2c_twi_send_byte(I2C_READ_ADDR(addr));
	*value = i2c_twi_recv_byte(false);
	i2c_twi_stop();

	return 0;
}

int8_t i2c_twi_write(uint8_t addr, uint8_t reg, uint8_t value)
{
	i2c_twi_start();
	i2c_twi_send_byte(I2C_WRITE_ADDR(addr));
	i2c_twi_send_byte(reg);
	i2c_twi_send_byte(value);
	i2c_twi_stop();

	return 0;
}

int8_t i2c_twi_read16(uint8_t addr, uint8_t reg, uint16_t *value)
{
	uint8_t msb, lsb;

	/* start the write transaction to select the register */
	i2c_twi_start();
	i2c_twi_send_byte(I2C_WRITE_ADDR(addr));
	i2c_twi_send_byte(reg);

	/* (re)start for the actual read transaction to read back MSB
	 * then LSB, fortunately the datashit describes the opposite w.r.t ACKs*/
	i2c_twi_start();
	i2c_twi_send_byte(I2C_READ_ADDR(addr));
	msb = i2c_twi_recv_byte(true);
	lsb = i2c_twi_recv_byte(false);
	i2c_twi_stop();

	*value = (msb << 8) | lsb;

	return 0;
}

int8_t i2c_twi_write16(uint8_t addr, uint8_t reg, uint16_t value)
{
	uint8_t msb, lsb;

	msb = value >> 8;
	lsb = value & 0xff;

	i2c_twi_start();
	i2c_twi_send_byte(I2C_WRITE_ADDR(addr));
	i2c_twi_send_byte(reg);
	i2c_twi_send_byte(msb);
	i2c_twi_send_byte(lsb);
        i2c_twi_stop();

	return 0;
}

/*
static const uint8_t I2C_ARA_ADDR = 0x0c;
uint8_t i2c_twi_smbus_ara(void)
{
	volatile uint8_t addr;;

	i2c_twi_start();
	i2c_twi_send_byte(I2C_READ_ADDR(I2C_ARA_ADDR));
	addr = i2c_twi_recv_byte(false);
	i2c_twi_stop();
	return addr;
}
*/
