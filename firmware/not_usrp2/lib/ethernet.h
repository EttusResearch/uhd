/*
 * Copyright 2010-2012 Ettus Research LLC
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

#ifndef INCLUDED_ETHERNET_H
#define INCLUDED_ETHERNET_H

#include <net/eth_mac_addr.h>
#include <lwip/ip_addr.h>
#include <stdbool.h>

typedef void (*ethernet_link_changed_callback_t)(int speed);


/*!
 * \brief one time call to initialize ethernet
 */
void ethernet_init(void);

/*!
 * \brief Specify the function to call on link state changes.
 * 
 * When the link comes up, speed is the link speed in Mbit/s.
 * When the link goes down, speed is 0.
 */
void ethernet_register_link_changed_callback(ethernet_link_changed_callback_t cb);

/*!
 * \returns ethernet MAC address
 */
const eth_mac_addr_t *ethernet_mac_addr(void);

/*!
 * \returns IP address
 */
const struct ip_addr *get_ip_addr(void);

/*!
 * \returns gateway address
 */
const struct ip_addr *get_gateway(void);

/*!
 * \returns subnet address
 */
const struct ip_addr *get_subnet(void);

/*!
 * \brief write ip address to eeprom and begin using it
 */
bool set_ip_addr(const struct ip_addr *t);

//! Apply default settings to eth addrs
void eth_addrs_set_default(void);

/*
 * \brief read RMON regs and return error mask
 */
int ethernet_check_errors(void);

#define	RME_RX_CRC	     0x0001
#define	RME_RX_FIFO_FULL     0x0002
#define RME_RX_2SHORT_2LONG  0x0004

#define	RME_TX_JAM_DROP	     0x0010
#define	RME_TX_FIFO_UNDER    0x0020
#define	RME_TX_FIFO_OVER     0x0040


typedef enum { LS_UNKNOWN, LS_DOWN, LS_UP } eth_link_state_t;

// flow control bitmasks
#define	FC_NONE		0x0
#define	FC_WE_TX	0x1			// we send PAUSE frames
#define	FC_WE_RX 	0x2			// we honor received PAUSE frames
#define	FC_SYMM		(FC_WE_TX | FC_WE_RX)

#define S_UNKNOWN (-1)			// unknown link speed

typedef struct {
  eth_link_state_t	link_state;
  int			link_speed;	// in Mb/s
  int			flow_control;
} ethernet_t;

#endif /* INCLUDED_ETHERNET_H */
