
/*
 *  Copyright (C) 2010 Ettus Research, LLC
 *
 *  Written by Philip Balister <philip@opensdr.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#ifndef __USRP_E_H
#define __USRP_E_H

#include <linux/types.h>
#include <linux/ioctl.h>

struct usrp_e_ctl16 {
	__u32 offset;
	__u32 count;
	__u16 buf[20];
};

struct usrp_e_ctl32 {
	__u32 offset;
	__u32 count;
	__u32 buf[10];
};

/* SPI interface */

#define UE_SPI_TXONLY	0
#define UE_SPI_TXRX	1

/* Defines for spi ctrl register */
#define UE_SPI_CTRL_TXNEG	(1<<10)
#define UE_SPI_CTRL_RXNEG	(1<<9)

#define UE_SPI_PUSH_RISE	0
#define UE_SPI_PUSH_FALL	UE_SPI_CTRL_TXNEG
#define UE_SPI_LATCH_RISE	0
#define UE_SPI_LATCH_FALL	UE_SPI_CTRL_RXNEG

struct usrp_e_spi {
	__u8 readback;
	__u32 slave;
	__u32 data;
	__u32 length;
	__u32 flags;
};

struct usrp_e_i2c {
	__u8 addr;
	__u32 len;
	__u8 data[];
};

#define USRP_E_IOC_MAGIC	'u'
#define USRP_E_WRITE_CTL16	_IOW(USRP_E_IOC_MAGIC, 0x20, struct usrp_e_ctl16)
#define USRP_E_READ_CTL16	_IOWR(USRP_E_IOC_MAGIC, 0x21, struct usrp_e_ctl16)
#define USRP_E_WRITE_CTL32	_IOW(USRP_E_IOC_MAGIC, 0x22, struct usrp_e_ctl32)
#define USRP_E_READ_CTL32	_IOWR(USRP_E_IOC_MAGIC, 0x23, struct usrp_e_ctl32)
#define USRP_E_SPI		_IOWR(USRP_E_IOC_MAGIC, 0x24, struct usrp_e_spi)
#define USRP_E_I2C_READ		_IOWR(USRP_E_IOC_MAGIC, 0x25, struct usrp_e_i2c)
#define USRP_E_I2C_WRITE	_IOW(USRP_E_IOC_MAGIC, 0x26, struct usrp_e_i2c)
#define USRP_E_GET_RB_INFO      _IOR(USRP_E_IOC_MAGIC, 0x27, struct usrp_e_ring_buffer_size_t)

/* Flag defines */
#define RB_USER (1<<0)
#define RB_KERNEL (1<<1)
#define RB_OVERRUN (1<<2)
#define RB_DMA_ACTIVE (1<<3)

struct ring_buffer_info {
	int flags;
	int len;
};

struct usrp_e_ring_buffer_size_t {
	int num_pages_rx_flags;
	int num_rx_frames;
	int num_pages_tx_flags;
	int num_tx_frames;
};

#endif
