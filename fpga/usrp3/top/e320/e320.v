/////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: e320
// Description:
//   E320 Top Level
//
/////////////////////////////////////////////////////////////////////

`default_nettype none
module e320 (

  // 5V Power Enable
  output wire ENA_PAPWR,

  // GPIO
  output wire      EN_GPIO_2V5,
  output wire      EN_GPIO_3V3,
  output wire      EN_GPIO_VAR_SUPPLY,
  inout wire [7:0] GPIO_PREBUFF,
  inout wire [7:0] GPIO_DIR,
  output wire      GPIO_OE_N,

  // GPS
  output wire CLK_GPS_PWR_EN,
  output wire GPS_INITSURV_N,
  output wire GPS_RST_N,
  input wire  GPS_ALARM,
  input wire  GPS_LOCK,
  input wire  GPS_PHASELOCK,
  input wire  GPS_SURVEY,
  input wire  GPS_WARMUP,

  // SFP
  input wire  SFP1_RX_P,
  input wire  SFP1_RX_N,
  output wire SFP1_TX_P,
  output wire SFP1_TX_N,
  input wire  SFP1_RXLOS,
  input wire  SFP1_TXFAULT,
  input wire  SFP1_MOD_ABS, // Unused
  output wire SFP1_RS0,
  output wire SFP1_RS1,
  output wire SFP1_TXDISABLE,

  // MGT Clocks

  output wire CLK_PLL_SCLK,
  output wire CLK_PLL_SDATA,
  output wire CLK_PLL_SLE,

  input wire  CLK_MGT_125M_P,
  input wire  CLK_MGT_125M_N,
  input wire  CLK_MGT_156_25M_P,
  input wire  CLK_MGT_156_25M_N,

  // PS Connections
  inout wire [53:0] PS_MIO,
  inout wire        PS_CLK,
  inout wire        PS_SRST_B,
  inout wire        PS_POR_B,
  inout wire        DDR_MODCLK_P,
  inout wire        DDR_MODCLK_N,
  inout wire        PS_DDR3_CKE,
  inout wire        PS_DDR3_RESET_N,
  inout wire [31:0] PS_DDR3_DQ,
  inout wire [14:0] PS_DDR3_ADDR,
  inout wire [3:0]  PS_DDR3_DM,
  inout wire [2:0]  PS_DDR3_BA,
  inout wire [3:0]  PS_DDR3_DQS_P,
  inout wire [3:0]  PS_DDR3_DQS_N,
  inout wire        PS_DDR3_ODT,
  inout wire        PS_DDR3_VRN,
  inout wire        PS_DDR3_VRP,
  inout wire        PS_DDR3_WE_N,
  inout wire        PS_DDR3_CS_N,
  inout wire        PS_DDR3_CAS_N,
  inout wire        PS_DDR3_RAS_N,

  // PL DRAM Interface
  input wire         sys_clk_p,
  input wire         sys_clk_n,
  //
  inout wire  [31:0] ddr3_dq,
  inout wire  [3:0]  ddr3_dqs_n,
  inout wire  [3:0]  ddr3_dqs_p,
  output wire [3:0]  ddr3_dm,
  //
  output wire [2:0]  ddr3_ba,
  output wire [15:0] ddr3_addr,
  output wire        ddr3_ras_n,
  output wire        ddr3_cas_n,
  output wire        ddr3_we_n,
  //
  output wire [0:0]  ddr3_cs_n,
  output wire [0:0]  ddr3_cke,
  output wire [0:0]  ddr3_odt,
  //
  output wire [0:0]  ddr3_ck_p,
  output wire [0:0]  ddr3_ck_n,
  //
  output wire        ddr3_reset_n,

  // LEDs
  output wire LED_LINK1,
  output wire LED_ACT1,

  // PPS, REFCLK
  input wire  CLK_SYNC_INT,      // PPS from GPS
  input wire  CLK_SYNC_INT_RAW,  // PPS_RAW from GPS (Unused)
  input wire  CLK_SYNC_EXT,      // PPS from external connector
  input wire  CLK_REF_RAW,       // FPGA reference clock (GPS or external)
  output wire CLK_REF_SEL,       // Select for GPS or external reference clock
  input wire  CLK_MUX_OUT,       // RF clock locked status

  // RF LVDS Data Interface
  //
  // Receive
  input wire RX_CLK_P,
  input wire RX_CLK_N,
  input wire RX_FRAME_P,
  input wire RX_FRAME_N,
  input wire [5:0] RX_DATA_P,
  input wire [5:0] RX_DATA_N,
  //
  // TraNSMIT
  output wire TX_CLK_P,
  output wire TX_CLK_N,
  output wire TX_FRAME_P,
  output wire TX_FRAME_N,
  output wire [5:0] TX_DATA_P,
  output wire [5:0] TX_DATA_N,

  // Switches
  output wire [2:0] FE1_SEL,
  output wire [2:0] FE2_SEL,
  output wire [1:0] RX1_SEL,
  output wire [1:0] RX2_SEL,
  output wire [5:0] RX1_BSEL,
  output wire [5:0] RX2_BSEL,
  output wire [5:0] TX1_BSEL,
  output wire [5:0] TX2_BSEL,

  // SPI
  input wire  XCVR_SPI_MISO,
  output wire XCVR_SPI_MOSI,
  output wire XCVR_SPI_CLK,
  output wire XCVR_SPI_CS_N,

  // AD9361
  output wire XCVR_ENABLE,
  output wire XCVR_SYNC,
  output wire XCVR_TXNRX,
  output wire XCVR_ENA_AGC,
  output wire XCVR_RESET_N,
  input  wire [7:0] XCVR_CTRL_OUT,
  output wire [3:0] XCVR_CTRL_IN,

  // Amplifiers
  output wire TX_HFAMP1_ENA,
  output wire TX_HFAMP2_ENA,
  output wire TX_LFAMP1_ENA,
  output wire TX_LFAMP2_ENA,

  // LEDs
  output wire RX1_GRN_ENA,
  output wire RX2_GRN_ENA,
  output wire TX1_RED_ENA,
  output wire TX2_RED_ENA,
  output wire TXRX1_GRN_ENA,
  output wire TXRX2_GRN_ENA

);

  // Include the RFNoC image core header file
  `ifdef RFNOC_IMAGE_CORE_HDR
    `include `"`RFNOC_IMAGE_CORE_HDR`"
  `else
    ERROR_RFNOC_IMAGE_CORE_HDR_not_defined();
    `define CHDR_WIDTH     64
    `define RFNOC_PROTOVER { 8'd1, 8'd0 }
  `endif
  localparam CHDR_W         = `CHDR_WIDTH;
  localparam RFNOC_PROTOVER = `RFNOC_PROTOVER;

  // This USRP currently only supports 64-bit CHDR width
  if (CHDR_W != 64) begin : gen_chdr_w_error
    CHDR_W_must_be_64_for_this_USRP();
  end

  // Log base 2 of the maximum transmission unit (MTU) in bytes
  localparam BYTE_MTU = $clog2(8192);

  `ifdef SFP_1GBE
    parameter PROTOCOL = "1GbE";
    parameter MDIO_EN = 1'b1;
    parameter MDIO_PHYADDR = 5'd4;
  `elsif SFP_10GBE
    parameter PROTOCOL = "10GbE";
    parameter MDIO_EN = 1'b1;
    parameter MDIO_PHYADDR = 5'd4;
  `elsif SFP_AURORA
    parameter PROTOCOL = "Aurora";
    parameter MDIO_EN = 1'b0;
    parameter MDIO_PHYADDR = 5'd0;
  `else
    parameter PROTOCOL = "Disabled";
    parameter MDIO_EN = 1'b0;
    parameter MDIO_PHYADDR = 5'd0;
  `endif

  // Constants
  localparam REG_AWIDTH = 14; // log2(0x4000)
  localparam REG_DWIDTH = 32;
  localparam DB_GPIO_WIDTH = 32;
  localparam FP_GPIO_OFFSET = 32; // Offset within ps_gpio_*
  localparam FP_GPIO_WIDTH = 8;

  //If bus_clk freq ever changes, update this parameter accordingly.
  localparam BUS_CLK_RATE = 32'd200000000; //200 MHz bus_clk rate.
  localparam SFP_PORTNUM = 8'b0; // Only one SFP port
  localparam NUM_RADIOS = 1;
  localparam NUM_CHANNELS_PER_RADIO = 2;
  localparam NUM_DBOARDS = 1;
  localparam NUM_CHANNELS = NUM_RADIOS * NUM_CHANNELS_PER_RADIO;

  // Clocks
  wire xgige_clk156;
  wire bus_clk;
  wire radio_clk;
  wire reg_clk;
  wire clk40;
  wire ddr3_dma_clk;
  wire FCLK_CLK0;
  wire FCLK_CLK1;
  wire FCLK_CLK2;
  wire FCLK_CLK3;

  // Resets
  wire global_rst;
  wire bus_rst;
  wire radio_rst;
  wire reg_rstn;
  wire clk40_rst;
  wire clk40_rstn;
  wire FCLK_RESET0_N;

  // Regport for SFP
  wire        m_axi_net_arvalid;
  wire        m_axi_net_awvalid;
  wire        m_axi_net_bready;
  wire        m_axi_net_rready;
  wire        m_axi_net_wvalid;
  wire [11:0] m_axi_net_arid;
  wire [11:0] m_axi_net_awid;
  wire [11:0] m_axi_net_wid;
  wire [31:0] m_axi_net_araddr;
  wire [31:0] m_axi_net_awaddr;
  wire [31:0] m_axi_net_wdata;
  wire [3:0]  m_axi_net_wstrb;
  wire        m_axi_net_arready;
  wire        m_axi_net_awready;
  wire        m_axi_net_bvalid;
  wire        m_axi_net_rlast;
  wire        m_axi_net_rvalid;
  wire        m_axi_net_wready;
  wire [1:0]  m_axi_net_bresp;
  wire [1:0]  m_axi_net_rresp;
  wire [31:0] m_axi_net_rdata;

  // Crossbar
  wire        m_axi_xbar_arvalid;
  wire        m_axi_xbar_awvalid;
  wire        m_axi_xbar_bready;
  wire        m_axi_xbar_rready;
  wire        m_axi_xbar_wvalid;
  wire [11:0] m_axi_xbar_arid;
  wire [11:0] m_axi_xbar_awid;
  wire [11:0] m_axi_xbar_wid;
  wire [31:0] m_axi_xbar_araddr;
  wire [31:0] m_axi_xbar_awaddr;
  wire [31:0] m_axi_xbar_wdata;
  wire [3:0]  m_axi_xbar_wstrb;
  wire        m_axi_xbar_arready;
  wire        m_axi_xbar_awready;
  wire        m_axi_xbar_bvalid;
  wire        m_axi_xbar_rlast;
  wire        m_axi_xbar_rvalid;
  wire        m_axi_xbar_wready;
  wire [1:0]  m_axi_xbar_bresp;
  wire [1:0]  m_axi_xbar_rresp;
  wire [31:0] m_axi_xbar_rdata;

  // ETH DMA
  wire        m_axi_eth_dma_arvalid;
  wire        m_axi_eth_dma_awvalid;
  wire        m_axi_eth_dma_bready;
  wire        m_axi_eth_dma_rready;
  wire        m_axi_eth_dma_wvalid;
  wire [11:0] m_axi_eth_dma_arid;
  wire [11:0] m_axi_eth_dma_awid;
  wire [11:0] m_axi_eth_dma_wid;
  wire [31:0] m_axi_eth_dma_araddr;
  wire [31:0] m_axi_eth_dma_awaddr;
  wire [31:0] m_axi_eth_dma_wdata;
  wire [3:0]  m_axi_eth_dma_wstrb;
  wire        m_axi_eth_dma_arready;
  wire        m_axi_eth_dma_awready;
  wire        m_axi_eth_dma_bvalid;
  wire        m_axi_eth_dma_rlast;
  wire        m_axi_eth_dma_rvalid;
  wire        m_axi_eth_dma_wready;
  wire [1:0]  m_axi_eth_dma_bresp;
  wire [1:0]  m_axi_eth_dma_rresp;
  wire [31:0] m_axi_eth_dma_rdata;
  wire        m_axi_eth_internal_arvalid;
  wire        m_axi_eth_internal_awvalid;
  wire        m_axi_eth_internal_bready;
  wire        m_axi_eth_internal_rready;
  wire        m_axi_eth_internal_wvalid;
  wire [31:0] m_axi_eth_internal_araddr;
  wire [31:0] m_axi_eth_internal_awaddr;
  wire [31:0] m_axi_eth_internal_wdata;
  wire [3:0]  m_axi_eth_internal_wstrb;
  wire        m_axi_eth_internal_arready;
  wire        m_axi_eth_internal_awready;
  wire        m_axi_eth_internal_bvalid;
  wire        m_axi_eth_internal_rvalid;
  wire        m_axi_eth_internal_wready;
  wire [1:0]  m_axi_eth_internal_bresp;
  wire [1:0]  m_axi_eth_internal_rresp;
  wire [31:0] m_axi_eth_internal_rdata;

  // Processing System
  wire [15:0] IRQ_F2P;

  // Internal Ethernet xport adapter to PS
  wire [63:0] h2e_tdata;
  wire [7:0]  h2e_tkeep;
  wire        h2e_tlast;
  wire        h2e_tready;
  wire        h2e_tvalid;

  wire [63:0] e2h_tdata;
  wire [7:0]  e2h_tkeep;
  wire        e2h_tlast;
  wire        e2h_tready;
  wire        e2h_tvalid;

  wire [63:0] m_axis_dma_tdata;
  wire        m_axis_dma_tlast;
  wire        m_axis_dma_tready;
  wire        m_axis_dma_tvalid;

  wire [63:0] s_axis_dma_tdata;
  wire        s_axis_dma_tlast;
  wire        s_axis_dma_tready;
  wire        s_axis_dma_tvalid;

  // HP0 -- High Performance port 0
  wire [31:0] s_axi_hp0_awaddr;
  wire [2:0]  s_axi_hp0_awprot;
  wire        s_axi_hp0_awvalid;
  wire        s_axi_hp0_awready;
  wire [63:0] s_axi_hp0_wdata;
  wire [7:0]  s_axi_hp0_wstrb;
  wire        s_axi_hp0_wvalid;
  wire        s_axi_hp0_wready;
  wire [1:0]  s_axi_hp0_bresp;
  wire        s_axi_hp0_bvalid;
  wire        s_axi_hp0_bready;
  wire [31:0] s_axi_hp0_araddr;
  wire [2:0]  s_axi_hp0_arprot;
  wire        s_axi_hp0_arvalid;
  wire        s_axi_hp0_arready;
  wire [63:0] s_axi_hp0_rdata;
  wire [1:0]  s_axi_hp0_rresp;
  wire        s_axi_hp0_rvalid;
  wire        s_axi_hp0_rready;
  wire        s_axi_hp0_rlast;
  wire [3:0]  s_axi_hp0_arcache;
  wire [7:0]  s_axi_hp0_awlen;
  wire [2:0]  s_axi_hp0_awsize;
  wire [1:0]  s_axi_hp0_awburst;
  wire [3:0]  s_axi_hp0_awcache;
  wire        s_axi_hp0_wlast;
  wire [7:0]  s_axi_hp0_arlen;
  wire [1:0]  s_axi_hp0_arburst;
  wire [2:0]  s_axi_hp0_arsize;

  wire [31:0] s_axi_eth_descriptor_awaddr;
  wire [2:0]  s_axi_eth_descriptor_awprot;
  wire        s_axi_eth_descriptor_awvalid;
  wire        s_axi_eth_descriptor_awready;
  wire [31:0] s_axi_eth_descriptor_wdata;
  wire [3:0]  s_axi_eth_descriptor_wstrb;
  wire        s_axi_eth_descriptor_wvalid;
  wire        s_axi_eth_descriptor_wready;
  wire [1:0]  s_axi_eth_descriptor_bresp;
  wire        s_axi_eth_descriptor_bvalid;
  wire        s_axi_eth_descriptor_bready;
  wire [31:0] s_axi_eth_descriptor_araddr;
  wire [2:0]  s_axi_eth_descriptor_arprot;
  wire        s_axi_eth_descriptor_arvalid;
  wire        s_axi_eth_descriptor_arready;
  wire [31:0] s_axi_eth_descriptor_rdata;
  wire [1:0]  s_axi_eth_descriptor_rresp;
  wire        s_axi_eth_descriptor_rvalid;
  wire        s_axi_eth_descriptor_rready;
  wire        s_axi_eth_descriptor_rlast;
  wire [3:0]  s_axi_eth_descriptor_arcache;
  wire [7:0]  s_axi_eth_descriptor_awlen;
  wire [2:0]  s_axi_eth_descriptor_awsize;
  wire [1:0]  s_axi_eth_descriptor_awburst;
  wire [3:0]  s_axi_eth_descriptor_awcache;
  wire        s_axi_eth_descriptor_wlast;
  wire [7:0]  s_axi_eth_descriptor_arlen;
  wire [1:0]  s_axi_eth_descriptor_arburst;
  wire [2:0]  s_axi_eth_descriptor_arsize;

  // ARM ethernet dma clock crossing
  wire [63:0] arm_eth_tx_tdata;
  wire        arm_eth_tx_tvalid;
  wire        arm_eth_tx_tlast;
  wire        arm_eth_tx_tready;
  wire [3:0]  arm_eth_tx_tuser;
  wire [7:0]  arm_eth_tx_tkeep;

  wire [63:0] arm_eth_tx_tdata_b;
  wire        arm_eth_tx_tvalid_b;
  wire        arm_eth_tx_tlast_b;
  wire        arm_eth_tx_tready_b;
  wire [3:0]  arm_eth_tx_tuser_b;
  wire [7:0]  arm_eth_tx_tkeep_b;

  wire [63:0] arm_eth_rx_tdata;
  wire        arm_eth_rx_tvalid;
  wire        arm_eth_rx_tlast;
  wire        arm_eth_rx_tready;
  wire [3:0]  arm_eth_rx_tuser;
  wire [7:0]  arm_eth_rx_tkeep;

  wire [63:0] arm_eth_rx_tdata_b;
  wire        arm_eth_rx_tvalid_b;
  wire        arm_eth_rx_tlast_b;
  wire        arm_eth_rx_tready_b;
  wire [3:0]  arm_eth_rx_tuser_b;
  wire [7:0]  arm_eth_rx_tkeep_b;

  wire        arm_eth_rx_irq;
  wire        arm_eth_tx_irq;

  // Vita to Ethernet
  wire [63:0] v2e_tdata;
  wire        v2e_tlast;
  wire        v2e_tvalid;
  wire        v2e_tready;

  // Ethernet to Vita
  wire [63:0] e2v_tdata;
  wire        e2v_tlast;
  wire        e2v_tvalid;
  wire        e2v_tready;

  // Misc
  wire [31:0] sfp_port_info;
  wire        sfp_link_up;
  wire [15:0] device_id;
  wire        clocks_locked;

  /////////////////////////////////////////////////////////////////////
  //
  // Resets:
  //  - PL - Global Reset --> Bus Reset
  //                      --> Radio Reset
  //  - PS - FCLK_RESET0_N --> clk40_rst(n)
  //
  //////////////////////////////////////////////////////////////////////

  // Global synchronous reset, on the bus_clk domain. De-asserts after 85
  // bus_clk cycles. Asserted by default.
  por_gen por_gen (
    .clk(bus_clk),
    .reset_out(global_rst)
  );

  // Synchronous reset for the radio_clk domain
  reset_sync radio_reset_gen (
    .clk(radio_clk),
    .reset_in(~clocks_locked),
    .reset_out(radio_rst)
  );

  // Synchronous reset for the bus_clk domain
  reset_sync bus_reset_gen (
    .clk(bus_clk),
    .reset_in(~clocks_locked),
    .reset_out(bus_rst)
  );


  // PS-based Resets //
  //
  // Synchronous reset for the clk40 domain. This is derived from the PS reset 0.
  reset_sync clk40_reset_gen (
    .clk(clk40),
    .reset_in(~FCLK_RESET0_N),
    .reset_out(clk40_rst)
  );
  // Invert for various modules.
  assign clk40_rstn = ~clk40_rst;
  assign reg_rstn = clk40_rstn;

  /////////////////////////////////////////////////////////////////////
  //
  // Clocks and PPS
  //
  /////////////////////////////////////////////////////////////////////

  wire pps_refclk;
  wire [1:0] pps_select;
  wire ref_select;
  wire refclk_locked_busclk;

  assign clk40   = FCLK_CLK1;   // 40 MHz
  assign bus_clk = FCLK_CLK3;   // 200 MHz
  assign reg_clk = clk40;

  e320_clocking e320_clocking_i (
    .global_rst(global_rst),
    .ref_clk_from_pin(CLK_REF_RAW),
    .ref_clk(),
    .clk156(xgige_clk156),
    .ddr3_dma_clk(ddr3_dma_clk),
    .clocks_locked(clocks_locked),
    .ext_pps_from_pin(CLK_SYNC_EXT),
    .gps_pps_from_pin(CLK_SYNC_INT),
    .pps_select(pps_select),
    .pps_refclk(pps_refclk)
  );

  assign CLK_REF_SEL = ref_select;

  synchronizer synchronize_rf_clk_lock (
    .clk(bus_clk), .rst(1'b0), .in(CLK_MUX_OUT), .out(refclk_locked_busclk)
  );

  /////////////////////////////////////////////////////////////////////
  //
  // PL DDR3 Memory Interface
  //
  /////////////////////////////////////////////////////////////////////

  wire         ddr3_axi_clk;           // 1/4 DDR external clock rate
  wire         ddr3_axi_rst;           // Synchronized to ddr_sys_clk
  wire         ddr3_running;           // DRAM calibration complete.
  wire [11:0]  device_temp;

  // Slave Interface Write Address Ports
  wire [3:0]   ddr3_axi_awid;
  wire [31:0]  ddr3_axi_awaddr;
  wire [7:0]   ddr3_axi_awlen;
  wire [2:0]   ddr3_axi_awsize;
  wire [1:0]   ddr3_axi_awburst;
  wire [0:0]   ddr3_axi_awlock;
  wire [3:0]   ddr3_axi_awcache;
  wire [2:0]   ddr3_axi_awprot;
  wire [3:0]   ddr3_axi_awqos;
  wire         ddr3_axi_awvalid;
  wire         ddr3_axi_awready;
  // Slave Interface Write Data Ports
  wire [255:0] ddr3_axi_wdata;
  wire [31:0]  ddr3_axi_wstrb;
  wire         ddr3_axi_wlast;
  wire         ddr3_axi_wvalid;
  wire         ddr3_axi_wready;
  // Slave Interface Write Response Ports
  wire         ddr3_axi_bready;
  wire [3:0]   ddr3_axi_bid;
  wire [1:0]   ddr3_axi_bresp;
  wire         ddr3_axi_bvalid;
  // Slave Interface Read Address Ports
  wire [3:0]   ddr3_axi_arid;
  wire [31:0]  ddr3_axi_araddr;
  wire [7:0]   ddr3_axi_arlen;
  wire [2:0]   ddr3_axi_arsize;
  wire [1:0]   ddr3_axi_arburst;
  wire [0:0]   ddr3_axi_arlock;
  wire [3:0]   ddr3_axi_arcache;
  wire [2:0]   ddr3_axi_arprot;
  wire [3:0]   ddr3_axi_arqos;
  wire         ddr3_axi_arvalid;
  wire         ddr3_axi_arready;
  // Slave Interface Read Data Ports
  wire         ddr3_axi_rready;
  wire [3:0]   ddr3_axi_rid;
  wire [255:0] ddr3_axi_rdata;
  wire [1:0]   ddr3_axi_rresp;
  wire         ddr3_axi_rlast;
  wire         ddr3_axi_rvalid;

  reg      ddr3_axi_rst_reg_n;

  // Copied this reset circuit from example design.
  always @(posedge ddr3_axi_clk)
    ddr3_axi_rst_reg_n <= ~ddr3_axi_rst;

  ddr3_32bit u_ddr3_32bit (
    // Memory interface ports
    .ddr3_addr           (ddr3_addr),
    .ddr3_ba             (ddr3_ba),
    .ddr3_cas_n          (ddr3_cas_n),
    .ddr3_ck_n           (ddr3_ck_n),
    .ddr3_ck_p           (ddr3_ck_p),
    .ddr3_cke            (ddr3_cke),
    .ddr3_ras_n          (ddr3_ras_n),
    .ddr3_reset_n        (ddr3_reset_n),
    .ddr3_we_n           (ddr3_we_n),
    .ddr3_dq             (ddr3_dq),
    .ddr3_dqs_n          (ddr3_dqs_n),
    .ddr3_dqs_p          (ddr3_dqs_p),
    .init_calib_complete (ddr3_running),
    .device_temp_i       (device_temp),
    .ddr3_cs_n           (ddr3_cs_n),
    .ddr3_dm             (ddr3_dm),
    .ddr3_odt            (ddr3_odt),
    // Application interface ports
    .ui_clk              (ddr3_axi_clk),
    .ui_clk_sync_rst     (ddr3_axi_rst),
    .aresetn             (ddr3_axi_rst_reg_n),
    .app_sr_req          (1'b0),
    .app_sr_active       (),
    .app_ref_req         (1'b0),
    .app_ref_ack         (),
    .app_zq_req          (1'b0),
    .app_zq_ack          (),
    // Slave Interface Write Address Ports
    .s_axi_awid          (ddr3_axi_awid),
    .s_axi_awaddr        (ddr3_axi_awaddr[30:0]),
    .s_axi_awlen         (ddr3_axi_awlen),
    .s_axi_awsize        (ddr3_axi_awsize),
    .s_axi_awburst       (ddr3_axi_awburst),
    .s_axi_awlock        (ddr3_axi_awlock),
    .s_axi_awcache       (ddr3_axi_awcache),
    .s_axi_awprot        (ddr3_axi_awprot),
    .s_axi_awqos         (ddr3_axi_awqos),
    .s_axi_awvalid       (ddr3_axi_awvalid),
    .s_axi_awready       (ddr3_axi_awready),
    // Slave Interface Write Data Ports
    .s_axi_wdata         (ddr3_axi_wdata),
    .s_axi_wstrb         (ddr3_axi_wstrb),
    .s_axi_wlast         (ddr3_axi_wlast),
    .s_axi_wvalid        (ddr3_axi_wvalid),
    .s_axi_wready        (ddr3_axi_wready),
    // Slave Interface Write Response Ports
    .s_axi_bid           (ddr3_axi_bid),
    .s_axi_bresp         (ddr3_axi_bresp),
    .s_axi_bvalid        (ddr3_axi_bvalid),
    .s_axi_bready        (ddr3_axi_bready),
    // Slave Interface Read Address Ports
    .s_axi_arid          (ddr3_axi_arid),
    .s_axi_araddr        (ddr3_axi_araddr[30:0]),
    .s_axi_arlen         (ddr3_axi_arlen),
    .s_axi_arsize        (ddr3_axi_arsize),
    .s_axi_arburst       (ddr3_axi_arburst),
    .s_axi_arlock        (ddr3_axi_arlock),
    .s_axi_arcache       (ddr3_axi_arcache),
    .s_axi_arprot        (ddr3_axi_arprot),
    .s_axi_arqos         (ddr3_axi_arqos),
    .s_axi_arvalid       (ddr3_axi_arvalid),
    .s_axi_arready       (ddr3_axi_arready),
    // Slave Interface Read Data Ports
    .s_axi_rid           (ddr3_axi_rid),
    .s_axi_rdata         (ddr3_axi_rdata),
    .s_axi_rresp         (ddr3_axi_rresp),
    .s_axi_rlast         (ddr3_axi_rlast),
    .s_axi_rvalid        (ddr3_axi_rvalid),
    .s_axi_rready        (ddr3_axi_rready),
    // System Clock Ports
    .sys_clk_p           (sys_clk_p),
    .sys_clk_n           (sys_clk_n),
    .clk_ref_i           (bus_clk),
    .sys_rst             (bus_rst)
  );

  // Temperature monitor module
  mig_7series_v4_2_tempmon #(
    .TEMP_MON_CONTROL ("INTERNAL"),
    .XADC_CLK_PERIOD  (5000)        // In ps, should match xadc_clk period
  ) tempmon_i (
    .clk           (bus_clk),
    .xadc_clk      (bus_clk),
    .rst           (bus_rst),
    .device_temp_i (12'd0),         // Not used for "INTERNAL"
    .device_temp   (device_temp)
  );


  /////////////////////////////////////////////////////////////////////
  //
  // Front-Panel GPIO
  //
  /////////////////////////////////////////////////////////////////////

  wire [31:0] fp_gpio_ctrl;
  wire [FP_GPIO_WIDTH-1:0] fp_gpio_in;
  wire [FP_GPIO_WIDTH-1:0] fp_gpio_out;
  wire [FP_GPIO_WIDTH-1:0] fp_gpio_tri;

  // Turn on GPIO buffers
  assign GPIO_OE_N = fp_gpio_ctrl[0];

  // Enable GPIO supply
  assign EN_GPIO_VAR_SUPPLY = fp_gpio_ctrl[1];

  // GPIO Voltage (1.8V, 2.5V, or 3.3V)
  //
  // 3V3 2V5 | Voltage
  // -----------------
  //  0   0  | 1.8 V
  //  0   1  | 2.5 V
  //  1   0  | 3.3 V
  assign EN_GPIO_2V5 = fp_gpio_ctrl[2];
  assign EN_GPIO_3V3 = fp_gpio_ctrl[3];

  genvar i;
  generate
    for (i = 0; i < FP_GPIO_WIDTH; i = i+1) begin : gen_gpio_iobuf
      assign fp_gpio_in[i]   = GPIO_PREBUFF[i];
      assign GPIO_PREBUFF[i] = fp_gpio_tri[i] ? 1'bZ : fp_gpio_out[i];
      assign GPIO_DIR[i]     = ~fp_gpio_tri[i];
    end
  endgenerate


  /////////////////////////////////////////////////////////////////////
  //
  // GPIO Interface
  //  - Control Filter Banks
  //  - LEDs
  //
  /////////////////////////////////////////////////////////////////////

  // Flattened Radio GPIO control
  wire [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_out_flat;
  wire [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_ddr_flat;
  wire [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_in_flat;
  wire [32*NUM_CHANNELS-1:0] leds_flat;

  // Radio GPIO control
  wire [DB_GPIO_WIDTH-1:0] db_gpio_in[0:NUM_CHANNELS-1];
  wire [DB_GPIO_WIDTH-1:0] db_gpio_out[0:NUM_CHANNELS-1];
  wire [DB_GPIO_WIDTH-1:0] db_gpio_ddr[0:NUM_CHANNELS-1];
  wire [DB_GPIO_WIDTH-1:0] db_gpio_pins[0:NUM_CHANNELS-1];
  wire [31:0] leds[0:NUM_CHANNELS-1];

  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin

      assign db_gpio_in_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH] = db_gpio_in[i];
      assign db_gpio_out[i] = db_gpio_out_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH];
      assign db_gpio_ddr[i] = db_gpio_ddr_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH];
      assign leds[i] = leds_flat[32*i +: 32];

      gpio_atr_io #(
        .WIDTH(DB_GPIO_WIDTH)
      ) gpio_atr_db_inst (
        .clk(radio_clk),
        .gpio_pins(db_gpio_pins[i]),
        .gpio_ddr(db_gpio_ddr[i]),
        .gpio_out(db_gpio_out[i]),
        .gpio_in(db_gpio_in[i])
      );
    end
  endgenerate

  // DB_GPIO and LED pin assignments with software mapping

  // Channel 1
  assign {TX_HFAMP1_ENA, // HF TX AMP
          TX_LFAMP1_ENA, // LF TX AMP
          FE1_SEL[2:0],  // TRX Switch
          TX1_BSEL[5:3], // TX_SW2
          TX1_BSEL[2:0], // TX_SW1
          RX1_SEL[1:0],  // RX_SW3
          RX1_BSEL[5:3], // RX_SW2
          RX1_BSEL[2:0]  // RX_SW1
         } = db_gpio_pins[0];

  assign {RX1_GRN_ENA,  //TX/RX led
          TX1_RED_ENA,  //TX/RX led
          TXRX1_GRN_ENA //RX2 led
         } = leds[0];

  // Channel 2
  assign {TX_HFAMP2_ENA, // HF TX AMP
          TX_LFAMP2_ENA, // LF TX AMP
          FE2_SEL[2:0],  // TRX Switch
          TX2_BSEL[5:3], // TX_SW2
          TX2_BSEL[2:0], // TX_SW1
          RX2_SEL[1:0],  // RX_SW3
          RX2_BSEL[5:3], // RX_SW2
          RX2_BSEL[2:0]  // RX_SW1
         } = db_gpio_pins[1];

  assign {RX2_GRN_ENA,
          TX2_RED_ENA,
          TXRX2_GRN_ENA
         } = leds[1];

  /////////////////////////////////////////////////////////////////////
  //
  // 5V Power Supply (TX Amplifier)
  //
  /////////////////////////////////////////////////////////////////////

  assign ENA_PAPWR = 1'b1;

  /////////////////////////////////////////////////////////////////////
  //
  // AD9361 Interface
  //
  /////////////////////////////////////////////////////////////////////

  wire [NUM_CHANNELS*32-1:0] rx_flat, tx_flat;

  wire [11:0] rx_i0, rx_q0, tx_i0, tx_q0;
  wire [11:0] rx_i1, rx_q1, tx_i1, tx_q1;

  wire [NUM_CHANNELS-1:0] rx_stb, tx_stb;
  wire [NUM_CHANNELS-1:0] rx_atr, tx_atr;

  wire [REG_DWIDTH-1:0] dboard_ctrl;
  wire [REG_DWIDTH-1:0] dboard_status;
  wire mimo_busclk, mimo_radioclk;
  wire tx_chan_sel_busclk, tx_chan_sel_radioclk;

  wire tx_pll_lock_busclk, rx_pll_lock_busclk;

  synchronizer synchronizer_tx_pll_lock (
    .clk(bus_clk), .rst(1'b0), .in(XCVR_CTRL_OUT[7]), .out(tx_pll_lock_busclk)
  );

  synchronizer synchronizer_rx_pll_lock (
    .clk(bus_clk), .rst(1'b0), .in(XCVR_CTRL_OUT[6]), .out(rx_pll_lock_busclk)
  );

  assign dboard_status = {
    24'b0,
    tx_pll_lock_busclk,   // TX PLL Lock
    rx_pll_lock_busclk,   // RX PLL Lock
    6'b0
  };

  assign mimo_busclk = dboard_ctrl[0];
  assign tx_chan_sel_busclk = dboard_ctrl[1];

  synchronizer synchronizer_mimo_radioclk (
    .clk(radio_clk), .rst(1'b0), .in(mimo_busclk), .out(mimo_radioclk)
  );

  synchronizer synchronizer_tx_chan_sel_radioclk (
    .clk(radio_clk), .rst(1'b0), .in(tx_chan_sel_busclk), .out(tx_chan_sel_radioclk)
  );

  assign rx_flat = {rx_i1, 4'b0, rx_q1, 4'b0,
                    rx_i0, 4'b0, rx_q0, 4'b0};

  assign tx_q0 = tx_flat[15:4];
  assign tx_i0 = tx_flat[31:20];
  assign tx_q1 = tx_flat[47:36];
  assign tx_i1 = tx_flat[63:52];

  // Tx and Rx have samples on every clock, so keep stb asserted
  assign rx_stb = { 1'b1, 1'b1 };
  assign tx_stb = { 1'b1, 1'b1 };

  // These delays depend on the internal clock routing delays of the FPGA.
  // Valid timing constraints are required to confirm that setup/hold are met
  // at both the input and output interfaces.
  localparam INPUT_DATA_DELAY   = 27;
  localparam OUTPUT_DATA_DELAY  = 19;

  assign XCVR_ENABLE = 1'b1;
  assign XCVR_SYNC = 1'b0;
  assign XCVR_TXNRX = 1'b1;
  assign XCVR_ENA_AGC = 1'b1;
  assign XCVR_RESET_N = ~bus_rst;

  cat_io_lvds_dual_mode #(
    .INVERT_FRAME_RX    (0),
    .INVERT_DATA_RX     (6'b00_0000),
    .INVERT_FRAME_TX    (0),
    .INVERT_DATA_TX     (6'b00_0000),
    .USE_CLOCK_IDELAY   (0),
    .USE_DATA_IDELAY    (1),
    .DATA_IDELAY_MODE   ("FIXED"),
    .CLOCK_IDELAY_MODE  ("FIXED"),
    .INPUT_CLOCK_DELAY  (0),
    .INPUT_DATA_DELAY   (INPUT_DATA_DELAY),
    .USE_CLOCK_ODELAY   (1),
    .USE_DATA_ODELAY    (1),
    .DATA_ODELAY_MODE   ("FIXED"),
    .CLOCK_ODELAY_MODE  ("FIXED"),
    .OUTPUT_CLOCK_DELAY (0),
    .OUTPUT_DATA_DELAY  (OUTPUT_DATA_DELAY)
  ) cat_io_lvds_dual_mode_i0 (
    .clk200 (bus_clk),    // 200 MHz clock

    // Data and frame timing
    .a_mimo  (mimo_busclk),
    .a_tx_ch (tx_chan_sel_busclk),

    // Delay control interface (not used)
    .ctrl_clk               (bus_clk),
    //
    .ctrl_in_data_delay     (5'b00000),
    .ctrl_in_clk_delay      (5'b00000),
    .ctrl_ld_in_data_delay  (1'b0),
    .ctrl_ld_in_clk_delay   (1'b0),
    //
    .ctrl_out_data_delay    (5'b00000),
    .ctrl_out_clk_delay     (5'b00000),
    .ctrl_ld_out_data_delay (1'b0),
    .ctrl_ld_out_clk_delay  (1'b0),

    // Sample interface
    .radio_rst    (radio_rst),
    .radio_clk    (radio_clk),
    //
    .rx_aligned   (),
    .rx_i0        (rx_i0),
    .rx_q0        (rx_q0),
    .rx_i1        (rx_i1),
    .rx_q1        (rx_q1),
    //
    .tx_i0        (tx_i0),
    .tx_q0        (tx_q0),
    .tx_i1        (tx_i1),
    .tx_q1        (tx_q1),

    // AD9361 interface
    .rx_clk_p   (RX_CLK_P),
    .rx_clk_n   (RX_CLK_N),
    .rx_frame_p (RX_FRAME_P),
    .rx_frame_n (RX_FRAME_N),
    .rx_d_p     (RX_DATA_P),
    .rx_d_n     (RX_DATA_N),
    //
    .tx_clk_p   (TX_CLK_P),
    .tx_clk_n   (TX_CLK_N),
    .tx_frame_p (TX_FRAME_P),
    .tx_frame_n (TX_FRAME_N),
    .tx_d_p     (TX_DATA_P),
    .tx_d_n     (TX_DATA_N)
  );

  /////////////////////////////////////////////////////////////////////
  //
  // SFP Connections:
  //   - 1G
  //   - 10G
  //   - Aurora
  //
  //////////////////////////////////////////////////////////////////////

  //--------------------------------------------------------------
  // SFP Reference Clocks:
  // 1G requires 125 MHz reference clock
  //--------------------------------------------------------------

  wire gige_refclk;
  wire gige_refclk_bufg;

  // dont_touch required for good SI on clock
  (* dont_touch = "true" *) IBUFDS_GTE2 gige_refclk_ibuf (
    .ODIV2(),
    .CEB  (1'b0),
    .I (CLK_MGT_125M_P),
    .IB(CLK_MGT_125M_N),
    .O (gige_refclk)
  );

  BUFG bufg_gige_refclk_i (
    .I(gige_refclk),
    .O(gige_refclk_bufg)
  );

  //--------------------------------------------------------------
  // SFP Reference Clocks:
  // XG requires 156.25 MHz reference clock
  //--------------------------------------------------------------

  wire xgige_refclk;
  wire xgige_dclk;

  // dont_touch required for good SI on clock
  (* dont_touch = "true" *) IBUFDS_GTE2 ten_gige_refclk_ibuf (
    .ODIV2(),
    .CEB  (1'b0),
    .I (CLK_MGT_156_25M_P),
    .IB(CLK_MGT_156_25M_N),
    .O (xgige_refclk)
  );

  ten_gige_phy_clk_gen xgige_clk_gen_i (
    .refclk_ibuf(xgige_refclk),
    .clk156(xgige_clk156),
    .dclk(xgige_dclk)
  );

  //--------------------------------------------------------------
  // SFP Reference Clocks:
  // XG requires 156.25 MHz reference clock
  //--------------------------------------------------------------

  wire aurora_refclk;
  wire aurora_clk156;
  wire aurora_init_clk;

  // Use the 156.25MHz reference clock for Aurora
  assign aurora_refclk = xgige_refclk;
  assign aurora_clk156 = xgige_clk156;
  assign aurora_init_clk = xgige_dclk;

  wire sfp_gt_refclk;
  wire sfp_gb_refclk;
  wire sfp_misc_clk;

  // Make SFP1_RS1 open drain to avoid a short circuit when it is connected to
  // ground by the SFP module (per the SFP+ specification).
  wire SFP1_RS1_t;
  assign SFP1_RS1 = SFP1_RS1_t ? 1'bZ : 1'b0;

  // Select Reference Clock according to Protocol
  generate
    if (PROTOCOL == "10GbE") begin

      assign sfp_gt_refclk = xgige_refclk;
      assign sfp_gb_refclk = xgige_clk156;
      assign sfp_misc_clk  = xgige_dclk;
      assign SFP1_RS0      = 1'b1;
      assign SFP1_RS1_t    = 1'b1;

    end else if (PROTOCOL == "1GbE") begin

      assign sfp_gt_refclk = gige_refclk;
      assign sfp_gb_refclk = gige_refclk_bufg;
      assign sfp_misc_clk  = gige_refclk_bufg;
      assign SFP1_RS0      = 1'b0;
      assign SFP1_RS1_t    = 1'b0;

    end else if (PROTOCOL == "Aurora") begin

      assign sfp_gt_refclk = aurora_refclk;
      assign sfp_gb_refclk = aurora_clk156;
      assign sfp_misc_clk  = aurora_init_clk;
      assign SFP1_RS0      = 1'b1;
      assign SFP1_RS1_t    = 1'b1;

    end else begin

      assign sfp_gt_refclk = 1'b0;
      assign sfp_gb_refclk = 1'b0;
      assign sfp_misc_clk  = 1'b0;
      assign SFP1_RS0      = 1'b0;
      assign SFP1_RS1_t    = 1'b0;

    end
  endgenerate

  /////////////////////////////////////////////////////////////////////
  //
  // SFP Wrapper: All protocols (1G/XG/AA) + eth_switch
  //
  /////////////////////////////////////////////////////////////////////

  e320_sfp_wrapper #(
    .PROTOCOL(PROTOCOL),
    .DWIDTH(REG_DWIDTH),     // Width of the AXI4-Lite data bus (must be 32 or 64)
    .AWIDTH(REG_AWIDTH),     // Width of the address bus
    .PORTNUM(SFP_PORTNUM),
    .MDIO_EN(MDIO_EN),
    .MDIO_PHYADDR(MDIO_PHYADDR),
    .BYTE_MTU(BYTE_MTU),
    .RFNOC_PROTOVER(RFNOC_PROTOVER),
    .NODE_INST(0)
  ) sfp_wrapper_i (
    // Resets
    .areset(bus_rst),
    .bus_rst(bus_rst),
    // Clocks
    .gt_refclk(sfp_gt_refclk),
    .gb_refclk(sfp_gb_refclk),
    .misc_clk(sfp_misc_clk),
    .bus_clk(bus_clk),
    // AXI4-Lite: Clock and reset
    .s_axi_aclk(reg_clk),
    .s_axi_aresetn(reg_rstn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(m_axi_net_awaddr[REG_AWIDTH-1:0]),
    .s_axi_awvalid(m_axi_net_awvalid),
    .s_axi_awready(m_axi_net_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(m_axi_net_wdata),
    .s_axi_wstrb(m_axi_net_wstrb),
    .s_axi_wvalid(m_axi_net_wvalid),
    .s_axi_wready(m_axi_net_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(m_axi_net_bresp),
    .s_axi_bvalid(m_axi_net_bvalid),
    .s_axi_bready(m_axi_net_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(m_axi_net_araddr[REG_AWIDTH-1:0]),
    .s_axi_arvalid(m_axi_net_arvalid),
    .s_axi_arready(m_axi_net_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata(m_axi_net_rdata),
    .s_axi_rresp(m_axi_net_rresp),
    .s_axi_rvalid(m_axi_net_rvalid),
    .s_axi_rready(m_axi_net_rready),
    // SFP high-speed IO
    .txp(SFP1_TX_P),
    .txn(SFP1_TX_N),
    .rxp(SFP1_RX_P),
    .rxn(SFP1_RX_N),
    // SFP low-speed IO
    .sfpp_present_n(1'b0),
    .sfpp_rxlos(SFP1_RXLOS),
    .sfpp_tx_fault(SFP1_TXFAULT),
    .sfpp_tx_disable(SFP1_TXDISABLE),
    // GT Common
    .qpllrefclklost(),
    .qplllock(1'b0),
    .qplloutclk(1'b0),
    .qplloutrefclk(1'b0),
    .qpllreset(),
    // Aurora MMCM
    .mmcm_locked(1'b0),
    .gt_pll_lock(),
    // Ethernet to RFNoC
    .e2v_tdata(e2v_tdata),
    .e2v_tlast(e2v_tlast),
    .e2v_tvalid(e2v_tvalid),
    .e2v_tready(e2v_tready),
    // RFNoC to Ethernet
    .v2e_tdata(v2e_tdata),
    .v2e_tlast(v2e_tlast),
    .v2e_tvalid(v2e_tvalid),
    .v2e_tready(v2e_tready),
    // Ethernet to CPU
    .e2c_tdata(arm_eth_rx_tdata_b),
    .e2c_tkeep(arm_eth_rx_tkeep_b),
    .e2c_tlast(arm_eth_rx_tlast_b),
    .e2c_tvalid(arm_eth_rx_tvalid_b),
    .e2c_tready(arm_eth_rx_tready_b),
    // CPU to Ethernet
    .c2e_tdata(arm_eth_tx_tdata_b),
    .c2e_tkeep(arm_eth_tx_tkeep_b),
    .c2e_tlast(arm_eth_tx_tlast_b),
    .c2e_tvalid(arm_eth_tx_tvalid_b),
    .c2e_tready(arm_eth_tx_tready_b),
    // Misc
    .port_info(sfp_port_info),
    .device_id(device_id),
    // LED
    .link_up(sfp_link_up),
    .activity(LED_ACT1)
  );

  assign ps_gpio_in[60] = ps_gpio_tri[60] ? sfp_link_up : ps_gpio_out[60];
  assign LED_LINK1 = sfp_link_up;

  /////////////////////////////////////////////////////////////////////
  //
  // Ethernet DMA (SFP to ARM)
  //
  //////////////////////////////////////////////////////////////////////

  assign  IRQ_F2P[0] = arm_eth_rx_irq;
  assign  IRQ_F2P[1] = arm_eth_tx_irq;

  axi_eth_dma inst_axi_eth_dma (
    .s_axi_lite_aclk(clk40),
    .m_axi_sg_aclk(clk40),
    .m_axi_mm2s_aclk(clk40),
    .m_axi_s2mm_aclk(clk40),
    .axi_resetn(clk40_rstn),

    .s_axi_lite_awaddr(m_axi_eth_dma_awaddr), //FIXME: Synthesis Warning: port width(10) doesn't match 32
    .s_axi_lite_awvalid(m_axi_eth_dma_awvalid),
    .s_axi_lite_awready(m_axi_eth_dma_awready),

    .s_axi_lite_wdata(m_axi_eth_dma_wdata),
    .s_axi_lite_wvalid(m_axi_eth_dma_wvalid),
    .s_axi_lite_wready(m_axi_eth_dma_wready),

    .s_axi_lite_bresp(m_axi_eth_dma_bresp),
    .s_axi_lite_bvalid(m_axi_eth_dma_bvalid),
    .s_axi_lite_bready(m_axi_eth_dma_bready),

    .s_axi_lite_araddr(m_axi_eth_dma_araddr), //FIXME: Synthesis Warning: port width(10) doesn't match 32
    .s_axi_lite_arvalid(m_axi_eth_dma_arvalid),
    .s_axi_lite_arready(m_axi_eth_dma_arready),

    .s_axi_lite_rdata(m_axi_eth_dma_rdata),
    .s_axi_lite_rresp(m_axi_eth_dma_rresp),
    .s_axi_lite_rvalid(m_axi_eth_dma_rvalid),
    .s_axi_lite_rready(m_axi_eth_dma_rready),

    .m_axi_sg_awaddr(s_axi_eth_descriptor_awaddr),
    .m_axi_sg_awlen(s_axi_eth_descriptor_awlen),
    .m_axi_sg_awsize(s_axi_eth_descriptor_awsize),
    .m_axi_sg_awburst(s_axi_eth_descriptor_awburst),
    .m_axi_sg_awprot(s_axi_eth_descriptor_awprot),
    .m_axi_sg_awcache(s_axi_eth_descriptor_awcache),
    .m_axi_sg_awvalid(s_axi_eth_descriptor_awvalid),
    .m_axi_sg_awready(s_axi_eth_descriptor_awready),
    .m_axi_sg_wdata(s_axi_eth_descriptor_wdata),
    .m_axi_sg_wstrb(s_axi_eth_descriptor_wstrb),
    .m_axi_sg_wlast(s_axi_eth_descriptor_wlast),
    .m_axi_sg_wvalid(s_axi_eth_descriptor_wvalid),
    .m_axi_sg_wready(s_axi_eth_descriptor_wready),
    .m_axi_sg_bresp(s_axi_eth_descriptor_bresp),
    .m_axi_sg_bvalid(s_axi_eth_descriptor_bvalid),
    .m_axi_sg_bready(s_axi_eth_descriptor_bready),
    .m_axi_sg_araddr(s_axi_eth_descriptor_araddr),
    .m_axi_sg_arlen(s_axi_eth_descriptor_arlen),
    .m_axi_sg_arsize(s_axi_eth_descriptor_arsize),
    .m_axi_sg_arburst(s_axi_eth_descriptor_arburst),
    .m_axi_sg_arprot(s_axi_eth_descriptor_arprot),
    .m_axi_sg_arcache(s_axi_eth_descriptor_arcache),
    .m_axi_sg_arvalid(s_axi_eth_descriptor_arvalid),
    .m_axi_sg_arready(s_axi_eth_descriptor_arready),
    .m_axi_sg_rdata(s_axi_eth_descriptor_rdata),
    .m_axi_sg_rresp(s_axi_eth_descriptor_rresp),
    .m_axi_sg_rlast(s_axi_eth_descriptor_rlast),
    .m_axi_sg_rvalid(s_axi_eth_descriptor_rvalid),
    .m_axi_sg_rready(s_axi_eth_descriptor_rready),

    .m_axi_mm2s_araddr(s_axi_hp0_araddr),
    .m_axi_mm2s_arlen(s_axi_hp0_arlen),
    .m_axi_mm2s_arsize(s_axi_hp0_arsize),
    .m_axi_mm2s_arburst(s_axi_hp0_arburst),
    .m_axi_mm2s_arprot(s_axi_hp0_arprot),
    .m_axi_mm2s_arcache(s_axi_hp0_arcache),
    .m_axi_mm2s_arvalid(s_axi_hp0_arvalid),
    .m_axi_mm2s_arready(s_axi_hp0_arready),
    .m_axi_mm2s_rdata(s_axi_hp0_rdata),
    .m_axi_mm2s_rresp(s_axi_hp0_rresp),
    .m_axi_mm2s_rlast(s_axi_hp0_rlast),
    .m_axi_mm2s_rvalid(s_axi_hp0_rvalid),
    .m_axi_mm2s_rready(s_axi_hp0_rready),

    .mm2s_prmry_reset_out_n(),
    .m_axis_mm2s_tdata(arm_eth_tx_tdata),
    .m_axis_mm2s_tkeep(arm_eth_tx_tkeep),
    .m_axis_mm2s_tvalid(arm_eth_tx_tvalid),
    .m_axis_mm2s_tready(arm_eth_tx_tready),
    .m_axis_mm2s_tlast(arm_eth_tx_tlast),

    .m_axi_s2mm_awaddr(s_axi_hp0_awaddr),
    .m_axi_s2mm_awlen(s_axi_hp0_awlen),
    .m_axi_s2mm_awsize(s_axi_hp0_awsize),
    .m_axi_s2mm_awburst(s_axi_hp0_awburst),
    .m_axi_s2mm_awprot(s_axi_hp0_awprot),
    .m_axi_s2mm_awcache(s_axi_hp0_awcache),
    .m_axi_s2mm_awvalid(s_axi_hp0_awvalid),
    .m_axi_s2mm_awready(s_axi_hp0_awready),
    .m_axi_s2mm_wdata(s_axi_hp0_wdata),
    .m_axi_s2mm_wstrb(s_axi_hp0_wstrb),
    .m_axi_s2mm_wlast(s_axi_hp0_wlast),
    .m_axi_s2mm_wvalid(s_axi_hp0_wvalid),
    .m_axi_s2mm_wready(s_axi_hp0_wready),
    .m_axi_s2mm_bresp(s_axi_hp0_bresp),
    .m_axi_s2mm_bvalid(s_axi_hp0_bvalid),
    .m_axi_s2mm_bready(s_axi_hp0_bready),

    .s2mm_prmry_reset_out_n(),
    .s_axis_s2mm_tdata(arm_eth_rx_tdata),
    .s_axis_s2mm_tkeep(arm_eth_rx_tkeep),
    .s_axis_s2mm_tvalid(arm_eth_rx_tvalid),
    .s_axis_s2mm_tready(arm_eth_rx_tready),
    .s_axis_s2mm_tlast(arm_eth_rx_tlast),

    .mm2s_introut(arm_eth_tx_irq),
    .s2mm_introut(arm_eth_rx_irq),
    .axi_dma_tstvec()
  );

  // Clock crossing fifo from dma(clk40) to sfp(bus_clk)
  axi_fifo_2clk #(.WIDTH(1+8+64), .SIZE(5)) eth_tx_fifo_2clk_i (
    .reset(clk40_rst),
    .i_aclk(clk40),
    .i_tdata({arm_eth_tx_tlast, arm_eth_tx_tkeep, arm_eth_tx_tdata}),
    .i_tvalid(arm_eth_tx_tvalid),
    .i_tready(arm_eth_tx_tready),
    .o_aclk(bus_clk),
    .o_tdata({arm_eth_tx_tlast_b, arm_eth_tx_tkeep_b, arm_eth_tx_tdata_b}),
    .o_tvalid(arm_eth_tx_tvalid_b),
    .o_tready(arm_eth_tx_tready_b)
  );

  // Clock crossing fifo from sfp(bus_clk) to dma(clk40)
  axi_fifo_2clk #(.WIDTH(1+8+64), .SIZE(5)) eth_rx_fifo_2clk_i (
    .reset(bus_rst),
    .i_aclk(bus_clk),
    .i_tdata({arm_eth_rx_tlast_b, arm_eth_rx_tkeep_b, arm_eth_rx_tdata_b}),
    .i_tvalid(arm_eth_rx_tvalid_b),
    .i_tready(arm_eth_rx_tready_b),
    .o_aclk(clk40),
    .o_tdata({arm_eth_rx_tlast, arm_eth_rx_tkeep, arm_eth_rx_tdata}),
    .o_tvalid(arm_eth_rx_tvalid),
    .o_tready(arm_eth_rx_tready)
  );

  /////////////////////////////////////////////////////////////////////
  //
  // Internal Ethernet Interface
  //
  //////////////////////////////////////////////////////////////////////
  eth_internal #(
    .DWIDTH         (REG_DWIDTH),
    .AWIDTH         (REG_AWIDTH),
    .PORTNUM        (8'd1),
    .BYTE_MTU       (BYTE_MTU),
    .RFNOC_PROTOVER (RFNOC_PROTOVER),
    .NODE_INST      (1)
  ) eth_internal_i (
    // Resets
    .bus_rst (bus_rst),

    // Clocks
    .bus_clk (bus_clk),

    //Axi-lite
    .s_axi_aclk     (clk40),
    .s_axi_aresetn  (clk40_rstn),
    .s_axi_awaddr   (m_axi_eth_internal_awaddr),
    .s_axi_awvalid  (m_axi_eth_internal_awvalid),
    .s_axi_awready  (m_axi_eth_internal_awready),

    .s_axi_wdata    (m_axi_eth_internal_wdata),
    .s_axi_wstrb    (m_axi_eth_internal_wstrb),
    .s_axi_wvalid   (m_axi_eth_internal_wvalid),
    .s_axi_wready   (m_axi_eth_internal_wready),

    .s_axi_bresp    (m_axi_eth_internal_bresp),
    .s_axi_bvalid   (m_axi_eth_internal_bvalid),
    .s_axi_bready   (m_axi_eth_internal_bready),

    .s_axi_araddr   (m_axi_eth_internal_araddr),
    .s_axi_arvalid  (m_axi_eth_internal_arvalid),
    .s_axi_arready  (m_axi_eth_internal_arready),

    .s_axi_rdata    (m_axi_eth_internal_rdata),
    .s_axi_rresp    (m_axi_eth_internal_rresp),
    .s_axi_rvalid   (m_axi_eth_internal_rvalid),
    .s_axi_rready   (m_axi_eth_internal_rready),

    // Host-Ethernet DMA interface
    .e2h_tdata    (e2h_tdata),
    .e2h_tkeep    (e2h_tkeep),
    .e2h_tlast    (e2h_tlast),
    .e2h_tvalid   (e2h_tvalid),
    .e2h_tready   (e2h_tready),

    .h2e_tdata    (h2e_tdata),
    .h2e_tkeep    (h2e_tkeep),
    .h2e_tlast    (h2e_tlast),
    .h2e_tvalid   (h2e_tvalid),
    .h2e_tready   (h2e_tready),

    // Vita router interface
    .e2v_tdata    (m_axis_dma_tdata),
    .e2v_tlast    (m_axis_dma_tlast),
    .e2v_tvalid   (m_axis_dma_tvalid),
    .e2v_tready   (m_axis_dma_tready),

    .v2e_tdata    (s_axis_dma_tdata),
    .v2e_tlast    (s_axis_dma_tlast),
    .v2e_tvalid   (s_axis_dma_tvalid),
    .v2e_tready   (s_axis_dma_tready),

    // MISC
    .port_info    (),
    .device_id    (device_id),

    .link_up      (),
    .activity     ()

  );




  /////////////////////////////////////////////////////////////////////
  //
  // PS Connections
  //
  //////////////////////////////////////////////////////////////////////

  wire [63:0] ps_gpio_in;
  wire [63:0] ps_gpio_out;
  wire [63:0] ps_gpio_tri;

  e320_ps_bd e320_ps_bd_i (
    // DDR Interface
    .DDR_VRN(PS_DDR3_VRN),
    .DDR_VRP(PS_DDR3_VRP),
    .DDR_addr(PS_DDR3_ADDR),
    .DDR_ba(PS_DDR3_BA),
    .DDR_cas_n(PS_DDR3_CAS_N),
    .DDR_ck_n(DDR_MODCLK_N),
    .DDR_ck_p(DDR_MODCLK_P),
    .DDR_cke(PS_DDR3_CKE),
    .DDR_cs_n(PS_DDR3_CS_N),
    .DDR_dm(PS_DDR3_DM),
    .DDR_dq(PS_DDR3_DQ),
    .DDR_dqs_n(PS_DDR3_DQS_N),
    .DDR_dqs_p(PS_DDR3_DQS_P),
    .DDR_odt(PS_DDR3_ODT),
    .DDR_ras_n(PS_DDR3_RAS_N),
    .DDR_reset_n(PS_DDR3_RESET_N),
    .DDR_we_n(PS_DDR3_WE_N),

    // Clocks
    .FCLK_CLK0(FCLK_CLK0),
    .FCLK_CLK1(FCLK_CLK1),
    .FCLK_CLK2(FCLK_CLK2),
    .FCLK_CLK3(FCLK_CLK3),

    // Resets
    .FCLK_RESET0_N(FCLK_RESET0_N),

    // GPIO
    .GPIO_0_tri_i(ps_gpio_in),
    .GPIO_0_tri_o(ps_gpio_out),
    .GPIO_0_tri_t(ps_gpio_tri),

    // Interrupts
    .IRQ_F2P(IRQ_F2P),

    // MIO
    .MIO(PS_MIO),

    .PS_CLK(PS_CLK),
    .PS_PORB(PS_POR_B),
    .PS_SRSTB(PS_SRST_B),

    // SPI
    .SPI0_MISO_I(XCVR_SPI_MISO),
    .SPI0_MISO_O(),
    .SPI0_MISO_T(),
    .SPI0_MOSI_I(1'b0),
    .SPI0_MOSI_O(XCVR_SPI_MOSI),
    .SPI0_MOSI_T(),
    .SPI0_SCLK_I(1'b0),
    .SPI0_SCLK_O(XCVR_SPI_CLK),
    .SPI0_SCLK_T(),
    .SPI0_SS1_O(),
    .SPI0_SS2_O(),
    .SPI0_SS_I(1'b1),
    .SPI0_SS_O(XCVR_SPI_CS_N),
    .SPI0_SS_T(),

    .SPI1_MISO_I(1'b0),
    .SPI1_MISO_O(),
    .SPI1_MISO_T(),
    .SPI1_MOSI_I(1'b0),
    .SPI1_MOSI_O(CLK_PLL_SDATA),
    .SPI1_MOSI_T(),
    .SPI1_SCLK_I(1'b0),
    .SPI1_SCLK_O(CLK_PLL_SCLK),
    .SPI1_SCLK_T(),
    .SPI1_SS1_O(),
    .SPI1_SS2_O(),
    .SPI1_SS_I(1'b1),
    .SPI1_SS_O(CLK_PLL_SLE),
    .SPI1_SS_T(),

    // Eth DMA Descriptor
    .s_axi_eth_descriptor_araddr(s_axi_eth_descriptor_araddr),
    .s_axi_eth_descriptor_arburst(s_axi_eth_descriptor_arburst),
    .s_axi_eth_descriptor_arcache(s_axi_eth_descriptor_arcache),
    .s_axi_eth_descriptor_arlen(s_axi_eth_descriptor_arlen),
    .s_axi_eth_descriptor_arlock(1'b0),
    .s_axi_eth_descriptor_arprot(s_axi_eth_descriptor_arprot),
    .s_axi_eth_descriptor_arqos(4'b0),
    .s_axi_eth_descriptor_arready(s_axi_eth_descriptor_arready),
    .s_axi_eth_descriptor_arsize(s_axi_eth_descriptor_arsize),
    .s_axi_eth_descriptor_arvalid(s_axi_eth_descriptor_arvalid),
    .s_axi_eth_descriptor_awaddr(s_axi_eth_descriptor_awaddr),
    .s_axi_eth_descriptor_awburst(s_axi_eth_descriptor_awburst),
    .s_axi_eth_descriptor_awcache(s_axi_eth_descriptor_awcache),
    .s_axi_eth_descriptor_awlen(s_axi_eth_descriptor_awlen),
    .s_axi_eth_descriptor_awlock(1'b0),
    .s_axi_eth_descriptor_awprot(s_axi_eth_descriptor_awprot),
    .s_axi_eth_descriptor_awqos(4'b0),
    .s_axi_eth_descriptor_awready(s_axi_eth_descriptor_awready),
    .s_axi_eth_descriptor_awsize(s_axi_eth_descriptor_awsize),
    .s_axi_eth_descriptor_awvalid(s_axi_eth_descriptor_awvalid),
    .s_axi_eth_descriptor_bready(s_axi_eth_descriptor_bready),
    .s_axi_eth_descriptor_bresp(s_axi_eth_descriptor_bresp),
    .s_axi_eth_descriptor_bvalid(s_axi_eth_descriptor_bvalid),
    .s_axi_eth_descriptor_rdata(s_axi_eth_descriptor_rdata),
    .s_axi_eth_descriptor_rlast(s_axi_eth_descriptor_rlast),
    .s_axi_eth_descriptor_rready(s_axi_eth_descriptor_rready),
    .s_axi_eth_descriptor_rresp(s_axi_eth_descriptor_rresp),
    .s_axi_eth_descriptor_rvalid(s_axi_eth_descriptor_rvalid),
    .s_axi_eth_descriptor_wdata(s_axi_eth_descriptor_wdata),
    .s_axi_eth_descriptor_wlast(s_axi_eth_descriptor_wlast),
    .s_axi_eth_descriptor_wready(s_axi_eth_descriptor_wready),
    .s_axi_eth_descriptor_wstrb(s_axi_eth_descriptor_wstrb),
    .s_axi_eth_descriptor_wvalid(s_axi_eth_descriptor_wvalid),

    // HP0 - Eth DMA
    .S_AXI_HP0_ACLK(clk40),
    .S_AXI_HP0_ARESETN(clk40_rstn),
    .S_AXI_HP0_araddr(s_axi_hp0_araddr),
    .S_AXI_HP0_arburst(s_axi_hp0_arburst),
    .S_AXI_HP0_arcache(s_axi_hp0_arcache),
    .S_AXI_HP0_arlen(s_axi_hp0_arlen),
    .S_AXI_HP0_arlock(1'b0),
    .S_AXI_HP0_arprot(s_axi_hp0_arprot),
    .S_AXI_HP0_arqos(4'b0),
    .S_AXI_HP0_arready(s_axi_hp0_arready),
    .S_AXI_HP0_arregion(4'b0),
    .S_AXI_HP0_arsize(s_axi_hp0_arsize),
    .S_AXI_HP0_arvalid(s_axi_hp0_arvalid),
    .S_AXI_HP0_awaddr(s_axi_hp0_awaddr),
    .S_AXI_HP0_awburst(s_axi_hp0_awburst),
    .S_AXI_HP0_awcache(s_axi_hp0_awcache),
    .S_AXI_HP0_awlen(s_axi_hp0_awlen),
    .S_AXI_HP0_awlock(1'b0),
    .S_AXI_HP0_awprot(s_axi_hp0_awprot),
    .S_AXI_HP0_awqos(4'b0),
    .S_AXI_HP0_awready(s_axi_hp0_awready),
    .S_AXI_HP0_awregion(4'b0),
    .S_AXI_HP0_awsize(s_axi_hp0_awsize),
    .S_AXI_HP0_awvalid(s_axi_hp0_awvalid),
    .S_AXI_HP0_bready(s_axi_hp0_bready),
    .S_AXI_HP0_bresp(s_axi_hp0_bresp),
    .S_AXI_HP0_bvalid(s_axi_hp0_bvalid),
    .S_AXI_HP0_rdata(s_axi_hp0_rdata),
    .S_AXI_HP0_rlast(s_axi_hp0_rlast),
    .S_AXI_HP0_rready(s_axi_hp0_rready),
    .S_AXI_HP0_rresp(s_axi_hp0_rresp),
    .S_AXI_HP0_rvalid(s_axi_hp0_rvalid),
    .S_AXI_HP0_wdata(s_axi_hp0_wdata),
    .S_AXI_HP0_wlast(s_axi_hp0_wlast),
    .S_AXI_HP0_wready(s_axi_hp0_wready),
    .S_AXI_HP0_wstrb(s_axi_hp0_wstrb),
    .S_AXI_HP0_wvalid(s_axi_hp0_wvalid),

    // Ethernet DMA engines
    .m_axi_eth_dma_araddr(m_axi_eth_dma_araddr),
    .m_axi_eth_dma_arprot(),
    .m_axi_eth_dma_arready(m_axi_eth_dma_arready),
    .m_axi_eth_dma_arvalid(m_axi_eth_dma_arvalid),
    .m_axi_eth_dma_awaddr(m_axi_eth_dma_awaddr),
    .m_axi_eth_dma_awprot(),
    .m_axi_eth_dma_awready(m_axi_eth_dma_awready),
    .m_axi_eth_dma_awvalid(m_axi_eth_dma_awvalid),
    .m_axi_eth_dma_bready(m_axi_eth_dma_bready),
    .m_axi_eth_dma_bresp(m_axi_eth_dma_bresp),
    .m_axi_eth_dma_bvalid(m_axi_eth_dma_bvalid),
    .m_axi_eth_dma_rdata(m_axi_eth_dma_rdata),
    .m_axi_eth_dma_rready(m_axi_eth_dma_rready),
    .m_axi_eth_dma_rresp(m_axi_eth_dma_rresp),
    .m_axi_eth_dma_rvalid(m_axi_eth_dma_rvalid),
    .m_axi_eth_dma_wdata(m_axi_eth_dma_wdata),
    .m_axi_eth_dma_wready(m_axi_eth_dma_wready),
    .m_axi_eth_dma_wstrb(m_axi_eth_dma_wstrb),
    .m_axi_eth_dma_wvalid(m_axi_eth_dma_wvalid),
    .m_axi_eth_internal_araddr(m_axi_eth_internal_araddr),
    .m_axi_eth_internal_arprot(),
    .m_axi_eth_internal_arready(m_axi_eth_internal_arready),
    .m_axi_eth_internal_arvalid(m_axi_eth_internal_arvalid),
    .m_axi_eth_internal_awaddr(m_axi_eth_internal_awaddr),
    .m_axi_eth_internal_awprot(),
    .m_axi_eth_internal_awready(m_axi_eth_internal_awready),
    .m_axi_eth_internal_awvalid(m_axi_eth_internal_awvalid),
    .m_axi_eth_internal_bready(m_axi_eth_internal_bready),
    .m_axi_eth_internal_bresp(m_axi_eth_internal_bresp),
    .m_axi_eth_internal_bvalid(m_axi_eth_internal_bvalid),
    .m_axi_eth_internal_rdata(m_axi_eth_internal_rdata),
    .m_axi_eth_internal_rready(m_axi_eth_internal_rready),
    .m_axi_eth_internal_rresp(m_axi_eth_internal_rresp),
    .m_axi_eth_internal_rvalid(m_axi_eth_internal_rvalid),
    .m_axi_eth_internal_wdata(m_axi_eth_internal_wdata),
    .m_axi_eth_internal_wready(m_axi_eth_internal_wready),
    .m_axi_eth_internal_wstrb(m_axi_eth_internal_wstrb),
    .m_axi_eth_internal_wvalid(m_axi_eth_internal_wvalid),

    // MGT IO Regport
    .m_axi_net_araddr(m_axi_net_araddr),
    .m_axi_net_arprot(),
    .m_axi_net_arready(m_axi_net_arready),
    .m_axi_net_arvalid(m_axi_net_arvalid),
    .m_axi_net_awaddr(m_axi_net_awaddr),
    .m_axi_net_awprot(),
    .m_axi_net_awready(m_axi_net_awready),
    .m_axi_net_awvalid(m_axi_net_awvalid),
    .m_axi_net_bready(m_axi_net_bready),
    .m_axi_net_bresp(m_axi_net_bresp),
    .m_axi_net_bvalid(m_axi_net_bvalid),
    .m_axi_net_rdata(m_axi_net_rdata),
    .m_axi_net_rready(m_axi_net_rready),
    .m_axi_net_rresp(m_axi_net_rresp),
    .m_axi_net_rvalid(m_axi_net_rvalid),
    .m_axi_net_wdata(m_axi_net_wdata),
    .m_axi_net_wready(m_axi_net_wready),
    .m_axi_net_wstrb(m_axi_net_wstrb),
    .m_axi_net_wvalid(m_axi_net_wvalid),

    // XBAR Regport
    .m_axi_xbar_araddr(m_axi_xbar_araddr),
    .m_axi_xbar_arprot(),
    .m_axi_xbar_arready(m_axi_xbar_arready),
    .m_axi_xbar_arvalid(m_axi_xbar_arvalid),
    .m_axi_xbar_awaddr(m_axi_xbar_awaddr),
    .m_axi_xbar_awprot(),
    .m_axi_xbar_awready(m_axi_xbar_awready),
    .m_axi_xbar_awvalid(m_axi_xbar_awvalid),
    .m_axi_xbar_bready(m_axi_xbar_bready),
    .m_axi_xbar_bresp(m_axi_xbar_bresp),
    .m_axi_xbar_bvalid(m_axi_xbar_bvalid),
    .m_axi_xbar_rdata(m_axi_xbar_rdata),
    .m_axi_xbar_rready(m_axi_xbar_rready),
    .m_axi_xbar_rresp(m_axi_xbar_rresp),
    .m_axi_xbar_rvalid(m_axi_xbar_rvalid),
    .m_axi_xbar_wdata(m_axi_xbar_wdata),
    .m_axi_xbar_wready(m_axi_xbar_wready),
    .m_axi_xbar_wstrb(m_axi_xbar_wstrb),
    .m_axi_xbar_wvalid(m_axi_xbar_wvalid),

    // USB
    .USBIND_0_port_indctl(),
    .USBIND_0_vbus_pwrfault(),
    .USBIND_0_vbus_pwrselect(),

    .bus_clk(bus_clk),
    .bus_rstn(~bus_rst),
    .clk40(clk40),
    .clk40_rstn(clk40_rstn),
    .S_AXI_GP0_ACLK(clk40),
    .S_AXI_GP0_ARESETN(clk40_rstn),

    // DMA
    .s_axis_dma_tdata(e2h_tdata),
    .s_axis_dma_tkeep(e2h_tkeep),
    .s_axis_dma_tlast(e2h_tlast),
    .s_axis_dma_tready(e2h_tready),
    .s_axis_dma_tvalid(e2h_tvalid),
    .m_axis_dma_tdata(h2e_tdata),
    .m_axis_dma_tkeep(h2e_tkeep),
    .m_axis_dma_tlast(h2e_tlast),
    .m_axis_dma_tready(h2e_tready),
    .m_axis_dma_tvalid(h2e_tvalid)
  );

  /////////////////////////////////////////////////////////////////////
  //
  // GPSDO Control and Status
  //
  /////////////////////////////////////////////////////////////////////

  wire [31:0] gps_ctrl;
  wire [31:0] gps_status;

  assign CLK_GPS_PWR_EN = gps_ctrl[0];
  assign GPS_RST_N      = gps_ctrl[1];
  assign GPS_INITSURV_N = gps_ctrl[2];
  assign gps_status[0] = GPS_LOCK;
  assign gps_status[1] = GPS_ALARM;
  assign gps_status[2] = GPS_PHASELOCK;
  assign gps_status[3] = GPS_SURVEY;
  assign gps_status[4] = GPS_WARMUP;
  assign gps_status[31:5] = 'd0;

  /////////////////////////////////////////////////////////////////////
  //
  // E320 Core:
  //   - xbar
  //   - Radio
  //   - DMA
  //   - DRAM
  //   - CEs
  //
  //////////////////////////////////////////////////////////////////////

  wire [31:0] build_datestamp;

  USR_ACCESSE2 usr_access_i (
    .DATA(build_datestamp), .CFGCLK(), .DATAVALID()
  );

  e320_core #(
    .REG_AWIDTH(REG_AWIDTH),
    .BUS_CLK_RATE(BUS_CLK_RATE),
    .NUM_RADIOS(NUM_RADIOS),
    .NUM_CHANNELS(NUM_CHANNELS),
    .NUM_DBOARDS(NUM_DBOARDS),
    .FP_GPIO_WIDTH(FP_GPIO_WIDTH),
    .DB_GPIO_WIDTH(DB_GPIO_WIDTH),
    .CHDR_W(CHDR_W),
    .BYTE_MTU(BYTE_MTU),
    .RFNOC_PROTOVER(RFNOC_PROTOVER)
  ) e320_core_i (

    //Clocks and resets
    .radio_clk(radio_clk),
    .radio_rst(radio_rst),
    .bus_clk(bus_clk),
    .bus_rst(bus_rst),
    .ddr3_dma_clk(ddr3_dma_clk),
    .clk40(clk40),

    // Clocking and PPS Controls/Indicators
    .pps_refclk(pps_refclk),
    .refclk_locked(refclk_locked_busclk),
    .pps_select(pps_select),
    .ref_select(ref_select),

    .s_axi_aclk(clk40),
    .s_axi_aresetn(clk40_rstn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(m_axi_xbar_awaddr),
    .s_axi_awvalid(m_axi_xbar_awvalid),
    .s_axi_awready(m_axi_xbar_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(m_axi_xbar_wdata),
    .s_axi_wstrb(m_axi_xbar_wstrb),
    .s_axi_wvalid(m_axi_xbar_wvalid),
    .s_axi_wready(m_axi_xbar_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(m_axi_xbar_bresp),
    .s_axi_bvalid(m_axi_xbar_bvalid),
    .s_axi_bready(m_axi_xbar_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(m_axi_xbar_araddr),
    .s_axi_arvalid(m_axi_xbar_arvalid),
    .s_axi_arready(m_axi_xbar_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata(m_axi_xbar_rdata),
    .s_axi_rresp(m_axi_xbar_rresp),
    .s_axi_rvalid(m_axi_xbar_rvalid),
    .s_axi_rready(m_axi_xbar_rready),

    // DRAM signals
    .ddr3_axi_clk              (ddr3_axi_clk),
    .ddr3_axi_rst              (ddr3_axi_rst),
    .ddr3_running              (ddr3_running),
    // Slave Interface Write Address Ports
    .ddr3_axi_awid             (ddr3_axi_awid),
    .ddr3_axi_awaddr           (ddr3_axi_awaddr),
    .ddr3_axi_awlen            (ddr3_axi_awlen),
    .ddr3_axi_awsize           (ddr3_axi_awsize),
    .ddr3_axi_awburst          (ddr3_axi_awburst),
    .ddr3_axi_awlock           (ddr3_axi_awlock),
    .ddr3_axi_awcache          (ddr3_axi_awcache),
    .ddr3_axi_awprot           (ddr3_axi_awprot),
    .ddr3_axi_awqos            (ddr3_axi_awqos),
    .ddr3_axi_awvalid          (ddr3_axi_awvalid),
    .ddr3_axi_awready          (ddr3_axi_awready),
    // Slave Interface Write Data Ports
    .ddr3_axi_wdata            (ddr3_axi_wdata),
    .ddr3_axi_wstrb            (ddr3_axi_wstrb),
    .ddr3_axi_wlast            (ddr3_axi_wlast),
    .ddr3_axi_wvalid           (ddr3_axi_wvalid),
    .ddr3_axi_wready           (ddr3_axi_wready),
    // Slave Interface Write Response Ports
    .ddr3_axi_bid              (ddr3_axi_bid),
    .ddr3_axi_bresp            (ddr3_axi_bresp),
    .ddr3_axi_bvalid           (ddr3_axi_bvalid),
    .ddr3_axi_bready           (ddr3_axi_bready),
    // Slave Interface Read Address Ports
    .ddr3_axi_arid             (ddr3_axi_arid),
    .ddr3_axi_araddr           (ddr3_axi_araddr),
    .ddr3_axi_arlen            (ddr3_axi_arlen),
    .ddr3_axi_arsize           (ddr3_axi_arsize),
    .ddr3_axi_arburst          (ddr3_axi_arburst),
    .ddr3_axi_arlock           (ddr3_axi_arlock),
    .ddr3_axi_arcache          (ddr3_axi_arcache),
    .ddr3_axi_arprot           (ddr3_axi_arprot),
    .ddr3_axi_arqos            (ddr3_axi_arqos),
    .ddr3_axi_arvalid          (ddr3_axi_arvalid),
    .ddr3_axi_arready          (ddr3_axi_arready),
    // Slave Interface Read Data Ports
    .ddr3_axi_rid              (ddr3_axi_rid),
    .ddr3_axi_rdata            (ddr3_axi_rdata),
    .ddr3_axi_rresp            (ddr3_axi_rresp),
    .ddr3_axi_rlast            (ddr3_axi_rlast),
    .ddr3_axi_rvalid           (ddr3_axi_rvalid),
    .ddr3_axi_rready           (ddr3_axi_rready),


    // Radio ATR
    .rx_atr(rx_atr),
    .tx_atr(tx_atr),

    // Front-Panel GPIO
    .fp_gpio_in(fp_gpio_in),
    .fp_gpio_tri(fp_gpio_tri),
    .fp_gpio_out(fp_gpio_out),

    // PS GPIO Connection
    .ps_gpio_tri(ps_gpio_tri[FP_GPIO_WIDTH+FP_GPIO_OFFSET-1: FP_GPIO_OFFSET]),
    .ps_gpio_out(ps_gpio_out[FP_GPIO_WIDTH+FP_GPIO_OFFSET-1: FP_GPIO_OFFSET]),
    .ps_gpio_in(ps_gpio_in[FP_GPIO_WIDTH+FP_GPIO_OFFSET-1: FP_GPIO_OFFSET]),

    // DB GPIO
    .db_gpio_out_flat(db_gpio_out_flat),
    .db_gpio_ddr_flat(db_gpio_ddr_flat),
    .db_gpio_in_flat(db_gpio_in_flat),
    .db_gpio_fab_flat(32'b0), // FIXME: Incorrect width

    // TX/RX LEDs
    .leds_flat(leds_flat),

    // Radio Strobes
    .rx_stb(rx_stb),
    .tx_stb(tx_stb),

    // Radio Data
    .rx(rx_flat),
    .tx(tx_flat),

    // Internal Ethernet DMA to PS
    .m_dma_tdata(s_axis_dma_tdata),
    .m_dma_tlast(s_axis_dma_tlast),
    .m_dma_tready(s_axis_dma_tready),
    .m_dma_tvalid(s_axis_dma_tvalid),

    .s_dma_tdata(m_axis_dma_tdata),
    .s_dma_tlast(m_axis_dma_tlast),
    .s_dma_tready(m_axis_dma_tready),
    .s_dma_tvalid(m_axis_dma_tvalid),

    // VITA to Ethernet
    .v2e_tdata(v2e_tdata),
    .v2e_tvalid(v2e_tvalid),
    .v2e_tlast(v2e_tlast),
    .v2e_tready(v2e_tready),

    // Ethernet to VITA
    .e2v_tdata(e2v_tdata),
    .e2v_tlast(e2v_tlast),
    .e2v_tvalid(e2v_tvalid),
    .e2v_tready(e2v_tready),

    .build_datestamp(build_datestamp),
    .sfp_ports_info(sfp_port_info),
    .gps_status(gps_status),
    .gps_ctrl(gps_ctrl),
    .dboard_status(dboard_status),
    .xadc_readback({20'h0, device_temp}),
    .fp_gpio_ctrl(fp_gpio_ctrl),
    .dboard_ctrl(dboard_ctrl),
    .device_id(device_id)
  );

  // Control pins to AD9361 will lay low for now
  assign XCVR_CTRL_IN = 4'h0;

endmodule // e320
`default_nettype wire
