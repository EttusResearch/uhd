/////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//////////////////////////////////////////////////////////////////////

module fifo64_to_axi4lite
(
  input         s_axi_aclk,
  input         s_axi_areset,

  input [31:0]  s_axi_awaddr,
  input         s_axi_awvalid,
  output        s_axi_awready,
  input [31:0]  s_axi_wdata,
  input [3:0]   s_axi_wstrb,
  input         s_axi_wvalid,
  output        s_axi_wready,
  output [1:0]  s_axi_bresp,
  output        s_axi_bvalid,
  input         s_axi_bready,
  input [31:0]  s_axi_araddr,
  input         s_axi_arvalid,
  output        s_axi_arready,
  output [31:0] s_axi_rdata,
  output [1:0]  s_axi_rresp,
  output        s_axi_rvalid,
  input         s_axi_rready,

  output        m_axis_tvalid,
  output        m_axis_tlast,
  output [63:0] m_axis_tdata,
  input         m_axis_tready,
  output [3:0]  m_axis_tuser,

  input         s_axis_tvalid,
  input         s_axis_tlast,
  input [63:0]  s_axis_tdata,
  output        s_axis_tready,
  input [3:0]   s_axis_tuser,
  output        irq
);

  wire        clear_txn;
  wire [31:0] tx_tdata;
  wire        tx_tlast;
  wire        tx_tvalid;
  wire        tx_tready;
  wire [3:0]  tx_tkeep;
  wire [1:0]  tx_tuser = (tx_tkeep == 4'b1111) ? 2'd0 :
                        (tx_tkeep == 4'b0001) ? 2'd1 :
                        (tx_tkeep == 4'b0011) ? 2'd2 :
                        (tx_tkeep == 4'b0111) ? 2'd3 : 2'd0;

  axi_fifo32_to_fifo64 inst_axi_fifo32_to_fifo64
  (
    .clk(s_axi_aclk),
    .reset(s_axi_areset | ~clear_txn),
    .i_tdata({tx_tdata[7:0], tx_tdata[15:8], tx_tdata[23:16], tx_tdata[31:24]}), // endian swap
    .i_tuser(tx_tuser),
    .i_tlast(tx_tlast),
    .i_tvalid(tx_tvalid),
    .i_tready(tx_tready),
    .o_tdata(m_axis_tdata),
    .o_tuser(m_axis_tuser),
    .o_tlast(m_axis_tlast),
    .o_tvalid(m_axis_tvalid),
    .o_tready(m_axis_tready)
  );


  wire        clear_rxn;
  wire [31:0] rx_tdata;
  wire        rx_tlast;
  wire        rx_tvalid;
  wire        rx_tready;
  wire [1:0]  rx_tuser;

  axi_fifo64_to_fifo32 inst_axi_fifo64_to_fifo32
  (
    .clk(s_axi_aclk),
    .reset(s_axi_areset || ~clear_rxn),
    .i_tdata(s_axis_tdata),
    .i_tuser(s_axis_tuser[2:0]),
    .i_tlast(s_axis_tlast),
    .i_tvalid(s_axis_tvalid),
    .i_tready(s_axis_tready),
    .o_tdata({rx_tdata[7:0], rx_tdata[15:8], rx_tdata[23:16], rx_tdata[31:24]}), // endian swap
    .o_tuser(rx_tuser),
    .o_tlast(rx_tlast),
    .o_tvalid(rx_tvalid),
    .o_tready(rx_tready)
  );

  wire [3:0] rx_tkeep = ~rx_tlast ? 4'b1111 : (rx_tuser == 2'd0) ? 4'b1111 :
             (rx_tuser == 2'd1) ? 4'b0001 :
             (rx_tuser == 2'd2) ? 4'b0011 :
             (rx_tuser == 2'd3) ? 4'b0111 :
                                  4'b1111;

  axis_fifo_to_axi4lite inst_axis_fifo_to_axi4lite0
  (
    .interrupt(irq),
    .s_axi_aclk(s_axi_aclk),
    .s_axi_aresetn(~s_axi_areset),
    .s_axi_awaddr({16'h0000, s_axi_awaddr}),
    .s_axi_awvalid(s_axi_awvalid),
    .s_axi_awready(s_axi_awready),
    .s_axi_wdata(s_axi_wdata),
    .s_axi_wstrb(s_axi_wstrb),
    .s_axi_wvalid(s_axi_wvalid),
    .s_axi_wready(s_axi_wready),
    .s_axi_bresp(s_axi_bresp),
    .s_axi_bvalid(s_axi_bvalid),
    .s_axi_bready(s_axi_bready),
    .s_axi_araddr({16'h0000, s_axi_araddr}),
    .s_axi_arvalid(s_axi_arvalid),
    .s_axi_arready(s_axi_arready),
    .s_axi_rdata(s_axi_rdata),
    .s_axi_rresp(s_axi_rresp),
    .s_axi_rvalid(s_axi_rvalid),
    .s_axi_rready(s_axi_rready),
    .mm2s_prmry_reset_out_n(clear_txn),
    .axi_str_txd_tvalid(tx_tvalid),
    .axi_str_txd_tready(tx_tready),
    .axi_str_txd_tlast(tx_tlast),
    .axi_str_txd_tdata(tx_tdata),
    .axi_str_txd_tkeep(tx_tkeep),
    .s2mm_prmry_reset_out_n(clear_rxn),
    .axi_str_rxd_tvalid(rx_tvalid),
    .axi_str_rxd_tready(rx_tready),
    .axi_str_rxd_tlast(rx_tlast),
    .axi_str_rxd_tdata(rx_tdata),
    .axi_str_rxd_tkeep(rx_tkeep)
);

endmodule
