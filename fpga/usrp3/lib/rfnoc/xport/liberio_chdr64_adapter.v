//
// Copyright 2019 Ettus Research, A National Instruments brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: liberio_chdr64_adapter
// Description: The transport adapter for a liberio transport with CHDR_W of
//   64. A tuser field is used to identify the DMA engine for return routes.
//   The stream for a given SrcEPID with ID s_dma_tuser will return packets to
//   that EPID with same ID on m_dma_tuser (after an Advertise management
//   operation).
//
// Parameters:
//   - PROTOVER: RFNoC protocol version {8'd<major>, 8'd<minor>}
//   - RT_TBL_SIZE: Log2 of the depth of the return-address routing table
//   - NODE_INST: The node type to return for a node-info discovery
//   - DMA_ID_WIDTH: The width of the tuser signal that identifies the DMA engine)
//
// Signals:
//   - device_id : The ID of the device that has instantiated this module
//   - s_dma_*: The input DMA stream from the CPU (plus tuser for source DMA engine ID)
//   - m_dma_*: The output DMA stream to the CPU (plus tuser for dest DMA engine ID)
//   - s_chdr_*: The input CHDR stream from the rfnoc infrastructure
//   - m_chdr_*: The output CHDR stream to the rfnoc infrastructure
//

module liberio_chdr64_adapter #(
  parameter [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter        RT_TBL_SIZE      = 6,
  parameter        NODE_INST        = 0,
  parameter        DMA_ID_WIDTH     = 8
)(
  // Clocking and reset interface
  input  wire        clk,
  input  wire        rst,
  // Device info
  input  wire [15:0] device_id,
  // AXI-Stream interface to/from DMA engines
  input  wire [63:0]              s_dma_tdata,
  input  wire [DMA_ID_WIDTH-1:0]  s_dma_tuser,
  input  wire                     s_dma_tlast,
  input  wire                     s_dma_tvalid,
  output wire                     s_dma_tready,
  output wire [63:0]              m_dma_tdata,
  output wire [DMA_ID_WIDTH-1:0]  m_dma_tuser,
  output wire                     m_dma_tlast,
  output wire                     m_dma_tvalid,
  input  wire                     m_dma_tready,
  // AXI-Stream interface to/from CHDR infrastructure
  input  wire [63:0] s_chdr_tdata,
  input  wire        s_chdr_tlast,
  input  wire        s_chdr_tvalid,
  output wire        s_chdr_tready,
  output wire [63:0] m_chdr_tdata,
  output wire        m_chdr_tlast,
  output wire        m_chdr_tvalid,
  input  wire        m_chdr_tready
);

  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"
  `include "rfnoc_xport_types.vh"

  //---------------------------------------
  // CHDR Transport Adapter
  //---------------------------------------
  wire [DMA_ID_WIDTH-1:0] m_axis_xport_tuser;

  chdr_xport_adapter_generic #(
    .PROTOVER(PROTOVER), .CHDR_W(64),
    .USER_W(DMA_ID_WIDTH), .TBL_SIZE(RT_TBL_SIZE),
    .NODE_SUBTYPE(NODE_SUBTYPE_XPORT_LIBERIO_CHDR64), .NODE_INST(NODE_INST)
  ) xport_adapter_gen_i (
    .clk                (clk),
    .rst                (rst),
    .device_id          (device_id),
    .s_axis_xport_tdata (s_dma_tdata),
    .s_axis_xport_tuser (s_dma_tuser),
    .s_axis_xport_tlast (s_dma_tlast),
    .s_axis_xport_tvalid(s_dma_tvalid),
    .s_axis_xport_tready(s_dma_tready),
    .m_axis_xport_tdata (m_dma_tdata),
    .m_axis_xport_tuser (m_axis_xport_tuser),
    .m_axis_xport_tlast (m_dma_tlast),
    .m_axis_xport_tvalid(m_dma_tvalid),
    .m_axis_xport_tready(m_dma_tready),
    .s_axis_rfnoc_tdata (s_chdr_tdata),
    .s_axis_rfnoc_tlast (s_chdr_tlast),
    .s_axis_rfnoc_tvalid(s_chdr_tvalid),
    .s_axis_rfnoc_tready(s_chdr_tready),
    .m_axis_rfnoc_tdata (m_chdr_tdata),
    .m_axis_rfnoc_tlast (m_chdr_tlast),
    .m_axis_rfnoc_tvalid(m_chdr_tvalid),
    .m_axis_rfnoc_tready(m_chdr_tready),
    .ctrlport_req_wr    (/* unused */),
    .ctrlport_req_rd    (/* unused */),
    .ctrlport_req_addr  (/* unused */),
    .ctrlport_req_data  (/* unused */),
    .ctrlport_resp_ack  (/* unused */ 1'b0),
    .ctrlport_resp_data (/* unused */ 32'd0)
  );

  // Ensure tdest does not change for entire packet
  reg m_hdr = 1'b1;
  always @(posedge clk) begin
    if (rst)
      m_hdr <= 1'b1;
    else if (m_dma_tvalid && m_dma_tready)
      m_hdr <= m_dma_tlast;
  end

  reg [DMA_ID_WIDTH-1:0] cached_dest = {DMA_ID_WIDTH{1'b0}};
  always @(posedge clk) begin
    if (m_hdr)
      cached_dest <= m_axis_xport_tuser;
  end

  assign m_dma_tuser = m_hdr ? m_axis_xport_tuser : cached_dest;

endmodule // liberio_chdr64_adapter
