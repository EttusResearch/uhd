//
// Copyright 2013 Ettus Research LLC
//


//
// Wrap XGE MAC so that:
//
// *) Signals are crossed between the MAC's own 156.25MHz clock domain and the
// main FPGA clock domain.
// *) 6 byte Padding is added at RX, including metadata so that IP headers become aligned.
// *) 6 Byte padding is stripped at TX, so that Eth header data starts immediately.
// *) TX & RX can buffer at least an MTU sized packet
// *) On TX, to not start an Ethernet Tx until a complete packet is present in the
// last Tx FIFO so that the MAC doesn't underrun.
//

module xge_mac_wrapper
  #(parameter PORTNUM=8'd0)

   (
    // XGMII
    input xgmii_clk,
    output [63:0] xgmii_txd,
    output [7:0]  xgmii_txc,
    input [63:0] xgmii_rxd,
    input [7:0]  xgmii_rxc,
    // MDIO
    output mdc,
    output mdio_in,
    input mdio_out,
    // Wishbone I/F
    input [7:0]   wb_adr_i,               // To wishbone_if0 of wishbone_if.v
    input         wb_clk_i,               // To sync_clk_wb0 of sync_clk_wb.v, ...
    input         wb_cyc_i,               // To wishbone_if0 of wishbone_if.v
    input [31:0]  wb_dat_i,               // To wishbone_if0 of wishbone_if.v
    input         wb_rst_i,               // To sync_clk_wb0 of sync_clk_wb.v, ...
    input         wb_stb_i,               // To wishbone_if0 of wishbone_if.v
    input         wb_we_i,                // To wishbone_if0 of wishbone_if.v
    output        wb_ack_o,               // From wishbone_if0 of wishbone_if.v
    output [31:0] wb_dat_o,               // From wishbone_if0 of wishbone_if.v
    output        wb_int_o,               // From wishbone_if0 of wishbone_if.v
    // Client FIFO Interfaces
    input sys_clk,
    input reset,          // From sys_clk domain.
    output [63:0] rx_tdata,
    output [3:0] rx_tuser,
    output rx_tlast,
    output rx_tvalid,
    input rx_tready,
    input [63:0] tx_tdata,
    input [3:0] tx_tuser,                // Bit[3] (error) is ignored for now.
    input tx_tlast,
    input tx_tvalid,
    output tx_tready,
    // Other
    input phy_ready,
    // Debug
    output [31:0] debug_rx,
    output [31:0] debug_tx);



   (* ASYNC_REG = "TRUE" *)
   reg           xgmii_reset_r1;
   reg 		   xgmii_reset;

   //
   // Generate 156MHz synchronized reset localy
   //
   always @(posedge xgmii_clk or posedge reset)
     begin
	if (reset) begin
	   xgmii_reset_r1 <= 1'b1;
	   xgmii_reset    <= 1'b1;
	end
	else begin
	   xgmii_reset_r1 <= 1'b0; // IJB. Was PLL lock here.
	   xgmii_reset    <= xgmii_reset_r1;
	end
     end // always @ (posedge xgmii_clk or posedge reset)

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

   xge_mac xge_mac
     (
      // Outputs
      .pkt_rx_avail               (eth_rx_avail),
      .pkt_rx_data                (eth_rx_data),
      .pkt_rx_eop                 (eth_rx_eof),
      .pkt_rx_err                 (eth_rx_err),
      .pkt_rx_mod                 (eth_rx_occ),
      .pkt_rx_sop                 (eth_rx_sof),
      .pkt_rx_val                 (eth_rx_valid),
      .pkt_tx_full                (eth_tx_full),
      .wb_ack_o                   (wb_ack_o),
      .wb_dat_o                   (wb_dat_o),
      .wb_int_o                   (xge_int),
      .xgmii_txc                  (xgmii_txc[7:0]),
      .xgmii_txd                  (xgmii_txd[63:0]),
      .mdc                        (mdc),
      .mdio_out                   (mdio_in),// Switch sense of in and out here for master and slave.
      .mdio_tri                   (mdio_tri),
      .xge_gpo                    ( ),
      // Inputs
      .clk_156m25                 (xgmii_clk),
      .clk_xgmii_rx               (xgmii_clk),
      .clk_xgmii_tx               (xgmii_clk),
      .pkt_rx_ren                 (eth_rx_ren),
      .pkt_tx_data                (eth_tx_data),
      .pkt_tx_eop                 (eth_tx_eof),
      .pkt_tx_mod                 (eth_tx_occ),
      .pkt_tx_sop                 (eth_tx_sof),
      .pkt_tx_val                 (eth_tx_valid),
      .reset_156m25_n             (~xgmii_reset),
      .reset_xgmii_rx_n           (~xgmii_reset),
      .reset_xgmii_tx_n           (~xgmii_reset),
      .wb_adr_i                   (wb_adr_i[7:0]),
      .wb_clk_i                   (wb_clk_i),
      .wb_cyc_i                   (wb_cyc_i),
      .wb_dat_i                   (wb_dat_i),
      .wb_rst_i                   (wb_rst_i),
      .wb_stb_i                   (wb_stb_i),
      .wb_we_i                    (wb_we_i),
      .xgmii_rxc                  (xgmii_rxc[7:0]),
      .xgmii_rxd                  (xgmii_rxd[63:0]),
      .mdio_in                    (mdio_out), // Switch sense of in and out here for master and slave.
      .xge_gpi                    (/*{2'b00,align_status,mgt_tx_ready,sync_status[3:0]}*/0)
      );


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
   xge_handshake xge_handshake
     (
      .clk(xgmii_clk),
      .reset(xgmii_reset),
      .pkt_rx_ren(eth_rx_ren),
      .pkt_rx_avail(eth_rx_avail),
      .pkt_rx_eop(eth_rx_eof)
      );

   //
   // Add pad of 6 empty bytes before MAC addresses of new Rxed packet so that IP
   // headers are alligned. Also put metadata in first octet of pad that shows
   // ingress port.
   //
   xge64_to_axi64  #(.LABEL(PORTNUM)) xge64_to_axi64
     (
      .clk(xgmii_clk),
      .reset(xgmii_reset),
      .clear(clear),
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

   //
   // Large FIFO must be able to run input side at 64b@156MHz to sustain 10Gb Rx.
   //

   axi64_4k_2clk_fifo rxfifo_2clk
     (
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
      .axis_rd_data_count() );


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


   axi64_4k_2clk_fifo txfifo_2clk_1x
     (
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
      .axis_rd_data_count() );

   //
   // Strip the 6 octet ethernet padding we used internally.
   // Put SOF into bit[3] of tuser.
   //
   axi64_to_xge64 axi64_to_xge64
     (
      .clk(xgmii_clk),
      .reset(xgmii_reset),
      .clear(clear),
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

   //
   // Large FIFO can hold a max sized ethernet packet.
   //
   axi64_8k_2clk_fifo txfifo_2clk_2
     (
      .s_aresetn(~xgmii_reset),
      .s_aclk(xgmii_clk),
      .s_axis_tvalid(tx_tvalid_int2),
      .s_axis_tready(tx_tready_int2),
      .s_axis_tdata(tx_tdata_int2),
      .s_axis_tlast(tx_tlast_int2),
      .s_axis_tuser(tx_tuser_int2),
      .axis_wr_data_count(),

      .m_aclk(xgmii_clk),
      .m_axis_tvalid(tx_tvalid_int3),
      .m_axis_tready(tx_tready_int3),
      .m_axis_tdata(eth_tx_data),
      .m_axis_tlast(eth_tx_eof),
      .m_axis_tuser({tx_sof_int3,eth_tx_occ}),
      .axis_rd_data_count() );

   //
   // Monitor number of Ethernet packets in tx_fifo2
   //
   axi_count_packets_in_fifo axi_count_packets_in_fifo
     (
      .clk(xgmii_clk),
      .reset(xgmii_reset),
      .in_axis_tvalid(tx_tvalid_int2),
      .in_axis_tready(tx_tready_int2),
      .in_axis_tlast(tx_tlast_int2),
      .out_axis_tvalid(tx_tvalid_int3),
      .out_axis_tready(tx_tready_int3),
      .out_axis_tlast(eth_tx_eof),
      .pkt_tx_full(eth_tx_full),
      .enable_tx(enable_tx) );

   //
   //
   // Supress FIFO flags to stop overflow of MAC in Tx direction
   //
   assign      eth_tx_valid = tx_tvalid_int3 & enable_tx;
   assign      tx_tready_int3 = enable_tx;
   assign      eth_tx_sof = tx_sof_int3 & enable_tx;


endmodule
