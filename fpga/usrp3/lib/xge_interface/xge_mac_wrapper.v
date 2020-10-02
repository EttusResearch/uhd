/////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: xge_mac_wrapper
// Description:
//   Wrap XGE MAC + optional wishbone interface
//
//   *) Signals are crossed between the MAC's own 156.25MHz clock domain and the
//   main FPGA clock domain.
//   *) 6 byte Padding is added at RX, including metadata so that IP headers become aligned.
//   *) 6 Byte padding is stripped at TX, so that Eth header data starts immediately.
//   *) TX & RX can buffer at least an MTU sized packet
//   *) On TX, to not start an Ethernet Tx until a complete packet is present in the
//   last Tx FIFO so that the MAC doesn't underrun.
//
// Parameters:
//   - PORTNUM: Refers to which ethernet port is being built. Added to padding but not used
//   - WISHBONE: If set use wishbone implementation
//   - ADD_PREAMBLE: If set add/remove 6 byte padding used in old ethernet_interface
//   - CROSS_TO_SYSCLK: If set cross AXI streams to the sys_clk domain.
//   - CUT_THROUGH: If > 0, how many words to wait before starting to transmit.
/////////////////////////////////////////////////////////////////////

module xge_mac_wrapper #(
  parameter PORTNUM = 8'd0,
  parameter WISHBONE = 1,
  parameter ADD_PREAMBLE = 1,
  parameter CROSS_TO_SYSCLK = 1,
  parameter CUT_THROUGH = 0
)(
  // XGMII
  input         xgmii_clk,
  output [63:0] xgmii_txd,
  output [7:0]  xgmii_txc,
  input  [63:0] xgmii_rxd,
  input  [7:0]  xgmii_rxc,
  // Client FIFO Interfaces
  input         sys_clk,
  input         sys_rst,          // From sys_clk domain.
  output [63:0] rx_tdata,
  output [3:0]  rx_tuser,
  output        rx_tlast,
  output        rx_tvalid,
  input         rx_tready,
  input  [63:0] tx_tdata,
  input  [3:0]  tx_tuser,                // Bit[3] (error) is ignored for now.
  input         tx_tlast,
  input         tx_tvalid,
  output        tx_tready,
  // Control and Status
  input         phy_ready,
  input         ctrl_tx_enable,
  output        status_crc_error,
  output        status_fragment_error,
  output        status_txdfifo_ovflow,
  output        status_txdfifo_udflow,
  output        status_rxdfifo_ovflow,
  output        status_rxdfifo_udflow,
  output        status_pause_frame_rx,
  output        status_local_fault,
  output        status_remote_fault,
  // MDIO
  output        mdc,
  output        mdio_in,
  input         mdio_out,
  // Wishbone interface
  input [7:0]   wb_adr_i,               // To wishbone_if0 of wishbone_if.v
  input         wb_clk_i,               // To sync_clk_wb0 of sync_clk_wb.v, ...
  input         wb_cyc_i,               // To wishbone_if0 of wishbone_if.v
  input [31:0]  wb_dat_i,               // To wishbone_if0 of wishbone_if.v
  input         wb_rst_i,               // To sync_clk_wb0 of sync_clk_wb.v, ...
  input         wb_stb_i,               // To wishbone_if0 of wishbone_if.v
  input         wb_we_i,                // To wishbone_if0 of wishbone_if.v
  output        wb_ack_o,               // From wishbone_if0 of wishbone_if.v
  output [31:0] wb_dat_o,               // From wishbone_if0 of wishbone_if.v
  output        wb_int_o                // From wishbone_if0 of wishbone_if.v
);

  //
  // Generate 156MHz synchronized sys_rst locally
  //

  wire xgmii_reset, ctrl_tx_enable_xclk;
  wire phy_ready_xgmiiclk, sys_rst_xgmiiclk;

  synchronizer #(
     .INITIAL_VAL(1'b0), .STAGES(3)
  ) phy_ready_sync_i (
     .clk(xgmii_clk), .rst(1'b0 /* no reset */), .in(phy_ready), .out(phy_ready_xgmiiclk)
  );

  if (CROSS_TO_SYSCLK) begin : reset_cross
    synchronizer #(
       .INITIAL_VAL(1'b1), .STAGES(3)
    ) sys_rst_sync_i (
       .clk(xgmii_clk), .rst(1'b0 /* no reset */), .in(sys_rst), .out(sys_rst_xgmiiclk)
    );
    assign xgmii_reset = !phy_ready_xgmiiclk || sys_rst_xgmiiclk;
  end else begin : reset_no_cross
    assign xgmii_reset = !phy_ready_xgmiiclk;
  end
  synchronizer #(
     .INITIAL_VAL(1'b1), .STAGES(3)
  ) tx_enabled_sync_i (
     .clk(xgmii_clk), .rst(1'b0 /* no reset */), .in(ctrl_tx_enable), .out(ctrl_tx_enable_xclk)
  );

  //
  // 10G MAC
  //
  wire [63:0] eth_rx_data;
  wire        eth_rx_avail;
  wire        eth_rx_eof;
  wire        eth_rx_err;
  wire [2:0]  eth_rx_occ;
  wire        eth_rx_sof;
  wire        eth_rx_valid;
  wire        eth_rx_ren;

  wire        eth_tx_full;
  wire [63:0] eth_tx_data;
  wire        eth_tx_eof;
  wire [2:0]  eth_tx_occ;
  wire        eth_tx_sof;
  wire        eth_tx_valid;

  generate if (WISHBONE == 1) begin : wishbone_mac
    xge_mac_wb xge_mac_wb (
      // Clocks and Resets
      .clk_156m25             (xgmii_clk),
      .clk_xgmii_rx           (xgmii_clk),
      .clk_xgmii_tx           (xgmii_clk),
      .reset_156m25_n         (~xgmii_reset),
      .reset_xgmii_rx_n       (~xgmii_reset),
      .reset_xgmii_tx_n       (~xgmii_reset),
      // XGMII
      .xgmii_txc              (xgmii_txc[7:0]),
      .xgmii_txd              (xgmii_txd[63:0]),
      .xgmii_rxc              (xgmii_rxc[7:0]),
      .xgmii_rxd              (xgmii_rxd[63:0]),
      // MDIO
      .mdc                    (mdc),
      .mdio_out               (mdio_in),// Switch sense of in and out here for master and slave.
      .mdio_tri               (mdio_tri),
      .xge_gpo                (),
      .mdio_in                (mdio_out), // Switch sense of in and out here for master and slave.
      .xge_gpi                (/*{2'b00,align_status,mgt_tx_ready,sync_status[3:0]}*/0),
      // Packet interface
      .pkt_rx_avail           (eth_rx_avail),
      .pkt_rx_data            (eth_rx_data),
      .pkt_rx_eop             (eth_rx_eof),
      .pkt_rx_err             (eth_rx_err),
      .pkt_rx_mod             (eth_rx_occ),
      .pkt_rx_sop             (eth_rx_sof),
      .pkt_rx_val             (eth_rx_valid),
      .pkt_tx_full            (eth_tx_full),
      // Inputs
      .pkt_rx_ren             (eth_rx_ren),
      .pkt_tx_data            (eth_tx_data),
      .pkt_tx_eop             (eth_tx_eof),
      .pkt_tx_mod             (eth_tx_occ),
      .pkt_tx_sop             (eth_tx_sof),
      .pkt_tx_val             (eth_tx_valid),
      .wb_ack_o               (wb_ack_o),
      .wb_dat_o               (wb_dat_o),
      .wb_adr_i               (wb_adr_i[7:0]),
      .wb_clk_i               (wb_clk_i),
      .wb_cyc_i               (wb_cyc_i),
      .wb_dat_i               (wb_dat_i),
      .wb_rst_i               (wb_rst_i),
      .wb_stb_i               (wb_stb_i),
      .wb_we_i                (wb_we_i),
      .wb_int_o               (xge_int)
    );

    assign status_crc_error = 1'b0;
    assign status_fragment_error = 1'b0;
    assign status_txdfifo_ovflow = 1'b0;
    assign status_txdfifo_udflow = 1'b0;
    assign status_rxdfifo_ovflow = 1'b0;
    assign status_rxdfifo_udflow = 1'b0;
    assign status_pause_frame_rx = 1'b0;
    assign status_local_fault = 1'b0;
    assign status_remote_fault = 1'b0;

  end else begin : xge_mac
    xge_mac xge_mac (
      // Clocks and Resets
      .clk_156m25             (xgmii_clk),
      .clk_xgmii_rx           (xgmii_clk),
      .clk_xgmii_tx           (xgmii_clk),
      .reset_156m25_n         (~xgmii_reset),
      .reset_xgmii_rx_n       (~xgmii_reset),
      .reset_xgmii_tx_n       (~xgmii_reset),
      // XGMII
      .xgmii_txc              (xgmii_txc[7:0]),
      .xgmii_txd              (xgmii_txd[63:0]),
      .xgmii_rxc              (xgmii_rxc[7:0]),
      .xgmii_rxd              (xgmii_rxd[63:0]),
      // Packet interface
      .pkt_rx_avail           (eth_rx_avail),
      .pkt_rx_data            (eth_rx_data),
      .pkt_rx_eop             (eth_rx_eof),
      .pkt_rx_err             (eth_rx_err),
      .pkt_rx_mod             (eth_rx_occ),
      .pkt_rx_sop             (eth_rx_sof),
      .pkt_rx_val             (eth_rx_valid),
      .pkt_tx_full            (eth_tx_full),
      // Inputs
      .pkt_rx_ren             (eth_rx_ren),
      .pkt_tx_data            (eth_tx_data),
      .pkt_tx_eop             (eth_tx_eof),
      .pkt_tx_mod             (eth_tx_occ),
      .pkt_tx_sop             (eth_tx_sof),
      .pkt_tx_val             (eth_tx_valid),
      // Control and Status
      .ctrl_tx_enable         (ctrl_tx_enable_xclk),
      .status_crc_error       (status_crc_error),
      .status_fragment_error  (status_fragment_error),
      .status_txdfifo_ovflow  (status_txdfifo_ovflow),
      .status_txdfifo_udflow  (status_txdfifo_udflow),
      .status_rxdfifo_ovflow  (status_rxdfifo_ovflow),
      .status_rxdfifo_udflow  (status_rxdfifo_udflow),
      .status_pause_frame_rx  (status_pause_frame_rx),
      .status_local_fault     (status_local_fault),
      .status_remote_fault    (status_remote_fault)
    );

    assign wb_ack_o = 1'b0;
    assign wb_dat_o = 1'b0;
    assign wb_int_o = 1'b0;
    assign mdio_in = 1'b0;
    assign mdc = 1'b0;
  end
  endgenerate

  ///////////////////////////////////////////////////////////////////////////////////////
  // RX FIFO Chain
  ///////////////////////////////////////////////////////////////////////////////////////
  wire [63:0] rx_tdata_int;
  wire [3:0]  rx_tuser_int;
  wire        rx_tlast_int;
  wire        rx_tvalid_int;
  wire        rx_tready_int;

  //
  // Logic to drive pkt_rx_ren on XGE MAC
  //
  xge_handshake xge_handshake (
    .clk(xgmii_clk),
    .reset(xgmii_reset),
    .pkt_rx_ren(eth_rx_ren),
    .pkt_rx_avail(eth_rx_avail),
    .pkt_rx_eop(eth_rx_eof)
  );


  if (ADD_PREAMBLE) begin : rx_preamble
    //
    // Add pad of 6 empty bytes before MAC addresses of new Rxed packet so that IP
    // headers are aligned. Also put metadata in first octet of pad that shows
    // ingress port.
    //
    xge64_to_axi64 #(
      .LABEL(PORTNUM)
    ) xge64_to_axi64 (
      .clk(xgmii_clk),
      .reset(xgmii_reset),
      .clear(1'b0),
      .datain(eth_rx_data),
      .occ(eth_rx_occ),
      .sof(eth_rx_sof),
      .eof(eth_rx_eof),
      .err(eth_rx_err),
      .valid(eth_rx_valid),
      .axis_tdata(rx_tdata_int),
      .axis_tuser(rx_tuser_int),
      .axis_tlast(rx_tlast_int),
      .axis_tvalid(rx_tvalid_int),
      .axis_tready(rx_tready_int)
    );
  end else begin : rx_no_preamble
    assign rx_tdata_int      = eth_rx_data;
    assign rx_tuser_int[3]   = eth_rx_err;
    assign rx_tuser_int[2:0] = eth_rx_occ;
    assign rx_tlast_int      = eth_rx_eof;
    assign rx_tvalid_int     = eth_rx_valid;
    // there is no holdoff so ignore rx_tready_int
  end


  if (CROSS_TO_SYSCLK) begin : rx_cross
    //
    // Large FIFO must be able to run input side at 64b@156MHz to sustain 10Gb Rx.
    //
    axi64_4k_2clk_fifo rxfifo_2clk (
      .s_aresetn(~xgmii_reset),
      .s_aclk(xgmii_clk),
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
  end else begin : rx_no_cross
    assign rx_tdata       = rx_tdata_int;
    assign rx_tuser       = rx_tuser_int;
    assign rx_tlast       = rx_tlast_int;
    assign rx_tvalid      = rx_tvalid_int;
    assign rx_tready_int  = rx_tready;
  end

  ///////////////////////////////////////////////////////////////////////////////////////
  // TX FIFO Chain
  ///////////////////////////////////////////////////////////////////////////////////////

  wire [63:0] tx_tdata_int;
  wire [3:0]  tx_tuser_int;
  wire        tx_tlast_int;
  wire        tx_tvalid_int;
  wire        tx_tready_int;

  wire [63:0] tx_tdata_int2;
  wire [3:0]  tx_tuser_int2;
  wire        tx_tlast_int2;
  wire        tx_tvalid_int2;
  wire        tx_tready_int2;

  wire        tx_tvalid_int3;
  wire        tx_tready_int3;
  wire        tx_sof_int3;
  wire        enable_tx;

  if (CROSS_TO_SYSCLK) begin : tx_cross
    axi64_4k_2clk_fifo txfifo_2clk_1x (
      .s_aresetn(~xgmii_reset),
      .s_aclk(sys_clk),
      .s_axis_tvalid(tx_tvalid),
      .s_axis_tready(tx_tready),
      .s_axis_tdata(tx_tdata),
      .s_axis_tlast(tx_tlast),
      .s_axis_tuser(tx_tuser),
      .axis_wr_data_count(),

      .m_aclk(xgmii_clk),
      .m_axis_tvalid(tx_tvalid_int),
      .m_axis_tready(tx_tready_int),
      .m_axis_tdata(tx_tdata_int),
      .m_axis_tlast(tx_tlast_int),
      .m_axis_tuser(tx_tuser_int),
      .axis_rd_data_count()
    );
  end else begin : tx_no_cross
    assign tx_tdata_int  = tx_tdata;
    assign tx_tuser_int  = tx_tuser;
    assign tx_tlast_int  = tx_tlast;
    assign tx_tvalid_int = tx_tvalid;
    assign tx_tready     = tx_tready_int;
  end


  if (ADD_PREAMBLE) begin : tx_preamble
    //
    // Strip the 6 octet ethernet padding we used internally.
    // Put SOF into bit[3] of tuser.
    //
    axi64_to_xge64 axi64_to_xge64 (
      .clk(xgmii_clk),
      .reset(xgmii_reset),
      .clear(1'b0),
      .s_axis_tdata(tx_tdata_int),
      .s_axis_tuser(tx_tuser_int),
      .s_axis_tlast(tx_tlast_int),
      .s_axis_tvalid(tx_tvalid_int),
      .s_axis_tready(tx_tready_int),
      .m_axis_tdata(tx_tdata_int2),
      .m_axis_tuser(tx_tuser_int2),
      .m_axis_tlast(tx_tlast_int2),
      .m_axis_tvalid(tx_tvalid_int2),
      .m_axis_tready(tx_tready_int2)
    );
  end else begin : tx_no_preamble
    reg sof = 1'b1;

    // Add SOF
    always @(posedge xgmii_clk) begin : add_sof
      if (xgmii_reset) begin
        sof <= 1'b1;
      end else if (tx_tvalid_int && tx_tready_int) begin
        sof <= tx_tlast_int;
      end
    end

    assign tx_tdata_int2      = tx_tdata_int;
    assign tx_tuser_int2[3]   = sof && tx_tvalid_int;
    assign tx_tuser_int2[2:0] = tx_tuser_int[2:0];
    assign tx_tlast_int2      = tx_tlast_int;
    assign tx_tvalid_int2     = tx_tvalid_int;
    assign tx_tready_int      = tx_tready_int2;
  end

  //
  // Large FIFO can hold a max sized ethernet packet.
  //
  wire [15:0] tx_occupied;

  localparam TX_FIFO_SIZE = CUT_THROUGH > 0 ? $clog2(CUT_THROUGH)+1 : 10;

  axi_fifo #(.WIDTH(64+4+1), .SIZE(TX_FIFO_SIZE)) txfifo (
    .clk(xgmii_clk), .reset(xgmii_reset), .clear(1'b0),
    .i_tdata({tx_tlast_int2, tx_tuser_int2, tx_tdata_int2}),
    .i_tvalid(tx_tvalid_int2),
    .i_tready(tx_tready_int2),
    .o_tvalid(tx_tvalid_int3),
    .o_tready(tx_tready_int3),
    .o_tdata({eth_tx_eof,tx_sof_int3,eth_tx_occ,eth_tx_data}),
    .space(), .occupied(tx_occupied)
  );

  //
  // add cut through if we have "enough" data buffered up
  //
  reg cut_through;
  reg cut_wait;
  reg [15:0] cut_idlecount;

  if (CUT_THROUGH > 0) begin : yes_cut_through

    wire cut_start;
    wire cut_end;

    // start under 2 conditions
    //    (1) we have more the CUT_THROUGH bytes buffered
    assign cut_start = tx_occupied > CUT_THROUGH || 
    //    (2) we have kept bytes waiting for too long
    //   The second case happens when less than CUT_THROUGH bytes are pushed
                       (cut_idlecount == 0);
    assign cut_end   = eth_tx_eof && eth_tx_valid;

    // Add SOF
    always @(posedge xgmii_clk) begin : cut_through_dff
      if (xgmii_reset) begin
        cut_through   <= 1'b0;
        cut_wait      <= 1'b0;
        cut_idlecount <= CUT_THROUGH+2;
      end else begin
        cut_wait    <= eth_tx_full;
        if (cut_start) begin
          cut_through <= 1'b1;
        end else if (cut_end) begin
          cut_through <= 1'b0;
        end
        if (tx_occupied > 0 && cut_through == 1'b0) begin
          if (cut_idlecount > 0) begin
            cut_idlecount <= cut_idlecount-1;
          end
        end else begin
          cut_idlecount <= CUT_THROUGH+2;
        end
      end
    end
  end else begin : no_cut_through
    always @(*) begin
      cut_through <= 0;
      cut_wait    <= 0;
    end
  end

  //
  // Monitor number of Ethernet packets in tx_fifo2
  //
  axi_count_packets_in_fifo axi_count_packets_in_fifo (
    .clk(xgmii_clk),
    .reset(xgmii_reset),
    .in_axis_tvalid(tx_tvalid_int2),
    .in_axis_tready(tx_tready_int2),
    .in_axis_tlast(tx_tlast_int2),
    .out_axis_tvalid(tx_tvalid_int3),
    .out_axis_tready(tx_tready_int3),
    .out_axis_tlast(eth_tx_eof),
    .pkt_tx_full(eth_tx_full),
    .enable_tx(enable_tx)
  );

  //
  //
  // Suppress FIFO flags to stop overflow of MAC in Tx direction
  //
  if (CUT_THROUGH > 0) begin : yes_cut_through_ready
    assign tx_tready_int3   = (cut_through && !(cut_wait));
    assign eth_tx_valid     = (cut_through && !(cut_wait)) & tx_tvalid_int3;
    assign eth_tx_sof       = (cut_through && !(cut_wait)) & tx_sof_int3;
  end else begin : no_cut_through_ready
    assign tx_tready_int3   = enable_tx;
    assign eth_tx_valid     = enable_tx & tx_tvalid_int3;
    assign eth_tx_sof       = enable_tx & tx_sof_int3;
  end

endmodule
