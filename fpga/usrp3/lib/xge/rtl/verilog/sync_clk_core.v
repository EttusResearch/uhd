//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "sync_clk_core.v"                                 ////
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
 
module sync_clk_core(/*AUTOARG*/
  // Inputs
  clk_xgmii_tx, reset_xgmii_tx_n
  );
 
input         clk_xgmii_tx;
input         reset_xgmii_tx_n;
 
//input         ctrl_tx_disable_padding;
 
//output        ctrl_tx_disable_padding_ccr;
 
 
/*AUTOREG*/
 
/*AUTOWIRE*/
 
//wire  [0:0]             sig_out;
 
//assign {ctrl_tx_disable_padding_ccr} = sig_out;
 
//meta_sync #(.DWIDTH (1)) meta_sync0 (
//                      // Outputs
//                      .out              (sig_out),
//                      // Inputs
//                      .clk              (clk_xgmii_tx),
//                      .reset_n          (reset_xgmii_tx_n),
//                      .in               ({
//                                          ctrl_tx_disable_padding
//                                         }));
 
endmodule
 
