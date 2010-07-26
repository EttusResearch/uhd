/* -*- c -*- */
/*
 * Copyright 2010 Ettus Research LLC
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
#ifndef INCLUDED_SOCKET_ADDRESS_H
#define INCLUDED_SOCKET_ADDRESS_H

#include <lwip/ip_addr.h>

// port and address are in network byte order

typedef struct socket_address {
  unsigned short   port;
  struct ip_addr   addr;
} socket_address_t;

static inline struct socket_address 
make_socket_address(struct ip_addr addr, int port)
{
  struct socket_address r;
  r.port = port;
  r.addr = addr;
  return r;
}



#endif /* INCLUDED_SOCKET_ADDRESS_H */
