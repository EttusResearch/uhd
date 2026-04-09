//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: b310
//
// Description:
//   This module implements the PCIe interface for the B310. It is responsible
//   for merging the DMA channels connected to the Host IP into a two streams
//   (1 for TX and 1 for RX) that are connected to the crossbar.
//
// Parameters:
// - DMA_STREAM_WIDTH: Width of the data bus. Currently 128 bits expected.
// - NUM_TX_STREAMS: Number of TX FIFOs.
// - NUM_RX_STREAMS: Number of RX FIFOs. Note: Despite having two different
//                   parameters, NUM_TX_STREAMS and NUM_RX_STREAMS need to be
//                   identical.

module b310_pcie_int #(
  int DMA_STREAM_WIDTH = 128,
  int NUM_TX_STREAMS   = 5,
  int NUM_RX_STREAMS   = 5,
  int BUS_CLK_RATE     = 166_666_666
) (
  //---------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------
  input wire          reg_clk,
  input wire          dma_clk,
  input wire          bus_clk,
  input wire          bus_rst,

  //---------------------------------------------------------
  // DMA streams to/from InchWorm (Domain: dma_clk)
  //---------------------------------------------------------
  input  wire [NUM_TX_STREAMS-1:0] [DMA_STREAM_WIDTH-1:0]  host_dma_tx_tdata,
  input  wire [NUM_TX_STREAMS-1:0]                         host_dma_tx_tvalid,
  output wire [NUM_TX_STREAMS-1:0]                         host_dma_tx_tready,

  output wire [NUM_RX_STREAMS-1:0] [DMA_STREAM_WIDTH-1:0]  host_dma_rx_tdata,
  output wire [NUM_RX_STREAMS-1:0]                         host_dma_rx_tvalid,
  input  wire [NUM_RX_STREAMS-1:0]                         host_dma_rx_tready,

  //---------------------------------------------------------
  // DMA stream to/from crossbar (Domain: bus_clk)
  //---------------------------------------------------------
  output wire [DMA_STREAM_WIDTH-1:0]        dma_tx_tdata,
  output wire [2:0]                         dma_tx_tuser,
  output wire                               dma_tx_tvalid,
  output wire                               dma_tx_tlast,
  input  wire                               dma_tx_tready,

  input  wire [DMA_STREAM_WIDTH-1:0]        dma_rx_tdata,
  input  wire [2:0]                         dma_rx_tuser,
  input  wire                               dma_rx_tvalid,
  input  wire                               dma_rx_tlast,
  output wire                               dma_rx_tready,

  //---------------------------------------------------------
  // CtrlPort register interface (Domain: reg_clk)
  //---------------------------------------------------------
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output wire        s_ctrlport_resp_ack,
  output wire  [1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------
  // Misc
  //---------------------------------------------------------
  input  wire  [15:0]  misc_status,
  output wire [127:0]  debug
);

  localparam DMA_CLK_XING_FIFO_SIZE = 5;  //Will synthesize fifo_short_2clk
  localparam DMA_PKT_GATE_FIFO_SIZE = 11; //Room for 2 8k packets
  localparam DMA_FRAME_SIZE_WIDTH   = 16;
  localparam DMA_RX_DEST_WIDTH      = $clog2(NUM_RX_STREAMS);


  wire [NUM_TX_STREAMS-1:0]                               dma_tx_clear, dma_tx_enabled;
  wire [NUM_TX_STREAMS-1:0]                               dma_tx_samp_stb, dma_tx_pkt_stb;
  wire [NUM_TX_STREAMS-1:0]                               dma_tx_busy, dma_tx_error;
  wire [NUM_TX_STREAMS-1:0]   [DMA_FRAME_SIZE_WIDTH-1:0]  dma_tx_frame_size;

  wire [NUM_RX_STREAMS-1:0]                               dma_rx_clear, dma_rx_enabled;
  wire [NUM_RX_STREAMS-1:0]                               dma_rx_samp_stb, dma_rx_pkt_stb;
  wire [NUM_RX_STREAMS-1:0]                               dma_rx_busy, dma_rx_error;
  wire [NUM_RX_STREAMS-1:0]   [DMA_FRAME_SIZE_WIDTH-1:0]  dma_rx_frame_size;
  wire [DMA_STREAM_WIDTH-1:0]                             dma_rx_header;

  //*******************************************************************************
  // CtrlPort interface conversion + crossing + decode
  //
  // Address map (0x200-address windows):
  //   Slave 0 (basic_regs):  0x00000 - 0x001FF
  //   Slave 1 (tx_dma_ctrl): 0x00200 - 0x003FF
  //   Slave 2 (rx_dma_ctrl): 0x00400 - 0x005FF
  //
  typedef enum int {
    BASIC_REGS,
    TX_DMA,
    RX_DMA
  } ctrlport_ep_t;
  localparam int NUM_EP = 3;

  localparam int EP_PORT_BASE [NUM_EP] = '{
    'h00000,  // BASIC_REGS
    'h00200,  // TX_DMA
    'h00400   // RX_DMA
  };
  localparam int EP_PORT_SIZE [NUM_EP] = '{
    'h00200,  // BASIC_REGS
    'h00200,  // TX_DMA
    'h00200   // RX_DMA
  };

  ctrlport_if reg_ctrlport_if (.clk(reg_clk), .rst(bus_rst));
  ctrlport_if bclk_ctrlport_if (.clk(bus_clk), .rst(bus_rst));
  ctrlport_if ep_ctrlport_if [NUM_EP] (
    .clk(bus_clk),
    .rst(bus_rst)
  );

  assign reg_ctrlport_if.req.wr            = s_ctrlport_req_wr;
  assign reg_ctrlport_if.req.rd            = s_ctrlport_req_rd;
  assign reg_ctrlport_if.req.addr          = s_ctrlport_req_addr;
  assign reg_ctrlport_if.req.port_id       = '0;
  assign reg_ctrlport_if.req.remote_epid   = '0;
  assign reg_ctrlport_if.req.remote_portid = '0;
  assign reg_ctrlport_if.req.data          = s_ctrlport_req_data;
  assign reg_ctrlport_if.req.byte_en       = '1;
  assign reg_ctrlport_if.req.has_time      = 1'b0;
  assign reg_ctrlport_if.req.timestamp     = '0;

  assign s_ctrlport_resp_ack    = reg_ctrlport_if.resp.ack;
  assign s_ctrlport_resp_status = reg_ctrlport_if.resp.status;
  assign s_ctrlport_resp_data   = reg_ctrlport_if.resp.data;


  //CtrlPort clock crossing: reg_clk -> bus_clk
  ctrlport_if_clk_cross reg_to_bus_clk_cross (
    .s_ctrlport(reg_ctrlport_if),
    .m_ctrlport(bclk_ctrlport_if)
  );

  ctrlport_if_decoder #(
    .NUM_SLAVES(NUM_EP),
    .PORT_BASE (EP_PORT_BASE),
    .PORT_SIZE (EP_PORT_SIZE)
  ) ctrlport_decoder (
    .s_ctrlport(bclk_ctrlport_if),
    .m_ctrlport(ep_ctrlport_if)
  );

  wire [63:0] basic_regi_tdata,  dma_tx_regi_tdata,  dma_rx_regi_tdata;
  wire        basic_regi_tvalid, dma_tx_regi_tvalid, dma_rx_regi_tvalid;
  wire        basic_regi_tready, dma_tx_regi_tready, dma_rx_regi_tready;
  wire [63:0] basic_rego_tdata,  dma_tx_rego_tdata,  dma_rx_rego_tdata;
  wire        basic_rego_tvalid, dma_tx_rego_tvalid, dma_rx_rego_tvalid;
  wire        basic_rego_tready, dma_tx_rego_tready, dma_rx_rego_tready;

  // Slave 0: pcie_basic_regs
  ctrlport_to_ioport2_stream basic_regs_bridge (
    .s_ctrlport             (ep_ctrlport_if[BASIC_REGS]),
    .regi_tdata             (basic_regi_tdata),
    .regi_tvalid            (basic_regi_tvalid),
    .regi_tready            (basic_regi_tready),
    .rego_tdata             (basic_rego_tdata),
    .rego_tvalid            (basic_rego_tvalid),
    .rego_tready            (basic_rego_tready)
  );

  // Slave 1: tx_dma_ctrl
  ctrlport_to_ioport2_stream tx_dma_bridge (
    .s_ctrlport             (ep_ctrlport_if[TX_DMA]),
    .regi_tdata             (dma_tx_regi_tdata),
    .regi_tvalid            (dma_tx_regi_tvalid),
    .regi_tready            (dma_tx_regi_tready),
    .rego_tdata             (dma_tx_rego_tdata),
    .rego_tvalid            (dma_tx_rego_tvalid),
    .rego_tready            (dma_tx_rego_tready)
  );

  // Slave 2: rx_dma_ctrl
  ctrlport_to_ioport2_stream rx_dma_bridge (
    .s_ctrlport             (ep_ctrlport_if[RX_DMA]),
    .regi_tdata             (dma_rx_regi_tdata),
    .regi_tvalid            (dma_rx_regi_tvalid),
    .regi_tready            (dma_rx_regi_tready),
    .rego_tdata             (dma_rx_rego_tdata),
    .rego_tvalid            (dma_rx_rego_tvalid),
    .rego_tready            (dma_rx_rego_tready)
  );

  wire [15:0] fpga_status;
  assign fpga_status[7:0]    = {|(dma_tx_error), 1'b0, dma_tx_enabled};
  assign fpga_status[15:8]   = {|(dma_rx_error), 1'b0, dma_rx_enabled};

  pcie_basic_regs #(
    .SIGNATURE    (32'h42333130 /*ASCII:"B310"*/),
    .CLK_FREQ     (BUS_CLK_RATE)
  ) basic_regs (
    .clk          (bus_clk),
    .reset        (bus_rst),
    .regi_tdata   (basic_regi_tdata),
    .regi_tvalid  (basic_regi_tvalid),
    .regi_tready  (basic_regi_tready),
    .rego_tdata   (basic_rego_tdata),
    .rego_tvalid  (basic_rego_tvalid),
    .rego_tready  (basic_rego_tready),
    .misc_status  ({fpga_status, misc_status})
  );

  pcie_dma_ctrl #(
    .NUM_STREAMS      (NUM_TX_STREAMS),
    .FRAME_SIZE_W     (DMA_FRAME_SIZE_WIDTH),
    .REG_BASE_ADDR    (20'h00000), // offset 0x200 extracted by the ctrlport_if_decoder
    .ENABLE_ROUTER    (0)
  ) tx_dma_ctrl_regs (
    .clk                (bus_clk),
    .reset              (bus_rst),
    .regi_tdata         (dma_tx_regi_tdata),
    .regi_tvalid        (dma_tx_regi_tvalid),
    .regi_tready        (dma_tx_regi_tready),
    .rego_tdata         (dma_tx_rego_tdata),
    .rego_tvalid        (dma_tx_rego_tvalid),
    .rego_tready        (dma_tx_rego_tready),
    .set_enabled        (dma_tx_enabled),
    .set_clear          (dma_tx_clear),
    .set_frame_size     (dma_tx_frame_size),
    .sample_stb         (dma_tx_samp_stb),
    .packet_stb         (dma_tx_pkt_stb),
    .stream_busy        (dma_tx_busy),
    .stream_err         (dma_tx_error),
    .rtr_sid            (8'h00),
    .rtr_dst            ()
  );

  pcie_dma_ctrl #(
    .NUM_STREAMS      (NUM_RX_STREAMS),
    .FRAME_SIZE_W     (DMA_FRAME_SIZE_WIDTH),
    .REG_BASE_ADDR    (20'h00000), // offset 0x400 extracted by the ctrlport_if_decoder
    .ENABLE_ROUTER    (0)
  ) rx_dma_ctrl_regs (
    .clk                (bus_clk),
    .reset              (bus_rst),
    .regi_tdata         (dma_rx_regi_tdata),
    .regi_tvalid        (dma_rx_regi_tvalid),
    .regi_tready        (dma_rx_regi_tready),
    .rego_tdata         (dma_rx_rego_tdata),
    .rego_tvalid        (dma_rx_rego_tvalid),
    .rego_tready        (dma_rx_rego_tready),
    .set_enabled        (dma_rx_enabled),
    .set_clear          (dma_rx_clear),
    .set_frame_size     (dma_rx_frame_size),
    .sample_stb         (dma_rx_samp_stb),
    .packet_stb         (dma_rx_pkt_stb),
    .stream_busy        (dma_rx_busy),
    .stream_err         (dma_rx_error),
    .rtr_sid            (8'h00),
    .rtr_dst            ()
  );
  //
  //*******************************************************************************

  //*******************************************************************************
  // TX DMA Datapath
  //
  wire [NUM_TX_STREAMS-1:0][DMA_STREAM_WIDTH-1:0] dma_tx_tdata_bclk,  dma_tx_tdata_in,
                                                  dma_tx_tdata_trun,  dma_tx_tdata_gt;
  wire [NUM_TX_STREAMS-1:0]                       dma_tx_tvalid_bclk, dma_tx_tvalid_in,
                                                  dma_tx_tvalid_trun, dma_tx_tvalid_gt;
  wire [NUM_TX_STREAMS-1:0]                       dma_tx_tready_bclk, dma_tx_tready_in,
                                                  dma_tx_tready_trun, dma_tx_tready_gt;
  wire [NUM_TX_STREAMS-1:0]                       dma_tx_tlast_trun,  dma_tx_tlast_gt;
  // Output of the axi_mux8
  wire [DMA_STREAM_WIDTH-1:0]                     dma_tx_tdata_mux;
  wire [DMA_RX_DEST_WIDTH-1:0]                    dma_tx_tuser_mux;
  wire                                            dma_tx_tvalid_mux, dma_tx_tlast_mux,
                                                  dma_tx_tready_mux;

  genvar tx_i;
  generate
    for (tx_i=0; tx_i<NUM_TX_STREAMS; tx_i=tx_i+1) begin: gen_tx_dma_path
      axi_fifo_2clk #(
        .WIDTH  (DMA_STREAM_WIDTH),
        .SIZE   (DMA_CLK_XING_FIFO_SIZE)
      ) tx_dma_clock_crossing_fifo (
        .reset        (bus_rst),
        .i_aclk       (dma_clk),
        .i_tdata      (host_dma_tx_tdata[tx_i]),
        .i_tvalid     (host_dma_tx_tvalid[tx_i]),
        .i_tready     (host_dma_tx_tready[tx_i]),
        .o_aclk       (bus_clk),
        .o_tdata      (dma_tx_tdata_bclk[tx_i]),
        .o_tvalid     (dma_tx_tvalid_bclk[tx_i]),
        .o_tready     (dma_tx_tready_bclk[tx_i])
      );

      pcie_lossy_samp_gate #(
        .DATA_WIDTH(DMA_STREAM_WIDTH)
      ) tx_samp_gate (
        .i_tdata      (dma_tx_tdata_bclk[tx_i]),
        .i_tvalid     (dma_tx_tvalid_bclk[tx_i]),
        .i_tready     (dma_tx_tready_bclk[tx_i]),
        .o_tdata      (dma_tx_tdata_in[tx_i]),
        .o_tvalid     (dma_tx_tvalid_in[tx_i]),
        .o_tready     (dma_tx_tready_in[tx_i]),
        .drop         (~dma_tx_enabled[tx_i]),
        .dropping     (dma_tx_busy[tx_i])
      );

      chdr_dechunker #(
        .DATA_WIDTH   (DMA_STREAM_WIDTH),
        .PAD_VALUE    ({DMA_STREAM_WIDTH{1'b1}})
      ) tx_dma_dechunker (
        .clk          (bus_clk),
        .reset        (bus_rst),
        .clear        (dma_tx_clear[tx_i]),
        .frame_size   (dma_tx_frame_size[tx_i]),
        .i_tdata      (dma_tx_tdata_in[tx_i]),
        .i_tvalid     (dma_tx_tvalid_in[tx_i]),
        .i_tready     (dma_tx_tready_in[tx_i]),
        .o_tdata      (dma_tx_tdata_trun[tx_i]),
        .o_tlast      (dma_tx_tlast_trun[tx_i]),
        .o_tvalid     (dma_tx_tvalid_trun[tx_i]),
        .o_tready     (dma_tx_tready_trun[tx_i]),
        .error        (dma_tx_error[tx_i])
      );

      assign dma_tx_samp_stb[tx_i] = dma_tx_tvalid_trun[tx_i] & dma_tx_tready_trun[tx_i];
      assign dma_tx_pkt_stb[tx_i] = dma_tx_samp_stb[tx_i] & dma_tx_tlast_trun[tx_i];

      axi_packet_gate #(
        .WIDTH      (DMA_STREAM_WIDTH),
        .SIZE       (DMA_PKT_GATE_FIFO_SIZE)
      ) vita_chdr_gate (
        .clk        (bus_clk),
        .reset      (bus_rst),
        .clear      (dma_tx_clear[tx_i]),
        .i_tdata    (dma_tx_tdata_trun[tx_i]),
        .i_tlast    (dma_tx_tlast_trun[tx_i]),
        .i_tvalid   (dma_tx_tvalid_trun[tx_i]),
        .i_tready   (dma_tx_tready_trun[tx_i]),
        .i_terror   (dma_tx_error[tx_i]),
        .o_tdata    (dma_tx_tdata_gt[tx_i]),
        .o_tlast    (dma_tx_tlast_gt[tx_i]),
        .o_tvalid   (dma_tx_tvalid_gt[tx_i]),
        .o_tready   (dma_tx_tready_gt[tx_i])
      );
    end
  endgenerate

  // [DrB] The following transport adapter requires a tuser input with the DMA
  // channel. Neither axi_mux8 nor axi_fifo_flop2 support tuser at the time of
  // this modification, and to reduce the risk we will simply widen tdata and
  // include the tuser value in there.
  axi_mux8 #(
    .PRIO     (0),
    .WIDTH    (DMA_STREAM_WIDTH+DMA_RX_DEST_WIDTH)
  ) output_dma_mux (
    .clk      (bus_clk),
    .reset    (bus_rst),                     .clear     (|(dma_tx_clear)),
    .i0_tdata ({DMA_RX_DEST_WIDTH'(0), dma_tx_tdata_gt[0]}),      .i0_tlast   ( dma_tx_tlast_gt[0]),
    .i0_tvalid(                        dma_tx_tvalid_gt[0]),      .i0_tready  (dma_tx_tready_gt[0]),
    .i1_tdata ({DMA_RX_DEST_WIDTH'(1), dma_tx_tdata_gt[1]}),      .i1_tlast   ( dma_tx_tlast_gt[1]),
    .i1_tvalid(                        dma_tx_tvalid_gt[1]),      .i1_tready  (dma_tx_tready_gt[1]),
    .i2_tdata ({DMA_RX_DEST_WIDTH'(2), dma_tx_tdata_gt[2]}),      .i2_tlast   ( dma_tx_tlast_gt[2]),
    .i2_tvalid(                        dma_tx_tvalid_gt[2]),      .i2_tready  (dma_tx_tready_gt[2]),
    .i3_tdata ({DMA_RX_DEST_WIDTH'(3), dma_tx_tdata_gt[3]}),      .i3_tlast   ( dma_tx_tlast_gt[3]),
    .i3_tvalid(                        dma_tx_tvalid_gt[3]),      .i3_tready  (dma_tx_tready_gt[3]),
    .i4_tdata ({DMA_RX_DEST_WIDTH'(4), dma_tx_tdata_gt[4]}),      .i4_tlast   ( dma_tx_tlast_gt[4]),
    .i4_tvalid(                        dma_tx_tvalid_gt[4]),      .i4_tready  (dma_tx_tready_gt[4]),
    // Unused inputs tied off
    .i5_tdata ({DMA_RX_DEST_WIDTH'(5), {DMA_STREAM_WIDTH{1'b0}}}),.i5_tlast(1'b0),
    .i5_tvalid(1'b0),                                             .i5_tready(),
    .i6_tdata ({DMA_RX_DEST_WIDTH'(6), {DMA_STREAM_WIDTH{1'b0}}}),.i6_tlast(1'b0),
    .i6_tvalid(1'b0),                                             .i6_tready(),
    .i7_tdata ({DMA_RX_DEST_WIDTH'(7), {DMA_STREAM_WIDTH{1'b0}}}),.i7_tlast(1'b0),
    .i7_tvalid(1'b0),                                             .i7_tready(),
    .o_tdata    ({dma_tx_tuser_mux, dma_tx_tdata_mux}),
    .o_tlast    (dma_tx_tlast_mux),
    .o_tvalid   (dma_tx_tvalid_mux),
    .o_tready   (dma_tx_tready_mux)
  );

  axi_fifo_flop2 #(
    .WIDTH(DMA_STREAM_WIDTH+1+DMA_RX_DEST_WIDTH)
  ) tx_pipeline_reg (
    .clk(bus_clk), .reset(bus_rst), .clear(|(dma_tx_clear)),
    .i_tdata    ({dma_tx_tuser_mux, dma_tx_tlast_mux, dma_tx_tdata_mux}),
    .i_tvalid   (dma_tx_tvalid_mux),
    .i_tready   (dma_tx_tready_mux),
    .o_tdata    ({dma_tx_tuser, dma_tx_tlast, dma_tx_tdata}),
    .o_tvalid   (dma_tx_tvalid),
    .o_tready   (dma_tx_tready),
    .space      (),
    .occupied   ()
  );
  //
  //*******************************************************************************

  //*******************************************************************************
  // RX DMA Datapath
  //
  wire [NUM_RX_STREAMS-1:0][DMA_STREAM_WIDTH-1:0] dma_rx_tdata_bclk,  dma_rx_tdata_pad,
                                                  dma_rx_tdata_out;
  wire [NUM_RX_STREAMS-1:0]                       dma_rx_tvalid_bclk, dma_rx_tvalid_pad,
                                                  dma_rx_tvalid_out;
  wire [NUM_RX_STREAMS-1:0]                       dma_rx_tready_bclk, dma_rx_tready_pad,
                                                  dma_rx_tready_out;
  wire [NUM_RX_STREAMS-1:0]                       dma_rx_tlast_bclk,  dma_rx_tlast_pad,
                                                  dma_rx_tlast_out;

  wire [DMA_STREAM_WIDTH-1:0]                     dma_rx_tdata_mux;
  wire [DMA_RX_DEST_WIDTH-1:0]                    dma_rx_tuser_mux;
  wire                                            dma_rx_tvalid_mux, dma_rx_tlast_mux,
                                                  dma_rx_tready_mux;

  axi_fifo_flop2 #(
    .WIDTH(DMA_STREAM_WIDTH+1+DMA_RX_DEST_WIDTH)
  ) rx_pipeline_reg (
    .clk        (bus_clk),
    .reset      (bus_rst),
    .clear      (|(dma_rx_clear)),
    .i_tdata    ({dma_rx_tuser, dma_rx_tlast, dma_rx_tdata}),
    .i_tvalid   (dma_rx_tvalid),
    .i_tready   (dma_rx_tready),
    .o_tdata    ({dma_rx_tuser_mux, dma_rx_tlast_mux, dma_rx_tdata_mux}),
    .o_tvalid   (dma_rx_tvalid_mux),
    .o_tready   (dma_rx_tready_mux),
    .space      (),
    .occupied   ()
  );


  localparam [7:0] DMA_RX_ACTIVE_CHAN =
    (NUM_RX_STREAMS >= 8) ? 8'hFF : (8'hFF >> (8-NUM_RX_STREAMS));

  axi_demux8 #(
    .ACTIVE_CHAN(DMA_RX_ACTIVE_CHAN),
    .WIDTH(DMA_STREAM_WIDTH)
  ) input_dma_demux (
    .clk        (bus_clk),
    .reset      (bus_rst),
    .clear      (|(dma_rx_clear)),
    .header     (),
    .dest       (dma_rx_tuser_mux),
    .i_tdata    (dma_rx_tdata_mux),       .i_tlast    (dma_rx_tlast_mux),
    .i_tvalid   (dma_rx_tvalid_mux),      .i_tready   (dma_rx_tready_mux),
    .o0_tdata   (dma_rx_tdata_bclk[0]),   .o0_tlast   (dma_rx_tlast_bclk[0]),
    .o0_tvalid  (dma_rx_tvalid_bclk[0]),  .o0_tready  (dma_rx_tready_bclk[0]),
    .o1_tdata   (dma_rx_tdata_bclk[1]),   .o1_tlast   (dma_rx_tlast_bclk[1]),
    .o1_tvalid  (dma_rx_tvalid_bclk[1]),  .o1_tready  (dma_rx_tready_bclk[1]),
    .o2_tdata   (dma_rx_tdata_bclk[2]),   .o2_tlast   (dma_rx_tlast_bclk[2]), 
    .o2_tvalid  (dma_rx_tvalid_bclk[2]),  .o2_tready  (dma_rx_tready_bclk[2]),
    .o3_tdata   (dma_rx_tdata_bclk[3]),   .o3_tlast   (dma_rx_tlast_bclk[3]), 
    .o3_tvalid  (dma_rx_tvalid_bclk[3]),  .o3_tready  (dma_rx_tready_bclk[3]),
    .o4_tdata   (dma_rx_tdata_bclk[4]),   .o4_tlast   (dma_rx_tlast_bclk[4]), 
    .o4_tvalid  (dma_rx_tvalid_bclk[4]),  .o4_tready  (dma_rx_tready_bclk[4]),
    // Unused outputs tied off
    .o5_tdata(), .o5_tlast(), .o5_tvalid(), .o5_tready(1'b0), //Unused port
    .o6_tdata(), .o6_tlast(), .o6_tvalid(), .o6_tready(1'b0), //Unused port
    .o7_tdata(), .o7_tlast(), .o7_tvalid(), .o7_tready(1'b0)  //Unused port
  );

  genvar rx_i;
  generate
    for (rx_i=0; rx_i<NUM_RX_STREAMS; rx_i=rx_i+1) begin: gen_rx_dma_path

      assign dma_rx_samp_stb[rx_i] = dma_rx_tvalid_bclk[rx_i] & dma_rx_tready_bclk[rx_i];
      assign dma_rx_pkt_stb[rx_i]  = dma_rx_samp_stb[rx_i]    & dma_rx_tlast_bclk[rx_i];

      chdr_chunker #(
        .DATA_WIDTH   (DMA_STREAM_WIDTH),
        .PAD_VALUE    ({DMA_STREAM_WIDTH{1'b1}})
      ) rx_dma_chunker (
        .clk          (bus_clk),
        .reset        (bus_rst),
        .clear        (dma_rx_clear[rx_i]),
        .frame_size   (dma_rx_frame_size[rx_i]),
        .i_tdata      (dma_rx_tdata_bclk[rx_i]),
        .i_tlast      (dma_rx_tlast_bclk[rx_i]),
        .i_tvalid     (dma_rx_tvalid_bclk[rx_i]),
        .i_tready     (dma_rx_tready_bclk[rx_i]),
        .o_tdata      (dma_rx_tdata_pad[rx_i]),
        .o_tlast      (dma_rx_tlast_pad[rx_i]),
        .o_tvalid     (dma_rx_tvalid_pad[rx_i]),
        .o_tready     (dma_rx_tready_pad[rx_i]),
        .error        (dma_rx_error[rx_i])
      );

      pcie_lossy_samp_gate #(
        .DATA_WIDTH(DMA_STREAM_WIDTH)
      ) rx_samp_gate (
        .i_tdata      (dma_rx_tdata_pad[rx_i]),
        .i_tvalid     (dma_rx_tvalid_pad[rx_i]),
        .i_tready     (dma_rx_tready_pad[rx_i]),
        .o_tdata      (dma_rx_tdata_out[rx_i]),
        .o_tvalid     (dma_rx_tvalid_out[rx_i]),
        .o_tready     (dma_rx_tready_out[rx_i]),
        .drop         (~dma_rx_enabled[rx_i]),
        .dropping     (dma_rx_busy[rx_i])
      );

      axi_fifo_2clk #(
        .WIDTH      (DMA_STREAM_WIDTH),
        .SIZE       (DMA_CLK_XING_FIFO_SIZE)
      ) rx_dma_clock_crossing_fifo (
        .reset       (bus_rst),
        .i_aclk      (bus_clk),
        .i_tdata     (dma_rx_tdata_out[rx_i]),
        .i_tvalid    (dma_rx_tvalid_out[rx_i]),
        .i_tready    (dma_rx_tready_out[rx_i]),
        .o_aclk      (dma_clk),
        .o_tdata     (host_dma_rx_tdata[rx_i]),
        .o_tvalid    (host_dma_rx_tvalid[rx_i]),
        .o_tready    (host_dma_rx_tready[rx_i])
      );
    end
  endgenerate
  //
  //*******************************************************************************
endmodule : b310_pcie_int
