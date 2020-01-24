//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//
//   Parameterized FIR filter RFNoC block with optional re-loadable
//   coefficients.
//
//   It has several optimizations for resource utilization such as using half
//   the number of DSP slices for symmetric coefficients, skipping coefficients
//   that are always set to zero, and using internal DSP slice registers to
//   hold coefficients.
//
//   For the most efficient DSP slice inference use these settings, set 
//   COEFF_WIDTH to be less than 18.
//
// Parameters:
// 
//   COEFF_WIDTH              : Coefficient width
//
//   NUM_COEFFS               : Number of coefficients / filter taps
//
//   COEFFS_VEC               : Vector of NUM_COEFFS values, each of width
//                              COEFF_WIDTH, to initialize the filter
//                              coefficients. Defaults to an impulse.
//
//   RELOADABLE_COEFFS        : Enable (1) or disable (0) reloading
//                              coefficients at runtime
//
//   SYMMETRIC_COEFFS         : Reduce multiplier usage by approximately half
//                              if coefficients are symmetric
//
//   SKIP_ZERO_COEFFS         : Reduce multiplier usage by assuming zero valued
//                              coefficients in DEFAULT_COEFFS are always zero.
//                              Useful for halfband filters.
//
//   USE_EMBEDDED_REGS_COEFFS : Reduce register usage by only using embedded
//                              registers in DSP slices. Updating taps while
//                              streaming will cause temporary output
//                              corruption!
//
//   Note: If using USE_EMBEDDED_REGS_COEFFS, coefficients must be written at 
//   least once since COEFFS_VEC is ignored!
//


