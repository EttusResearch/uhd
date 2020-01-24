//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module delay_type3
  #(parameter FIFOSIZE=5,
    parameter MAX_LEN_LOG2=10,
    parameter WIDTH=16,
    parameter DELAY_VAL=0)
   (input clk, input reset, input clear,
    input [MAX_LEN_LOG2-1:0] len,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire [WIDTH-1:0]    int_tdata;
   wire 	       int_tlast, int_tvalid, int_tready;
   
   axi_fifo #(.WIDTH(WIDTH+1), .SIZE(MAX_LEN_LOG2)) sample_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata({int_tlast,int_tdata}), .o_tvalid(int_tvalid), .o_tready(int_tready));

   delay_type2 #(.MAX_LEN_LOG2(MAX_LEN_LOG2), .WIDTH(WIDTH), .DELAY_VAL(DELAY_VAL)) delay
     (.clk(clk), .reset(reset), .clear(clear),
      .len(len),
      .i_tdata(int_tdata), .i_tlast(int_tlast), .i_tvalid(int_tvalid), .i_tready(int_tready),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

endmodule // delay_type3