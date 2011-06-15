/* 
 * USRP - Universal Software Radio Peripheral
 *
 * Copyright (C) 2003,2004 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301  USA
 */

#ifndef INCLUDED_FPGA_REV1_H
#define INCLUDED_FPGA_REV1_H

/*
 * return TRUE if FPGA internal fifo has room for a single packet
 */
#define fpga_has_room_for_data_packet()	(!(GPIFREADYSTAT & bmDATA_FIFO_FULL))
#define fpga_has_room_for_ctrl_packet()	(!(GPIFREADYSTAT & bmCTRL_FIFO_FULL))

/*
 * return TRUE if FPGA internal fifo has at least one packet available
 */
#define fpga_has_data_packet_avail()		(!(GPIFREADYSTAT & bmDATA_EMPTY))
#define fpga_has_ctrl_packet_avail()            (!(GPIFREADYSTAT & bmCTRL_EMPTY))

#define fx2_has_ctrl_packet_avail()         (!(EP24FIFOFLGS & EP4FIFOEMPTY))
#define fx2_has_data_packet_avail()         (!(EP24FIFOFLGS & EP2FIFOEMPTY))

#define fx2_has_room_for_ctrl_packet()      (!(EP8CS & bmEPFULL))
#define fx2_has_room_for_data_packet()      (!(EP6CS & bmEPFULL))

#define fx2_gpif_is_idle()                  (GPIFTRIG & bmGPIF_IDLE) 

#endif /* INCLUDED_FPGA_REV1_H */
