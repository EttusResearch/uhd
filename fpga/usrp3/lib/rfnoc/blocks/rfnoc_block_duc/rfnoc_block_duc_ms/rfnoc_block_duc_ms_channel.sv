//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_ms_channel
//
// Description:
//   Per-channel wrapper for rfnoc_block_duc_ms.
//   This module instantiates the
//     - per-port ctrlport decode split
//     - axi_rate_change_ms, and
//     - duc_ms (actual signal processing) chain.
//
// Parameters:
//   NUM_HB        : Number of halfband filter stages to implement in each DDC chain.
//                   This determines the maximum decimation rate of the halfband filters,
//                   which is 2^NUM_HB.
//   CIC_MAX_INTERP: Maximum interpolation rate of the CIC filter in each DUC chain.
//                   The total maximum interpolation rate of the DUC is
//                   CIC_MAX_INTERP * 2^NUM_HB.
//   SAMP_W        : Width of an I+Q sample.
//   SPC           : Number of samples processed per clock cycle.
//                   This determines the width of the input and output data streams,
//                   which are SAMP_W*SPC bits wide.
//                   The DUC will process SPC samples in parallel every clock cycle.
//   SPC_MTU_LOG2  : Log2 of the maximum number of words (SAMP_W*SPC-wide) per packet.
//
//

