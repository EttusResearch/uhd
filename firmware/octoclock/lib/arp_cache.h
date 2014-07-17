/* -*- c++ -*- */
/*
 * Copyright 2009-2011,2014 Ettus Research LLC
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
#ifndef INCLUDED_ARP_CACHE_H
#define INCLUDED_ARP_CACHE_H

#include <lwip/ip_addr.h>
#include <net/eth_mac_addr.h>
#include <stdbool.h>

void arp_cache_init(void);

void arp_cache_update(const struct ip_addr *ip,
		      const eth_mac_addr_t *mac);

bool arp_cache_lookup_mac(const struct ip_addr *ip,
			  eth_mac_addr_t *mac);

#endif /* INCLUDED_ARP_CACHE_H */
