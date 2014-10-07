/*
 * Copyright 2009 Ettus Research LLC
 */

#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>

#define IO_PX(port, pin) ((uint8_t)(((port - 'A') << 4) + pin))
#define IO_PA(pin) IO_PX('A', pin)
#define IO_PB(pin) IO_PX('B', pin)
#define IO_PC(pin) IO_PX('C', pin)
#define IO_PD(pin) IO_PX('D', pin)

typedef const uint8_t io_pin_t;

void io_output_pin(io_pin_t pin);
void io_input_pin(io_pin_t pin);
bool io_is_output(io_pin_t pin);
bool io_is_input(io_pin_t pin);

void io_set_pin(io_pin_t pin);
void io_clear_pin(io_pin_t pin);
void io_enable_pin(io_pin_t pin, bool enable);
bool io_is_pin_set(io_pin_t pin);

bool io_test_pin(io_pin_t pin);

#endif /* IO_H */
