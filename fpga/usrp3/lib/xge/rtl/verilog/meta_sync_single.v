//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "meta_sync_single.v"                              ////
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
 
 
module meta_sync_single(/*AUTOARG*/
  // Outputs
  out,
  // Inputs
  clk, reset_n, in
  );
 
parameter EDGE_DETECT = 0;
 
input   clk;
input   reset_n;
 
input   in;
 
output  out;
 
reg     out;
 
 
 
generate
 
    if (EDGE_DETECT) begin
 
      reg   meta;
      reg   edg1;
      reg   edg2;
 
        always @(posedge clk or negedge reset_n) begin
 
            if (reset_n == 1'b0) begin
                meta <= 1'b0;
                edg1 <= 1'b0;
                edg2 <= 1'b0;
                out <= 1'b0;
            end
            else begin
                meta <= in;
                edg1 <= meta;
                edg2 <= edg1;
                out <= edg1 ^ edg2;
            end
        end
 
    end
    else begin
 
      reg   meta;
 
        always @(posedge clk or negedge reset_n) begin
 
            if (reset_n == 1'b0) begin
                meta <= 1'b0;
                out <= 1'b0;
            end
            else begin
                meta <= in;
                out <= meta;
            end
        end
 
    end
 
endgenerate
 
endmodule
 
