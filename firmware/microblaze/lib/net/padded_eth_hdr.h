/* -*- c++ -*- */
/*
 * Copyright 2009,2010 Ettus Research LLC
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
#ifndef INCLUDED_PADDED_ETH_HDR_H
#define INCLUDED_PADDED_ETH_HDR_H

#include <compiler.h>
#include <net/eth_mac_addr.h>

/*!
 * \brief Standard 14-byte ethernet header plus two leading bytes of padding.
 *
 * This is what a buffer contains in line 1 when using the "slow mode"
 */
typedef struct {
  uint16_t	 pad;
  eth_mac_addr_t dst;
  eth_mac_addr_t src;
  uint16_t 	 ethertype;
} _AL4 padded_eth_hdr_t;


#endif /* INCLUDED_PADDED_ETH_HDR_H */
