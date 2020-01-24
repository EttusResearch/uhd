//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


//
// Single FIFO (register) with AXI4-STREAM interface
//

module axi_fifo_flop
  #(parameter WIDTH=32)
   (input clk,
    input reset,
    input clear,
    input [WIDTH-1:0] i_tdata,
    input i_tvalid,
    output i_tready,
    output reg [WIDTH-1:0] o_tdata = 'd0,
    output reg o_tvalid = 1'b0,
    input o_tready,
    output space,
    output occupied);

   assign i_tready = ~reset & (~o_tvalid | o_tready);

   always @(posedge clk)
     if(reset | clear)
       o_tvalid <= 1'b0;
     else
       o_tvalid <= (i_tready & i_tvalid) | (o_tvalid & ~o_tready);

   always @(posedge clk)
     if(i_tvalid & i_tready)
       o_tdata <= i_tdata;

   // These aren't terribly useful, but include them for consistency
   assign space = i_tready;
   assign occupied = o_tvalid;

endmodule // axi_fifo_flop
