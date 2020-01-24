//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Fast magnitude approximation.
// 
// ALPHA_DENOM & BETA_DENOM should be a power of 2
// Multiplierless if ALPHA_NUM & BETA_NUM are 1
//
// Mag ~= Alpha * max(|I|, |Q|) + Beta * min(|I|, |Q|)
//
// (table taken from http://www.dspguru.com/dsp/tricks/magnitude-estimator)
// =========================================
// Alpha  Beta       Avg Err   RMS   Peak
//                   (linear)  (dB)  (dB)
// -----------------------------------------
// 1,     1/2        -0.086775 -20.7 -18.6
// 1,     1/4         0.006456 -27.6 -18.7
// 1,     11/32      -0.028505 -28.0 -24.8
// 1,     3/8        -0.040159 -26.4 -23.4
// 15/16, 15/32      -0.018851 -29.2 -24.1
// 15/16, 1/2        -0.030505 -26.9 -24.1
// 31/32, 11/32      -0.000371 -31.6 -22.9
// 31/32, 3/8        -0.012024 -31.4 -26.1
// 61/64, 3/8         0.002043 -32.5 -24.3
// 61/64, 13/32       0.009611 -31.8 -26.6
// =========================================
//
// Input: Complex, Output: Unsigned Int

`ifndef LOG2
`define LOG2(N) ( \
   N < 2    ? 0 : \
   N < 4    ? 1 : \
   N < 8    ? 2 : \
   N < 16   ? 3 : \
   N < 32   ? 4 : \
   N < 64   ? 5 : \
   N < 128  ? 6 : \
   N < 256  ? 7 : \
   N < 512  ? 8 : \
   N < 1024 ? 9 : \
              10)
`endif

module complex_to_mag_approx #(
  parameter ALPHA_NUM = 1,
  parameter ALPHA_DENOM = 1,
  parameter BETA_NUM = 1,
  parameter BETA_DENOM = 4,
  parameter LATENCY = 3, // 0, 1, 2, or 3
  parameter SAMP_WIDTH = 16)
(
  input clk, input reset, input clear,
  input [2*SAMP_WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [SAMP_WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire [2*SAMP_WIDTH-1:0] pipeline_i_tdata[0:2], pipeline_o_tdata[0:2];
  wire [2:0] pipeline_i_tvalid, pipeline_i_tlast, pipeline_i_tready;
  wire [2:0] pipeline_o_tvalid, pipeline_o_tlast, pipeline_o_tready;
  wire signed [SAMP_WIDTH-1:0] i, q, max, max_int, min, min_int;
  wire [SAMP_WIDTH-1:0] i_abs, q_abs, i_abs_int, q_abs_int, mag;


  // Absolute value
  assign i = i_tdata[2*SAMP_WIDTH-1:SAMP_WIDTH];
  assign q = i_tdata[SAMP_WIDTH-1:0];
  assign i_abs_int = i[SAMP_WIDTH-1] ? (~i + 1'b1) : i;
  assign q_abs_int = q[SAMP_WIDTH-1] ? (~q + 1'b1) : q;


  // First stage pipeline
  assign pipeline_i_tdata[0]  = {i_abs_int,q_abs_int};
  assign pipeline_i_tlast[0]  = i_tlast;
  assign pipeline_i_tvalid[0] = i_tvalid;
  assign pipeline_o_tready[0] = pipeline_i_tready[1];

  axi_fifo_flop #(.WIDTH(SAMP_WIDTH*2+1))
  pipeline0_axi_fifo_flop (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({pipeline_i_tlast[0],pipeline_i_tdata[0]}), .i_tvalid(pipeline_i_tvalid[0]), .i_tready(pipeline_i_tready[0]),
    .o_tdata({pipeline_o_tlast[0],pipeline_o_tdata[0]}), .o_tvalid(pipeline_o_tvalid[0]), .o_tready(pipeline_o_tready[0]));


  // Max & Min
  assign i_abs = (LATENCY == 3) ? pipeline_o_tdata[0][2*SAMP_WIDTH-1:SAMP_WIDTH] : i_abs;
  assign q_abs = (LATENCY == 3) ? pipeline_o_tdata[0][SAMP_WIDTH-1:0]            : q_abs;
  assign max_int = (i_abs > q_abs) ? i_abs : q_abs;
  assign min_int = (i_abs > q_abs) ? q_abs : i_abs;


  // Second stage pipeline
  assign pipeline_i_tdata[1]  = {max_int,min_int};
  assign pipeline_i_tlast[1]  = (LATENCY == 2) ? i_tlast              : pipeline_o_tlast[0];
  assign pipeline_i_tvalid[1] = (LATENCY == 2) ? i_tvalid             : pipeline_o_tvalid[0];
  assign pipeline_o_tready[1] = pipeline_i_tready[2];

  axi_fifo_flop #(.WIDTH(SAMP_WIDTH*2+1))
  pipeline1_axi_fifo_flop (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({pipeline_i_tlast[1],pipeline_i_tdata[1]}), .i_tvalid(pipeline_i_tvalid[1]), .i_tready(pipeline_i_tready[1]),
    .o_tdata({pipeline_o_tlast[1],pipeline_o_tdata[1]}), .o_tvalid(pipeline_o_tvalid[1]), .o_tready(pipeline_o_tready[1]));


  // Magnitude Approx
  assign max = (LATENCY >= 2) ? pipeline_o_tdata[1][2*SAMP_WIDTH-1:SAMP_WIDTH] : max_int;
  assign min = (LATENCY >= 2) ? pipeline_o_tdata[1][SAMP_WIDTH-1:0]            : min_int;
  assign mag = ALPHA_NUM * {{`LOG2(ALPHA_DENOM){1'b0}},max[SAMP_WIDTH-1:`LOG2(ALPHA_DENOM)]} +
                BETA_NUM * {{`LOG2( BETA_DENOM){1'b0}},min[SAMP_WIDTH-1:`LOG2( BETA_DENOM)]};


  // Third stage pipeline
  assign pipeline_i_tdata[2][SAMP_WIDTH-1:0] = mag;
  assign pipeline_i_tlast[2]  = (LATENCY == 1) ? i_tlast  : pipeline_o_tlast[1];
  assign pipeline_i_tvalid[2] = (LATENCY == 1) ? i_tvalid : pipeline_o_tvalid[1];
  assign pipeline_o_tready[2] = o_tready;

  axi_fifo_flop #(.WIDTH(SAMP_WIDTH+1))
  pipeline2_axi_fifo_flop (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({pipeline_i_tlast[2],pipeline_i_tdata[2][SAMP_WIDTH-1:0]}), .i_tvalid(pipeline_i_tvalid[2]), .i_tready(pipeline_i_tready[2]),
    .o_tdata({pipeline_o_tlast[2],pipeline_o_tdata[2][SAMP_WIDTH-1:0]}), .o_tvalid(pipeline_o_tvalid[2]), .o_tready(pipeline_o_tready[2]));


  // Output based on LATENCY mux
  assign o_tdata  = (LATENCY == 0) ? mag      : pipeline_o_tdata[2][SAMP_WIDTH-1:0];
  assign o_tlast  = (LATENCY == 0) ? i_tlast  : pipeline_o_tlast[2];
  assign o_tvalid = (LATENCY == 0) ? i_tvalid : pipeline_o_tvalid[2];
  assign i_tready = (LATENCY == 0) ? o_tready : 
                    (LATENCY == 1) ? pipeline_i_tready[2] :
                    (LATENCY == 2) ? pipeline_i_tready[1] : pipeline_i_tready[0];

endmodule