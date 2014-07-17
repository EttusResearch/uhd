/*! \file enc28j60.c \brief Microchip ENC28J60 Ethernet Interface Driver. */
//*****************************************************************************
//
// File Name	: 'enc28j60.c'
// Title		: Microchip ENC28J60 Ethernet Interface Driver
// Author		: Pascal Stang (c)2005
// Created		: 9/22/2005
// Revised		: 5/19/2014
// Version		: 0.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// Description	: This driver provides initialization and transmit/receive
//	functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//
//*****************************************************************************

#include <octoclock.h>

#include <net/enc28j60.h>
#include <net/enc28j60conf.h>

#include <avr/io.h>
#include <util/delay.h>

u08 Enc28j60Bank;
u16 NextPacketPtr;

u08 enc28j60ReadOp(u08 op, u08 address)
{
	u08 data;
   
	// assert CS
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);
	
	// issue read command
	SPDR = op | (address & ADDR_MASK);
	while(!(SPSR & (1<<SPIF)));
	// read data
	SPDR = 0x00;
	while(!(SPSR & (1<<SPIF)));
	// do dummy read if needed
	if(address & 0x80)
	{
		SPDR = 0x00;
		while(!(inb(SPSR) & (1<<SPIF)));
	}
	data = SPDR;
	
	// release CS
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);

	return data;
}

void enc28j60WriteOp(u08 op, u08 address, u08 data)
{
	// assert CS
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);

	// issue write command
	SPDR = op | (address & ADDR_MASK);
	while(!(SPSR & (1<<SPIF)));
	// write data
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));

	// release CS
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
}

void enc28j60ReadBuffer(u16 len, u08* data)
{
	// assert CS
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);
	
	// issue read command
	SPDR = ENC28J60_READ_BUF_MEM;
	while(!(SPSR & (1<<SPIF)));
	while(len--)
	{
		// read data
		SPDR = 0x00;
		while(!(SPSR & (1<<SPIF)));
		*data++ = SPDR;
	}	
	// release CS
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
}

void enc28j60WriteBuffer(u16 len, u08* data)
{
	// assert CS
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);
	
	// issue write command
	SPDR = ENC28J60_WRITE_BUF_MEM;
	while(!(SPSR & (1<<SPIF)));
	while(len--)
	{
		// write data
		SPDR = *data++;
		while(!(SPSR & (1<<SPIF)));
	}	
	// release CS
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
}

void enc28j60SetBank(u08 address)
{
	// set the bank (if needed)
	if((address & BANK_MASK) != Enc28j60Bank)
	{
		// set the bank
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

u08 enc28j60Read(u08 address)
{
	// set the bank
	enc28j60SetBank(address);
	// do the read
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60Write(u08 address, u08 data)
{
	// set the bank
	enc28j60SetBank(address);
	// do the write
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

u16 enc28j60PhyRead(u08 address)
{
	u16 data;

	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);

	// quit reading
	enc28j60Write(MICMD, 0x00);
	
	// get data value
	data  = enc28j60Read(MIRDL);
	data |= enc28j60Read(MIRDH);
	// return the data
	return data;
}

void enc28j60PhyWrite(u08 address, u16 data)
{
	// set the PHY register address
	enc28j60Write(MIREGADR, address);
	
	// write the PHY data
	enc28j60Write(MIWRL, data);	
	enc28j60Write(MIWRH, data>>8);

	// wait until the PHY write completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);
}

void enc28j60Init(u08* macaddr)
{
	// initialize I/O
	sbi(ENC28J60_CONTROL_DDR, ENC28J60_CONTROL_CS);
	sbi(ENC28J60_CONTROL_PORT, ENC28J60_CONTROL_CS);

	// setup SPI I/O pins
	sbi(ENC28J60_SPI_PORT, ENC28J60_SPI_SCK);	// set SCK hi
	sbi(ENC28J60_SPI_DDR, ENC28J60_SPI_SCK);	// set SCK as output
	cbi(ENC28J60_SPI_DDR, ENC28J60_SPI_MISO);	// set MISO as input
	sbi(ENC28J60_SPI_DDR, ENC28J60_SPI_MOSI);	// set MOSI as output
	sbi(ENC28J60_SPI_DDR, ENC28J60_SPI_SS);		// SS must be output for Master mode to work
	// initialize SPI interface
	// master mode
	sbi(SPCR, MSTR);
	// select clock phase positive-going in middle of data
	cbi(SPCR, CPOL);
	// Data order MSB first
	cbi(SPCR,DORD);
	// switch to f/4 2X = f/2 bitrate
	cbi(SPCR, SPR0);
	cbi(SPCR, SPR1);
	sbi(SPSR, SPI2X);
	// enable SPI
	sbi(SPCR, SPE);

	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	// check CLKRDY bit to see if reset is complete
	_delay_us(51);

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// set transmit buffer start
	// ETXST defaults to 0x0000 (beginnging of ram)
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);

	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);

	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR5, macaddr[0]);
	enc28j60Write(MAADR4, macaddr[1]);
	enc28j60Write(MAADR3, macaddr[2]);
	enc28j60Write(MAADR2, macaddr[3]);
	enc28j60Write(MAADR1, macaddr[4]);
	enc28j60Write(MAADR0, macaddr[5]);

	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

void enc28j60PacketSend(unsigned int len1, unsigned char* packet1, unsigned int len2, unsigned char* packet2)
{
	//Errata: Transmit Logic reset
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);

	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT&0xff);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len1+len2));
	enc28j60Write(ETXNDH, (TXSTART_INIT+len1+len2)>>8);

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len1, packet1);
	if(len2>0) enc28j60WriteBuffer(len2, packet2);
	
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

unsigned int enc28j60PacketReceive(unsigned int maxlen, u08* buf)
{
	u16 rxstat;
	u16 len;

	// check if a packet has been received and buffered
	if( !enc28j60Read(EPKTCNT) )
		return 0;

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	// (we reduce the MAC-reported length by 4 to remove the CRC)
	len = MIN(len, maxlen);

	// copy the packet from the receive buffer
	enc28j60ReadBuffer(len, buf);

	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60Write(ERXRDPTL, (NextPacketPtr));
	enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);

	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return len;
}
