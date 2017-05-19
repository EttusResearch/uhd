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
#include "usrp_commands.h"

/*
 * the host side fpga loader code pushes an MD5 hash of the bitstream
 * into hash1.
 */
#define	  USRP_HASH_SIZE      16
__xdata __at USRP_HASH_SLOT_0_ADDR unsigned char hash0[USRP_HASH_SIZE];


#define REG_RX_PWR_DN		 1
#define	REG_TX_PWR_DN		 8
#define	REG_TX_MODULATOR	20

void eeprom_init (void)
{
  unsigned short counter;
  unsigned char	 i;

  // configure IO ports (B and D are used by GPIF)

  IOA = bmPORT_A_INITIAL;	// Port A initial state
  OEA = bmPORT_A_OUTPUTS;	// Port A direction register

  IOC = bmPORT_C_INITIAL;	// Port C initial state
  OEC = bmPORT_C_OUTPUTS;	// Port C direction register

  IOE = bmPORT_E_INITIAL;	// Port E initial state
  OEE = bmPORT_E_OUTPUTS;	// Port E direction register

  EP0BCH = 0;			SYNCDELAY;

  // USBCS &= ~bmRENUM;		// chip firmware handles commands
  USBCS = 0;			// chip firmware handles commands

  //USRP_PC &= ~bmPC_nRESET;	// active low reset
  //USRP_PC |=  bmPC_nRESET;

  // zero firmware hash slot
  i = 0;
  do {
    hash0[i] = 0;
    i++;
  } while (i != USRP_HASH_SIZE);

  counter = 0;
  while (1){
    counter++;
    if (counter & 0x8000)
      IOC ^= bmPC_LED0;
  }
}
