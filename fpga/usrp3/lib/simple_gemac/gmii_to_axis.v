//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module gmii_to_axis
#(
  parameter RX_FLOW_CTRL=0,
  parameter PORTNUM=8'd0
)
(
  input         clk125,
  input         reset,

  // GMII
  output        gmii_gtx_clk,

  output        gmii_tx_en,
  output        gmii_tx_er,
  output [7:0]  gmii_txd,

  input         gmii_rx_clk,
  input         gmii_rx_dv,
  input         gmii_rx_er,
  input [7:0]   gmii_rxd,

  // Client FIFO Interfaces
  input         sys_clk,
  output [63:0] rx_tdata,
  output [3:0]  rx_tuser,
  output        rx_tlast,
  output        rx_tvalid,
  input         rx_tready,

  input [63:0]  tx_tdata,
  input [3:0]   tx_tuser,
  input         tx_tlast,
  input         tx_tvalid,
  output        tx_tready,

  output [31:0] debug_rx,
  output [31:0] debug_tx
);

  wire clear = 1'b0;
  wire [7:0] rx_data, tx_data;
  wire tx_clk, tx_valid, tx_error, tx_ack;
  wire rx_clk, rx_valid, rx_error, rx_ack;

  wire        pause_req;
  wire        pause_request_en, pause_respect_en;
  wire [15:0] pause_time, pause_thresh, pause_time_req, rx_fifo_space;

  wire [31:0] debug_state;

  wire tx_reset, rx_reset;
  reset_sync reset_sync_tx
  (
    .clk(tx_clk),
    .reset_in(reset),
    .reset_out(tx_reset)
  );

  reset_sync reset_sync_rx (
    .clk(rx_clk),
    .reset_in(reset),
    .reset_out(rx_reset)
  );

  simple_gemac simple_gemac
  (
    .clk125(clk125),  .reset(reset),
    .GMII_GTX_CLK(gmii_gtx_clk),
    .GMII_TX_EN(gmii_tx_en),
    .GMII_TX_ER(gmii_tx_er),
    .GMII_TXD(gmii_txd),
    .GMII_RX_CLK(gmii_rx_clk),
    .GMII_RX_DV(gmii_rx_dv),
    .GMII_RX_ER(gmii_rx_er),
    .GMII_RXD(gmii_rxd),
    .pause_req(RX_FLOW_CTRL ? pause_req : 1'b0),
    .pause_time_req(RX_FLOW_CTRL ? pause_time_req : 16'd0),
    .pause_respect_en(pause_respect_en),
    .ucast_addr(48'h0),
    .mcast_addr(48'h0),
    .pass_ucast(1'b0),
    .pass_mcast(1'b0),
    .pass_bcast(1'b0),
    .pass_pause(1'b0),
    .pass_all(1'b1),
    .rx_clk(rx_clk),
    .rx_data(rx_data),
    .rx_valid(rx_valid),
    .rx_error(rx_error),
    .rx_ack(rx_ack),
    .tx_clk(tx_clk),
    .tx_data(tx_data),
    .tx_valid(tx_valid),
    .tx_error(tx_error),
    .tx_ack(tx_ack),
    .debug(debug_state)
  );

  assign pause_respect_en = 1'b0;
  assign pause_request_en = 1'b0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // RX FIFO Chain
  wire        rx_ll_eof;
  wire        rx_ll_error;
  wire        rx_ll_src_rdy;
  wire        rx_ll_dst_rdy;
  wire [7:0]  rx_ll_data;

  wire [63:0] rx_tdata_int;
  wire [3:0]  rx_tuser_int;
  wire        rx_tlast_int;
  wire        rx_tvalid_int;
  wire        rx_tready_int;

  rxmac_to_ll8 rxmac_to_ll8
  (
    .clk(rx_clk),
    .reset(rx_reset),
    .clear(clear),
    .rx_data(rx_data),
    .rx_valid(rx_valid),
    .rx_error(rx_error),
    .rx_ack(rx_ack),

    .ll_data(rx_ll_data),
    .ll_sof(),
    .ll_eof(rx_ll_eof),
    .ll_error(rx_ll_error),  // ignore sof
    .ll_src_rdy(rx_ll_src_rdy),
    .ll_dst_rdy(rx_ll_dst_rdy)
  );

  ll8_to_axi64 #(.START_BYTE(6), .LABEL(PORTNUM)) ll8_to_axi64
  (
    .clk(rx_clk),
    .reset(rx_reset),
    .clear(clear),
    .ll_data(rx_ll_data),
    .ll_eof(rx_ll_eof),
    .ll_error(rx_ll_error),
    .ll_src_rdy(rx_ll_src_rdy),
    .ll_dst_rdy(rx_ll_dst_rdy),
    .axi64_tdata(rx_tdata_int),
    .axi64_tlast(rx_tlast_int),
    .axi64_tuser(rx_tuser_int),
    .axi64_tvalid(rx_tvalid_int),
    .axi64_tready(rx_tready_int)
  );

  axi64_8k_2clk_fifo rxfifo_2clk
  (
    .s_aresetn(~rx_reset),
    .s_aclk(rx_clk),
    .s_axis_tvalid(rx_tvalid_int),
    .s_axis_tready(rx_tready_int),
    .s_axis_tdata(rx_tdata_int),
    .s_axis_tlast(rx_tlast_int),
    .s_axis_tuser(rx_tuser_int),
    .axis_wr_data_count(),

    .m_aclk(sys_clk),
    .m_axis_tvalid(rx_tvalid),
    .m_axis_tready(rx_tready),
    .m_axis_tdata(rx_tdata),
    .m_axis_tlast(rx_tlast),
    .m_axis_tuser(rx_tuser),
    .axis_rd_data_count()
  );

  ///////////////////////////////////////////////////////////////////////////////////////
  // TX FIFO Chain
  wire tx_ll_eof, tx_ll_src_rdy, tx_ll_dst_rdy;
  wire [7:0] tx_ll_data;

  wire [63:0] tx_tdata_int;
  wire [3:0]  tx_tuser_int;
  wire        tx_tlast_int;
  wire        tx_tvalid_int;
  wire        tx_tready_int;

  axi64_8k_2clk_fifo txfifo_2clk
  (
    .s_aresetn(~tx_reset),
    .s_aclk(sys_clk),
    .s_axis_tvalid(tx_tvalid),
    .s_axis_tready(tx_tready),
    .s_axis_tdata(tx_tdata),
    .s_axis_tlast(tx_tlast),
    .s_axis_tuser(tx_tuser),
    .axis_wr_data_count(),
    .m_aclk(tx_clk),
    .m_axis_tvalid(tx_tvalid_int),
    .m_axis_tready(tx_tready_int),
    .m_axis_tdata(tx_tdata_int),
    .m_axis_tlast(tx_tlast_int),
    .m_axis_tuser(tx_tuser_int),
    .axis_rd_data_count()
  );

  axi64_to_ll8 #(.START_BYTE(6)) axi64_to_ll8
  (
    .clk(tx_clk),
    .reset(tx_reset),
    .clear(clear),
    .axi64_tdata(tx_tdata_int),
    .axi64_tlast(tx_tlast_int),
    .axi64_tuser(tx_tuser_int),
    .axi64_tvalid(tx_tvalid_int),
    .axi64_tready(tx_tready_int),
    .ll_data(tx_ll_data),
    .ll_eof(tx_ll_eof),
    .ll_src_rdy(tx_ll_src_rdy),
    .ll_dst_rdy(tx_ll_dst_rdy)
  );

  ll8_to_txmac ll8_to_txmac
  (
    .clk(tx_clk),
    .reset(tx_reset),
    .clear(clear),
    .ll_data(tx_ll_data),
    .ll_eof(tx_ll_eof),
    .ll_src_rdy(tx_ll_src_rdy),
    .ll_dst_rdy(tx_ll_dst_rdy),
    .tx_data(tx_data),
    .tx_valid(tx_valid),
    .tx_error(tx_error),
    .tx_ack(tx_ack)
  );

endmodule
