/* -*- c++ -*- */
/*
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
#ifndef INCLUDED_UDP_BURNER_PACKET_H
#define INCLUDED_UDP_BURNER_PACKET_H

#include <net/socket_address.h>

void
handle_udp_burner_packet(struct socket_address src, struct socket_address dst,
			 unsigned char *payload, int payload_len);


#endif /* INCLUDED_UDP_BURNER_PACKET_H */
