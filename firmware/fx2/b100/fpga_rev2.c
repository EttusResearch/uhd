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

#include "fpga.h"
#include "fpga_regs_common.h"
#include "usrp_common.h"
#include "usrp_globals.h"

unsigned char g_tx_reset = 0;
unsigned char g_rx_reset = 0;

void
fpga_write_reg (unsigned char regno, const __xdata unsigned char *regval)
{
	//nop
}


static __xdata unsigned char regval[4] = {0, 0, 0, 0};

// Resets both AD9862's and the FPGA serial bus interface.

void
fpga_set_reset (unsigned char on)
{
  on &= 0x1;

  if (on){
  }
  else
    ;
}
