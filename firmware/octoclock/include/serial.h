/*
 * Copyright 2014 Ettus Research LLC
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

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdint.h>
#include <stdbool.h>

#include <octoclock.h>

#define BAUD_DELAY(baud) ((1.0 / (double)baud) * ((double)F_CPU / 8.0))
#define BAUD_115200_DELAY BAUD_DELAY(115200)

void serial_init(volatile uint8_t* port, uint8_t index);
void serial_tx_P(const char* message, volatile uint8_t* port, uint8_t index, bool newline);
void serial_tx(const char* message, volatile uint8_t* port, uint8_t index, bool newline);
void serial_tx_byte(uint8_t byte, volatile uint8_t* port, uint8_t index, bool newline);
void serial_tx_hex(uint8_t byte, volatile uint8_t* port, uint8_t index, bool newline);
char serial_rx_char(volatile uint8_t* port, uint8_t index);
char serial_rx_char_nowait(volatile uint8_t* port, uint8_t index);

#endif /* _SERIAL_H_ */
