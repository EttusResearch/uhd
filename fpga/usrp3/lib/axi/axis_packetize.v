//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_packetize
//
// Description:
//
//   This module takes in an axi_stream without packet boundaries (i.e.,
//   without tlast) and groups the data into packets by adding tlast at the
//   appropriate time. The size of the packet is controlled by the "size"
//   input, which is sampled at the beginning of each packet to be output. The
//   packet_size input indicates the number of i_tdata words to group into a
//   packet. The i_tlength output indicates the length of the packet being
//   output.
//
//   The gate input causes data transfers to be stopped at the end of the
//   current packet when the gate input is asserted. It is not legal to
//   deassert tvalid once it has been asserted until the next transfer is
//   completed, so this module monitors the state of the AXI-Stream protocol
//   so that no protocol violations occur.
//
//   Note that the current transfer may still complete after gate has been
//   asserted, so the downstream logic must be able to account for at least
//   one more transfer.
//
// Parameters:
//
//   DATA_W       : Width of the tdata signals.
//   SIZE_W       : The width of the packet size port. This dictates the
//                  maximum packet size.
//   FLUSH        : Controls whether or not the input should be stalled or
//                  flushed. That is, when FLUSH=0, the input data is stalled
//                  whenever the gate is on (i_tready becomes 0). When
//                  FLUSH=1, the input data is dropped whenever the gate is on
//                  (i_tready becomes 1).
//   DEFAULT_SIZE : The default packet size to use, if it doesn't need to be
//                  changed at run time.
//


module axis_packetize #(
  parameter DATA_W       = 32,
  parameter SIZE_W       = 16,
  parameter FLUSH        = 0,
  parameter DEFAULT_SIZE = 2**SIZE_W-1
) (
  input wire clk,
  input wire rst,

  input wire              gate,       // Stop or "gate" packet output
  input wire [SIZE_W-1:0] size,       // Size to use for the next packet

  // Input data stream
  input  wire [DATA_W-1:0] i_tdata,
  input  wire              i_tvalid,
  output wire              i_tready,

  // Output data stream
  output wire [DATA_W-1:0] o_tdata,
  output reg               o_tlast,
  output wire              o_tvalid,
  input  wire              o_tready,
  output wire [SIZE_W-1:0] o_tuser     // Current packet's size
);

  reg              start_of_packet = 1;    // Next sample is start of a packet
  reg [SIZE_W-1:0] word_count      = 0;    // Count of output words
  reg [SIZE_W-1:0] current_size    = DEFAULT_SIZE;  // Current packet size

  reg gating     = 1'b0;     // Indicate if output is blocked
  reg mid_packet = 1'b0;     // Indicate if we're in the middle of a packet

  //---------------------------------------------------------------------------
  // Packet Size Logic
  //---------------------------------------------------------------------------

  assign o_tuser = current_size;

  always @(posedge clk) begin
    if (rst) begin
      start_of_packet <= 1'b1;
      current_size    <= DEFAULT_SIZE;
      word_count      <= 0;
      o_tlast         <= (DEFAULT_SIZE == 1);
    end else begin
      if (gating) begin
        // Wait until we're enabled. Setup for the start of the next packet.
        start_of_packet <= 1'b1;
        current_size    <= size;
        word_count      <= size;
        o_tlast         <= (size == 1);
      end else if (o_tvalid && o_tready) begin
        start_of_packet <= 1'b0;
        word_count      <= word_count - 1;
        if (o_tlast) begin
          // This is the last sample, so restart everything for a new packet.
          start_of_packet <= 1'b1;
          current_size    <= size;
          word_count      <= size;
          o_tlast         <= (size == 1);
        end else if (word_count == 2) begin
          // This is the second to last sample, so we assert tlast for the
          // last sample.
          o_tlast <= 1'b1;
        end
      end else if (start_of_packet) begin
        // We're waiting for the start of the next packet. Keep checking the
        // size input until the next packet starts.
        current_size <= size;
        word_count   <= size;
        o_tlast      <= (size == 1);
      end
    end
  end

  //---------------------------------------------------------------------------
  // Handshake Monitor
  //---------------------------------------------------------------------------
  
  // Monitor the state of the handshake so we know when it's OK to
  // enable/disable data transfer.
  always @(posedge clk) begin
    if (rst) begin
      gating     = 1'b0;
      mid_packet = 1'b0;
    end else begin
      // Keep track of if we are in the middle of a packet or not. Note that
      // mid_packet will be 0 for the first transfer of a packet.
      if (o_tvalid && o_tready) begin
        if (o_tlast) begin
          mid_packet = 1'b0;
        end else begin
          mid_packet = 1'b1;
        end
      end

      if (gating) begin
        // We can stop gating any time
        if (!gate) gating <= 0;
      end else begin
        // Only start gating between packets when the output is idle, or after
        // the output transfer completes at the end of packet.
        if ((!mid_packet && !o_tvalid) || (o_tvalid && o_tready && o_tlast)) begin
          gating <= gate;
        end
      end
    end
  end

  //---------------------------------------------------------------------------
  // Data Pass-Through
  //---------------------------------------------------------------------------

  // Note that "gating" only asserts when a transfer completes at the end of a
  // packet, or between packets when the output is idle. This ensures that
  // o_tvalid won't deassert during a transfer and cause a handshake protocol
  // violation.

  assign o_tdata  = i_tdata;
  assign o_tvalid = i_tvalid && !gating;
  assign i_tready = FLUSH ? (o_tready || gating) : (o_tready && !gating);

endmodule
