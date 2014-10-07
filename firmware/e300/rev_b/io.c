/*
 * Copyright 2009-2012 Ettus Research LLC
 */

#include "io.h"
#include <avr/io.h>

#define _GET_PIN(pin)           ((pin) & 0xf)
#define _GET_MASK(pin)          (_BV(_GET_PIN(pin)))
#define _GET_REG(pin, reg_x)    (*reg_x[pin >> 4])

#ifndef IO_DEBUG
static volatile uint8_t *ddr_x[] = {&DDRA, &DDRB, &DDRC, &DDRD};		// 0: input, 1: output
static volatile uint8_t *port_x[] = {&PORTA, &PORTB, &PORTC, &PORTD};	// Port contents (will appear at output if direction is set to output. If input, '1' enables pull-ups, '0' set tri-state)
static volatile uint8_t *pin_x[] = {&PINA, &PINB, &PINC, &PIND};		// Port contents (input) If output, will return value on PORT
#endif

void io_output_pin(io_pin_t pin){
#ifndef IO_DEBUG
	_GET_REG(pin, ddr_x) |= _GET_MASK(pin);
#endif
}

void io_input_pin(io_pin_t pin){
#ifndef IO_DEBUG
	_GET_REG(pin, ddr_x) &= ~_GET_MASK(pin);
#endif
}

bool io_is_output(io_pin_t pin){
#ifndef IO_DEBUG
	return bit_is_set(_GET_REG(pin, ddr_x), _GET_PIN(pin));
#else
	return 0;
#endif
}

bool io_is_input(io_pin_t pin){
	return !io_is_output(pin);
}

void io_set_pin(io_pin_t pin){	// In input mode, will enable pull-ups
#ifndef IO_DEBUG
	_GET_REG(pin, port_x) |= _GET_MASK(pin);
#endif
}

void io_clear_pin(io_pin_t pin){	// In input mode, will disable pull-ups
#ifndef IO_DEBUG
	_GET_REG(pin, port_x) &= ~_GET_MASK(pin);
#endif
}

bool io_is_pin_set(io_pin_t pin){
#ifndef IO_DEBUG
	return bit_is_set(_GET_REG(pin, port_x), _GET_PIN(pin));
#else
	return 0;
#endif
}

void io_enable_pin(io_pin_t pin, bool enable){
    if (enable)
        io_set_pin(pin);
    else
        io_clear_pin(pin);
}

bool io_test_pin(io_pin_t pin){
#ifndef IO_DEBUG
	return bit_is_set(_GET_REG(pin, pin_x), _GET_PIN(pin));
#else
	return 0;
#endif
}
