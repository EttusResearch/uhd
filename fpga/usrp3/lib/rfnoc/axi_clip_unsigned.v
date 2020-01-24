//
// Copyright 2015, Ettus Research
//
// Reduces bit width by removing MSBs. Input assumed to be unsigned.

module axi_clip_unsigned
  #(parameter WIDTH_IN=24,
    parameter WIDTH_OUT=16,
    parameter FIFOSIZE=0)  // leave at 0 for a normal single flop
   (input clk, input reset,
    input [WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire overflow = |i_tdata[WIDTH_IN-1:WIDTH_OUT];

   wire [WIDTH_OUT-1:0] out = overflow ? {1'b0,{(WIDTH_OUT-1){1'b1}}} : i_tdata[WIDTH_OUT-1:0];

   axi_fifo #(.WIDTH(WIDTH_OUT+1), .SIZE(FIFOSIZE)) flop
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({i_tlast, out}), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .occupied(), .space());

endmodule // clip
