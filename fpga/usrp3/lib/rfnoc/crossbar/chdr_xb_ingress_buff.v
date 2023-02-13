//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_ingress_buff
//
// Description:
//
//   Ingress buffer module for the CHDR crossbar. This module stores and gates
//   the incoming packet and simultaneously determines the destination (TDEST)
//   by inspecting the incoming TID. If the TID is CHDR_MGMT_ROUTE_EPID then we
//   perform a lookup on the TID to determine the correct output for TDEST.
//
// Parameters:
//
//   WIDTH       : Data width of the CHDR interfaces (TDATA)
//   MTU         : Maximum transmission unit, in WIDTH-sized words, is 2**MTU
//   DEST_W      : Width of the destination routing information (TDEST)
//   NODE_ID     : Numeric identifier for this port
//   EN_PKT_GATE : Enable packet gate to make each packet contiguous. This
//                 reduces congestion in the crossbar.
//

module chdr_xb_ingress_buff #(
  parameter       WIDTH       = 64,
  parameter       MTU         = 10,
  parameter       DEST_W      = 4,
  parameter [9:0] NODE_ID     = 0,
  parameter       EN_PKT_GATE = 0
) (
  input  wire               clk,
  input  wire               reset,
  // CHDR input port
  input  wire [WIDTH-1:0]   s_axis_chdr_tdata,
  input  wire [DEST_W-1:0]  s_axis_chdr_tdest,
  input  wire [1:0]         s_axis_chdr_tid,
  input  wire               s_axis_chdr_tlast,
  input  wire               s_axis_chdr_tvalid,
  output wire               s_axis_chdr_tready,
  // CHDR output port (with a tdest and tkeep)
  output wire [WIDTH-1:0]   m_axis_chdr_tdata,
  output wire [DEST_W-1:0]  m_axis_chdr_tdest,
  output wire               m_axis_chdr_tkeep,
  output wire               m_axis_chdr_tlast,
  output wire               m_axis_chdr_tvalid,
  input  wire               m_axis_chdr_tready,
  // Find port going to routing table
  output wire [15:0]        m_axis_find_tdata,
  output wire               m_axis_find_tvalid,
  input  wire               m_axis_find_tready,
  // Result port from routing table
  input  wire [DEST_W-1:0]  s_axis_result_tdata,
  input  wire               s_axis_result_tkeep,
  input  wire               s_axis_result_tvalid,
  output wire               s_axis_result_tready
);

  // RFNoC Includes
  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"


  //---------------------------------------------------------------------------
  // Packet Buffer
  //---------------------------------------------------------------------------

  wire [WIDTH-1:0]  gate_i_tdata , gate_o_tdata ;
  wire              gate_i_tlast , gate_o_tlast ;
  wire              gate_i_tvalid, gate_o_tvalid;
  wire              gate_i_tready, gate_o_tready;

  // The axi_packet_gate queues up an entire packet before letting it go out.
  // This reduces congestion in the crossbar for slowly-built packets.
  if (EN_PKT_GATE) begin : gen_pkt_gate
    axi_packet_gate #(
      .WIDTH (WIDTH),
      .SIZE  (MTU)
    ) axi_packet_gate_i (
      .clk      (clk),
      .reset    (reset),
      .clear    (1'b0),
      .i_tdata  (gate_i_tdata),
      .i_tlast  (gate_i_tlast),
      .i_terror (1'b0),
      .i_tvalid (gate_i_tvalid),
      .i_tready (gate_i_tready),
      .o_tdata  (gate_o_tdata),
      .o_tlast  (gate_o_tlast),
      .o_tvalid (gate_o_tvalid),
      .o_tready (gate_o_tready)
    );
  end else begin : gen_no_pkt_gate
    axi_fifo_flop2 #(
      .WIDTH(WIDTH+1)
    ) axi_fifo_flop2_i (
      .clk     (clk                         ),
      .reset   (reset                       ),
      .clear   (1'b0                        ),
      .i_tdata ({gate_i_tlast, gate_i_tdata}),
      .i_tvalid(gate_i_tvalid               ),
      .i_tready(gate_i_tready               ),
      .o_tdata ({gate_o_tlast, gate_o_tdata}),
      .o_tvalid(gate_o_tvalid               ),
      .o_tready(gate_o_tready               ),
      .space   (                            ),
      .occupied(                            )
    );
  end


  //---------------------------------------------------------------------------
  // Destination (TDEST) Muxing
  //---------------------------------------------------------------------------

  wire [15:0]       find_tdata;
  wire              find_tvalid, find_tready;

  wire [DEST_W-1:0] dest_i_tdata;
  wire              dest_i_tkeep, dest_i_tvalid, dest_i_tready;
  wire [DEST_W-1:0] dest_o_tdata;
  wire              dest_o_tkeep, dest_o_tvalid, dest_o_tready;

  // The find_fifo holds the lookup requests from the find_* AXI stream and
  // sends them on to the m_axis_find_* stream port. It is required because the
  // input logic (see below) doesn't obey the AXI handshake protocol but this
  // FIFO can tolerate it.
  axi_fifo #(
    .WIDTH (16),
    .SIZE  (1)
  ) find_fifo_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  (find_tdata),
    .i_tvalid (find_tvalid),
    .i_tready (find_tready),
    .o_tdata  (m_axis_find_tdata),
    .o_tvalid (m_axis_find_tvalid),
    .o_tready (m_axis_find_tready),
    .space    (),
    .occupied ()
  );

  // The destination (TDEST) can come from two sources: Directly from the
  // packet info (in which case TDEST was immediately determined and comes in
  // on dest_* AXI stream) or via a lookup (in which case the result comes in
  // on s_axis_result_*). Only one of these data paths is used at a time, so we
  // mux them together here create a single stream (dest_o_*) that contains the
  // destination for the next packet.
  axi_mux #(
    .WIDTH          (DEST_W+1),
    .SIZE           (2),
    .PRIO           (1),
    .PRE_FIFO_SIZE  (1),
    .POST_FIFO_SIZE (1)
  ) dest_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({dest_i_tkeep, dest_i_tdata, 
                s_axis_result_tkeep, s_axis_result_tdata}),
    .i_tlast  (2'b11),
    .i_tvalid ({dest_i_tvalid, s_axis_result_tvalid}),
    .i_tready ({dest_i_tready, s_axis_result_tready}),
    .o_tdata  ({dest_o_tkeep, dest_o_tdata}),
    .o_tlast  (),
    .o_tvalid (dest_o_tvalid),
    .o_tready (dest_o_tready)
  );


  //---------------------------------------------------------------------------
  // Input Logic
  //---------------------------------------------------------------------------
  //
  // When a packet comes in, we may have to do one of the following:
  //   1) Lookup the TDEST using the EPID
  //   2) Use the specified input TDEST
  //   3) Use the NODE_ID as the TDEST (to return the packet)
  //
  //---------------------------------------------------------------------------

  // The s_axis_chdr_hdr_valid signal indicates when TDATA and TID contain the
  // header information for the current packet.
  reg s_axis_chdr_hdr_valid = 1'b1;

  always @(posedge clk) begin
    if (reset) begin
      s_axis_chdr_hdr_valid <= 1'b1;
    end else if (s_axis_chdr_tvalid & s_axis_chdr_tready) begin
      s_axis_chdr_hdr_valid <= s_axis_chdr_tlast;
    end
  end

  // The dest_find_tready signal indicates if the find_fifo is ready or if the
  // dest port of the dest_muax is ready, depending on which path will be used.
  reg dest_find_tready;

  always @(*) begin
    if (s_axis_chdr_hdr_valid) begin
      case (s_axis_chdr_tid)
        CHDR_MGMT_ROUTE_EPID:
          dest_find_tready = find_tready;
        CHDR_MGMT_ROUTE_TDEST:
          dest_find_tready = dest_i_tready;
        CHDR_MGMT_RETURN_TO_SRC:
          dest_find_tready = dest_i_tready;
        default:
          dest_find_tready = dest_i_tready; // We should never get here
      endcase
    end else begin
      dest_find_tready = 1'b1;
    end
  end

  // We can accept a transfer from the input CHDR stream only if the the packet
  // gate and dest/find datapaths are ready.
  assign s_axis_chdr_tready = s_axis_chdr_tvalid &&
                              gate_i_tready      &&
                              dest_find_tready;

  // The chdr_header_stb signal indicates when we write data into the dest/find
  // data path. This happens when we're accepting the header word of the packet
  // into the packet gate.
  wire chdr_header_stb = s_axis_chdr_tvalid &&
                         s_axis_chdr_tready &&
                         s_axis_chdr_hdr_valid;

  // **************************************************************************
  // WARNING: The logic below violates AXI-Stream by having a tready -> tvalid
  //          dependency To ensure no deadlocks, we must place FIFOs downstream
  //          of gate_i_*, find_* and dest_i_*

  // Here we decide if we need to do a lookup using the find_* path or if the
  // destination is known and can be put directly on the dest_* path.
  //
  // Start a lookup request if the TID is CHDR_MGMT_ROUTE_EPID.
  assign find_tdata  = chdr_get_dst_epid(s_axis_chdr_tdata[63:0]);
  assign find_tvalid = chdr_header_stb && 
                       (s_axis_chdr_tid == CHDR_MGMT_ROUTE_EPID);
  // Set TDEST directly if TID is CHDR_MGMT_ROUTE_TDEST or
  // CHDR_MGMT_RETURN_TO_SRC.
  assign dest_i_tdata  = (s_axis_chdr_tid == CHDR_MGMT_ROUTE_TDEST) ? 
                         s_axis_chdr_tdest : NODE_ID[DEST_W-1:0];
  assign dest_i_tkeep  = 1'b1;
  assign dest_i_tvalid = chdr_header_stb && 
                         (s_axis_chdr_tid != CHDR_MGMT_ROUTE_EPID);

  // Input logic for axi_packet_gate
  assign gate_i_tdata  = s_axis_chdr_tdata;
  assign gate_i_tlast  = s_axis_chdr_tlast;
  assign gate_i_tvalid = s_axis_chdr_tready && s_axis_chdr_tvalid;

  //
  // **************************************************************************


  //---------------------------------------------------------------------------
  // Output Logic
  //---------------------------------------------------------------------------
  //
  // The destination for the packet (TDEST) must be valid before we allow the
  // header of the packet to pass through. So the packet must be blocked until
  // the output of the dest_o_* is valid. TDEST and TKEEP must remain valid
  // until the end of the packet.
  //
  //---------------------------------------------------------------------------

  assign m_axis_chdr_tdata  = gate_o_tdata;
  assign m_axis_chdr_tlast  = gate_o_tlast;
  assign m_axis_chdr_tdest  = dest_o_tdata;
  assign m_axis_chdr_tkeep  = dest_o_tkeep;
  assign m_axis_chdr_tvalid = gate_o_tvalid && dest_o_tvalid;

  assign gate_o_tready = m_axis_chdr_tvalid && m_axis_chdr_tready;
  assign dest_o_tready = m_axis_chdr_tvalid && m_axis_chdr_tready && m_axis_chdr_tlast;

endmodule

