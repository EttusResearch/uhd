
/*
 *  Copyright (C) 2010-2012 Ettus Research, LLC
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

#define USRP_E_IOC_MAGIC	'u'
#define USRP_E_WRITE_CTL16	_IOW(USRP_E_IOC_MAGIC, 0x20, struct usrp_e_ctl16)
#define USRP_E_READ_CTL16	_IOWR(USRP_E_IOC_MAGIC, 0x21, struct usrp_e_ctl16)
#define USRP_E_WRITE_CTL32	_IOW(USRP_E_IOC_MAGIC, 0x22, struct usrp_e_ctl32)
#define USRP_E_READ_CTL32	_IOWR(USRP_E_IOC_MAGIC, 0x23, struct usrp_e_ctl32)
#define USRP_E_GET_RB_INFO      _IOR(USRP_E_IOC_MAGIC, 0x27, struct usrp_e_ring_buffer_size_t)
#define USRP_E_GET_COMPAT_NUMBER _IO(USRP_E_IOC_MAGIC, 0x28)

#define USRP_E_COMPAT_NUMBER 4

/* Flag defines */
#define RB_USER (1<<0)
#define RB_KERNEL (1<<1)
#define RB_OVERRUN (1<<2)
#define RB_DMA_ACTIVE (1<<3)
#define RB_USER_PROCESS (1<<4)

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
