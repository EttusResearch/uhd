//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_tag_time
//
// Description:
//
//   This module accepts samples on s_axis_data_* and outputs them on
//   m_axis_data_*. When the user executes a timed settings bus command to a
//   register, this module will tag the sample that the command should apply on
//   by asserting the bit corresponding to that register in m_axis_data_tag.
//
//   The order of operations is as follows:
//
//   1) Receives settings bus command on in_set_*.
//
//      a) If in_set_has_time == 0, output on non-timed settings bus (out_set_*).
//
//      b) If in_set_has_time == 1, output on timed settings bus (timed_set_*)
//         and store the time in a FIFO. It is assumed the user will use
//         timed_settings_bus.v implementation.
//
//   2) When the sample corresponding to the time stored in the FIFO arrives on
//      s_axis_data_*, assert the corresponding bit in m_axis_data_tag at the
//      same time the sample is output on m_axis_data_*.
//
//   The TUSER port contains the packet header information:
//
//     tuser[125]  : Has timestamp
//     tuser[63:0] : Timestamp
//
//   This field should be valid for the duration of the packet, although in
//   reality the timestamp is only read during the first sample of each packet.
//   The "has time" bit will be read at various points depending on the
//   requested time.
//

module axi_tag_time #(
  parameter                          WIDTH         = 32,
  parameter                          HEADER_WIDTH  = 128,
  parameter                          SR_AWIDTH     = 8,
  parameter                          SR_DWIDTH     = 32,
  parameter                          SR_TWIDTH     = 64,
  parameter                          NUM_TAGS      = 1,
  parameter [NUM_TAGS*SR_AWIDTH-1:0] SR_TAG_ADDRS  = 0,
  parameter                          CMD_FIFO_SIZE = 5,
  parameter                          MAX_TICK_RATE = 2**16-1
) (
  input clk,
  input reset,
  input clear,

  input  [$clog2(MAX_TICK_RATE)-1:0] tick_rate,
  output                             timed_cmd_fifo_full,

  // Input sample stream
  input  [       WIDTH-1:0] s_axis_data_tdata,
  input  [HEADER_WIDTH-1:0] s_axis_data_tuser,
  input                     s_axis_data_tlast,
  input                     s_axis_data_tvalid,
  output                    s_axis_data_tready,

  // Output sample string, with tag
  output [       WIDTH-1:0] m_axis_data_tdata,
  output [HEADER_WIDTH-1:0] m_axis_data_tuser,
  output [    NUM_TAGS-1:0] m_axis_data_tag,
  output                    m_axis_data_tlast,
  output                    m_axis_data_tvalid,
  input                     m_axis_data_tready,

  // Settings bus
  input                 in_set_stb,
  input [SR_AWIDTH-1:0] in_set_addr,
  input [SR_DWIDTH-1:0] in_set_data,
  input [SR_TWIDTH-1:0] in_set_time,
  input                 in_set_has_time,

  // Non-timed settings bus to user
  output                 out_set_stb,
  output [SR_AWIDTH-1:0] out_set_addr,
  output [SR_DWIDTH-1:0] out_set_data,

  // Timed settings bus to user
  output                 timed_set_stb,
  output [SR_AWIDTH-1:0] timed_set_addr,
  output [SR_DWIDTH-1:0] timed_set_data
);

  assign out_set_addr   = in_set_addr;
  assign out_set_data   = in_set_data;
  assign out_set_stb    = in_set_stb & ~in_set_has_time;

  assign timed_set_addr = in_set_addr;
  assign timed_set_data = in_set_data;
  assign timed_set_stb  = in_set_stb & in_set_has_time;

  // Extract vita time from s_axis_data_tuser
  wire [63:0] vita_time_in;
  cvita_hdr_decoder cvita_hdr_decoder_in (
    .header         (s_axis_data_tuser),
    .pkt_type       (),
    .eob            (),
    .has_time       (),
    .seqnum         (),
    .length         (),
    .payload_length (),
    .src_sid        (),
    .dst_sid        (),
    .vita_time      (vita_time_in)
  );

  // Track time
  reg header_valid = 1'b1;
  reg [63:0] vita_time_now = 64'd0, set_time_hold = 64'd0;
  always @(posedge clk) begin
    if (reset | clear) begin
      header_valid     <= 1'b1;
    end else begin
      if (s_axis_data_tvalid & s_axis_data_tready) begin
        if (s_axis_data_tlast) begin
          header_valid <= 1'b1;
        end else begin
          header_valid <= 1'b0;
        end
        if (header_valid) begin
          vita_time_now <= vita_time_in;
        end else begin
          vita_time_now <= vita_time_now + tick_rate;
        end
      end
    end
  end

  // Create the tags vector by asserting only the tag bit corresponding to the
  // register being addressed.
  genvar i;
  wire [NUM_TAGS-1:0] tags;
  generate
    for (i = 0; i < NUM_TAGS; i = i + 1) begin
      assign tags[i] = (in_set_addr == SR_TAG_ADDRS[SR_AWIDTH*(i+1)-1:SR_AWIDTH*i]);
    end
  endgenerate

  // FIFO to hold tags + times
  wire [SR_TWIDTH-1:0] fifo_set_time;
  wire [NUM_TAGS-1:0] fifo_tags;
  wire fifo_tvalid, fifo_tready;
  wire timed_cmd_fifo_full_n;
  axi_fifo #(
    .WIDTH (SR_TWIDTH+NUM_TAGS),
    .SIZE  (CMD_FIFO_SIZE)
  ) axi_fifo (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear),
    .i_tdata  ({in_set_time,tags}),
    .i_tvalid (timed_set_stb),
    .i_tready (timed_cmd_fifo_full_n),
    .o_tdata  ({fifo_set_time,fifo_tags}),
    .o_tvalid (fifo_tvalid),
    .o_tready (fifo_tready),
    .space    (),
    .occupied ()
  );

  // Extract has_time from m_axis_data_tuser
  wire has_time;
  cvita_hdr_decoder cvita_hdr_decoder_out (
    .header         (m_axis_data_tuser),
    .pkt_type       (),
    .eob            (),
    .has_time       (has_time),
    .seqnum         (),
    .length         (),
    .payload_length (),
    .src_sid        (),
    .dst_sid        (),
    .vita_time      ()
  );

  assign timed_cmd_fifo_full = ~timed_cmd_fifo_full_n;
  assign fifo_tready = m_axis_data_tvalid &
                       m_axis_data_tready &
                       fifo_tvalid        &
                       has_time           &
                       (vita_time_now >= fifo_set_time);

  // Need a single cycle delay to allow vita_time_now to update at the start of
  // a new packet.
  axi_fifo_flop #(
    .WIDTH (WIDTH+HEADER_WIDTH+1)
  ) axi_fifo_flop (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear),
    .i_tdata  ({ s_axis_data_tdata, s_axis_data_tuser, s_axis_data_tlast }),
    .i_tvalid (s_axis_data_tvalid),
    .i_tready (s_axis_data_tready),
    .o_tdata  ({ m_axis_data_tdata, m_axis_data_tuser, m_axis_data_tlast }),
    .o_tvalid (m_axis_data_tvalid),
    .o_tready (m_axis_data_tready)
  );

  assign m_axis_data_tag =
    ((vita_time_now >= fifo_set_time) & fifo_tvalid & has_time) ? fifo_tags : 'd0;

endmodule
