//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "rx_hold_fifo.v"                                  ////
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
 
 
`include "defines.v"
 
module rx_hold_fifo(/*AUTOARG*/
  // Outputs
  rxhfifo_rdata, rxhfifo_rstatus, rxhfifo_rempty,
  rxhfifo_ralmost_empty,
  // Inputs
  clk_xgmii_rx, reset_xgmii_rx_n, rxhfifo_wdata, rxhfifo_wstatus,
  rxhfifo_wen, rxhfifo_ren
  );
 
input         clk_xgmii_rx;
input         reset_xgmii_rx_n;
 
input [63:0]  rxhfifo_wdata;
input [7:0]   rxhfifo_wstatus;
input         rxhfifo_wen;
 
input         rxhfifo_ren;
 
output [63:0] rxhfifo_rdata;
output [7:0]  rxhfifo_rstatus;
output        rxhfifo_rempty;
output        rxhfifo_ralmost_empty;
 
generic_fifo #(
  .DWIDTH (72),
  .AWIDTH (`RX_HOLD_FIFO_AWIDTH),
  .REGISTER_READ (1),
  .EARLY_READ (1),
  .CLOCK_CROSSING (0),
  .ALMOST_EMPTY_THRESH (7),
  .MEM_TYPE (`MEM_AUTO_XILINX)
)
fifo0(
    .wclk (clk_xgmii_rx),
    .wrst_n (reset_xgmii_rx_n),
    .wen (rxhfifo_wen),
    .wdata ({rxhfifo_wstatus, rxhfifo_wdata}),
    .wfull (),
    .walmost_full (),
 
    .rclk (clk_xgmii_rx),
    .rrst_n (reset_xgmii_rx_n),
    .ren (rxhfifo_ren),
    .rdata ({rxhfifo_rstatus, rxhfifo_rdata}),
    .rempty (rxhfifo_rempty),
    .ralmost_empty (rxhfifo_ralmost_empty)
);
 
 
endmodule
 
