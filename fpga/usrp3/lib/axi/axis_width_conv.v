//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_width_conv
// Description: 
//   An AXI-Stream width conversion module that can convert from
//   an arbitrary input width to an arbitrary output width. The
//   module also supports an optional clock crossing. Data bits
//   are grouped into words which will be rearranged by this module.
//   The contents of a word are not rearranged.
//   Example (WORD_W=4, IN_WORDS=4, OUT_WORDS=6):
//    Input  : 3_1_2_0, x_6_5_4           (comma-delimited packets)
//    Output : 5_4_3_2_1_0, x_x_x_x_x_6   (comma-delimited packets)
//   NOTE: The use of tkeep in this module is a slight deviation from
//         the AXI standard where the bits are "byte qualifiers". In
//         this module, tkeep is a "word qualifier" where the width
//         of a word can be arbitrary. If WORD_W = 8, the behavior
//         of this module is identical to an AXI width converter. 
//
// Parameters:
//   - WORD_W: Bitwidth of a word
//   - IN_WORDS: Number of words in the input stream
//   - OUT_WORDS: Number of words in the output stream
//   - SYNC_CLKS: Are s_axis_aclk and m_axis_aclk synchronous to each other?
//   - PIPELINE: Which ports to pipeline? {NONE, IN, OUT, INOUT}
//
// Signals:
//   - s_axis_* : Input sample stream (AXI-Stream)
//   - m_axis_* : Output sample stream (AXI-Stream)

