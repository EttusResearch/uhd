//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "defines.v"                                       ////
////                                                              ////
////  This file is part of the "10GE MAC" project                 ////
////  http://www.opencores.org/cores/xge_mac/                     ////
////                                                              ////
////  Author(s):                                                  ////
////      - A. Tanguay (antanguay@opencores.org)                  ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2008 AUTHORS. All rights reserved.             ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
 
// Define MDIO to add support for clause 22 and clause 45 MDIO interface
`define MDIO
// If WB clock is 62.5MHz and max MDC spec is 2.5MHz, then divide by 25
//`define MDC_HALF_PERIOD 13 // Closest int to 12.5
`define MDC_HALF_PERIOD 100
 
// CPU Registers
 
`define CPUREG_CONFIG0      8'h00
`define CPUREG_INT_PENDING  8'h04
`define CPUREG_INT_STATUS   8'h08
`define CPUREG_INT_MASK     8'h0c
//`ifdef MDIO
`define CPUREG_MDIO_DATA    8'h10
`define CPUREG_MDIO_ADDR    8'h14
`define CPUREG_MDIO_OP      8'h18
`define CPUREG_MDIO_CONTROL 8'h1c
`define CPUREG_MDIO_STATUS  8'h1c
`define CPUREG_GPIO         8'h20
//`endif

// Ethernet codes
 
`define IDLE       8'h07
`define PREAMBLE   8'h55
`define SEQUENCE   8'h9c
`define SFD        8'hd5
`define START      8'hfb
`define TERMINATE  8'hfd
`define ERROR      8'hfe
 
 
 
`define LINK_FAULT_OK      2'd0
`define LINK_FAULT_LOCAL   2'd1
`define LINK_FAULT_REMOTE  2'd2
 
`define FAULT_SEQ_LOCAL  1'b0
`define FAULT_SEQ_REMOTE 1'b1
 
`define LOCAL_FAULT   8'd1
`define REMOTE_FAULT  8'd2
 
`define PAUSE_FRAME   48'h010000c28001
 
`define LANE0        7:0
`define LANE1       15:8
`define LANE2      23:16
`define LANE3      31:24
`define LANE4      39:32
`define LANE5      47:40
`define LANE6      55:48
`define LANE7      63:56
 
 
`define TXSTATUS_NONE       8'h0
`define TXSTATUS_EOP        3'd6
`define TXSTATUS_SOP        3'd7
 
`define RXSTATUS_NONE       8'h0
`define RXSTATUS_ERR        3'd5
`define RXSTATUS_EOP        3'd6
`define RXSTATUS_SOP        3'd7
 
 
//
// FIFO Size: 8 * (2^AWIDTH) will be the size in bytes
//            7 --> 128 entries, 1024 bytes for data fifo
//
`define TX_DATA_FIFO_AWIDTH 9
`define RX_DATA_FIFO_AWIDTH 9
 
//
// FIFO Size: Holding FIFOs are 16 deep
//
`define TX_HOLD_FIFO_AWIDTH 4
`define RX_HOLD_FIFO_AWIDTH 4
 
 
// Memory types
`define MEM_AUTO_SMALL 1
`define MEM_AUTO_MEDIUM 2
`define MEM_AUTO_XILINX 3
 
 
// Changed system packet interface to big endian (12/12/2009)
// Comment out to use legacy mode
`define BIGENDIAN
