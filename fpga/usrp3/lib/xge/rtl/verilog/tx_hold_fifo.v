//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "tx_hold_fifo.v"                                  ////
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
 
module tx_hold_fifo(/*AUTOARG*/
  // Outputs
  txhfifo_wfull, txhfifo_walmost_full, txhfifo_rdata, txhfifo_rstatus,
  txhfifo_rempty, txhfifo_ralmost_empty,
  // Inputs
  clk_xgmii_tx, reset_xgmii_tx_n, txhfifo_wdata, txhfifo_wstatus,
  txhfifo_wen, txhfifo_ren
  );
 
input         clk_xgmii_tx;
input         reset_xgmii_tx_n;
 
input [63:0]  txhfifo_wdata;
input [7:0]   txhfifo_wstatus;
input         txhfifo_wen;
 
input         txhfifo_ren;
 
output        txhfifo_wfull;
output        txhfifo_walmost_full;
 
output [63:0] txhfifo_rdata;
output [7:0]  txhfifo_rstatus;
output        txhfifo_rempty;
output        txhfifo_ralmost_empty;
 
generic_fifo #(
  .DWIDTH (72),
  .AWIDTH (`TX_HOLD_FIFO_AWIDTH),
  .REGISTER_READ (1),
  .EARLY_READ (1),
  .CLOCK_CROSSING (0),
  .ALMOST_EMPTY_THRESH (7),
  .ALMOST_FULL_THRESH (4),
//  .MEM_TYPE (`MEM_AUTO_SMALL)
  .MEM_TYPE (`MEM_AUTO_XILINX)
)
fifo0(
    .wclk (clk_xgmii_tx),
    .wrst_n (reset_xgmii_tx_n),
    .wen (txhfifo_wen),
    .wdata ({txhfifo_wstatus, txhfifo_wdata}),
    .wfull (txhfifo_wfull),
    .walmost_full (txhfifo_walmost_full),
 
    .rclk (clk_xgmii_tx),
    .rrst_n (reset_xgmii_tx_n),
    .ren (txhfifo_ren),
    .rdata ({txhfifo_rstatus, txhfifo_rdata}),
    .rempty (txhfifo_rempty),
    .ralmost_empty (txhfifo_ralmost_empty)
);
 
endmodule
 
 