//
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: nirio_chdr64_adapter
//
// Description: A transport adapter specific to connecting an NI-RIO streaming
//   interface to CHDR. It assumes to be connected to x300_pcie_int.
//   See also chdr_xport_adapter_generic.
//
//   The tuser inputs/outputs used for routing are the index of the DMA channel.
//   Because we have 6 DMA channels on NI-RIO on the X300, these are always 3
//   bits wide.
//
// Parameters:
//   - PROTOVER: RFNoC protocol version {8'd<major>, 8'd<minor>}
//   - MTU: Log2 of the MTU of the packet in 64-bit words
//   - RT_TBL_SIZE: Log2 of the depth of the return-address routing table
//   - NODE_INST: The node type to return for a node-info discovery
//
// Signals:
//   - device_id : The ID of the device that has instantiated this module
//   - s_dma_*: The input Ethernet stream from the MAC (plus tuser for source DMA engine ID)
//   - m_dma_*: The output Ethernet stream to the MAC (plus tuser for dest DMA engine ID)
//   - s_chdr_*: The input CHDR stream from the rfnoc infrastructure
//   - m_chdr_*: The output CHDR stream to the rfnoc infrastructure
//

module nirio_chdr64_adapter #(
  parameter [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter        MTU              = 10,
  parameter        RT_TBL_SIZE      = 6,
  parameter        NODE_INST        = 0,
  parameter        DMA_ID_WIDTH     = 3
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

  `include "../../lib/rfnoc/core/rfnoc_chdr_utils.vh"
  `include "../../lib/rfnoc/core/rfnoc_chdr_internal_utils.vh"
  `include "../../lib/rfnoc/xport/rfnoc_xport_types.vh"

  //---------------------------------------
  // CHDR Transport Adapter
  //---------------------------------------

  chdr_xport_adapter_generic #(
    .PROTOVER     (PROTOVER),
    .CHDR_W       (64),
    .USER_W       (DMA_ID_WIDTH),
    .TBL_SIZE     (RT_TBL_SIZE),
    .NODE_SUBTYPE (NODE_SUBTYPE_XPORT_NIRIO_CHDR64),
    .NODE_INST    (NODE_INST),
    .ALLOW_DISC   (0)
  ) xport_adapter_gen_i (
    .clk                 (clk),
    .rst                 (rst),
    .device_id           (device_id),
    .s_axis_xport_tdata  (s_dma_tdata),
    .s_axis_xport_tuser  (s_dma_tuser),
    .s_axis_xport_tlast  (s_dma_tlast),
    .s_axis_xport_tvalid (s_dma_tvalid),
    .s_axis_xport_tready (s_dma_tready),
    .m_axis_xport_tdata  (m_dma_tdata),
    .m_axis_xport_tuser  (m_dma_tuser),
    .m_axis_xport_tlast  (m_dma_tlast),
    .m_axis_xport_tvalid (m_dma_tvalid),
    .m_axis_xport_tready (m_dma_tready),
    .s_axis_rfnoc_tdata  (s_chdr_tdata),
    .s_axis_rfnoc_tlast  (s_chdr_tlast),
    .s_axis_rfnoc_tvalid (s_chdr_tvalid),
    .s_axis_rfnoc_tready (s_chdr_tready),
    .m_axis_rfnoc_tdata  (m_chdr_tdata),
    .m_axis_rfnoc_tlast  (m_chdr_tlast),
    .m_axis_rfnoc_tvalid (m_chdr_tvalid),
    .m_axis_rfnoc_tready (m_chdr_tready),
    .ctrlport_req_wr     (/* unused */),
    .ctrlport_req_rd     (/* unused */),
    .ctrlport_req_addr   (/* unused */),
    .ctrlport_req_data   (/* unused */),
    .ctrlport_resp_ack   (/* unused */ 1'b0),
    .ctrlport_resp_data  (/* unused */ 32'd0)
  );

endmodule
