//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_dechunker
//
// Description:
//
//   Reconstruct chdr packets from quantized chdr packets. o_tlast and
//   i_tready will be held off until the entire quantized packet is transferred.
//   If quantum is changed, it is the responsibility of the client to clear
//   this module. 'error' is asserted if a packet is larger than the quantum.
//
// Parameters:
//   DATA_WIDTH : Width of the data bus
//   PAD_VALUE  : Value used for padding
//

`default_nettype none

module chdr_dechunker # (
  parameter DATA_WIDTH = 64,
  parameter PAD_VALUE = {DATA_WIDTH{1'b1}}
) (
  input  wire                   clk,
  input  wire                   reset,
  input  wire                   clear,
  input  wire [15:0]            frame_size,

  input  wire [DATA_WIDTH-1:0]  i_tdata,
  input  wire                   i_tvalid,
  output wire                   i_tready,

  output wire [DATA_WIDTH-1:0]  o_tdata,
  output wire                   o_tlast,
  output wire                   o_tvalid,
  input  wire                   o_tready,

  output wire                   error
);

  localparam ST_HEADER  = 2'd0;
  localparam ST_DATA    = 2'd1;
  localparam ST_PADDING = 2'd2;
  localparam ST_ERROR   = 2'd3;

  reg [1:0]   state;
  reg [15:0]  frame_rem, pkt_rem;
  wire        i_tlast;

  localparam BYTES_IN_DATA = DATA_WIDTH / 8;
  localparam LEN_WIDTH = $clog2(BYTES_IN_DATA);

  // axi_len = ceil(length / BYTES_IN_DATA)
  wire [15:0] cvita_len_ceil = i_tdata[31:16] + (BYTES_IN_DATA - 1);
  wire [15:0] axi_len = {{LEN_WIDTH{1'b0}}, cvita_len_ceil[15:LEN_WIDTH]};

  always @(posedge clk) begin
    if (reset | clear) begin
        state <= ST_HEADER;
        frame_rem <= 16'd0;
        pkt_rem   <= 16'd0;
    end else if (i_tvalid & i_tready) begin
      case (state)
        ST_HEADER: begin
          if (axi_len > frame_size) begin
            state <= ST_ERROR;
          end else if (~o_tlast) begin
            state <= ST_DATA;
          end else begin
            state <= ST_PADDING;
          end

          frame_rem <= frame_size - 16'd1;
          pkt_rem   <= axi_len - 16'd1;
        end

        ST_DATA: begin
          if (o_tlast) begin
            state   <= i_tlast ? ST_HEADER : ST_PADDING;
            pkt_rem <= 16'd0;
          end else begin
            state   <= ST_DATA;
            pkt_rem <= pkt_rem - 16'd1;
          end
          frame_rem <= frame_rem - 16'd1;
        end

        ST_PADDING: begin
          if (i_tlast) begin
            state   <= ST_HEADER;
            frame_rem <= 16'd0;
          end else begin
            state   <= ST_PADDING;
            frame_rem <= frame_rem - 16'd1;
          end
        end

        ST_ERROR: begin
          // We never leave the error state. However, we can't reach it
          // with PCIe if we configure our transport according to the
          // NI-RIO configuration.
          state <= ST_ERROR;
        end
      endcase
    end
  end

  assign i_tready = o_tready | (state == ST_PADDING);
  assign i_tlast = (frame_rem == 16'd1); //Temp signal

  assign o_tvalid = i_tvalid & (state != ST_PADDING);
  assign o_tlast = (pkt_rem != 0) ? (pkt_rem == 16'd1) : (axi_len == 16'd1);
  assign o_tdata  = i_tdata;

  assign error = (state == ST_ERROR);

endmodule // chdr_dechunker

`default_nettype wire
