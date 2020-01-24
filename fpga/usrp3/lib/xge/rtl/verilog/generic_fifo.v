//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "generic_fifo.v"                                  ////
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
 
module generic_fifo(
 
    wclk,
    wrst_n,
    wen,
    wdata,
    wfull,
    walmost_full,
 
    rclk,
    rrst_n,
    ren,
    rdata,
    rempty,
    ralmost_empty
);
 
//---
// Parameters
 
parameter DWIDTH = 32;
parameter AWIDTH = 3;
parameter RAM_DEPTH = (1 << AWIDTH);
parameter SYNC_WRITE = 1;
parameter SYNC_READ = 1;
parameter REGISTER_READ = 0;
parameter EARLY_READ = 0;
parameter CLOCK_CROSSING = 1;
parameter ALMOST_EMPTY_THRESH = 1;
parameter ALMOST_FULL_THRESH = RAM_DEPTH-2;
parameter MEM_TYPE = `MEM_AUTO_SMALL;
 
//---
// Ports
 
input          wclk;
input          wrst_n;
input          wen;
input  [DWIDTH-1:0] wdata;
output         wfull;
output         walmost_full;
 
input          rclk;
input          rrst_n;
input          ren;
output [DWIDTH-1:0] rdata;
output         rempty;
output         ralmost_empty;
 
// Wires
 
wire             mem_wen;
wire [AWIDTH:0]  mem_waddr;
 
wire             mem_ren;
wire [AWIDTH:0]  mem_raddr;
 
 
generic_fifo_ctrl #(.AWIDTH (AWIDTH),
                    .RAM_DEPTH (RAM_DEPTH), 
                    .EARLY_READ (EARLY_READ),
                    .CLOCK_CROSSING (CLOCK_CROSSING),
                    .ALMOST_EMPTY_THRESH (ALMOST_EMPTY_THRESH),
                    .ALMOST_FULL_THRESH (ALMOST_FULL_THRESH)
                    )
  ctrl0(.wclk (wclk),
        .wrst_n (wrst_n),
        .wen (wen),
        .wfull (wfull),
        .walmost_full (walmost_full),
 
        .mem_wen (mem_wen),
        .mem_waddr (mem_waddr),
 
        .rclk (rclk),
        .rrst_n (rrst_n),
        .ren (ren),
        .rempty (rempty),
        .ralmost_empty (ralmost_empty),
 
        .mem_ren (mem_ren),
        .mem_raddr (mem_raddr)
        );
 
 
   generate
      if (MEM_TYPE == `MEM_AUTO_SMALL) begin
	 
         generic_mem_small #(.DWIDTH (DWIDTH),
                             .AWIDTH (AWIDTH),
                             .RAM_DEPTH (RAM_DEPTH),
                             .SYNC_WRITE (SYNC_WRITE),
                             .SYNC_READ (SYNC_READ),
                             .REGISTER_READ (REGISTER_READ)
                             )
           mem0(.wclk (wclk),
		.wrst_n (wrst_n),
		.wen (mem_wen),	
		.waddr (mem_waddr),
		.wdata (wdata),
	   
		.rclk (rclk),
		.rrst_n (rrst_n),
		.ren (mem_ren),
		.roen (ren),
		.raddr (mem_raddr),
		.rdata (rdata)
		);
	 
      end
      
      if (MEM_TYPE == `MEM_AUTO_MEDIUM) begin
	 
         generic_mem_medium #(.DWIDTH (DWIDTH),
                              .AWIDTH (AWIDTH),
                              .RAM_DEPTH (RAM_DEPTH),
                              .SYNC_WRITE (SYNC_WRITE),
                              .SYNC_READ (SYNC_READ),
                              .REGISTER_READ (REGISTER_READ)
                              )
           mem0(.wclk (wclk),
		.wrst_n (wrst_n),
		.wen (mem_wen),	
		.waddr (mem_waddr),
		.wdata (wdata),
	   
		.rclk (rclk),
		.rrst_n (rrst_n),
		.ren (mem_ren),
		.roen (ren),
		.raddr (mem_raddr),
		.rdata (rdata)
		);
	 
      end // if (MEM_TYPE == `MEM_AUTO_MEDIUM)

      if (MEM_TYPE == `MEM_AUTO_XILINX) begin

	 
	 generic_mem_xilinx_block #(.DWIDTH (DWIDTH),
				    .AWIDTH (AWIDTH),
				    .RAM_DEPTH (RAM_DEPTH),
				    .SYNC_WRITE (SYNC_WRITE),
				    .SYNC_READ (SYNC_READ),
				    .REGISTER_READ (REGISTER_READ)
				    )
           mem0(.wclk (wclk),
		.wrst_n (wrst_n),
		.wen (mem_wen),	
		.waddr (mem_waddr),
		.wdata (wdata),
	   
		.rclk (rclk),
		.rrst_n (rrst_n),
		.ren (mem_ren),
		.roen (ren),
		.raddr (mem_raddr),
		.rdata (rdata)
		);
      end

   endgenerate
   
endmodule

