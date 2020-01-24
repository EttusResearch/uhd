//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_pad_packet
// Description:
//  This module pads extra data on the AXI-Stream bus
//  to the requested packet size. This module is for
//  creating len-sized packets, for DMA engines that
//  do not support partial transfers.
//
// Parameters:
//  - CHDR_W: Width of the CHDR tdata bus in bits
//
// Signals:
//  - s_axis_* : Input AXI-Stream CHDR bus
//  - m_axis_* : Output AXI-Stream CHDR bus
//  - len      : Requested number of CHDR_W lines in the packet (must be > 1)

`default_nettype none
module chdr_pad_packet #(
  parameter CHDR_W = 256
)(
  input  wire              clk,
  input  wire              rst,
  input  wire [15:0]       len,
  input  wire [CHDR_W-1:0] s_axis_tdata,
  input  wire              s_axis_tlast,
  input  wire              s_axis_tvalid,
  output reg               s_axis_tready,
  output wire [CHDR_W-1:0] m_axis_tdata,
  output reg               m_axis_tlast,
  output reg               m_axis_tvalid,
  input  wire              m_axis_tready
);

  localparam [1:0] ST_HEADER = 2'd0;
  localparam [1:0] ST_BODY   = 2'd1;
  localparam [1:0] ST_PAD    = 2'd2;
  localparam [1:0] ST_DROP   = 2'd3;

  reg [1:0]   state;
  reg [15:0]  lines_left;

  always @(posedge clk) begin
    if (rst || (len <= 16'd1)) begin
      state <= ST_HEADER;
    end else begin
      case(state)
        ST_HEADER: begin
          lines_left <= len - 16'd1;
          if (s_axis_tvalid && m_axis_tready) begin
              if (!s_axis_tlast) begin
                // Packet is more than one line and length not reached
                state <= ST_BODY;
              end else begin
                // Packet is only one line and length not reached
                state <= ST_PAD;
              end
          end
        end
        ST_BODY: begin
          if (s_axis_tvalid && m_axis_tready) begin
            lines_left <= lines_left - 16'd1;
            if (s_axis_tlast && (lines_left == 16'd1)) begin
              // End of input and reached length
              state <= ST_HEADER;
            end else if (s_axis_tlast && (lines_left != 16'd1)) begin
              // End of input, but length not reached
              state <= ST_PAD;
            end else if (!s_axis_tlast && (lines_left == 16'd1)) begin
              // Reached length, but input continues...
              state <= ST_DROP;
            end
          end
        end
        ST_PAD: begin
          if (m_axis_tready) begin
            lines_left <= lines_left - 16'd1;
            if (lines_left == 16'd1) begin
              state <= ST_HEADER;
            end
          end
        end
        ST_DROP: begin
          if (s_axis_tvalid && s_axis_tlast) begin
            state <= ST_HEADER;
          end
        end
        default: begin
          // We should never get here
          state <= ST_HEADER;
        end
      endcase
    end
  end

  assign m_axis_tdata  = s_axis_tdata;

  always @(*) begin
    case(state)
      ST_HEADER: begin
        if (len <= 16'd1) begin
          s_axis_tready <= 1'b0;
          m_axis_tvalid <= 1'b0;
        end else begin
          s_axis_tready <= m_axis_tready;
          m_axis_tvalid <= s_axis_tvalid;
        end
        m_axis_tlast <= 1'b0;
      end
      ST_BODY: begin
        s_axis_tready <= m_axis_tready;
        m_axis_tvalid <= s_axis_tvalid;
        m_axis_tlast  <= (lines_left == 16'd1);
      end
      ST_PAD: begin
        s_axis_tready <= 1'b0;
        m_axis_tvalid <= 1'b1;
        m_axis_tlast  <= (lines_left == 16'd1);
      end
      ST_DROP: begin
        s_axis_tready <= 1'b1;
        m_axis_tvalid <= 1'b0;
        m_axis_tlast  <= 1'b0;
      end
    endcase
  end

endmodule // chdr_pad_packet
`default_nettype wire
