/*
 * USRP - Universal Software Radio Peripheral
 *
 * Copyright (C) 2003 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301  USA
 */

/*
 * These are the register definitions for the Rev 1 USRP prototype
 * The Rev 1 is the version with the AD9862's and daughterboards
 */

#ifndef _B100_REGS_H_
#define _B100_REGS_H_

#include "fx2regs.h"

/*
 * Port A (bit addressable):
 */

#define USRP_PA			IOA		// Port A
#define	USRP_PA_OE		OEA		// Port A direction register

#define	USRP_ALTERA_CONFIG	USRP_PA   // Now on port A, not C

#define bmALTERA_DCLK		bmBIT0
#define bmALTERA_NCONFIG	bmBIT1
#define bmALTERA_DATA0		bmBIT3
#define bmALTERA_NSTATUS	bmBIT4
#define bmRESET_FPGA_FIFOS  bmBIT7


#define	bmALTERA_BITS		(bmALTERA_DCLK			\
				 | bmALTERA_NCONFIG		\
				 | bmALTERA_DATA0		\
				 | bmALTERA_NSTATUS		\
				)


#define	bmPORT_A_OUTPUTS	(bmALTERA_DCLK			\
			 	 | bmALTERA_NCONFIG		\
				 | bmALTERA_DATA0		\
				)

#define	bmPORT_A_INITIAL	0

#define PORT_A_ADDR 0x80
#define PORT_C_ADDR 0xA0

sbit at PORT_A_ADDR+0 bitALTERA_DCLK;	// 0x80 is the bit address of PORT A
sbit at PORT_A_ADDR+1 bitALTERA_NCONFIG;
sbit at PORT_A_ADDR+3 bitALTERA_DATA0;
sbit at PORT_A_ADDR+6 bitSHORT_PACKET_SIGNAL;

sbit at PORT_C_ADDR+7 bitALTERA_CONF_DONE;


/* Port B: GPIF	FD[7:0]			*/

/*
 * Port C (bit addressable):
 *    5:1 FPGA configuration
 */

#define	USRP_PC			IOC		// Port C
#define	USRP_PC_OE		OEC		// Port C direction register

#define	bmPC_nRESET		0 //bmBIT0		// reset line to codecs (active low)
#define	bmPC_LED0		bmBIT0		// active low
#define	bmPC_LED1		bmBIT1		// active low

#define	bmPORT_C_OUTPUTS	(bmPC_LED0 | bmPC_LED1)
#define	bmPORT_C_INITIAL	(bmPC_LED0 | bmPC_LED1)


#define	USRP_LED_REG		USRP_PC
#define	bmLED0			bmPC_LED0
#define	bmLED1			bmPC_LED1


/* Port D: GPIF	FD[15:8]		*/

/* Port E: not bit addressible		*/

#define	USRP_PE			IOE		// Port E
#define	USRP_PE_OE		OEE		// Port E direction register

#define	bmPORT_E_OUTPUTS	(0)
#define	bmPORT_E_INITIAL	(0)

/*
 * FPGA output lines that are tied to FX2 RDYx inputs.
 * These are readable using GPIFREADYSTAT.
 */
//#define	bmFPGA_HAS_SPACE		bmBIT0	// usbrdy[0] has room for 512 byte packet
//#define	bmFPGA_PKT_AVAIL		bmBIT1	// usbrdy[1] has >= 512 bytes available

#define bmDATA_EMPTY            bmBIT0 //data output FIFO has no data ready
#define bmDATA_FIFO_FULL        bmBIT1 //data input FIFO is full
#define bmCTRL_EMPTY            bmBIT2 //control output FIFO has no data ready
#define bmCTRL_FIFO_FULL        bmBIT3 //control input FIFO is full

// #define	bmTX_UNDERRUN			bmBIT2  // usbrdy[2] D/A ran out of data
// #define	bmRX_OVERRUN			bmBIT3	// usbrdy[3] A/D ran out of buffer

/*
 * FPGA input lines that are tied to the FX2 CTLx outputs.
 *
 * These are controlled by the GPIF microprogram...
 */
// WE					bmBIT0	// usbctl[0] write enable
// RE					bmBIT1	// usbctl[1] read enable
// OE					bmBIT2	// usbctl[2] output enable
// EP                   bmBIT3  // usbctl[3] endpoint select (data/ctrl)

#endif /* _USRP_REV1_REGS_H_ */
