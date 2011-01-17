/* -*- c++ -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
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

#ifndef INCLUDED_HAL_IO_H
#define INCLUDED_HAL_IO_H

#include "memory_map.h"
#include "hal_uart.h"

void hal_io_init(void);
void hal_finish();
char *gets(char * const s);
int fputstr(hal_uart_name_t u, const char *s);
int fnputstr(hal_uart_name_t u, const char *s, int len);
int fngets(hal_uart_name_t u, char * const s, int len);
int fngets_noblock(hal_uart_name_t u, char * const s, int len);

/*
 * ------------------------------------------------------------------------
 *			   control the leds
 *
 * Low 4-bits are the general purpose leds on the board
 * The next bit is the led on the ethernet connector
 * ------------------------------------------------------------------------
 */

void hal_set_leds(int value, int mask);
void hal_set_led_src(int value, int mask);
void hal_toggle_leds(int mask);

/*
 * ------------------------------------------------------------------------
 *			   simple timeouts
 * ------------------------------------------------------------------------
 */



static inline void
hal_set_timeout(int delta_ticks)
{
  sr_simple_timer->onetime = delta_ticks;
}

/*
 * ------------------------------------------------------------------------
 *			interrupt enable/disable
 * ------------------------------------------------------------------------
 */

/*!
 * \brief Disable interrupts and return previous interrupt enable state.
 * [Microblaze specific]
 */
static inline int
hal_disable_ints(void)
{
  return 0; /* NOP */
}

/*!
 * \brief Enable interrupts and return previous interrupt enable state.
 * [Microblaze specific]
 */
static inline int
hal_enable_ints(void)
{
  return 0; /* NOP */
}

/*!
 * \brief Set interrupt enable state to \p prev_state.
 * [Microblaze specific]
 */
static inline void
hal_restore_ints(int prev_state)
{
    /* NOP */
}

#endif /* INCLUDED_HAL_IO_H */
