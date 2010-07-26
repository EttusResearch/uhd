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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "udp_burner_packet.h"
#include "net_common.h"
#include "loader_parser.h"
#include <stdint.h>
#include <compiler.h>
#include <nonstdio.h>


void
handle_udp_burner_packet(struct socket_address src, struct socket_address dst,
			 unsigned char *payload, int payload_len)
{
  unsigned char reply[128] _AL4;
  size_t actual_reply_len;
  loader_parser(payload, payload_len, reply, sizeof(reply), &actual_reply_len);
  send_udp_pkt(dst.port, src, reply, actual_reply_len);
}
