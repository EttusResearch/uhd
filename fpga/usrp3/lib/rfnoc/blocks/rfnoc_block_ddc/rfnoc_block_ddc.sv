//
// Copyright 2019 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc
//
// Description:  An digital down-converter block for RFNoC.
//
// Parameters:
//
//   THIS_PORTID   : Control crossbar port to which this block is connected
//   CHDR_W        : AXIS CHDR interface data width
//   NUM_PORTS     : Number of DDCs to instantiate
//   MTU           : Maximum transmission unit (i.e., maximum packet size) in
//                   CHDR words is 2**MTU.
//   NUM_HB        : Number of half-band decimation blocks to include (0-3)
//   CIC_MAX_DECIM : Maximum decimation to support in the CIC filter
//   NIPC          : If > 1, instantiate DDC capable of processing multiple items per clock cycle (NIPC).
//

module rfnoc_block_ddc #(
  parameter THIS_PORTID   = 0,
  parameter CHDR_W        = 64,
  parameter NUM_PORTS     = 2,
  parameter MTU           = 10,
  parameter NUM_HB        = 3,
  parameter CIC_MAX_DECIM = 255,
  parameter NIPC          = 1
) (
  //---------------------------------------------------------------------------
  // AXIS CHDR Port
  //---------------------------------------------------------------------------

  input wire rfnoc_chdr_clk,
  input wire ce_clk,

  // CHDR inputs from framework
  input  wire [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,

  // CHDR outputs to framework
  output wire [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tready,

  // Backend interface
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status,

  //---------------------------------------------------------------------------
  // AXIS CTRL Port
  //---------------------------------------------------------------------------

  input wire rfnoc_ctrl_clk,

  // CTRL port requests from framework
  input  wire [31:0] s_rfnoc_ctrl_tdata,
  input  wire        s_rfnoc_ctrl_tlast,
  input  wire        s_rfnoc_ctrl_tvalid,
  output wire        s_rfnoc_ctrl_tready,

  // CTRL port requests to framework
  output wire [31:0] m_rfnoc_ctrl_tdata,
  output wire        m_rfnoc_ctrl_tlast,
  output wire        m_rfnoc_ctrl_tvalid,
  input  wire        m_rfnoc_ctrl_tready
);

  // Derived parameter: select multisample (NIPC > 1) vs single-sample (NIPC == 1)
  localparam MULTI_SPC_SUPPORT = (NIPC > 1);

  generate
    if (MULTI_SPC_SUPPORT == 0) begin : gen_single_spc

      rfnoc_block_ddc_ss #(
        .THIS_PORTID   (THIS_PORTID),
        .CHDR_W        (CHDR_W),
        .NUM_PORTS     (NUM_PORTS),
        .MTU           (MTU),
        .NUM_HB        (NUM_HB),
        .CIC_MAX_DECIM (CIC_MAX_DECIM)
      ) rfnoc_block_ddc_ss_i (
        .rfnoc_chdr_clk      (rfnoc_chdr_clk),
        .ce_clk              (ce_clk),
        .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
        .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
        .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
        .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
        .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
        .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
        .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
        .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
        .rfnoc_core_config   (rfnoc_core_config),
        .rfnoc_core_status   (rfnoc_core_status),
        .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
        .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
        .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
        .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
        .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
        .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
        .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
        .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
        .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready)
      );

    end else begin : gen_multi_spc

      rfnoc_block_ddc_ms #(
        .THIS_PORTID   (THIS_PORTID),
        .CHDR_W        (CHDR_W),
        .NUM_PORTS     (NUM_PORTS),
        .MTU           (MTU),
        .NUM_HB        (NUM_HB),
        .CIC_MAX_DECIM (CIC_MAX_DECIM),
        .NIPC          (NIPC)
      ) rfnoc_block_ddc_ms_i (
        .rfnoc_chdr_clk      (rfnoc_chdr_clk),
        .ce_clk              (ce_clk),
        .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
        .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
        .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
        .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
        .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
        .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
        .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
        .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
        .rfnoc_core_config   (rfnoc_core_config),
        .rfnoc_core_status   (rfnoc_core_status),
        .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
        .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
        .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
        .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
        .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
        .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
        .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
        .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
        .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready)
      );

    end
  endgenerate

endmodule
