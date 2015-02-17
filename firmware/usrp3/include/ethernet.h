/* -*- c -*- */
/*
 * Copyright 2007,2009 Free Software Foundation, Inc.
 * Copyright 2009 Ettus Research LLC
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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef void (*ethernet_link_changed_callback_t)(int ethnum, int speed);

#define MDIO_PORT 4

/*!
 * \brief one time call to initialize ethernet
 */
void ethernet_init(const uint32_t eth);

/*!
 * \brief Return number of ethernet interfaces
 */
int ethernet_ninterfaces(void);


void dump_mdio_regs(const uint8_t eth, uint32_t mdio_port);

/*!
 * \brief Test status of SFP+ modules
 */
void poll_sfpp_status(const uint32_t eth);

//! get the link status of eth (true for link up)
bool ethernet_get_link_up(const uint32_t eth);

#endif /* INCLUDED_ETHERNET_H */
