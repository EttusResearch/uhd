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

void hal_io_init(void);
void hal_finish();

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
  int result, t0;

  asm volatile("mfs   %0, rmsr       \n\
		andni %1, %0, 0x2    \n\
		mts   rmsr, %1"
	       : "=r" (result), "=r" (t0));
  return result;
}

/*!
 * \brief Enable interrupts and return previous interrupt enable state.
 * [Microblaze specific]
 */
static inline int
hal_enable_ints(void)
{
  int result, t0;

  asm volatile("mfs  %0, rmsr	      \n\
		ori  %1, %0, 0x2      \n\
		mts  rmsr, %1"
	       : "=r" (result), "=r" (t0));
  return result;
}

/*!
 * \brief Set interrupt enable state to \p prev_state.
 * [Microblaze specific]
 */
static inline void
hal_restore_ints(int prev_state)
{
  int t0, t1;
  asm volatile("andi  %0, %2, 0x2	\n\
		mfs   %1, rmsr          \n\
		andni %1, %1, 0x2	\n\
		or    %1, %1, %0	\n\
		mts   rmsr, %1"
	       : "=r" (t0), "=r"(t1) : "r" (prev_state));
}

#endif /* INCLUDED_HAL_IO_H */
