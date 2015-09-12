/*
 * Copyright 2015 Ettus Research LLC
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

#ifndef _NET_ENC28J60_H_
#define _NET_ENC28J60_H_

#include <avr/io.h>

#define SPI_DDR  DDRB
#define SPI_PORT PORTB
#define SPI_CS   0
#define SPI_MOSI 2
#define SPI_MISO 3
#define SPI_SCK  1

// Register Masks
#define ADDR_MASK 0x1F
#define BANK_MASK 0x60
#define SPRD_MASK 0x80

// All Banks Registers
#define EIE      0x1B 
#define EIR      0x1C
#define ESTAT    0x1D
#define ECON2    0x1E
#define ECON1    0x1F

// Bank 0 Registers
#define ERDPTL   0x00
#define ERDPTH   0x01
#define EWRPTL   0x02
#define EWRPTH   0x03
#define ETXSTL   0x04
#define ETXSTH   0x05
#define ETXNDL   0x06
#define ETXNDH   0x07
#define ERXSTL   0x08
#define ERXSTH   0x09
#define ERXNDL   0x0A
#define ERXNDH   0x0B
#define ERXRDPTL 0x0C
#define ERXRDPTH 0x0D
#define ERXWRPTL 0x0E
#define ERXWRPTH 0x0F
#define EDMASTL  0x10
#define EDMASTH  0x11
#define EDMANDL  0x12
#define EDMANDH  0x13
#define EDMADSTL 0x14
#define EDMADSTH 0x15
#define EDMACSL  0x16
#define EDMACSH  0x17

// Bank 1 Registers
#define EHT0     0x20
#define EHT1     0x21
#define EHT2     0x22
#define EHT3     0x23
#define EHT4     0x24
#define EHT5     0x25
#define EHT6     0x26
#define EHT7     0x27
#define EPMM0    0x28
#define EPMM1    0x29
#define EPMM2    0x2A
#define EPMM3    0x2B
#define EPMM4    0x2C
#define EPMM5    0x2D
#define EPMM6    0x2E
#define EPMM7    0x2F
#define EPMCSL   0x30
#define EPMCSH   0x31
#define EPMOL    0x34
#define EPMOH    0x35
#define EWOLIE   0x36
#define EWOLIR   0x37
#define ERXFCON  0x38
#define EPKTCNT  0x39

// Bank 2 Register
#define MACON1   0xC0
#define MACON2   0xC1
#define MACON3   0xC2
#define MACON4   0xC3
#define MABBIPG  0xC4
#define MAIPGL   0xC6
#define MAIPGH   0xC7
#define MACLCON1 0xC8
#define MACLCON2 0xC9
#define MAMXFLL  0xCA
#define MAMXFLH  0xCB
#define MAPHSUP  0xCD
#define MICON    0xD1
#define MICMD    0xD2
#define MIREGADR 0xD4
#define MIWRL    0xD6
#define MIWRH    0xD7
#define MIRDL    0xD8
#define MIRDH    0xD9

// Bank 3 Registers
#define MAADR1   0xE0
#define MAADR0   0xE1
#define MAADR3   0xE2
#define MAADR2   0xE3
#define MAADR5   0xE4
#define MAADR4   0xE5
#define EBSTSD   0x66
#define EBSTCON  0x67
#define EBSTCSL  0x68
#define EBSTCSH  0x69
#define MISTAT   0xEA
#define EREVID   0x72
#define ECOCON   0x75
#define EFLOCON  0x77
#define EPAUSL   0x78
#define EPAUSH   0x79

// PHY Registers
#define PHCON1    0x00
#define PHSTAT1   0x01
#define PHHID1    0x02
#define PHHID2    0x03
#define PHCON2    0x10
#define PHSTAT2   0x11
#define PHIE      0x12
#define PHIR      0x13
#define PHLCON    0x14

// ERXFCON bit definitions
#define UCEN      0x80
#define ANDOR     0x40
#define CRCEN     0x20
#define PMEN      0x10
#define MPEN      0x08
#define HTEN      0x04
#define MCEN      0x02
#define BCEN      0x01

// EIE bit definitions
#define INTIE     0x80
#define PKTIE     0x40
#define DMAIE     0x20
#define LINKIE    0x10
#define TXIE      0x08
#define WOLIE     0x04
#define TXERIE    0x02
#define RXERIE    0x01

// EIR bit definitions
#define PKTIF     0x40
#define DMAIF     0x20
#define LINKIF    0x10
#define TXIF      0x08
#define WOLIF     0x04
#define TXERIF    0x02
#define RXERIF    0x01

// ESTAT bit definitions
#define INT       0x80
#define LATECOL   0x10
#define RXBUSY    0x04
#define TXABRT    0x02
#define CLKRDY    0x01

// ECON2 bit definitions
#define AUTOINC   0x80
#define PKTDEC    0x40
#define PWRSV     0x20
#define VRPS      0x08

// ECON1 bit definitions
#define TXRST     0x80
#define RXRST     0x40
#define DMAST     0x20
#define CSUMEN    0x10
#define TXRTS     0x08
#define ENCRXEN   0x04
#define BSEL1     0x02
#define BSEL0     0x01

// MACON1 bit definitions
#define LOOPBK    0x10
#define TXPAUS    0x08
#define RXPAUS    0x04
#define PASSALL   0x02
#define MARXEN    0x01

// MACON2 bit definitions
#define MARST     0x80
#define RNDRST    0x40
#define MARXRST   0x08
#define RFUNRST   0x04
#define MATXRST   0x02
#define TFUNRST   0x01

// MACON3 bit definitions
#define PADCFG2   0x80
#define PADCFG1   0x40
#define PADCFG0   0x20
#define TXCRCEN   0x10
#define PHDRLEN   0x08
#define HFRMLEN   0x04
#define FRMLNEN   0x02
#define FULDPX    0x01

// MICMD bit definitions
#define MIISCAN   0x02
#define MIIRD     0x01

// MISTAT bit definitions
#define NVALID    0x04
#define SCAN      0x02
#define BUSY      0x01

// PHCON1 bit definitions
#define PRST      0x8000
#define PLOOPBK   0x4000
#define PPWRSV    0x0800
#define PDPXMD    0x0100

// PHSTAT1 bit definitions
#define PFDPX     0x1000
#define PHDPX     0x0800
#define LLSTAT    0x0004
#define JBSTAT    0x0002

// PHCON2 bit definitions
#define FRCLINK   0x4000
#define TXDIS     0x2000
#define JABBER    0x0400
#define HDLDIS    0x0100

// Packet Control bit Definitions
#define PHUGEEN   0x08
#define PPADEN    0x04
#define PCRCEN    0x02
#define POVERRIDE 0x01

// SPI Instruction Set
#define RCR 0x00 // Read Control Register
#define RBM 0x3A // Read Buffer Memory
#define WCR 0x40 // Write Control Register
#define WBM 0x7A // Write Buffer Memory
#define BFS 0x80 // Bit Field Set
#define BFC 0xA0 // Bit Field Clear
#define SC  0xFF // Soft Reset

// Buffer
#define RXSTART_INIT 0x0000
#define RXSTOP_INIT  (0x1FFF-0x0600-1)
#define TXSTART_INIT (0x1FFF-0x0600)
#define TXSTOP_INIT  0x1FFF
#define MAX_FRAMELEN 1500

void enc28j60_init(uint8_t* mac_addr);

uint16_t enc28j60_recv(uint8_t* buffer, uint16_t max_len);

void enc28j60_send(uint8_t* buffer, uint16_t len);

#endif /* _NET_ENC28J60_H_ */
