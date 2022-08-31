//
// Copyright 2018 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: n3xx_mgt_wrapper
//
// Description:
//
//   Wrapper for single MGT, including support for Aurora, WhiteRabbit, 10 GbE,
//   and 1 GbE.
//
// Parameters:
//
//   PROTOCOL         : Must be "10GbE", "1GbE", "Aurora", or "Disabled".
//   REG_DWIDTH       : Data width of the AXI-Lite interface and RegPort
//   REG_AWIDTH       : Address width of the AXI-Lite interface and RegPort\
//   GT_COMMON        : Use GT Common ports on MGT
//   PORTNUM          : Port number
//   MDIO_EN          : Enables internal MDIO master
//   MDIO_PHYADDR     : Address to use for the MDIO
//   BYTE_MTU         : Log base 2 of the MTU size in bytes
//   RFNOC_PROTOVER   : RFNoC protocol version to be reported by transport
//                      adapters.
//   NODE_INST        : RFNoC transport adapter node instance for this port
//   EN_RX_KV_MAP_CFG : Enable KV-map configuration port on transport adapter
//   EN_RX_RAW_PYLD   : Enable raw UDP support on transport adapter
//

`default_nettype none


module n3xx_mgt_wrapper #(
  parameter        PROTOCOL         = "10GbE",
  parameter        REG_DWIDTH       = 32,
  parameter        REG_AWIDTH       = 14,
  parameter        REG_BASE         = 16'h0000,
  parameter        GT_COMMON        = 1,
  parameter [ 7:0] PORTNUM          = 8'd0,
  parameter        MDIO_EN          = 0,
  parameter [ 4:0] MDIO_PHYADDR     = 5'd0,
  parameter        BYTE_MTU         = $clog2(8192),
  parameter [15:0] RFNOC_PROTOVER   = {8'd1, 8'd0},
  parameter        NODE_INST        = 0,
  parameter        EN_RX_KV_MAP_CFG = 1,
  parameter        EN_RX_RAW_PYLD   = 1
) (
  // Resets
  input  wire        areset,
  input  wire        bus_rst,

  // Clocks
  input  wire        gt_refclk,
  input  wire        gb_refclk,
  input  wire        misc_clk,
  input  wire        bus_clk,
  input  wire        user_clk,
  input  wire        sync_clk,

  // RegPort interface
  input  wire                  reg_wr_req,
  input  wire [REG_AWIDTH-1:0] reg_wr_addr,
  input  wire [REG_DWIDTH-1:0] reg_wr_data,
  input  wire                  reg_rd_req,
  input  wire [REG_AWIDTH-1:0] reg_rd_addr,
  output wire                  reg_rd_resp,
  output wire [REG_DWIDTH-1:0] reg_rd_data,

  // High-speed IO
  output wire        txp,
  output wire        txn,
  input  wire        rxp,
  input  wire        rxn,

  // SFP low-speed IO
  input  wire        mod_present_n,
  input  wire        mod_rxlos,
  input  wire        mod_tx_fault,
  output wire        mod_tx_disable,

  // GT Common
  input  wire        qpllrefclklost,
  input  wire        qplllock,
  input  wire        qplloutclk,
  input  wire        qplloutrefclk,
  output wire        qpllreset,

  // Aurora MMCM
  input  wire        mmcm_locked,
  output wire        gt_pll_lock,
  output wire        gt_tx_out_clk_unbuf,

  // Vita router interface
  output wire [63:0] e2v_tdata,
  output wire        e2v_tlast,
  output wire        e2v_tvalid,
  input  wire        e2v_tready,

  input  wire [63:0] v2e_tdata,
  input  wire        v2e_tlast,
  input  wire        v2e_tvalid,
  output wire        v2e_tready,

  // CPU
  output wire [63:0] e2c_tdata,
  output wire [7:0]  e2c_tkeep,
  output wire        e2c_tlast,
  output wire        e2c_tvalid,
  input  wire        e2c_tready,

  input  wire [63:0] c2e_tdata,
  input  wire [7:0]  c2e_tkeep,
  input  wire        c2e_tlast,
  input  wire        c2e_tvalid,
  output wire        c2e_tready,

  // MISC
  output wire [31:0] port_info,
  input  wire [15:0] device_id,

  // Timebase Outputs
  output wire         mod_pps,
  output wire         mod_refclk,

  // Sideband White Rabbit Control
  input  wire         wr_reset_n,
  input  wire         wr_refclk,

  output wire         wr_dac_sclk,
  output wire         wr_dac_din,
  output wire         wr_dac_clr_n,
  output wire         wr_dac_cs_n,
  output wire         wr_dac_ldac_n,

  output wire         wr_eeprom_scl_o,
  input  wire         wr_eeprom_scl_i,
  output wire         wr_eeprom_sda_o,
  input  wire         wr_eeprom_sda_i,

  input  wire         wr_uart_rx,
  output wire         wr_uart_tx,

  // WR AXI Control
  output wire                    wr_axi_aclk,
  input  wire                    wr_axi_aresetn,
  input  wire [31:0]             wr_axi_awaddr,
  input  wire                    wr_axi_awvalid,
  output wire                    wr_axi_awready,
  input  wire [REG_DWIDTH-1:0]   wr_axi_wdata,
  input  wire [REG_DWIDTH/8-1:0] wr_axi_wstrb,
  input  wire                    wr_axi_wvalid,
  output wire                    wr_axi_wready,
  output wire [1:0]              wr_axi_bresp,
  output wire                    wr_axi_bvalid,
  input  wire                    wr_axi_bready,
  input  wire [31:0]             wr_axi_araddr,
  input  wire                    wr_axi_arvalid,
  output wire                    wr_axi_arready,
  output wire [REG_DWIDTH-1:0]   wr_axi_rdata,
  output wire [1:0]              wr_axi_rresp,
  output wire                    wr_axi_rvalid,
  input  wire                    wr_axi_rready,
  output wire                    wr_axi_rlast,

  output wire         link_up,
  output wire         activity

);

  localparam [REG_AWIDTH-1:0] REG_BASE_MGT_IO      = {REG_AWIDTH{1'b0}} + REG_BASE;
  localparam [REG_AWIDTH-1:0] REG_BASE_ETH_SWITCH  = {REG_AWIDTH{1'b0}} + 16'h1000 + REG_BASE;

  // AXI4-Lite to RegPort (PS to PL Register Access)
  wire                    reg_rd_resp_io, reg_rd_resp_eth_if;
  wire  [REG_DWIDTH-1:0]  reg_rd_data_io, reg_rd_data_eth_if;

  // Regport Mux for response
  regport_resp_mux #(
    .WIDTH      (REG_DWIDTH),
    .NUM_SLAVES (2)
  ) reg_resp_mux_i (
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp({reg_rd_resp_eth_if, reg_rd_resp_io}),
    .sla_rd_data({reg_rd_data_eth_if, reg_rd_data_io}),
    .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
  );

  wire [63:0] mgto_tdata, mgti_tdata;
  wire [3:0]  mgto_tuser, mgti_tuser;
  wire        mgto_tlast, mgti_tlast, mgto_tvalid, mgti_tvalid, mgto_tready, mgti_tready;

  generate
    if (PROTOCOL != "WhiteRabbit") begin
      n3xx_mgt_io_core #(
        .PROTOCOL       (PROTOCOL),
        .REG_BASE       (REG_BASE_MGT_IO),
        .REG_DWIDTH     (REG_DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
        .REG_AWIDTH     (REG_AWIDTH),   // Width of the address bus
        .GT_COMMON      (GT_COMMON),
        .MDIO_EN        (MDIO_EN),
        .MDIO_PHYADDR   (MDIO_PHYADDR),
        .PORTNUM        (PORTNUM)
      ) mgt_io_i (
        //must reset all channels on quad when other gtx core is reset
        .areset         (areset),
        .gt_refclk      (gt_refclk),
        .gb_refclk      (gb_refclk),
        .misc_clk       (misc_clk),
        .user_clk       (user_clk),
        .sync_clk       (sync_clk),
        .gt_tx_out_clk_unbuf(gt_tx_out_clk_unbuf),

        .bus_rst        (bus_rst),
        .bus_clk        (bus_clk),
        .qpllreset      (qpllreset),
        .qplllock       (qplllock),
        .qplloutclk     (qplloutclk),
        .qplloutrefclk  (qplloutrefclk),
        .qpllrefclklost (qpllrefclklost),
        .mmcm_locked    (mmcm_locked),
        .gt_pll_lock    (gt_pll_lock),

        .txp            (txp),
        .txn            (txn),
        .rxp            (rxp),
        .rxn            (rxn),

        .sfpp_rxlos     (mod_rxlos),
        .sfpp_tx_fault  (mod_tx_fault),
        .sfpp_tx_disable(mod_tx_disable),

        //RegPort
        .reg_wr_req     (reg_wr_req),
        .reg_wr_addr    (reg_wr_addr),
        .reg_wr_data    (reg_wr_data),
        .reg_rd_req     (reg_rd_req),
        .reg_rd_addr    (reg_rd_addr),
        .reg_rd_resp    (reg_rd_resp_io),
        .reg_rd_data    (reg_rd_data_io),

        // Vita to Ethernet
        .s_axis_tdata   (mgti_tdata),
        .s_axis_tuser   (mgti_tuser),
        .s_axis_tlast   (mgti_tlast),
        .s_axis_tvalid  (mgti_tvalid),
        .s_axis_tready  (mgti_tready),

        // Ethernet to Vita
        .m_axis_tdata   (mgto_tdata),
        .m_axis_tuser   (mgto_tuser),
        .m_axis_tlast   (mgto_tlast),
        .m_axis_tvalid  (mgto_tvalid),
        .m_axis_tready  (mgto_tready),

        .port_info      (port_info),
        .link_up        (link_up),
        .activity       (activity)
      );

    end else begin
      //---------------------------------------------------------------------------------
      // White Rabbit
      //---------------------------------------------------------------------------------

      wire wr_sfp_scl, wr_sfp_sda_o, wr_sfp_sda_i;

      n3xx_wr_top #(
        .g_simulation(1'b0),                // in std_logic
        .g_dpram_size(131072/4),
        .g_dpram_initf("../../../../bin/wrpc/wrc_phy16.bram")
      ) wr_inst (
        .areset_n_i        (wr_reset_n),      // in std_logic;   -- active low reset, optional
        .wr_refclk_buf_i   (wr_refclk),       // in std_logic;   -- 20MHz VCXO after IBUFGDS
        .gige_refclk_buf_i (gt_refclk),       // in std_logic;   -- 125 MHz MGT Ref after IBUFDS_GTE2
        .dac_sclk_o        (wr_dac_sclk),     // out std_logic;  -- N3xx cWB-DAC-SCLK
        .dac_din_o         (wr_dac_din),      // out std_logic;  -- N3xx cWB-DAC-DIN
        .dac_clr_n_o       (wr_dac_clr_n),    // out std_logic;  -- N3xx cWB-DAC-nCLR
        .dac_cs_n_o        (wr_dac_cs_n),     // out std_logic;  -- N3xx cWB-DAC-nSYNC
        .dac_ldac_n_o      (wr_dac_ldac_n),   // out std_logic;  -- N3xx cWB-DAC-nLDAC
        .LED_ACT           (activity),        // out std_logic;  -- connect to SFP+ ACT
        .LED_LINK          (link_up),         // out std_logic;  -- connect to SFP+ LINK
        .sfp_txp_o         (txp),             // out std_logic;
        .sfp_txn_o         (txn),             // out std_logic;
        .sfp_rxp_i         (rxp),             // in  std_logic;
        .sfp_rxn_i         (rxn),             // in  std_logic;
        .sfp_mod_def0_b    (mod_present_n),   // in std_logic;   - sfp detect
        .eeprom_scl_o      (wr_eeprom_scl_o),
        .eeprom_scl_i      (wr_eeprom_scl_i),
        .eeprom_sda_o      (wr_eeprom_sda_o),
        .eeprom_sda_i      (wr_eeprom_sda_i),
        .sfp_scl_o         (wr_sfp_scl),
        .sfp_scl_i         (wr_sfp_scl),
        .sfp_sda_o         (wr_sfp_sda_o),
        .sfp_sda_i         (wr_sfp_sda_i),
        .sfp_tx_fault_i    (mod_tx_fault),    // in  std_logic;
        .sfp_tx_disable_o  (mod_tx_disable),  // out std_logic;
        .sfp_los_i         (mod_rxlos),       // in  std_logic;
        .wr_uart_rxd       (wr_uart_rx),      // in std_logic;
        .wr_uart_txd       (wr_uart_tx),      // out std_logic;

        .s00_axi_aclk_o    (wr_axi_aclk),
        .s00_axi_aresetn   (wr_axi_aresetn),
        .s00_axi_awaddr    (wr_axi_awaddr),
        .s00_axi_awprot    (3'b0),
        .s00_axi_awvalid   (wr_axi_awvalid),
        .s00_axi_awready   (wr_axi_awready),
        .s00_axi_wdata     (wr_axi_wdata),
        .s00_axi_wstrb     (wr_axi_wstrb),
        .s00_axi_wvalid    (wr_axi_wvalid),
        .s00_axi_wready    (wr_axi_wready),
        .s00_axi_bresp     (wr_axi_bresp),
        .s00_axi_bvalid    (wr_axi_bvalid),
        .s00_axi_bready    (wr_axi_bready),
        .s00_axi_araddr    (wr_axi_araddr),
        .s00_axi_arprot    (3'b0),
        .s00_axi_arvalid   (wr_axi_arvalid),
        .s00_axi_arready   (wr_axi_arready),
        .s00_axi_rdata     (wr_axi_rdata),
        .s00_axi_rresp     (wr_axi_rresp),
        .s00_axi_rvalid    (wr_axi_rvalid),
        .s00_axi_rready    (wr_axi_rready),
        .s00_axi_rlast     (wr_axi_rlast),
        .axi_int_o         (),

        .pps_o             (mod_pps),    // out    std_logic;
        .clk_pps_o         (mod_refclk), // out    std_logic;
        .link_ok_o         (),           // out    std_logic;
        .clk_sys_locked_o  (),           // out    std_logic;
        .clk_dmtd_locked_o (),           // out    std_logic);
        .wr_debug0_o       (),
        .wr_debug1_o       ()
      );

      // TEMPORARY mimic the AXGE SFP EEROM
      sfp_eeprom sfp_eeprom_i (
        .clk_i(bus_clk),
        .sfp_scl(wr_sfp_scl),
        .sfp_sda_i(wr_sfp_sda_o),
        .sfp_sda_o(wr_sfp_sda_i));

      // Assign the port_info vector similarly to mgt_io_core
      localparam [7:0] COMPAT_NUM         = 8'd2;
      localparam [7:0] MGT_PROTOCOL       = 8'd4;
      assign port_info = {COMPAT_NUM, 6'h0, activity, link_up, MGT_PROTOCOL, PORTNUM};

      // Tie off unused outputs.
      assign gt_pll_lock = 1'b0;
      assign gt_tx_out_clk_unbuf = 1'b0;
    end
  endgenerate

  generate
    // Tie off the Ethernet switch for these protocols that do not use it.
    if(PROTOCOL == "Aurora" || PROTOCOL == "Disabled") begin
      // Set unused wires to default values
      assign e2c_tdata      = 64'h0;
      assign e2c_tkeep      = 8'h0;
      assign e2c_tlast      = 1'b0;
      assign e2c_tvalid     = 1'b0;
      assign c2e_tready     = 1'b1;

      assign reg_rd_resp_eth_if = 1'b0;
      assign reg_rd_data_eth_if = 'h0;

      assign e2v_tdata      = mgto_tdata;
      assign e2v_tlast      = mgto_tlast;
      assign e2v_tvalid     = mgto_tvalid;
      assign mgto_tready    = e2v_tready;

      assign mgti_tdata     = v2e_tdata;
      assign mgti_tlast     = v2e_tlast;
      assign mgti_tvalid    = v2e_tvalid;
      assign v2e_tready     = mgti_tready;

    end else if(PROTOCOL == "WhiteRabbit") begin
      // Set unused wires to default values
      assign e2c_tdata      = 64'h0;
      assign e2c_tkeep      = 8'h0;
      assign e2c_tlast      = 1'b0;
      assign e2c_tvalid     = 1'b0;
      assign c2e_tready     = 1'b1;

      assign reg_rd_resp_eth_if = 1'b0;
      assign reg_rd_data_eth_if = 'h0;

      assign e2v_tdata      = 64'b0;
      assign e2v_tlast      =  1'b0;
      assign e2v_tvalid     =  1'b0;

      assign v2e_tready     = 1'b1;

    end else begin

      wire [3:0] e2c_tuser;
      wire [3:0] c2e_tuser;

      // In AXI Stream, tkeep is the byte qualifier that indicates
      // whether the content of the associated byte
      // of TDATA is processed as part of the data stream.
      // tuser as used in eth_switch is the numbier of valid bytes

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
          .REG_AWIDTH       (REG_AWIDTH),
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
          .eth_tx_tdata     (mgti_tdata),
          .eth_tx_tuser     (mgti_tuser),
          .eth_tx_tkeep     (),
          .eth_tx_tlast     (mgti_tlast),
          .eth_tx_tvalid    (mgti_tvalid),
          .eth_tx_tready    (mgti_tready),
          .eth_rx_tdata     (mgto_tdata),
          .eth_rx_tuser     (mgto_tuser),
          .eth_rx_tlast     (mgto_tlast),
          .eth_rx_tvalid    (mgto_tvalid),
          .eth_rx_tready    (mgto_tready),
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
          .MTU        (BYTE_MTU-3),    // Log base 2 of the MTU in 64-bit words
          .NODE_INST  (NODE_INST),
          .REG_AWIDTH (REG_AWIDTH),
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
          .eth_tx_tdata  (mgti_tdata),
          .eth_tx_tuser  (mgti_tuser),
          .eth_tx_tlast  (mgti_tlast),
          .eth_tx_tvalid (mgti_tvalid),
          .eth_tx_tready (mgti_tready),
          .eth_rx_tdata  (mgto_tdata),
          .eth_rx_tuser  (mgto_tuser),
          .eth_rx_tlast  (mgto_tlast),
          .eth_rx_tvalid (mgto_tvalid),
          .eth_rx_tready (mgto_tready),
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
    end
  endgenerate

endmodule // n3xx_mgt_wrapper


`default_nettype wire
