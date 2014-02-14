//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "meta_sync.v"                                     ////
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
 
 
module meta_sync(/*AUTOARG*/
  // Outputs
  out,
  // Inputs
  clk, reset_n, in
  );
 
parameter DWIDTH = 1;
parameter EDGE_DETECT = 0;
 
input                clk;
input                reset_n;
 
input  [DWIDTH-1:0]  in;
 
output [DWIDTH-1:0]  out;
 
generate
genvar               i;
 
    for (i = 0; i < DWIDTH; i = i + 1) begin : meta
 
        meta_sync_single #(.EDGE_DETECT (EDGE_DETECT))
          meta_sync_single0 (
                      // Outputs
                      .out              (out[i]),
                      // Inputs
                      .clk              (clk),
                      .reset_n          (reset_n),
                      .in               (in[i]));
 
    end
 
endgenerate
 
endmodule
 
 