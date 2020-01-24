//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc
//
// Description:  An digital down-converter block for RFNoC.
//
// Parameters:
//
//   THIS_PORTID    : Control crossbar port to which this block is connected
//   CHDR_W         : AXIS CHDR interface data width
//   NUM_PORTS      : Number of DDCs to instantiate
//   MTU            : Maximum transmission unit (i.e., maximum packet size) in
//                    CHDR words is 2**MTU.
//   CTRL_FIFO_SIZE : Size of the Control Port slave FIFO. This affects the
//                    number of outstanding commands that can be pending.
//   NUM_HB         : Number of half-band decimation blocks to include (0-3)
//   CIC_MAX_DECIM  : Maximum decimation to support in the CIC filter
//

module rfnoc_block_ddc #(
  parameter THIS_PORTID    = 0,
  parameter CHDR_W         = 64,
  parameter NUM_PORTS      = 2,
  parameter MTU            = 10,
  parameter CTRL_FIFO_SIZE = 6,
  parameter NUM_HB         = 3,
  parameter CIC_MAX_DECIM  = 255
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

  localparam NOC_ID = 'hDDC0_0000;

  localparam COMPAT_MAJOR  = 16'h0;
  localparam COMPAT_MINOR  = 16'h0;

  `include "rfnoc_block_ddc_regs.vh"
  `include "../../core/rfnoc_axis_ctrl_utils.vh"


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  wire rfnoc_chdr_rst;

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
  wire [    16*NUM_PORTS-1:0] m_axis_data_tlength;
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

  wire ddc_rst;

  // Cross the CHDR reset to the ce_clk domain
  synchronizer ddc_rst_sync_i (
    .clk (ce_clk),
    .rst (1'b0),
    .in  (rfnoc_chdr_rst),
    .out (ddc_rst)
  );


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_ddc #(
    .NOC_ID          (NOC_ID),
    .THIS_PORTID     (THIS_PORTID),
    .CHDR_W          (CHDR_W),
    .CTRLPORT_SLV_EN (0),
    .CTRLPORT_MST_EN (1),
    .CTRL_FIFO_SIZE  (CTRL_FIFO_SIZE),
    .NUM_DATA_I      (NUM_PORTS),
    .NUM_DATA_O      (NUM_PORTS),
    .ITEM_W          (ITEM_W),
    .NIPC            (NIPC),
    .PYLD_FIFO_SIZE  (MTU),
    .MTU             (MTU)
  ) noc_shell_ddc_i (
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
    .ctrlport_rst              (ddc_rst),
    .m_ctrlport_req_wr         (ctrlport_req_wr),
    .m_ctrlport_req_rd         (ctrlport_req_rd),
    .m_ctrlport_req_addr       (ctrlport_req_addr),
    .m_ctrlport_req_data       (ctrlport_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (ctrlport_req_has_time),
    .m_ctrlport_req_time       (ctrlport_req_time),
    .m_ctrlport_resp_ack       (ctrlport_resp_ack),
    .m_ctrlport_resp_status    (AXIS_CTRL_STS_OKAY),
    .m_ctrlport_resp_data      (ctrlport_resp_data),
    .s_ctrlport_req_wr         (1'b0),
    .s_ctrlport_req_rd         (1'b0),
    .s_ctrlport_req_addr       (20'b0),
    .s_ctrlport_req_portid     (10'b0),
    .s_ctrlport_req_rem_epid   (16'b0),
    .s_ctrlport_req_rem_portid (10'b0),
    .s_ctrlport_req_data       (32'b0),
    .s_ctrlport_req_byte_en    (4'b0),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       (64'b0),
    .s_ctrlport_resp_ack       (),
    .s_ctrlport_resp_status    (),
    .s_ctrlport_resp_data      (),
    .axis_data_clk             (ce_clk),
    .axis_data_rst             (ddc_rst),
    .m_axis_tdata              (m_axis_data_tdata),
    .m_axis_tkeep              (),
    .m_axis_tlast              (m_axis_data_tlast),
    .m_axis_tvalid             (m_axis_data_tvalid),
    .m_axis_tready             (m_axis_data_tready),
    .m_axis_ttimestamp         (m_axis_data_ttimestamp),
    .m_axis_thas_time          (m_axis_data_thas_time),
    .m_axis_tlength            (m_axis_data_tlength),
    .m_axis_teov               (),
    .m_axis_teob               (m_axis_data_teob),
    .s_axis_tdata              (s_axis_data_tdata),
    .s_axis_tkeep              ({NUM_PORTS*NIPC{1'b1}}),
    .s_axis_tlast              (s_axis_data_tlast),
    .s_axis_tvalid             (s_axis_data_tvalid),
    .s_axis_tready             (s_axis_data_tready),
    .s_axis_ttimestamp         (s_axis_data_ttimestamp),
    .s_axis_thas_time          (s_axis_data_thas_time),
    .s_axis_teov               ({NUM_PORTS{1'b0}}),
    .s_axis_teob               (s_axis_data_teob)
  );


  //---------------------------------------------------------------------------
  // Register Translation
  //---------------------------------------------------------------------------
  //
  // Each DDC block is allocated an address spaces. This block translates CTRL
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
  wire [   NUM_PORTS-1:0] rb_stb;

  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_PORTS)
  ) ctrlport_to_settings_bus_i (
    .ctrlport_clk             (ce_clk),
    .ctrlport_rst             (ddc_rst),
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
    .rb_stb                   (rb_stb),
    .rb_addr                  (rb_addr),
    .rb_data                  (rb_data));


  //---------------------------------------------------------------------------
  // DDC Implementation
  //---------------------------------------------------------------------------

  // Unused signals
  wire [   NUM_PORTS-1:0] clear_tx_seqnum = 0;
  wire [16*NUM_PORTS-1:0] src_sid         = 0;
  wire [16*NUM_PORTS-1:0] next_dst_sid    = 0;

  localparam MAX_N = CIC_MAX_DECIM * 2 << (NUM_HB-1);

  genvar i;
  generate
    for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_ddc_chains
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

      // TODO: Read-back register for number of FIR filter taps
      always @(*) begin
        case(rb_addr[8*i+7:8*i])
          RB_COMPAT_NUM    : rb_data[64*i+63:64*i] <= {COMPAT_MAJOR, COMPAT_MINOR};
          RB_NUM_HB        : rb_data[64*i+63:64*i] <= NUM_HB;
          RB_CIC_MAX_DECIM : rb_data[64*i+63:64*i] <= CIC_MAX_DECIM;
          default          : rb_data[64*i+63:64*i] <= 64'h0BADC0DE0BADC0DE;
        endcase
      end

      ////////////////////////////////////////////////////////////
      //
      // Timed Commands
      //
      ////////////////////////////////////////////////////////////
      wire [31:0]  m_axis_tagged_tdata;
      wire         m_axis_tagged_tlast;
      wire         m_axis_tagged_tvalid;
      wire         m_axis_tagged_tready;
      wire [127:0] m_axis_tagged_tuser;
      wire         m_axis_tagged_tag;

      wire         out_set_stb;
      wire [7:0]   out_set_addr;
      wire [31:0]  out_set_data;
      wire         timed_set_stb;
      wire [7:0]   timed_set_addr;
      wire [31:0]  timed_set_data;

      wire         timed_cmd_fifo_full;

      axi_tag_time #(
        .NUM_TAGS(1),
        .SR_TAG_ADDRS(SR_FREQ_ADDR))
      axi_tag_time (
        .clk(ce_clk),
        .reset(ddc_rst),
        .clear(clear_tx_seqnum[i]),
        .tick_rate(16'd1),
        .timed_cmd_fifo_full(timed_cmd_fifo_full),
        .s_axis_data_tdata(m_axis_data_tdata[i*ITEM_W+:ITEM_W]), .s_axis_data_tlast(m_axis_data_tlast[i]),
        .s_axis_data_tvalid(m_axis_data_tvalid[i]), .s_axis_data_tready(m_axis_data_tready[i]),
        .s_axis_data_tuser(m_axis_data_tuser[128*i+:128]),
        .m_axis_data_tdata(m_axis_tagged_tdata), .m_axis_data_tlast(m_axis_tagged_tlast),
        .m_axis_data_tvalid(m_axis_tagged_tvalid), .m_axis_data_tready(m_axis_tagged_tready),
        .m_axis_data_tuser(m_axis_tagged_tuser), .m_axis_data_tag(m_axis_tagged_tag),
        .in_set_stb(set_stb_int), .in_set_addr(set_addr_int), .in_set_data(set_data_int),
        .in_set_time(set_time_int), .in_set_has_time(set_has_time_int),
        .out_set_stb(out_set_stb), .out_set_addr(out_set_addr), .out_set_data(out_set_data),
        .timed_set_stb(timed_set_stb), .timed_set_addr(timed_set_addr), .timed_set_data(timed_set_data));

      // Hold off reading additional commands if internal FIFO is full
      assign rb_stb[i] = ~timed_cmd_fifo_full;

      ////////////////////////////////////////////////////////////
      //
      // Reduce Rate
      //
      ////////////////////////////////////////////////////////////
      wire [31:0] sample_in_tdata, sample_out_tdata;
      wire sample_in_tuser, sample_in_eob;
      wire sample_in_tvalid, sample_in_tready, sample_in_tlast;
      wire sample_out_tvalid, sample_out_tready;
      wire clear_user;
      wire nc;
      axi_rate_change #(
        .WIDTH(33),
        .MAX_N(MAX_N),
        .MAX_M(1),
        .SR_N_ADDR(SR_N_ADDR),
        .SR_M_ADDR(SR_M_ADDR),
        .SR_CONFIG_ADDR(SR_CONFIG_ADDR))
      axi_rate_change (
        .clk(ce_clk), .reset(ddc_rst), .clear(clear_tx_seqnum[i]), .clear_user(clear_user),
        .src_sid(src_sid[16*i+15:16*i]), .dst_sid(next_dst_sid[16*i+15:16*i]),
        .set_stb(out_set_stb), .set_addr(out_set_addr), .set_data(out_set_data),
        .i_tdata({m_axis_tagged_tag,m_axis_tagged_tdata}), .i_tlast(m_axis_tagged_tlast),
        .i_tvalid(m_axis_tagged_tvalid), .i_tready(m_axis_tagged_tready),
        .i_tuser(m_axis_tagged_tuser),
        .o_tdata({nc,s_axis_data_tdata[i*ITEM_W+:ITEM_W]}), .o_tlast(s_axis_data_tlast[i]), .o_tvalid(s_axis_data_tvalid[i]),
        .o_tready(s_axis_data_tready[i]), .o_tuser(s_axis_data_tuser[128*i+:128]),
        .m_axis_data_tdata({sample_in_tuser,sample_in_tdata}), .m_axis_data_tlast(sample_in_tlast),
        .m_axis_data_tvalid(sample_in_tvalid), .m_axis_data_tready(sample_in_tready),
        .s_axis_data_tdata({1'b0,sample_out_tdata}), .s_axis_data_tlast(1'b0),
        .s_axis_data_tvalid(sample_out_tvalid), .s_axis_data_tready(sample_out_tready),
        .warning_long_throttle(),
        .error_extra_outputs(),
        .error_drop_pkt_lockup());

      assign sample_in_eob = m_axis_tagged_tuser[124]; //this should align with last packet output from axi_rate_change

      ////////////////////////////////////////////////////////////
      //
      // Digital Down Converter
      //
      ////////////////////////////////////////////////////////////

      ddc #(
        .SR_FREQ_ADDR(SR_FREQ_ADDR),
        .SR_SCALE_IQ_ADDR(SR_SCALE_IQ_ADDR),
        .SR_DECIM_ADDR(SR_DECIM_ADDR),
        .SR_MUX_ADDR(SR_MUX_ADDR),
        .SR_COEFFS_ADDR(SR_COEFFS_ADDR),
        .NUM_HB(NUM_HB),
        .CIC_MAX_DECIM(CIC_MAX_DECIM))
      ddc (
        .clk(ce_clk), .reset(ddc_rst),
        .clear(clear_user | clear_tx_seqnum[i]), // Use AXI Rate Change's clear user to reset block to initial state after EOB
        .set_stb(out_set_stb), .set_addr(out_set_addr), .set_data(out_set_data),
        .timed_set_stb(timed_set_stb), .timed_set_addr(timed_set_addr), .timed_set_data(timed_set_data),
        .sample_in_tdata(sample_in_tdata), .sample_in_tlast(sample_in_tlast),
        .sample_in_tvalid(sample_in_tvalid), .sample_in_tready(sample_in_tready),
        .sample_in_tuser(sample_in_tuser), .sample_in_eob(sample_in_eob),
        .sample_out_tdata(sample_out_tdata), .sample_out_tlast(),
        .sample_out_tvalid(sample_out_tvalid), .sample_out_tready(sample_out_tready)
        );

    end
  endgenerate

endmodule
