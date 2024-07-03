//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_ta_chdr_dma
//
// Description:
//
//   Top-level file for the CHDR DMA transport adapter.
//
//   This is the transport adapter for streaming for PL into PS and vice versa.
//   Note that this is a passthrough TA, because the DMA engine is still
//   implemented outside the image core.
//
// Parameters:
//
//   CHDR_W     : CHDR width used by RFNoC on the FPGA
//   PROTOVER   : RFNoC protocol version for IPv4 interface
//

module rfnoc_ta_chdr_dma #(
  parameter        CHDR_W     = 64,
  parameter [15:0] PROTOVER   = {8'd1, 8'd0}
) (
  // Standard Clocks and Resets (these are unused in this particular module)
  input  logic core_arst,
  input  logic rfnoc_ctrl_clk,
  input  logic rfnoc_ctrl_rst,
  input  logic rfnoc_chdr_clk,
  input  logic rfnoc_chdr_rst,

  // CHDR Buses (to and from crossbar)
  output logic [4*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  output logic [         3:0] s_rfnoc_chdr_tlast,
  output logic [         3:0] s_rfnoc_chdr_tvalid,
  input  logic [         3:0] s_rfnoc_chdr_tready,

  input  logic [4*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  input  logic [         3:0] m_rfnoc_chdr_tlast,
  input  logic [         3:0] m_rfnoc_chdr_tvalid,
  output logic [         3:0] m_rfnoc_chdr_tready,

  // CHDR Buses (to and from DMA engine)
  input  logic [4*CHDR_W-1:0] s_dma_tdata,
  input  logic [         3:0] s_dma_tlast,
  input  logic [         3:0] s_dma_tvalid,
  output logic [         3:0] s_dma_tready,

  output logic [4*CHDR_W-1:0] m_dma_tdata,
  output logic [         3:0] m_dma_tlast,
  output logic [         3:0] m_dma_tvalid,
  input  logic [         3:0] m_dma_tready
);
  `default_nettype none

  assign s_rfnoc_chdr_tdata  = s_dma_tdata;
  assign s_rfnoc_chdr_tlast  = s_dma_tlast;
  assign s_rfnoc_chdr_tvalid = s_dma_tvalid;
  assign s_dma_tready        = s_rfnoc_chdr_tready;

  assign m_dma_tdata         = m_rfnoc_chdr_tdata;
  assign m_dma_tlast         = m_rfnoc_chdr_tlast;
  assign m_dma_tvalid        = m_rfnoc_chdr_tvalid;
  assign m_rfnoc_chdr_tready = m_dma_tready;

  `default_nettype wire
endmodule : rfnoc_ta_chdr_dma

