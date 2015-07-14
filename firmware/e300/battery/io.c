/* USRP E310 IO helpers
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

#include <avr/io.h>

#include "io.h"
#include "utils.h"

#define _GET_PIN(pin)           ((pin) & 0xf)
#define _GET_MASK(pin)          (BIT(_GET_PIN(pin)))
#define _GET_REG(pin, reg_x)    (*reg_x[pin >> 4])

static volatile uint8_t *ddr_x[] = {&DDRA, &DDRB, &DDRC, &DDRD};
static volatile uint8_t *port_x[] = {&PORTA, &PORTB, &PORTC, &PORTD};
static volatile uint8_t *pin_x[] = {&PINA, &PINB, &PINC, &PIND};

void io_output_pin(io_pin_t pin)
{
	_GET_REG(pin, ddr_x) |= _GET_MASK(pin);
}

void io_input_pin(io_pin_t pin)
{
	_GET_REG(pin, ddr_x) &= ~_GET_MASK(pin);
}

bool io_is_output(io_pin_t pin)
{
	return bit_is_set(_GET_REG(pin, ddr_x), _GET_PIN(pin));
}

bool io_is_input(io_pin_t pin)
{
	return !io_is_output(pin);
}

void io_set_pin(io_pin_t pin)
{
	_GET_REG(pin, port_x) |= _GET_MASK(pin);
}

void io_clear_pin(io_pin_t pin)
{
	_GET_REG(pin, port_x) &= ~_GET_MASK(pin);
}

bool io_is_pin_set(io_pin_t pin)
{
	return bit_is_set(_GET_REG(pin, port_x), _GET_PIN(pin));
}

bool io_test_pin(io_pin_t pin)
{
	return bit_is_set(_GET_REG(pin, pin_x), _GET_PIN(pin));
}
