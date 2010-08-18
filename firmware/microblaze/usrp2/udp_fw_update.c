/* -*- c++ -*- */
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

//Routines to handle updating the SPI Flash firmware via UDP

#include "net_common.h"
#include "usrp2/fw_common.h"
#include "udp_fw_update.h"
#include <nonstdio.h>

//Firmware update packet handler
void handle_udp_fw_update_packet(struct socket_address src, struct socket_address dst,
                                 unsigned char *payload, int payload_len) {

  udp_fw_update_data_t update_data_out;
  update_data_out.id = USRP2_FW_UPDATE_ID_WAT;

  send_udp_pkt(USRP2_UDP_UPDATE_PORT, src, &update_data_out, sizeof(update_data_out));
}
