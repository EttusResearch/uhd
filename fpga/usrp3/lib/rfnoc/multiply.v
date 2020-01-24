//
// Copyright 2015 Ettus Research
//
// AXI Stream multiplier. Relies on synthesis engine for proper DSP inference.

module multiply #(
  parameter WIDTH_A     = 16,
  parameter WIDTH_B     = 16,
  parameter WIDTH_P     = 32,
  parameter DROP_TOP_P  = 1,  // Default drops extra bit (16-bit signed x 16-bit signed => 31-bits signed)
  parameter LATENCY     = 3,  // multiplier pipeline latency, 0 - 4
  parameter EN_SATURATE = 0,  // Enable saturating output to avoid overflow (adds +1 to latency)
  parameter EN_ROUND    = 0,  // Enable rounding dropped LSBs (adds +1 to latency, total of +2 if used with EN_SATURATE)
  parameter SIGNED      = 1)  // Signed multiply
(
  input clk, input reset,
  input [WIDTH_A-1:0] a_tdata, input a_tlast, input a_tvalid, output a_tready,
  input [WIDTH_B-1:0] b_tdata, input b_tlast, input b_tvalid, output b_tready,
  output [WIDTH_P-1:0] p_tdata, output p_tlast, output p_tvalid, input p_tready
);

  localparam A_LATENCY = (LATENCY == 1) ? 1 :
                         (LATENCY == 2) ? 1 :
                         (LATENCY == 3) ? 2 :
                         (LATENCY == 4) ? 2 : 2;
  localparam B_LATENCY = A_LATENCY;
  localparam P_LATENCY = (LATENCY == 2) ? 1 :
                         (LATENCY == 3) ? 1 :
                         (LATENCY == 4) ? 2 : 2;

  reg [WIDTH_A-1:0]         a_reg[A_LATENCY-1:0];
  reg [WIDTH_B-1:0]         b_reg[B_LATENCY-1:0];
  reg [WIDTH_A+WIDTH_B-1:0] p_reg[P_LATENCY-1:0];

  wire [A_LATENCY-1:0] en_a_reg;
  wire [B_LATENCY-1:0] en_b_reg;
  wire [P_LATENCY-1:0] en_p_reg;
  wire p_int_tlast, p_int_tvalid, p_int_tready;
  axi_pipe_join #(
    .PRE_JOIN_STAGES0(A_LATENCY),
    .PRE_JOIN_STAGES1(B_LATENCY),
    .POST_JOIN_STAGES(P_LATENCY))
  axi_pipe_join (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i0_tlast(a_tlast), .i0_tvalid(a_tvalid), .i0_tready(a_tready),
    .i1_tlast(b_tlast), .i1_tvalid(b_tvalid), .i1_tready(b_tready),
    .o_tlast(p_int_tlast), .o_tvalid(p_int_tvalid), .o_tready(p_int_tready),
    .enables0(en_a_reg), .enables1(en_b_reg), .enables_post(en_p_reg));

  // Multiply
  wire [WIDTH_A+WIDTH_B-1:0] p_mult_signed   = (LATENCY == 0) ? $signed(a_tdata) * $signed(b_tdata) : $signed(a_reg[A_LATENCY-1]) * $signed(b_reg[B_LATENCY-1]);
  wire [WIDTH_A+WIDTH_B-1:0] p_mult_unsigned = (LATENCY == 0) ? a_tdata * b_tdata : a_reg[A_LATENCY-1] * b_reg[B_LATENCY-1];
  wire [WIDTH_A+WIDTH_B-1:0] p_int_tdata     = (LATENCY == 0) ? (SIGNED ? p_mult_signed : p_mult_unsigned) : p_reg[P_LATENCY-1];

  // Register pipeline
  integer i;
  always @(posedge clk) begin
    if (reset) begin
      for (i = 0; i < A_LATENCY; i = i + 1) begin
        a_reg[i] <= 'd0;
      end
      for (i = 0; i < B_LATENCY; i = i + 1) begin
        b_reg[i] <= 'd0;
      end
      for (i = 0; i < P_LATENCY; i = i + 1) begin
        p_reg[i] <= 'd0;
      end
    end else begin
      for (i = 0; i < A_LATENCY; i = i + 1) begin
        if (en_a_reg[i]) begin
          if (i == 0) begin
            a_reg[i] <= $signed(a_tdata);
          end else begin
            a_reg[i] <= a_reg[i-1];
          end
        end
      end
      for (i = 0; i < B_LATENCY; i = i + 1) begin
        if (en_b_reg[i]) begin
          if (i == 0) begin
            b_reg[i] <= $signed(b_tdata);
          end else begin
            b_reg[i] <= b_reg[i-1];
          end
        end
      end
      for (i = 0; i < P_LATENCY; i = i + 1) begin
        if (en_p_reg[i]) begin
          if (i == 0) begin
            p_reg[i] <= SIGNED ? p_mult_signed : p_mult_unsigned;
          end else begin
            p_reg[i] <= p_reg[i-1];
          end
        end
      end
    end
  end

  // Saturate & Round
  // TODO: Might be able to replace axi_round with DSP's built in rounding
  generate
    if ((EN_SATURATE == 1) && (EN_ROUND == 1)) begin
      axi_round_and_clip #(
        .WIDTH_IN(WIDTH_A+WIDTH_B),
        .WIDTH_OUT(WIDTH_P),
        .CLIP_BITS(DROP_TOP_P))
      axi_round_and_clip (
        .clk(clk), .reset(reset),
        .i_tdata(p_int_tdata), .i_tlast(p_int_tlast), .i_tvalid(p_int_tvalid), .i_tready(p_int_tready),
        .o_tdata(p_tdata), .o_tlast(p_tlast), .o_tvalid(p_tvalid), .o_tready(p_tready));
    end else if ((EN_SATURATE == 0) && (EN_ROUND == 1)) begin
      axi_round #(
        .WIDTH_IN(WIDTH_A+WIDTH_B-DROP_TOP_P),
        .WIDTH_OUT(WIDTH_P))
      axi_round (
        .clk(clk), .reset(reset),
        .i_tdata(p_int_tdata[WIDTH_A+WIDTH_B-DROP_TOP_P-1:0]), .i_tlast(p_int_tlast), .i_tvalid(p_int_tvalid), .i_tready(p_int_tready),
        .o_tdata(p_tdata), .o_tlast(p_tlast), .o_tvalid(p_tvalid), .o_tready(p_tready));
    end else if ((EN_SATURATE == 1) && (EN_ROUND == 0)) begin
      wire [WIDTH_A+WIDTH_B-DROP_TOP_P-1:0] p_clip_tdata;
      axi_clip #(
        .WIDTH_IN(WIDTH_A+WIDTH_B),
        .WIDTH_OUT(WIDTH_A+WIDTH_B-DROP_TOP_P),
        .CLIP_BITS(DROP_TOP_P))
      axi_clip (
        .clk(clk), .reset(reset),
        .i_tdata(p_int_tdata), .i_tlast(p_int_tlast), .i_tvalid(p_int_tvalid), .i_tready(p_int_tready),
        .o_tdata(p_clip_tdata), .o_tlast(p_tlast), .o_tvalid(p_tvalid), .o_tready(p_tready));
      assign p_tdata = p_clip_tdata[WIDTH_A+WIDTH_B-DROP_TOP_P-1:WIDTH_A+WIDTH_B-DROP_TOP_P-WIDTH_P];
    end else begin
      assign p_tdata      = p_int_tdata[WIDTH_A+WIDTH_B-DROP_TOP_P-1:WIDTH_A+WIDTH_B-DROP_TOP_P-WIDTH_P];
      assign p_tlast      = p_int_tlast;
      assign p_tvalid     = p_int_tvalid;
      assign p_int_tready = p_tready;
    end
  endgenerate

endmodule