module axis_width_conv #(
  parameter WORD_W    = 8,
  parameter IN_WORDS  = 4,
  parameter OUT_WORDS = 6,
  parameter SYNC_CLKS = 0,
  parameter PIPELINE  = "NONE"
)(
  // Data In (AXI-Stream)                
  input  wire                          s_axis_aclk,    // Input stream Clock
  input  wire                          s_axis_rst,     // Input stream Reset
  input  wire [(IN_WORDS*WORD_W)-1:0]  s_axis_tdata,   // Input stream tdata
  input  wire [IN_WORDS-1:0]           s_axis_tkeep,   // Input stream tkeep
  input  wire                          s_axis_tlast,   // Input stream tlast
  input  wire                          s_axis_tvalid,  // Input stream tvalid
  output wire                          s_axis_tready,  // Input stream tready
  // Data Out (AXI-Stream)             
  input  wire                          m_axis_aclk,    // Output stream Clock
  input  wire                          m_axis_rst,     // Output stream Reset
  output wire [(OUT_WORDS*WORD_W)-1:0] m_axis_tdata,   // Output stream tdata
  output wire [OUT_WORDS-1:0]          m_axis_tkeep,   // Output stream tkeep
  output wire                          m_axis_tlast,   // Output stream tlast
  output wire                          m_axis_tvalid,  // Output stream tvalid
  input  wire                          m_axis_tready   // Output stream tready
);

  //----------------------------------------------
  // Pipeline Logic
  //----------------------------------------------
  // Add optional input and output pipeline stages

  wire [(IN_WORDS*WORD_W)-1:0]  i_tdata;
  wire [IN_WORDS-1:0]           i_tkeep;
  wire                          i_tlast, i_tvalid, i_tready;
  wire [(OUT_WORDS*WORD_W)-1:0] o_tdata;
  wire [OUT_WORDS-1:0]          o_tkeep;
  wire                          o_tlast, o_tvalid, o_tready;

  generate
    if (PIPELINE == "IN" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH((IN_WORDS*(WORD_W+1))+1)) in_pipe_i (
        .clk(s_axis_aclk), .reset(s_axis_rst), .clear(1'b0),
        .i_tdata({s_axis_tlast, s_axis_tkeep, s_axis_tdata}),
        .i_tvalid(s_axis_tvalid), .i_tready(s_axis_tready),
        .o_tdata({i_tlast, i_tkeep, i_tdata}),
        .o_tvalid(i_tvalid), .o_tready(i_tready),
        .space(), .occupied()
      );
    end else begin
      assign {i_tlast, i_tkeep, i_tdata} = {s_axis_tlast, s_axis_tkeep, s_axis_tdata};
      assign i_tvalid = s_axis_tvalid;
      assign s_axis_tready = i_tready;
    end

    if (PIPELINE == "OUT" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH((OUT_WORDS*(WORD_W+1))+1)) out_pipe_i (
        .clk(m_axis_aclk), .reset(m_axis_rst), .clear(1'b0),
        .i_tdata({o_tlast, o_tkeep, o_tdata}),
        .i_tvalid(o_tvalid), .i_tready(o_tready),
        .o_tdata({m_axis_tlast, m_axis_tkeep, m_axis_tdata}),
        .o_tvalid(m_axis_tvalid), .o_tready(m_axis_tready),
        .space(), .occupied()
      );
    end else begin
      assign {m_axis_tlast, m_axis_tkeep, m_axis_tdata} = {o_tlast, o_tkeep, o_tdata};
      assign m_axis_tvalid = o_tvalid;
      assign o_tready = m_axis_tready;
    end
  endgenerate

  //----------------------------------------------
  // Intermediate Data Bus
  //----------------------------------------------
  // To perform an M to N width conversion, we first
  // convert from M to LCM(M, N), then to N

  // Function to compute the least common multiple
  // of two numbers (parameters or localparams only)
  function integer lcm;
    input integer a;
    input integer b;
    integer x, y, swap;
    reg done;
  begin
    done = 1'b0;
    x = a;
    y = b;
    while (!done) begin
      if (x < y) begin
        swap = x; 
        x = y; 
        y = swap; 
      end else if (y != 0) begin
        x = x - y;
      end else begin
        done = 1'b1; 
      end
    end
    // x is the greatest common divisor
    // LCM = (a*b)/GCD
    lcm = (a*b)/x;
  end
  endfunction

  // Intermediate bus parameters
  localparam integer INT_KEEP_W     = lcm(IN_WORDS, OUT_WORDS);
  localparam integer INT_DATA_W     = INT_KEEP_W * WORD_W;
  localparam integer UPSIZE_RATIO   = INT_KEEP_W / IN_WORDS;
  localparam integer DOWNSIZE_RATIO = INT_KEEP_W / OUT_WORDS;

  wire [INT_DATA_W-1:0] fifo_i_tdata, fifo_o_tdata;
  wire [INT_KEEP_W-1:0] fifo_i_tkeep, fifo_o_tkeep;
  wire                  fifo_i_tlast, fifo_i_tvalid, fifo_i_tready;
  wire                  fifo_o_tlast, fifo_o_tvalid, fifo_o_tready;

  // Skip the intermediate FIFO if
  // - The input and output clocks are the same
  // - The upsizer is effectively a passthrough and input registering is requested
  // - The downsizer is effectively a passthrough and output registering is requested
  localparam [0:0] SKIP_FIFO = (SYNC_CLKS == 1) && (
      ((PIPELINE == "IN"  || PIPELINE == "INOUT") && (UPSIZE_RATIO == 1)) ||
      ((PIPELINE == "OUT" || PIPELINE == "INOUT") && (DOWNSIZE_RATIO == 1))
    );
  localparam FIFO_SIZE = 1;

  //----------------------------------------------
  // In => Upsizer => FIFO => Downsizer => Out
  //----------------------------------------------

  wire [INT_KEEP_W-1:0]     up_keep_flat;
  wire [UPSIZE_RATIO-1:0]   up_keep_keep;
  wire [DOWNSIZE_RATIO-1:0] down_keep_keep;

  axis_upsizer #(
    .IN_DATA_W(IN_WORDS*WORD_W), .IN_USER_W(IN_WORDS),
    .RATIO(UPSIZE_RATIO)
  ) upsizer_i (
    .clk(s_axis_aclk), .reset(s_axis_rst),
    .s_axis_tdata(i_tdata), .s_axis_tuser(i_tkeep),
    .s_axis_tlast(i_tlast), .s_axis_tvalid(i_tvalid), .s_axis_tready(i_tready),
    .m_axis_tdata(fifo_i_tdata), .m_axis_tuser(up_keep_flat), .m_axis_tkeep(up_keep_keep),
    .m_axis_tlast(fifo_i_tlast), .m_axis_tvalid(fifo_i_tvalid), .m_axis_tready(fifo_i_tready)
  );

  // tkeep unmasking logic after upsizer
  genvar i;
  generate for (i = 0; i < INT_KEEP_W; i = i + 1) begin
    // tkeep is assumed to be valid only when tlast is asserted
    // otherwise it is 1
    assign fifo_i_tkeep[i] = ~fifo_i_tlast |
      (up_keep_keep[i/IN_WORDS] ? up_keep_flat[i] : 1'b0);
  end endgenerate

  generate
    if (SKIP_FIFO) begin
      assign fifo_o_tdata  = fifo_i_tdata;
      assign fifo_o_tkeep  = fifo_i_tkeep;
      assign fifo_o_tlast  = fifo_i_tlast;
      assign fifo_o_tvalid = fifo_i_tvalid;
      assign fifo_i_tready = fifo_o_tready;
    end else begin
      if (SYNC_CLKS) begin
        axi_fifo #(.WIDTH(INT_DATA_W+INT_KEEP_W+1), .SIZE(FIFO_SIZE)) fifo_i (
          .clk(s_axis_aclk), .reset(s_axis_rst), .clear(1'b0),
          .i_tdata({fifo_i_tlast, fifo_i_tkeep, fifo_i_tdata}),
          .i_tvalid(fifo_i_tvalid), .i_tready(fifo_i_tready),
          .o_tdata({fifo_o_tlast, fifo_o_tkeep, fifo_o_tdata}),
          .o_tvalid(fifo_o_tvalid), .o_tready(fifo_o_tready),
          .space(), .occupied()
        );
      end else begin
        axi_fifo_2clk #(.WIDTH(INT_DATA_W+INT_KEEP_W+1), .SIZE(FIFO_SIZE)) fifo_i (
          .reset(s_axis_rst),
          .i_aclk(s_axis_aclk),
          .i_tdata({fifo_i_tlast, fifo_i_tkeep, fifo_i_tdata}),
          .i_tvalid(fifo_i_tvalid), .i_tready(fifo_i_tready),
          .o_aclk(m_axis_aclk),
          .o_tdata({fifo_o_tlast, fifo_o_tkeep, fifo_o_tdata}),
          .o_tvalid(fifo_o_tvalid), .o_tready(fifo_o_tready)
        );
      end
    end
  endgenerate

  // tkeep masking logic after downsizer
  generate for (i = 0; i < DOWNSIZE_RATIO; i = i + 1) begin
    assign down_keep_keep[i] = |fifo_o_tkeep[i*OUT_WORDS+:OUT_WORDS];
  end endgenerate

  axis_downsizer #(
    .OUT_DATA_W(OUT_WORDS*WORD_W), .OUT_USER_W(OUT_WORDS),
    .RATIO(DOWNSIZE_RATIO)
  ) downsizer_i (
    .clk(m_axis_aclk), .reset(m_axis_rst),
    .s_axis_tdata(fifo_o_tdata), .s_axis_tuser(fifo_o_tkeep), .s_axis_tkeep(down_keep_keep),
    .s_axis_tlast(fifo_o_tlast), .s_axis_tvalid(fifo_o_tvalid), .s_axis_tready(fifo_o_tready),
    .m_axis_tdata(o_tdata), .m_axis_tuser(o_tkeep),
    .m_axis_tlast(o_tlast), .m_axis_tvalid(o_tvalid), .m_axis_tready(o_tready)
  );

endmodule // axis_width_conv
