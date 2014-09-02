/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "usrp_common.h"

void
set_led_0 (unsigned char on)
{
  if (!on)			// active low
    USRP_PC |= bmPC_LED0;
  else
    USRP_PC &= ~bmPC_LED0;
}

void 
set_led_1 (unsigned char on)
{
  if (!on)			// active low
    USRP_PC |= bmPC_LED1;
  else
    USRP_PC &= ~bmPC_LED1;
}

void
toggle_led_0 (void)
{
  USRP_PC ^= bmPC_LED0;
}

void
toggle_led_1 (void)
{
  USRP_PC ^= bmPC_LED1;
}

void
set_sleep_bits (unsigned char bits, unsigned char mask)
{
  // NOP on usrp1
}

static __xdata unsigned char xbuf[1];

void
init_board (void)
{
  //init_spi ();

  //USRP_PC &= ~bmPC_nRESET;	// active low reset
  //USRP_PC |= bmPC_nRESET;
}
