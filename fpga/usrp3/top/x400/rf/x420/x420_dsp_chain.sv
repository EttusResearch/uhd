//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x420_dsp_chain
//
// Description:
//
//   Combines the ctrlport handling, IQ impairment correction, and DC offset
//   compensation for a single TX and RX channel. This module is shared
//   between rf_core_1000m and rf_core_400m_x420.
//
//   TX path: impairment_correction -> dc_offset
//   RX path: dc_offset -> impairment_correction
//

module x420_dsp_chain
  import ctrlport_pkg::*;
  import XmlSvPkgRF_CORE_REGMAP::*;
#(
  parameter int RADIO_SPC = 4
) (

  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------
  input  logic clk,
  input  logic ctrlport_rst,

  //---------------------------------------------------------------------------
  // Control-Port Interface (clk domain)
  //---------------------------------------------------------------------------
  input  logic                       s_ctrlport_req_wr,
  input  logic                       s_ctrlport_req_rd,
  input  logic [CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  logic [CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  output logic                       s_ctrlport_resp_ack,
  output logic [ CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output logic [CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // TX Data Interface (clk domain)
  //---------------------------------------------------------------------------
  input  logic [32*RADIO_SPC-1:0] tx_in_axis_tdata,
  input  logic                    tx_in_axis_tvalid,
  output logic                    tx_in_axis_tready,

  output logic [32*RADIO_SPC-1:0] tx_out_axis_tdata,
  output logic                    tx_out_axis_tvalid,
  input  logic                    tx_out_axis_tready,

  //---------------------------------------------------------------------------
  // RX Data Interface (clk domain)
  //---------------------------------------------------------------------------
  input  logic [32*RADIO_SPC-1:0] rx_in_axis_tdata,
  input  logic                    rx_in_axis_tvalid,

  output logic [32*RADIO_SPC-1:0] rx_out_axis_tdata,
  output logic                    rx_out_axis_tvalid,
  input  logic                    rx_out_axis_tready
);

  //---------------------------------------------------------------------------
  // Ctrlport Interface Network
  //---------------------------------------------------------------------------
  ctrlport_if s_ctrlport       (.clk(clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_tx    (.clk(clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_rx    (.clk(clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_tx_dc (.clk(clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_rx_dc (.clk(clk), .rst(ctrlport_rst));

  // translate ctrlport interface to/from signals
  always_comb begin
    // request
    s_ctrlport.req.wr      = s_ctrlport_req_wr;
    s_ctrlport.req.rd      = s_ctrlport_req_rd;
    s_ctrlport.req.addr    = s_ctrlport_req_addr;
    s_ctrlport.req.data    = s_ctrlport_req_data;
    // response
    s_ctrlport_resp_ack    = s_ctrlport.resp.ack;
    s_ctrlport_resp_status = s_ctrlport.resp.status;
    s_ctrlport_resp_data   = s_ctrlport.resp.data;
  end

  // Split ctrlport for TX and RX
  ctrlport_if_decoder #(
    .NUM_SLAVES (4),
    .PORT_BASE  ('{kTX_IQ_IMPAIRMENTS,     kRX_IQ_IMPAIRMENTS,     kTX_DC_OFFSET,     kRX_DC_OFFSET}),
    .PORT_SIZE  ('{kTX_IQ_IMPAIRMENTSSize, kRX_IQ_IMPAIRMENTSSize, kTX_DC_OFFSETSize, kRX_DC_OFFSETSize})
  ) ctrlport_decoder_inst (
    .s_ctrlport (s_ctrlport.slave),
    .m_ctrlport ('{s_ctrlport_tx.master, s_ctrlport_rx.master, s_ctrlport_tx_dc.master, s_ctrlport_rx_dc.master})
  );

  //---------------------------------------------------------------------------
  // TX Path: impairment_correction -> dc_offset
  //---------------------------------------------------------------------------
  logic [32*RADIO_SPC-1:0] dac_imp_corr_tdata;
  logic                    dac_imp_corr_tvalid;
  logic                    dac_imp_corr_tready;

  impairment_correction #(
    .NUM_SPC    (RADIO_SPC),
    .NUM_COEFFS (15)
  ) impairment_correction_tx (
    .clk            (clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (tx_in_axis_tdata),
    .s_axis_tvalid  (tx_in_axis_tvalid),
    .s_axis_tready  (tx_in_axis_tready),
    .m_axis_tdata   (dac_imp_corr_tdata),
    .m_axis_tvalid  (dac_imp_corr_tvalid),
    .m_axis_tready  (dac_imp_corr_tready),
    .s_ctrlport     (s_ctrlport_tx.slave)
  );

  dc_offset #(
    .NUM_SPC    (RADIO_SPC)
  ) dc_offset_compensation_tx (
    .clk            (clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (dac_imp_corr_tdata),
    .s_axis_tvalid  (dac_imp_corr_tvalid),
    .s_axis_tready  (dac_imp_corr_tready),
    .s_axis_tlast   ('0),
    .m_axis_tdata   (tx_out_axis_tdata),
    .m_axis_tvalid  (tx_out_axis_tvalid),
    .m_axis_tready  (tx_out_axis_tready),
    .m_axis_tlast   (),
    .s_ctrlport     (s_ctrlport_tx_dc.slave)
  );

  //---------------------------------------------------------------------------
  // RX Path: dc_offset -> impairment_correction
  //---------------------------------------------------------------------------
  logic [32*RADIO_SPC-1:0] adc_dc_offset_tdata;
  logic                    adc_dc_offset_tvalid;
  logic                    adc_dc_offset_tready;

  dc_offset #(
    .NUM_SPC    (RADIO_SPC)
  ) dc_offset_compensation_rx (
    .clk            (clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (rx_in_axis_tdata),
    .s_axis_tvalid  (rx_in_axis_tvalid),
    .s_axis_tready  (),
    .s_axis_tlast   ('0),
    .m_axis_tdata   (adc_dc_offset_tdata),
    .m_axis_tvalid  (adc_dc_offset_tvalid),
    .m_axis_tready  (adc_dc_offset_tready),
    .m_axis_tlast   (),
    .s_ctrlport     (s_ctrlport_rx_dc.slave)
  );

  impairment_correction #(
    .NUM_SPC    (RADIO_SPC),
    .NUM_COEFFS (15)
  ) impairment_correction_rx (
    .clk            (clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (adc_dc_offset_tdata),
    .s_axis_tvalid  (adc_dc_offset_tvalid),
    .s_axis_tready  (adc_dc_offset_tready),
    .m_axis_tdata   (rx_out_axis_tdata),
    .m_axis_tvalid  (rx_out_axis_tvalid),
    .m_axis_tready  (rx_out_axis_tready),
    .s_ctrlport     (s_ctrlport_rx.slave)
  );

endmodule

//XmlParse xml_on
//<regmap name="RF_CORE_REGMAP">
//  <group name="IQ_IMPAIRMENT_WINDOWS">
//    <window name="TX_IQ_IMPAIRMENTS" offset="0x00" size="0x20" targetregmap="IQ_IMPAIRMENT_REGMAP">
//      <info>Interface for IQ impairments DSP in the TX data path.</info>
//    </window>
//    <window name="RX_IQ_IMPAIRMENTS" offset="0x20" size="0x20" targetregmap="IQ_IMPAIRMENT_REGMAP">
//      <info>Interface for IQ impairments DSP in the RX data path.</info>
//    </window>
//    <window name="TX_DC_OFFSET" offset="0x40" size="0x10" targetregmap="DC_OFFSET_REGMAP">
//      <info>Interface for DC offset correction block in the TX data path.</info>
//    </window>
//    <window name="RX_DC_OFFSET" offset="0x50" size="0x10" targetregmap="DC_OFFSET_REGMAP">
//      <info>Interface for DC offset correction block in the RX data path.</info>
//    </window>
//  </group>
//</regmap>
//XmlParse xml_off
