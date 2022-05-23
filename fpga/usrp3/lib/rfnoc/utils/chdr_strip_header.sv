//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_strip_header
//
// Description:
//
//   Removes the CHDR header depending on the state of strip_en on the first
//   word of the packet.
//

`default_nettype none


module chdr_strip_header #(
  parameter  CHDR_W = 64,
  localparam USER_W = 16
) (
  // Clock and reset
  input wire clk,
  input wire rst,

  // Strip enable, sampled when the first word of the packet is transferred
  input wire strip_en,

  // CHDR input packet
  input  wire  [CHDR_W-1:0] s_chdr_tdata,
  input  wire               s_chdr_tlast,
  input  wire               s_chdr_tvalid,
  output logic              s_chdr_tready,

  // Output packet (with or without header)
  output logic [CHDR_W-1:0] m_tdata,
  output logic [USER_W-1:0] m_tuser,
  output logic              m_tlast,
  output logic              m_tvalid,
  input  wire               m_tready
);

  `include "../core/rfnoc_chdr_utils.vh"

  enum {
    ST_HDR,   // CHDR header
    ST_STRIP, // Strip header
    ST_PASS   // Pass packet through
  } state;

  localparam COUNT_W = (CHDR_W == 64) ?
    6 : // Max count is timestamp + metadata
    5;  // Max count is metadata only (timestamp is in the first word)

  // The number of CHDR words we need to skip to get to the payload, minus one.
  logic [COUNT_W-1:0] skip_length;
  // Counter to track where we are in the header.
  logic [COUNT_W-1:0] skip_count;

  // Total length of the header, including timestamp and metadata, minus 1.
  logic [COUNT_W-1:0] hdr_length;

  // Register to hold the new packet length, in bytes
  logic [15:0] packet_length;

  //---------------------------------------------------------------------------
  // State Machine Registers
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin : sm_reg
    case (state)
      ST_HDR : begin
        skip_count    <= 1;
        skip_length   <= hdr_length;
        packet_length <= chdr_get_length(s_chdr_tdata) - (CHDR_W/8);

        if (s_chdr_tvalid && s_chdr_tready) begin
          if (hdr_length > 0 && strip_en) begin
            state <= ST_STRIP;
          end else begin
            state <= ST_PASS;
          end
        end
      end
      ST_STRIP : begin
        if (s_chdr_tvalid && s_chdr_tready) begin
          packet_length <= packet_length - (CHDR_W/8);
          skip_count <= skip_count + 1;
          if (s_chdr_tlast) begin
            state <= ST_HDR;
          end else if (skip_count == skip_length) begin
            state <= ST_PASS;
          end
        end
      end
      ST_PASS : begin
        if (s_chdr_tvalid && s_chdr_tready && s_chdr_tlast) begin
          state <= ST_HDR;
        end
      end
    endcase

    if (rst) begin
      state <= ST_HDR;
    end
  end : sm_reg

  //---------------------------------------------------------------------------
  // State Machine Combinatorial Logic
  //---------------------------------------------------------------------------

  always_comb begin : sm_comb
    m_tdata = s_chdr_tdata;
    m_tlast = s_chdr_tlast;

    // Calculate the length of the header, including timestamp and metadata,
    // minus 1.
    if (CHDR_W == 64 && chdr_get_pkt_type(s_chdr_tdata) == CHDR_PKT_TYPE_DATA_TS) begin
      hdr_length = chdr_get_num_mdata(s_chdr_tdata) + 1;
    end else begin
      hdr_length = chdr_get_num_mdata(s_chdr_tdata);
    end

    // Drive m_tvalid and s_chdr_tready
    case (state)
      ST_HDR : begin
        m_tuser = chdr_get_length(s_chdr_tdata);
        if (strip_en) begin
          // Drop header word
          m_tvalid      = 0;
          s_chdr_tready = 1;
        end else begin
          // Pass header through
          m_tvalid      = s_chdr_tvalid;
          s_chdr_tready = m_tready;
        end
      end
      ST_STRIP : begin
        // Drop incoming words
        m_tvalid      = 0;
        m_tuser       = 'X;
        s_chdr_tready = 1;
      end
      ST_PASS : begin
        // Pass words through
        m_tvalid      = s_chdr_tvalid;
        m_tuser       = packet_length;
        s_chdr_tready = m_tready;
      end
    endcase
  end : sm_comb

endmodule : chdr_strip_header


`default_nettype wire
