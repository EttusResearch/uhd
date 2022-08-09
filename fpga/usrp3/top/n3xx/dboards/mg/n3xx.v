///////////////////////////////////////////////////////////////////
///
// Copyright 2016-2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: n3xx
// Description:
//   Top Level for N3xx devices
//
//////////////////////////////////////////////////////////////////////

module n3xx (
  inout [11:0] FPGA_GPIO,

  input FPGA_REFCLK_P,
  input FPGA_REFCLK_N,
  input REF_1PPS_IN,
  input NETCLK_REF_P,
  input NETCLK_REF_N,
  //input REF_1PPS_IN_MGMT,
  output REF_1PPS_OUT,

  //TDC
  inout UNUSED_PIN_TDCA_0,
  inout UNUSED_PIN_TDCA_1,
  inout UNUSED_PIN_TDCA_2,
  inout UNUSED_PIN_TDCA_3,
  inout UNUSED_PIN_TDCB_0,
  inout UNUSED_PIN_TDCB_1,
  inout UNUSED_PIN_TDCB_2,
  inout UNUSED_PIN_TDCB_3,

`ifdef NPIO_LANES
  input  NPIO_RX0_P,
  input  NPIO_RX0_N,
  output NPIO_TX0_P,
  output NPIO_TX0_N,
  input  NPIO_RX1_P,
  input  NPIO_RX1_N,
  output NPIO_TX1_P,
  output NPIO_TX1_N,
`endif
`ifdef QSFP_LANES
  input  [`QSFP_LANES-1:0] QSFP_RX_P,
  input  [`QSFP_LANES-1:0] QSFP_RX_N,
  output [`QSFP_LANES-1:0] QSFP_TX_P,
  output [`QSFP_LANES-1:0] QSFP_TX_N,
  output QSFP_RESET_B,
  output QSFP_LED,
  output QSFP_MODSEL_B,
  output QSFP_LPMODE,
  input  QSFP_PRESENT_B,
  input  QSFP_INT_B,
  inout  QSFP_I2C_SCL,
  inout  QSFP_I2C_SDA,
`endif
  //TODO: Uncomment when connected here
  //input NPIO_0_RXSYNC_0_P, NPIO_0_RXSYNC_1_P,
  //input NPIO_0_RXSYNC_0_N, NPIO_0_RXSYNC_1_N,
  //output NPIO_0_TXSYNC_0_P, NPIO_0_TXSYNC_1_P,
  //output NPIO_0_TXSYNC_0_N, NPIO_0_TXSYNC_1_N,
  //input NPIO_1_RXSYNC_0_P, NPIO_1_RXSYNC_1_P,
  //input NPIO_1_RXSYNC_0_N, NPIO_1_RXSYNC_1_N,
  //output NPIO_1_TXSYNC_0_P, NPIO_1_TXSYNC_1_P,
  //output NPIO_1_TXSYNC_0_N, NPIO_1_TXSYNC_1_N,
  //input NPIO_2_RXSYNC_0_P, NPIO_2_RXSYNC_1_P,
  //input NPIO_2_RXSYNC_0_N, NPIO_2_RXSYNC_1_N,
  //output NPIO_2_TXSYNC_0_P, NPIO_2_TXSYNC_1_P,
  //output NPIO_2_TXSYNC_0_N, NPIO_2_TXSYNC_1_N,

  //GPS
  input GPS_1PPS,
  //input GPS_1PPS_RAW,

  //Misc
  input ENET0_CLK125,
  //inout ENET0_PTP,
  //output ENET0_PTP_DIR,
  //inout ATSHA204_SDA,
  input FPGA_PL_RESETN, // TODO:  Add to reset logic
  // output reg [1:0] FPGA_TEST,
  //input PWR_CLK_FPGA, // TODO: check direction
  input FPGA_PUDC_B,

  //White Rabbit
  input WB_20MHZ_P,
  input WB_20MHZ_N,
  output WB_DAC_DIN,
  output WB_DAC_NCLR,
  output WB_DAC_NLDAC,
  output WB_DAC_NSYNC,
  output WB_DAC_SCLK,

  //LEDS
  output PANEL_LED_GPS,
  output PANEL_LED_LINK,
  output PANEL_LED_PPS,
  output PANEL_LED_REF,

  // ARM Connections (PS)
  inout [53:0]  MIO,
  inout         PS_SRSTB,
  inout         PS_CLK,
  inout         PS_PORB,
  inout         DDR_Clk,
  inout         DDR_Clk_n,
  inout         DDR_CKE,
  inout         DDR_CS_n,
  inout         DDR_RAS_n,
  inout         DDR_CAS_n,
  inout         DDR_WEB,
  inout [2:0]   DDR_BankAddr,
  inout [14:0]  DDR_Addr,
  inout         DDR_ODT,
  inout         DDR_DRSTB,
  inout [31:0]  DDR_DQ,
  inout [3:0]   DDR_DM,
  inout [3:0]   DDR_DQS,
  inout [3:0]   DDR_DQS_n,
  inout         DDR_VRP,
  inout         DDR_VRN,


  ///////////////////////////////////
  //
  // High Speed SPF+ signals and clocking
  //
  ///////////////////////////////////

  // These clock inputs must always be enabled with a buffer regardless of the build
  // target to avoid damage to the FPGA.
  input NETCLK_P,
  input NETCLK_N,
  input MGT156MHZ_CLK1_P,
  input MGT156MHZ_CLK1_N,

  input SFP_0_RX_P, input SFP_0_RX_N,
  output SFP_0_TX_P, output SFP_0_TX_N,
  input SFP_1_RX_P, input SFP_1_RX_N,
  output SFP_1_TX_P, output SFP_1_TX_N,


  ///////////////////////////////////
  //
  // DRAM Interface
  //
  ///////////////////////////////////
  inout [31:0] ddr3_dq,     // Data pins. Input for Reads, Output for Writes.
  inout [3:0] ddr3_dqs_n,   // Data Strobes. Input for Reads, Output for Writes.
  inout [3:0] ddr3_dqs_p,
  //
  output [15:0] ddr3_addr,  // Address
  output [2:0] ddr3_ba,     // Bank Address
  output ddr3_ras_n,        // Row Address Strobe.
  output ddr3_cas_n,        // Column address select
  output ddr3_we_n,         // Write Enable
  output ddr3_reset_n,      // SDRAM reset pin.
  output [0:0] ddr3_ck_p,         // Differential clock
  output [0:0] ddr3_ck_n,
  output [0:0] ddr3_cke,    // Clock Enable
  output [0:0] ddr3_cs_n,         // Chip Select
  output [3:0] ddr3_dm,     // Data Mask [3] = UDM.U26, [2] = LDM.U26, ...
  output [0:0] ddr3_odt,    // On-Die termination enable.
  //
  input sys_clk_p,          // Differential
  input sys_clk_n,          // 100MHz clock source to generate DDR3 clocking.


  ///////////////////////////////////
  //
  // Supporting I/O for SPF+ interfaces
  //  (non high speed stuff)
  //
  ///////////////////////////////////

  //SFP+ 0, Slow Speed, Bank 13 3.3V
  input SFP_0_I2C_NPRESENT,
  output SFP_0_LED_A,
  output SFP_0_LED_B,
  input SFP_0_LOS,
  output SFP_0_RS0,
  output SFP_0_RS1,
  output SFP_0_TXDISABLE,
  input SFP_0_TXFAULT,

  //SFP+ 1, Slow Speed, Bank 13 3.3V
  //input SFP_1_I2C_NPRESENT,
  output SFP_1_LED_A,
  output SFP_1_LED_B,
  input SFP_1_LOS,
  output SFP_1_RS0,
  output SFP_1_RS1,
  output SFP_1_TXDISABLE,
  input SFP_1_TXFAULT,

  //USRP IO A
  output        DBA_CPLD_PS_SPI_SCLK,
  output        DBA_CPLD_PS_SPI_LE,
  output        DBA_CPLD_PS_SPI_SDI,
  input         DBA_CPLD_PS_SPI_SDO,
  output [1:0]  DBA_CPLD_PS_SPI_ADDR,

  output        DBA_ATR_RX_1,
  output        DBA_ATR_RX_2,
  output        DBA_ATR_TX_1,
  output        DBA_ATR_TX_2,

  output [5:0]  DBA_CH1_TX_DSA_DATA,
  output [5:0]  DBA_CH1_RX_DSA_DATA,
  output [5:0]  DBA_CH2_TX_DSA_DATA,
  output [5:0]  DBA_CH2_RX_DSA_DATA,

  output        DBA_CPLD_PL_SPI_SCLK,
  output        DBA_CPLD_PL_SPI_LE,
  output        DBA_CPLD_PL_SPI_SDI,
  input         DBA_CPLD_PL_SPI_SDO,
  output [2:0]  DBA_CPLD_PL_SPI_ADDR,

  output        DBA_MYK_SPI_SCLK,
  output        DBA_MYK_SPI_CS_n,
  input         DBA_MYK_SPI_SDO,
  output        DBA_MYK_SPI_SDIO,
  input         DBA_MYK_INTRQ,

  output        DBA_MYK_SYNC_IN_n,
  input         DBA_MYK_SYNC_OUT_n,

  inout         DBA_CPLD_JTAG_TCK,
  inout         DBA_CPLD_JTAG_TMS,
  inout         DBA_CPLD_JTAG_TDI,
  input         DBA_CPLD_JTAG_TDO,

  output        DBA_MYK_GPIO_0,
  output        DBA_MYK_GPIO_1,
  output        DBA_MYK_GPIO_3,
  output        DBA_MYK_GPIO_4,
  output        DBA_MYK_GPIO_12,
  output        DBA_MYK_GPIO_13,
  output        DBA_MYK_GPIO_14,
  output        DBA_MYK_GPIO_15,

  input         DBA_FPGA_CLK_P,
  input         DBA_FPGA_CLK_N,
  input         DBA_FPGA_SYSREF_P,
  input         DBA_FPGA_SYSREF_N,

  input         USRPIO_A_MGTCLK_P,
  input         USRPIO_A_MGTCLK_N,

  input  [3:0]  USRPIO_A_RX_P,
  input  [3:0]  USRPIO_A_RX_N,
  output [3:0]  USRPIO_A_TX_P,
  output [3:0]  USRPIO_A_TX_N

`ifndef N300
  //USRP IO B
  ,output       DBB_CPLD_PS_SPI_SCLK,
  output        DBB_CPLD_PS_SPI_LE,
  output        DBB_CPLD_PS_SPI_SDI,
  input         DBB_CPLD_PS_SPI_SDO,
  output [1:0]  DBB_CPLD_PS_SPI_ADDR,

  output        DBB_ATR_RX_1,
  output        DBB_ATR_RX_2,
  output        DBB_ATR_TX_1,
  output        DBB_ATR_TX_2,

  output [5:0]  DBB_CH1_TX_DSA_DATA,
  output [5:0]  DBB_CH1_RX_DSA_DATA,
  output [5:0]  DBB_CH2_TX_DSA_DATA,
  output [5:0]  DBB_CH2_RX_DSA_DATA,

  output        DBB_CPLD_PL_SPI_SCLK,
  output        DBB_CPLD_PL_SPI_LE,
  output        DBB_CPLD_PL_SPI_SDI,
  input         DBB_CPLD_PL_SPI_SDO,
  output [2:0]  DBB_CPLD_PL_SPI_ADDR,

  output        DBB_MYK_SPI_SCLK,
  output        DBB_MYK_SPI_CS_n,
  input         DBB_MYK_SPI_SDO,
  output        DBB_MYK_SPI_SDIO,
  input         DBB_MYK_INTRQ,

  output        DBB_MYK_SYNC_IN_n,
  input         DBB_MYK_SYNC_OUT_n,

  inout         DBB_CPLD_JTAG_TCK,
  inout         DBB_CPLD_JTAG_TMS,
  inout         DBB_CPLD_JTAG_TDI,
  input         DBB_CPLD_JTAG_TDO,

  output        DBB_MYK_GPIO_0,
  output        DBB_MYK_GPIO_1,
  output        DBB_MYK_GPIO_3,
  output        DBB_MYK_GPIO_4,
  output        DBB_MYK_GPIO_12,
  output        DBB_MYK_GPIO_13,
  output        DBB_MYK_GPIO_14,
  output        DBB_MYK_GPIO_15,

  input         DBB_FPGA_CLK_P,
  input         DBB_FPGA_CLK_N,
  input         DBB_FPGA_SYSREF_P,
  input         DBB_FPGA_SYSREF_N,

  input         USRPIO_B_MGTCLK_P,
  input         USRPIO_B_MGTCLK_N,

  input  [3:0]  USRPIO_B_RX_P,
  input  [3:0]  USRPIO_B_RX_N,
  output [3:0]  USRPIO_B_TX_P,
  output [3:0]  USRPIO_B_TX_N
`endif
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

  localparam N_AXILITE_SLAVES = 4;
  localparam REG_AWIDTH = 14; // log2(0x4000)
  localparam QSFP_REG_AWIDTH = 17; // log2(0x20000)
  localparam REG_DWIDTH = 32;
  localparam FP_GPIO_OFFSET = 32;
  localparam FP_GPIO_WIDTH = 12;

`ifdef N310
  localparam NUM_RADIOS = 2;
  localparam NUM_CHANNELS_PER_RADIO = 2;
  localparam NUM_DBOARDS = 2;
`elsif N300
  localparam NUM_RADIOS = 1;
  localparam NUM_CHANNELS_PER_RADIO = 2;
  localparam NUM_DBOARDS = 1;
`endif
  localparam NUM_CHANNELS = NUM_RADIOS * NUM_CHANNELS_PER_RADIO;

  // Internal connections to PS
  // HP0 -- High Performance port 0, FPGA is the master
  wire [31:0] S_AXI_HP0_AWADDR;
  wire [2:0]  S_AXI_HP0_AWPROT;
  wire        S_AXI_HP0_AWVALID;
  wire        S_AXI_HP0_AWREADY;
  wire [63:0] S_AXI_HP0_WDATA;
  wire [7:0]  S_AXI_HP0_WSTRB;
  wire        S_AXI_HP0_WVALID;
  wire        S_AXI_HP0_WREADY;
  wire [1:0]  S_AXI_HP0_BRESP;
  wire        S_AXI_HP0_BVALID;
  wire        S_AXI_HP0_BREADY;
  wire [31:0] S_AXI_HP0_ARADDR;
  wire [2:0]  S_AXI_HP0_ARPROT;
  wire        S_AXI_HP0_ARVALID;
  wire        S_AXI_HP0_ARREADY;
  wire [63:0] S_AXI_HP0_RDATA;
  wire [1:0]  S_AXI_HP0_RRESP;
  wire        S_AXI_HP0_RVALID;
  wire        S_AXI_HP0_RREADY;
  wire        S_AXI_HP0_RLAST;
  wire [3:0]  S_AXI_HP0_ARCACHE;
  wire [7:0]  S_AXI_HP0_AWLEN;
  wire [2:0]  S_AXI_HP0_AWSIZE;
  wire [1:0]  S_AXI_HP0_AWBURST;
  wire [3:0]  S_AXI_HP0_AWCACHE;
  wire        S_AXI_HP0_WLAST;
  wire [7:0]  S_AXI_HP0_ARLEN;
  wire [1:0]  S_AXI_HP0_ARBURST;
  wire [2:0]  S_AXI_HP0_ARSIZE;

  // GP0 -- General Purpose port 0, FPGA is the master
  wire [31:0] S_AXI_GP0_AWADDR;
  wire [2:0]  S_AXI_GP0_AWPROT;
  wire        S_AXI_GP0_AWVALID;
  wire        S_AXI_GP0_AWREADY;
  wire [31:0] S_AXI_GP0_WDATA;
  wire [3:0]  S_AXI_GP0_WSTRB;
  wire        S_AXI_GP0_WVALID;
  wire        S_AXI_GP0_WREADY;
  wire [1:0]  S_AXI_GP0_BRESP;
  wire        S_AXI_GP0_BVALID;
  wire        S_AXI_GP0_BREADY;
  wire [31:0] S_AXI_GP0_ARADDR;
  wire [2:0]  S_AXI_GP0_ARPROT;
  wire        S_AXI_GP0_ARVALID;
  wire        S_AXI_GP0_ARREADY;
  wire [31:0] S_AXI_GP0_RDATA;
  wire [1:0]  S_AXI_GP0_RRESP;
  wire        S_AXI_GP0_RVALID;
  wire        S_AXI_GP0_RREADY;
  wire        S_AXI_GP0_RLAST;
  wire [3:0]  S_AXI_GP0_ARCACHE;
  wire [7:0]  S_AXI_GP0_AWLEN;
  wire [2:0]  S_AXI_GP0_AWSIZE;
  wire [1:0]  S_AXI_GP0_AWBURST;
  wire [3:0]  S_AXI_GP0_AWCACHE;
  wire        S_AXI_GP0_WLAST;
  wire [7:0]  S_AXI_GP0_ARLEN;
  wire [1:0]  S_AXI_GP0_ARBURST;
  wire [2:0]  S_AXI_GP0_ARSIZE;

  // HP1 -- High Performance port 1, FPGA is the master
  wire [5:0]  S_AXI_HP1_AWID;
  wire [31:0] S_AXI_HP1_AWADDR;
  wire [2:0]  S_AXI_HP1_AWPROT;
  wire        S_AXI_HP1_AWVALID;
  wire        S_AXI_HP1_AWREADY;
  wire [63:0] S_AXI_HP1_WDATA;
  wire [7:0]  S_AXI_HP1_WSTRB;
  wire        S_AXI_HP1_WVALID;
  wire        S_AXI_HP1_WREADY;
  wire [1:0]  S_AXI_HP1_BRESP;
  wire        S_AXI_HP1_BVALID;
  wire        S_AXI_HP1_BREADY;
  wire [5:0]  S_AXI_HP1_ARID;
  wire [31:0] S_AXI_HP1_ARADDR;
  wire [2:0]  S_AXI_HP1_ARPROT;
  wire        S_AXI_HP1_ARVALID;
  wire        S_AXI_HP1_ARREADY;
  wire [63:0] S_AXI_HP1_RDATA;
  wire [1:0]  S_AXI_HP1_RRESP;
  wire        S_AXI_HP1_RVALID;
  wire        S_AXI_HP1_RREADY;
  wire        S_AXI_HP1_RLAST;
  wire [3:0]  S_AXI_HP1_ARCACHE;
  wire [7:0]  S_AXI_HP1_AWLEN;
  wire [2:0]  S_AXI_HP1_AWSIZE;
  wire [1:0]  S_AXI_HP1_AWBURST;
  wire [3:0]  S_AXI_HP1_AWCACHE;
  wire        S_AXI_HP1_WLAST;
  wire [7:0]  S_AXI_HP1_ARLEN;
  wire [1:0]  S_AXI_HP1_ARBURST;
  wire [2:0]  S_AXI_HP1_ARSIZE;

  // GP1 -- General Purpose port 1, FPGA is the master
  wire [4:0]  S_AXI_GP1_AWID;
  wire [31:0] S_AXI_GP1_AWADDR;
  wire [2:0]  S_AXI_GP1_AWPROT;
  wire        S_AXI_GP1_AWVALID;
  wire        S_AXI_GP1_AWREADY;
  wire [31:0] S_AXI_GP1_WDATA;
  wire [3:0]  S_AXI_GP1_WSTRB;
  wire        S_AXI_GP1_WVALID;
  wire        S_AXI_GP1_WREADY;
  wire [1:0]  S_AXI_GP1_BRESP;
  wire        S_AXI_GP1_BVALID;
  wire        S_AXI_GP1_BREADY;
  wire [4:0]  S_AXI_GP1_ARID;
  wire [31:0] S_AXI_GP1_ARADDR;
  wire [2:0]  S_AXI_GP1_ARPROT;
  wire        S_AXI_GP1_ARVALID;
  wire        S_AXI_GP1_ARREADY;
  wire [31:0] S_AXI_GP1_RDATA;
  wire [1:0]  S_AXI_GP1_RRESP;
  wire        S_AXI_GP1_RVALID;
  wire        S_AXI_GP1_RREADY;
  wire        S_AXI_GP1_RLAST;
  wire [3:0]  S_AXI_GP1_ARCACHE;
  wire [7:0]  S_AXI_GP1_AWLEN;
  wire [2:0]  S_AXI_GP1_AWSIZE;
  wire [1:0]  S_AXI_GP1_AWBURST;
  wire [3:0]  S_AXI_GP1_AWCACHE;
  wire        S_AXI_GP1_WLAST;
  wire [7:0]  S_AXI_GP1_ARLEN;
  wire [1:0]  S_AXI_GP1_ARBURST;
  wire [2:0]  S_AXI_GP1_ARSIZE;

  // GP0 -- General Purpose port 0, FPGA is the slave
  wire        M_AXI_GP0_ARVALID;
  wire        M_AXI_GP0_AWVALID;
  wire        M_AXI_GP0_BREADY;
  wire        M_AXI_GP0_RREADY;
  wire        M_AXI_GP0_WVALID;
  wire [11:0] M_AXI_GP0_ARID;
  wire [11:0] M_AXI_GP0_AWID;
  wire [11:0] M_AXI_GP0_WID;
  wire [31:0] M_AXI_GP0_ARADDR;
  wire [31:0] M_AXI_GP0_AWADDR;
  wire [31:0] M_AXI_GP0_WDATA;
  wire [3:0]  M_AXI_GP0_WSTRB;
  wire        M_AXI_GP0_ARREADY;
  wire        M_AXI_GP0_AWREADY;
  wire        M_AXI_GP0_BVALID;
  wire        M_AXI_GP0_RLAST;
  wire        M_AXI_GP0_RVALID;
  wire        M_AXI_GP0_WREADY;
  wire [1:0]  M_AXI_GP0_BRESP;
  wire [1:0]  M_AXI_GP0_RRESP;
  wire [31:0] M_AXI_GP0_RDATA;

  // ETH DMA
  wire        M_AXI_ETH_DMA0_ARVALID;
  wire        M_AXI_ETH_DMA0_AWVALID;
  wire        M_AXI_ETH_DMA0_BREADY;
  wire        M_AXI_ETH_DMA0_RREADY;
  wire        M_AXI_ETH_DMA0_WVALID;
  wire [11:0] M_AXI_ETH_DMA0_ARID;
  wire [11:0] M_AXI_ETH_DMA0_AWID;
  wire [11:0] M_AXI_ETH_DMA0_WID;
  wire [31:0] M_AXI_ETH_DMA0_ARADDR;
  wire [31:0] M_AXI_ETH_DMA0_AWADDR;
  wire [31:0] M_AXI_ETH_DMA0_WDATA;
  wire [3:0]  M_AXI_ETH_DMA0_WSTRB;
  wire        M_AXI_ETH_DMA0_ARREADY;
  wire        M_AXI_ETH_DMA0_AWREADY;
  wire        M_AXI_ETH_DMA0_BVALID;
  wire        M_AXI_ETH_DMA0_RLAST;
  wire        M_AXI_ETH_DMA0_RVALID;
  wire        M_AXI_ETH_DMA0_WREADY;
  wire [1:0]  M_AXI_ETH_DMA0_BRESP;
  wire [1:0]  M_AXI_ETH_DMA0_RRESP;
  wire [31:0] M_AXI_ETH_DMA0_RDATA;
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

  wire        M_AXI_NET0_ARVALID;
  wire        M_AXI_NET0_AWVALID;
  wire        M_AXI_NET0_BREADY;
  wire        M_AXI_NET0_RREADY;
  wire        M_AXI_NET0_WVALID;
  wire [11:0] M_AXI_NET0_ARID;
  wire [11:0] M_AXI_NET0_AWID;
  wire [11:0] M_AXI_NET0_WID;
  wire [31:0] M_AXI_NET0_ARADDR;
  wire [31:0] M_AXI_NET0_AWADDR;
  wire [31:0] M_AXI_NET0_WDATA;
  wire [3:0]  M_AXI_NET0_WSTRB;
  wire        M_AXI_NET0_ARREADY;
  wire        M_AXI_NET0_AWREADY;
  wire        M_AXI_NET0_BVALID;
  wire        M_AXI_NET0_RLAST;
  wire        M_AXI_NET0_RVALID;
  wire        M_AXI_NET0_WREADY;
  wire [1:0]  M_AXI_NET0_BRESP;
  wire [1:0]  M_AXI_NET0_RRESP;
  wire [31:0] M_AXI_NET0_RDATA;

  wire        M_AXI_ETH_DMA1_ARVALID;
  wire        M_AXI_ETH_DMA1_AWVALID;
  wire        M_AXI_ETH_DMA1_BREADY;
  wire        M_AXI_ETH_DMA1_RREADY;
  wire        M_AXI_ETH_DMA1_WVALID;
  wire [11:0] M_AXI_ETH_DMA1_ARID;
  wire [11:0] M_AXI_ETH_DMA1_AWID;
  wire [11:0] M_AXI_ETH_DMA1_WID;
  wire [31:0] M_AXI_ETH_DMA1_ARADDR;
  wire [31:0] M_AXI_ETH_DMA1_AWADDR;
  wire [31:0] M_AXI_ETH_DMA1_WDATA;
  wire [3:0]  M_AXI_ETH_DMA1_WSTRB;
  wire        M_AXI_ETH_DMA1_ARREADY;
  wire        M_AXI_ETH_DMA1_AWREADY;
  wire        M_AXI_ETH_DMA1_BVALID;
  wire        M_AXI_ETH_DMA1_RLAST;
  wire        M_AXI_ETH_DMA1_RVALID;
  wire        M_AXI_ETH_DMA1_WREADY;
  wire [1:0]  M_AXI_ETH_DMA1_BRESP;
  wire [1:0]  M_AXI_ETH_DMA1_RRESP;
  wire [31:0] M_AXI_ETH_DMA1_RDATA;

  wire        M_AXI_NET1_ARVALID;
  wire        M_AXI_NET1_AWVALID;
  wire        M_AXI_NET1_BREADY;
  wire        M_AXI_NET1_RREADY;
  wire        M_AXI_NET1_WVALID;
  wire [11:0] M_AXI_NET1_ARID;
  wire [11:0] M_AXI_NET1_AWID;
  wire [11:0] M_AXI_NET1_WID;
  wire [31:0] M_AXI_NET1_ARADDR;
  wire [31:0] M_AXI_NET1_AWADDR;
  wire [31:0] M_AXI_NET1_WDATA;
  wire [3:0]  M_AXI_NET1_WSTRB;
  wire        M_AXI_NET1_ARREADY;
  wire        M_AXI_NET1_AWREADY;
  wire        M_AXI_NET1_BVALID;
  wire        M_AXI_NET1_RLAST;
  wire        M_AXI_NET1_RVALID;
  wire        M_AXI_NET1_WREADY;
  wire [1:0]  M_AXI_NET1_BRESP;
  wire [1:0]  M_AXI_NET1_RRESP;
  wire [31:0] M_AXI_NET1_RDATA;

  wire        M_AXI_NET2_ARVALID;
  wire        M_AXI_NET2_AWVALID;
  wire        M_AXI_NET2_BREADY;
  wire        M_AXI_NET2_RREADY;
  wire        M_AXI_NET2_WVALID;
  wire [11:0] M_AXI_NET2_ARID;
  wire [11:0] M_AXI_NET2_AWID;
  wire [11:0] M_AXI_NET2_WID;
  wire [31:0] M_AXI_NET2_ARADDR;
  wire [31:0] M_AXI_NET2_AWADDR;
  wire [31:0] M_AXI_NET2_WDATA;
  wire [3:0]  M_AXI_NET2_WSTRB;
  wire        M_AXI_NET2_ARREADY;
  wire        M_AXI_NET2_AWREADY;
  wire        M_AXI_NET2_BVALID;
  wire        M_AXI_NET2_RLAST;
  wire        M_AXI_NET2_RVALID;
  wire        M_AXI_NET2_WREADY;
  wire [1:0]  M_AXI_NET2_BRESP;
  wire [1:0]  M_AXI_NET2_RRESP;
  wire [31:0] M_AXI_NET2_RDATA;

  wire        M_AXI_XBAR_ARVALID;
  wire        M_AXI_XBAR_AWVALID;
  wire        M_AXI_XBAR_BREADY;
  wire        M_AXI_XBAR_RREADY;
  wire        M_AXI_XBAR_WVALID;
  wire [11:0] M_AXI_XBAR_ARID;
  wire [11:0] M_AXI_XBAR_AWID;
  wire [11:0] M_AXI_XBAR_WID;
  wire [31:0] M_AXI_XBAR_ARADDR;
  wire [31:0] M_AXI_XBAR_AWADDR;
  wire [31:0] M_AXI_XBAR_WDATA;
  wire [3:0]  M_AXI_XBAR_WSTRB;
  wire        M_AXI_XBAR_ARREADY;
  wire        M_AXI_XBAR_AWREADY;
  wire        M_AXI_XBAR_BVALID;
  wire        M_AXI_XBAR_RLAST;
  wire        M_AXI_XBAR_RVALID;
  wire        M_AXI_XBAR_WREADY;
  wire [1:0]  M_AXI_XBAR_BRESP;
  wire [1:0]  M_AXI_XBAR_RRESP;
  wire [31:0] M_AXI_XBAR_RDATA;

  wire        M_AXI_JESD0_ARVALID;
  wire        M_AXI_JESD0_AWVALID;
  wire        M_AXI_JESD0_BREADY;
  wire        M_AXI_JESD0_RREADY;
  wire        M_AXI_JESD0_WVALID;
  wire [11:0] M_AXI_JESD0_ARID;
  wire [11:0] M_AXI_JESD0_AWID;
  wire [11:0] M_AXI_JESD0_WID;
  wire [31:0] M_AXI_JESD0_ARADDR;
  wire [31:0] M_AXI_JESD0_AWADDR;
  wire [31:0] M_AXI_JESD0_WDATA;
  wire [3:0]  M_AXI_JESD0_WSTRB;
  wire        M_AXI_JESD0_ARREADY;
  wire        M_AXI_JESD0_AWREADY;
  wire        M_AXI_JESD0_BVALID;
  wire        M_AXI_JESD0_RLAST;
  wire        M_AXI_JESD0_RVALID;
  wire        M_AXI_JESD0_WREADY;
  wire [1:0]  M_AXI_JESD0_BRESP;
  wire [1:0]  M_AXI_JESD0_RRESP;
  wire [31:0] M_AXI_JESD0_RDATA;

  wire        M_AXI_JESD1_ARVALID;
  wire        M_AXI_JESD1_AWVALID;
  wire        M_AXI_JESD1_BREADY;
  wire        M_AXI_JESD1_RREADY;
  wire        M_AXI_JESD1_WVALID;
  wire [11:0] M_AXI_JESD1_ARID;
  wire [11:0] M_AXI_JESD1_AWID;
  wire [11:0] M_AXI_JESD1_WID;
  wire [31:0] M_AXI_JESD1_ARADDR;
  wire [31:0] M_AXI_JESD1_AWADDR;
  wire [31:0] M_AXI_JESD1_WDATA;
  wire [3:0]  M_AXI_JESD1_WSTRB;
  wire        M_AXI_JESD1_ARREADY;
  wire        M_AXI_JESD1_AWREADY;
  wire        M_AXI_JESD1_BVALID;
  wire        M_AXI_JESD1_RLAST;
  wire        M_AXI_JESD1_RVALID;
  wire        M_AXI_JESD1_WREADY;
  wire [1:0]  M_AXI_JESD1_BRESP;
  wire [1:0]  M_AXI_JESD1_RRESP;
  wire [31:0] M_AXI_JESD1_RDATA;

  // White Rabbit
  wire wr_uart_txd;
  wire wr_uart_rxd;
  wire pps_wr_refclk;
  wire wr_ref_clk;

  // AXI bus from PS to WR Core
  wire m_axi_wr_clk;
  wire [31:0] m_axi_wr_araddr;
  wire [0:0]  m_axi_wr_arready;
  wire [0:0]  m_axi_wr_arvalid;
  wire [31:0] m_axi_wr_awaddr;
  wire [0:0]  m_axi_wr_awready;
  wire [0:0]  m_axi_wr_awvalid;
  wire [0:0]  m_axi_wr_bready;
  wire [1:0]  m_axi_wr_bresp;
  wire [0:0]  m_axi_wr_bvalid;
  wire [31:0] m_axi_wr_rdata;
  wire [0:0]  m_axi_wr_rready;
  wire [1:0]  m_axi_wr_rresp;
  wire [0:0]  m_axi_wr_rvalid;
  wire [31:0] m_axi_wr_wdata;
  wire [0:0]  m_axi_wr_wready;
  wire [3:0]  m_axi_wr_wstrb;
  wire [0:0]  m_axi_wr_wvalid;

  wire [63:0] ps_gpio_out;
  wire [63:0] ps_gpio_in;
  wire [63:0] ps_gpio_tri;

  wire [15:0] IRQ_F2P;
  wire        FCLK_CLK0;
  wire        FCLK_CLK1;
  wire        FCLK_CLK2;
  wire        FCLK_CLK3;
  wire        clk100;
  wire        clk40;
  wire        meas_clk_ref;
  wire        bus_clk;
  wire        gige_refclk;
  wire        gige_refclk_bufg;
  wire        xgige_refclk;
  wire        xgige_clk156;
  wire        xgige_dclk;

  wire        global_rst;
  wire        radio_rst;
  wire        bus_rst;
  wire        FCLK_RESET0_N;
  wire        clk40_rst;
  wire        clk40_rstn;

  wire [1:0]  USB0_PORT_INDCTL;
  wire        USB0_VBUS_PWRSELECT;
  wire        USB0_VBUS_PWRFAULT;

  wire        ref_clk;
  wire        wr_refclk_buf;
  wire        netclk_buf;
  wire        meas_clk;
  wire        ddr3_dma_clk;
  wire        meas_clk_reset;
  wire        meas_clk_locked;
  wire        enable_ref_clk_async;
  wire        pps_radioclk1x_iob;
  wire        pps_radioclk1x;
  wire [3:0]  pps_select;
  wire        pps_out_enb;
  wire [1:0]  pps_select_sfp;
  wire        pps_refclk;
  wire        export_pps_radioclk;
  wire        radio_clk;
  wire        radio_clk_2x;

  wire        qsfp_sda_i;
  wire        qsfp_sda_o;
  wire        qsfp_sda_t;
  wire        qsfp_scl_i;
  wire        qsfp_scl_o;
  wire        qsfp_scl_t;

  /////////////////////////////////////////////////////////////////////
  //
  // Resets
  //
  //////////////////////////////////////////////////////////////////////

  // Global synchronous reset, on the bus_clk domain. De-asserts after 85
  // bus_clk cycles. Asserted by default.
  por_gen por_gen(.clk(bus_clk), .reset_out(global_rst));

  // Synchronous reset for the radio_clk domain, based on the global_rst.
  reset_sync radio_reset_gen (
    .clk(radio_clk),
    .reset_in(global_rst),
    .reset_out(radio_rst)
  );

  // Synchronous reset for the bus_clk domain, based on the global_rst.
  reset_sync bus_reset_gen (
    .clk(bus_clk),
    .reset_in(global_rst),
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


  /////////////////////////////////////////////////////////////////////
  //
  // Timing
  //
  //////////////////////////////////////////////////////////////////////

  // Clocks from the PS
  //
  // These clocks appear to have BUFGs already instantiated by the ip generator.
  // Simply rename them here for clarity.
  //   FCLK_CLK0 :      100 MHz
  //   FCLK_CLK1 :       40 MHz
  //   FCLK_CLK2 : 166.6667 MHz
  //   FCLK_CLK3 :      200 MHz
  assign clk100       = FCLK_CLK0;
  assign clk40        = FCLK_CLK1;
  assign meas_clk_ref = FCLK_CLK2;
  assign bus_clk      = FCLK_CLK3;

  //If bus_clk freq ever changes, update this paramter accordingly.
  localparam BUS_CLK_RATE = 32'd200000000; //200 MHz bus_clk rate.

  n3xx_clocking n3xx_clocking_i (
    .enable_ref_clk_async(enable_ref_clk_async),
    .FPGA_REFCLK_P(FPGA_REFCLK_P),
    .FPGA_REFCLK_N(FPGA_REFCLK_N),
    .ref_clk(ref_clk),
    .WB_20MHz_P(WB_20MHZ_P),
    .WB_20MHz_N(WB_20MHZ_N),
    .wr_refclk_buf(wr_refclk_buf),
    .NETCLK_REF_P(NETCLK_REF_P),
    .NETCLK_REF_N(NETCLK_REF_N),
    .netclk_buf(netclk_buf),
    .NETCLK_P(NETCLK_P),
    .NETCLK_N(NETCLK_N),
    .gige_refclk_buf(gige_refclk),
    .MGT156MHZ_CLK1_P(MGT156MHZ_CLK1_P),
    .MGT156MHZ_CLK1_N(MGT156MHZ_CLK1_N),
    .xgige_refclk_buf(xgige_refclk),
    .misc_clks_ref(meas_clk_ref),
    .meas_clk(meas_clk),
    .ddr3_dma_clk(ddr3_dma_clk),
    .misc_clks_reset(meas_clk_reset),
    .misc_clks_locked(meas_clk_locked),
    .ext_pps_from_pin(REF_1PPS_IN),
    .gps_pps_from_pin(GPS_1PPS),
    .pps_select(pps_select),
    .pps_refclk(pps_refclk)
  );

  // Drive the rear panel connector with another controllable copy of the post-TDC PPS
  // that SW can enable/disable. The user is free to hack this to be whatever
  // they desire. Flop the PPS signal one more time in order that it can be packed into
  // an IOB. This extra flop stage matches the additional flop inside DbCore to allow
  // pps_radioclk1x and pps_out_radioclk to be in sync with one another.
  synchronizer #(
    .FALSE_PATH_TO_IN(0)
  ) pps_export_dsync (
    .clk(radio_clk), .rst(1'b0), .in(pps_out_enb), .out(export_pps_radioclk)
  );

  // The radio_clk rate is between [122.88M, 250M] for all known N3xx variants,
  // resulting in approximately [8ns, 4ns] periods. To pulse-extend the PPS output,
  // we create a 25 bit-wide counter, creating ~[.262s, .131s] long output high pulses,
  // variable depending on our radio_clk rate. Create two of the same output signal
  // in order that the PPS_OUT gets packed into an IOB for tight timing.
  reg [24:0] pps_out_count = 'b0;
  reg        pps_out_radioclk = 1'b0;
  reg        pps_led_radioclk = 1'b0;

  always @(posedge radio_clk) begin
    if (export_pps_radioclk) begin
      if (pps_radioclk1x_iob) begin
        pps_out_radioclk <= 1'b1;
        pps_led_radioclk <= 1'b1;
        pps_out_count <= {25{1'b1}};
      end else begin
        if (pps_out_count > 0) begin
          pps_out_count <= pps_out_count - 1'b1;
        end else begin
          pps_out_radioclk <= 1'b0;
          pps_led_radioclk <= 1'b0;
        end
      end
    end else begin
      pps_out_radioclk <= 1'b0;
      pps_led_radioclk <= 1'b0;
    end
  end
  // Local to output.
  assign REF_1PPS_OUT  = pps_out_radioclk;
  assign PANEL_LED_PPS = pps_led_radioclk;

  /////////////////////////////////////////////////////////////////////
  //
  // SFP, QSFP and NPIO MGT Connections
  //
  //////////////////////////////////////////////////////////////////////
  wire                    reg_wr_req_npio;
  wire [REG_AWIDTH-1:0]   reg_wr_addr_npio;
  wire [REG_DWIDTH-1:0]   reg_wr_data_npio;
  wire                    reg_rd_req_npio;
  wire [REG_AWIDTH-1:0]   reg_rd_addr_npio;
  wire                    reg_rd_resp_npio, reg_rd_resp_npio0, reg_rd_resp_npio1;
  wire [REG_DWIDTH-1:0]   reg_rd_data_npio, reg_rd_data_npio0, reg_rd_data_npio1;

  regport_resp_mux #(
    .WIDTH      (REG_DWIDTH),
    .NUM_SLAVES (2)
  ) npio_resp_mux_i(
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp({reg_rd_resp_npio0, reg_rd_resp_npio1}),
    .sla_rd_data({reg_rd_data_npio0, reg_rd_data_npio1}),
    .mst_rd_resp(reg_rd_resp_npio), .mst_rd_data(reg_rd_data_npio)
  );

  //--------------------------------------------------------------
  // SFP/MGT Reference Clocks
  //--------------------------------------------------------------

  // We support the HG, XG, XA, AA targets, all of which require
  // the 156.25MHz reference clock. Instantiate it here.
  ten_gige_phy_clk_gen xgige_clk_gen_i (
    .refclk_ibuf(xgige_refclk),
    .clk156(xgige_clk156),
    .dclk(xgige_dclk)
  );

  wire qpllreset;
  wire qpllreset_sfp0, qpllreset_sfp1, qpllreset_npio0, qpllreset_npio1;
  wire qplllock;
  wire qplloutclk;
  wire qplloutrefclk;

  // We reuse this GT_COMMON wrapper for both ethernet and Aurora because
  // the behavior is identical
  ten_gig_eth_pcs_pma_gt_common # (
    .WRAPPER_SIM_GTRESET_SPEEDUP("TRUE") //Does not affect hardware
  ) ten_gig_eth_pcs_pma_gt_common_block (
    .refclk(xgige_refclk),
    .qpllreset(qpllreset), //from 2nd sfp
    .qplllock(qplllock),
    .qplloutclk(qplloutclk),
    .qplloutrefclk(qplloutrefclk),
    .qpllrefclksel(3'b101 /*GTSOUTHREFCLK0*/)
  );

  // The quad's QPLL should reset if any of the channels request it
  // This should never really happen because we are not changing the reference clock
  // source for the QPLL.
  assign qpllreset = qpllreset_sfp0 | qpllreset_sfp1 | qpllreset_npio0 | qpllreset_npio1;

  // Use the 156.25MHz reference clock for Aurora
  wire aurora_refclk = xgige_refclk;
  wire aurora_clk156 = xgige_clk156;
  wire aurora_init_clk = xgige_dclk;

  // White Rabbit and 1GbE both use the same clocking
`ifdef SFP0_1GBE
  `define SFP0_WR_1GBE
`endif
`ifdef SFP0_WR
  `define SFP0_WR_1GBE
`endif

`ifdef SFP0_WR_1GBE
  // HG and WX targets require the 1GbE clock support
  BUFG bufg_gige_refclk_i (
    .I(gige_refclk),
    .O(gige_refclk_bufg)
  );
  assign SFP_0_RS0  = 1'b0;
  assign SFP_0_RS1  = 1'b0;
`else
  assign SFP_0_RS0  = 1'b1;
  assign SFP_0_RS1  = 1'b1;
`endif

  // SFP 1 is always set to run at ~10Gbps rates.
  assign SFP_1_RS0  = 1'b1;
  assign SFP_1_RS1  = 1'b1;

  // SFP port specific reference clocks
  wire  sfp0_gt_refclk, sfp1_gt_refclk;
  wire  sfp0_gb_refclk, sfp1_gb_refclk;
  wire  sfp0_misc_clk, sfp1_misc_clk;

`ifdef SFP0_10GBE
  assign sfp0_gt_refclk = xgige_refclk;
  assign sfp0_gb_refclk = xgige_clk156;
  assign sfp0_misc_clk  = xgige_dclk;
`endif
`ifdef SFP0_WR_1GBE
  assign sfp0_gt_refclk = gige_refclk;
  assign sfp0_gb_refclk = gige_refclk_bufg;
  assign sfp0_misc_clk  = gige_refclk_bufg;
`endif
`ifdef SFP0_AURORA
  assign sfp0_gt_refclk = aurora_refclk;
  assign sfp0_gb_refclk = aurora_clk156;
  assign sfp0_misc_clk  = aurora_init_clk;
`endif

`ifdef SFP1_10GBE
  assign sfp1_gt_refclk = xgige_refclk;
  assign sfp1_gb_refclk = xgige_clk156;
  assign sfp1_misc_clk  = xgige_dclk;
`endif
`ifdef SFP1_1GBE
  assign sfp1_gt_refclk = gige_refclk;
  assign sfp1_gb_refclk = gige_refclk_bufg;
  assign sfp1_misc_clk  = gige_refclk_bufg;
`endif
`ifdef SFP1_AURORA
  assign sfp1_gt_refclk = aurora_refclk;
  assign sfp1_gb_refclk = aurora_clk156;
  assign sfp1_misc_clk  = aurora_init_clk;
`endif

  // Instantiate Aurora MMCM if either of the SFPs
  // or NPIOs are Aurora
  wire au_tx_clk;
  wire au_mmcm_reset;
  wire au_user_clk;
  wire au_sync_clk;
  wire au_mmcm_locked;
  wire sfp0_tx_out_clk, sfp1_tx_out_clk;
  wire sfp0_gt_pll_lock, sfp1_gt_pll_lock;
  wire npio0_tx_out_clk, npio1_tx_out_clk;
  wire npio0_gt_pll_lock, npio1_gt_pll_lock;

  //NOTE: need to declare one of these defines in order to enable Aurora on
  //any SFP or NPIO lane.
`ifdef SFP1_AURORA
  `define SFP_AU_MMCM
  assign au_tx_clk     = sfp1_tx_out_clk;
  assign au_mmcm_reset = ~sfp1_gt_pll_lock;
`elsif NPIO0
  `define SFP_AU_MMCM
  assign au_tx_clk     = npio0_tx_out_clk;
  assign au_mmcm_reset = ~npio0_gt_pll_lock;
`elsif NPIO1
  `define SFP_AU_MMCM
  assign au_tx_clk     = npio1_tx_out_clk;
  assign au_mmcm_reset = ~npio1_gt_pll_lock;
`endif


`ifdef SFP_AU_MMCM
  aurora_phy_mmcm au_phy_mmcm_i (
    .aurora_tx_clk_unbuf(au_tx_clk),
    .mmcm_reset(au_mmcm_reset),
    .user_clk(au_user_clk),
    .sync_clk(au_sync_clk),
    .mmcm_locked(au_mmcm_locked)
  );
`else
  assign au_user_clk = 1'b0;
  assign au_sync_clk = 1'b0;
  assign au_mmcm_locked = 1'b0;
`endif

  //--------------------------------------------------------------
  // NPIO-QSFP MGT Lanes (Example loopback config)
  //--------------------------------------------------------------

`ifdef QSFP_LANES
  localparam NUM_QSFP_LANES = `QSFP_LANES;

  // QSFP wires to the ARM core and the crossbar
  // These will only be connected if QSFP is 2x10 GbE
  wire [NUM_QSFP_LANES*64-1:0] arm_eth_qsfp_tx_tdata_b;
  wire [NUM_QSFP_LANES-1:0]    arm_eth_qsfp_tx_tvalid_b;
  wire [NUM_QSFP_LANES-1:0]    arm_eth_qsfp_tx_tlast_b;
  wire [NUM_QSFP_LANES-1:0]    arm_eth_qsfp_tx_tready_b;
  wire [NUM_QSFP_LANES*4-1:0]  arm_eth_qsfp_tx_tuser_b;
  wire [NUM_QSFP_LANES*8-1:0]  arm_eth_qsfp_tx_tkeep_b;

  wire [NUM_QSFP_LANES*64-1:0] arm_eth_qsfp_rx_tdata_b;
  wire [NUM_QSFP_LANES-1:0]    arm_eth_qsfp_rx_tvalid_b;
  wire [NUM_QSFP_LANES-1:0]    arm_eth_qsfp_rx_tlast_b;
  wire [NUM_QSFP_LANES-1:0]    arm_eth_qsfp_rx_tready_b;
  wire [NUM_QSFP_LANES*4-1:0]  arm_eth_qsfp_rx_tuser_b;
  wire [NUM_QSFP_LANES*8-1:0]  arm_eth_qsfp_rx_tkeep_b;

  wire [NUM_QSFP_LANES*64-1:0] v2e_qsfp_tdata;
  wire [NUM_QSFP_LANES-1:0]    v2e_qsfp_tlast;
  wire [NUM_QSFP_LANES-1:0]    v2e_qsfp_tvalid;
  wire [NUM_QSFP_LANES-1:0]    v2e_qsfp_tready;

  wire [NUM_QSFP_LANES*64-1:0] e2v_qsfp_tdata;
  wire [NUM_QSFP_LANES-1:0]    e2v_qsfp_tlast;
  wire [NUM_QSFP_LANES-1:0]    e2v_qsfp_tvalid;
  wire [NUM_QSFP_LANES-1:0]    e2v_qsfp_tready;

  wire [NUM_QSFP_LANES-1:0] qsfp_link_up;

  // QSFP quad's specific reference clocks
  wire qsfp_gt_refclk;
  wire qsfp_gb_refclk;
  wire qsfp_misc_clk;

  wire qsfp_qplloutclk;
  wire qsfp_qplloutrefclk;
  wire qsfp_qplllock;
  wire qsfp_qpllreset;

  wire qsfp_gt_tx_out_clk;
  wire qsfp_gt_pll_lock;

  wire qsfp_au_user_clk;
  wire qsfp_au_sync_clk;
  wire qsfp_au_mmcm_locked;


`ifdef QSFP_10GBE
  assign qsfp_gt_refclk = xgige_refclk;
  assign qsfp_gb_refclk = xgige_clk156;
  assign qsfp_misc_clk  = xgige_dclk;
`endif
`ifdef QSFP_AURORA
  assign qsfp_gt_refclk = aurora_refclk;
  assign qsfp_gb_refclk = aurora_clk156;
  assign qsfp_misc_clk  = aurora_init_clk;
`endif

  // We reuse this GT_COMMON wrapper for both ethernet and Aurora because
  // the behavior is identical
  ten_gig_eth_pcs_pma_gt_common # (
    .WRAPPER_SIM_GTRESET_SPEEDUP("TRUE") //Does not affect hardware
  ) qsfp_gt_common_block (
    .refclk(xgige_refclk),
    .qpllreset(qsfp_qpllreset),
    .qplllock(qsfp_qplllock),
    .qplloutclk(qsfp_qplloutclk),
    .qplloutrefclk(qsfp_qplloutrefclk),
    .qpllrefclksel(3'b001 /*GTREFCLK0*/)
  );

  `ifdef QSFP_AURORA
    aurora_phy_mmcm aurora_phy_mmcm (
      .aurora_tx_clk_unbuf(qsfp_gt_tx_out_clk),
      .mmcm_reset(~qsfp_gt_pll_lock),
      .user_clk(qsfp_au_user_clk),
      .sync_clk(qsfp_au_sync_clk),
      .mmcm_locked(qsfp_au_mmcm_locked)
    );
  `else
    assign qsfp_au_user_clk = 1'b0;
    assign qsfp_au_sync_clk = 1'b0;
    assign qsfp_au_mmcm_locked = 1'b0;
  `endif

  n3xx_mgt_channel_wrapper #(
  `ifdef QSFP_10GBE
    .PROTOCOL       ("10GbE"),
    .MDIO_EN        (1'b1),
    .MDIO_PHYADDR   (5'd4),
  `elsif QSFP_AURORA
    .PROTOCOL       ("Aurora"),
    .MDIO_EN        (1'b0),
  `endif
    .LANES          (NUM_QSFP_LANES),
    .PORTNUM_BASE   (4),
    .REG_DWIDTH     (REG_DWIDTH),
    .REG_AWIDTH     (QSFP_REG_AWIDTH)
  ) qsfp_wrapper_i (
    .areset         (global_rst),
    .gt_refclk      (qsfp_gt_refclk),
    .gb_refclk      (qsfp_gb_refclk),
    .misc_clk       (qsfp_misc_clk),
    .user_clk       (qsfp_au_user_clk),
    .sync_clk       (qsfp_au_sync_clk),
    .gt_tx_out_clk_unbuf(qsfp_gt_tx_out_clk),

    .bus_clk        (bus_clk),
    .bus_rst        (bus_rst),

    // GT Common
    .qpllrefclklost (),
    .qplllock       (qsfp_qplllock),
    .qplloutclk     (qsfp_qplloutclk),
    .qplloutrefclk  (qsfp_qplloutrefclk),
    .qpllreset      (qsfp_qpllreset),

    // Aurora MMCM
    .mmcm_locked    (qsfp_au_mmcm_locked),
    .gt_pll_lock    (qsfp_gt_pll_lock),

    .txp            (QSFP_TX_P),
    .txn            (QSFP_TX_N),
    .rxp            (QSFP_RX_P),
    .rxn            (QSFP_RX_N),

    .mod_present_n  (QSFP_PRESENT_B),
    .mod_rxlos      (1'b0),
    .mod_tx_fault   (1'b0),
    .mod_tx_disable (),
    .mod_int_n      (QSFP_INT_B),
    .mod_reset_n    (QSFP_RESET_B),
    .mod_lpmode     (QSFP_LPMODE),
    .mod_sel_n      (QSFP_MODSEL_B),

    // Clock and reset
    .s_axi_aclk     (clk40),
    .s_axi_aresetn  (clk40_rstn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr   (M_AXI_NET2_AWADDR[QSFP_REG_AWIDTH-1:0]),
    .s_axi_awvalid  (M_AXI_NET2_AWVALID),
    .s_axi_awready  (M_AXI_NET2_AWREADY),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata    (M_AXI_NET2_WDATA),
    .s_axi_wstrb    (M_AXI_NET2_WSTRB),
    .s_axi_wvalid   (M_AXI_NET2_WVALID),
    .s_axi_wready   (M_AXI_NET2_WREADY),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp    (M_AXI_NET2_BRESP),
    .s_axi_bvalid   (M_AXI_NET2_BVALID),
    .s_axi_bready   (M_AXI_NET2_BREADY),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr   (M_AXI_NET2_ARADDR[QSFP_REG_AWIDTH-1:0]),
    .s_axi_arvalid  (M_AXI_NET2_ARVALID),
    .s_axi_arready  (M_AXI_NET2_ARREADY),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata    (M_AXI_NET2_RDATA),
    .s_axi_rresp    (M_AXI_NET2_RRESP),
    .s_axi_rvalid   (M_AXI_NET2_RVALID),
    .s_axi_rready   (M_AXI_NET2_RREADY),

    // Ethernet to Vita
    .e2v_tdata      (e2v_qsfp_tdata),
    .e2v_tlast      (e2v_qsfp_tlast),
    .e2v_tvalid     (e2v_qsfp_tvalid),
    .e2v_tready     (e2v_qsfp_tready),

    // Vita to Ethernet
    .v2e_tdata      (v2e_qsfp_tdata),
    .v2e_tlast      (v2e_qsfp_tlast),
    .v2e_tvalid     (v2e_qsfp_tvalid),
    .v2e_tready     (v2e_qsfp_tready),

    // Ethernet to CPU
    .e2c_tdata      (arm_eth_qsfp_rx_tdata_b),
    .e2c_tkeep      (arm_eth_qsfp_rx_tkeep_b),
    .e2c_tlast      (arm_eth_qsfp_rx_tlast_b),
    .e2c_tvalid     (arm_eth_qsfp_rx_tvalid_b),
    .e2c_tready     (arm_eth_qsfp_rx_tready_b),

    // CPU to Ethernet
    .c2e_tdata      (arm_eth_qsfp_tx_tdata_b),
    .c2e_tkeep      (arm_eth_qsfp_tx_tkeep_b),
    .c2e_tlast      (arm_eth_qsfp_tx_tlast_b),
    .c2e_tvalid     (arm_eth_qsfp_tx_tvalid_b),
    .c2e_tready     (arm_eth_qsfp_tx_tready_b),

    // Sideband White Rabbit Control
    .wr_reset_n     (1'b1),
    .wr_refclk      (1'b0),

    .wr_dac_sclk    (),
    .wr_dac_din     (),
    .wr_dac_clr_n   (),
    .wr_dac_cs_n    (),
    .wr_dac_ldac_n  (),

    .wr_eeprom_scl_o(),
    .wr_eeprom_scl_i(1'b0),
    .wr_eeprom_sda_o(),
    .wr_eeprom_sda_i(1'b0),

    .wr_uart_rx     (1'b0),
    .wr_uart_tx     (),

    .mod_pps        (),
    .mod_refclk     (),

    // WR AXI Control
    .wr_axi_aclk    (),
    .wr_axi_aresetn (1'b1),
    .wr_axi_awaddr  (),
    .wr_axi_awvalid (),
    .wr_axi_awready (),
    .wr_axi_wdata   (),
    .wr_axi_wstrb   (),
    .wr_axi_wvalid  (),
    .wr_axi_wready  (),
    .wr_axi_bresp   (),
    .wr_axi_bvalid  (),
    .wr_axi_bready  (),
    .wr_axi_araddr  (),
    .wr_axi_arvalid (),
    .wr_axi_arready (),
    .wr_axi_rdata   (),
    .wr_axi_rresp   (),
    .wr_axi_rvalid  (),
    .wr_axi_rready  (),
    .wr_axi_rlast   (),

    .port_info      (),
    .device_id      (device_id),

    .link_up        (qsfp_link_up),
    .activity       ()
  );

  assign QSFP_I2C_SCL = qsfp_scl_t ? 1'bz : qsfp_scl_o;
  assign qsfp_scl_i   = QSFP_I2C_SCL;
  assign QSFP_I2C_SDA = qsfp_sda_t ? 1'bz : qsfp_sda_o;
  assign qsfp_sda_i   = QSFP_I2C_SDA;

  assign QSFP_LED = |qsfp_link_up;
`else

  axi_dummy #(
    .DEC_ERR(1'b0)
  ) inst_axi_dummy_qsfp (
    .s_axi_aclk(bus_clk),
    .s_axi_areset(bus_rst),

    .s_axi_awaddr(M_AXI_NET2_AWADDR),
    .s_axi_awvalid(M_AXI_NET2_AWVALID),
    .s_axi_awready(M_AXI_NET2_AWREADY),

    .s_axi_wdata(M_AXI_NET2_WDATA),
    .s_axi_wvalid(M_AXI_NET2_WVALID),
    .s_axi_wready(M_AXI_NET2_WREADY),

    .s_axi_bresp(M_AXI_NET2_BRESP),
    .s_axi_bvalid(M_AXI_NET2_BVALID),
    .s_axi_bready(M_AXI_NET2_BREADY),

    .s_axi_araddr(M_AXI_NET2_ARADDR),
    .s_axi_arvalid(M_AXI_NET2_ARVALID),
    .s_axi_arready(M_AXI_NET2_ARREADY),

    .s_axi_rdata(M_AXI_NET2_RDATA),
    .s_axi_rresp(M_AXI_NET2_RRESP),
    .s_axi_rvalid(M_AXI_NET2_RVALID),
    .s_axi_rready(M_AXI_NET2_RREADY)

  );

  assign qsfp_scl_i = qsfp_scl_t ? 1'b1 : qsfp_scl_o;
  assign qsfp_sda_i = qsfp_sda_t ? 1'b1 : qsfp_sda_o;

`endif

  //--------------------------------------------------------------
  // NPIO MGT Lanes (Example loopback config)
  //--------------------------------------------------------------

`ifdef NPIO_LANES

  wire [127:0]  npio_loopback_tdata;
  wire [1:0]    npio_loopback_tvalid;
  wire [1:0]    npio_loopback_tready;
  wire [1:0]    npio_loopback_tlast;

  n3xx_mgt_io_core #(
    .PROTOCOL       ("Aurora"),
    .REG_BASE       (14'h00),             // Base offset removed by n3xx_core
    .REG_DWIDTH     (REG_DWIDTH),         // Width of the AXI4-Lite data bus (must be 32 or 64)
    .REG_AWIDTH     (REG_AWIDTH),         // Width of the address bus
    .PORTNUM        (8'd2),
    .MDIO_EN        (0)
  ) npio_ln_0_i (
    .areset         (global_rst),
    .gt_refclk      (aurora_refclk),
    .gb_refclk      (aurora_clk156),
    .misc_clk       (aurora_init_clk),
    .user_clk       (au_user_clk),
    .sync_clk       (au_sync_clk),
    .gt_tx_out_clk_unbuf(npio0_tx_out_clk),

    .bus_clk        (bus_clk),//clk for status reg reads to mdio interface
    .bus_rst        (bus_rst),
    .qpllreset      (qpllreset_npio0),
    .qplloutclk     (qplloutclk),
    .qplloutrefclk  (qplloutrefclk),
    .qplllock       (qplllock),
    .qpllrefclklost (),

    .rxp            (NPIO_RX0_P),
    .rxn            (NPIO_RX0_N),
    .txp            (NPIO_TX0_P),
    .txn            (NPIO_TX0_N),

    .sfpp_rxlos     (1'b0),
    .sfpp_tx_fault  (1'b0),

    //RegPort
    .reg_wr_req     (reg_wr_req_npio),
    .reg_wr_addr    (reg_wr_addr_npio),
    .reg_wr_data    (reg_wr_data_npio),
    .reg_rd_req     (reg_rd_req_npio),
    .reg_rd_addr    (reg_rd_addr_npio),
    .reg_rd_resp    (reg_rd_resp_npio0),
    .reg_rd_data    (reg_rd_data_npio0),

    //DATA (loopback mode)
    .s_axis_tdata   (npio_loopback_tdata[63:0]), //Data to aurora core
    .s_axis_tuser   (4'b0),
    .s_axis_tvalid  (npio_loopback_tvalid[0]),
    .s_axis_tlast   (npio_loopback_tlast[0]),
    .s_axis_tready  (npio_loopback_tready[0]),
    .m_axis_tdata   (npio_loopback_tdata[63:0]), //Data from aurora core
    .m_axis_tuser   (),
    .m_axis_tvalid  (npio_loopback_tvalid[0]),
    .m_axis_tlast   (npio_loopback_tlast[0]),
    .m_axis_tready  (npio_loopback_tready[0]),

    .mmcm_locked    (au_mmcm_locked),
    .gt_pll_lock    (npio0_gt_pll_lock)
  );

  n3xx_mgt_io_core #(
    .PROTOCOL       ("Aurora"),
    .REG_BASE       (14'h40),             // Base offset removed by n3xx_core
    .REG_DWIDTH     (REG_DWIDTH),         // Width of the AXI4-Lite data bus (must be 32 or 64)
    .REG_AWIDTH     (REG_AWIDTH),         // Width of the address bus
    .PORTNUM        (8'd3),
    .MDIO_EN        (0)
  ) npio_ln_1_i (
    .areset         (global_rst),
    .gt_refclk      (aurora_refclk),
    .gb_refclk      (aurora_clk156),
    .misc_clk       (aurora_init_clk),
    .user_clk       (au_user_clk),
    .sync_clk       (au_sync_clk),
    .gt_tx_out_clk_unbuf(npio1_tx_out_clk),

    .bus_clk        (bus_clk),//clk for status reg reads to mdio interface
    .bus_rst        (bus_rst),
    .qpllreset      (qpllreset_npio1),
    .qplloutclk     (qplloutclk),
    .qplloutrefclk  (qplloutrefclk),
    .qplllock       (qplllock),
    .qpllrefclklost (),

    .rxp            (NPIO_RX1_P),
    .rxn            (NPIO_RX1_N),
    .txp            (NPIO_TX1_P),
    .txn            (NPIO_TX1_N),

    .sfpp_rxlos     (1'b0),
    .sfpp_tx_fault  (1'b0),

    //RegPort
    .reg_wr_req     (reg_wr_req_npio),
    .reg_wr_addr    (reg_wr_addr_npio),
    .reg_wr_data    (reg_wr_data_npio),
    .reg_rd_req     (reg_rd_req_npio),
    .reg_rd_addr    (reg_rd_addr_npio),
    .reg_rd_resp    (reg_rd_resp_npio1),
    .reg_rd_data    (reg_rd_data_npio1),

    //DATA (loopback mode)
    .s_axis_tdata   (npio_loopback_tdata[127:64]), //Data to aurora core
    .s_axis_tuser   (4'b0),
    .s_axis_tvalid  (npio_loopback_tvalid[1]),
    .s_axis_tlast   (npio_loopback_tlast[1]),
    .s_axis_tready  (npio_loopback_tready[1]),
    .m_axis_tdata   (npio_loopback_tdata[127:64]), //Data from aurora core
    .m_axis_tuser   (),
    .m_axis_tvalid  (npio_loopback_tvalid[1]),
    .m_axis_tlast   (npio_loopback_tlast[1]),
    .m_axis_tready  (npio_loopback_tready[1]),

    .mmcm_locked    (au_mmcm_locked),
    .gt_pll_lock    (npio1_gt_pll_lock)
  );

`else

  assign reg_rd_resp_npio0 = 1'b0;
  assign reg_rd_data_npio0 = 'h0;
  assign reg_rd_resp_npio1 = 1'b0;
  assign reg_rd_data_npio1 = 'h0;
  assign npio0_gt_pll_lock = 1'b1;
  assign npio1_gt_pll_lock = 1'b1;
  assign qpllreset_npio0   = 1'b0;
  assign qpllreset_npio1   = 1'b0;

`endif


  // ARM ethernet 0 bridge signals
  wire [63:0] arm_eth0_tx_tdata;
  wire        arm_eth0_tx_tvalid;
  wire        arm_eth0_tx_tlast;
  wire        arm_eth0_tx_tready;
  wire [3:0]  arm_eth0_tx_tuser;
  wire [7:0]  arm_eth0_tx_tkeep;

  wire [63:0] arm_eth0_tx_tdata_b;
  wire        arm_eth0_tx_tvalid_b;
  wire        arm_eth0_tx_tlast_b;
  wire        arm_eth0_tx_tready_b;
  wire [3:0]  arm_eth0_tx_tuser_b;
  wire [7:0]  arm_eth0_tx_tkeep_b;

  wire [63:0] arm_eth_sfp0_tx_tdata_b;
  wire        arm_eth_sfp0_tx_tvalid_b;
  wire        arm_eth_sfp0_tx_tlast_b;
  wire        arm_eth_sfp0_tx_tready_b;
  wire [3:0]  arm_eth_sfp0_tx_tuser_b;
  wire [7:0]  arm_eth_sfp0_tx_tkeep_b;

  wire [63:0] arm_eth0_rx_tdata;
  wire        arm_eth0_rx_tvalid;
  wire        arm_eth0_rx_tlast;
  wire        arm_eth0_rx_tready;
  wire [3:0]  arm_eth0_rx_tuser;
  wire [7:0]  arm_eth0_rx_tkeep;

  wire [63:0] arm_eth0_rx_tdata_b;
  wire        arm_eth0_rx_tvalid_b;
  wire        arm_eth0_rx_tlast_b;
  wire        arm_eth0_rx_tready_b;
  wire [3:0]  arm_eth0_rx_tuser_b;
  wire [7:0]  arm_eth0_rx_tkeep_b;

  wire [63:0] arm_eth_sfp0_rx_tdata_b;
  wire        arm_eth_sfp0_rx_tvalid_b;
  wire        arm_eth_sfp0_rx_tlast_b;
  wire        arm_eth_sfp0_rx_tready_b;
  wire [3:0]  arm_eth_sfp0_rx_tuser_b;
  wire [7:0]  arm_eth_sfp0_rx_tkeep_b;

  wire        arm_eth0_rx_irq;
  wire        arm_eth0_tx_irq;

  // ARM ethernet 1 bridge signals
  wire [63:0] arm_eth1_tx_tdata;
  wire        arm_eth1_tx_tvalid;
  wire        arm_eth1_tx_tlast;
  wire        arm_eth1_tx_tready;
  wire [3:0]  arm_eth1_tx_tuser;
  wire [7:0]  arm_eth1_tx_tkeep;

  wire [63:0] arm_eth1_tx_tdata_b;
  wire        arm_eth1_tx_tvalid_b;
  wire        arm_eth1_tx_tlast_b;
  wire        arm_eth1_tx_tready_b;
  wire [3:0]  arm_eth1_tx_tuser_b;
  wire [7:0]  arm_eth1_tx_tkeep_b;

  wire [63:0] arm_eth_sfp1_tx_tdata_b;
  wire        arm_eth_sfp1_tx_tvalid_b;
  wire        arm_eth_sfp1_tx_tlast_b;
  wire        arm_eth_sfp1_tx_tready_b;
  wire [3:0]  arm_eth_sfp1_tx_tuser_b;
  wire [7:0]  arm_eth_sfp1_tx_tkeep_b;

  wire [63:0] arm_eth1_rx_tdata;
  wire        arm_eth1_rx_tvalid;
  wire        arm_eth1_rx_tlast;
  wire        arm_eth1_rx_tready;
  wire [3:0]  arm_eth1_rx_tuser;
  wire [7:0]  arm_eth1_rx_tkeep;

  wire [63:0] arm_eth1_rx_tdata_b;
  wire        arm_eth1_rx_tvalid_b;
  wire        arm_eth1_rx_tlast_b;
  wire        arm_eth1_rx_tready_b;
  wire [3:0]  arm_eth1_rx_tuser_b;
  wire [7:0]  arm_eth1_rx_tkeep_b;

  wire [63:0] arm_eth_sfp1_rx_tdata_b;
  wire        arm_eth_sfp1_rx_tvalid_b;
  wire        arm_eth_sfp1_rx_tlast_b;
  wire        arm_eth_sfp1_rx_tready_b;
  wire [3:0]  arm_eth_sfp1_rx_tuser_b;
  wire [7:0]  arm_eth_sfp1_rx_tkeep_b;

  wire        arm_eth1_tx_irq;
  wire        arm_eth1_rx_irq;

  // Vita to Ethernet
  wire  [63:0]  v2e0_tdata;
  wire          v2e0_tlast;
  wire          v2e0_tvalid;
  wire          v2e0_tready;

  wire  [63:0]  v2e1_tdata;
  wire          v2e1_tlast;
  wire          v2e1_tvalid;
  wire          v2e1_tready;

  wire  [63:0]  v2e_sfp0_tdata;
  wire          v2e_sfp0_tlast;
  wire          v2e_sfp0_tvalid;
  wire          v2e_sfp0_tready;

  wire  [63:0]  v2e_sfp1_tdata;
  wire          v2e_sfp1_tlast;
  wire          v2e_sfp1_tvalid;
  wire          v2e_sfp1_tready;

  // Ethernet to Vita
  wire  [63:0]  e2v0_tdata;
  wire          e2v0_tlast;
  wire          e2v0_tvalid;
  wire          e2v0_tready;

  wire  [63:0]  e2v1_tdata;
  wire          e2v1_tlast;
  wire          e2v1_tvalid;
  wire          e2v1_tready;

  wire  [63:0]  e2v_sfp0_tdata;
  wire          e2v_sfp0_tlast;
  wire          e2v_sfp0_tvalid;
  wire          e2v_sfp0_tready;

  wire  [63:0]  e2v_sfp1_tdata;
  wire          e2v_sfp1_tlast;
  wire          e2v_sfp1_tvalid;
  wire          e2v_sfp1_tready;

  // Ethernet crossover
  wire  [63:0]  e01_tdata, e10_tdata;
  wire  [3:0]   e01_tuser, e10_tuser;
  wire          e01_tlast, e01_tvalid, e01_tready;
  wire          e10_tlast, e10_tvalid, e10_tready;


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

  // Misc
  wire [31:0] sfp_port0_info;
  wire [31:0] sfp_port1_info;
  wire sfp0_link_up, sfp1_link_up;
  wire [15:0] device_id;

  /////////////////////////////////////////////////////////////////////
  //
  // SFP Wrapper 0: Network Interface (1/10G or Aurora)
  //
  //////////////////////////////////////////////////////////////////////

  n3xx_mgt_channel_wrapper #(
    .LANES(1),
  `ifdef SFP0_10GBE
    .PROTOCOL("10GbE"),
    .MDIO_EN(1'b1),
    .MDIO_PHYADDR(5'd4), // PHYADDR must match the "reg" property for PHY in DTS file
  `elsif SFP0_AURORA
    .PROTOCOL("Aurora"),
    .MDIO_EN(1'b0),
  `elsif SFP0_1GBE
    .PROTOCOL("1GbE"),
    .MDIO_EN(1'b1),
    .MDIO_PHYADDR(5'd4), // PHYADDR must match the "reg" property for PHY in DTS file
  `elsif SFP0_WR
    .PROTOCOL("WhiteRabbit"),
    .MDIO_EN(1'b0),
  `endif
    .REG_DWIDTH(REG_DWIDTH), // Width of the AXI4-Lite data bus (must be 32 or 64)
    .REG_AWIDTH(REG_AWIDTH), // Width of the address bus
    .PORTNUM_BASE(8'd0)
   ) sfp_wrapper_0 (
     .areset(global_rst),
     .gt_refclk(sfp0_gt_refclk),
     .gb_refclk(sfp0_gb_refclk),
     .misc_clk(sfp0_misc_clk),
     .user_clk(au_user_clk),
     .sync_clk(au_sync_clk),
     .gt_tx_out_clk_unbuf(sfp0_tx_out_clk),

     .bus_rst(bus_rst),
     .bus_clk(bus_clk),

     .qpllreset(qpllreset_sfp0),
     .qplllock(qplllock),
     .qplloutclk(qplloutclk),
     .qplloutrefclk(qplloutrefclk),
     .qpllrefclklost(),

     .mmcm_locked(au_mmcm_locked),
     .gt_pll_lock(sfp0_gt_pll_lock),

     .txp(SFP_0_TX_P),
     .txn(SFP_0_TX_N),
     .rxp(SFP_0_RX_P),
     .rxn(SFP_0_RX_N),

     .mod_present_n(SFP_0_I2C_NPRESENT),
     .mod_rxlos(SFP_0_LOS),
     .mod_tx_fault(SFP_0_TXFAULT),
     .mod_tx_disable(SFP_0_TXDISABLE),

     // Clock and reset
     .s_axi_aclk(clk40),
     .s_axi_aresetn(clk40_rstn),
     // AXI4-Lite: Write address port (domain: s_axi_aclk)
     .s_axi_awaddr(M_AXI_NET0_AWADDR[REG_AWIDTH-1:0]),
     .s_axi_awvalid(M_AXI_NET0_AWVALID),
     .s_axi_awready(M_AXI_NET0_AWREADY),
     // AXI4-Lite: Write data port (domain: s_axi_aclk)
     .s_axi_wdata(M_AXI_NET0_WDATA),
     .s_axi_wstrb(M_AXI_NET0_WSTRB),
     .s_axi_wvalid(M_AXI_NET0_WVALID),
     .s_axi_wready(M_AXI_NET0_WREADY),
     // AXI4-Lite: Write response port (domain: s_axi_aclk)
     .s_axi_bresp(M_AXI_NET0_BRESP),
     .s_axi_bvalid(M_AXI_NET0_BVALID),
     .s_axi_bready(M_AXI_NET0_BREADY),
     // AXI4-Lite: Read address port (domain: s_axi_aclk)
     .s_axi_araddr(M_AXI_NET0_ARADDR[REG_AWIDTH-1:0]),
     .s_axi_arvalid(M_AXI_NET0_ARVALID),
     .s_axi_arready(M_AXI_NET0_ARREADY),
     // AXI4-Lite: Read data port (domain: s_axi_aclk)
     .s_axi_rdata(M_AXI_NET0_RDATA),
     .s_axi_rresp(M_AXI_NET0_RRESP),
     .s_axi_rvalid(M_AXI_NET0_RVALID),
     .s_axi_rready(M_AXI_NET0_RREADY),

     // Ethernet to Vita
     .e2v_tdata(e2v_sfp0_tdata),
     .e2v_tlast(e2v_sfp0_tlast),
     .e2v_tvalid(e2v_sfp0_tvalid),
     .e2v_tready(e2v_sfp0_tready),

     // Vita to Ethernet
     .v2e_tdata(v2e_sfp0_tdata),
     .v2e_tlast(v2e_sfp0_tlast),
     .v2e_tvalid(v2e_sfp0_tvalid),
     .v2e_tready(v2e_sfp0_tready),

     // Ethernet to CPU
     .e2c_tdata(arm_eth_sfp0_rx_tdata_b),
     .e2c_tkeep(arm_eth_sfp0_rx_tkeep_b),
     .e2c_tlast(arm_eth_sfp0_rx_tlast_b),
     .e2c_tvalid(arm_eth_sfp0_rx_tvalid_b),
     .e2c_tready(arm_eth_sfp0_rx_tready_b),

     // CPU to Ethernet
     .c2e_tdata(arm_eth_sfp0_tx_tdata_b),
     .c2e_tkeep(arm_eth_sfp0_tx_tkeep_b),
     .c2e_tlast(arm_eth_sfp0_tx_tlast_b),
     .c2e_tvalid(arm_eth_sfp0_tx_tvalid_b),
     .c2e_tready(arm_eth_sfp0_tx_tready_b),

      // White Rabbit Specific
`ifdef SFP0_WR
     .wr_reset_n   (~ps_gpio_out[48]), // reset for WR only
     .wr_refclk    (wr_refclk_buf),
     .wr_dac_sclk  (WB_DAC_SCLK),
     .wr_dac_din   (WB_DAC_DIN),
     .wr_dac_clr_n (WB_DAC_NCLR),
     .wr_dac_cs_n  (WB_DAC_NSYNC),
     .wr_dac_ldac_n(WB_DAC_NLDAC),
     .wr_eeprom_scl_o(), // storage for delay characterization
     .wr_eeprom_scl_i(1'b0), // temp
     .wr_eeprom_sda_o(),
     .wr_eeprom_sda_i(1'b0), // temp
     .wr_uart_rx(wr_uart_rxd), // to/from PS
     .wr_uart_tx(wr_uart_txd),
     .mod_pps(pps_wr_refclk), // out, reference clock and pps
     .mod_refclk(wr_ref_clk),
     // WR Slave Port to PS
     .wr_axi_aclk(m_axi_wr_clk), // out to PS
     .wr_axi_aresetn(1'b1), // in
     .wr_axi_awaddr(m_axi_wr_awaddr),
     .wr_axi_awvalid(m_axi_wr_awvalid),
     .wr_axi_awready(m_axi_wr_awready),
     .wr_axi_wdata(m_axi_wr_wdata),
     .wr_axi_wstrb(m_axi_wr_wstrb),
     .wr_axi_wvalid(m_axi_wr_wvalid),
     .wr_axi_wready(m_axi_wr_wready),
     .wr_axi_bresp(m_axi_wr_bresp),
     .wr_axi_bvalid(m_axi_wr_bvalid),
     .wr_axi_bready(m_axi_wr_bready),
     .wr_axi_araddr(m_axi_wr_araddr),
     .wr_axi_arvalid(m_axi_wr_arvalid),
     .wr_axi_arready(m_axi_wr_arready),
     .wr_axi_rdata(m_axi_wr_rdata),
     .wr_axi_rresp(m_axi_wr_rresp),
     .wr_axi_rvalid(m_axi_wr_rvalid),
     .wr_axi_rready(m_axi_wr_rready),
     .wr_axi_rlast(),
`else
     .wr_reset_n(1'b1),
     .wr_refclk(1'b0),
     .wr_eeprom_scl_i(1'b0),
     .wr_eeprom_sda_i(1'b0),
     .wr_uart_rx(1'b0),
`endif

     // Misc
     .port_info(sfp_port0_info),
     .device_id(device_id),

     // LED
     .link_up(sfp0_link_up),
     .activity(SFP_0_LED_A)
   );

   assign ps_gpio_in[60] = ps_gpio_tri[60] ? sfp0_link_up : ps_gpio_out[60];
   assign SFP_0_LED_B = sfp0_link_up;

`ifndef SFP0_WR
  assign WB_DAC_SCLK  = 1'b0;
  assign WB_DAC_DIN   = 1'b0;
  assign WB_DAC_NCLR  = 1'b1;
  assign WB_DAC_NSYNC = 1'b1;
  assign WB_DAC_NLDAC = 1'b1;
  assign pps_wr_refclk = 1'b0;
  assign wr_ref_clk = 1'b0;
`endif


   /////////////////////////////////////////////////////////////////////
   //
   // SFP Wrapper 1: Network Interface (1/10G or Aurora)
   //
   //////////////////////////////////////////////////////////////////////

   n3xx_mgt_channel_wrapper #(
    .LANES(1),
  `ifdef SFP1_10GBE
    .PROTOCOL("10GbE"),
    .MDIO_EN(1'b1),
    .MDIO_PHYADDR(5'd4), // PHYADDR must match the "reg" property for PHY in DTS file
  `elsif SFP1_AURORA
    .PROTOCOL("Aurora"),
    .MDIO_EN(1'b0),
  `endif
    .REG_DWIDTH(REG_DWIDTH),     // Width of the AXI4-Lite data bus (must be 32 or 64)
    .REG_AWIDTH(REG_AWIDTH),     // Width of the address bus
    .PORTNUM_BASE(8'd1)
   ) sfp_wrapper_1 (
     .areset(global_rst),

     .gt_refclk(sfp1_gt_refclk),
     .gb_refclk(sfp1_gb_refclk),
     .misc_clk(sfp1_misc_clk),
     .user_clk(au_user_clk),
     .sync_clk(au_sync_clk),
     .gt_tx_out_clk_unbuf(sfp1_tx_out_clk),

     .bus_rst(bus_rst),
     .bus_clk(bus_clk),

     .qpllreset(qpllreset_sfp1),
     .qplllock(qplllock),
     .qplloutclk(qplloutclk),
     .qplloutrefclk(qplloutrefclk),
     .qpllrefclklost(),

     .mmcm_locked(au_mmcm_locked),
     .gt_pll_lock(sfp1_gt_pll_lock),

     .txp(SFP_1_TX_P),
     .txn(SFP_1_TX_N),
     .rxp(SFP_1_RX_P),
     .rxn(SFP_1_RX_N),

     .mod_rxlos(SFP_1_LOS),
     .mod_tx_fault(SFP_1_TXFAULT),
     .mod_tx_disable(SFP_1_TXDISABLE),

     // Clock and reset
     .s_axi_aclk(clk40),
     .s_axi_aresetn(clk40_rstn),
     // AXI4-Lite: Write address port (domain: s_axi_aclk)
     .s_axi_awaddr(M_AXI_NET1_AWADDR[REG_AWIDTH-1:0]),
     .s_axi_awvalid(M_AXI_NET1_AWVALID),
     .s_axi_awready(M_AXI_NET1_AWREADY),
     // AXI4-Lite: Write data port (domain: s_axi_aclk)
     .s_axi_wdata(M_AXI_NET1_WDATA),
     .s_axi_wstrb(M_AXI_NET1_WSTRB),
     .s_axi_wvalid(M_AXI_NET1_WVALID),
     .s_axi_wready(M_AXI_NET1_WREADY),
     // AXI4-Lite: Write response port (domain: s_axi_aclk)
     .s_axi_bresp(M_AXI_NET1_BRESP),
     .s_axi_bvalid(M_AXI_NET1_BVALID),
     .s_axi_bready(M_AXI_NET1_BREADY),
     // AXI4-Lite: Read address port (domain: s_axi_aclk)
     .s_axi_araddr(M_AXI_NET1_ARADDR[REG_AWIDTH-1:0]),
     .s_axi_arvalid(M_AXI_NET1_ARVALID),
     .s_axi_arready(M_AXI_NET1_ARREADY),
     // AXI4-Lite: Read data port (domain: s_axi_aclk)
     .s_axi_rdata(M_AXI_NET1_RDATA),
     .s_axi_rresp(M_AXI_NET1_RRESP),
     .s_axi_rvalid(M_AXI_NET1_RVALID),
     .s_axi_rready(M_AXI_NET1_RREADY),

     // Ethernet to Vita
     .e2v_tdata(e2v_sfp1_tdata),
     .e2v_tlast(e2v_sfp1_tlast),
     .e2v_tvalid(e2v_sfp1_tvalid),
     .e2v_tready(e2v_sfp1_tready),

     // Vita to Ethernet
     .v2e_tdata(v2e_sfp1_tdata),
     .v2e_tlast(v2e_sfp1_tlast),
     .v2e_tvalid(v2e_sfp1_tvalid),
     .v2e_tready(v2e_sfp1_tready),

     // Ethernet to CPU
     .e2c_tdata(arm_eth_sfp1_rx_tdata_b),
     .e2c_tkeep(arm_eth_sfp1_rx_tkeep_b),
     .e2c_tlast(arm_eth_sfp1_rx_tlast_b),
     .e2c_tvalid(arm_eth_sfp1_rx_tvalid_b),
     .e2c_tready(arm_eth_sfp1_rx_tready_b),

     // CPU to Ethernet
     .c2e_tdata(arm_eth_sfp1_tx_tdata_b),
     .c2e_tkeep(arm_eth_sfp1_tx_tkeep_b),
     .c2e_tlast(arm_eth_sfp1_tx_tlast_b),
     .c2e_tvalid(arm_eth_sfp1_tx_tvalid_b),
     .c2e_tready(arm_eth_sfp1_tx_tready_b),

     // Misc
     .port_info(sfp_port1_info),
     .device_id(device_id),

     // LED
     .link_up(sfp1_link_up),
     .activity(SFP_1_LED_A)
   );

   assign ps_gpio_in[61] = ps_gpio_tri[61] ? sfp1_link_up : ps_gpio_out[61];
   assign SFP_1_LED_B = sfp1_link_up;

  /////////////////////////////////////////////////////////////////////
  //
  // Ethernet DMA 0
  //
  //////////////////////////////////////////////////////////////////////

  assign  IRQ_F2P[0] = arm_eth0_rx_irq;
  assign  IRQ_F2P[1] = arm_eth0_tx_irq;


`ifdef QSFP_10GBE
  // QSFP+ lanes connect to DMA engines and crossbar
  // Connect first QSFP+ 10 GbE port to a DMA engine (and the PS/ARM)
  assign arm_eth_qsfp_tx_tdata_b[0*64 +: 64] = arm_eth0_tx_tdata_b;
  assign arm_eth_qsfp_tx_tvalid_b[0]         = arm_eth0_tx_tvalid_b;
  assign arm_eth_qsfp_tx_tlast_b[0]          = arm_eth0_tx_tlast_b;
  assign arm_eth0_tx_tready_b                = arm_eth_qsfp_tx_tready_b[0];
  assign arm_eth_qsfp_tx_tuser_b[0*4 +: 4]   = arm_eth0_tx_tuser_b;
  assign arm_eth_qsfp_tx_tkeep_b[0*8 +: 8]   = arm_eth0_tx_tkeep_b;

  assign arm_eth0_rx_tdata_b         = arm_eth_qsfp_rx_tdata_b[0*64 +: 64];
  assign arm_eth0_rx_tvalid_b        = arm_eth_qsfp_rx_tvalid_b[0];
  assign arm_eth0_rx_tlast_b         = arm_eth_qsfp_rx_tlast_b[0];
  assign arm_eth_qsfp_rx_tready_b[0] = arm_eth0_rx_tready_b;
  assign arm_eth0_rx_tuser_b         = arm_eth_qsfp_rx_tuser_b[0*4 +: 4];
  assign arm_eth0_rx_tkeep_b         = arm_eth_qsfp_rx_tkeep_b[0*8 +: 8];

  // Connect first QSFP+ 10 GbE port to the crossbar
  assign v2e_qsfp_tdata[0*64 +: 64] = v2e0_tdata;
  assign v2e_qsfp_tlast[0]          = v2e0_tlast;
  assign v2e_qsfp_tvalid[0]         = v2e0_tvalid;
  assign v2e0_tready                = v2e_qsfp_tready[0];

  assign e2v0_tdata                 = e2v_qsfp_tdata[0*64 +: 64];
  assign e2v0_tlast                 = e2v_qsfp_tlast[0];
  assign e2v0_tvalid                = e2v_qsfp_tvalid[0];
  assign e2v_qsfp_tready[0]         = e2v0_tready;

  // Connect second QSFP+ 10 GbE port to a DMA engine (and the PS/ARM)
  assign arm_eth_qsfp_tx_tdata_b[1*64 +: 64] = arm_eth1_tx_tdata_b;
  assign arm_eth_qsfp_tx_tvalid_b[1]         = arm_eth1_tx_tvalid_b;
  assign arm_eth_qsfp_tx_tlast_b[1]          = arm_eth1_tx_tlast_b;
  assign arm_eth1_tx_tready_b                = arm_eth_qsfp_tx_tready_b[1];
  assign arm_eth_qsfp_tx_tuser_b[1*4 +: 4]   = arm_eth1_tx_tuser_b;
  assign arm_eth_qsfp_tx_tkeep_b[1*8 +: 8]   = arm_eth1_tx_tkeep_b;

  assign arm_eth1_rx_tdata_b         = arm_eth_qsfp_rx_tdata_b[1*64 +: 64];
  assign arm_eth1_rx_tvalid_b        = arm_eth_qsfp_rx_tvalid_b[1];
  assign arm_eth1_rx_tlast_b         = arm_eth_qsfp_rx_tlast_b[1];
  assign arm_eth_qsfp_rx_tready_b[1] = arm_eth1_rx_tready_b;
  assign arm_eth1_rx_tuser_b         = arm_eth_qsfp_rx_tuser_b[1*4 +: 4];
  assign arm_eth1_rx_tkeep_b         = arm_eth_qsfp_rx_tkeep_b[1*8 +: 8];

  // Connect second QSFP+ 10 GbE port to the crossbar
  assign v2e_qsfp_tdata[1*64 +: 64] = v2e1_tdata;
  assign v2e_qsfp_tlast[1]          = v2e1_tlast;
  assign v2e_qsfp_tvalid[1]         = v2e1_tvalid;
  assign v2e1_tready                = v2e_qsfp_tready[1];

  assign e2v1_tdata                 = e2v_qsfp_tdata[1*64 +: 64];
  assign e2v1_tlast                 = e2v_qsfp_tlast[1];
  assign e2v1_tvalid                = e2v_qsfp_tvalid[1];
  assign e2v_qsfp_tready[1]         = e2v1_tready;
`else
  // SFP+ ports connects to DMA engines and crossbar
  // Connect first SFP+ 10 GbE port to a DMA engine (and the PS/ARM)
  assign arm_eth_sfp0_tx_tdata_b  = arm_eth0_tx_tdata_b;
  assign arm_eth_sfp0_tx_tvalid_b = arm_eth0_tx_tvalid_b;
  assign arm_eth_sfp0_tx_tlast_b  = arm_eth0_tx_tlast_b;
  assign arm_eth0_tx_tready_b     = arm_eth_sfp0_tx_tready_b;
  assign arm_eth_sfp0_tx_tuser_b  = arm_eth0_tx_tuser_b;
  assign arm_eth_sfp0_tx_tkeep_b  = arm_eth0_tx_tkeep_b;

  assign arm_eth0_rx_tdata_b      = arm_eth_sfp0_rx_tdata_b;
  assign arm_eth0_rx_tvalid_b     = arm_eth_sfp0_rx_tvalid_b;
  assign arm_eth0_rx_tlast_b      = arm_eth_sfp0_rx_tlast_b;
  assign arm_eth_sfp0_rx_tready_b = arm_eth0_rx_tready_b;
  assign arm_eth0_rx_tuser_b      = arm_eth_sfp0_rx_tuser_b;
  assign arm_eth0_rx_tkeep_b      = arm_eth_sfp0_rx_tkeep_b;

  // Connect first SFP+ 10 GbE port to the crossbar
  assign v2e_sfp0_tdata  = v2e0_tdata;
  assign v2e_sfp0_tlast  = v2e0_tlast;
  assign v2e_sfp0_tvalid = v2e0_tvalid;
  assign v2e0_tready     = v2e_sfp0_tready;

  assign e2v0_tdata      = e2v_sfp0_tdata;
  assign e2v0_tlast      = e2v_sfp0_tlast;
  assign e2v0_tvalid     = e2v_sfp0_tvalid;
  assign e2v_sfp0_tready = e2v0_tready;

  // Connect second SFP+ 10 GbE port to a DMA engine (and the PS/ARM)
  assign arm_eth_sfp1_tx_tdata_b  = arm_eth1_tx_tdata_b;
  assign arm_eth_sfp1_tx_tvalid_b = arm_eth1_tx_tvalid_b;
  assign arm_eth_sfp1_tx_tlast_b  = arm_eth1_tx_tlast_b;
  assign arm_eth1_tx_tready_b     = arm_eth_sfp1_tx_tready_b;
  assign arm_eth_sfp1_tx_tuser_b  = arm_eth1_tx_tuser_b;
  assign arm_eth_sfp1_tx_tkeep_b  = arm_eth1_tx_tkeep_b;

  assign arm_eth1_rx_tdata_b      = arm_eth_sfp1_rx_tdata_b;
  assign arm_eth1_rx_tvalid_b     = arm_eth_sfp1_rx_tvalid_b;
  assign arm_eth1_rx_tlast_b      = arm_eth_sfp1_rx_tlast_b;
  assign arm_eth_sfp1_rx_tready_b = arm_eth1_rx_tready_b;
  assign arm_eth1_rx_tuser_b      = arm_eth_sfp1_rx_tuser_b;
  assign arm_eth1_rx_tkeep_b      = arm_eth_sfp1_rx_tkeep_b;

  // Connect first SFP+ 10 GbE port to the crossbar
  assign v2e_sfp1_tdata  = v2e1_tdata;
  assign v2e_sfp1_tlast  = v2e1_tlast;
  assign v2e_sfp1_tvalid = v2e1_tvalid;
  assign v2e1_tready     = v2e_sfp1_tready;

  assign e2v1_tdata      = e2v_sfp1_tdata;
  assign e2v1_tlast      = e2v_sfp1_tlast;
  assign e2v1_tvalid     = e2v_sfp1_tvalid;
  assign e2v_sfp1_tready = e2v1_tready;

  // Don't actually instantiate DMA engines if protocols can't use them
  `ifdef SFP0_AURORA
    `define NO_ETH_DMA_0
  `elsif SFP0_WR
    `define NO_ETH_DMA_0
  `endif

  `ifdef SFP1_AURORA
    `define NO_ETH_DMA_1
  `endif
`endif

`ifdef NO_ETH_DMA_0
  //If inst Aurora, tie off each axi/axi-lite interface
  axi_dummy #(
    .DEC_ERR(1'b0)
  ) inst_axi_dummy_sfp0_eth_dma (
    .s_axi_aclk(bus_clk),
    .s_axi_areset(bus_rst),

    .s_axi_awaddr(M_AXI_ETH_DMA0_AWADDR),
    .s_axi_awvalid(M_AXI_ETH_DMA0_AWVALID),
    .s_axi_awready(M_AXI_ETH_DMA0_AWREADY),

    .s_axi_wdata(M_AXI_ETH_DMA0_WDATA),
    .s_axi_wvalid(M_AXI_ETH_DMA0_WVALID),
    .s_axi_wready(M_AXI_ETH_DMA0_WREADY),

    .s_axi_bresp(M_AXI_ETH_DMA0_BRESP),
    .s_axi_bvalid(M_AXI_ETH_DMA0_BVALID),
    .s_axi_bready(M_AXI_ETH_DMA0_BREADY),

    .s_axi_araddr(M_AXI_ETH_DMA0_ARADDR),
    .s_axi_arvalid(M_AXI_ETH_DMA0_ARVALID),
    .s_axi_arready(M_AXI_ETH_DMA0_ARREADY),

    .s_axi_rdata(M_AXI_ETH_DMA0_RDATA),
    .s_axi_rresp(M_AXI_ETH_DMA0_RRESP),
    .s_axi_rvalid(M_AXI_ETH_DMA0_RVALID),
    .s_axi_rready(M_AXI_ETH_DMA0_RREADY)

  );
  //S_AXI_GP0 outputs from axi_eth_dma, so needs some sort of controller/tie off
  assign S_AXI_GP0_AWADDR = 32'h0;
  assign S_AXI_GP0_AWLEN = 8'h0;
  assign S_AXI_GP0_AWSIZE = 4'h0;
  assign S_AXI_GP0_AWBURST = 3'h0;
  assign S_AXI_GP0_AWPROT = 3'h0;
  assign S_AXI_GP0_AWCACHE = 4'h0;
  assign S_AXI_GP0_AWVALID = 1'b0;
  //S_AXI_GP0_AWREADY output from PS
  assign S_AXI_GP0_WDATA = 32'h0;
  assign S_AXI_GP0_WSTRB = 4'h0;
  assign S_AXI_GP0_WLAST = 1'b0;
  assign S_AXI_GP0_WVALID = 1'b0;
  //S_AXI_GP0_WREADY output from PS
  //S_AXI_GP0_BRESP
  //S_AXI_GP0_BVALID
  assign S_AXI_GP0_BREADY = 1'b1;
  assign S_AXI_GP0_ARADDR = 32'h0;
  assign S_AXI_GP0_ARLEN = 8'h0;
  assign S_AXI_GP0_ARSIZE = 3'h0;
  assign S_AXI_GP0_ARBURST = 2'h0;
  assign S_AXI_GP0_ARPROT = 3'h0;
  assign S_AXI_GP0_ARCACHE = 4'h0;
  assign S_AXI_GP0_ARVALID = 1'b0;
  //S_AXI_GP0_ARREADY
  //S_AXI_GP0_RDATA
  //S_AXI_GP0_RRESP
  //S_AXI_GP0_RLAST
  //S_AXI_GP0_RVALID
  assign S_AXI_GP0_RREADY = 1'b1;

  //S_AXI_HP0 from axi_eth_dma
  assign S_AXI_HP0_ARADDR = 32'h0;
  assign S_AXI_HP0_ARLEN = 8'h0;
  assign S_AXI_HP0_ARSIZE = 3'h0;
  assign S_AXI_HP0_ARBURST = 2'h0;
  assign S_AXI_HP0_ARPROT = 3'h0;
  assign S_AXI_HP0_ARCACHE = 4'h0;
  assign S_AXI_HP0_ARVALID = 1'b0;
  //S_AXI_HP0_ARREADY
  //S_AXI_HP0_RDATA
  //S_AXI_HP0_RRESP
  //S_AXI_HP0_RLAST
  //S_AXI_HP0_RVALID
  assign S_AXI_HP0_RREADY = 1'b1;
  assign S_AXI_HP0_AWADDR = 32'h0;
  assign S_AXI_HP0_AWLEN = 8'h0;
  assign S_AXI_HP0_AWSIZE = 3'h0;
  assign S_AXI_HP0_AWBURST = 2'h0;
  assign S_AXI_HP0_AWPROT = 3'h0;
  assign S_AXI_HP0_AWCACHE = 4'h0;
  assign S_AXI_HP0_AWVALID = 1'b0;
  //S_AXI_HP0_AWREADY
  assign S_AXI_HP0_WDATA = 64'h0;
  assign S_AXI_HP0_WSTRB = 8'h0;
  assign S_AXI_HP0_WLAST = 1'b0;
  assign S_AXI_HP0_WVALID = 1'b0;
  //S_AXI_HP0_WREADY
  //S_AXI_HP0_BRESP
  //S_AXI_HP0_BVALID
  assign S_AXI_HP0_BREADY = 1'b1;

`else

  axi_eth_dma inst_axi_eth_dma0 (
    .s_axi_lite_aclk(clk40),
    .m_axi_sg_aclk(clk40),
    .m_axi_mm2s_aclk(clk40),
    .m_axi_s2mm_aclk(clk40),
    .axi_resetn(clk40_rstn),

    .s_axi_lite_awaddr(M_AXI_ETH_DMA0_AWADDR),
    .s_axi_lite_awvalid(M_AXI_ETH_DMA0_AWVALID),
    .s_axi_lite_awready(M_AXI_ETH_DMA0_AWREADY),

    .s_axi_lite_wdata(M_AXI_ETH_DMA0_WDATA),
    .s_axi_lite_wvalid(M_AXI_ETH_DMA0_WVALID),
    .s_axi_lite_wready(M_AXI_ETH_DMA0_WREADY),

    .s_axi_lite_bresp(M_AXI_ETH_DMA0_BRESP),
    .s_axi_lite_bvalid(M_AXI_ETH_DMA0_BVALID),
    .s_axi_lite_bready(M_AXI_ETH_DMA0_BREADY),

    .s_axi_lite_araddr(M_AXI_ETH_DMA0_ARADDR),
    .s_axi_lite_arvalid(M_AXI_ETH_DMA0_ARVALID),
    .s_axi_lite_arready(M_AXI_ETH_DMA0_ARREADY),

    .s_axi_lite_rdata(M_AXI_ETH_DMA0_RDATA),
    .s_axi_lite_rresp(M_AXI_ETH_DMA0_RRESP),
    .s_axi_lite_rvalid(M_AXI_ETH_DMA0_RVALID),
    .s_axi_lite_rready(M_AXI_ETH_DMA0_RREADY),

    .m_axi_sg_awaddr(S_AXI_GP0_AWADDR),
    .m_axi_sg_awlen(S_AXI_GP0_AWLEN),
    .m_axi_sg_awsize(S_AXI_GP0_AWSIZE),
    .m_axi_sg_awburst(S_AXI_GP0_AWBURST),
    .m_axi_sg_awprot(S_AXI_GP0_AWPROT),
    .m_axi_sg_awcache(S_AXI_GP0_AWCACHE),
    .m_axi_sg_awvalid(S_AXI_GP0_AWVALID),
    .m_axi_sg_awready(S_AXI_GP0_AWREADY),
    .m_axi_sg_wdata(S_AXI_GP0_WDATA),
    .m_axi_sg_wstrb(S_AXI_GP0_WSTRB),
    .m_axi_sg_wlast(S_AXI_GP0_WLAST),
    .m_axi_sg_wvalid(S_AXI_GP0_WVALID),
    .m_axi_sg_wready(S_AXI_GP0_WREADY),
    .m_axi_sg_bresp(S_AXI_GP0_BRESP),
    .m_axi_sg_bvalid(S_AXI_GP0_BVALID),
    .m_axi_sg_bready(S_AXI_GP0_BREADY),
    .m_axi_sg_araddr(S_AXI_GP0_ARADDR),
    .m_axi_sg_arlen(S_AXI_GP0_ARLEN),
    .m_axi_sg_arsize(S_AXI_GP0_ARSIZE),
    .m_axi_sg_arburst(S_AXI_GP0_ARBURST),
    .m_axi_sg_arprot(S_AXI_GP0_ARPROT),
    .m_axi_sg_arcache(S_AXI_GP0_ARCACHE),
    .m_axi_sg_arvalid(S_AXI_GP0_ARVALID),
    .m_axi_sg_arready(S_AXI_GP0_ARREADY),
    .m_axi_sg_rdata(S_AXI_GP0_RDATA),
    .m_axi_sg_rresp(S_AXI_GP0_RRESP),
    .m_axi_sg_rlast(S_AXI_GP0_RLAST),
    .m_axi_sg_rvalid(S_AXI_GP0_RVALID),
    .m_axi_sg_rready(S_AXI_GP0_RREADY),

    .m_axi_mm2s_araddr(S_AXI_HP0_ARADDR),
    .m_axi_mm2s_arlen(S_AXI_HP0_ARLEN),
    .m_axi_mm2s_arsize(S_AXI_HP0_ARSIZE),
    .m_axi_mm2s_arburst(S_AXI_HP0_ARBURST),
    .m_axi_mm2s_arprot(S_AXI_HP0_ARPROT),
    .m_axi_mm2s_arcache(S_AXI_HP0_ARCACHE),
    .m_axi_mm2s_arvalid(S_AXI_HP0_ARVALID),
    .m_axi_mm2s_arready(S_AXI_HP0_ARREADY),
    .m_axi_mm2s_rdata(S_AXI_HP0_RDATA),
    .m_axi_mm2s_rresp(S_AXI_HP0_RRESP),
    .m_axi_mm2s_rlast(S_AXI_HP0_RLAST),
    .m_axi_mm2s_rvalid(S_AXI_HP0_RVALID),
    .m_axi_mm2s_rready(S_AXI_HP0_RREADY),

    .mm2s_prmry_reset_out_n(),
    .m_axis_mm2s_tdata(arm_eth0_tx_tdata),
    .m_axis_mm2s_tkeep(arm_eth0_tx_tkeep),
    .m_axis_mm2s_tvalid(arm_eth0_tx_tvalid),
    .m_axis_mm2s_tready(arm_eth0_tx_tready),
    .m_axis_mm2s_tlast(arm_eth0_tx_tlast),

    .m_axi_s2mm_awaddr(S_AXI_HP0_AWADDR),
    .m_axi_s2mm_awlen(S_AXI_HP0_AWLEN),
    .m_axi_s2mm_awsize(S_AXI_HP0_AWSIZE),
    .m_axi_s2mm_awburst(S_AXI_HP0_AWBURST),
    .m_axi_s2mm_awprot(S_AXI_HP0_AWPROT),
    .m_axi_s2mm_awcache(S_AXI_HP0_AWCACHE),
    .m_axi_s2mm_awvalid(S_AXI_HP0_AWVALID),
    .m_axi_s2mm_awready(S_AXI_HP0_AWREADY),
    .m_axi_s2mm_wdata(S_AXI_HP0_WDATA),
    .m_axi_s2mm_wstrb(S_AXI_HP0_WSTRB),
    .m_axi_s2mm_wlast(S_AXI_HP0_WLAST),
    .m_axi_s2mm_wvalid(S_AXI_HP0_WVALID),
    .m_axi_s2mm_wready(S_AXI_HP0_WREADY),
    .m_axi_s2mm_bresp(S_AXI_HP0_BRESP),
    .m_axi_s2mm_bvalid(S_AXI_HP0_BVALID),
    .m_axi_s2mm_bready(S_AXI_HP0_BREADY),

    .s2mm_prmry_reset_out_n(),
    .s_axis_s2mm_tdata(arm_eth0_rx_tdata),
    .s_axis_s2mm_tkeep(arm_eth0_rx_tkeep),
    .s_axis_s2mm_tvalid(arm_eth0_rx_tvalid),
    .s_axis_s2mm_tready(arm_eth0_rx_tready),
    .s_axis_s2mm_tlast(arm_eth0_rx_tlast),

    .mm2s_introut(arm_eth0_tx_irq),
    .s2mm_introut(arm_eth0_rx_irq),
    .axi_dma_tstvec()
  );

  axi_fifo_2clk #(
    .WIDTH(1+8+64),
    .SIZE(5)
  ) eth_tx_0_fifo_2clk_i (
    .reset(clk40_rst),
    .i_aclk(clk40),
    .i_tdata({arm_eth0_tx_tlast, arm_eth0_tx_tkeep, arm_eth0_tx_tdata}),
    .i_tvalid(arm_eth0_tx_tvalid),
    .i_tready(arm_eth0_tx_tready),
    .o_aclk(bus_clk),
    .o_tdata({arm_eth0_tx_tlast_b, arm_eth0_tx_tkeep_b, arm_eth0_tx_tdata_b}),
    .o_tvalid(arm_eth0_tx_tvalid_b),
    .o_tready(arm_eth0_tx_tready_b)
  );

  axi_fifo_2clk #(
    .WIDTH(1+8+64),
    .SIZE(5)
  ) eth_rx_0_fifo_2clk_i (
    .reset(bus_rst),
    .i_aclk(bus_clk),
    .i_tdata({arm_eth0_rx_tlast_b, arm_eth0_rx_tkeep_b, arm_eth0_rx_tdata_b}),
    .i_tvalid(arm_eth0_rx_tvalid_b),
    .i_tready(arm_eth0_rx_tready_b),
    .o_aclk(clk40),
    .o_tdata({arm_eth0_rx_tlast, arm_eth0_rx_tkeep, arm_eth0_rx_tdata}),
    .o_tvalid(arm_eth0_rx_tvalid),
    .o_tready(arm_eth0_rx_tready)
  );

`endif

  /////////////////////////////////////////////////////////////////////
  //
  // Ethernet DMA 1
  //
  //////////////////////////////////////////////////////////////////////

  assign  IRQ_F2P[2] = arm_eth1_rx_irq;
  assign  IRQ_F2P[3] = arm_eth1_tx_irq;

  assign {S_AXI_HP1_AWID, S_AXI_HP1_ARID} = 12'd0;
  assign {S_AXI_GP1_AWID, S_AXI_GP1_ARID} = 10'd0;

`ifdef NO_ETH_DMA_1
  //If inst Aurora, tie off each axi/axi-lite interface
  axi_dummy #(.DEC_ERR(1'b0)) inst_axi_dummy_sfp1_eth_dma
  (
    .s_axi_aclk(bus_clk),
    .s_axi_areset(bus_rst),

    .s_axi_awaddr(M_AXI_ETH_DMA1_AWADDR),
    .s_axi_awvalid(M_AXI_ETH_DMA1_AWVALID),
    .s_axi_awready(M_AXI_ETH_DMA1_AWREADY),

    .s_axi_wdata(M_AXI_ETH_DMA1_WDATA),
    .s_axi_wvalid(M_AXI_ETH_DMA1_WVALID),
    .s_axi_wready(M_AXI_ETH_DMA1_WREADY),

    .s_axi_bresp(M_AXI_ETH_DMA1_BRESP),
    .s_axi_bvalid(M_AXI_ETH_DMA1_BVALID),
    .s_axi_bready(M_AXI_ETH_DMA1_BREADY),

    .s_axi_araddr(M_AXI_ETH_DMA1_ARADDR),
    .s_axi_arvalid(M_AXI_ETH_DMA1_ARVALID),
    .s_axi_arready(M_AXI_ETH_DMA1_ARREADY),

    .s_axi_rdata(M_AXI_ETH_DMA1_RDATA),
    .s_axi_rresp(M_AXI_ETH_DMA1_RRESP),
    .s_axi_rvalid(M_AXI_ETH_DMA1_RVALID),
    .s_axi_rready(M_AXI_ETH_DMA1_RREADY)

  );
  //S_AXI_GP0 outputs from axi_eth_dma, so needs some sort of controller/tie off
  assign S_AXI_GP1_AWADDR = 32'h0;
  assign S_AXI_GP1_AWLEN = 8'h0;
  assign S_AXI_GP1_AWSIZE = 4'h0;
  assign S_AXI_GP1_AWBURST = 3'h0;
  assign S_AXI_GP1_AWPROT = 3'h0;
  assign S_AXI_GP1_AWCACHE = 4'h0;
  assign S_AXI_GP1_AWVALID = 1'b0;
  //S_AXI_GP1_AWREADY output from PS
  assign S_AXI_GP1_WDATA = 32'h0;
  assign S_AXI_GP1_WSTRB = 4'h0;
  assign S_AXI_GP1_WLAST = 1'b0;
  assign S_AXI_GP1_WVALID = 1'b0;
  //S_AXI_GP1_WREADY output from PS
  //S_AXI_GP1_BRESP
  //S_AXI_GP1_BVALID
  assign S_AXI_GP1_BREADY = 1'b1;
  assign S_AXI_GP1_ARADDR = 32'h0;
  assign S_AXI_GP1_ARLEN = 8'h0;
  assign S_AXI_GP1_ARSIZE = 3'h0;
  assign S_AXI_GP1_ARBURST = 2'h0;
  assign S_AXI_GP1_ARPROT = 3'h0;
  assign S_AXI_GP1_ARCACHE = 4'h0;
  assign S_AXI_GP1_ARVALID = 1'b0;
  //S_AXI_GP1_ARREADY
  //S_AXI_GP1_RDATA
  //S_AXI_GP1_RRESP
  //S_AXI_GP1_RLAST
  //S_AXI_GP1_RVALID
  assign S_AXI_GP1_RREADY = 1'b1;

  //S_AXI_HP0 from axi_eth_dma
  assign S_AXI_HP1_ARADDR = 32'h0;
  assign S_AXI_HP1_ARLEN = 8'h0;
  assign S_AXI_HP1_ARSIZE = 3'h0;
  assign S_AXI_HP1_ARBURST = 2'h0;
  assign S_AXI_HP1_ARPROT = 3'h0;
  assign S_AXI_HP1_ARCACHE = 4'h0;
  assign S_AXI_HP1_ARVALID = 1'b0;
  //S_AXI_HP1_ARREADY
  //S_AXI_HP1_RDATA
  //S_AXI_HP1_RRESP
  //S_AXI_HP1_RLAST
  //S_AXI_HP1_RVALID
  assign S_AXI_HP1_RREADY = 1'b1;
  assign S_AXI_HP1_AWADDR = 32'h0;
  assign S_AXI_HP1_AWLEN = 8'h0;
  assign S_AXI_HP1_AWSIZE = 3'h0;
  assign S_AXI_HP1_AWBURST = 2'h0;
  assign S_AXI_HP1_AWPROT = 3'h0;
  assign S_AXI_HP1_AWCACHE = 4'h0;
  assign S_AXI_HP1_AWVALID = 1'b0;
  //S_AXI_HP1_AWREADY
  assign S_AXI_HP1_WDATA = 64'h0;
  assign S_AXI_HP1_WSTRB = 8'h0;
  assign S_AXI_HP1_WLAST = 1'b0;
  assign S_AXI_HP1_WVALID = 1'b0;
  //S_AXI_HP1_WREADY
  //S_AXI_HP1_BRESP
  //S_AXI_HP1_BVALID
  assign S_AXI_HP1_BREADY = 1'b1;

`else

  axi_eth_dma inst_axi_eth_dma1 (
    .s_axi_lite_aclk(clk40),
    .m_axi_sg_aclk(clk40),
    .m_axi_mm2s_aclk(clk40),
    .m_axi_s2mm_aclk(clk40),
    .axi_resetn(clk40_rstn),

    .s_axi_lite_awaddr(M_AXI_ETH_DMA1_AWADDR),
    .s_axi_lite_awvalid(M_AXI_ETH_DMA1_AWVALID),
    .s_axi_lite_awready(M_AXI_ETH_DMA1_AWREADY),

    .s_axi_lite_wdata(M_AXI_ETH_DMA1_WDATA),
    .s_axi_lite_wvalid(M_AXI_ETH_DMA1_WVALID),
    .s_axi_lite_wready(M_AXI_ETH_DMA1_WREADY),

    .s_axi_lite_bresp(M_AXI_ETH_DMA1_BRESP),
    .s_axi_lite_bvalid(M_AXI_ETH_DMA1_BVALID),
    .s_axi_lite_bready(M_AXI_ETH_DMA1_BREADY),

    .s_axi_lite_araddr(M_AXI_ETH_DMA1_ARADDR),
    .s_axi_lite_arvalid(M_AXI_ETH_DMA1_ARVALID),
    .s_axi_lite_arready(M_AXI_ETH_DMA1_ARREADY),

    .s_axi_lite_rdata(M_AXI_ETH_DMA1_RDATA),
    .s_axi_lite_rresp(M_AXI_ETH_DMA1_RRESP),
    .s_axi_lite_rvalid(M_AXI_ETH_DMA1_RVALID),
    .s_axi_lite_rready(M_AXI_ETH_DMA1_RREADY),

    .m_axi_sg_awaddr(S_AXI_GP1_AWADDR),
    .m_axi_sg_awlen(S_AXI_GP1_AWLEN),
    .m_axi_sg_awsize(S_AXI_GP1_AWSIZE),
    .m_axi_sg_awburst(S_AXI_GP1_AWBURST),
    .m_axi_sg_awprot(S_AXI_GP1_AWPROT),
    .m_axi_sg_awcache(S_AXI_GP1_AWCACHE),
    .m_axi_sg_awvalid(S_AXI_GP1_AWVALID),
    .m_axi_sg_awready(S_AXI_GP1_AWREADY),
    .m_axi_sg_wdata(S_AXI_GP1_WDATA),
    .m_axi_sg_wstrb(S_AXI_GP1_WSTRB),
    .m_axi_sg_wlast(S_AXI_GP1_WLAST),
    .m_axi_sg_wvalid(S_AXI_GP1_WVALID),
    .m_axi_sg_wready(S_AXI_GP1_WREADY),
    .m_axi_sg_bresp(S_AXI_GP1_BRESP),
    .m_axi_sg_bvalid(S_AXI_GP1_BVALID),
    .m_axi_sg_bready(S_AXI_GP1_BREADY),
    .m_axi_sg_araddr(S_AXI_GP1_ARADDR),
    .m_axi_sg_arlen(S_AXI_GP1_ARLEN),
    .m_axi_sg_arsize(S_AXI_GP1_ARSIZE),
    .m_axi_sg_arburst(S_AXI_GP1_ARBURST),
    .m_axi_sg_arprot(S_AXI_GP1_ARPROT),
    .m_axi_sg_arcache(S_AXI_GP1_ARCACHE),
    .m_axi_sg_arvalid(S_AXI_GP1_ARVALID),
    .m_axi_sg_arready(S_AXI_GP1_ARREADY),
    .m_axi_sg_rdata(S_AXI_GP1_RDATA),
    .m_axi_sg_rresp(S_AXI_GP1_RRESP),
    .m_axi_sg_rlast(S_AXI_GP1_RLAST),
    .m_axi_sg_rvalid(S_AXI_GP1_RVALID),
    .m_axi_sg_rready(S_AXI_GP1_RREADY),

    .m_axi_mm2s_araddr(S_AXI_HP1_ARADDR),
    .m_axi_mm2s_arlen(S_AXI_HP1_ARLEN),
    .m_axi_mm2s_arsize(S_AXI_HP1_ARSIZE),
    .m_axi_mm2s_arburst(S_AXI_HP1_ARBURST),
    .m_axi_mm2s_arprot(S_AXI_HP1_ARPROT),
    .m_axi_mm2s_arcache(S_AXI_HP1_ARCACHE),
    .m_axi_mm2s_arvalid(S_AXI_HP1_ARVALID),
    .m_axi_mm2s_arready(S_AXI_HP1_ARREADY),
    .m_axi_mm2s_rdata(S_AXI_HP1_RDATA),
    .m_axi_mm2s_rresp(S_AXI_HP1_RRESP),
    .m_axi_mm2s_rlast(S_AXI_HP1_RLAST),
    .m_axi_mm2s_rvalid(S_AXI_HP1_RVALID),
    .m_axi_mm2s_rready(S_AXI_HP1_RREADY),

    .mm2s_prmry_reset_out_n(),
    .m_axis_mm2s_tdata(arm_eth1_tx_tdata),
    .m_axis_mm2s_tkeep(arm_eth1_tx_tkeep),
    .m_axis_mm2s_tvalid(arm_eth1_tx_tvalid),
    .m_axis_mm2s_tready(arm_eth1_tx_tready),
    .m_axis_mm2s_tlast(arm_eth1_tx_tlast),

    .m_axi_s2mm_awaddr(S_AXI_HP1_AWADDR),
    .m_axi_s2mm_awlen(S_AXI_HP1_AWLEN),
    .m_axi_s2mm_awsize(S_AXI_HP1_AWSIZE),
    .m_axi_s2mm_awburst(S_AXI_HP1_AWBURST),
    .m_axi_s2mm_awprot(S_AXI_HP1_AWPROT),
    .m_axi_s2mm_awcache(S_AXI_HP1_AWCACHE),
    .m_axi_s2mm_awvalid(S_AXI_HP1_AWVALID),
    .m_axi_s2mm_awready(S_AXI_HP1_AWREADY),
    .m_axi_s2mm_wdata(S_AXI_HP1_WDATA),
    .m_axi_s2mm_wstrb(S_AXI_HP1_WSTRB),
    .m_axi_s2mm_wlast(S_AXI_HP1_WLAST),
    .m_axi_s2mm_wvalid(S_AXI_HP1_WVALID),
    .m_axi_s2mm_wready(S_AXI_HP1_WREADY),
    .m_axi_s2mm_bresp(S_AXI_HP1_BRESP),
    .m_axi_s2mm_bvalid(S_AXI_HP1_BVALID),
    .m_axi_s2mm_bready(S_AXI_HP1_BREADY),

    .s2mm_prmry_reset_out_n(),
    .s_axis_s2mm_tdata(arm_eth1_rx_tdata),
    .s_axis_s2mm_tkeep(arm_eth1_rx_tkeep),
    .s_axis_s2mm_tvalid(arm_eth1_rx_tvalid),
    .s_axis_s2mm_tready(arm_eth1_rx_tready),
    .s_axis_s2mm_tlast(arm_eth1_rx_tlast),

    .mm2s_introut(arm_eth1_tx_irq),
    .s2mm_introut(arm_eth1_rx_irq),
    .axi_dma_tstvec()
  );

  axi_fifo_2clk #(
    .WIDTH(1+8+64),
    .SIZE(5)
  ) eth_tx_1_fifo_2clk_i (
    .reset(clk40_rst),
    .i_aclk(clk40),
    .i_tdata({arm_eth1_tx_tlast, arm_eth1_tx_tkeep, arm_eth1_tx_tdata}),
    .i_tvalid(arm_eth1_tx_tvalid),
    .i_tready(arm_eth1_tx_tready),
    .o_aclk(bus_clk),
    .o_tdata({arm_eth1_tx_tlast_b, arm_eth1_tx_tkeep_b, arm_eth1_tx_tdata_b}),
    .o_tvalid(arm_eth1_tx_tvalid_b),
    .o_tready(arm_eth1_tx_tready_b)
  );

  axi_fifo_2clk #(
    .WIDTH(1+8+64),
    .SIZE(5)
  ) eth_rx_1_fifo_2clk_i (
    .reset(bus_rst),
    .i_aclk(bus_clk),
    .i_tdata({arm_eth1_rx_tlast_b, arm_eth1_rx_tkeep_b, arm_eth1_rx_tdata_b}),
    .i_tvalid(arm_eth1_rx_tvalid_b),
    .i_tready(arm_eth1_rx_tready_b),
    .o_aclk(clk40),
    .o_tdata({arm_eth1_rx_tlast, arm_eth1_rx_tkeep, arm_eth1_rx_tdata}),
    .o_tvalid(arm_eth1_rx_tvalid),
    .o_tready(arm_eth1_rx_tready)
  );
`endif

  /////////////////////////////////////////////////////////////////////
  //
  // Internal Ethernet Interface
  //
  //////////////////////////////////////////////////////////////////////
  eth_internal #(
    .DWIDTH(REG_DWIDTH),
    .AWIDTH(REG_AWIDTH),
    .PORTNUM(8'd1)
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
  // Processing System
  //
  //////////////////////////////////////////////////////////////////////

  wire spi0_sclk;
  wire spi0_mosi;
  wire spi0_miso;
  wire spi0_ss0;
  wire spi0_ss1;
  wire spi0_ss2;
  wire spi1_sclk;
  wire spi1_mosi;
  wire spi1_miso;
  wire spi1_ss0;
  wire spi1_ss1;
  wire spi1_ss2;

  assign ps_gpio_in[10] = DBA_MYK_INTRQ;
`ifndef N300
  assign ps_gpio_in[11] = DBB_MYK_INTRQ;
`else
  assign ps_gpio_in[11] = 1'b0;
`endif

  // Processing System
  n310_ps_bd inst_n310_ps (
    .SPI0_SCLK_I(1'b0),
    .SPI0_SCLK_O(spi0_sclk),
    .SPI0_SCLK_T(),
    .SPI0_MOSI_I(1'b0),
    .SPI0_MOSI_O(spi0_mosi),
    .SPI0_MOSI_T(),
    .SPI0_MISO_I(spi0_miso),
    .SPI0_MISO_O(),
    .SPI0_MISO_T(),
    .SPI0_SS_I(1'b1),
    .SPI0_SS_O(spi0_ss0),
    .SPI0_SS1_O(spi0_ss1),
    .SPI0_SS2_O(spi0_ss2),
    .SPI0_SS_T(),

  `ifndef N300
    .SPI1_SCLK_I(1'b0),
    .SPI1_SCLK_O(spi1_sclk),
    .SPI1_SCLK_T(),
    .SPI1_MOSI_I(1'b0),
    .SPI1_MOSI_O(spi1_mosi),
    .SPI1_MOSI_T(),
    .SPI1_MISO_I(spi1_miso),
    .SPI1_MISO_O(),
    .SPI1_MISO_T(),
    .SPI1_SS_I(1'b1),
    .SPI1_SS_O(spi1_ss0),
    .SPI1_SS1_O(spi1_ss1),
    .SPI1_SS2_O(spi1_ss2),
    .SPI1_SS_T(),
  `else
    .SPI1_SCLK_I(1'b0),
    .SPI1_SCLK_O(),
    .SPI1_SCLK_T(),
    .SPI1_MOSI_I(1'b0),
    .SPI1_MOSI_O(),
    .SPI1_MOSI_T(),
    .SPI1_MISO_I(1'b0),
    .SPI1_MISO_O(),
    .SPI1_MISO_T(),
    .SPI1_SS_I(1'b1),
    .SPI1_SS_O(),
    .SPI1_SS1_O(),
    .SPI1_SS2_O(),
    .SPI1_SS_T(),
  `endif

    .bus_clk(bus_clk),
    .bus_rstn(~bus_rst),
    .clk40(clk40),
    .clk40_rstn(clk40_rstn),

    .M_AXI_ETH_DMA0_araddr(M_AXI_ETH_DMA0_ARADDR),
    .M_AXI_ETH_DMA0_arprot(),
    .M_AXI_ETH_DMA0_arready(M_AXI_ETH_DMA0_ARREADY),
    .M_AXI_ETH_DMA0_arvalid(M_AXI_ETH_DMA0_ARVALID),

    .M_AXI_ETH_DMA0_awaddr(M_AXI_ETH_DMA0_AWADDR),
    .M_AXI_ETH_DMA0_awprot(),
    .M_AXI_ETH_DMA0_awready(M_AXI_ETH_DMA0_AWREADY),
    .M_AXI_ETH_DMA0_awvalid(M_AXI_ETH_DMA0_AWVALID),

    .M_AXI_ETH_DMA0_wdata(M_AXI_ETH_DMA0_WDATA),
    .M_AXI_ETH_DMA0_wready(M_AXI_ETH_DMA0_WREADY),
    .M_AXI_ETH_DMA0_wstrb(M_AXI_ETH_DMA0_WSTRB),
    .M_AXI_ETH_DMA0_wvalid(M_AXI_ETH_DMA0_WVALID),

    .M_AXI_ETH_DMA0_rdata(M_AXI_ETH_DMA0_RDATA),
    .M_AXI_ETH_DMA0_rready(M_AXI_ETH_DMA0_RREADY),
    .M_AXI_ETH_DMA0_rresp(M_AXI_ETH_DMA0_RRESP),
    .M_AXI_ETH_DMA0_rvalid(M_AXI_ETH_DMA0_RVALID),

    .M_AXI_ETH_DMA0_bready(M_AXI_ETH_DMA0_BREADY),
    .M_AXI_ETH_DMA0_bresp(M_AXI_ETH_DMA0_BRESP),
    .M_AXI_ETH_DMA0_bvalid(M_AXI_ETH_DMA0_BVALID),

    .M_AXI_ETH_DMA1_araddr(M_AXI_ETH_DMA1_ARADDR),
    .M_AXI_ETH_DMA1_arprot(),
    .M_AXI_ETH_DMA1_arready(M_AXI_ETH_DMA1_ARREADY),
    .M_AXI_ETH_DMA1_arvalid(M_AXI_ETH_DMA1_ARVALID),

    .M_AXI_ETH_DMA1_awaddr(M_AXI_ETH_DMA1_AWADDR),
    .M_AXI_ETH_DMA1_awprot(),
    .M_AXI_ETH_DMA1_awready(M_AXI_ETH_DMA1_AWREADY),
    .M_AXI_ETH_DMA1_awvalid(M_AXI_ETH_DMA1_AWVALID),

    .M_AXI_ETH_DMA1_bready(M_AXI_ETH_DMA1_BREADY),
    .M_AXI_ETH_DMA1_bresp(M_AXI_ETH_DMA1_BRESP),
    .M_AXI_ETH_DMA1_bvalid(M_AXI_ETH_DMA1_BVALID),

    .M_AXI_ETH_DMA1_rdata(M_AXI_ETH_DMA1_RDATA),
    .M_AXI_ETH_DMA1_rready(M_AXI_ETH_DMA1_RREADY),
    .M_AXI_ETH_DMA1_rresp(M_AXI_ETH_DMA1_RRESP),
    .M_AXI_ETH_DMA1_rvalid(M_AXI_ETH_DMA1_RVALID),

    .M_AXI_ETH_DMA1_wdata(M_AXI_ETH_DMA1_WDATA),
    .M_AXI_ETH_DMA1_wready(M_AXI_ETH_DMA1_WREADY),
    .M_AXI_ETH_DMA1_wstrb(M_AXI_ETH_DMA1_WSTRB),
    .M_AXI_ETH_DMA1_wvalid(M_AXI_ETH_DMA1_WVALID),

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

    .M_AXI_JESD0_araddr(M_AXI_JESD0_ARADDR),
    .M_AXI_JESD0_arprot(),
    .M_AXI_JESD0_arready(M_AXI_JESD0_ARREADY),
    .M_AXI_JESD0_arvalid(M_AXI_JESD0_ARVALID),

    .M_AXI_JESD0_awaddr(M_AXI_JESD0_AWADDR),
    .M_AXI_JESD0_awprot(),
    .M_AXI_JESD0_awready(M_AXI_JESD0_AWREADY),
    .M_AXI_JESD0_awvalid(M_AXI_JESD0_AWVALID),

    .M_AXI_JESD0_bready(M_AXI_JESD0_BREADY),
    .M_AXI_JESD0_bresp(M_AXI_JESD0_BRESP),
    .M_AXI_JESD0_bvalid(M_AXI_JESD0_BVALID),

    .M_AXI_JESD0_rdata(M_AXI_JESD0_RDATA),
    .M_AXI_JESD0_rready(M_AXI_JESD0_RREADY),
    .M_AXI_JESD0_rresp(M_AXI_JESD0_RRESP),
    .M_AXI_JESD0_rvalid(M_AXI_JESD0_RVALID),

    .M_AXI_JESD0_wdata(M_AXI_JESD0_WDATA),
    .M_AXI_JESD0_wready(M_AXI_JESD0_WREADY),
    .M_AXI_JESD0_wstrb(M_AXI_JESD0_WSTRB),
    .M_AXI_JESD0_wvalid(M_AXI_JESD0_WVALID),

    .M_AXI_JESD1_araddr(M_AXI_JESD1_ARADDR),
    .M_AXI_JESD1_arprot(),
    .M_AXI_JESD1_arready(M_AXI_JESD1_ARREADY),
    .M_AXI_JESD1_arvalid(M_AXI_JESD1_ARVALID),

    .M_AXI_JESD1_awaddr(M_AXI_JESD1_AWADDR),
    .M_AXI_JESD1_awprot(),
    .M_AXI_JESD1_awready(M_AXI_JESD1_AWREADY),
    .M_AXI_JESD1_awvalid(M_AXI_JESD1_AWVALID),

    .M_AXI_JESD1_bready(M_AXI_JESD1_BREADY),
    .M_AXI_JESD1_bresp(M_AXI_JESD1_BRESP),
    .M_AXI_JESD1_bvalid(M_AXI_JESD1_BVALID),

    .M_AXI_JESD1_rdata(M_AXI_JESD1_RDATA),
    .M_AXI_JESD1_rready(M_AXI_JESD1_RREADY),
    .M_AXI_JESD1_rresp(M_AXI_JESD1_RRESP),
    .M_AXI_JESD1_rvalid(M_AXI_JESD1_RVALID),

    .M_AXI_JESD1_wdata(M_AXI_JESD1_WDATA),
    .M_AXI_JESD1_wready(M_AXI_JESD1_WREADY),
    .M_AXI_JESD1_wstrb(M_AXI_JESD1_WSTRB),
    .M_AXI_JESD1_wvalid(M_AXI_JESD1_WVALID),

    .M_AXI_NET0_araddr(M_AXI_NET0_ARADDR),
    .M_AXI_NET0_arprot(),
    .M_AXI_NET0_arready(M_AXI_NET0_ARREADY),
    .M_AXI_NET0_arvalid(M_AXI_NET0_ARVALID),

    .M_AXI_NET0_awaddr(M_AXI_NET0_AWADDR),
    .M_AXI_NET0_awprot(),
    .M_AXI_NET0_awready(M_AXI_NET0_AWREADY),
    .M_AXI_NET0_awvalid(M_AXI_NET0_AWVALID),

    .M_AXI_NET0_bready(M_AXI_NET0_BREADY),
    .M_AXI_NET0_bresp(M_AXI_NET0_BRESP),
    .M_AXI_NET0_bvalid(M_AXI_NET0_BVALID),

    .M_AXI_NET0_rdata(M_AXI_NET0_RDATA),
    .M_AXI_NET0_rready(M_AXI_NET0_RREADY),
    .M_AXI_NET0_rresp(M_AXI_NET0_RRESP),
    .M_AXI_NET0_rvalid(M_AXI_NET0_RVALID),

    .M_AXI_NET0_wdata(M_AXI_NET0_WDATA),
    .M_AXI_NET0_wready(M_AXI_NET0_WREADY),
    .M_AXI_NET0_wstrb(M_AXI_NET0_WSTRB),
    .M_AXI_NET0_wvalid(M_AXI_NET0_WVALID),

    .M_AXI_NET1_araddr(M_AXI_NET1_ARADDR),
    .M_AXI_NET1_arprot(),
    .M_AXI_NET1_arready(M_AXI_NET1_ARREADY),
    .M_AXI_NET1_arvalid(M_AXI_NET1_ARVALID),

    .M_AXI_NET1_awaddr(M_AXI_NET1_AWADDR),
    .M_AXI_NET1_awprot(),
    .M_AXI_NET1_awready(M_AXI_NET1_AWREADY),
    .M_AXI_NET1_awvalid(M_AXI_NET1_AWVALID),

    .M_AXI_NET1_bready(M_AXI_NET1_BREADY),
    .M_AXI_NET1_bresp(M_AXI_NET1_BRESP),
    .M_AXI_NET1_bvalid(M_AXI_NET1_BVALID),

    .M_AXI_NET1_rdata(M_AXI_NET1_RDATA),
    .M_AXI_NET1_rready(M_AXI_NET1_RREADY),
    .M_AXI_NET1_rresp(M_AXI_NET1_RRESP),
    .M_AXI_NET1_rvalid(M_AXI_NET1_RVALID),

    .M_AXI_NET1_wdata(M_AXI_NET1_WDATA),
    .M_AXI_NET1_wready(M_AXI_NET1_WREADY),
    .M_AXI_NET1_wstrb(M_AXI_NET1_WSTRB),
    .M_AXI_NET1_wvalid(M_AXI_NET1_WVALID),

    .M_AXI_NET2_araddr(M_AXI_NET2_ARADDR),
    .M_AXI_NET2_arprot(),
    .M_AXI_NET2_arready(M_AXI_NET2_ARREADY),
    .M_AXI_NET2_arvalid(M_AXI_NET2_ARVALID),

    .M_AXI_NET2_awaddr(M_AXI_NET2_AWADDR),
    .M_AXI_NET2_awprot(),
    .M_AXI_NET2_awready(M_AXI_NET2_AWREADY),
    .M_AXI_NET2_awvalid(M_AXI_NET2_AWVALID),

    .M_AXI_NET2_bready(M_AXI_NET2_BREADY),
    .M_AXI_NET2_bresp(M_AXI_NET2_BRESP),
    .M_AXI_NET2_bvalid(M_AXI_NET2_BVALID),

    .M_AXI_NET2_rdata(M_AXI_NET2_RDATA),
    .M_AXI_NET2_rready(M_AXI_NET2_RREADY),
    .M_AXI_NET2_rresp(M_AXI_NET2_RRESP),
    .M_AXI_NET2_rvalid(M_AXI_NET2_RVALID),

    .M_AXI_NET2_wdata(M_AXI_NET2_WDATA),
    .M_AXI_NET2_wready(M_AXI_NET2_WREADY),
    .M_AXI_NET2_wstrb(M_AXI_NET2_WSTRB),
    .M_AXI_NET2_wvalid(M_AXI_NET2_WVALID),

    .M_AXI_WR_CLK(m_axi_wr_clk),
    .M_AXI_WR_RSTn(1'b1),
    .M_AXI_WR_araddr(m_axi_wr_araddr),
    .M_AXI_WR_arready(m_axi_wr_arready),
    .M_AXI_WR_arvalid(m_axi_wr_arvalid),
    .M_AXI_WR_awaddr(m_axi_wr_awaddr),
    .M_AXI_WR_awready(m_axi_wr_awready),
    .M_AXI_WR_awvalid(m_axi_wr_awvalid),
    .M_AXI_WR_bready(m_axi_wr_bready),
    .M_AXI_WR_bresp(m_axi_wr_bresp),
    .M_AXI_WR_bvalid(m_axi_wr_bvalid),
    .M_AXI_WR_rdata(m_axi_wr_rdata),
    .M_AXI_WR_rready(m_axi_wr_rready),
    .M_AXI_WR_rresp(m_axi_wr_rresp),
    .M_AXI_WR_rvalid(m_axi_wr_rvalid),
    .M_AXI_WR_wdata(m_axi_wr_wdata),
    .M_AXI_WR_wready(m_axi_wr_wready),
    .M_AXI_WR_wstrb(m_axi_wr_wstrb),
    .M_AXI_WR_wvalid(m_axi_wr_wvalid),

    .M_AXI_XBAR_araddr(M_AXI_XBAR_ARADDR),
    .M_AXI_XBAR_arprot(),
    .M_AXI_XBAR_arready(M_AXI_XBAR_ARREADY),
    .M_AXI_XBAR_arvalid(M_AXI_XBAR_ARVALID),

    .M_AXI_XBAR_awaddr(M_AXI_XBAR_AWADDR),
    .M_AXI_XBAR_awprot(),
    .M_AXI_XBAR_awready(M_AXI_XBAR_AWREADY),
    .M_AXI_XBAR_awvalid(M_AXI_XBAR_AWVALID),

    .M_AXI_XBAR_bready(M_AXI_XBAR_BREADY),
    .M_AXI_XBAR_bresp(M_AXI_XBAR_BRESP),
    .M_AXI_XBAR_bvalid(M_AXI_XBAR_BVALID),

    .M_AXI_XBAR_rdata(M_AXI_XBAR_RDATA),
    .M_AXI_XBAR_rready(M_AXI_XBAR_RREADY),
    .M_AXI_XBAR_rresp(M_AXI_XBAR_RRESP),
    .M_AXI_XBAR_rvalid(M_AXI_XBAR_RVALID),

    .M_AXI_XBAR_wdata(M_AXI_XBAR_WDATA),
    .M_AXI_XBAR_wready(M_AXI_XBAR_WREADY),
    .M_AXI_XBAR_wstrb(M_AXI_XBAR_WSTRB),
    .M_AXI_XBAR_wvalid(M_AXI_XBAR_WVALID),

    .S_AXI_GP0_ACLK(clk40),
    .S_AXI_GP0_ARESETN(clk40_rstn),
    .S_AXI_GP0_araddr(S_AXI_GP0_ARADDR),
    .S_AXI_GP0_arburst(S_AXI_GP0_ARBURST),
    .S_AXI_GP0_arcache(S_AXI_GP0_ARCACHE),
    .S_AXI_GP0_arlen(S_AXI_GP0_ARLEN),
    .S_AXI_GP0_arlock(1'b0),
    .S_AXI_GP0_arprot(S_AXI_GP0_ARPROT),
    .S_AXI_GP0_arqos(4'b0000),
    .S_AXI_GP0_arready(S_AXI_GP0_ARREADY),
    .S_AXI_GP0_arsize(S_AXI_GP0_ARSIZE),
    .S_AXI_GP0_arvalid(S_AXI_GP0_ARVALID),
    .S_AXI_GP0_awaddr(S_AXI_GP0_AWADDR),
    .S_AXI_GP0_awburst(S_AXI_GP0_AWBURST),
    .S_AXI_GP0_awcache(S_AXI_GP0_AWCACHE),
    .S_AXI_GP0_awlen(S_AXI_GP0_AWLEN),
    .S_AXI_GP0_awlock(1'b0),
    .S_AXI_GP0_awprot(S_AXI_GP0_AWPROT),
    .S_AXI_GP0_awqos(4'b0000),
    .S_AXI_GP0_awregion(4'b0000),
    .S_AXI_GP0_awready(S_AXI_GP0_AWREADY),
    .S_AXI_GP0_awsize(S_AXI_GP0_AWSIZE),
    .S_AXI_GP0_awvalid(S_AXI_GP0_AWVALID),
    .S_AXI_GP0_bready(S_AXI_GP0_BREADY),
    .S_AXI_GP0_bresp(S_AXI_GP0_BRESP),
    .S_AXI_GP0_bvalid(S_AXI_GP0_BVALID),
    .S_AXI_GP0_rdata(S_AXI_GP0_RDATA),
    .S_AXI_GP0_rlast(S_AXI_GP0_RLAST),
    .S_AXI_GP0_rready(S_AXI_GP0_RREADY),
    .S_AXI_GP0_rresp(S_AXI_GP0_RRESP),
    .S_AXI_GP0_rvalid(S_AXI_GP0_RVALID),
    .S_AXI_GP0_wdata(S_AXI_GP0_WDATA),
    .S_AXI_GP0_wlast(S_AXI_GP0_WLAST),
    .S_AXI_GP0_wready(S_AXI_GP0_WREADY),
    .S_AXI_GP0_wstrb(S_AXI_GP0_WSTRB),
    .S_AXI_GP0_wvalid(S_AXI_GP0_WVALID),

    .S_AXI_GP1_ACLK(clk40),
    .S_AXI_GP1_ARESETN(clk40_rstn),
    .S_AXI_GP1_araddr(S_AXI_GP1_ARADDR),
    .S_AXI_GP1_arburst(S_AXI_GP1_ARBURST),
    .S_AXI_GP1_arcache(S_AXI_GP1_ARCACHE),
    .S_AXI_GP1_arid(S_AXI_GP1_ARID),
    .S_AXI_GP1_arlen(S_AXI_GP1_ARLEN),
    .S_AXI_GP1_arlock(1'b0),
    .S_AXI_GP1_arprot(S_AXI_GP1_ARPROT),
    .S_AXI_GP1_arqos(4'b000),
    .S_AXI_GP1_arready(S_AXI_GP1_ARREADY),
    .S_AXI_GP1_arsize(S_AXI_GP1_ARSIZE),
    .S_AXI_GP1_arvalid(S_AXI_GP1_ARVALID),
    .S_AXI_GP1_awaddr(S_AXI_GP1_AWADDR),
    .S_AXI_GP1_awburst(S_AXI_GP1_AWBURST),
    .S_AXI_GP1_awcache(S_AXI_GP1_AWCACHE),
    .S_AXI_GP1_awid(S_AXI_GP1_AWID),
    .S_AXI_GP1_awlen(S_AXI_GP1_AWLEN),
    .S_AXI_GP1_awlock(1'b0),
    .S_AXI_GP1_awprot(S_AXI_GP1_AWPROT),
    .S_AXI_GP1_awqos(4'b0000),
    .S_AXI_GP1_awregion(4'b0000),
    .S_AXI_GP1_awready(S_AXI_GP1_AWREADY),
    .S_AXI_GP1_awsize(S_AXI_GP1_AWSIZE),
    .S_AXI_GP1_awvalid(S_AXI_GP1_AWVALID),
    .S_AXI_GP1_bid(),
    .S_AXI_GP1_bready(S_AXI_GP1_BREADY),
    .S_AXI_GP1_bresp(S_AXI_GP1_BRESP),
    .S_AXI_GP1_bvalid(S_AXI_GP1_BVALID),
    .S_AXI_GP1_rdata(S_AXI_GP1_RDATA),
    .S_AXI_GP1_rid(),
    .S_AXI_GP1_rlast(S_AXI_GP1_RLAST),
    .S_AXI_GP1_rready(S_AXI_GP1_RREADY),
    .S_AXI_GP1_rresp(S_AXI_GP1_RRESP),
    .S_AXI_GP1_rvalid(S_AXI_GP1_RVALID),
    .S_AXI_GP1_wdata(S_AXI_GP1_WDATA),
    .S_AXI_GP1_wlast(S_AXI_GP1_WLAST),
    .S_AXI_GP1_wready(S_AXI_GP1_WREADY),
    .S_AXI_GP1_wstrb(S_AXI_GP1_WSTRB),
    .S_AXI_GP1_wvalid(S_AXI_GP1_WVALID),

    .S_AXI_HP0_ACLK(clk40),
    .S_AXI_HP0_ARESETN(clk40_rstn),
    .S_AXI_HP0_araddr(S_AXI_HP0_ARADDR),
    .S_AXI_HP0_arburst(S_AXI_HP0_ARBURST),
    .S_AXI_HP0_arcache(S_AXI_HP0_ARCACHE),
    .S_AXI_HP0_arlen(S_AXI_HP0_ARLEN),
    .S_AXI_HP0_arlock(1'b0),
    .S_AXI_HP0_arprot(S_AXI_HP0_ARPROT),
    .S_AXI_HP0_arqos(4'b0000),
    .S_AXI_HP0_arready(S_AXI_HP0_ARREADY),
    .S_AXI_HP0_arregion(4'b0),
    .S_AXI_HP0_arsize(S_AXI_HP0_ARSIZE),
    .S_AXI_HP0_arvalid(S_AXI_HP0_ARVALID),
    .S_AXI_HP0_awaddr(S_AXI_HP0_AWADDR),
    .S_AXI_HP0_awburst(S_AXI_HP0_AWBURST),
    .S_AXI_HP0_awcache(S_AXI_HP0_AWCACHE),
    .S_AXI_HP0_awlen(S_AXI_HP0_AWLEN),
    .S_AXI_HP0_awlock(1'b0),
    .S_AXI_HP0_awprot(S_AXI_HP0_AWPROT),
    .S_AXI_HP0_awqos(4'b0000),
    .S_AXI_HP0_awready(S_AXI_HP0_AWREADY),
    .S_AXI_HP0_awregion(4'b0),
    .S_AXI_HP0_awsize(S_AXI_HP0_AWSIZE),
    .S_AXI_HP0_awvalid(S_AXI_HP0_AWVALID),
    .S_AXI_HP0_bready(S_AXI_HP0_BREADY),
    .S_AXI_HP0_bresp(S_AXI_HP0_BRESP),
    .S_AXI_HP0_bvalid(S_AXI_HP0_BVALID),
    .S_AXI_HP0_rdata(S_AXI_HP0_RDATA),
    .S_AXI_HP0_rlast(S_AXI_HP0_RLAST),
    .S_AXI_HP0_rready(S_AXI_HP0_RREADY),
    .S_AXI_HP0_rresp(S_AXI_HP0_RRESP),
    .S_AXI_HP0_rvalid(S_AXI_HP0_RVALID),
    .S_AXI_HP0_wdata(S_AXI_HP0_WDATA),
    .S_AXI_HP0_wlast(S_AXI_HP0_WLAST),
    .S_AXI_HP0_wready(S_AXI_HP0_WREADY),
    .S_AXI_HP0_wstrb(S_AXI_HP0_WSTRB),
    .S_AXI_HP0_wvalid(S_AXI_HP0_WVALID),

    .S_AXI_HP1_ACLK(clk40),
    .S_AXI_HP1_ARESETN(clk40_rstn),
    .S_AXI_HP1_araddr(S_AXI_HP1_ARADDR),
    .S_AXI_HP1_arburst(S_AXI_HP1_ARBURST),
    .S_AXI_HP1_arcache(S_AXI_HP1_ARCACHE),
    .S_AXI_HP1_arid(S_AXI_HP1_ARID),
    .S_AXI_HP1_arlen(S_AXI_HP1_ARLEN),
    .S_AXI_HP1_arlock(1'b0),
    .S_AXI_HP1_arprot(S_AXI_HP1_ARPROT),
    .S_AXI_HP1_arqos(4'b0000),
    .S_AXI_HP1_arready(S_AXI_HP1_ARREADY),
    .S_AXI_HP1_arsize(S_AXI_HP1_ARSIZE),
    .S_AXI_HP1_arvalid(S_AXI_HP1_ARVALID),
    .S_AXI_HP1_awaddr(S_AXI_HP1_AWADDR),
    .S_AXI_HP1_awburst(S_AXI_HP1_AWBURST),
    .S_AXI_HP1_awcache(S_AXI_HP1_AWCACHE),
    .S_AXI_HP1_awid(S_AXI_HP1_AWID),
    .S_AXI_HP1_awlen(S_AXI_HP1_AWLEN),
    .S_AXI_HP1_awlock(1'b0),
    .S_AXI_HP1_awprot(S_AXI_HP1_AWPROT),
    .S_AXI_HP1_awqos(4'b0000),
    .S_AXI_HP1_awready(S_AXI_HP1_AWREADY),
    .S_AXI_HP1_awsize(S_AXI_HP1_AWSIZE),
    .S_AXI_HP1_awvalid(S_AXI_HP1_AWVALID),
    .S_AXI_HP1_bid(),
    .S_AXI_HP1_bready(S_AXI_HP1_BREADY),
    .S_AXI_HP1_bresp(S_AXI_HP1_BRESP),
    .S_AXI_HP1_bvalid(S_AXI_HP1_BVALID),
    .S_AXI_HP1_rdata(S_AXI_HP1_RDATA),
    .S_AXI_HP1_rid(),
    .S_AXI_HP1_rlast(S_AXI_HP1_RLAST),
    .S_AXI_HP1_rready(S_AXI_HP1_RREADY),
    .S_AXI_HP1_rresp(S_AXI_HP1_RRESP),
    .S_AXI_HP1_rvalid(S_AXI_HP1_RVALID),
    .S_AXI_HP1_wdata(S_AXI_HP1_WDATA),
    .S_AXI_HP1_wlast(S_AXI_HP1_WLAST),
    .S_AXI_HP1_wready(S_AXI_HP1_WREADY),
    .S_AXI_HP1_wstrb(S_AXI_HP1_WSTRB),
    .S_AXI_HP1_wvalid(S_AXI_HP1_WVALID),

    // ARM DMA
    .s_axis_dma_tdata(e2h_tdata),
    .s_axis_dma_tkeep(e2h_tkeep),
    .s_axis_dma_tlast(e2h_tlast),
    .s_axis_dma_tready(e2h_tready),
    .s_axis_dma_tvalid(e2h_tvalid),
    .m_axis_dma_tdata(h2e_tdata),
    .m_axis_dma_tkeep(h2e_tkeep),
    .m_axis_dma_tlast(h2e_tlast),
    .m_axis_dma_tready(h2e_tready),
    .m_axis_dma_tvalid(h2e_tvalid),

    // Misc Interrupts, GPIO, clk
    .IRQ_F2P(IRQ_F2P),

    .GPIO_0_tri_i(ps_gpio_in),
    .GPIO_0_tri_o(ps_gpio_out),
    .GPIO_0_tri_t(ps_gpio_tri),

    .JTAG0_TCK(DBA_CPLD_JTAG_TCK),
    .JTAG0_TMS(DBA_CPLD_JTAG_TMS),
    .JTAG0_TDI(DBA_CPLD_JTAG_TDI),
    .JTAG0_TDO(DBA_CPLD_JTAG_TDO),

  `ifndef N300
    .JTAG1_TCK(DBB_CPLD_JTAG_TCK),
    .JTAG1_TMS(DBB_CPLD_JTAG_TMS),
    .JTAG1_TDI(DBB_CPLD_JTAG_TDI),
    .JTAG1_TDO(DBB_CPLD_JTAG_TDO),
  `else
    .JTAG1_TCK(),
    .JTAG1_TMS(),
    .JTAG1_TDI(),
    .JTAG1_TDO('b0),
  `endif

    .FCLK_CLK0(FCLK_CLK0),
    .FCLK_RESET0_N(FCLK_RESET0_N),
    .FCLK_CLK1(FCLK_CLK1),
    .FCLK_RESET1_N(),
    .FCLK_CLK2(FCLK_CLK2),
    .FCLK_RESET2_N(),
    .FCLK_CLK3(FCLK_CLK3),
    .FCLK_RESET3_N(),

    .WR_UART_txd(wr_uart_rxd), // rx <-> tx
    .WR_UART_rxd(wr_uart_txd), // rx <-> tx

    .qsfp_sda_i(qsfp_sda_i),
    .qsfp_sda_o(qsfp_sda_o),
    .qsfp_sda_t(qsfp_sda_t),
    .qsfp_scl_i(qsfp_scl_i),
    .qsfp_scl_o(qsfp_scl_o),
    .qsfp_scl_t(qsfp_scl_t),

    .USBIND_0_port_indctl(),
    .USBIND_0_vbus_pwrfault(),
    .USBIND_0_vbus_pwrselect(),

    // Outward connections to the pins
    .MIO(MIO),
    .DDR_cas_n(DDR_CAS_n),
    .DDR_cke(DDR_CKE),
    .DDR_ck_n(DDR_Clk_n),
    .DDR_ck_p(DDR_Clk),
    .DDR_cs_n(DDR_CS_n),
    .DDR_reset_n(DDR_DRSTB),
    .DDR_odt(DDR_ODT),
    .DDR_ras_n(DDR_RAS_n),
    .DDR_we_n(DDR_WEB),
    .DDR_ba(DDR_BankAddr),
    .DDR_addr(DDR_Addr),
    .DDR_VRN(DDR_VRN),
    .DDR_VRP(DDR_VRP),
    .DDR_dm(DDR_DM),
    .DDR_dq(DDR_DQ),
    .DDR_dqs_n(DDR_DQS_n),
    .DDR_dqs_p(DDR_DQS),
    .PS_SRSTB(PS_SRSTB),
    .PS_CLK(PS_CLK),
    .PS_PORB(PS_PORB)
  );

  ///////////////////////////////////////////////////////////////////////////////////
  //
  // Xilinx DDR3 Controller and PHY.
  //
  ///////////////////////////////////////////////////////////////////////////////////

  wire         ddr3_axi_clk;           // 1/4 DDR external clock rate (200MHz)
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


  // Instantiate the DDR3 MIG core
  //
  // The top-level IP block has no parameters defined for some reason.
  // Most of configurable parameters are hard-coded in the mig so get
  // some additional knobs we pull those out into verilog headers.
  //
  // Synthesis params:  ip/ddr3_32bit/ddr3_32bit_mig_parameters.vh
  // Simulation params: ip/ddr3_32bit/ddr3_32bit_mig_sim_parameters.vh

  ddr3_32bit u_ddr3_32bit (
    // Memory interface ports
    .ddr3_addr                      (ddr3_addr),
    .ddr3_ba                        (ddr3_ba),
    .ddr3_cas_n                     (ddr3_cas_n),
    .ddr3_ck_n                      (ddr3_ck_n),
    .ddr3_ck_p                      (ddr3_ck_p),
    .ddr3_cke                       (ddr3_cke),
    .ddr3_ras_n                     (ddr3_ras_n),
    .ddr3_reset_n                   (ddr3_reset_n),
    .ddr3_we_n                      (ddr3_we_n),
    .ddr3_dq                        (ddr3_dq),
    .ddr3_dqs_n                     (ddr3_dqs_n),
    .ddr3_dqs_p                     (ddr3_dqs_p),
    .init_calib_complete            (ddr3_running),
    .device_temp_i                  (device_temp),

    .ddr3_cs_n                      (ddr3_cs_n),
    .ddr3_dm                        (ddr3_dm),
    .ddr3_odt                       (ddr3_odt),
    // Application interface ports
    .ui_clk                         (ddr3_axi_clk),  // 200Hz clock out
    .ui_clk_sync_rst                (ddr3_axi_rst),  // Active high Reset signal synchronised to 200 MHz.
    .aresetn                        (ddr3_axi_rst_reg_n),
    .app_sr_req                     (1'b0),
    .app_sr_active                  (),
    .app_ref_req                    (1'b0),
    .app_ref_ack                    (),
    .app_zq_req                     (1'b0),
    .app_zq_ack                     (),
    // Slave Interface Write Address Ports
    .s_axi_awid                     (ddr3_axi_awid),
    .s_axi_awaddr                   (ddr3_axi_awaddr),
    .s_axi_awlen                    (ddr3_axi_awlen),
    .s_axi_awsize                   (ddr3_axi_awsize),
    .s_axi_awburst                  (ddr3_axi_awburst),
    .s_axi_awlock                   (ddr3_axi_awlock),
    .s_axi_awcache                  (ddr3_axi_awcache),
    .s_axi_awprot                   (ddr3_axi_awprot),
    .s_axi_awqos                    (ddr3_axi_awqos),
    .s_axi_awvalid                  (ddr3_axi_awvalid),
    .s_axi_awready                  (ddr3_axi_awready),
    // Slave Interface Write Data Ports
    .s_axi_wdata                    (ddr3_axi_wdata),
    .s_axi_wstrb                    (ddr3_axi_wstrb),
    .s_axi_wlast                    (ddr3_axi_wlast),
    .s_axi_wvalid                   (ddr3_axi_wvalid),
    .s_axi_wready                   (ddr3_axi_wready),
    // Slave Interface Write Response Ports
    .s_axi_bid                      (ddr3_axi_bid),
    .s_axi_bresp                    (ddr3_axi_bresp),
    .s_axi_bvalid                   (ddr3_axi_bvalid),
    .s_axi_bready                   (ddr3_axi_bready),
    // Slave Interface Read Address Ports
    .s_axi_arid                     (ddr3_axi_arid),
    .s_axi_araddr                   (ddr3_axi_araddr),
    .s_axi_arlen                    (ddr3_axi_arlen),
    .s_axi_arsize                   (ddr3_axi_arsize),
    .s_axi_arburst                  (ddr3_axi_arburst),
    .s_axi_arlock                   (ddr3_axi_arlock),
    .s_axi_arcache                  (ddr3_axi_arcache),
    .s_axi_arprot                   (ddr3_axi_arprot),
    .s_axi_arqos                    (ddr3_axi_arqos),
    .s_axi_arvalid                  (ddr3_axi_arvalid),
    .s_axi_arready                  (ddr3_axi_arready),
    // Slave Interface Read Data Ports
    .s_axi_rid                      (ddr3_axi_rid),
    .s_axi_rdata                    (ddr3_axi_rdata),
    .s_axi_rresp                    (ddr3_axi_rresp),
    .s_axi_rlast                    (ddr3_axi_rlast),
    .s_axi_rvalid                   (ddr3_axi_rvalid),
    .s_axi_rready                   (ddr3_axi_rready),
    // System Clock Ports
    .sys_clk_p                      (sys_clk_p),
    .sys_clk_n                      (sys_clk_n),
    .clk_ref_i                      (bus_clk),

    .sys_rst                        (~global_rst) // IJB. Poorly named active low. Should change RST_ACT_LOW.
  );

  // Temperature monitor module
  mig_7series_v4_2_tempmon #(
     .TEMP_MON_CONTROL("INTERNAL"),
     .XADC_CLK_PERIOD(5000 /* 200MHz clock period in ps */)
  ) tempmon_i (
     .clk(bus_clk), .xadc_clk(bus_clk), .rst(bus_rst),
     .device_temp_i(12'd0 /* ignored */), .device_temp(device_temp)
  );

  ///////////////////////////////////////////////////////
  //
  // DB PS SPI Connections
  //
  ///////////////////////////////////////////////////////
  wire [NUM_CHANNELS-1:0] rx_atr;
  wire [NUM_CHANNELS-1:0] tx_atr;
  (* IOB = "true" *) reg  [NUM_CHANNELS-1:0] rx_atr_reg;
  (* IOB = "true" *) reg  [NUM_CHANNELS-1:0] tx_atr_reg;

  // Radio GPIO control for DSA
  wire [16*NUM_CHANNELS-1:0] db_gpio_out;
  wire [16*NUM_CHANNELS-1:0] db_gpio_ddr;
  wire [16*NUM_CHANNELS-1:0] db_gpio_in;
  wire [16*NUM_CHANNELS-1:0] db_gpio_fab;

  // DB A SPI Connections
  wire cpld_a_cs_n;
  wire lmk_a_cs_n;
  wire dac_a_cs_n;
  wire myk_a_cs_n;

  // Split out the SCLK and MOSI data to Mykonos and the CPLD.
  assign DBA_CPLD_PS_SPI_SCLK = spi0_sclk;
  assign DBA_CPLD_PS_SPI_SDI  = spi0_mosi;

  assign DBA_MYK_SPI_SCLK     = spi0_sclk;
  assign DBA_MYK_SPI_SDIO     = spi0_mosi;
  // Assign individual chip selects from PS SPI MASTER 0.
  assign cpld_a_cs_n = spi0_ss0;
  assign lmk_a_cs_n  = spi0_ss1;
  assign dac_a_cs_n  = ps_gpio_out[8]; // DAC select driven through GPIO.
  assign myk_a_cs_n  = spi0_ss2;

  // Returned data mux from the SPI interfaces.
  assign spi0_miso = ~myk_a_cs_n      ? DBA_MYK_SPI_SDO  : // From Mykonos
                     DBA_CPLD_PS_SPI_SDO;

  // For the PS SPI connection to the CPLD, we use the LE and ADDR lines as individual
  // chip selects for the CPLD endpoint as well as the LMK and DAC endpoints.
  // LE      = CPLD
  // ADDR[0] = LMK
  // ADDR[1] = DAC
  assign DBA_CPLD_PS_SPI_LE       = cpld_a_cs_n;
  assign DBA_CPLD_PS_SPI_ADDR[0]  = lmk_a_cs_n;
  assign DBA_CPLD_PS_SPI_ADDR[1]  = dac_a_cs_n;
  assign DBA_MYK_SPI_CS_n         = myk_a_cs_n;

  // Instantiate DSA registers in the IOB
  (* IOB = "true" *) reg [5:0] dsa_tx1_a_out_iob, dsa_rx1_a_out_iob;
  (* IOB = "true" *) reg [5:0] dsa_tx2_a_out_iob, dsa_rx2_a_out_iob;
  always @(posedge radio_clk) begin
    dsa_tx1_a_out_iob  <= db_gpio_out[16*0+11:16*0+6];
    dsa_rx1_a_out_iob  <= db_gpio_out[16*0+5:16*0+0];
    dsa_tx2_a_out_iob  <= db_gpio_out[16*1+11:16*1+6];
    dsa_rx2_a_out_iob  <= db_gpio_out[16*1+5:16*1+0];
  end

  assign DBA_CH1_TX_DSA_DATA = dsa_tx1_a_out_iob;
  assign DBA_CH1_RX_DSA_DATA = dsa_rx1_a_out_iob;
  assign DBA_CH2_TX_DSA_DATA = dsa_tx2_a_out_iob;
  assign DBA_CH2_RX_DSA_DATA = dsa_rx2_a_out_iob;

  assign DBA_ATR_RX_1 = rx_atr_reg[0];
  assign DBA_ATR_RX_2 = rx_atr_reg[1];
  assign DBA_ATR_TX_1 = tx_atr_reg[0];
  assign DBA_ATR_TX_2 = tx_atr_reg[1];

  assign DBA_MYK_GPIO_0  = 1'b0;
  assign DBA_MYK_GPIO_1  = 1'b0;
  assign DBA_MYK_GPIO_3  = 1'b0;
  assign DBA_MYK_GPIO_4  = 1'b0;
  assign DBA_MYK_GPIO_12 = 1'b0;
  assign DBA_MYK_GPIO_13 = 1'b0;
  assign DBA_MYK_GPIO_14 = 1'b0;
  assign DBA_MYK_GPIO_15 = 1'b0;

`ifndef N300

  // DB B SPI Connections
  wire cpld_b_cs_n;
  wire lmk_b_cs_n;
  wire dac_b_cs_n;
  wire myk_b_cs_n;

  // Split out the SCLK and MOSI data to Mykonos and the CPLD.
  assign DBB_CPLD_PS_SPI_SCLK = spi1_sclk;
  assign DBB_CPLD_PS_SPI_SDI  = spi1_mosi;

  assign DBB_MYK_SPI_SCLK     = spi1_sclk;
  assign DBB_MYK_SPI_SDIO     = spi1_mosi;

  // Assign individual chip selects from PS SPI MASTER 1.
  assign cpld_b_cs_n = spi1_ss0;
  assign lmk_b_cs_n  = spi1_ss1;
  assign dac_b_cs_n  = ps_gpio_out[9]; // DAC select driven through GPIO.
  assign myk_b_cs_n  = spi1_ss2;

  // Returned data mux from the SPI interfaces.
  assign spi1_miso = ~myk_b_cs_n      ? DBB_MYK_SPI_SDO  : // From Mykonos
                     DBB_CPLD_PS_SPI_SDO;

  // For the PS SPI connection to the CPLD, we use the LE and ADDR lines as individual
  // chip selects for the CPLD endpoint as well as the LMK and DAC endpoints.
  // LE      = CPLD
  // ADDR[0] = LMK
  // ADDR[1] = DAC
  assign DBB_CPLD_PS_SPI_LE       = cpld_b_cs_n;
  assign DBB_CPLD_PS_SPI_ADDR[0]  = lmk_b_cs_n;
  assign DBB_CPLD_PS_SPI_ADDR[1]  = dac_b_cs_n;
  assign DBB_MYK_SPI_CS_n         = myk_b_cs_n;

  // Instantiate DSA registers in the IOB
  (* IOB = "true" *) reg [5:0] dsa_tx1_b_out_iob, dsa_rx1_b_out_iob;
  (* IOB = "true" *) reg [5:0] dsa_tx2_b_out_iob, dsa_rx2_b_out_iob;
  always @(posedge radio_clk) begin
    dsa_tx1_b_out_iob  <= db_gpio_out[16*2+11:16*2+6];
    dsa_rx1_b_out_iob  <= db_gpio_out[16*2+5:16*2+0];
    dsa_tx2_b_out_iob  <= db_gpio_out[16*3+11:16*3+6];
    dsa_rx2_b_out_iob  <= db_gpio_out[16*3+5:16*3+0];
  end

  assign DBB_CH1_TX_DSA_DATA = dsa_tx1_b_out_iob;
  assign DBB_CH1_RX_DSA_DATA = dsa_rx1_b_out_iob;
  assign DBB_CH2_TX_DSA_DATA = dsa_tx2_b_out_iob;
  assign DBB_CH2_RX_DSA_DATA = dsa_rx2_b_out_iob;

  assign DBB_ATR_RX_1 = rx_atr_reg[2];
  assign DBB_ATR_RX_2 = rx_atr_reg[3];
  assign DBB_ATR_TX_1 = tx_atr_reg[2];
  assign DBB_ATR_TX_2 = tx_atr_reg[3];

  assign DBB_MYK_GPIO_0  = 1'b0;
  assign DBB_MYK_GPIO_1  = 1'b0;
  assign DBB_MYK_GPIO_3  = 1'b0;
  assign DBB_MYK_GPIO_4  = 1'b0;
  assign DBB_MYK_GPIO_12 = 1'b0;
  assign DBB_MYK_GPIO_13 = 1'b0;
  assign DBB_MYK_GPIO_14 = 1'b0;
  assign DBB_MYK_GPIO_15 = 1'b0;

`endif

  ///////////////////////////////////////////////////////
  //
  // N3xx CORE
  //
  ///////////////////////////////////////////////////////

  wire [31:0] rx[NUM_CHANNELS-1:0];
  wire [31:0] tx[NUM_CHANNELS-1:0];
  wire [32*NUM_CHANNELS-1:0] rx_flat;
  wire [32*NUM_CHANNELS-1:0] tx_flat;

  wire [NUM_CHANNELS-1:0] rx_stb;
  wire [NUM_CHANNELS-1:0] tx_stb;

  wire [31:0] build_datestamp;

  genvar i;
  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin
      // Radio Data
      assign rx_flat[32*i+31:32*i] = rx[i];
      assign tx[i] = tx_flat[32*i+31:32*i];
    end
  endgenerate

  USR_ACCESSE2 usr_access_i (
    .DATA(build_datestamp), .CFGCLK(), .DATAVALID()
  );

  n3xx_core #(
    .REG_AWIDTH(14),
    .BUS_CLK_RATE(BUS_CLK_RATE),
    .FP_GPIO_WIDTH(FP_GPIO_WIDTH),
    .NUM_CHANNELS_PER_RADIO(NUM_CHANNELS_PER_RADIO),
    .NUM_CHANNELS(NUM_CHANNELS),
    .NUM_DBOARDS(NUM_DBOARDS),
    .CHDR_W(CHDR_W),
    .RFNOC_PROTOVER(RFNOC_PROTOVER),
  `ifdef USE_REPLAY
    .USE_REPLAY(1)
  `else
    .USE_REPLAY(0)
  `endif
  ) n3xx_core(
    // Clocks and resets
  `ifdef NO_DB
    .radio_clk(bus_clk),
    .radio_rst(bus_rst),
  `else
    .radio_clk(radio_clk),
    .radio_rst(radio_rst),
  `endif
    .bus_clk(bus_clk),
    .bus_rst(bus_rst),
    .ddr3_dma_clk(ddr3_dma_clk),
    .clk40(clk40),

    // Clocking and PPS Controls/Indicators
    .pps(pps_radioclk1x),
    .pps_select(pps_select),
    .pps_out_enb(pps_out_enb),
    .pps_select_sfp(pps_select_sfp),
    .ref_clk_reset(),
    .meas_clk_reset(meas_clk_reset),
    .ref_clk_locked(1'b1),
    .meas_clk_locked(meas_clk_locked),
    .enable_ref_clk_async(enable_ref_clk_async),

    .s_axi_aclk(clk40),
    .s_axi_aresetn(clk40_rstn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(M_AXI_XBAR_AWADDR),
    .s_axi_awvalid(M_AXI_XBAR_AWVALID),
    .s_axi_awready(M_AXI_XBAR_AWREADY),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(M_AXI_XBAR_WDATA),
    .s_axi_wstrb(M_AXI_XBAR_WSTRB),
    .s_axi_wvalid(M_AXI_XBAR_WVALID),
    .s_axi_wready(M_AXI_XBAR_WREADY),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(M_AXI_XBAR_BRESP),
    .s_axi_bvalid(M_AXI_XBAR_BVALID),
    .s_axi_bready(M_AXI_XBAR_BREADY),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(M_AXI_XBAR_ARADDR),
    .s_axi_arvalid(M_AXI_XBAR_ARVALID),
    .s_axi_arready(M_AXI_XBAR_ARREADY),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata(M_AXI_XBAR_RDATA),
    .s_axi_rresp(M_AXI_XBAR_RRESP),
    .s_axi_rvalid(M_AXI_XBAR_RVALID),
    .s_axi_rready(M_AXI_XBAR_RREADY),
    // ps gpio source
    .ps_gpio_tri(ps_gpio_tri[FP_GPIO_WIDTH+FP_GPIO_OFFSET-1:FP_GPIO_OFFSET]),
    .ps_gpio_out(ps_gpio_out[FP_GPIO_WIDTH+FP_GPIO_OFFSET-1:FP_GPIO_OFFSET]),
    .ps_gpio_in(ps_gpio_in[FP_GPIO_WIDTH+FP_GPIO_OFFSET-1:FP_GPIO_OFFSET]),
    // FP_GPIO
    .fp_gpio_inout(FPGA_GPIO),
    // Radio ATR
    .rx_atr(rx_atr),
    .tx_atr(tx_atr),
    // Radio GPIO DSA
    .db_gpio_out_flat(db_gpio_out),
    .db_gpio_in_flat(db_gpio_in),
    .db_gpio_ddr_flat(db_gpio_ddr),
    .db_gpio_fab_flat(db_gpio_fab),
    // Radio Strobes
    .rx_stb(rx_stb),
    .tx_stb(tx_stb),
    // Radio Data
    .rx(rx_flat),
    .tx(tx_flat),
    // CPLD RX_LO TX_LO SPI
  `ifndef N300
    .sclk_flat({DBB_CPLD_PL_SPI_SCLK,
                DBA_CPLD_PL_SPI_SCLK}),
    .sen_flat({5'b0,DBB_CPLD_PL_SPI_ADDR[1],DBB_CPLD_PL_SPI_ADDR[0],DBB_CPLD_PL_SPI_LE,
               5'b0,DBA_CPLD_PL_SPI_ADDR[1],DBA_CPLD_PL_SPI_ADDR[0],DBA_CPLD_PL_SPI_LE}),
    .mosi_flat({DBB_CPLD_PL_SPI_SDI,
                DBA_CPLD_PL_SPI_SDI}),
    .miso_flat({DBB_CPLD_PL_SPI_SDO,
                DBA_CPLD_PL_SPI_SDO}),
  `else
    .sclk_flat(DBA_CPLD_PL_SPI_SCLK),
    .sen_flat({5'b0,DBA_CPLD_PL_SPI_ADDR[1],DBA_CPLD_PL_SPI_ADDR[0],DBA_CPLD_PL_SPI_LE}),
    .mosi_flat(DBA_CPLD_PL_SPI_SDI),
    .miso_flat(DBA_CPLD_PL_SPI_SDO),
  `endif
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
    .v2e0_tdata(v2e0_tdata),
    .v2e0_tvalid(v2e0_tvalid),
    .v2e0_tlast(v2e0_tlast),
    .v2e0_tready(v2e0_tready),

    .v2e1_tdata(v2e1_tdata),
    .v2e1_tlast(v2e1_tlast),
    .v2e1_tvalid(v2e1_tvalid),
    .v2e1_tready(v2e1_tready),

    // Ethernet to VITA
    .e2v0_tdata(e2v0_tdata),
    .e2v0_tlast(e2v0_tlast),
    .e2v0_tvalid(e2v0_tvalid),
    .e2v0_tready(e2v0_tready),

    .e2v1_tdata(e2v1_tdata),
    .e2v1_tlast(e2v1_tlast),
    .e2v1_tvalid(e2v1_tvalid),
    .e2v1_tready(e2v1_tready),

    //regport interface to npio
    .reg_wr_req_npio(reg_wr_req_npio),
    .reg_wr_addr_npio(reg_wr_addr_npio),
    .reg_wr_data_npio(reg_wr_data_npio),
    .reg_rd_req_npio(reg_rd_req_npio),
    .reg_rd_addr_npio(reg_rd_addr_npio),
    .reg_rd_resp_npio(reg_rd_resp_npio),
    .reg_rd_data_npio(reg_rd_data_npio),

    .build_datestamp(build_datestamp),
    .xadc_readback({20'h0, device_temp}),
    .sfp_ports_info({sfp_port1_info, sfp_port0_info}),
    .device_id(device_id)
  );

  // Register the ATR bits once between sending them out to the CPLD to avoid
  // glitches on the outputs!
  always @(posedge radio_clk) begin
    rx_atr_reg <= rx_atr;
    tx_atr_reg <= tx_atr;
  end

  // //////////////////////////////////////////////////////////////////////
  //
  // Daughterboard Cores
  //
  // //////////////////////////////////////////////////////////////////////

  wire [49:0] bRegPortInFlatA;
  wire [33:0] bRegPortOutFlatA;
  wire rx_a_valid;
  wire tx_a_rfi;
`ifndef N300
  wire [49:0] bRegPortInFlatB;
  wire [33:0] bRegPortOutFlatB;
  wire rx_b_valid;
  wire tx_b_rfi;
`endif

`ifdef BUILD_WR
  localparam INCL_WR_TDC = 1'b1;
`else
  localparam INCL_WR_TDC = 1'b0;
`endif

  wire          reg_portA_rd;
  wire          reg_portA_wr;
  wire [14-1:0] reg_portA_addr;
  wire [32-1:0] reg_portA_wr_data;
  wire [32-1:0] reg_portA_rd_data;
  wire          reg_portA_ready;
  wire          validA_unused;

  assign bRegPortInFlatA = {2'b0, reg_portA_addr, reg_portA_wr_data, reg_portA_rd, reg_portA_wr};
  assign {reg_portA_rd_data, validA_unused, reg_portA_ready} = bRegPortOutFlatA;

  DbCore #(
    .kInclWhiteRabbitTdc(INCL_WR_TDC)
  ) dba_core (
    .bBusReset(clk40_rst),                   //in  std_logic
    .BusClk(clk40),                          //in  std_logic
    .Clk40(clk40),                           //in  std_logic
    .MeasClk(meas_clk),                      //in  std_logic
    .FpgaClk_p(DBA_FPGA_CLK_P),              //in  std_logic
    .FpgaClk_n(DBA_FPGA_CLK_N),              //in  std_logic
    .SampleClk1xOut(radio_clk),              //out std_logic
    .SampleClk1x(radio_clk),                 //in  std_logic
    .SampleClk2xOut(radio_clk_2x),           //out std_logic
    .SampleClk2x(radio_clk_2x),              //in  std_logic
    .bRegPortInFlat(bRegPortInFlatA),        //in  std_logic_vector(49:0)
    .bRegPortOutFlat(bRegPortOutFlatA),      //out std_logic_vector(33:0)
    .kSlotId(1'b0),                          //in  std_logic
    .sSysRefFpgaLvds_p(DBA_FPGA_SYSREF_P),   //in  std_logic
    .sSysRefFpgaLvds_n(DBA_FPGA_SYSREF_N),   //in  std_logic
    .aLmkSync(DBA_CPLD_PL_SPI_ADDR[2]),      //out std_logic
    .JesdRefClk_p(USRPIO_A_MGTCLK_P),        //in  std_logic
    .JesdRefClk_n(USRPIO_A_MGTCLK_N),        //in  std_logic
    .aAdcRx_p(USRPIO_A_RX_P),                //in  std_logic_vector(3:0)
    .aAdcRx_n(USRPIO_A_RX_N),                //in  std_logic_vector(3:0)
    .aSyncAdcOut_n(DBA_MYK_SYNC_IN_n),       //out std_logic
    .aDacTx_p(USRPIO_A_TX_P),                //out std_logic_vector(3:0)
    .aDacTx_n(USRPIO_A_TX_N),                //out std_logic_vector(3:0)
    .aSyncDacIn_n(DBA_MYK_SYNC_OUT_n),       //in  std_logic
    .sAdcDataValid(rx_a_valid),              //out std_logic
    .sAdcDataSamples0I(rx[0][31:16]),        //out std_logic_vector(15:0)
    .sAdcDataSamples0Q(rx[0][15:0]),         //out std_logic_vector(15:0)
    .sAdcDataSamples1I(rx[1][31:16]),        //out std_logic_vector(15:0)
    .sAdcDataSamples1Q(rx[1][15:0]),         //out std_logic_vector(15:0)
    .sDacReadyForInput(tx_a_rfi),            //out std_logic
    .sDacDataSamples0I(tx[0][31:16]),        //in  std_logic_vector(15:0)
    .sDacDataSamples0Q(tx[0][15:0]),         //in  std_logic_vector(15:0)
    .sDacDataSamples1I(tx[1][31:16]),        //in  std_logic_vector(15:0)
    .sDacDataSamples1Q(tx[1][15:0]),         //in  std_logic_vector(15:0)
    .RefClk(ref_clk),                        //in  std_logic
    .rPpsPulse(pps_refclk),                  //in  std_logic
    .rGatedPulseToPin(UNUSED_PIN_TDCA_0),    //inout std_logic
    .sGatedPulseToPin(UNUSED_PIN_TDCA_1),    //inout std_logic
    .sPps(pps_radioclk1x),                   //out std_logic
    .sPpsToIob(pps_radioclk1x_iob),          //out std_logic
    .WrRefClk(wr_ref_clk),                   //in  std_logic
    .rWrPpsPulse(pps_wr_refclk),             //in  std_logic
    .rWrGatedPulseToPin(UNUSED_PIN_TDCA_2),  //inout std_logic
    .sWrGatedPulseToPin(UNUSED_PIN_TDCA_3),  //inout std_logic
    .aPpsSfpSel(pps_select_sfp),             //out std_logic_vector(1:0)
    .sAdcSync(),                             //out std_logic
    .sDacSync(),                             //out std_logic
    .sSysRef(),                              //out std_logic
    .rRpTransfer(),                          //out std_logic
    .sSpTransfer(),                          //out std_logic
    .rWrRpTransfer(),                        //out std_logic
    .sWrSpTransfer()                         //out std_logic
  );

  assign rx_stb[0] = rx_a_valid;
  assign rx_stb[1] = rx_a_valid;
  assign tx_stb[0] = tx_a_rfi;
  assign tx_stb[1] = tx_a_rfi;

  axil_to_ni_regport #(
    .RP_DWIDTH   (32),
    .RP_AWIDTH   (14),
    .TIMEOUT     (512)
  ) ni_regportA_inst (
    // Clock and reset
    .s_axi_aclk    (clk40),
    .s_axi_areset  (clk40_rst),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(M_AXI_JESD0_AWADDR),
    .s_axi_awvalid(M_AXI_JESD0_AWVALID),
    .s_axi_awready(M_AXI_JESD0_AWREADY),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(M_AXI_JESD0_WDATA),
    .s_axi_wstrb(M_AXI_JESD0_WSTRB),
    .s_axi_wvalid(M_AXI_JESD0_WVALID),
    .s_axi_wready(M_AXI_JESD0_WREADY),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(M_AXI_JESD0_BRESP),
    .s_axi_bvalid(M_AXI_JESD0_BVALID),
    .s_axi_bready(M_AXI_JESD0_BREADY),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(M_AXI_JESD0_ARADDR),
    .s_axi_arvalid(M_AXI_JESD0_ARVALID),
    .s_axi_arready(M_AXI_JESD0_ARREADY),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata(M_AXI_JESD0_RDATA),
    .s_axi_rresp(M_AXI_JESD0_RRESP),
    .s_axi_rvalid(M_AXI_JESD0_RVALID),
    .s_axi_rready(M_AXI_JESD0_RREADY),
    // Register port
    .reg_port_in_rd    (reg_portA_rd),
    .reg_port_in_wt    (reg_portA_wr),
    .reg_port_in_addr  (reg_portA_addr),
    .reg_port_in_data  (reg_portA_wr_data),
    .reg_port_out_data (reg_portA_rd_data),
    .reg_port_out_ready(reg_portA_ready)
  );

`ifndef N300
  wire          reg_portB_rd;
  wire          reg_portB_wr;
  wire [14-1:0] reg_portB_addr;
  wire [32-1:0] reg_portB_wr_data;
  wire [32-1:0] reg_portB_rd_data;
  wire          reg_portB_ready;
  wire          validB_unused;

  assign bRegPortInFlatB = {2'b0, reg_portB_addr, reg_portB_wr_data, reg_portB_rd, reg_portB_wr};
  assign {reg_portB_rd_data, validB_unused, reg_portB_ready} = bRegPortOutFlatB;

  DbCore #(
    .kInclWhiteRabbitTdc(INCL_WR_TDC)
  ) dbb_core (
    .bBusReset(clk40_rst),                   //in  std_logic
    .BusClk(clk40),                          //in  std_logic
    .Clk40(clk40),                           //in  std_logic
    .MeasClk(meas_clk),                      //in  std_logic
    .FpgaClk_p(DBB_FPGA_CLK_P),              //in  std_logic
    .FpgaClk_n(DBB_FPGA_CLK_N),              //in  std_logic
    .SampleClk1xOut(),                       //out std_logic
    .SampleClk1x(radio_clk),                 //in  std_logic
    .SampleClk2xOut(),                       //out std_logic
    .SampleClk2x(radio_clk_2x),              //in  std_logic
    .bRegPortInFlat(bRegPortInFlatB),        //in  std_logic_vector(49:0)
    .bRegPortOutFlat(bRegPortOutFlatB),      //out std_logic_vector(33:0)
    .kSlotId(1'b1),                          //in  std_logic
    .sSysRefFpgaLvds_p(DBB_FPGA_SYSREF_P),   //in  std_logic
    .sSysRefFpgaLvds_n(DBB_FPGA_SYSREF_N),   //in  std_logic
    .aLmkSync(DBB_CPLD_PL_SPI_ADDR[2]),      //out std_logic
    .JesdRefClk_p(USRPIO_B_MGTCLK_P),        //in  std_logic
    .JesdRefClk_n(USRPIO_B_MGTCLK_N),        //in  std_logic
    .aAdcRx_p(USRPIO_B_RX_P),                //in  std_logic_vector(3:0)
    .aAdcRx_n(USRPIO_B_RX_N),                //in  std_logic_vector(3:0)
    .aSyncAdcOut_n(DBB_MYK_SYNC_IN_n),       //out std_logic
    .aDacTx_p(USRPIO_B_TX_P),                //out std_logic_vector(3:0)
    .aDacTx_n(USRPIO_B_TX_N),                //out std_logic_vector(3:0)
    .aSyncDacIn_n(DBB_MYK_SYNC_OUT_n),       //in  std_logic
    .sAdcDataValid(rx_b_valid),              //out std_logic
    .sAdcDataSamples0I(rx[2][31:16]),        //out std_logic_vector(15:0)
    .sAdcDataSamples0Q(rx[2][15:0]),         //out std_logic_vector(15:0)
    .sAdcDataSamples1I(rx[3][31:16]),        //out std_logic_vector(15:0)
    .sAdcDataSamples1Q(rx[3][15:0]),         //out std_logic_vector(15:0)
    .sDacReadyForInput(tx_b_rfi),            //out std_logic
    .sDacDataSamples0I(tx[2][31:16]),        //in  std_logic_vector(15:0)
    .sDacDataSamples0Q(tx[2][15:0]),         //in  std_logic_vector(15:0)
    .sDacDataSamples1I(tx[3][31:16]),        //in  std_logic_vector(15:0)
    .sDacDataSamples1Q(tx[3][15:0]),         //in  std_logic_vector(15:0)
    .RefClk(ref_clk),                        //in  std_logic
    .rPpsPulse(pps_refclk),                  //in  std_logic
    .rGatedPulseToPin(UNUSED_PIN_TDCB_0),    //inout std_logic
    .sGatedPulseToPin(UNUSED_PIN_TDCB_1),    //inout std_logic
    .sPps(),                                 //out std_logic
    .sPpsToIob(),                            //out std_logic
    .WrRefClk(wr_ref_clk),                   //in  std_logic
    .rWrPpsPulse(pps_wr_refclk),             //in  std_logic
    .rWrGatedPulseToPin(UNUSED_PIN_TDCB_2),  //inout std_logic
    .sWrGatedPulseToPin(UNUSED_PIN_TDCB_3),  //inout std_logic
    .aPpsSfpSel(2'b0),                       //out std_logic_vector(1:0)
    .sAdcSync(),                             //out std_logic
    .sDacSync(),                             //out std_logic
    .sSysRef(),                              //out std_logic
    .rRpTransfer(),                          //out std_logic
    .sSpTransfer(),                          //out std_logic
    .rWrRpTransfer(),                        //out std_logic
    .sWrSpTransfer()                         //out std_logic
  );

  assign rx_stb[2] = rx_b_valid;
  assign rx_stb[3] = rx_b_valid;
  assign tx_stb[2] = tx_b_rfi;
  assign tx_stb[3] = tx_b_rfi;

  axil_to_ni_regport #(
    .RP_DWIDTH   (32),
    .RP_AWIDTH   (14),
    .TIMEOUT     (512)
  ) ni_regportB_inst (
    // Clock and reset
    .s_axi_aclk    (clk40),
    .s_axi_areset  (clk40_rst),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(M_AXI_JESD1_AWADDR),
    .s_axi_awvalid(M_AXI_JESD1_AWVALID),
    .s_axi_awready(M_AXI_JESD1_AWREADY),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(M_AXI_JESD1_WDATA),
    .s_axi_wstrb(M_AXI_JESD1_WSTRB),
    .s_axi_wvalid(M_AXI_JESD1_WVALID),
    .s_axi_wready(M_AXI_JESD1_WREADY),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(M_AXI_JESD1_BRESP),
    .s_axi_bvalid(M_AXI_JESD1_BVALID),
    .s_axi_bready(M_AXI_JESD1_BREADY),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(M_AXI_JESD1_ARADDR),
    .s_axi_arvalid(M_AXI_JESD1_ARVALID),
    .s_axi_arready(M_AXI_JESD1_ARREADY),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata   (M_AXI_JESD1_RDATA),
    .s_axi_rresp   (M_AXI_JESD1_RRESP),
    .s_axi_rvalid  (M_AXI_JESD1_RVALID),
    .s_axi_rready  (M_AXI_JESD1_RREADY),
    // Register port
    .reg_port_in_rd    (reg_portB_rd),
    .reg_port_in_wt    (reg_portB_wr),
    .reg_port_in_addr  (reg_portB_addr),
    .reg_port_in_data  (reg_portB_wr_data),
    .reg_port_out_data (reg_portB_rd_data),
    .reg_port_out_ready(reg_portB_ready)
  );
`else
  // Tie off second daughterboard interface
  axi_dummy #(
    .DEC_ERR(1'b0)
  ) inst_axi_dummy_dbb_core (
    // Clock and reset
    .s_axi_aclk    (clk40),
    .s_axi_areset  (clk40_rst),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(M_AXI_JESD1_AWADDR),
    .s_axi_awvalid(M_AXI_JESD1_AWVALID),
    .s_axi_awready(M_AXI_JESD1_AWREADY),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(M_AXI_JESD1_WDATA),
    .s_axi_wstrb(M_AXI_JESD1_WSTRB),
    .s_axi_wvalid(M_AXI_JESD1_WVALID),
    .s_axi_wready(M_AXI_JESD1_WREADY),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(M_AXI_JESD1_BRESP),
    .s_axi_bvalid(M_AXI_JESD1_BVALID),
    .s_axi_bready(M_AXI_JESD1_BREADY),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(M_AXI_JESD1_ARADDR),
    .s_axi_arvalid(M_AXI_JESD1_ARVALID),
    .s_axi_arready(M_AXI_JESD1_ARREADY),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata   (M_AXI_JESD1_RDATA),
    .s_axi_rresp   (M_AXI_JESD1_RRESP),
    .s_axi_rvalid  (M_AXI_JESD1_RVALID),
    .s_axi_rready  (M_AXI_JESD1_RREADY)
  );

`endif

  // //////////////////////////////////////////////////////////////////////
  //
  // LEDS
  //
  // //////////////////////////////////////////////////////////////////////

   assign PANEL_LED_LINK = ps_gpio_out[45];
   assign PANEL_LED_REF  = ps_gpio_out[46];
   assign PANEL_LED_GPS  = ps_gpio_out[47];


  /////////////////////////////////////////////////////////////////////
  //
  // PUDC Workaround
  //
  //////////////////////////////////////////////////////////////////////
  // This is a workaround for a silicon bug in Series 7 FPGA where a
  // race condition with the reading of PUDC during the erase of the FPGA
  // image cause glitches on output IO pins.
  //
  // Workaround:
  // - Define the PUDC pin in the XDC file with a pullup.
  // - Implements an IBUF on the PUDC input and make sure that it does
  //   not get optimized out.
  (* dont_touch = "true" *) wire fpga_pudc_b_buf;
  IBUF pudc_ibuf_i (
    .I(FPGA_PUDC_B),
    .O(fpga_pudc_b_buf));

endmodule
