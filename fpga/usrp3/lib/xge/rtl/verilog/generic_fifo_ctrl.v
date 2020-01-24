//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "generic_fifo_ctrl.v"                             ////
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
 
 
module generic_fifo_ctrl(
 
    wclk,
    wrst_n,
    wen,
    wfull,
    walmost_full,
 
    mem_wen,
    mem_waddr,
 
    rclk,
    rrst_n,
    ren,
    rempty,
    ralmost_empty,
 
    mem_ren,
    mem_raddr
);
 
//---
// Parameters
 
parameter AWIDTH = 3;
parameter RAM_DEPTH = (1 << AWIDTH);
parameter EARLY_READ = 0;
parameter CLOCK_CROSSING = 1;
parameter ALMOST_EMPTY_THRESH = 1;
parameter ALMOST_FULL_THRESH = RAM_DEPTH-2;
 
//---
// Ports
 
input              wclk;
input              wrst_n;
input              wen;
output             wfull;
output             walmost_full;
 
output             mem_wen;
output [AWIDTH:0]  mem_waddr;
 
input              rclk;
input              rrst_n;
input              ren;
output             rempty;
output             ralmost_empty;
 
output             mem_ren;
output [AWIDTH:0]  mem_raddr;
 
 
 
//---
// Local declarations
 
// Registers
 
reg  [AWIDTH:0]   wr_ptr;
reg  [AWIDTH:0]   rd_ptr;
reg  [AWIDTH:0]   next_rd_ptr;
 
// Combinatorial
 
wire [AWIDTH:0]   wr_gray;
reg  [AWIDTH:0]   wr_gray_reg;
reg  [AWIDTH:0]   wr_gray_meta;
reg  [AWIDTH:0]   wr_gray_sync;
reg  [AWIDTH:0]   wck_rd_ptr;
wire [AWIDTH:0]   wck_level;
 
wire [AWIDTH:0]   rd_gray;
reg  [AWIDTH:0]   rd_gray_reg;
reg  [AWIDTH:0]   rd_gray_meta;
reg  [AWIDTH:0]   rd_gray_sync;
reg  [AWIDTH:0]   rck_wr_ptr;
wire [AWIDTH:0]   rck_level;
 
wire [AWIDTH:0]   depth;
wire [AWIDTH:0]   empty_thresh;
wire [AWIDTH:0]   full_thresh;
 
// Variables
 
integer         i;
 
//---
// Assignments
 
assign depth = RAM_DEPTH[AWIDTH:0];
assign empty_thresh = ALMOST_EMPTY_THRESH[AWIDTH:0];
assign full_thresh = ALMOST_FULL_THRESH[AWIDTH:0];
 
assign wfull = (wck_level == depth);
assign walmost_full = (wck_level >= (depth - full_thresh));
assign rempty = (rck_level == 0);
assign ralmost_empty = (rck_level <= empty_thresh);
 
//---
// Write Pointer
 
always @(posedge wclk or negedge wrst_n)
begin
    if (!wrst_n) begin
        wr_ptr <= {(AWIDTH+1){1'b0}};
    end
    else if (wen && !wfull) begin
        wr_ptr <= wr_ptr + {{(AWIDTH){1'b0}}, 1'b1};
    end
end
 
//---
// Read Pointer
 
always @(ren, rd_ptr, rck_wr_ptr)
begin
    next_rd_ptr = rd_ptr;
    if (ren && rd_ptr != rck_wr_ptr) begin
        next_rd_ptr = rd_ptr + {{(AWIDTH){1'b0}}, 1'b1};
    end
end
 
always @(posedge rclk or negedge rrst_n)
begin
    if (!rrst_n) begin
        rd_ptr <= {(AWIDTH+1){1'b0}};
    end
    else begin
        rd_ptr <= next_rd_ptr;
    end
end
 
//---
// Binary to Gray conversion
 
assign wr_gray = wr_ptr ^ (wr_ptr >> 1);
assign rd_gray = rd_ptr ^ (rd_ptr >> 1);
 
//---
// Gray to Binary conversion
 
always @(wr_gray_sync)
begin
    rck_wr_ptr[AWIDTH] = wr_gray_sync[AWIDTH];
    for (i = 0; i < AWIDTH; i = i + 1) begin
        rck_wr_ptr[AWIDTH-i-1] = rck_wr_ptr[AWIDTH-i] ^ wr_gray_sync[AWIDTH-i-1];
    end
end
 
always @(rd_gray_sync)
begin
    wck_rd_ptr[AWIDTH] = rd_gray_sync[AWIDTH];
    for (i = 0; i < AWIDTH; i = i + 1) begin
        wck_rd_ptr[AWIDTH-i-1] = wck_rd_ptr[AWIDTH-i] ^ rd_gray_sync[AWIDTH-i-1];
    end
end
 
//---
// Clock-Domain Crossing
 
generate
    if (CLOCK_CROSSING) begin
 
        // Instantiate metastability flops
        always @(posedge rclk or negedge rrst_n)
        begin
            if (!rrst_n) begin
                rd_gray_reg <= {(AWIDTH+1){1'b0}};
                wr_gray_meta <= {(AWIDTH+1){1'b0}};
                wr_gray_sync <= {(AWIDTH+1){1'b0}};
            end
            else begin
                rd_gray_reg <= rd_gray;
                wr_gray_meta <= wr_gray_reg;
                wr_gray_sync <= wr_gray_meta;
            end
        end
 
        always @(posedge wclk or negedge wrst_n)
        begin
            if (!wrst_n) begin
                wr_gray_reg <= {(AWIDTH+1){1'b0}};
                rd_gray_meta <= {(AWIDTH+1){1'b0}};
                rd_gray_sync <= {(AWIDTH+1){1'b0}};
            end
            else begin
                wr_gray_reg <= wr_gray;
                rd_gray_meta <= rd_gray_reg;
                rd_gray_sync <= rd_gray_meta;
            end
        end
    end
    else begin
 
        // No clock domain crossing
        always @(wr_gray or rd_gray)
        begin
            wr_gray_sync = wr_gray;
            rd_gray_sync = rd_gray;
        end
    end
endgenerate
 
//---
// FIFO Level
 
assign wck_level = wr_ptr - wck_rd_ptr;
assign rck_level = rck_wr_ptr - rd_ptr;
 
//---
// Memory controls
 
assign  mem_waddr = wr_ptr;
assign  mem_wen = wen && !wfull;
 
generate
    if (EARLY_READ) begin
 
        // With early read, data will be present at output
        // before ren is asserted. Usufull if we want to add
        // an output register and not add latency.
        assign mem_raddr = next_rd_ptr;
        assign mem_ren = 1'b1;
 
    end
    else begin
 
        assign mem_raddr = rd_ptr;
        assign mem_ren = ren;
 
    end
endgenerate
 
endmodule
