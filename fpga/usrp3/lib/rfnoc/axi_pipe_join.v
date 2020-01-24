
// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later


module axi_pipe_join
  #(parameter PRE_JOIN_STAGES0=3,
    parameter PRE_JOIN_STAGES1=3,
    parameter POST_JOIN_STAGES=3)
   (input clk, input reset, input clear,
    input i0_tlast, input i0_tvalid, output i0_tready,
    input i1_tlast, input i1_tvalid, output i1_tready,
    output o_tlast, output o_tvalid, input o_tready,
    output [PRE_JOIN_STAGES0-1:0] enables0,
    output [PRE_JOIN_STAGES1-1:0] enables1,
    output [POST_JOIN_STAGES-1:0] enables_post);

   wire 			  join_tlast, join_tvalid, join_tready;
   wire 			  int0_tlast, int0_tvalid, int0_tready;
   wire 			  int1_tlast, int1_tvalid, int1_tready;
   
   axi_pipe #(.STAGES(PRE_JOIN_STAGES0)) pipe_pre_0
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(i0_tlast), .i_tvalid(i0_tvalid), .i_tready(i0_tready),
      .o_tlast(int0_tlast), .o_tvalid(int0_tvalid), .o_tready(int0_tready),
      .enables(enables0), .valids());
   
   axi_pipe #(.STAGES(PRE_JOIN_STAGES1)) pipe_pre_1
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(i1_tlast), .i_tvalid(i1_tvalid), .i_tready(i1_tready),
      .o_tlast(int1_tlast), .o_tvalid(int1_tvalid), .o_tready(int1_tready),
      .enables(enables1), .valids());
   
   axi_pipe #(.STAGES(POST_JOIN_STAGES)) pipe_post
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(join_tlast), .i_tvalid(join_tvalid), .i_tready(join_tready),
      .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .enables(enables_post), .valids());
   
   axi_join #(.INPUTS(2)) joiner
     (.i_tlast({int1_tlast,int0_tlast}), .i_tvalid({int1_tvalid,int0_tvalid}), .i_tready({int1_tready,int0_tready}),
      .o_tlast(join_tlast), .o_tvalid(join_tvalid), .o_tready(join_tready));
   
endmodule // axi_pipe_join
