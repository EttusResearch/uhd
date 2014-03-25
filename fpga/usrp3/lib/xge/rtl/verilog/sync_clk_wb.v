//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "sync_clk_wb.v"                                   ////
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
 
module sync_clk_wb(/*AUTOARG*/
  // Outputs
  status_crc_error, status_fragment_error, status_txdfifo_ovflow,
  status_txdfifo_udflow, status_rxdfifo_ovflow, status_rxdfifo_udflow,
  status_pause_frame_rx, status_local_fault, status_remote_fault,
  // Inputs
  wb_clk_i, wb_rst_i, status_crc_error_tog, status_fragment_error_tog,
  status_txdfifo_ovflow_tog, status_txdfifo_udflow_tog,
  status_rxdfifo_ovflow_tog, status_rxdfifo_udflow_tog,
  status_pause_frame_rx_tog, status_local_fault_crx,
  status_remote_fault_crx
  );
 
input         wb_clk_i;
input         wb_rst_i;
 
input         status_crc_error_tog;
input         status_fragment_error_tog;
 
input         status_txdfifo_ovflow_tog;
 
input         status_txdfifo_udflow_tog;
 
input         status_rxdfifo_ovflow_tog;
 
input         status_rxdfifo_udflow_tog;
 
input         status_pause_frame_rx_tog;
 
input         status_local_fault_crx;
input         status_remote_fault_crx;
 
output        status_crc_error;
output        status_fragment_error;
 
output        status_txdfifo_ovflow;
 
output        status_txdfifo_udflow;
 
output        status_rxdfifo_ovflow;
 
output        status_rxdfifo_udflow;
 
output        status_pause_frame_rx;
 
output        status_local_fault;
output        status_remote_fault;
 
/*AUTOREG*/
 
/*AUTOWIRE*/
 
wire  [6:0]             sig_out1;
wire  [1:0]             sig_out2;
 
assign status_crc_error = sig_out1[6];
assign status_fragment_error = sig_out1[5];
assign status_txdfifo_ovflow = sig_out1[4];
assign status_txdfifo_udflow = sig_out1[3];
assign status_rxdfifo_ovflow = sig_out1[2];
assign status_rxdfifo_udflow = sig_out1[1];
assign status_pause_frame_rx = sig_out1[0];
 
assign status_local_fault = sig_out2[1];
assign status_remote_fault = sig_out2[0];
 
meta_sync #(.DWIDTH (7), .EDGE_DETECT (1)) meta_sync0 (
                      // Outputs
                      .out              (sig_out1),
                      // Inputs
                      .clk              (wb_clk_i),
                      .reset_n          (~wb_rst_i),
                      .in               ({
                                          status_crc_error_tog,
                                          status_fragment_error_tog,
                                          status_txdfifo_ovflow_tog,
                                          status_txdfifo_udflow_tog,
                                          status_rxdfifo_ovflow_tog,
                                          status_rxdfifo_udflow_tog,
                                          status_pause_frame_rx_tog
                                         }));
 
meta_sync #(.DWIDTH (2), .EDGE_DETECT (0)) meta_sync1 (
                      // Outputs
                      .out              (sig_out2),
                      // Inputs
                      .clk              (wb_clk_i),
                      .reset_n          (~wb_rst_i),
                      .in               ({
                                          status_local_fault_crx,
                                          status_remote_fault_crx
                                         }));
 
endmodule
 