//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_qsfp_wrapper
//
// Description:
//
//   Consolidates the logic necessary for a QSFP port, depending on the
//   requested protocol.
//
// Parameters:
//
//   PROTOCOL       : Indicates the protocol to use for each of the 4 QSFP
//                    lanes. See x4xx_mgt_types.vh for possible values.
//   CPU_W          : Width of CPU interface
//   CHDR_W         : CHDR bus width
//   BYTE_MTU       : Transport MTU in bytes
//   PORTNUM        : Port number to distinguish multiple QSFP ports
//   NODE_INST      : RFNoC transport adapter node instance for the first port
//   RFNOC_PROTOVER : RFNoC protocol version for IPv4 interface
//

`include "./x4xx_mgt_types.vh"


module x4xx_qsfp_wrapper #(
  // Must be a value defined in x4xx_mgt_types.vh
  parameter integer PROTOCOL [3:0] = {`MGT_Disabled,
                                      `MGT_Disabled,
                                      `MGT_Disabled,
                                      `MGT_Disabled},
  parameter         CPU_W          = 64,
  parameter         CHDR_W         = 64,
  parameter         BYTE_MTU       = $clog2(8*1024),
  parameter  [ 7:0] PORTNUM        = 8'd0,
  parameter         NODE_INST      = 0,
  parameter  [15:0] RFNOC_PROTOVER = {8'd1, 8'd0}
)(
  // Resets
  input logic areset,
  input logic bus_rst,
  input logic clk40_rst,

  // Clocks
  input logic refclk_p,
  input logic refclk_n,
  input logic clk100,
  input logic bus_clk,

  // AXI-Lite register access
  AxiLiteIf.slave s_axi,

  // Ethernet DMA AXI to PS memory
  AxiIf.master axi_hp,

  // MGT high-speed IO
  output logic [3:0] tx_p,
  output logic [3:0] tx_n,
  input  logic [3:0] rx_p,
  input  logic [3:0] rx_n,

  // CHDR router interface
  AxiStreamIf.master e2v [4],
  AxiStreamIf.slave  v2e [4],

  // ETH DMA IRQs
  output logic [3:0] eth_rx_irq,
  output logic [3:0] eth_tx_irq,

  // Misc.
  output logic        rx_rec_clk_out,
  input  logic [15:0] device_id,

  output logic [3:0][31:0] port_info,

  output logic [3:0] link_up,
  output logic [3:0] activity
);

  import PkgAxiLite::*;

  localparam REG_BASE_SFP_IO      = 14'h0;
  localparam REG_BASE_ETH_SWITCH  = 14'h1000;
  localparam CPU_USER_W           = $clog2(CPU_W/8)+1;
  localparam CHDR_USER_W          = $clog2(CHDR_W/8);
  localparam REG_DWIDTH           = 32;
  localparam REG_AWIDTH_MISC      = 14;
  localparam logic [3:0] DISABLED = { PROTOCOL[3] == `MGT_Disabled,
                                      PROTOCOL[2] == `MGT_Disabled,
                                      PROTOCOL[1] == `MGT_Disabled,
                                      PROTOCOL[0] == `MGT_Disabled };
  localparam logic [3:0] IS10GBE  = { PROTOCOL[3] == `MGT_10GbE,
                                      PROTOCOL[2] == `MGT_10GbE,
                                      PROTOCOL[1] == `MGT_10GbE,
                                      PROTOCOL[0] == `MGT_10GbE };
  localparam logic [3:0] IS100GBE = { 3'b0,PROTOCOL[0] == `MGT_100GbE };
  localparam logic [3:0] ISAURORA = { 3'b0,PROTOCOL[0] == `MGT_Aurora };


  `include "../../lib/axi4_sv/axi.vh"
  `include "../../lib/axi4lite_sv/axi_lite.vh"


  //---------------------------------------------------------------------------
  // Interfaces
  //---------------------------------------------------------------------------

  // AXI-Lite interface
  AxiLiteIf #(REG_DWIDTH,40)
    m_axi_dma[3:0] (s_axi.clk, s_axi.rst);

  // 0x0000-0x3FFF - Bottom goes to XGE top goes to UIO
  AxiLiteIf #(REG_DWIDTH,40)
    m_axi_misc[3:0] (s_axi.clk, s_axi.rst);
  AxiLiteIf_v #(REG_DWIDTH,REG_AWIDTH_MISC)
    m_axi_misc_v[3:0] (s_axi.clk, s_axi.rst);

  // 0x4000-0x5FFF - Goes to 100G Mac
  AxiLiteIf #(REG_DWIDTH,40)
    m_axi_mac[3:0] (s_axi.clk, s_axi.rst);

  // AXI (Full) for DMA back to CPU memory
  AxiIf #(128,49)
    axi_hp_dma[3:0] (s_axi.clk, s_axi.rst);


  //---------------------------------------------------------------------------
  // AXI Interconnect
  //---------------------------------------------------------------------------
  //
  // Break the incoming register request into 12 different spaces:
  //
  //   0x0_0000 - dma0
  //   0x0_8000 - misc0 - +0x0000 NIXGE
  //                      +0x2000 UIO
  //   0x0_C000 - mac0
  //
  //   0x1_0000 - dma1
  //   0x1_8000 - misc1 - +0x0000 NIXGE
  //                      +0x2000 UIO
  //   0x1_C000 - mac1
  //
  //   0x2_0000 - dma2
  //   0x2_8000 - misc2 - +0x0000 NIXGE
  //                      +0x2000 UIO
  //   0x2_C000 - mac2
  //
  //   0x3_0000 - dma3
  //   0x3_8000 - misc3 - +0x0000 NIXGE
  //                      +0x2000 UIO
  //  0x3_C000 - mac3
  //
  //---------------------------------------------------------------------------

  axi_interconnect_eth axi_interconnect_eth_i (
    .s_axi_eth  (s_axi),
    .m_axi_dma  (m_axi_dma),
    .m_axi_misc (m_axi_misc),
    .m_axi_mac  (m_axi_mac)
  );


  //---------------------------------------------------------------------------
  // Map DMA Engine Masters to CPU Memory Port
  //---------------------------------------------------------------------------

  // Everything Disabled
  if (DISABLED == 4'b1111) begin : axi_hp_noconnect
    always_comb begin
      axi_hp.drive_read_idle();
      axi_hp.drive_aw_idle();
      axi_hp.drive_w_idle();
      axi_hp.bready = 1'b0;
      axi_hp.rready = 1'b0;
    end
  end : axi_hp_noconnect else
  // Only port0 Enabled
  if (DISABLED == 4'b1110) begin : axi_hp_directconnect
    always_comb begin
      `AXI4_ASSIGN(axi_hp,axi_hp_dma[0])
      axi_hp_dma[1].wready  = 1'b0;
      axi_hp_dma[2].wready  = 1'b0;
      axi_hp_dma[3].wready  = 1'b0;
      axi_hp_dma[1].awready = 1'b0;
      axi_hp_dma[2].awready = 1'b0;
      axi_hp_dma[3].awready = 1'b0;
      axi_hp_dma[1].arready = 1'b0;
      axi_hp_dma[2].arready = 1'b0;
      axi_hp_dma[3].arready = 1'b0;
      axi_hp_dma[1].bvalid  = 1'b0;
      axi_hp_dma[2].bvalid  = 1'b0;
      axi_hp_dma[3].bvalid  = 1'b0;
      axi_hp_dma[1].rvalid  = 1'b0;
      axi_hp_dma[2].rvalid  = 1'b0;
      axi_hp_dma[3].rvalid  = 1'b0;
    end
  // All other cases
  end : axi_hp_directconnect else begin : axi_hp_interconnect
    axi_interconnect_dma axi_interconnect_dma_i (
      .m_axi_hp     (axi_hp),
      .s_axi_hp_dma (axi_hp_dma)
    );
  end : axi_hp_interconnect


  //---------------------------------------------------------------------------
  // 10 Gigabit Ethernet
  //---------------------------------------------------------------------------

  logic       refclk;   // 156 Mhz Ref 10 GbE
  logic [0:0] qpll0_reset;
  logic [3:0] qpll0_reset_i;
  logic [0:0] qpll0_lock;
  logic [0:0] qpll0_clk;
  logic [0:0] qpll0_refclk;
  logic [0:0] qpll1_reset;
  logic [3:0] qpll1_reset_i;
  logic [0:0] qpll1_lock;
  logic [0:0] qpll1_clk;
  logic [0:0] qpll1_refclk;

  assign qpll0_reset[0] = qpll0_reset_i[0] || qpll0_reset_i[1] ||
                          qpll0_reset_i[2] || qpll0_reset_i[3];
  assign qpll1_reset[0] = qpll1_reset_i[0] || qpll1_reset_i[1] ||
                          qpll1_reset_i[2] || qpll1_reset_i[3];

  // The following logic is shared amongst potentially 4X10GBE interfaces
  if (IS10GBE != 0) begin : xge_common

    // Clocking signals for MGTs
    IBUFDS_GTE4 ibufds_gte4_refclk (
      .I     (refclk_p),
      .IB    (refclk_n),
      .CEB   (1'b0),
      .O     (refclk),
      .ODIV2 ()
    );

    xge_pcs_pma_common_wrapper xge_pcs_pma_common_wrapper_i (
      .refclk         (refclk),
      .qpll0reset     (qpll0_reset),
      .qpll0lock      (qpll0_lock),
      .qpll0outclk    (qpll0_clk),
      .qpll0outrefclk (qpll0_refclk),
      .qpll1reset     (qpll1_reset),
      .qpll1lock      (qpll1_lock),
      .qpll1outclk    (qpll1_clk),
      .qpll1outrefclk (qpll1_refclk)
    );

  end : xge_common


  //---------------------------------------------------------------------------
  // Generate QSFP Lanes
  //---------------------------------------------------------------------------

  logic [3:0] rx_rec_clk_out_i;
  assign rx_rec_clk_out = rx_rec_clk_out_i[0];

  generate
    genvar lane;
    begin : mgt_lanes
      // Repeat logic for up to 4 QSFP lanes
      for(lane = 0; lane < 4; lane++) begin : lane_loop

        //---------------------------------------
        // AXI-Lite to RegPort Bridge
        //---------------------------------------

        // Map to 0x4000 space
        always_comb begin
          `AXI4LITE_ASSIGN(m_axi_misc_v[lane],m_axi_misc[lane])
           m_axi_misc_v[lane].araddr = 0;
           m_axi_misc_v[lane].araddr[13:0] = m_axi_misc[lane].araddr[13:0];
           m_axi_misc_v[lane].awaddr = 0;
           m_axi_misc_v[lane].awaddr[13:0] = m_axi_misc[lane].awaddr[13:0];
        end

        // AXI4-Lite to RegPort (PS to PL Register Access)
        // NOTE: We always have a register interface even if the block is
        //       unused, so that the driver can query the status.
        typedef logic [REG_AWIDTH_MISC-1:0] reg_addr_t;
        typedef logic [REG_DWIDTH-1:0]      reg_data_t;

        logic      reg_wr_req;
        reg_addr_t reg_wr_addr;
        reg_data_t reg_wr_data;
        logic      reg_rd_req;
        reg_addr_t reg_rd_addr;
        logic      reg_rd_resp, reg_rd_resp_io, reg_rd_resp_eth_if;
        reg_data_t reg_rd_data, reg_rd_data_io, reg_rd_data_eth_if;

        axil_regport_master #(
          .DWIDTH  (REG_DWIDTH),      // Width of the AXI4-Lite data bus (must be 32 or 64)
          .AWIDTH  (REG_AWIDTH_MISC), // Width of the address bus
          .WRBASE  (0),               // Write address base
          .RDBASE  (0),               // Read address base
          .TIMEOUT (10)               // log2(timeout). Read will timeout after (2^TIMEOUT - 1) cycles
        ) axil_regport_master_i (
          // Clock and reset
          .s_axi_aclk    (m_axi_misc_v[lane].clk),
          .s_axi_aresetn (!m_axi_misc_v[lane].rst),
          `AXI4LITE_PORT_ASSIGN_NR(s_axi,m_axi_misc_v[lane])
          // Register port: Write port (domain: reg_clk)
          .reg_clk       (bus_clk),
          .reg_wr_req    (reg_wr_req),
          .reg_wr_addr   (reg_wr_addr),
          .reg_wr_data   (reg_wr_data),
          .reg_wr_keep   (/*unused*/),
          // Register port: Read port (domain: reg_clk)
          .reg_rd_req    (reg_rd_req),
          .reg_rd_addr   (reg_rd_addr),
          .reg_rd_resp   (reg_rd_resp),
          .reg_rd_data   (reg_rd_data)
        );

        // Regport Mux for response
        regport_resp_mux #(
          .WIDTH      (REG_DWIDTH),
          .NUM_SLAVES (2)
        ) regport_resp_mux_i (
          .clk(bus_clk), .reset(bus_rst),
          .sla_rd_resp({reg_rd_resp_eth_if, reg_rd_resp_io}),
          .sla_rd_data({reg_rd_data_eth_if, reg_rd_data_io}),
          .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
        );


        //---------------------------------------
        // MGT IO Core
        //---------------------------------------

        localparam MGT_W      = (IS100GBE) ? 512 : 64;
        localparam MGT_USER_W = $clog2(MGT_W/8)+1;

        // The Clocking for the MGT interfaces comes from the MGT Wrapper
        // depending on the bus it may change.
        logic mgt_rst, mgt_clk;
        AxiStreamIf #(.DATA_WIDTH(MGT_W),.USER_WIDTH(MGT_USER_W))
          mgt_tx(mgt_clk, mgt_rst);
        AxiStreamIf #(.DATA_WIDTH(MGT_W),.USER_WIDTH(MGT_USER_W),.TKEEP(0))
          mgt_rx(mgt_clk, mgt_rst);

        logic mgt_pause_req;
        logic [3:0] tx_p_lane;
        logic [3:0] tx_n_lane;

        if (IS10GBE[lane]) begin
          // Single lane case:
          assign tx_p[lane] = tx_p_lane[lane];
          assign tx_n[lane] = tx_n_lane[lane];
        end else if (IS100GBE[lane] || ISAURORA[lane]) begin
          // Multi lane case:
          assign tx_p = tx_p_lane;
          assign tx_n = tx_n_lane;
        end

        x4xx_mgt_io_core #(
          .PROTOCOL   (PROTOCOL[lane]),
          .REG_BASE   (REG_BASE_SFP_IO),
          .REG_DWIDTH (REG_DWIDTH),      // Width of the AXI4-Lite data bus (must be 32 or 64)
          .REG_AWIDTH (REG_AWIDTH_MISC), // Width of the address bus
          .PORTNUM    (PORTNUM),
          .LANENUM    (lane)
        ) x4xx_mgt_io_core_i (
          // Must reset all channels on quad when QSFP GTX core is reset
          .areset         (areset),

          .mgt_rst        (mgt_rst),
          .mgt_clk        (mgt_clk),

          .clk100         (clk100),

          .bus_rst        (bus_rst),
          .bus_clk        (bus_clk),

          .refclk_p       (refclk_p),
          .refclk_n       (refclk_n),
          .tx_p           (tx_p_lane),
          .tx_n           (tx_n_lane),
          .rx_p           (rx_p),
          .rx_n           (rx_n),

          // Common signals (for single lane instances)
          .qpll0_reset    (qpll0_reset_i[lane]),
          .qpll0_lock     (qpll0_lock),
          .qpll0_clk      (qpll0_clk),
          .qpll0_refclk   (qpll0_refclk),
          .qpll1_reset    (qpll1_reset_i[lane]),
          .qpll1_lock     (qpll1_lock),
          .qpll1_clk      (qpll1_clk),
          .qpll1_refclk   (qpll1_refclk),

          // RegPort
          .reg_wr_req     (reg_wr_req),
          .reg_wr_addr    (reg_wr_addr),
          .reg_wr_data    (reg_wr_data),
          .reg_rd_req     (reg_rd_req),
          .reg_rd_addr    (reg_rd_addr),
          .reg_rd_resp    (reg_rd_resp_io),
          .reg_rd_data    (reg_rd_data_io),
          // AxiLite
          .m_axi_mac      (m_axi_mac[lane]),
          // Pause
          .mgt_pause_req  (mgt_pause_req),
          // Data
          .mgt_tx         (mgt_tx),
          .mgt_rx         (mgt_rx),

          // Misc.
          .rx_rec_clk_out (rx_rec_clk_out_i[lane]),
          .port_info      (port_info[lane]),
          .link_up        (link_up[lane]),
          .activity       (activity[lane])
        );


        if (IS100GBE[lane] || IS10GBE[lane]) begin : eth_port

          //---------------------------------------
          // Ethernet IPv4 Interface for CHDR
          //---------------------------------------

          // Option to use a bigger FIFO for 100GBe.
          // This is address width so +1 doubles the size +2 quadruples it.
          localparam CHDR_FIFO_SIZE = (IS100GBE[lane]) ? BYTE_MTU+2 : BYTE_MTU;

          AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TUSER(0))
            c2e (s_axi.clk, s_axi.rst);
          AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TUSER(0))
            e2c (s_axi.clk, s_axi.rst);

          localparam PAUSE_EN = (IS100GBE[lane]) ? 1 : 0;

          // Ethernet interface
          // (1) routes the packet to CHDR/CPU
          // (2) implements a wrap back (eth_tx/eth_rx)
          eth_ipv4_interface #(
            .PROTOVER       (RFNOC_PROTOVER),
            .CPU_FIFO_SIZE  (BYTE_MTU),
            .CHDR_FIFO_SIZE (CHDR_FIFO_SIZE),
            .NODE_INST      (NODE_INST+lane),
            .BASE           (REG_BASE_ETH_SWITCH),
            .PREAMBLE_BYTES (0),
            .ADD_SOF        (0),
            .SYNC           (0),  // c2e/e2c don't use the same clock as eth_tx/eth_rx
            .PAUSE_EN       (PAUSE_EN),
            .ENET_W         (MGT_W),
            .CPU_W          (CPU_W),
            .CHDR_W         (CHDR_W)
          ) eth_ipv4_interface_i (
            .bus_clk          (bus_clk),
            .bus_rst          (bus_rst),
            .device_id        (device_id),
            .reg_wr_req       (reg_wr_req),
            .reg_wr_addr      (reg_wr_addr),
            .reg_wr_data      (reg_wr_data),
            .reg_rd_req       (reg_rd_req),
            .reg_rd_addr      (reg_rd_addr),
            .reg_rd_resp      (reg_rd_resp_eth_if),
            .reg_rd_data      (reg_rd_data_eth_if),
            .eth_pause_req    (mgt_pause_req),
            .eth_tx           (mgt_tx),
            .eth_rx           (mgt_rx),
            .e2v              (e2v[lane]),
            .v2e              (v2e[lane]),
            .e2c              (e2c),
            .c2e              (c2e),
            .my_udp_chdr_port (/* unused */),
            .my_ip            (/* unused */),
            .my_mac           (/* unused */)
          );

          axi_eth_dma axi_eth_dma_i (
            .c2e           (c2e),
            .e2c           (e2c),
            .s_axi_eth_dma (m_axi_dma[lane]),
            .axi_hp        (axi_hp_dma[lane]),
            .eth_tx_irq    (eth_tx_irq[lane]),
            .eth_rx_irq    (eth_rx_irq[lane])
          );

        end : eth_port else begin : not_eth

          //---------------------------------------
          // Terminate DMA for Unused Ethernet
          //---------------------------------------

          // Set unused ETH_DMA ports to default value
          always_comb begin
            m_axi_dma[lane].drive_read_resp(.resp(SLVERR),.data(0));
            m_axi_dma[lane].drive_write_resp(.resp(SLVERR));
            m_axi_dma[lane].arready = 1'b1;
            m_axi_dma[lane].awready = 1'b1;
            m_axi_dma[lane].wready  = 1'b1;

            axi_hp_dma[lane].drive_read_idle();
            axi_hp_dma[lane].drive_aw_idle();
            axi_hp_dma[lane].drive_w_idle();
            axi_hp_dma[lane].bready  = 1'b0;
            axi_hp_dma[lane].rready  = 1'b0;

            mgt_pause_req = 0'b0;

            eth_rx_irq[lane] = 1'b0;
            eth_tx_irq[lane] = 1'b0;

            reg_rd_resp_eth_if = 1'b0;
            reg_rd_data_eth_if = 'h0;
          end

          if (ISAURORA[lane]) begin : aurora_port

            //---------------------------------------
            // Aurora
            //---------------------------------------

            Aurora_not_yet_supported();

            // if MGT_W and CHDR_W mismatch figure out what to do
            always_comb begin
              e2v[lane].tdata  = mgt_rx.tdata;
              e2v[lane].tuser  = 'b0;
              e2v[lane].tkeep  = 'b1;
              e2v[lane].tlast  = mgt_rx.tlast;
              e2v[lane].tvalid = mgt_rx.tvalid;
              mgt_rx.tready    = e2v[lane].tready;

              mgt_tx.tdata     = v2e[lane].tdata;
              mgt_tx.tuser     = 'b0;
              mgt_tx.tkeep     = 'b1;
              mgt_tx.tlast     = v2e[lane].tlast;
              mgt_tx.tvalid    = v2e[lane].tvalid;
              v2e[lane].tready = mgt_tx.tready;
            end

          end else begin : inactive_port

            //---------------------------------------
            // Disabled Port
            //---------------------------------------

            always_comb begin
              e2v[lane].tdata  =  'b0;
              e2v[lane].tuser  =  'b0;
              e2v[lane].tkeep  =  'b1;
              e2v[lane].tlast  = 1'b0;
              e2v[lane].tvalid = 1'b0;
              mgt_rx.tready    = 1'b1;

              mgt_tx.tdata     =  'b0;
              mgt_tx.tuser     =  'b0;
              mgt_tx.tkeep     =  'b1;
              mgt_tx.tlast     = 1'b0;
              mgt_tx.tvalid    = 1'b0;
              v2e[lane].tready = 1'b1;
            end

          end : inactive_port
        end : not_eth
      end : lane_loop
    end : mgt_lanes
  endgenerate

endmodule
