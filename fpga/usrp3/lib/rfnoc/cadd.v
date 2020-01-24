//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Complex adder

module cadd
  #(parameter WIDTH=16)
   (input clk, input reset,
    input [WIDTH*2-1:0] a_tdata, input a_tlast, input a_tvalid, output a_tready,
    input [WIDTH*2-1:0] b_tdata, input b_tlast, input b_tvalid, output b_tready,
    output [WIDTH*2-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire int_tlast = a_tlast | b_tlast;
   wire int_tvalid, int_tready;
   wire [WIDTH*2-1:0] int_tdata;

   assign int_tdata[WIDTH*2-1:WIDTH] = a_tdata[WIDTH*2-1:WIDTH] + b_tdata[WIDTH*2-1:WIDTH];
   assign int_tdata[WIDTH-1:0] = a_tdata[WIDTH-1:0] + b_tdata[WIDTH-1:0];

   assign int_tvalid = a_tvalid & b_tvalid;
   assign a_tready = int_tvalid & int_tready;
   assign b_tready = a_tready;
   
   axi_fifo #(.WIDTH(WIDTH*2+1), .SIZE(0)) flop_output
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({int_tlast, int_tdata}), .i_tvalid(int_tvalid), .i_tready(int_tready),
      .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready));
   
endmodule // cadd
