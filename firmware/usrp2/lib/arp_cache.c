/* -*- c++ -*- */
/*
 * Copyright 2009-2011 Ettus Research LLC
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "arp_cache.h"
#include <stddef.h>

typedef struct {
  struct ip_addr	ip;
  eth_mac_addr_t	mac;
} arp_cache_t;

#define	NENTRIES 8	// power-of-2

static size_t nentries;
static size_t victim;
static arp_cache_t cache[NENTRIES];

void
arp_cache_init(void)
{
  nentries = 0;
  victim = 0;
}

// returns non-negative index if found, else -1
static int
arp_cache_lookup(const struct ip_addr *ip)
{
  int i;
  for (i = 0; i < nentries; i++)
    if (cache[i].ip.addr == ip->addr)
      return i;

  return -1;
}

static int
arp_cache_alloc(void)
{
  if (nentries < NENTRIES)
    return nentries++;

  int i = victim;
  victim = (victim + 1) % NENTRIES;
  return i;
}

void 
arp_cache_update(const struct ip_addr *ip,
		 const eth_mac_addr_t *mac)
{
  int i = arp_cache_lookup(ip);
  if (i < 0){
    i = arp_cache_alloc();
    cache[i].ip = *ip;
    cache[i].mac = *mac;
  }
  else {
    cache[i].mac = *mac;
  }
}

bool
arp_cache_lookup_mac(const struct ip_addr *ip,
		     eth_mac_addr_t *mac)
{
  int i = arp_cache_lookup(ip);
  if (i < 0)
    return false;

  *mac = cache[i].mac;
  return true;
}
