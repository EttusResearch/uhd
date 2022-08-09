/* -*- c -*- */
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#ifndef INCLUDED_HAL_UART_H
#define INCLUDED_HAL_UART_H

/*!
 * \brief uart mode flags
 */
#define	UART_MODE_RAW		0x0000	// no mapping on input or output
#define	UART_MODE_ONLCR	0x0001	// map \n to \r\n on output (default)

#define DEFAULT_UART UART_DEBUG //which UART printf, gets, etc. use

typedef enum {
  US_9600   = 0,
  US_19200  = 1,
  US_38400  = 2,
  US_57600  = 3,
  US_115200 = 4,
  US_230400 = 5
} hal_uart_speed_t;

typedef struct {
  hal_uart_speed_t	speed;
} hal_uart_config_t;

typedef enum {
  UART_DEBUG = 0,
  UART_EXP   = 1,
  UART_GPS   = 2
} hal_uart_name_t;

/*
 * \brief Set uart mode
 */
void hal_uart_set_mode(hal_uart_name_t uart, int flags);

/*!
 * \brief one-time call to init
 */
void hal_uart_init(void);

/*!
 * \brief Set uart parameters
 *  Default is 115,200 bps, 8N1.
 */
void hal_uart_set_config(const hal_uart_config_t *c);

/*!
 * \brief Get uart configuation.
 */
void hal_uart_get_config(hal_uart_config_t *c);

/*!
 * \brief Enqueue \p ch for output over serial port
 */
void hal_uart_putc(hal_uart_name_t u, int ch);

/*!
 * \brief Enqueue \p ch for output over serial port, silent fail if queue is full
 */
void hal_uart_putc_nowait(hal_uart_name_t u, int ch);

/*
 * \brief Blocking read of next char from serial port
 */
int hal_uart_getc(hal_uart_name_t u);

/*
 * \brief Non-blocking read of next char from serial port, return -1 if nothing available
 */
int hal_uart_getc_noblock(hal_uart_name_t u);

int hal_uart_rx_flush(hal_uart_name_t u);

#endif /* INCLUDED_HAL_UART_H */
