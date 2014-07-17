/*! \file enc28j60conf.h \brief Microchip ENC28J60 Ethernet Interface Driver Configuration. */
//*****************************************************************************
//
// File Name	: 'enc28j60conf.h'
// Title		: Microchip ENC28J60 Ethernet Interface Driver Configuration
// Author		: Pascal Stang
// Created		: 10/5/2004
// Revised		: 8/22/2005
// Version		: 0.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// Description	: This driver provides initialization and transmit/receive
//		functions for the ENC28J60 10Mb Ethernet Controller and PHY.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef ENC28J60CONF_H
#define ENC28J60CONF_H

#include <stdint.h>
typedef uint8_t  u08;
typedef uint16_t u16;
typedef uint32_t u32;

// ENC28J60 SPI port
#define ENC28J60_SPI_PORT		PORTB
#define ENC28J60_SPI_DDR		DDRB
#define ENC28J60_SPI_SCK		PORTB1
#define ENC28J60_SPI_MOSI		PORTB2
#define ENC28J60_SPI_MISO		PORTB3
#define ENC28J60_SPI_SS			PORTB0
// ENC28J60 control port
#define ENC28J60_CONTROL_PORT	PORTB
#define ENC28J60_CONTROL_DDR	DDRB
#define ENC28J60_CONTROL_CS		PORTB0

// MAC address for this interface
#define ENC28J60_MAC0 '0'
#define ENC28J60_MAC1 'F'
#define ENC28J60_MAC2 'F'
#define ENC28J60_MAC3 'I'
#define ENC28J60_MAC4 'C'
#define ENC28J60_MAC5 'E'

#endif /* ENC28J60CONF_H */
