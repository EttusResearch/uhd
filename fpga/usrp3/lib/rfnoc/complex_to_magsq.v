//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module complex_to_magsq #(
  parameter WIDTH = 16)
(
  input clk, input reset, input clear,
  input [2*WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [2*WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire [WIDTH-1:0]    ii_tdata, iq_tdata;
   wire 	       ii_tlast, ii_tvalid, ii_tready, iq_tlast, iq_tvalid, iq_tready;

   wire [2*WIDTH-1:0]    i_sq_tdata;
   wire 	       i_sq_tlast, i_sq_tvalid, i_sq_tready;

   split_complex #(.WIDTH(WIDTH)) split_complex
     (.i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .oi_tdata(ii_tdata), .oi_tlast(ii_tlast), .oi_tvalid(ii_tvalid), .oi_tready(ii_tready), 
      .oq_tdata(iq_tdata), .oq_tlast(iq_tlast), .oq_tvalid(iq_tvalid), .oq_tready(iq_tready),
      .error());
      
  // i^2
  mult #(
   .WIDTH_A(WIDTH),
   .WIDTH_B(WIDTH),
   .WIDTH_P(2*WIDTH),
   .DROP_TOP_P(5),
   .LATENCY(2),       // NOTE: If using CASCADE_OUT, set to 3
   .CASCADE_OUT(0))   // FIXME can use cascade once we get ISE to accept it
  i_sq_mult (
    .clk(clk), .reset(reset),
    .a_tdata(ii_tdata), .a_tlast(ii_tlast), .a_tvalid(ii_tvalid), .a_tready(ii_tready),
    .b_tdata(ii_tdata), .b_tlast(ii_tlast), .b_tvalid(ii_tvalid), .b_tready(),
    .p_tdata(i_sq_tdata), .p_tlast(i_sq_tlast), .p_tvalid(i_sq_tvalid), .p_tready(i_sq_tready));

  // q^2 + i^2
  mult_add #(
   .WIDTH_A(WIDTH),
   .WIDTH_B(WIDTH),
   .WIDTH_P(2*WIDTH),
   .DROP_TOP_P(5),
   .LATENCY(4),
   .CASCADE_IN(0),   // FIXME this can be 1 once we get ISE to accept cascading
   .CASCADE_OUT(0))
  q_sq_mult (
    .clk(clk), .reset(reset),
    .a_tdata(iq_tdata), .a_tlast(iq_tlast), .a_tvalid(iq_tvalid), .a_tready(iq_tready),
    .b_tdata(iq_tdata), .b_tlast(iq_tlast), .b_tvalid(iq_tvalid), .b_tready(),
    .c_tdata(i_sq_tdata), .c_tlast(i_sq_tlast), .c_tvalid(i_sq_tvalid), .c_tready(i_sq_tready),
    .p_tdata(o_tdata), .p_tlast(o_tlast), .p_tvalid(o_tvalid), .p_tready(o_tready));

endmodule