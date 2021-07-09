//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc
//
// Description:  An digital up-converter block for RFNoC.
//
// Parameters:
//
//   THIS_PORTID    : Control crossbar port to which this block is connected
//   CHDR_W         : AXIS CHDR interface data width
//   NUM_PORTS      : Number of DUC signal processing chains
//   MTU            : Maximum transmission unit (i.e., maximum packet size) in
//                    CHDR words is 2**MTU.
//   NUM_HB         : Number of half-band filter blocks to include (0-3)
//   CIC_MAX_INTERP : Maximum interpolation to support in the CIC filter
//

`default_nettype none



module rfnoc_block_duc #(
  parameter THIS_PORTID    = 0,
  parameter CHDR_W         = 64,
  parameter NUM_PORTS      = 2,
  parameter MTU            = 10,
  parameter NUM_HB         = 2,
  parameter CIC_MAX_INTERP = 128
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

  // These are the only supported values for now
  localparam ITEM_W = 32;
  localparam NIPC   = 1;

  localparam COMPAT_MAJOR  = 16'h0;
  localparam COMPAT_MINOR  = 16'h1;

  `include "rfnoc_block_duc_regs.vh"
  `include "../../core/rfnoc_axis_ctrl_utils.vh"


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  wire        ctrlport_req_wr;
  wire        ctrlport_req_rd;
  wire [19:0] ctrlport_req_addr;
  wire [31:0] ctrlport_req_data;
  wire        ctrlport_req_has_time;
  wire [63:0] ctrlport_req_time;
  wire        ctrlport_resp_ack;
  wire [31:0] ctrlport_resp_data;

  wire [NUM_PORTS*ITEM_W-1:0] m_axis_data_tdata;
  wire [       NUM_PORTS-1:0] m_axis_data_tlast;
  wire [       NUM_PORTS-1:0] m_axis_data_tvalid;
  wire [       NUM_PORTS-1:0] m_axis_data_tready;
  wire [    NUM_PORTS*64-1:0] m_axis_data_ttimestamp;
  wire [       NUM_PORTS-1:0] m_axis_data_thas_time;
  wire [    NUM_PORTS*16-1:0] m_axis_data_tlength;
  wire [       NUM_PORTS-1:0] m_axis_data_teob;
  wire [   NUM_PORTS*128-1:0] m_axis_data_tuser;

  wire [NUM_PORTS*ITEM_W-1:0] s_axis_data_tdata;
  wire [       NUM_PORTS-1:0] s_axis_data_tlast;
  wire [       NUM_PORTS-1:0] s_axis_data_tvalid;
  wire [       NUM_PORTS-1:0] s_axis_data_tready;
  wire [   NUM_PORTS*128-1:0] s_axis_data_tuser;
  wire [       NUM_PORTS-1:0] s_axis_data_teob;
  wire [    NUM_PORTS*64-1:0] s_axis_data_ttimestamp;
  wire [       NUM_PORTS-1:0] s_axis_data_thas_time;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire ce_rst;

  noc_shell_duc #(
    .THIS_PORTID (THIS_PORTID),
    .CHDR_W      (CHDR_W),
    .MTU         (MTU),
    .NUM_PORTS   (NUM_PORTS)
  ) noc_shell_duc_i (
    .rfnoc_chdr_clk          (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk          (rfnoc_ctrl_clk),
    .ce_clk                  (ce_clk),
    .rfnoc_chdr_rst          (),
    .rfnoc_ctrl_rst          (),
    .ce_rst                  (ce_rst),
    .rfnoc_core_config       (rfnoc_core_config),
    .rfnoc_core_status       (rfnoc_core_status),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata      (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast      (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid     (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready     (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata      (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast      (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid     (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready     (m_rfnoc_ctrl_tready),
    .ctrlport_clk            (),
    .ctrlport_rst            (),
    .m_ctrlport_req_wr       (ctrlport_req_wr),
    .m_ctrlport_req_rd       (ctrlport_req_rd),
    .m_ctrlport_req_addr     (ctrlport_req_addr),
    .m_ctrlport_req_data     (ctrlport_req_data),
    .m_ctrlport_req_has_time (ctrlport_req_has_time),
    .m_ctrlport_req_time     (ctrlport_req_time),
    .m_ctrlport_resp_ack     (ctrlport_resp_ack),
    .m_ctrlport_resp_data    (ctrlport_resp_data),
    .axis_data_clk           (),
    .axis_data_rst           (),
    .m_in_axis_tdata         (m_axis_data_tdata),
    .m_in_axis_tkeep         (),
    .m_in_axis_tlast         (m_axis_data_tlast),
    .m_in_axis_tvalid        (m_axis_data_tvalid),
    .m_in_axis_tready        (m_axis_data_tready),
    .m_in_axis_ttimestamp    (m_axis_data_ttimestamp),
    .m_in_axis_thas_time     (m_axis_data_thas_time),
    .m_in_axis_tlength       (m_axis_data_tlength),
    .m_in_axis_teov          (),
    .m_in_axis_teob          (m_axis_data_teob),
    .s_out_axis_tdata        (s_axis_data_tdata),
    .s_out_axis_tkeep        ({NUM_PORTS*NIPC{1'b1}}),
    .s_out_axis_tlast        (s_axis_data_tlast),
    .s_out_axis_tvalid       (s_axis_data_tvalid),
    .s_out_axis_tready       (s_axis_data_tready),
    .s_out_axis_ttimestamp   (s_axis_data_ttimestamp),
    .s_out_axis_thas_time    (s_axis_data_thas_time),
    .s_out_axis_teov         ({NUM_PORTS{1'b0}}),
    .s_out_axis_teob         (s_axis_data_teob)
  );


  //---------------------------------------------------------------------------
  // Register Translation
  //---------------------------------------------------------------------------
  //
  // Each DUC block is allocated an address spaces. This block translates CTRL
  // port transactions in that space to settings bus.
  //
  //---------------------------------------------------------------------------

  wire [ 8*NUM_PORTS-1:0] set_addr;
  wire [32*NUM_PORTS-1:0] set_data;
  wire [   NUM_PORTS-1:0] set_has_time;
  wire [   NUM_PORTS-1:0] set_stb;
  wire [64*NUM_PORTS-1:0] set_time;
  wire [ 8*NUM_PORTS-1:0] rb_addr;
  reg  [64*NUM_PORTS-1:0] rb_data;

  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_PORTS)
  ) ctrlport_to_settings_bus_i (
    .ctrlport_clk             (ce_clk),
    .ctrlport_rst             (ce_rst),
    .s_ctrlport_req_wr        (ctrlport_req_wr),
    .s_ctrlport_req_rd        (ctrlport_req_rd),
    .s_ctrlport_req_addr      (ctrlport_req_addr),
    .s_ctrlport_req_data      (ctrlport_req_data),
    .s_ctrlport_req_has_time  (ctrlport_req_has_time),
    .s_ctrlport_req_time      (ctrlport_req_time),
    .s_ctrlport_resp_ack      (ctrlport_resp_ack),
    .s_ctrlport_resp_data     (ctrlport_resp_data),
    .set_data                 (set_data),
    .set_addr                 (set_addr),
    .set_stb                  (set_stb),
    .set_time                 (set_time),
    .set_has_time             (set_has_time),
    .rb_stb                   ({NUM_PORTS{1'b1}}),
    .rb_addr                  (rb_addr),
    .rb_data                  (rb_data),
    .timestamp                (64'b0)
  );


  //---------------------------------------------------------------------------
  // DUC Implementation
  //---------------------------------------------------------------------------

  // Unused signals
  wire [   NUM_PORTS-1:0] clear_tx_seqnum = 0;
  wire [16*NUM_PORTS-1:0] src_sid         = 0;
  wire [16*NUM_PORTS-1:0] next_dst_sid    = 0;

  localparam MAX_M = CIC_MAX_INTERP * 2<<(NUM_HB-1);

  genvar i;
  generate
    for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_duc_chains
      wire clear_user;
      wire clear_duc = clear_tx_seqnum[i] | clear_user;

      wire        set_stb_int      = set_stb[i];
      wire [7:0]  set_addr_int     = set_addr[8*i+7:8*i];
      wire [31:0] set_data_int     = set_data[32*i+31:32*i];
      wire [63:0] set_time_int     = set_time[64*i+63:64*i];
      wire        set_has_time_int = set_has_time[i];

      // Build the expected tuser CHDR header
      cvita_hdr_encoder cvita_hdr_encoder_i (
        .pkt_type       (2'b0),
        .eob            (m_axis_data_teob[i]),
        .has_time       (m_axis_data_thas_time[i]),
        .seqnum         (12'b0),
        .payload_length (m_axis_data_tlength[16*i +: 16]),
        .src_sid        (16'b0),
        .dst_sid        (16'b0),
        .vita_time      (m_axis_data_ttimestamp[64*i +: 64]),
        .header         (m_axis_data_tuser[128*i+:128])
      );

      // Extract bit fields from outgoing tuser CHDR header
      assign s_axis_data_teob[i]              = s_axis_data_tuser[128*i+124 +:  1];
      assign s_axis_data_thas_time[i]         = s_axis_data_tuser[128*i+125 +:  1];
      assign s_axis_data_ttimestamp[64*i+:64] = s_axis_data_tuser[128*i+  0 +: 64];

      // TODO Readback register for number of FIR filter taps
      always @(*) begin
        case(rb_addr[i*8+7:i*8])
          RB_COMPAT_NUM     : rb_data[i*64+63:i*64] <= {COMPAT_MAJOR, COMPAT_MINOR};
          RB_NUM_HB         : rb_data[i*64+63:i*64] <= NUM_HB;
          RB_CIC_MAX_INTERP : rb_data[i*64+63:i*64] <= CIC_MAX_INTERP;
          default           : rb_data[i*64+63:i*64] <= 64'h0BADC0DE0BADC0DE;
        endcase
      end

      //-------------------------------
      // Timed DDS + Complex Multiplier
      //-------------------------------
      wire [31:0]  m_axis_rc_tdata;
      wire         m_axis_rc_tlast;
      wire         m_axis_rc_tvalid;
      wire         m_axis_rc_tready;
      wire [127:0] m_axis_rc_tuser;

      dds_timed #(
        .SR_FREQ_ADDR(SR_FREQ_ADDR),
        .SR_SCALE_IQ_ADDR(SR_SCALE_IQ_ADDR))
      dds_timed (
        .clk(ce_clk), .reset(ce_rst), .clear(clear_tx_seqnum[i]),
        .timed_cmd_fifo_full(),
        .set_stb(set_stb_int), .set_addr(set_addr_int), .set_data(set_data_int),
        .set_time(set_time_int), .set_has_time(set_has_time_int),
        .i_tdata(m_axis_rc_tdata), .i_tlast(m_axis_rc_tlast), .i_tvalid(m_axis_rc_tvalid),
        .i_tready(m_axis_rc_tready), .i_tuser(m_axis_rc_tuser),
        .o_tdata(s_axis_data_tdata[ITEM_W*i+:ITEM_W]), .o_tlast(s_axis_data_tlast[i]), .o_tvalid(s_axis_data_tvalid[i]),
        .o_tready(s_axis_data_tready[i]), .o_tuser(s_axis_data_tuser[128*i+:128]));

      //-------------------------------
      // Increase Rate
      //-------------------------------
      wire [31:0] sample_tdata, sample_duc_tdata;
      wire sample_tvalid, sample_tready;
      wire sample_duc_tvalid, sample_duc_tready;
      axi_rate_change #(
        .WIDTH(32),
        .MAX_N(1),
        .MAX_M(MAX_M),
        .SR_N_ADDR(SR_N_ADDR),
        .SR_M_ADDR(SR_M_ADDR),
        .SR_CONFIG_ADDR(SR_CONFIG_ADDR),
        .SR_TIME_INCR_ADDR(SR_TIME_INCR_ADDR))
      axi_rate_change (
        .clk(ce_clk), .reset(ce_rst), .clear(clear_tx_seqnum[i]), .clear_user(clear_user),
        .src_sid(src_sid[16*i+15:16*i]), .dst_sid(next_dst_sid[16*i+15:16*i]),
        .set_stb(set_stb_int), .set_addr(set_addr_int), .set_data(set_data_int),
        .i_tdata(m_axis_data_tdata[ITEM_W*i+:ITEM_W]), .i_tlast(m_axis_data_tlast[i]), .i_tvalid(m_axis_data_tvalid[i]),
        .i_tready(m_axis_data_tready[i]), .i_tuser(m_axis_data_tuser[128*i+:128]),
        .o_tdata(m_axis_rc_tdata), .o_tlast(m_axis_rc_tlast), .o_tvalid(m_axis_rc_tvalid),
        .o_tready(m_axis_rc_tready), .o_tuser(m_axis_rc_tuser),
        .m_axis_data_tdata({sample_tdata}), .m_axis_data_tlast(), .m_axis_data_tvalid(sample_tvalid),
        .m_axis_data_tready(sample_tready),
        .s_axis_data_tdata(sample_duc_tdata), .s_axis_data_tlast(1'b0), .s_axis_data_tvalid(sample_duc_tvalid),
        .s_axis_data_tready(sample_duc_tready),
        .warning_long_throttle(), .error_extra_outputs(), .error_drop_pkt_lockup());

      //-------------------------------
      // Digital Up Converter
      //-------------------------------
      duc #(
        .SR_INTERP_ADDR(SR_INTERP_ADDR),
        .NUM_HB(NUM_HB),
        .CIC_MAX_INTERP(CIC_MAX_INTERP))
      duc (
        .clk(ce_clk), .reset(ce_rst), .clear(clear_duc),
        .set_stb(set_stb_int), .set_addr(set_addr_int), .set_data(set_data_int),
        .i_tdata(sample_tdata), .i_tuser(128'b0), .i_tvalid(sample_tvalid), .i_tready(sample_tready),
        .o_tdata(sample_duc_tdata), .o_tuser(), .o_tvalid(sample_duc_tvalid), .o_tready(sample_duc_tready));

    end
  endgenerate

endmodule


`default_nettype wire