module rfnoc_block_fir_filter #(
  // RFNoC Parameters
  parameter THIS_PORTID              = 0,
  parameter CHDR_W                   = 64,
  parameter NUM_PORTS                = 2,
  parameter MTU                      = 10,
  // FIR Filter Parameters
  parameter COEFF_WIDTH              = 16,
  parameter NUM_COEFFS               = 41,
  parameter [NUM_COEFFS*COEFF_WIDTH-1:0] COEFFS_VEC = // Make impulse by default
    { 
      {1'b0, {(COEFF_WIDTH-1){1'b1}} },        // Max positive value
      {(COEFF_WIDTH*(NUM_COEFFS-1)){1'b0}}     // Zero for remaining coefficients
    },
  parameter RELOADABLE_COEFFS        = 1,
  parameter SYMMETRIC_COEFFS         = 0,
  parameter SKIP_ZERO_COEFFS         = 0,
  parameter USE_EMBEDDED_REGS_COEFFS = 1
)(
  // Clock to use for signal processing
  input wire ce_clk,


  //---------------------------------------------------------------------------
  // AXIS CHDR Port
  //---------------------------------------------------------------------------

  input wire rfnoc_chdr_clk,

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

  `include "rfnoc_fir_filter_regs.vh"

  // These are the only supported values for now
  localparam ITEM_W = 32;
  localparam NIPC   = 1;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire        ctrlport_reg_req_wr;
  wire        ctrlport_reg_req_rd;
  wire [19:0] ctrlport_reg_req_addr;
  wire [31:0] ctrlport_reg_req_data;
  wire        ctrlport_reg_resp_ack;
  wire [ 1:0] ctrlport_reg_resp_status;
  wire [31:0] ctrlport_reg_resp_data;

  wire [(NUM_PORTS*ITEM_W*NIPC)-1:0] axis_to_fir_tdata;
  wire [              NUM_PORTS-1:0] axis_to_fir_tlast;
  wire [              NUM_PORTS-1:0] axis_to_fir_tvalid;
  wire [              NUM_PORTS-1:0] axis_to_fir_tready;

  wire [(NUM_PORTS*ITEM_W*NIPC)-1:0] axis_from_fir_tdata;
  wire [              NUM_PORTS-1:0] axis_from_fir_tlast;
  wire [              NUM_PORTS-1:0] axis_from_fir_tvalid;
  wire [              NUM_PORTS-1:0] axis_from_fir_tready;

  wire [(NUM_PORTS*CHDR_W)-1:0] m_axis_context_tdata;
  wire [     (4*NUM_PORTS)-1:0] m_axis_context_tuser;
  wire [         NUM_PORTS-1:0] m_axis_context_tlast;
  wire [         NUM_PORTS-1:0] m_axis_context_tvalid;
  wire [         NUM_PORTS-1:0] m_axis_context_tready;

  wire [(NUM_PORTS*CHDR_W)-1:0] s_axis_context_tdata;
  wire [     (4*NUM_PORTS)-1:0] s_axis_context_tuser;
  wire [         NUM_PORTS-1:0] s_axis_context_tlast;
  wire [         NUM_PORTS-1:0] s_axis_context_tvalid;
  wire [         NUM_PORTS-1:0] s_axis_context_tready;

  wire rfnoc_chdr_rst;
  wire ce_rst;

  localparam NOC_ID = 32'hF112_0000;


  // Cross the CHDR reset to the ddc_clk domain
  synchronizer ce_rst_sync_i (
    .clk (ce_clk),
    .rst (1'b0),
    .in  (rfnoc_chdr_rst),
    .out (ce_rst)
  );
  

  noc_shell_fir_filter #(
    .NOC_ID          (NOC_ID),
    .THIS_PORTID     (THIS_PORTID),
    .CHDR_W          (CHDR_W),
    .CTRLPORT_SLV_EN (0),
    .CTRLPORT_MST_EN (1),
    .NUM_DATA_I      (NUM_PORTS),
    .NUM_DATA_O      (NUM_PORTS),
    .ITEM_W          (ITEM_W),
    .NIPC            (NIPC),
    .PYLD_FIFO_SIZE  (5),
    .CTXT_FIFO_SIZE  (5),
    .MTU             (MTU)
  ) noc_shell_fir_filter_i (
    .rfnoc_chdr_clk            (rfnoc_chdr_clk),
    .rfnoc_chdr_rst            (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk            (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst            (),
    .rfnoc_core_config         (rfnoc_core_config),
    .rfnoc_core_status         (rfnoc_core_status),
    .s_rfnoc_chdr_tdata        (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast        (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid       (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready       (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata        (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast        (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid       (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready       (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata        (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast        (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid       (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready       (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata        (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast        (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid       (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready       (m_rfnoc_ctrl_tready),
    .ctrlport_clk              (ce_clk),
    .ctrlport_rst              (ce_rst),
    .m_ctrlport_req_wr         (ctrlport_reg_req_wr),
    .m_ctrlport_req_rd         (ctrlport_reg_req_rd),
    .m_ctrlport_req_addr       (ctrlport_reg_req_addr),
    .m_ctrlport_req_data       (ctrlport_reg_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (ctrlport_reg_resp_ack),
    .m_ctrlport_resp_status    (ctrlport_reg_resp_status),
    .m_ctrlport_resp_data      (ctrlport_reg_resp_data),
    .s_ctrlport_req_wr         (1'b0),
    .s_ctrlport_req_rd         (1'b0),
    .s_ctrlport_req_addr       (20'b0),
    .s_ctrlport_req_portid     (10'b0),
    .s_ctrlport_req_rem_epid   (16'b0),
    .s_ctrlport_req_rem_portid (10'b0),
    .s_ctrlport_req_data       (32'b0),
    .s_ctrlport_req_byte_en    (4'hF),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       (64'b0),
    .s_ctrlport_resp_ack       (),
    .s_ctrlport_resp_status    (),
    .s_ctrlport_resp_data      (),
    .axis_data_clk             (ce_clk),
    .axis_data_rst             (ce_rst),
    .m_axis_payload_tdata      (axis_to_fir_tdata),
    .m_axis_payload_tkeep      (),
    .m_axis_payload_tlast      (axis_to_fir_tlast),
    .m_axis_payload_tvalid     (axis_to_fir_tvalid),
    .m_axis_payload_tready     (axis_to_fir_tready),
    .s_axis_payload_tdata      (axis_from_fir_tdata),
    .s_axis_payload_tkeep      ({NUM_PORTS*NIPC{1'b1}}),
    .s_axis_payload_tlast      (axis_from_fir_tlast),
    .s_axis_payload_tvalid     (axis_from_fir_tvalid),
    .s_axis_payload_tready     (axis_from_fir_tready),
    .m_axis_context_tdata      (m_axis_context_tdata),
    .m_axis_context_tuser      (m_axis_context_tuser),
    .m_axis_context_tlast      (m_axis_context_tlast),
    .m_axis_context_tvalid     (m_axis_context_tvalid),
    .m_axis_context_tready     (m_axis_context_tready),
    .s_axis_context_tdata      (s_axis_context_tdata),
    .s_axis_context_tuser      (s_axis_context_tuser),
    .s_axis_context_tlast      (s_axis_context_tlast),
    .s_axis_context_tvalid     (s_axis_context_tvalid),
    .s_axis_context_tready     (s_axis_context_tready)
  );


  // The input packets are the same configuration as the output packets, so
  // just use the header information for each incoming to create the header for
  // each outgoing packet. This is done by connecting m_axis_context to
  // directly to s_axis_context.
  assign s_axis_context_tdata  = m_axis_context_tdata;
  assign s_axis_context_tuser  = m_axis_context_tuser;
  assign s_axis_context_tlast  = m_axis_context_tlast;
  assign s_axis_context_tvalid = m_axis_context_tvalid;
  assign m_axis_context_tready = s_axis_context_tready;


  //---------------------------------------------------------------------------
  // Control Port Address Decoding
  //---------------------------------------------------------------------------

  wire [   NUM_PORTS-1:0] m_ctrlport_req_wr;
  wire [   NUM_PORTS-1:0] m_ctrlport_req_rd;
  wire [20*NUM_PORTS-1:0] m_ctrlport_req_addr;
  wire [32*NUM_PORTS-1:0] m_ctrlport_req_data;
  wire [   NUM_PORTS-1:0] m_ctrlport_resp_ack;
  wire [32*NUM_PORTS-1:0] m_ctrlport_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES   (NUM_PORTS),
    .BASE_ADDR    (0),
    .SLAVE_ADDR_W (FIR_FILTER_ADDR_W)
  ) ctrlport_deocder_i (
    .ctrlport_clk            (ce_clk),
    .ctrlport_rst            (ce_rst),
    .s_ctrlport_req_wr       (ctrlport_reg_req_wr),
    .s_ctrlport_req_rd       (ctrlport_reg_req_rd),
    .s_ctrlport_req_addr     (ctrlport_reg_req_addr),
    .s_ctrlport_req_data     (ctrlport_reg_req_data),
    .s_ctrlport_req_byte_en  (4'b0),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'b0),
    .s_ctrlport_resp_ack     (ctrlport_reg_resp_ack),
    .s_ctrlport_resp_status  (ctrlport_reg_resp_status),
    .s_ctrlport_resp_data    (ctrlport_reg_resp_data),
    .m_ctrlport_req_wr       (m_ctrlport_req_wr),
    .m_ctrlport_req_rd       (m_ctrlport_req_rd),
    .m_ctrlport_req_addr     (m_ctrlport_req_addr),
    .m_ctrlport_req_data     (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status  ({NUM_PORTS{2'b0}}),
    .m_ctrlport_resp_data    (m_ctrlport_resp_data)
  );


  //---------------------------------------------------------------------------
  // FIR Core Instances
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_PORTS; i = i+1) begin : gen_rfnoc_fir_filter_cores
    rfnoc_fir_filter_core #(
      .DATA_W                   (ITEM_W*NIPC),
      .COEFF_WIDTH              (COEFF_WIDTH),
      .NUM_COEFFS               (NUM_COEFFS),
      .COEFFS_VEC               (COEFFS_VEC),
      .RELOADABLE_COEFFS        (RELOADABLE_COEFFS),
      .SYMMETRIC_COEFFS         (SYMMETRIC_COEFFS),
      .SKIP_ZERO_COEFFS         (SKIP_ZERO_COEFFS),
      .USE_EMBEDDED_REGS_COEFFS (USE_EMBEDDED_REGS_COEFFS)
    ) rfnoc_fir_filter_core_i (
      .clk                  (ce_clk),
      .rst                  (ce_rst),
      .s_ctrlport_req_wr    (m_ctrlport_req_wr[i]),
      .s_ctrlport_req_rd    (m_ctrlport_req_rd[i]),
      .s_ctrlport_req_addr  (m_ctrlport_req_addr[20*i +: 20]),
      .s_ctrlport_req_data  (m_ctrlport_req_data[32*i +: 32]),
      .s_ctrlport_resp_ack  (m_ctrlport_resp_ack[i]),
      .s_ctrlport_resp_data (m_ctrlport_resp_data[32*i +: 32]),
      .s_axis_tdata         (axis_to_fir_tdata[i*(ITEM_W*NIPC) +: (ITEM_W*NIPC)]),
      .s_axis_tlast         (axis_to_fir_tlast[i]),
      .s_axis_tvalid        (axis_to_fir_tvalid[i]),
      .s_axis_tready        (axis_to_fir_tready[i]),
      .m_axis_tdata         (axis_from_fir_tdata[i*(ITEM_W*NIPC) +: (ITEM_W*NIPC)]),
      .m_axis_tlast         (axis_from_fir_tlast[i]),
      .m_axis_tvalid        (axis_from_fir_tvalid[i]),
      .m_axis_tready        (axis_from_fir_tready[i])
    );
  end

endmodule
