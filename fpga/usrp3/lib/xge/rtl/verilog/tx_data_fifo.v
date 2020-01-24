//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "tx_data_fifo.v"                                  ////
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
 
module tx_data_fifo(/*AUTOARG*/
  // Outputs
  txdfifo_wfull, txdfifo_walmost_full, txdfifo_rdata, txdfifo_rstatus,
  txdfifo_rempty, txdfifo_ralmost_empty,
  // Inputs
  clk_xgmii_tx, clk_156m25, reset_xgmii_tx_n, reset_156m25_n,
  txdfifo_wdata, txdfifo_wstatus, txdfifo_wen, txdfifo_ren
  );
 
input         clk_xgmii_tx;
input         clk_156m25;
input         reset_xgmii_tx_n;
input         reset_156m25_n;
 
input [63:0]  txdfifo_wdata;
input [7:0]   txdfifo_wstatus;
input         txdfifo_wen;
 
input         txdfifo_ren;
 
output        txdfifo_wfull;
output        txdfifo_walmost_full;
 
output [63:0] txdfifo_rdata;
output [7:0]  txdfifo_rstatus;
output        txdfifo_rempty;
output        txdfifo_ralmost_empty;
 
generic_fifo #(
  .DWIDTH (72),
  .AWIDTH (`TX_DATA_FIFO_AWIDTH),
  .REGISTER_READ (1),
  .EARLY_READ (1),
  .CLOCK_CROSSING (1),
  .ALMOST_EMPTY_THRESH (7),
  .ALMOST_FULL_THRESH (12),
  .MEM_TYPE (`MEM_AUTO_XILINX)
)
fifo0(
    .wclk (clk_156m25),
    .wrst_n (reset_156m25_n),
    .wen (txdfifo_wen),
    .wdata ({txdfifo_wstatus, txdfifo_wdata}),
    .wfull (txdfifo_wfull),
    .walmost_full (txdfifo_walmost_full),
 
    .rclk (clk_xgmii_tx),
    .rrst_n (reset_xgmii_tx_n),
    .ren (txdfifo_ren),
    .rdata ({txdfifo_rstatus, txdfifo_rdata}),
    .rempty (txdfifo_rempty),
    .ralmost_empty (txdfifo_ralmost_empty)
);
 
endmodule
 
