//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "rx_data_fifo.v"                                  ////
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
 
module rx_data_fifo(/*AUTOARG*/
  // Outputs
  rxdfifo_wfull, rxdfifo_rdata, rxdfifo_rstatus, rxdfifo_rempty,
  rxdfifo_ralmost_empty,
  // Inputs
  clk_xgmii_rx, clk_156m25, reset_xgmii_rx_n, reset_156m25_n,
  rxdfifo_wdata, rxdfifo_wstatus, rxdfifo_wen, rxdfifo_ren
  );
 
input         clk_xgmii_rx;
input         clk_156m25;
input         reset_xgmii_rx_n;
input         reset_156m25_n;
 
input [63:0]  rxdfifo_wdata;
input [7:0]   rxdfifo_wstatus;
input         rxdfifo_wen;
 
input         rxdfifo_ren;
 
output        rxdfifo_wfull;
 
output [63:0] rxdfifo_rdata;
output [7:0]  rxdfifo_rstatus;
output        rxdfifo_rempty;
output        rxdfifo_ralmost_empty;
 
generic_fifo #(
  .DWIDTH (72),
  .AWIDTH (`RX_DATA_FIFO_AWIDTH),
  .REGISTER_READ (0),
  .EARLY_READ (1),
  .CLOCK_CROSSING (1),
  .ALMOST_EMPTY_THRESH (4),
  .MEM_TYPE (`MEM_AUTO_XILINX)
)
fifo0(
    .wclk (clk_xgmii_rx),
    .wrst_n (reset_xgmii_rx_n),
    .wen (rxdfifo_wen),
    .wdata ({rxdfifo_wstatus, rxdfifo_wdata}),
    .wfull (rxdfifo_wfull),
    .walmost_full (),
 
    .rclk (clk_156m25),
    .rrst_n (reset_156m25_n),
    .ren (rxdfifo_ren),
    .rdata ({rxdfifo_rstatus, rxdfifo_rdata}),
    .rempty (rxdfifo_rempty),
    .ralmost_empty (rxdfifo_ralmost_empty)
);
 
 
endmodule
 
