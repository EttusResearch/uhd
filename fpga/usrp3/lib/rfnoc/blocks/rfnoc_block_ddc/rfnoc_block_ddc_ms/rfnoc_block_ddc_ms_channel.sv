//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc_ms_channel
//
// Description:
//   Per-channel wrapper for rfnoc_block_ddc_ms.
//   This module instantiates the
//     - per-port ctrlport decode split
//     - axi_rate_change_ms, and
//     - ddc_ms (actual signal processing) chain.
//
// Parameters:
//   NUM_HB        : Number of halfband filter stages to implement in each DDC chain.
//                   This determines the maximum decimation rate of the halfband filters,
//                   which is 2^NUM_HB.
//   CIC_MAX_DECIM : Maximum decimation rate of the CIC filter in each DDC chain.
//                   The total maximum decimation rate of the DDC is
//                   CIC_MAX_DECIM * 2^NUM_HB.
//   SAMP_W        : Width of an I+Q sample.
//   SPC           : Number of samples processed per clock cycle.
//                   This determines the width of the input and output data streams,
//                   which are SAMP_W*SPC bits wide.
//                   The DDC will process SPC samples in parallel every clock cycle.
//   SPC_MTU_LOG2  : Log2 of the maximum number of words (SAMP_W*SPC-wide) per packet.
//

