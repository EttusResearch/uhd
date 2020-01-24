
// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Latency must be 3 or 4

module axi_pipe_mac
  #(parameter LATENCY=3,
    parameter CASCADE_IN=0)
   (input clk, input reset, input clear,
    input a_tlast, input a_tvalid, output a_tready,
    input b_tlast, input b_tvalid, output b_tready,
    input c_tlast, input c_tvalid, output c_tready,
    output p_tlast, output p_tvalid, input p_tready,
    output [LATENCY-3:0] enables_a,
    output [LATENCY-3:0] enables_b,
    output enable_c,
    output enable_m,
    output enable_p);
   
   wire 			  join_tlast, join_tvalid, join_tready;
   wire 			  join1_tlast, join1_tvalid, join1_tready;
   wire 			  int0_tlast, int0_tvalid, int0_tready;
   wire 			  int1_tlast, int1_tvalid, int1_tready;
   wire 			  int2_tlast, int2_tvalid, int2_tready;
   wire 			  int3_tlast, int3_tvalid, int3_tready;
   
   axi_pipe #(.STAGES(LATENCY-2)) pipe_a
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(a_tlast), .i_tvalid(a_tvalid), .i_tready(a_tready),
      .o_tlast(int0_tlast), .o_tvalid(int0_tvalid), .o_tready(int0_tready),
      .enables(enables_a), .valids());
   
   axi_pipe #(.STAGES(LATENCY-2)) pipe_b
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(b_tlast), .i_tvalid(b_tvalid), .i_tready(b_tready),
      .o_tlast(int1_tlast), .o_tvalid(int1_tvalid), .o_tready(int1_tready),
      .enables(enables_b), .valids());
   
   axi_join #(.INPUTS(2)) join_ab
     (.i_tlast({int1_tlast,int0_tlast}), .i_tvalid({int1_tvalid,int0_tvalid}), .i_tready({int1_tready,int0_tready}),
      .o_tlast(join_tlast), .o_tvalid(join_tvalid), .o_tready(join_tready));
   
   axi_pipe #(.STAGES(1)) pipe_m
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(join_tlast), .i_tvalid(join_tvalid), .i_tready(join_tready),
      .o_tlast(int2_tlast), .o_tvalid(int2_tvalid), .o_tready(int2_tready),
      .enables(enable_m), .valids());

   // If we use the cascade input, there is no flop in the input side adder
   generate
      if(CASCADE_IN)
	begin
	   assign int3_tlast = c_tlast;
	   assign int3_tvalid = c_tvalid;
	   assign c_tready = int3_tready;
	   assign enable_c = 1'b0;
	end
      else
	axi_pipe #(.STAGES(1)) pipe_c
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tlast(c_tlast), .i_tvalid(c_tvalid), .i_tready(c_tready),
	   .o_tlast(int3_tlast), .o_tvalid(int3_tvalid), .o_tready(int3_tready),
	   .enables(enable_c), .valids());
   endgenerate
   
   axi_join #(.INPUTS(2)) joiner_mc
     (.i_tlast({int2_tlast,int3_tlast}), .i_tvalid({int2_tvalid,int3_tvalid}), .i_tready({int2_tready,int3_tready}),
      .o_tlast(join1_tlast), .o_tvalid(join1_tvalid), .o_tready(join1_tready));
   
   axi_pipe #(.STAGES(1)) pipe_p
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tlast(join1_tlast), .i_tvalid(join1_tvalid), .i_tready(join1_tready),
      .o_tlast(p_tlast), .o_tvalid(p_tvalid), .o_tready(p_tready),
      .enables(enable_p), .valids());
   
endmodule // axi_pipe_mac
