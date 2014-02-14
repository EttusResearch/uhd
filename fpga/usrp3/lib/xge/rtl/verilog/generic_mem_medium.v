//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "generic_mem_medium.v"                            ////
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
 
 
module generic_mem_medium(
 
    wclk,
    wrst_n,
    wen,
    waddr,
    wdata,
 
    rclk,
    rrst_n,
    ren,
    roen,
    raddr,
    rdata
);
 
//---
// Parameters
 
parameter DWIDTH = 32;
parameter AWIDTH = 3;
parameter RAM_DEPTH = (1 << AWIDTH);
parameter SYNC_WRITE = 1;
parameter SYNC_READ = 1;
parameter REGISTER_READ = 0;
 
//---
// Ports
 
input               wclk;
input               wrst_n;
input               wen;
input  [AWIDTH:0]   waddr;
input  [DWIDTH-1:0] wdata;
 
input               rclk;
input               rrst_n;
input               ren;
input               roen;
input  [AWIDTH:0]   raddr;
output [DWIDTH-1:0] rdata;
 
// Registered outputs
reg    [DWIDTH-1:0] rdata;
 
 
//---
// Local declarations
 
// Registers
 
reg  [DWIDTH-1:0] mem_rdata;
 
 
// Memory - infer using Xilinx pragma for block ram.
//synthesis attribute ram_style of mem is block
reg  [DWIDTH-1:0] mem [0:RAM_DEPTH-1];
 
// Variables
 
integer         i;
 
 
//---
// Memory Write
 
generate
    if (SYNC_WRITE) begin
 
        // Generate synchronous write
        always @(posedge wclk)
        begin
            if (wen) begin
                mem[waddr[AWIDTH-1:0]] <= wdata;
            end
        end
    end
    else begin
 
        // Generate asynchronous write
        always @(wen, waddr, wdata)
        begin
            if (wen) begin
                mem[waddr[AWIDTH-1:0]] = wdata;
            end
        end
    end
endgenerate
 
//---
// Memory Read
 
generate
    if (SYNC_READ) begin
 
        // Generate registered memory read
        always @(posedge rclk /* IJB  or negedge rrst_n*/)
        begin
            if (!rrst_n) begin
                mem_rdata <= {(DWIDTH){1'b0}};
            end else if (ren) begin
                mem_rdata <= mem[raddr[AWIDTH-1:0]];
            end
        end
    end
    else begin
 
        // Generate unregisters memory read
        always @(raddr, rclk)
        begin
            mem_rdata = mem[raddr[AWIDTH-1:0]];
        end
    end
endgenerate
 
generate
    if (REGISTER_READ) begin
 
        // Generate registered output
        always @(posedge rclk /* IJB or negedge rrst_n*/ )
        begin
            if (!rrst_n) begin
                rdata <= {(DWIDTH){1'b0}};
            end else if (roen) begin
                rdata <= mem_rdata;
            end
        end
 
    end
    else begin
 
        // Generate unregisters output
        always @(mem_rdata)
        begin
            rdata = mem_rdata;
        end
 
    end
endgenerate
 
endmodule
 
