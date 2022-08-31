//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: e320_sfp_wrapper
//
// Description:
//
//   Wrapper for SFP port, including support for Aurora, 10 GbE, and 1 GbE.
//
// Parameters:
//
//   PROTOCOL         : Must be "10GbE", "1GbE", "Aurora", or "Disabled".
//   REG_DWIDTH       : Data width of the AXI-Lite interface and RegPort
//   REG_AWIDTH       : Address width of the AXI-Lite interface and RegPort
//   PORTNUM          : Port number
//   MDIO_EN          : Enables internal MDIO master
//   MDIO_PHYADDR     : Address to use for the MDIO
//   BYTE_MTU         : Log base 2 of the MTU in bytes
//   RFNOC_PROTOVER   : RFNoC protocol version to be reported by transport
//                      adapters.
//   NODE_INST        : RFNoC transport adapter node instance for this port
//   EN_RX_KV_MAP_CFG : Enable KV-map configuration port on transport adapter
//   EN_RX_RAW_PYLD   : Enable raw UDP support on transport adapter
//

`default_nettype none


module e320_sfp_wrapper #(
  parameter        PROTOCOL         = "10GbE",
  parameter        DWIDTH           = 32,
  parameter        AWIDTH           = 14,
  parameter [7:0]  PORTNUM          = 8'd0,
  parameter        MDIO_EN          = 0,
  parameter [4:0]  MDIO_PHYADDR     = 5'd0,
  parameter        BYTE_MTU         = $clog2(8192),
  parameter [15:0] RFNOC_PROTOVER   = {8'd1, 8'd0},
  parameter        NODE_INST        = 0,
  parameter        EN_RX_KV_MAP_CFG = 1,
  parameter        EN_RX_RAW_PYLD   = 1
) (
  // Resets
  input  wire   areset,
  input  wire   bus_rst,

  // Clocks
  input  wire   gt_refclk,
  input  wire   gb_refclk,
  input  wire   misc_clk,
  input  wire   bus_clk,

  // AXI-Lite
  input  wire                s_axi_aclk,
  input  wire                s_axi_aresetn,
  input  wire [  AWIDTH-1:0] s_axi_awaddr,
  input  wire                s_axi_awvalid,
  output wire                s_axi_awready,
  input  wire [  DWIDTH-1:0] s_axi_wdata,
  input  wire [DWIDTH/8-1:0] s_axi_wstrb,
  input  wire                s_axi_wvalid,
  output wire                s_axi_wready,
  output wire [         1:0] s_axi_bresp,
  output wire                s_axi_bvalid,
  input  wire                s_axi_bready,
  input  wire [  AWIDTH-1:0] s_axi_araddr,
  input  wire                s_axi_arvalid,
  output wire                s_axi_arready,
  output wire [  DWIDTH-1:0] s_axi_rdata,
  output wire [         1:0] s_axi_rresp,
  output wire                s_axi_rvalid,
  input  wire                s_axi_rready,

  // SFP high-speed IO
  output wire txp,
  output wire txn,
  input  wire rxp,
  input  wire rxn,

  // SFP low-speed IO
  input  wire sfpp_present_n,
  input  wire sfpp_rxlos,
  input  wire sfpp_tx_fault,
  output wire sfpp_tx_disable,

  // GT Common
  input  wire qpllrefclklost,
  input  wire qplllock,
  input  wire qplloutclk,
  input  wire qplloutrefclk,
  output wire qpllreset,

  // Aurora MMCM
  input  wire mmcm_locked,
  output wire gt_pll_lock,
  output wire gt_tx_out_clk_unbuf,

  // Ethernet to RFNoC
  output wire [63:0] e2v_tdata,
  output wire        e2v_tlast,
  output wire        e2v_tvalid,
  input  wire        e2v_tready,

  // RFNoC to Ethernet
  input  wire [63:0] v2e_tdata,
  input  wire        v2e_tlast,
  input  wire        v2e_tvalid,
  output wire        v2e_tready,

  // Ethernet to CPU
  output wire [63:0] e2c_tdata,
  output wire [ 7:0] e2c_tkeep,
  output wire        e2c_tlast,
  output wire        e2c_tvalid,
  input  wire        e2c_tready,

  // CPU to Ethernet
  input  wire [63:0] c2e_tdata,
  input  wire [ 7:0] c2e_tkeep,
  input  wire        c2e_tlast,
  input  wire        c2e_tvalid,
  output wire        c2e_tready,

  // Misc
  output wire [31:0] port_info,
  input  wire [15:0] device_id,
  output wire        link_up,
  output wire        activity
);

  localparam REG_BASE_SFP_IO      = 14'h0;
  localparam REG_BASE_ETH_SWITCH  = 14'h1000;

  // AXI4-Lite to RegPort (PS to PL Register Access)
  wire                reg_wr_req;
  wire  [AWIDTH-1:0]  reg_wr_addr;
  wire  [DWIDTH-1:0]  reg_wr_data;
  wire                reg_rd_req;
  wire  [AWIDTH-1:0]  reg_rd_addr;
  wire                reg_rd_resp, reg_rd_resp_io, reg_rd_resp_eth_if;
  wire  [DWIDTH-1:0]  reg_rd_data, reg_rd_data_io, reg_rd_data_eth_if;

  axil_regport_master #(
    .DWIDTH         (DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
    .AWIDTH         (AWIDTH),   // Width of the address bus
    .WRBASE         (0),        // Write address base
    .RDBASE         (0),        // Read address base
    .TIMEOUT        (10)        // log2(timeout). Read will timeout after (2^TIMEOUT - 1) cycles
  ) sfp_reg_mst_i (
    // Clock and reset
    .s_axi_aclk     (s_axi_aclk),
    .s_axi_aresetn  (s_axi_aresetn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr   (s_axi_awaddr),
    .s_axi_awvalid  (s_axi_awvalid),
    .s_axi_awready  (s_axi_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata    (s_axi_wdata),
    .s_axi_wstrb    (s_axi_wstrb),
    .s_axi_wvalid   (s_axi_wvalid),
    .s_axi_wready   (s_axi_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp    (s_axi_bresp),
    .s_axi_bvalid   (s_axi_bvalid),
    .s_axi_bready   (s_axi_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr   (s_axi_araddr),
    .s_axi_arvalid  (s_axi_arvalid),
    .s_axi_arready  (s_axi_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata    (s_axi_rdata),
    .s_axi_rresp    (s_axi_rresp),
    .s_axi_rvalid   (s_axi_rvalid),
    .s_axi_rready   (s_axi_rready),
    // Register port: Write port (domain: reg_clk)
    .reg_clk        (bus_clk),
    .reg_wr_req     (reg_wr_req),
    .reg_wr_addr    (reg_wr_addr),
    .reg_wr_data    (reg_wr_data),
    // Register port: Read port (domain: reg_clk)
    .reg_rd_req     (reg_rd_req),
    .reg_rd_addr    (reg_rd_addr),
    .reg_rd_resp    (reg_rd_resp),
    .reg_rd_data    (reg_rd_data)
  );

  // RegPort Mux for response
  regport_resp_mux #(
    .WIDTH      (DWIDTH),
    .NUM_SLAVES (2)
  ) reg_resp_mux_i (
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp({reg_rd_resp_eth_if, reg_rd_resp_io}),
    .sla_rd_data({reg_rd_data_eth_if, reg_rd_data_io}),
    .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
  );

  wire [63:0] sfpo_tdata, sfpi_tdata;
  wire [3:0]  sfpo_tuser, sfpi_tuser;
  wire        sfpo_tlast, sfpi_tlast, sfpo_tvalid, sfpi_tvalid, sfpo_tready, sfpi_tready;

  e320_mgt_io_core #(
    .PROTOCOL       (PROTOCOL),
    .REG_BASE       (REG_BASE_SFP_IO),
    .REG_DWIDTH     (DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
    .REG_AWIDTH     (AWIDTH),   // Width of the address bus
    .MDIO_EN        (MDIO_EN),
    .MDIO_PHYADDR   (MDIO_PHYADDR),
    .PORTNUM        (PORTNUM)
  ) mgt_io_i (
    .areset         (areset),
    .gt_refclk      (gt_refclk),
    .gb_refclk      (gb_refclk),
    .misc_clk       (misc_clk),

    .bus_rst        (bus_rst),
    .bus_clk        (bus_clk),

    .txp            (txp),
    .txn            (txn),
    .rxp            (rxp),
    .rxn            (rxn),

    .sfpp_rxlos     (sfpp_rxlos),
    .sfpp_tx_fault  (sfpp_tx_fault),
    .sfpp_tx_disable(sfpp_tx_disable),

    // RegPort
    .reg_wr_req     (reg_wr_req),
    .reg_wr_addr    (reg_wr_addr),
    .reg_wr_data    (reg_wr_data),
    .reg_rd_req     (reg_rd_req),
    .reg_rd_addr    (reg_rd_addr),
    .reg_rd_resp    (reg_rd_resp_io),
    .reg_rd_data    (reg_rd_data_io),

    // RFNoC to Ethernet
    .s_axis_tdata   (sfpi_tdata),
    .s_axis_tuser   (sfpi_tuser),
    .s_axis_tlast   (sfpi_tlast),
    .s_axis_tvalid  (sfpi_tvalid),
    .s_axis_tready  (sfpi_tready),

    // Ethernet to RFNoC
    .m_axis_tdata   (sfpo_tdata),
    .m_axis_tuser   (sfpo_tuser),
    .m_axis_tlast   (sfpo_tlast),
    .m_axis_tvalid  (sfpo_tvalid),
    .m_axis_tready  (sfpo_tready),

    .port_info      (port_info),
    .link_up        (link_up),
    .activity       (activity)
  );

  generate
    // Tie off the Ethernet switch for these protocols that do not use it.
    if (PROTOCOL == "Aurora" || PROTOCOL == "Disabled") begin : gen_no_eth

      //set unused wires to default value
      assign e2c_tdata      = 64'h0;
      assign e2c_tkeep      = 8'h0;
      assign e2c_tlast      = 1'b0;
      assign e2c_tvalid     = 1'b0;
      assign c2e_tready     = 1'b1;

      assign reg_rd_resp_eth_if = 1'b0;
      assign reg_rd_data_eth_if = 'h0;

    end else begin : gen_eth

      wire [3:0] e2c_tuser;
      wire [3:0] c2e_tuser;

      // In AXI Stream, tkeep is the byte qualifier that indicates
      // whether the content of the associated byte
      // of TDATA is processed as part of the data stream.
      // tuser as used in eth_interface is the number of valid bytes

      // Converting tuser to tkeep for ingress packets
      assign e2c_tkeep = ~e2c_tlast ? 8'b1111_1111
                       : (e2c_tuser[2:0] == 3'd0) ? 8'b1111_1111
                       : (e2c_tuser[2:0] == 3'd1) ? 8'b0000_0001
                       : (e2c_tuser[2:0] == 3'd2) ? 8'b0000_0011
                       : (e2c_tuser[2:0] == 3'd3) ? 8'b0000_0111
                       : (e2c_tuser[2:0] == 3'd4) ? 8'b0000_1111
                       : (e2c_tuser[2:0] == 3'd5) ? 8'b0001_1111
                       : (e2c_tuser[2:0] == 3'd6) ? 8'b0011_1111
                       : 8'b0111_1111;

      // Converting tkeep to tuser for egress packets
      assign c2e_tuser = ~c2e_tlast ? 4'd0
                       : (c2e_tkeep == 8'b1111_1111) ? 4'd0
                       : (c2e_tkeep == 8'b0111_1111) ? 4'd7
                       : (c2e_tkeep == 8'b0011_1111) ? 4'd6
                       : (c2e_tkeep == 8'b0001_1111) ? 4'd5
                       : (c2e_tkeep == 8'b0000_1111) ? 4'd4
                       : (c2e_tkeep == 8'b0000_0111) ? 4'd3
                       : (c2e_tkeep == 8'b0000_0011) ? 4'd2
                       : (c2e_tkeep == 8'b0000_0001) ? 4'd1
                       : 4'd0;

      if (EN_RX_KV_MAP_CFG || EN_RX_RAW_PYLD) begin : gen_eth_ipv4_interface_wrapper
        eth_ipv4_interface_wrapper #(
          .PROTOVER         (RFNOC_PROTOVER),
          .CPU_FIFO_SIZE    (BYTE_MTU),
          .CHDR_FIFO_SIZE   (BYTE_MTU),
          .NODE_INST        (NODE_INST),
          .REG_AWIDTH       (AWIDTH),
          .BASE             (REG_BASE_ETH_SWITCH),
          .SYNC             (1),
          .EN_RX_KV_MAP_CFG (EN_RX_KV_MAP_CFG),
          .EN_RX_RAW_PYLD   (EN_RX_RAW_PYLD)
        ) eth_ipv4_interface_wrapper_i (
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
          .my_mac           (),
          .my_ip            (),
          .my_udp_chdr_port (),
          .eth_clk          (bus_clk),
          .eth_rst          (bus_rst),
          .eth_pause_req    (),
          .eth_tx_tdata     (sfpi_tdata),
          .eth_tx_tuser     (sfpi_tuser),
          .eth_tx_tkeep     (),
          .eth_tx_tlast     (sfpi_tlast),
          .eth_tx_tvalid    (sfpi_tvalid),
          .eth_tx_tready    (sfpi_tready),
          .eth_rx_tdata     (sfpo_tdata),
          .eth_rx_tuser     (sfpo_tuser),
          .eth_rx_tlast     (sfpo_tlast),
          .eth_rx_tvalid    (sfpo_tvalid),
          .eth_rx_tready    (sfpo_tready),
          .e2v_tdata        (e2v_tdata),
          .e2v_tlast        (e2v_tlast),
          .e2v_tvalid       (e2v_tvalid),
          .e2v_tready       (e2v_tready),
          .v2e_tdata        (v2e_tdata),
          .v2e_tlast        (v2e_tlast),
          .v2e_tvalid       (v2e_tvalid),
          .v2e_tready       (v2e_tready),
          .cpu_clk          (bus_clk),
          .cpu_rst          (bus_rst),
          .e2c_tdata        (e2c_tdata),
          .e2c_tuser        (e2c_tuser),
          .e2c_tlast        (e2c_tlast),
          .e2c_tvalid       (e2c_tvalid),
          .e2c_tready       (e2c_tready),
          .c2e_tdata        (c2e_tdata),
          .c2e_tuser        (c2e_tuser),
          .c2e_tlast        (c2e_tlast),
          .c2e_tvalid       (c2e_tvalid),
          .c2e_tready       (c2e_tready)
        );
      end // gen_eth_ipv4_interface_wrapper
      else begin : gen_eth_interface
        eth_interface #(
          .PROTOVER   (RFNOC_PROTOVER),
          .MTU        (BYTE_MTU-3),        // Log base 2 of the MTU in 64-words
          .NODE_INST  (NODE_INST),
          .REG_AWIDTH (AWIDTH),
          .BASE       (REG_BASE_ETH_SWITCH)
        ) eth_interface (
          .clk           (bus_clk),
          .reset         (bus_rst),
          .device_id     (device_id),
          .reg_wr_req    (reg_wr_req),
          .reg_wr_addr   (reg_wr_addr),
          .reg_wr_data   (reg_wr_data),
          .reg_rd_req    (reg_rd_req),
          .reg_rd_addr   (reg_rd_addr),
          .reg_rd_resp   (reg_rd_resp_eth_if),
          .reg_rd_data   (reg_rd_data_eth_if),
          .my_mac        (),
          .my_ip         (),
          .my_udp_port   (),
          .eth_tx_tdata  (sfpi_tdata),
          .eth_tx_tuser  (sfpi_tuser),
          .eth_tx_tlast  (sfpi_tlast),
          .eth_tx_tvalid (sfpi_tvalid),
          .eth_tx_tready (sfpi_tready),
          .eth_rx_tdata  (sfpo_tdata),
          .eth_rx_tuser  (sfpo_tuser),
          .eth_rx_tlast  (sfpo_tlast),
          .eth_rx_tvalid (sfpo_tvalid),
          .eth_rx_tready (sfpo_tready),
          .e2v_tdata     (e2v_tdata),
          .e2v_tlast     (e2v_tlast),
          .e2v_tvalid    (e2v_tvalid),
          .e2v_tready    (e2v_tready),
          .v2e_tdata     (v2e_tdata),
          .v2e_tlast     (v2e_tlast),
          .v2e_tvalid    (v2e_tvalid),
          .v2e_tready    (v2e_tready),
          .e2c_tdata     (e2c_tdata),
          .e2c_tuser     (e2c_tuser),
          .e2c_tlast     (e2c_tlast),
          .e2c_tvalid    (e2c_tvalid),
          .e2c_tready    (e2c_tready),
          .c2e_tdata     (c2e_tdata),
          .c2e_tuser     (c2e_tuser),
          .c2e_tlast     (c2e_tlast),
          .c2e_tvalid    (c2e_tvalid),
          .c2e_tready    (c2e_tready)
        );
      end // gen_eth_interface
    end // gen_eth
  endgenerate

endmodule // e320_sfp_wrapper


`default_nettype wire
