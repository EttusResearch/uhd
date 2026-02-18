//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_core_1000m
//
// Description:
//
//   Top-level wrapper for data reordering from RFDC to RFNoC and vice versa.
//   This module includes signal processing for IQ impairments and DC offset.
//   It supports a single data channel in TX and RX direction with a
//   configurable number of samples per clock.
//
//   Data/RF Specs:
//     DBs:   1
//     Data Rate: rfdc_clk @ RADIO_SPC
//

module rf_core_1000m
  import ctrlport_pkg::*;
# (
  parameter  int RADIO_SPC = 8,
  localparam int NUM_ADC_CHANNELS = 1,
  localparam int NUM_DAC_CHANNELS = 1
) (

  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------

  // Main Clock Inputs
  input  logic rfdc_clk,

  // AXI4-Lite Config Clock
  // This clock is used to synchronize status bits for the RFDC
  // registers in the AXI-S clock domain.
  input  logic s_axi_config_clk,

  //---------------------------------------------------------------------------
  // Control-Port Interface (rfdc_clk domain)
  //---------------------------------------------------------------------------
  input  logic                       ctrlport_rst,
  input  logic                       s_ctrlport_req_wr,
  input  logic                       s_ctrlport_req_rd,
  input  logic [CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  logic [CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  output logic                       s_ctrlport_resp_ack,
  output logic [ CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output logic [CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // RFDC Data Interfaces
  //---------------------------------------------------------------------------
  // All ports here are in the rfdc_clk domain.

  // ADC
  input  logic [16*RADIO_SPC-1:0]     adc_data_in_i_tdata [0:NUM_ADC_CHANNELS-1],
  output logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready,
  input  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tvalid,
  input  logic [16*RADIO_SPC-1:0]     adc_data_in_q_tdata [0:NUM_ADC_CHANNELS-1],
  output logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready,
  input  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tvalid,

  // DAC
  output logic [32*RADIO_SPC-1:0]     dac_data_out_tdata [0:NUM_DAC_CHANNELS-1],
  input  logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tready,
  output logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tvalid,

  //---------------------------------------------------------------------------
  // User Data Interfaces
  //---------------------------------------------------------------------------
  // All ports here are in the rfdc_clk domain on the X420.

  // ADC
  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output logic [32*RADIO_SPC-1:0]     adc_data_out_tdata [0:NUM_ADC_CHANNELS-1],
  output logic [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid,

  // DAC
  input  logic [32*RADIO_SPC-1:0]     dac_data_in_tdata [0:NUM_DAC_CHANNELS-1],
  output logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tready,
  input  logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tvalid,

  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------
  // Invert I/Q control signals from RFDC to DSP chain.
  input  logic [NUM_ADC_CHANNELS-1:0] invert_adc_iq_rclk,
  input  logic [NUM_DAC_CHANNELS-1:0] invert_dac_iq_rclk,

  // Control/status vectors from/to RFDC.
  // Notice these are all in the s_axi_config_clk domain.
  output logic [ 9:0] dsp_info_sclk,
  output logic [15:0] axi_status_sclk,
  output logic [15:0] rfdc_info_sclk,

  // Enable signal
  input logic adc_enable_data_rclk,
  input logic adc_rfdc_axi_resetn_rclk,

  // Version (Constant)
  output logic [95:0] version_info
);

  import XmlSvPkgRF_CORE_REGMAP::*;
  `include "../../regmap/x420/versioning_regs_regmap_utils.vh"
  `include "../../regmap/versioning_utils.vh"

  //---------------------------------------------------------------------------
  // RFDC conversion
  //---------------------------------------------------------------------------
  // ADC
  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  logic [32*RADIO_SPC-1:0]     adc_data_int_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_int_tvalid;

  // DAC
  logic [32*RADIO_SPC-1:0]     dac_data_int_tdata [0:NUM_DAC_CHANNELS-1];
  logic [NUM_DAC_CHANNELS-1:0] dac_data_int_tready;
  logic [NUM_DAC_CHANNELS-1:0] dac_data_int_tvalid;

  rf_core_full #(
    .NUM_ADC_CHANNELS(NUM_ADC_CHANNELS),
    .NUM_DAC_CHANNELS(NUM_DAC_CHANNELS),
    .RADIO_SPC(RADIO_SPC)
  ) rf_core_full_i (
    .rfdc_clk                 (rfdc_clk),
    .s_axi_config_clk         (s_axi_config_clk),
    .adc_data_in_i_tdata      (adc_data_in_i_tdata),
    .adc_data_in_i_tready     (adc_data_in_i_tready),
    .adc_data_in_i_tvalid     (adc_data_in_i_tvalid),
    .adc_data_in_q_tdata      (adc_data_in_q_tdata),
    .adc_data_in_q_tready     (adc_data_in_q_tready),
    .adc_data_in_q_tvalid     (adc_data_in_q_tvalid),
    .dac_data_out_tdata       (dac_data_out_tdata),
    .dac_data_out_tready      (dac_data_out_tready),
    .dac_data_out_tvalid      (dac_data_out_tvalid),
    .adc_data_out_tdata       (adc_data_int_tdata),
    .adc_data_out_tvalid      (adc_data_int_tvalid),
    .dac_data_in_tdata        (dac_data_int_tdata),
    .dac_data_in_tready       (dac_data_int_tready),
    .dac_data_in_tvalid       (dac_data_int_tvalid),
    .invert_adc_iq_rclk       (invert_adc_iq_rclk),
    .invert_dac_iq_rclk       (invert_dac_iq_rclk),
    .dsp_info_sclk            (dsp_info_sclk),
    .rfdc_info_sclk           (rfdc_info_sclk),
    .axi_status_sclk          (axi_status_sclk),
    .adc_enable_data_rclk     (adc_enable_data_rclk),
    .adc_rfdc_axi_resetn_rclk (adc_rfdc_axi_resetn_rclk),
    .version_info             ()
  );

  //---------------------------------------------------------------------------
  // Ctrlport Interface Network
  //---------------------------------------------------------------------------
  // convert to ctrlport interface
  ctrlport_if s_ctrlport       (.clk(rfdc_clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_tx    (.clk(rfdc_clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_rx    (.clk(rfdc_clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_tx_dc (.clk(rfdc_clk), .rst(ctrlport_rst));
  ctrlport_if s_ctrlport_rx_dc (.clk(rfdc_clk), .rst(ctrlport_rst));

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
  // IQ Impairment Correction
  //---------------------------------------------------------------------------
  // Implementation is currently handling a single channel only for X420.
  // Therefore the data paths are using the static index 0 below.

  // -------- TX ----------

  // Intermediate signals for TX path
  logic [32*RADIO_SPC-1:0] dac_imp_corr_tdata;
  logic                    dac_imp_corr_tvalid;
  logic                    dac_imp_corr_tready;

  impairment_correction #(
    .NUM_SPC    (RADIO_SPC),
    .NUM_COEFFS (15)
  ) impairment_correction_tx (
    .clk            (rfdc_clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (dac_data_in_tdata[0]),
    .s_axis_tvalid  (dac_data_in_tvalid),
    .s_axis_tready  (dac_data_in_tready),
    .m_axis_tdata   (dac_imp_corr_tdata),
    .m_axis_tvalid  (dac_imp_corr_tvalid),
    .m_axis_tready  (dac_imp_corr_tready),
    .s_ctrlport     (s_ctrlport_tx.slave)
  );

  dc_offset #(
    .NUM_SPC    (RADIO_SPC)
  ) dc_offset_compensation_tx (
    .clk            (rfdc_clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (dac_imp_corr_tdata),
    .s_axis_tvalid  (dac_imp_corr_tvalid),
    .s_axis_tready  (dac_imp_corr_tready),
    .s_axis_tlast   ('0),
    .m_axis_tdata   (dac_data_int_tdata[0]),
    .m_axis_tvalid  (dac_data_int_tvalid),
    .m_axis_tready  (dac_data_int_tready),
    .m_axis_tlast   (),
    .s_ctrlport     (s_ctrlport_tx_dc.slave)
  );

  // -------- RX ----------

  // Intermediate signals for RX path
  logic [32*RADIO_SPC-1:0] adc_dc_offset_tdata;
  logic                    adc_dc_offset_tvalid;
  logic                    adc_dc_offset_tready;

  dc_offset #(
    .NUM_SPC    (RADIO_SPC)
  ) dc_offset_compensation_rx (
    .clk            (rfdc_clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (adc_data_int_tdata[0]),
    .s_axis_tvalid  (adc_data_int_tvalid),
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
    .clk            (rfdc_clk),
    .reset          (s_ctrlport.rst),
    .s_axis_tdata   (adc_dc_offset_tdata),
    .s_axis_tvalid  (adc_dc_offset_tvalid),
    .s_axis_tready  (adc_dc_offset_tready),
    .m_axis_tdata   (adc_data_out_tdata[0]),
    .m_axis_tvalid  (adc_data_out_tvalid),
    .m_axis_tready  ('1),
    .s_ctrlport     (s_ctrlport_rx.slave)
  );

  // check version info from rf_core_full
  if (build_version(
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MAJOR,
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MINOR,
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_BUILD
    ) != build_version(1,0,0))
    $error("rf_core_1000m: Incompatible rf_core_full version detected!");

  // assign version info
  assign version_info = build_component_versions(
    RF_CORE_1000M_VERSION_LAST_MODIFIED_TIME,
    build_version(
      RF_CORE_1000M_OLDEST_COMPATIBLE_VERSION_MAJOR,
      RF_CORE_1000M_OLDEST_COMPATIBLE_VERSION_MINOR,
      RF_CORE_1000M_OLDEST_COMPATIBLE_VERSION_BUILD
    ),
    build_version(
      RF_CORE_1000M_CURRENT_VERSION_MAJOR,
      RF_CORE_1000M_CURRENT_VERSION_MINOR,
      RF_CORE_1000M_CURRENT_VERSION_BUILD
    )
  );

endmodule


//XmlParse xml_on
//<regmap name="VERSIONING_REGS_REGMAP">
//  <group name="VERSIONING_CONSTANTS">
//    <enumeratedtype name="RF_CORE_1000M_VERSION" showhex="true">
//      <info>
//        1000 MHz RF core.{BR/}
//        For guidance on when to update these revision numbers,
//        please refer to the register map documentation accordingly:
//        <li> Current version: @.VERSIONING_REGS_REGMAP..CURRENT_VERSION
//        <li> Oldest compatible version: @.VERSIONING_REGS_REGMAP..OLDEST_COMPATIBLE_VERSION
//        <li> Version last modified: @.VERSIONING_REGS_REGMAP..VERSION_LAST_MODIFIED
//      </info>
//      <value name="RF_CORE_1000M_CURRENT_VERSION_MAJOR"           integer="2"/>
//      <value name="RF_CORE_1000M_CURRENT_VERSION_MINOR"           integer="0"/>
//      <value name="RF_CORE_1000M_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="RF_CORE_1000M_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="2"/>
//      <value name="RF_CORE_1000M_OLDEST_COMPATIBLE_VERSION_MINOR" integer="0"/>
//      <value name="RF_CORE_1000M_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="RF_CORE_1000M_VERSION_LAST_MODIFIED_TIME"      integer="0x25120310"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//
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
