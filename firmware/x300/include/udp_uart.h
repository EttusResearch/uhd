/*
 * Copyright 2011-2013 Ettus Research LLC
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

#ifndef INCLUDED_UDP_UART_H
#define INCLUDED_UDP_UART_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*!
 * Initialize the UDP/UART module.
 * Registers handler into the network.
 */
void udp_uart_init(const uint32_t uart_base, const uint16_t udp_port);

/*!
 * Polls the UART state machine,
 * and sends messages over UDP.
 */
void udp_uart_poll(void);

#endif /* INCLUDED_UDP_UART_H */