`default_nettype none

module rfnoc_block_duc_ms_channel
  import ctrlport_pkg::*;
  import rfnoc_chdr_utils_pkg::*;
#(
  parameter NUM_HB         = 3,
  parameter CIC_MAX_INTERP = 255,
  parameter SAMP_W         = 32,
  parameter SPC            = 1,
  parameter SPC_MTU_LOG2   = 10
) (
  input  wire                 ce_clk,
  input  wire                 ce_rst,

  // CTRL port for this channel
  input  wire                          ctrlport_req_wr,
  input  wire                          ctrlport_req_rd,
  input  wire [   CTRLPORT_ADDR_W-1:0] ctrlport_req_addr,
  input  wire [   CTRLPORT_DATA_W-1:0] ctrlport_req_data,
  input  wire [CTRLPORT_BYTE_EN_W-1:0] ctrlport_req_byte_en,
  input  wire                          ctrlport_req_has_time,
  input  wire [   CTRLPORT_TIME_W-1:0] ctrlport_req_time,
  output wire                          ctrlport_resp_ack,
  output wire [    CTRLPORT_STS_W-1:0] ctrlport_resp_status,
  output wire [   CTRLPORT_DATA_W-1:0] ctrlport_resp_data,

  // Input data from NoC shell
  input  wire [SPC-1:0][   SAMP_W-1:0] m_axis_data_tdata,
  input  wire                          m_axis_data_tlast,
  input  wire                          m_axis_data_tvalid,
  output wire                          m_axis_data_tready,
  input  wire [  CHDR_TIMESTAMP_W-1:0] m_axis_data_ttimestamp,
  input  wire                          m_axis_data_thas_time,
  input  wire [     CHDR_LENGTH_W-1:0] m_axis_data_tlength,
  input  wire                          m_axis_data_teob,

  // Output data to NoC shell
  output wire [SPC-1:0][   SAMP_W-1:0] s_axis_data_tdata,
  output wire                          s_axis_data_tlast,
  output wire                          s_axis_data_tvalid,
  input  wire                          s_axis_data_tready,
  output wire                          s_axis_data_teob,
  output wire [  CHDR_TIMESTAMP_W-1:0] s_axis_data_ttimestamp,
  output wire                          s_axis_data_thas_time,
  output wire [     CHDR_LENGTH_W-1:0] s_axis_data_tlength
);

  import rfnoc_block_duc_ms_pkg::*;

  // Maximum interpolation rate of the DUC
  localparam int DDS_PHASE_W     = 24;
  localparam int TICK_RATE_W     = 16;
  localparam int DDS_SAMP_FRAC_W = 15;
  // LEFT-ALIGN if the DDS_PHASE_W is less than the CTRLPORT_DATA_W
  // and the DDS_PHASE is a fully fractional value.
  localparam int MSB_ALIGN       = 1;

  // Local signals for ctrlport interface
  // Axi rate change ctrlport signals
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

  // Sampling rate change (SR) ctrlport signals
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

  // DDS ctrlport signals
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


  // AXI-S bus signals to and from the duc_ms block
  logic [SPC-1:0][   SAMP_W-1:0] duc_in_tdata;
  logic                          duc_in_tlast;
  logic                          duc_in_tvalid;
  logic                          duc_in_tready;

  logic [SPC-1:0][   SAMP_W-1:0] duc_out_tdata;
  logic                          duc_out_tlast;
  logic                          duc_out_tvalid;
  logic                          duc_out_tready;

  // AXI-S bus signals to and from axi_tag_time_ms
  logic [SPC-1:0][   SAMP_W-1:0] pre_tag_tdata;
  logic                          pre_tag_tlast;
  logic                          pre_tag_tvalid;
  logic                          pre_tag_tready;
  logic                          pre_tag_teob;
  logic [  CHDR_TIMESTAMP_W-1:0] pre_tag_ttimestamp;
  logic                          pre_tag_thas_time;


  logic [SPC-1:0][   SAMP_W-1:0] post_tag_tdata;
  logic [               SPC-1:0] post_tag_ttags;
  logic                          post_tag_tlast;
  logic                          post_tag_tvalid;
  logic                          post_tag_tready;
  logic                          post_tag_teob;
  logic [  CHDR_TIMESTAMP_W-1:0] post_tag_ttimestamp;
  logic                          post_tag_thas_time;

  logic [       DDS_PHASE_W-1:0] dds_cmd_tdata;
  logic                          dds_cmd_tvalid;
  logic                          dds_cmd_tready;
  logic                          dds_cmd_tuser;



  // Clear signal from axi_rate_change_ms to clear processing chain.
  logic                          clear_user_i;

  //---------------------------------------------------------------------------
  // Split DUC port-specific register address range into:
  //   - AXI rate change sub-port
  //   - Sampling rate change (SR) sub-port
  //   - DDS sub-port
  //---------------------------------------------------------------------------

  ctrlport_decoder_param #(
    .NUM_SLAVES (3                                           ),
    .PORT_BASE  ({CTRLPORT_ADDR_W'(DUC_PORT_DDS_OFFSET),
                  CTRLPORT_ADDR_W'(DUC_PORT_SR_OFFSET),
                  CTRLPORT_ADDR_W'(DUC_PORT_AXI_RATE_OFFSET)}),
    .PORT_ADDR_W({DUC_PORT_DDS_ADDR_W,
                  DUC_PORT_SR_ADDR_W,
                  DUC_PORT_AXI_RATE_ADDR_W}                  )
  ) ctrlport_decoder_duc_channel_x (
    .ctrlport_clk           (ce_clk                          ),
    .ctrlport_rst           (ce_rst                          ),
    .s_ctrlport_req_wr      (ctrlport_req_wr                 ),
    .s_ctrlport_req_rd      (ctrlport_req_rd                 ),
    .s_ctrlport_req_addr    (ctrlport_req_addr               ),
    .s_ctrlport_req_data    (ctrlport_req_data               ),
    .s_ctrlport_req_byte_en (ctrlport_req_byte_en            ),
    .s_ctrlport_req_has_time(ctrlport_req_has_time           ),
    .s_ctrlport_req_time    (ctrlport_req_time               ),
    .s_ctrlport_resp_ack    (ctrlport_resp_ack               ),
    .s_ctrlport_resp_status (ctrlport_resp_status            ),
    .s_ctrlport_resp_data   (ctrlport_resp_data              ),
    // slave connections
    .m_ctrlport_req_wr      ({ctrlport_dds_req_wr,
                              ctrlport_sr_req_wr,
                              ctrlport_axi_rate_req_wr}      ),
    .m_ctrlport_req_rd      ({ctrlport_dds_req_rd,
                             ctrlport_sr_req_rd,
                             ctrlport_axi_rate_req_rd}       ),
    .m_ctrlport_req_addr    ({ctrlport_dds_req_addr,
                              ctrlport_sr_req_addr,
                              ctrlport_axi_rate_req_addr}    ),
    .m_ctrlport_req_data    ({ctrlport_dds_req_data,
                              ctrlport_sr_req_data,
                              ctrlport_axi_rate_req_data}    ),
    .m_ctrlport_req_byte_en ({ctrlport_dds_req_byte_en,
                              ctrlport_sr_req_byte_en,
                              ctrlport_axi_rate_req_byte_en} ),
    .m_ctrlport_req_has_time({ctrlport_dds_req_has_time,
                              ctrlport_sr_req_has_time,
                              ctrlport_axi_rate_req_has_time}),
    .m_ctrlport_req_time    ({ctrlport_dds_req_time,
                              ctrlport_sr_req_time,
                              ctrlport_axi_rate_req_time}    ),
    .m_ctrlport_resp_ack    ({ctrlport_dds_resp_ack,
                              ctrlport_sr_resp_ack,
                              ctrlport_axi_rate_resp_ack}    ),
    .m_ctrlport_resp_status ({ctrlport_dds_resp_status,
                              ctrlport_sr_resp_status,
                              ctrlport_axi_rate_resp_status} ),
    .m_ctrlport_resp_data   ({ctrlport_dds_resp_data,
                              ctrlport_sr_resp_data,
                              ctrlport_axi_rate_resp_data}   )
  );


  axi_rate_change_ms #(
    .WIDTH       (SAMP_W*SPC  ),
    .SPC         (SPC         ),
    .SPC_MTU_LOG2(SPC_MTU_LOG2)
  ) axi_rate_change_ms_i (
    .clk                    (ce_clk                       ),
    .rst                    (ce_rst                       ),
    .clear                  (1'b0                         ),
    .clear_user             (clear_user_i                 ),
    .s_ctrlport_req_wr      (ctrlport_axi_rate_req_wr     ),
    .s_ctrlport_req_rd      (ctrlport_axi_rate_req_rd     ),
    .s_ctrlport_req_addr    (ctrlport_axi_rate_req_addr   ),
    .s_ctrlport_req_data    (ctrlport_axi_rate_req_data   ),
    .s_ctrlport_resp_ack    (ctrlport_axi_rate_resp_ack   ),
    .s_ctrlport_resp_status (ctrlport_axi_rate_resp_status),
    .s_ctrlport_resp_data   (ctrlport_axi_rate_resp_data  ),
    .i_tdata                (m_axis_data_tdata            ),
    .i_tlast                (m_axis_data_tlast            ),
    .i_tvalid               (m_axis_data_tvalid           ),
    .i_tready               (m_axis_data_tready           ),
    .i_ttags                ('0                           ),
    .i_teob                 (m_axis_data_teob             ),
    .i_ttimestamp           (m_axis_data_ttimestamp       ),
    .i_thas_time            (m_axis_data_thas_time        ),
    .i_tlength              (m_axis_data_tlength          ),
    .o_tdata                (pre_tag_tdata                ),
    .o_tlast                (pre_tag_tlast                ),
    .o_tvalid               (pre_tag_tvalid               ),
    .o_tready               (pre_tag_tready               ),
    .o_teob                 (pre_tag_teob                 ),
    .o_ttimestamp           (pre_tag_ttimestamp           ),
    .o_thas_time            (pre_tag_thas_time            ),
    .m_axis_data_tdata      (duc_in_tdata                 ),
    .m_axis_data_tlast      (duc_in_tlast                 ),
    .m_axis_data_tvalid     (duc_in_tvalid                ),
    .m_axis_data_tready     (duc_in_tready                ),
    .m_axis_data_ttags      (/* unused */                 ),
    .m_axis_data_teob       (/* unused */                 ),
    .s_axis_data_tdata      (duc_out_tdata                ),
    .s_axis_data_tlast      (duc_out_tlast                ),
    .s_axis_data_tvalid     (duc_out_tvalid               ),
    .s_axis_data_tready     (duc_out_tready               )
  );

  // DUC processing chain.
  duc_ms #(
    .NUM_HB         (NUM_HB        ),
    .CIC_MAX_INTERP (CIC_MAX_INTERP),
    .SAMP_W         (SAMP_W        ),
    .SPC            (SPC           )
  ) duc_ms_i (
    .clk                    (ce_clk                  ),
    .reset                  (ce_rst                  ),
    .clear                  (clear_user_i            ),
    .s_ctrlport_req_wr      (ctrlport_sr_req_wr      ),
    .s_ctrlport_req_rd      (ctrlport_sr_req_rd      ),
    .s_ctrlport_req_addr    (ctrlport_sr_req_addr    ),
    .s_ctrlport_req_data    (ctrlport_sr_req_data    ),
    .s_ctrlport_req_byte_en (ctrlport_sr_req_byte_en ),
    .s_ctrlport_req_has_time(ctrlport_sr_req_has_time),
    .s_ctrlport_req_time    (ctrlport_sr_req_time    ),
    .s_ctrlport_resp_ack    (ctrlport_sr_resp_ack    ),
    .s_ctrlport_resp_status (ctrlport_sr_resp_status ),
    .s_ctrlport_resp_data   (ctrlport_sr_resp_data   ),
    .sample_in_tdata        (duc_in_tdata            ),
    .sample_in_tlast        (duc_in_tlast            ),
    .sample_in_tvalid       (duc_in_tvalid           ),
    .sample_in_tready       (duc_in_tready           ),
    .sample_out_tdata       (duc_out_tdata           ),
    .sample_out_tlast       (duc_out_tlast           ),
    .sample_out_tvalid      (duc_out_tvalid          ),
    .sample_out_tready      (duc_out_tready          )
  );

  axi_tag_time_ms #(
    .SPC          (SPC        ),
    .SAMP_W       (SAMP_W     ),
    .TICK_RATE_W  (TICK_RATE_W),
    .CMD_DATA_W   (DDS_PHASE_W),
    .MSB_ALIGN    (MSB_ALIGN  )
  ) axi_tag_time_ms_i (
    .clk                    (ce_clk                   ),
    .rst                    (ce_rst                   ),
    .cmd_fifo_full          (                         ),
    .s_axis_din_tdata       (pre_tag_tdata            ),
    .s_axis_din_tlast       (pre_tag_tlast            ),
    .s_axis_din_tvalid      (pre_tag_tvalid           ),
    .s_axis_din_thas_time   (pre_tag_thas_time        ),
    .s_axis_din_ttimestamp  (pre_tag_ttimestamp       ),
    .s_axis_din_teob        (pre_tag_teob             ),
    .s_axis_din_tlength     ('0                       ),
    .s_axis_din_tready      (pre_tag_tready           ),
    .m_axis_dout_tdata      (post_tag_tdata           ),
    .m_axis_dout_ttags      (post_tag_ttags           ),
    .m_axis_dout_tlast      (post_tag_tlast           ),
    .m_axis_dout_tvalid     (post_tag_tvalid          ),
    .m_axis_dout_thas_time  (post_tag_thas_time       ),
    .m_axis_dout_ttimestamp (post_tag_ttimestamp      ),
    .m_axis_dout_teob       (post_tag_teob            ),
    .m_axis_dout_tlength    (/* unused */             ),
    .m_axis_dout_tready     (post_tag_tready          ),
    .ctrlport_req_wr        (ctrlport_dds_req_wr      ),
    .ctrlport_req_rd        (ctrlport_dds_req_rd      ),
    .ctrlport_req_addr      (ctrlport_dds_req_addr    ),
    .ctrlport_req_data      (ctrlport_dds_req_data    ),
    .ctrlport_req_has_time  (ctrlport_dds_req_has_time),
    .ctrlport_req_time      (ctrlport_dds_req_time    ),
    .ctrlport_resp_ack      (ctrlport_dds_resp_ack    ),
    .ctrlport_resp_data     (ctrlport_dds_resp_data   ),
    .ctrlport_resp_status   (ctrlport_dds_resp_status ),
    .m_axis_cmd_tdata       (dds_cmd_tdata            ),
    .m_axis_cmd_tvalid      (dds_cmd_tvalid           ),
    .m_axis_cmd_tuser       (dds_cmd_tuser            ),
    .m_axis_cmd_tready      (dds_cmd_tready           )
  );

  dds_ms #(
    .SPC          (SPC            ),
    .SAMP_W       (SAMP_W         ),
    .SAMP_FRAC_W  (DDS_SAMP_FRAC_W),
    .PHASE_WIDTH  (DDS_PHASE_W    )
  ) duc_dds_ms_i (
    .clk                (ce_clk            ),
    .rst                (ce_rst            ),
    .s_axis_din_tdata   (post_tag_tdata    ),
    .s_axis_din_tlast   (post_tag_tlast    ),
    .s_axis_din_tready  (post_tag_tready   ),
    .s_axis_din_tvalid  (post_tag_tvalid   ),
    .s_axis_din_tuser   ({post_tag_ttags,
                          post_tag_teob}   ),
    .s_axis_phase_tdata (dds_cmd_tdata     ),
    .s_axis_phase_tready(dds_cmd_tready    ),
    .s_axis_phase_tvalid(dds_cmd_tvalid    ),
    .s_axis_phase_tuser (dds_cmd_tuser     ),
    .m_axis_dout_tdata  (s_axis_data_tdata ),
    .m_axis_dout_tlast  (s_axis_data_tlast ),
    .m_axis_dout_tready (s_axis_data_tready),
    .m_axis_dout_tvalid (s_axis_data_tvalid),
    .m_axis_dout_tuser  (s_axis_data_teob  )
  );

  // Delay the axis sideband signals that bypass the dds
  // to align with the delayed data path through the dds.
  //  - post_tag_ttimestamp
  //  - post_tag_thas_time
  //
  // The dds is a fixed-latency AXI-Stream pipeline, so rather than tracking the
  // exact pipeline depth we buffer the bypass sideband in a small FIFO: a word
  // is pushed every time a sample word is accepted into the dds and popped every
  // time a sample word leaves the dds. This keeps the sideband aligned with the
  // data regardless of the dds latency.
  localparam int SIDEBAND_W = CHDR_TIMESTAMP_W + 1;

  logic sideband_push;
  logic sideband_pop;

  assign sideband_push = post_tag_tvalid  & post_tag_tready;
  assign sideband_pop  = s_axis_data_tvalid & s_axis_data_tready;

  axi_fifo #(
    .WIDTH(SIDEBAND_W),
    .SIZE (5)
  ) sideband_delay_fifo_i (
    .clk     (ce_clk),
    .reset   (ce_rst),
    .clear   (1'b0),
    .i_tdata ({post_tag_ttimestamp, post_tag_thas_time}),
    .i_tvalid(sideband_push),
    .i_tready(),
    .o_tdata ({s_axis_data_ttimestamp, s_axis_data_thas_time}),
    .o_tvalid(),
    .o_tready(sideband_pop),
    .space   (),
    .occupied()
  );

endmodule : rfnoc_block_duc_ms_channel

`default_nettype wire
