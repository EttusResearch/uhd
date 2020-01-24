//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_trim_payload
// Description:
//  This module trims any extra data on the AXI-Stream
//  bus to the CHDR payload size. This ensures that the
//  line with tlast is the actual last line of the packet
//
// Parameters:
//  - CHDR_W: Width of the CHDR tdata bus in bits
//  - USER_W: Width of the tuser bus in bits
//
// Signals:
//  - s_axis_* : Input AXI-Stream CHDR bus
//  - m_axis_* : Output AXI-Stream CHDR bus

module chdr_trim_payload #(
  parameter CHDR_W = 256,
  parameter USER_W = 16
)(
  input  wire              clk,
  input  wire              rst,
  input  wire [CHDR_W-1:0] s_axis_tdata,
  input  wire [USER_W-1:0] s_axis_tuser,
  input  wire              s_axis_tlast,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,
  output wire [CHDR_W-1:0] m_axis_tdata,
  output wire [USER_W-1:0] m_axis_tuser,
  output wire              m_axis_tlast,
  output wire              m_axis_tvalid,
  input  wire              m_axis_tready
);

  `include "../core/rfnoc_chdr_utils.vh"

  localparam LOG2_CHDR_W_BYTES = $clog2(CHDR_W/8);

  localparam [1:0] ST_HEADER = 2'd0;
  localparam [1:0] ST_BODY   = 2'd1;
  localparam [1:0] ST_DUMP   = 2'd2;

  reg [1:0]   state;
  reg [15:0]  lines_left;

  wire [15:0] pkt_length = chdr_get_length(s_axis_tdata[63:0]);
  wire [15:0] lines_in_pkt = pkt_length[15:LOG2_CHDR_W_BYTES] + (|pkt_length[LOG2_CHDR_W_BYTES-1:0]);
  wire        last_line = (lines_left == 16'd0);

  always @(posedge clk) begin
    if (rst) begin
      state <= ST_HEADER;
      lines_left <= 16'd0;
    end else if(s_axis_tvalid & s_axis_tready) begin
      case(state)
        ST_HEADER: begin
          if ((lines_in_pkt == 16'd1) && !s_axis_tlast) begin
            // First line is valid, dump rest
            state <= ST_DUMP;
          end else begin
            lines_left <= lines_in_pkt - 16'd2;
            state <= ST_BODY;
          end
        end
        ST_BODY: begin
          if (last_line && !s_axis_tlast) begin
            state <= ST_DUMP;
          end else if (s_axis_tlast) begin
            state <= ST_HEADER;
          end else begin
            lines_left <= lines_left - 16'd1;
          end
        end
        ST_DUMP: begin
          if (s_axis_tlast)
            state <= ST_HEADER;
        end
        default: begin
          // We should never get here
          state <= ST_HEADER;
        end
      endcase
    end
  end

  assign m_axis_tdata  = s_axis_tdata;
  assign m_axis_tuser  = s_axis_tuser;
  assign m_axis_tlast  = s_axis_tlast ||
                         ((state == ST_HEADER) && (lines_in_pkt == 16'd1)) ||
                         ((state == ST_BODY) && last_line);
  assign m_axis_tvalid = s_axis_tvalid && (state != ST_DUMP);
  assign s_axis_tready = m_axis_tready || (state == ST_DUMP);

endmodule // chdr_trim_payload