`default_nettype none

module rfnoc_block_ddc_ms_channel
  import ctrlport_pkg::*;
  import rfnoc_chdr_utils_pkg::*;
#(
  parameter NUM_HB        = 3,
  parameter CIC_MAX_DECIM = 255,
  parameter SAMP_W        = 32,
  parameter SPC           = 1,
  parameter SPC_MTU_LOG2  = 10
) (
  input  wire logic ce_clk,
  input  wire logic ce_rst,

  // CTRL port for this channel
  input  wire logic                          ctrlport_req_wr,
  input  wire logic                          ctrlport_req_rd,
  input  wire logic [   CTRLPORT_ADDR_W-1:0] ctrlport_req_addr,
  input  wire logic [   CTRLPORT_DATA_W-1:0] ctrlport_req_data,
  input  wire logic [CTRLPORT_BYTE_EN_W-1:0] ctrlport_req_byte_en,
  input  wire logic                          ctrlport_req_has_time,
  input  wire logic [   CTRLPORT_TIME_W-1:0] ctrlport_req_time,
  output      logic                          ctrlport_resp_ack,
  output      logic [    CTRLPORT_STS_W-1:0] ctrlport_resp_status,
  output      logic [   CTRLPORT_DATA_W-1:0] ctrlport_resp_data,

  // Input data from NoC shell
  input  wire logic [SPC-1:0][SAMP_W-1:0]  m_axis_data_tdata,
  input  wire logic                        m_axis_data_tlast,
  input  wire logic                        m_axis_data_tvalid,
  output      logic                        m_axis_data_tready,
  input  wire logic [CHDR_TIMESTAMP_W-1:0] m_axis_data_ttimestamp,
  input  wire logic                        m_axis_data_thas_time,
  input  wire logic [CHDR_LENGTH_W-1:0]    m_axis_data_tlength,
  input  wire logic                        m_axis_data_teob,

  // Output data to NoC shell
  output      logic [SPC-1:0][SAMP_W-1:0]  s_axis_data_tdata,
  output      logic                        s_axis_data_tlast,
  output      logic                        s_axis_data_tvalid,
  input  wire logic                        s_axis_data_tready,
  output      logic                        s_axis_data_teob,
  output      logic [CHDR_TIMESTAMP_W-1:0] s_axis_data_ttimestamp,
  output      logic                        s_axis_data_thas_time
);

  import rfnoc_block_ddc_ms_regs_pkg::*;

  // Local parameters
  localparam int PHASE_W     = 24;
  localparam int TICK_RATE_W = 16;

  // Ctrlport decoder signals
  logic                          ctrlport_axi_rate_req_wr;
  logic                          ctrlport_axi_rate_req_rd;
  logic [   CTRLPORT_ADDR_W-1:0] ctrlport_axi_rate_req_addr;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_axi_rate_req_data;
  logic [CTRLPORT_BYTE_EN_W-1:0] ctrlport_axi_rate_req_byte_en;
  logic                          ctrlport_axi_rate_req_has_time;
  logic [   CTRLPORT_TIME_W-1:0] ctrlport_axi_rate_req_time;
  logic                          ctrlport_axi_rate_resp_ack;
  logic [    CTRLPORT_STS_W-1:0] ctrlport_axi_rate_resp_status;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_axi_rate_resp_data;

  logic                          ctrlport_dds_req_wr;
  logic                          ctrlport_dds_req_rd;
  logic [   CTRLPORT_ADDR_W-1:0] ctrlport_dds_req_addr;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_dds_req_data;
  logic [CTRLPORT_BYTE_EN_W-1:0] ctrlport_dds_req_byte_en;
  logic                          ctrlport_dds_req_has_time;
  logic [   CTRLPORT_TIME_W-1:0] ctrlport_dds_req_time;
  logic                          ctrlport_dds_resp_ack;
  logic [    CTRLPORT_STS_W-1:0] ctrlport_dds_resp_status;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_dds_resp_data;

  logic                          ctrlport_sr_req_wr;
  logic                          ctrlport_sr_req_rd;
  logic [   CTRLPORT_ADDR_W-1:0] ctrlport_sr_req_addr;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_sr_req_data;
  logic [CTRLPORT_BYTE_EN_W-1:0] ctrlport_sr_req_byte_en;
  logic                          ctrlport_sr_req_has_time;
  logic [   CTRLPORT_TIME_W-1:0] ctrlport_sr_req_time;
  logic                          ctrlport_sr_resp_ack;
  logic [    CTRLPORT_STS_W-1:0] ctrlport_sr_resp_status;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_sr_resp_data;

  // Internal data path signals
  logic [SPC-1:0][SAMP_W-1:0]  arc_in_tdata;
  logic                        arc_in_tlast;
  logic                        arc_in_tvalid;
  logic                        arc_in_tready;
  logic [SPC-1:0]              arc_in_ttags;
  logic [CHDR_TIMESTAMP_W-1:0] arc_in_ttimestamp;
  logic                        arc_in_thas_time;
  logic                        arc_in_teob;
  logic [CHDR_LENGTH_W-1:0]    arc_in_tlength;

  logic [PHASE_W-1:0]         cmd_out_tdata;
  logic                       cmd_out_tvalid;
  logic                       cmd_out_tready;
  logic                       cmd_out_tuser;
  logic                       cmd_fifo_full;

  logic [SPC-1:0][SAMP_W-1:0] ddc_in_tdata;
  logic                       ddc_in_tlast;
  logic                       ddc_in_tvalid;
  logic                       ddc_in_tready;
  logic                       ddc_in_teob;
  logic [SPC-1:0]             ddc_in_ttags;

  logic [SPC-1:0][SAMP_W-1:0] ddc_out_tdata;
  logic                       ddc_out_tlast;
  logic                       ddc_out_tvalid;
  logic                       ddc_out_tready;

  // Strobed by axi_rate_change_ms at EOB (enable_clear_user=1).
  // Used to reset axi_decim phase/state between bursts.
  logic clear_user_i;

  //---------------------------------------------------------------------------
  // Split DDC port-specific register address range into
  //   - AXI rate change registers (to be passed to AXI rate change)
  //   - SR registers (to be passed to the DDC DSP chain)
  //   - DDS freq shift registers (to be passed to axi_tag_time_ms)
  // Note: the port base is 0 since the upstream decoder already took care
  //       of splitting the address range.
  //---------------------------------------------------------------------------

  ctrlport_decoder_param #(
    .NUM_SLAVES  (3),
    // Per-port local address space after ctrlport_decoder_ports:
    //   0x000..0x0FF : AXI rate
    //   0x100..0x1FF : remaining DSP path (forwarded to ddc_ms)
    //   0x200..0x2FF : DDS freq shift (forwarded to axi_tag_time_ms)
    .PORT_BASE  ({CTRLPORT_ADDR_W'(DDC_PORT_DDS_OFFSET),
                  CTRLPORT_ADDR_W'(DDC_PORT_SR_OFFSET),
                  CTRLPORT_ADDR_W'(DDC_PORT_AXI_RATE_OFFSET)}),
    .PORT_ADDR_W({DDC_PORT_DDS_ADDR_W,
                  DDC_PORT_SR_ADDR_W,
                  DDC_PORT_AXI_RATE_ADDR_W})
  ) ctrlport_decoder_param_axirate_other_ports (
    .ctrlport_clk            (ce_clk),
    .ctrlport_rst            (ce_rst),
    .s_ctrlport_req_wr       (ctrlport_req_wr),
    .s_ctrlport_req_rd       (ctrlport_req_rd),
    .s_ctrlport_req_addr     (ctrlport_req_addr),
    .s_ctrlport_req_data     (ctrlport_req_data),
    .s_ctrlport_req_byte_en  (ctrlport_req_byte_en),
    .s_ctrlport_req_has_time (ctrlport_req_has_time),
    .s_ctrlport_req_time     (ctrlport_req_time),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack),
    .s_ctrlport_resp_status  (ctrlport_resp_status),
    .s_ctrlport_resp_data    (ctrlport_resp_data),
    // slave connections
    .m_ctrlport_req_wr       ({ctrlport_dds_req_wr,
                               ctrlport_sr_req_wr,
                               ctrlport_axi_rate_req_wr}),
    .m_ctrlport_req_rd       ({ctrlport_dds_req_rd,
                               ctrlport_sr_req_rd,
                               ctrlport_axi_rate_req_rd}),
    .m_ctrlport_req_addr     ({ctrlport_dds_req_addr,
                               ctrlport_sr_req_addr,
                               ctrlport_axi_rate_req_addr}),
    .m_ctrlport_req_data     ({ctrlport_dds_req_data,
                               ctrlport_sr_req_data,
                               ctrlport_axi_rate_req_data}),
    .m_ctrlport_req_byte_en  ({ctrlport_dds_req_byte_en,
                               ctrlport_sr_req_byte_en,
                               ctrlport_axi_rate_req_byte_en}),
    .m_ctrlport_req_has_time ({ctrlport_dds_req_has_time,
                               ctrlport_sr_req_has_time,
                               ctrlport_axi_rate_req_has_time}),
    .m_ctrlport_req_time     ({ctrlport_dds_req_time,
                               ctrlport_sr_req_time,
                               ctrlport_axi_rate_req_time}),
    .m_ctrlport_resp_ack     ({ctrlport_dds_resp_ack,
                               ctrlport_sr_resp_ack,
                               ctrlport_axi_rate_resp_ack}),
    .m_ctrlport_resp_status  ({ctrlport_dds_resp_status,
                               ctrlport_sr_resp_status,
                               ctrlport_axi_rate_resp_status}),
    .m_ctrlport_resp_data    ({ctrlport_dds_resp_data,
                               ctrlport_sr_resp_data,
                               ctrlport_axi_rate_resp_data})
  );

  //---------------------------------------------------------------------------
  // AXI tag time module handles timed/untimed DDS freq shift commands and
  // generates a stream of one-hot encoded tags to the DDC chain.
  // Each tag bit corresponds to an IQ sample on the data bus and
  // indicates to the downstream DDS module within DDC when to apply the next
  // timed frequency change.
  // Tag and command are streamed to the DDC when command time arrives.
  //
  // parameters:
  //   SPC          : Number of samples processed per clock cycle.
  //                 Also used to determine the width of tags generated
  //   SAMP_W       : Width of an I+Q sample.
  //   TICK_RATE_W  : Width of the internal ticks_per_sample register (in bits).
  //   CMD_DATA_W   : Width of the DDS freq shift command data (in bits).
  //   MSB_ALIGN    : Left-align the DDS freq shift command data (convert from
  //                 format #Q0.31 to #Q0.24)
  //---------------------------------------------------------------------------

  axi_tag_time_ms #(
    .SPC          (SPC),
    .SAMP_W       (SAMP_W),
    .TICK_RATE_W  (TICK_RATE_W),
    .CMD_DATA_W   (PHASE_W),
    .MSB_ALIGN    (1)
  ) axi_tag_time_i (
    .clk                    (ce_clk),
    .rst                    (ce_rst),
    .cmd_fifo_full          (cmd_fifo_full),
    .s_axis_din_tdata       (m_axis_data_tdata),
    .s_axis_din_tlast       (m_axis_data_tlast),
    .s_axis_din_tvalid      (m_axis_data_tvalid),
    .s_axis_din_thas_time   (m_axis_data_thas_time),
    .s_axis_din_ttimestamp  (m_axis_data_ttimestamp),
    .s_axis_din_teob        (m_axis_data_teob),
    .s_axis_din_tlength     (m_axis_data_tlength),
    .s_axis_din_tready      (m_axis_data_tready),
    .m_axis_dout_tdata      (arc_in_tdata),
    .m_axis_dout_ttags      (arc_in_ttags),
    .m_axis_dout_tlast      (arc_in_tlast),
    .m_axis_dout_tvalid     (arc_in_tvalid),
    .m_axis_dout_thas_time  (arc_in_thas_time),
    .m_axis_dout_ttimestamp (arc_in_ttimestamp),
    .m_axis_dout_teob       (arc_in_teob),
    .m_axis_dout_tlength    (arc_in_tlength),
    .m_axis_dout_tready     (arc_in_tready),
    .m_axis_cmd_tdata       (cmd_out_tdata),
    .m_axis_cmd_tvalid      (cmd_out_tvalid),
    .m_axis_cmd_tuser       (cmd_out_tuser),
    .m_axis_cmd_tready      (cmd_out_tready),
    .ctrlport_req_wr        (ctrlport_dds_req_wr),
    .ctrlport_req_rd        (ctrlport_dds_req_rd),
    .ctrlport_req_addr      (ctrlport_dds_req_addr),
    .ctrlport_req_data      (ctrlport_dds_req_data),
    .ctrlport_req_has_time  (ctrlport_dds_req_has_time),
    .ctrlport_req_time      (ctrlport_dds_req_time),
    .ctrlport_resp_ack      (ctrlport_dds_resp_ack),
    .ctrlport_resp_data     (ctrlport_dds_resp_data),
    .ctrlport_resp_status   (ctrlport_dds_resp_status)
  );

  axi_rate_change_ms #(
    .WIDTH        (SAMP_W*SPC),
    .SPC          (SPC),
    .SPC_MTU_LOG2 (SPC_MTU_LOG2)
  ) axi_rate_change_ms_i (
    .clk                      (ce_clk),
    .rst                      (ce_rst),
    .clear                    (1'b0),
    .clear_user               (clear_user_i),
    .s_ctrlport_req_wr        (ctrlport_axi_rate_req_wr),
    .s_ctrlport_req_rd        (ctrlport_axi_rate_req_rd),
    .s_ctrlport_req_addr      (ctrlport_axi_rate_req_addr),
    .s_ctrlport_req_data      (ctrlport_axi_rate_req_data),
    .s_ctrlport_resp_ack      (ctrlport_axi_rate_resp_ack),
    .s_ctrlport_resp_status   (ctrlport_axi_rate_resp_status),
    .s_ctrlport_resp_data     (ctrlport_axi_rate_resp_data),
    .i_tdata                  (arc_in_tdata),
    .i_tlast                  (arc_in_tlast),
    .i_tvalid                 (arc_in_tvalid),
    .i_tready                 (arc_in_tready),
    .i_ttags                  (arc_in_ttags),
    .i_teob                   (arc_in_teob),
    .i_ttimestamp             (arc_in_ttimestamp),
    .i_thas_time              (arc_in_thas_time),
    .i_tlength                (arc_in_tlength),
    .o_tdata                  (s_axis_data_tdata),
    .o_tlast                  (s_axis_data_tlast),
    .o_tvalid                 (s_axis_data_tvalid),
    .o_tready                 (s_axis_data_tready),
    .o_teob                   (s_axis_data_teob),
    .o_ttimestamp             (s_axis_data_ttimestamp),
    .o_thas_time              (s_axis_data_thas_time),
    .m_axis_data_tdata        (ddc_in_tdata),
    .m_axis_data_tlast        (ddc_in_tlast),
    .m_axis_data_tvalid       (ddc_in_tvalid),
    .m_axis_data_tready       (ddc_in_tready),
    .m_axis_data_teob         (ddc_in_teob),
    .m_axis_data_ttags        (ddc_in_ttags),
    .s_axis_data_tdata        (ddc_out_tdata),
    .s_axis_data_tlast        (ddc_out_tlast),
    .s_axis_data_tvalid       (ddc_out_tvalid),
    .s_axis_data_tready       (ddc_out_tready)
  );

  ddc_ms #(
    .SPC           (SPC),
    .NUM_HB        (NUM_HB),
    .CIC_MAX_DECIM (CIC_MAX_DECIM),
    .SAMP_W        (SAMP_W),
    .PHASE_W       (PHASE_W)
  ) ddc_ms_i (
    .clk                    (ce_clk),
    .reset                  (ce_rst),
    .clear                  (clear_user_i),
    .s_ctrlport_req_wr      (ctrlport_sr_req_wr),
    .s_ctrlport_req_rd      (ctrlport_sr_req_rd),
    .s_ctrlport_req_addr    (ctrlport_sr_req_addr),
    .s_ctrlport_req_data    (ctrlport_sr_req_data),
    .s_ctrlport_req_byte_en (ctrlport_sr_req_byte_en),
    .s_ctrlport_req_has_time(ctrlport_sr_req_has_time),
    .s_ctrlport_req_time    (ctrlport_sr_req_time),
    .s_ctrlport_resp_ack    (ctrlport_sr_resp_ack),
    .s_ctrlport_resp_status (ctrlport_sr_resp_status),
    .s_ctrlport_resp_data   (ctrlport_sr_resp_data),
    .sample_in_tdata        (ddc_in_tdata),
    .sample_in_tvalid       (ddc_in_tvalid),
    .sample_in_tlast        (ddc_in_tlast),
    .sample_in_tready       (ddc_in_tready),
    .sample_in_tuser        ({ddc_in_ttags, ddc_in_teob}),
    .sample_out_tdata       (ddc_out_tdata),
    .sample_out_tvalid      (ddc_out_tvalid),
    .sample_out_tready      (ddc_out_tready),
    .sample_out_tlast       (ddc_out_tlast),
    .phase_in_tdata         (cmd_out_tdata),
    .phase_in_tvalid        (cmd_out_tvalid),
    .phase_in_tuser         (cmd_out_tuser),
    .phase_in_tready        (cmd_out_tready)
  );

endmodule

`default_nettype wire
