//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_channel_mux.sv
//
// Description:
//
// The module is used to multiplex incoming CHDR data packets from the connected data channels into
// a single stream of interleaved data packets. Forwarding of the data packets is done according to
// a round-robin priority scheme by default. The module modifies the VC field of the incoming data
// packets to indicate the channel from which the packet originated.
//
// Parameters:
//
//  - NUM_PORTS       : The number of data channels to multiplex.
//  - CHDR_W          : The width of the CHDR data packets.
//  - CHANNEL_OFFSET  : The offset for setting the channel number in the VC field of the CHDR header
//                      (default is 0). The VC field of a packet coming in on a specific port is set
//                      to the port number + CHANNEL_OFFSET.
//  - PRE_FIFO_SIZE   : The size of the FIFO buffering the inputs in log2. Set to -1 to disable.
//  - POST_FIFO_SIZE  : The size of the FIFO after the multiplexer in log2. Set to 0 to disable.
//  - PRIORITY        : The priority scheme to use for forwarding the data packets.
//                      0 - Round-robin
//                      1 - Priority (lower number ports get priority)
//

`default_nettype none

module chdr_channel_mux #(
  parameter int NUM_PORTS      = 2,
  parameter int CHDR_W         = 64,
  parameter int CHANNEL_OFFSET = 0,
  parameter int PRE_FIFO_SIZE  = 1,
  parameter int POST_FIFO_SIZE = 1,
  parameter int PRIORITY       = 0
) (
  input  wire logic                             clk,
  input  wire logic                             rst,
  input  wire logic [NUM_PORTS-1:0][CHDR_W-1:0] in_tdata,
  input  wire logic [NUM_PORTS-1:0]             in_tvalid,
  input  wire logic [NUM_PORTS-1:0]             in_tlast,
  output wire logic [NUM_PORTS-1:0]             in_tready,
  output wire logic [   CHDR_W-1:0]             out_tdata,
  output wire logic                             out_tvalid,
  output wire logic                             out_tlast,
  input  wire logic                             out_tready
);
  import rfnoc_chdr_utils_pkg::*;
  // Make sure channel number fits in VC field
  if ($clog2(CHANNEL_OFFSET + NUM_PORTS) > CHDR_VC_W) begin : gen_vc_assertion
    $error("Channel number exceeds VC field size!");
  end

  //---------------------------------------------------------------------------
  // Local variables
  //---------------------------------------------------------------------------
  logic [NUM_PORTS-1:0]             hdr_next = '1;  // Header is expected in the next cycle
  // Mux inputs
  logic [NUM_PORTS-1:0][CHDR_W-1:0] in_tdata_mux;
  logic [NUM_PORTS-1:0]             in_tvalid_mux;
  logic [NUM_PORTS-1:0]             in_tlast_mux;
  logic [NUM_PORTS-1:0]             in_tready_mux;
  // Mux input buffer fifo
  logic [NUM_PORTS-1:0][CHDR_W-1:0] in_tdata_fifo;
  logic [NUM_PORTS-1:0]             in_tvalid_fifo;
  logic [NUM_PORTS-1:0]             in_tlast_fifo;
  logic [NUM_PORTS-1:0]             in_tready_fifo;

  //---------------------------------------------------------------------------
  // Module logic
  //---------------------------------------------------------------------------
  genvar ch;
  for (ch = 0; ch < NUM_PORTS; ch++) begin : gen_input_channels
    // Buffer multiplexer inputs with single stage flop FIFO
    axi_fifo #(
      .WIDTH(CHDR_W + 1),
      .SIZE (PRE_FIFO_SIZE)
    ) axi_fifo_input_ch_i (
      .clk(clk),
      .reset(rst),
      .clear('0),
      .i_tdata({in_tlast[ch], in_tdata[ch]}),
      .i_tvalid(in_tvalid[ch]),
      .i_tready(in_tready[ch]),
      .o_tdata({in_tlast_fifo[ch], in_tdata_fifo[ch]}),
      .o_tvalid(in_tvalid_fifo[ch]),
      .o_tready(in_tready_fifo[ch]),
      .space(),
      .occupied()
    );
    // Detect upcoming header
    always_ff @(posedge clk) begin : detect_hdr
      if (rst) begin
        hdr_next[ch] <= 1'b1;
      end else if (in_tlast_fifo[ch] && in_tvalid_fifo[ch] && in_tready_fifo[ch]) begin
        hdr_next[ch] <= 1'b1;
      end else if (in_tvalid_fifo[ch] && in_tready_fifo[ch]) begin
        hdr_next[ch] <= 1'b0;
      end
    end
    // Set header field for VC according to port
    always_comb begin : set_vc
      in_tdata_mux[ch] = in_tdata_fifo[ch];
      if (hdr_next[ch]) begin
        in_tdata_mux[ch][CHDR_HEADER_W-1:0] =
          chdr_set_vc(in_tdata_fifo[ch][CHDR_HEADER_W-1:0], CHANNEL_OFFSET + ch);
      end
    end
    assign in_tvalid_mux[ch]  = in_tvalid_fifo[ch];
    assign in_tlast_mux[ch]   = in_tlast_fifo[ch];
    assign in_tready_fifo[ch] = in_tready_mux[ch];

  end

  //---------------------------------------------------------------------------
  // Multiplexer
  //---------------------------------------------------------------------------
  if (NUM_PORTS == 1) begin : gen_single_port
    assign out_tdata = in_tdata_mux[0];
    assign out_tvalid = in_tvalid_mux[0];
    assign out_tlast = in_tlast_mux[0];
    assign in_tready_mux[0] = out_tready;
  end else begin : gen_multi_port_mux
    axi_mux #(
      .SIZE(NUM_PORTS),
      .WIDTH(CHDR_W),
      .PRIO(PRIORITY),
      .PRE_FIFO_SIZE(1),
      .POST_FIFO_SIZE(POST_FIFO_SIZE)
    ) axi_mux_i (
      .clk(clk),
      .reset(rst),
      .clear('0),
      .i_tdata(in_tdata_mux),
      .i_tvalid(in_tvalid_mux),
      .i_tlast(in_tlast_mux),
      .i_tready(in_tready_mux),
      .o_tdata(out_tdata),
      .o_tvalid(out_tvalid),
      .o_tlast(out_tlast),
      .o_tready(out_tready)
    );
  end

endmodule

`default_nettype wire
