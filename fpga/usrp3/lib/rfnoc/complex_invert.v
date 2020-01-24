//
// Copyright 2015 Ettus Research LLC
//
// General complex invert algorithm:
//   1        1      a - bi     a - bi          a          bi
// ------ = ------ * ------ = ----------- = --------- - ---------
// a + bi   a + bi   a - bi   a^2 + b^2     a^2 + b^2   a^2 + b^2
//

module complex_invert
(
  input clk, input reset, input clear,
  input  [31:0] i_tdata, input  i_tlast, input  i_tvalid, output i_tready,
  output [31:0] o_tdata, output o_tlast, output o_tvalid, input  o_tready);

  wire [15:0] a_tdata;
  wire [31:0] a_tdata_int;
  wire        a_tlast;
  wire        a_tvalid;
  wire        a_tready;
  wire [15:0] b_tdata;
  wire [31:0] b_tdata_int;
  wire        b_tlast;
  wire        b_tvalid;
  wire        b_tready;
  wire [31:0] a_b_tdata;
  wire        a_b_tlast;
  wire        a_b_tvalid;
  wire        a_b_tready;

  // Replicate input data into three streams with FIFOing to account for varying latency on the paths
  split_stream_fifo #(
    .WIDTH(32),
    .ACTIVE_MASK(4'b0111),
    .FIFO_SIZE(5))
  input_split_stream_fifo0 (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o0_tdata(a_tdata_int), .o0_tlast(a_tlast), .o0_tvalid(a_tvalid), .o0_tready(a_tready),
    .o1_tdata(b_tdata_int), .o1_tlast(b_tlast), .o1_tvalid(b_tvalid), .o1_tready(b_tready),
    .o2_tdata(a_b_tdata), .o2_tlast(a_b_tlast), .o2_tvalid(a_b_tvalid), .o2_tready(a_b_tready),
    .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b0));

  assign a_tdata = a_tdata_int[31:16];
  assign b_tdata = b_tdata_int[15:0];

  wire [31:0] a2_plus_b2_tdata;
  wire        a2_plus_b2_tlast;
  wire        a2_plus_b2_tvalid;
  wire        a2_plus_b2_tready;

  // a^2 + b^2
  complex_to_magsq
  a2_p_b2_complex_to_magsq (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(a_b_tdata), .i_tlast(a_b_tlast), .i_tvalid(a_b_tvalid), .i_tready(a_b_tready),
    .o_tdata(a2_plus_b2_tdata), .o_tlast(a2_plus_b2_tlast), .o_tvalid(a2_plus_b2_tvalid), .o_tready(a2_plus_b2_tready));

  wire [31:0] a2_plus_b2_0_tdata;
  wire        a2_plus_b2_0_tlast;
  wire        a2_plus_b2_0_tvalid;
  wire        a2_plus_b2_0_tready;
  wire [31:0] a2_plus_b2_1_tdata;
  wire        a2_plus_b2_1_tlast;
  wire        a2_plus_b2_1_tvalid;
  wire        a2_plus_b2_1_tready;

  // Replicate two a^2 + b^2 streams for dividers
  split_stream_fifo #(
    .WIDTH(32),
    .ACTIVE_MASK(4'b0011),
    .FIFO_SIZE(5))
  input_split_stream_fifo1 (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(a2_plus_b2_tdata), .i_tlast(a2_plus_b2_tlast), .i_tvalid(a2_plus_b2_tvalid), .i_tready(a2_plus_b2_tready),
    .o0_tdata(a2_plus_b2_0_tdata), .o0_tlast(a2_plus_b2_0_tlast), .o0_tvalid(a2_plus_b2_0_tvalid), .o0_tready(a2_plus_b2_0_tready),
    .o1_tdata(a2_plus_b2_1_tdata), .o1_tlast(a2_plus_b2_1_tlast), .o1_tvalid(a2_plus_b2_1_tvalid), .o1_tready(a2_plus_b2_1_tready),
    .o2_tdata(), .o2_tlast(), .o2_tvalid(), .o2_tready(1'b0),
    .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b0));

  wire        div_by_zero_a;
  wire [47:0] a_div_a2_plus_b2_tdata_int; // signed bit, 15 integer bits, fraction sign bit, 31 fraction
  wire [47:0] a_div_a2_plus_b2_tdata = div_by_zero_a ? 48'd0 : a_div_a2_plus_b2_tdata_int;
  wire        a_div_a2_plus_b2_tlast;
  wire        a_div_a2_plus_b2_tvalid;
  wire        a_div_a2_plus_b2_tready;

  //     a
  // ---------
  // a^2 + b^2
  // Warning: Divider does not sign extend fractional part into the integer part, although we throw away the integer
  //          part so this issue does not affect our design.
  divide_int16_int32
  a_div_a2_plus_b2_divider (
    .aclk(clk), .aresetn(~reset),
    .s_axis_divisor_tdata(a2_plus_b2_0_tdata), .s_axis_divisor_tlast(a2_plus_b2_0_tlast), .s_axis_divisor_tvalid(a2_plus_b2_0_tvalid), .s_axis_divisor_tready(a2_plus_b2_0_tready),
    .s_axis_dividend_tdata(a_tdata), .s_axis_dividend_tlast(a_tlast), .s_axis_dividend_tvalid(a_tvalid), .s_axis_dividend_tready(a_tready),
    .m_axis_dout_tdata(a_div_a2_plus_b2_tdata_int), .m_axis_dout_tlast(a_div_a2_plus_b2_tlast), .m_axis_dout_tvalid(a_div_a2_plus_b2_tvalid), .m_axis_dout_tready(a_div_a2_plus_b2_tready),
    .m_axis_dout_tuser(div_by_zero_a));

  wire [15:0] neg_b_tdata;
  wire        neg_b_tlast;
  wire        neg_b_tvalid;
  wire        neg_b_tready;
  wire [15:0] neg_b = (b_tdata == -16'sd32768) ? 16'sd32767 : (~b_tdata + 1'b1);

  // Negate b
  axi_fifo_flop #(.WIDTH(17))
  neg_b_axi_fifo_flop (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({b_tlast,neg_b}), .i_tvalid(b_tvalid), .i_tready(b_tready),
    .o_tdata({neg_b_tlast,neg_b_tdata}), .o_tvalid(neg_b_tvalid), .o_tready(neg_b_tready),
    .space(), .occupied());

  wire        div_by_zero_b;
  wire [47:0] neg_b_div_a2_plus_b2_tdata_int;
  wire [47:0] neg_b_div_a2_plus_b2_tdata = div_by_zero_b ? 48'd0 : neg_b_div_a2_plus_b2_tdata_int;
  wire        neg_b_div_a2_plus_b2_tlast;
  wire        neg_b_div_a2_plus_b2_tvalid;
  wire        neg_b_div_a2_plus_b2_tready;

  //     bi
  // ---------
  // a^2 + b^2
  divide_int16_int32
  neg_b_div_a2_plus_b2_divider (
    .aclk(clk), .aresetn(~reset),
    .s_axis_divisor_tdata(a2_plus_b2_1_tdata), .s_axis_divisor_tlast(a2_plus_b2_1_tlast), .s_axis_divisor_tvalid(a2_plus_b2_1_tvalid), .s_axis_divisor_tready(a2_plus_b2_1_tready),
    .s_axis_dividend_tdata(neg_b_tdata), .s_axis_dividend_tlast(neg_b_tlast), .s_axis_dividend_tvalid(neg_b_tvalid), .s_axis_dividend_tready(neg_b_tready),
    .m_axis_dout_tdata(neg_b_div_a2_plus_b2_tdata_int), .m_axis_dout_tlast(neg_b_div_a2_plus_b2_tlast), .m_axis_dout_tvalid(neg_b_div_a2_plus_b2_tvalid), .m_axis_dout_tready(neg_b_div_a2_plus_b2_tready),
    .m_axis_dout_tuser(div_by_zero_b));

  // Throw away integer part as the result will always be a fraction due to a^2 + b^2 > a (or b)
  wire [63:0] one_div_a_plus_bi_tdata = {a_div_a2_plus_b2_tdata[31:0],neg_b_div_a2_plus_b2_tdata[31:0]};
  wire        one_div_a_plus_bi_tlast;
  wire        one_div_a_plus_bi_tvalid;
  wire        one_div_a_plus_bi_tready;

  // Join into one word
  axi_join #(
    .INPUTS(2))
  inst_axi_join (
   .i_tlast({a_div_a2_plus_b2_tlast,neg_b_div_a2_plus_b2_tlast}), .i_tvalid({a_div_a2_plus_b2_tvalid,neg_b_div_a2_plus_b2_tvalid}), .i_tready({a_div_a2_plus_b2_tready,neg_b_div_a2_plus_b2_tready}),
   .o_tlast(one_div_a_plus_bi_tlast), .o_tvalid(one_div_a_plus_bi_tvalid), .o_tready(one_div_a_plus_bi_tready));

  // Truncate to a complex int16
  axi_round_and_clip_complex #(
    .WIDTH_IN(32),
    .WIDTH_OUT(16),
    .CLIP_BITS(11), // Calibrated value
    .FIFOSIZE())
  inst_axi_round_and_clip_complex (
    .clk(clk), .reset(reset),
    .i_tdata(one_div_a_plus_bi_tdata), .i_tlast(one_div_a_plus_bi_tlast), .i_tvalid(one_div_a_plus_bi_tvalid), .i_tready(one_div_a_plus_bi_tready),
    .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

endmodule
