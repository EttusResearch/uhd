//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// - Tracks and fills out header information for an axi stream that is
//   asynchronous or does not have a 1:1 input / output ratio.
// - User must pass through **ALL** received words and use the tkeep
//   signal to flag which words to keep.
// - This module is not intended to work with decimation / interpolation blocks.
//
// Open design questions:
// - If a tkeep burst occurs between packet boundaries, an internal tlast is
//   generated splitting the burst up into two (or more) packets. This is
//   an easy way to make sure the packet sizes are bounded and the VITA
//   time is correct. Is this desirable, since the downstream block
//   will likely want the full burst and is then forced to aggregate packets?
//

module axi_async_stream #(
  parameter WIDTH            = 32,
  parameter HEADER_WIDTH     = 128,
  parameter HEADER_FIFO_SIZE = 5,
  parameter MAX_TICK_RATE    = 2**16-1)
(
  input clk,
  input reset,
  input clear,
  input [15:0] src_sid,
  input [15:0] dst_sid,
  input [$clog2(MAX_TICK_RATE)-1:0] tick_rate,
  output header_fifo_full,
  // From AXI Wrapper
  input [WIDTH-1:0] s_axis_data_tdata,
  input [HEADER_WIDTH-1:0] s_axis_data_tuser,
  input s_axis_data_tlast,
  input s_axis_data_tvalid,
  output s_axis_data_tready,
  // To AXI Wrapper
  output [WIDTH-1:0] m_axis_data_tdata,
  output [HEADER_WIDTH-1:0] m_axis_data_tuser,
  output m_axis_data_tlast,
  output m_axis_data_tvalid,
  input m_axis_data_tready,
  // To User
  output [WIDTH-1:0] o_tdata,
  output o_tlast,
  output o_tvalid,
  input o_tready,
  // From User
  input [WIDTH-1:0] i_tdata,
  input i_tlast,
  input i_tvalid,
  input i_tkeep,
  output i_tready
);

  wire [WIDTH-1:0] i_reg_tdata;
  wire i_reg_tvalid, i_reg_tlast, i_reg_tkeep, i_reg_tready;

  reg [WIDTH-1:0] pipe_tdata;
  reg pipe_tvalid, pipe_tlast, pipe_tkeep;
  wire pipe_tready;

  /********************************************************
  ** Register user input
  ** - The output logic in some cases needs to wait for
  **   i_tvalid to assert before asserting i_tready.
  **   However, users may implement logic that waits for
  **   i_tready to assert before asserting i_tvalid.
  **   Without this register, that would result in a
  **   deadlock.
  ** - Note: Technically, the user waiting for i_tready
  **   violates the AXI specification that a master cannot
  **   wait for ready from the slave. However, it is common
  **   for users to accidentally break this rule and this is
  **   a cheap workaround.
  ********************************************************/
  axi_fifo_flop #(.WIDTH(WIDTH+2)) axi_fifo_flop (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({i_tkeep,i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata({i_reg_tkeep,i_reg_tlast,i_reg_tdata}), .o_tvalid(i_reg_tvalid), .o_tready(i_reg_tready));

  /********************************************************
  ** Keep track of headers for user
  ********************************************************/
  wire header_in_tready, header_in_tvalid, header_out_tvalid, header_out_tready;
  wire [HEADER_WIDTH-1:0] header_in_tdata, header_out_tdata;

  reg first_word = 1'b1;
  reg [15:0] word_cnt;
  reg [16+$clog2(MAX_TICK_RATE)-1:0] time_cnt; // 16 bit payload length + max tick rate increment

  wire [63:0] vita_time;
  wire [15:0] payload_length;

  // Track first word to make sure header is read only once per packet
  always @(posedge clk) begin
    if (reset | clear) begin
      first_word <= 1'b1;
    end else begin
      if (s_axis_data_tvalid & s_axis_data_tready) begin
        if (s_axis_data_tlast) begin
          first_word <= 1'b1;
        end else if (first_word) begin
          first_word <= 1'b0;
        end
      end
    end
  end

  // Header FIFO
  axi_fifo #(.WIDTH(HEADER_WIDTH), .SIZE(HEADER_FIFO_SIZE)) axi_fifo (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(header_in_tdata), .i_tvalid(header_in_tvalid), .i_tready(header_in_tready),
    .o_tdata(header_out_tdata), .o_tvalid(header_out_tvalid), .o_tready(header_out_tready),
    .space(), .occupied());

  assign header_in_tdata   = s_axis_data_tuser;
  assign header_in_tvalid  = s_axis_data_tvalid & o_tready & first_word;
  assign header_out_tready = i_reg_tvalid & i_reg_tready & (word_cnt >= payload_length);
  assign header_fifo_full  = ~header_in_tready;

  // Track VITA time offset and word count for emptying header FIFO
  always @(posedge clk) begin
    if (reset | clear) begin
      word_cnt       <= WIDTH/8;
      time_cnt       <= 0;
    end else begin
      if (pipe_tvalid & pipe_tready) begin
        if (word_cnt >= payload_length) begin
          word_cnt <= WIDTH/8;
          time_cnt <= 0;
        end else begin
          word_cnt <= word_cnt + WIDTH/8;
          time_cnt <= time_cnt + tick_rate;
        end
      end
    end
  end

  // Form output header
  cvita_hdr_decoder cvita_hdr_decoder (
    .header(header_out_tdata),
    .pkt_type(), .eob(), .has_time(),
    .seqnum(), .payload_length(payload_length),
    .src_sid(), .dst_sid(),
    .vita_time(vita_time));

  cvita_hdr_modify cvita_hdr_modify (
    .header_in(header_out_tdata),
    .header_out(m_axis_data_tuser),
    .use_pkt_type(1'b0),       .pkt_type(),
    .use_has_time(1'b0),       .has_time(),
    .use_eob(1'b0),            .eob(),
    .use_seqnum(1'b0),         .seqnum(), // AXI Wrapper handles this
    .use_length(1'b0),         .length(), // AXI Wrapper handles this
    .use_payload_length(1'b0), .payload_length(),
    .use_src_sid(1'b1),        .src_sid(src_sid),
    .use_dst_sid(1'b1),        .dst_sid(dst_sid),
    .use_vita_time(1'b1),      .vita_time(vita_time + time_cnt));

  /********************************************************
  ** Data to user from AXI Wrapper
  ** - Throttles if header FIFO is full
  ********************************************************/
  assign o_tdata            = s_axis_data_tdata;
  assign o_tvalid           = s_axis_data_tvalid & header_in_tready;
  assign o_tlast            = s_axis_data_tlast;
  assign s_axis_data_tready = o_tready & header_in_tready;

  /********************************************************
  ** Data from user to AXI Wrapper
  ** - Handles asserting tlast
  ** - Asserts tlast in three cases:
  **   1) User asserts tlast
  **   2) End of a burst of samples (i.e. when tkeep deasserts).
  **   3) End of a packet, in case VITA is different between packets
  ********************************************************/
  wire ready;
  always @(posedge clk) begin
    if (reset | clear) begin
      pipe_tdata      <= 'd0;
      pipe_tvalid     <= 1'b0;
      pipe_tlast      <= 1'b0;
      pipe_tkeep      <= 1'b0;
    end else begin
      if (pipe_tready) begin
        pipe_tdata    <= i_reg_tdata;
        pipe_tvalid   <= i_reg_tvalid;
        pipe_tlast    <= i_reg_tlast;
        pipe_tkeep    <= i_reg_tkeep;
      end
    end
  end

  assign pipe_tready        = ~pipe_tvalid | (m_axis_data_tready & header_out_tvalid & (i_reg_tvalid | (m_axis_data_tvalid & m_axis_data_tlast)));
  assign i_reg_tready       = pipe_tready;
  assign m_axis_data_tdata  = pipe_tdata;
  assign m_axis_data_tvalid = pipe_tvalid & pipe_tkeep & (i_reg_tvalid | m_axis_data_tlast) & header_out_tvalid;
  assign m_axis_data_tlast  = pipe_tlast | (i_reg_tvalid & ~i_reg_tkeep) | (word_cnt >= payload_length);

endmodule
