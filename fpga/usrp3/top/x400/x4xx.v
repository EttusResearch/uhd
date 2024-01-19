//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx
//
// Description: Top-level module for X410 devices.
//

`default_nettype none


module x4xx (

  //-----------------------------------
  // RF block signals
  //-----------------------------------

  // Clocking and sync
  input  wire       SYSREF_RF_P,
  input  wire       SYSREF_RF_N,
  input  wire [3:0] ADC_CLK_P,
  input  wire [3:0] ADC_CLK_N,
  input  wire [1:0] DAC_CLK_P,
  input  wire [1:0] DAC_CLK_N,

  // Analog ports

  `ifdef X440
  input  wire [3:0] DB0_RX_P,
  input  wire [3:0] DB0_RX_N,
  input  wire [3:0] DB1_RX_P,
  input  wire [3:0] DB1_RX_N,
  output wire [3:0] DB0_TX_P,
  output wire [3:0] DB0_TX_N,
  output wire [3:0] DB1_TX_P,
  output wire [3:0] DB1_TX_N,
  `else // X410
  input  wire [1:0] DB0_RX_P,
  input  wire [1:0] DB0_RX_N,
  input  wire [1:0] DB1_RX_P,
  input  wire [1:0] DB1_RX_N,
  output wire [1:0] DB0_TX_P,
  output wire [1:0] DB0_TX_N,
  output wire [1:0] DB1_TX_P,
  output wire [1:0] DB1_TX_N,
  `endif

  //-----------------------------------
  // MGTs (Quad 128-131)
  //-----------------------------------
  //
  //     Quad    |    Connector
  //   Bank 128  |  QSFP28 (1)
  //   Bank 129  |  iPass+zHD (1)
  //   Bank 130  |  iPass+zHD (0)
  //   Bank 131  |  QSFP28 (0)
  //
  //-----------------------------------

  // Clock references
  input  wire MGT_REFCLK_LMK0_P,
  input  wire MGT_REFCLK_LMK0_N,
  input  wire MGT_REFCLK_LMK1_P,
  input  wire MGT_REFCLK_LMK1_N,
  input  wire MGT_REFCLK_LMK2_P,
  input  wire MGT_REFCLK_LMK2_N,
  input  wire MGT_REFCLK_LMK3_P,
  input  wire MGT_REFCLK_LMK3_N,

  // Quad 128 transceivers: QSFP28 (1)
  `ifdef QSFP1_0
  input  wire QSFP1_0_RX_P,
  input  wire QSFP1_0_RX_N,
  output wire QSFP1_0_TX_P,
  output wire QSFP1_0_TX_N,
  `endif
  `ifdef QSFP1_1
  input  wire QSFP1_1_RX_P,
  input  wire QSFP1_1_RX_N,
  output wire QSFP1_1_TX_P,
  output wire QSFP1_1_TX_N,
  `endif
  `ifdef QSFP1_2
  input  wire QSFP1_2_RX_P,
  input  wire QSFP1_2_RX_N,
  output wire QSFP1_2_TX_P,
  output wire QSFP1_2_TX_N,
  `endif
  `ifdef QSFP1_3
  input  wire QSFP1_3_RX_P,
  input  wire QSFP1_3_RX_N,
  output wire QSFP1_3_TX_P,
  output wire QSFP1_3_TX_N,
  `endif

  // Quad 129 transceivers: iPass+zHD (1)
  `ifdef IPASS1_LANES
  input  wire [`IPASS1_LANES-1:0] IPASS1_RX_P,
  input  wire [`IPASS1_LANES-1:0] IPASS1_RX_N,
  output wire [`IPASS1_LANES-1:0] IPASS1_TX_P,
  output wire [`IPASS1_LANES-1:0] IPASS1_TX_N,
  `endif

  // Quad 130 transceivers: iPass+zHD (0)
  `ifdef IPASS0_LANES
  input  wire [`IPASS0_LANES-1:0] IPASS0_RX_P,
  input  wire [`IPASS0_LANES-1:0] IPASS0_RX_N,
  output wire [`IPASS0_LANES-1:0] IPASS0_TX_P,
  output wire [`IPASS0_LANES-1:0] IPASS0_TX_N,
  `endif

  // Quad 131 transceivers: QSFP28 (0)
  `ifdef QSFP0_0
  input  wire QSFP0_0_RX_P,
  input  wire QSFP0_0_RX_N,
  output wire QSFP0_0_TX_P,
  output wire QSFP0_0_TX_N,
  `endif
  `ifdef QSFP0_1
  input  wire QSFP0_1_RX_P,
  input  wire QSFP0_1_RX_N,
  output wire QSFP0_1_TX_P,
  output wire QSFP0_1_TX_N,
  `endif
  `ifdef QSFP0_2
  input  wire QSFP0_2_RX_P,
  input  wire QSFP0_2_RX_N,
  output wire QSFP0_2_TX_P,
  output wire QSFP0_2_TX_N,
  `endif
  `ifdef QSFP0_3
  input  wire QSFP0_3_RX_P,
  input  wire QSFP0_3_RX_N,
  output wire QSFP0_3_TX_P,
  output wire QSFP0_3_TX_N,
  `endif


  //-----------------------------------
  // HD banks
  //-----------------------------------

  inout  wire [19:0] DB0_GPIO,
  output wire        DB0_SYNTH_SYNC,
  inout  wire [19:0] DB1_GPIO,
  output wire        DB1_SYNTH_SYNC,

  output wire        LMK_SYNC,
  input  wire        PPS_IN,
  output wire        PL_CPLD_SCLK,    // Dual-purpose CPLD JTAG TCK
  output wire        PL_CPLD_MOSI,    // Dual-purpose CPLD JTAG TDI
  input  wire        PL_CPLD_MISO,    // Dual-purpose CPLD JTAG TDO


  //-----------------------------------
  // eCPRI
  //-----------------------------------

  input  wire        FPGA_AUX_REF,

  `ifdef QSFP1_0
  output wire        GTY_RCV_CLK_P,
  output wire        GTY_RCV_CLK_N,
  `endif

  output wire        FABRIC_CLK_OUT_P,
  output wire        FABRIC_CLK_OUT_N,


  //-----------------------------------
  // Misc HP banks
  //-----------------------------------

  input  wire        PLL_REFCLK_FPGA_P,
  input  wire        PLL_REFCLK_FPGA_N,
  input  wire        BASE_REFCLK_FPGA_P,
  input  wire        BASE_REFCLK_FPGA_N,

  input  wire        SYSREF_FABRIC_P,
  input  wire        SYSREF_FABRIC_N,

  input  wire        QSFP0_MODPRS_n,
  output wire        QSFP0_RESET_n,
  output wire        QSFP0_LPMODE_n,
  input  wire        QSFP1_MODPRS_n,
  output wire        QSFP1_RESET_n,
  output wire        QSFP1_LPMODE_n,

  inout  wire [11:0] DIOA_FPGA,
  inout  wire [11:0] DIOB_FPGA,

  output wire        CPLD_JTAG_OE_n,

  output wire        PPS_LED,
  inout  wire        TRIG_IO,
  output wire        PL_CPLD_JTAGEN,
  output wire        PL_CPLD_CS0_n,    // Dual-purpose CPLD JTAG TMS
  output wire        PL_CPLD_CS1_n,


  //-----------------------------------
  // DRAM
  //-----------------------------------

  // DRAM Bank 0
  input  wire        DRAM0_REFCLK_P,
  input  wire        DRAM0_REFCLK_N,
  output wire        DRAM0_ACT_n,
  output wire [16:0] DRAM0_ADDR,
  output wire [ 1:0] DRAM0_BA,
  output wire [ 0:0] DRAM0_BG,
  output wire [ 0:0] DRAM0_CKE,
  output wire [ 0:0] DRAM0_ODT,
  output wire [ 0:0] DRAM0_CS_n,
  output wire [ 0:0] DRAM0_CLK_P,
  output wire [ 0:0] DRAM0_CLK_N,
  output wire        DRAM0_RESET_n,
  inout  wire [ 7:0] DRAM0_DM_n,
  inout  wire [63:0] DRAM0_DQ,
  inout  wire [ 7:0] DRAM0_DQS_p,
  inout  wire [ 7:0] DRAM0_DQS_n,

  // DRAM Bank 1
  input  wire        DRAM1_REFCLK_P,
  input  wire        DRAM1_REFCLK_N,
  output wire        DRAM1_ACT_n,
  output wire [16:0] DRAM1_ADDR,
  output wire [ 1:0] DRAM1_BA,
  output wire [ 0:0] DRAM1_BG,
  output wire [ 0:0] DRAM1_CKE,
  output wire [ 0:0] DRAM1_ODT,
  output wire [ 0:0] DRAM1_CS_n,
  output wire [ 0:0] DRAM1_CLK_P,
  output wire [ 0:0] DRAM1_CLK_N,
  output wire        DRAM1_RESET_n,
  inout  wire [ 7:0] DRAM1_DM_n,
  inout  wire [63:0] DRAM1_DQ,
  inout  wire [ 7:0] DRAM1_DQS_p,
  inout  wire [ 7:0] DRAM1_DQS_n


  //-----------------------------------
  // Unused pins
  //-----------------------------------

  // input  wire [1:0] IPASS_SIDEBAND,
  // input  wire       PCIE_RESET,
  // input  wire       PL_CPLD_IRQ,
  // output wire       FPGA_TEST,
  // output wire       TDC_SPARE_0,
  // output wire       TDC_SPARE_1

);

  // Variant-dependent register map.
  `ifdef X440
    `include "regmap/x440/versioning_regs_regmap_utils.vh"
  `else // Use X410 as the default variant for regmap.
    `include "regmap/x410/versioning_regs_regmap_utils.vh"
  `endif

  // Both of these provide the same values for what we need, but we'll keep the
  // if/else clause for consistency with other includes.
  `ifdef X440
    `include "regmap/x440/rfdc_regs_regmap_utils.vh"
  `else // Use X410 as the default variant for regmap.
    `include "regmap/x410/rfdc_regs_regmap_utils.vh"
  `endif


  `include "regmap/global_regs_regmap_utils.vh"
  `include "regmap/versioning_utils.vh"
  `include "x4xx_mgt_types.vh"


  //---------------------------------------------------------------------------
  // Build Configuration
  //---------------------------------------------------------------------------

  // Include the RFNoC image core header file
  `ifdef RFNOC_IMAGE_CORE_HDR
    `include `"`RFNOC_IMAGE_CORE_HDR`"
  `else
    ERROR_RFNOC_IMAGE_CORE_HDR_not_defined();
    `define CHDR_WIDTH     64
    `define RFNOC_PROTOVER { 8'd1, 8'd0 }
  `endif

  // Check the requested bandwidth
  `ifdef RF_BW
    localparam RF_BANDWIDTH = `RF_BW;  // RF Bandwidth (MHz)
  `else
    localparam RF_BANDWIDTH = 100;
    ERROR_RF_bandwidth_must_be_defined();
  `endif

  // Determine the samples per cycles and which RF core to use
  `ifdef RF_CORE_100M
    localparam RF_CORE   = "100M";
    localparam RADIO_SPC = 1;
  `elsif RF_CORE_200M
    localparam RF_CORE   = "200M";
    localparam RADIO_SPC = 2;
  `elsif RF_CORE_400M
    localparam RF_CORE   = "400M";
    localparam RADIO_SPC = 4;
  `elsif RF_CORE_FULL
    localparam RF_CORE   = "FULL";
    localparam RADIO_SPC = 8;
  `else
    ERROR_RF_CORE_must_be_defined();
    localparam RF_CORE   = "100M";
    localparam RADIO_SPC = 1;
  `endif

  localparam NUM_DBOARDS = 2;

  `ifdef X440
    localparam NUM_TIMEKEEPERS = NUM_DBOARDS;
  `else
    localparam NUM_TIMEKEEPERS = 1;
  `endif

  // Set the width for each transport adapter. All ports within a single QSFP
  // will have the same width.
  localparam [31:0] QSFP0_W = `CHDR_WIDTH;
  localparam [31:0] QSFP1_W = `CHDR_WIDTH;
  // Width of the signals used to connect the Ethernet transport adapters to
  // RFNoC. Set this to the width of the widest port.
  localparam [31:0] ENET_W = (QSFP0_W > QSFP1_W) ? QSFP0_W : QSFP1_W;
  // Actual width of each SFP port's transport adapter interface. Each port of
  // the same QSFP must have the same width.
  localparam [8*32-1:0] ENET_WIDTHS = {{4{QSFP1_W}}, {4{QSFP0_W}}};

  localparam RFNOC_PROTOVER = `RFNOC_PROTOVER;
  localparam NET_CHDR_W     = ENET_W;
  localparam CHDR_W         = `CHDR_WIDTH;
  localparam DMA_W          = `CHDR_WIDTH;

  localparam REG_AWIDTH     = 15;
  localparam REG_DWIDTH     = 32;

  // Log2 of the maximum transmission unit (MTU)
  localparam BYTE_MTU = $clog2(8192);                  // MTU in bytes
  localparam CHDR_MTU = BYTE_MTU - $clog2(CHDR_W/8);   // MTU in CHDR words


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  // Clocking and sync signals for RFDC
  wire pll_ref_clk_in, pll_ref_clk;
  wire sysref_pl;
  wire base_ref_clk;

  // Buffer the incoming RFDC PLL clock
  IBUFGDS ibufgds_pll_ref_clk (
    .O  (pll_ref_clk_in),
    .I  (PLL_REFCLK_FPGA_P),
    .IB (PLL_REFCLK_FPGA_N)
  );

  assign DB0_SYNTH_SYNC = 1'b0;
  assign DB1_SYNTH_SYNC = 1'b0;

  // Buffer the incoming RFDC PL SYSREF
  IBUFGDS ibufgds_pl_sysref (
    .O  (sysref_pl),
    .I  (SYSREF_FABRIC_P),
    .IB (SYSREF_FABRIC_N)
  );

  // Buffer the incoming base reference clock
  IBUFGDS ibufgds_base_ref_clk (
    .O  (base_ref_clk),
    .I  (BASE_REFCLK_FPGA_P),
    .IB (BASE_REFCLK_FPGA_N)
  );

  // Clocking signals for RF data processing/moving
  wire [NUM_DBOARDS-1:0] rfdc_clk;
  wire [NUM_DBOARDS-1:0] rfdc_clk_2x;
  wire data_clk;
  wire data_clk_2x;
  wire [NUM_DBOARDS-1:0] radio_clk;
  wire [NUM_DBOARDS-1:0] radio_clk_2x;

  // Low-power output clocks from PS to PL
  wire clk40;  //  40.000 MHz
  wire clk100; // 100.000 MHz
  wire clk166; // 166.667 MHz
  wire clk200; // 200.000 MHz
  wire bus_clk;

  // Asynchronous resets from PS to PL
  wire pl_resetn0;
  wire areset;

  assign areset = ~pl_resetn0;

  // Synchronous resets derived from the reset coming from the PS
  wire clk40_rst, clk40_rstn;
  wire clk166_rst;
  wire clk200_rst;
  wire bus_clk_rst, bus_clk_rstn;
  wire brc_rst;
  wire prc_rst;
  wire [NUM_DBOARDS-1:0] radio_rst;

  reset_sync reset_sync_clk40 (
    .clk       (clk40),
    .reset_in  (areset),
    .reset_out (clk40_rst)
  );

  reset_sync reset_sync_clk166 (
    .clk       (clk166),
    .reset_in  (areset),
    .reset_out (clk166_rst)
  );

  reset_sync reset_sync_clk200 (
    .clk       (clk200),
    .reset_in  (areset),
    .reset_out (clk200_rst)
  );

  genvar db_i;
  generate
    for ( db_i = 0; db_i < NUM_DBOARDS; db_i = db_i + 1 ) begin: reset_sync_gen
      reset_sync reset_sync_radio (
        .clk       (radio_clk[db_i]),
        .reset_in  (areset),
        .reset_out (radio_rst[db_i])
      );
    end

    // Set radio clock and bus clk
    if (RF_CORE == "FULL") begin: radio_rfdc_clk_gen
      assign radio_clk    = rfdc_clk; // separate radio clocks for each db
      assign radio_clk_2x = rfdc_clk_2x;
      assign bus_clk = clk200;
      assign bus_clk_rst = clk200_rst;
    end else begin: radio_data_clk_gen
      assign radio_clk    = {NUM_DBOARDS{data_clk}};
      assign radio_clk_2x = {NUM_DBOARDS{data_clk_2x}};
      assign bus_clk = clk200;
      assign bus_clk_rst = clk200_rst;
    end
  endgenerate
  localparam CHDR_CLK_RATE = 'd200000000;

  reset_sync reset_sync_brc (
    .clk       (base_ref_clk),
    .reset_in  (areset),
    .reset_out (brc_rst)
  );

  reset_sync reset_sync_prc (
    .clk       (pll_ref_clk),
    .reset_in  (areset),
    .reset_out (prc_rst)
  );

  // Invert reset for various modules.
  assign clk40_rstn  = ~clk40_rst;
  assign bus_clk_rstn = ~bus_clk_rst;

  // Need a 5 MHz pulse on X440 to use as a sync ref
  `ifdef X440
    wire clk5;

    clock_div #(
      .N(2)
    ) clock_div_5mhz (
      .clk_in     (base_ref_clk),
      .clk_in_rst (brc_rst),
      .clk_out    (clk5)
    );
  `endif

  // Compute Engine Clock
  //
  //----------------------------------------------------------------------------
  //  Output     Output      Phase    Duty Cycle   Pk-to-Pk     Phase
  //   Clock     Freq (MHz)  (degrees)    (%)     Jitter (ps)  Error (ps)
  //----------------------------------------------------------------------------
  // __ce_clk__266.66667______0.000______50.0_______94.562_____97.786
  //
  //----------------------------------------------------------------------------
  // Input Clock   Freq (MHz)    Input Jitter (UI)
  //----------------------------------------------------------------------------
  // __primary_____________200____________0.010

  wire ce_gen_clkfbout;
  wire ce_gen_clkout0;
  wire ce_clk;

  PLLE4_ADV #(
    .COMPENSATION      ("AUTO" ),
    .STARTUP_WAIT      ("FALSE"),
    .DIVCLK_DIVIDE     (1      ),
    .CLKFBOUT_MULT     (4      ),
    .CLKFBOUT_PHASE    (0.000  ),
    .CLKOUT0_DIVIDE    (3      ),
    .CLKOUT0_PHASE     (0.000  ),
    .CLKOUT0_DUTY_CYCLE(0.500  ),
    .CLKIN_PERIOD      (5.000  )
  ) ce_clk_gen_i (
    .CLKFBOUT   (ce_gen_clkfbout),
    .CLKOUT0    (ce_gen_clkout0 ),
    .CLKOUT0B   (               ),
    .CLKOUT1    (               ),
    .CLKOUT1B   (               ),
    .CLKFBIN    (ce_gen_clkfbout),
    .CLKIN      (clk200         ),
    .DADDR      (7'h0           ),
    .DCLK       (1'b0           ),
    .DEN        (1'b0           ),
    .DI         (16'h0          ),
    .DO         (               ),
    .DRDY       (               ),
    .DWE        (1'b0           ),
    .CLKOUTPHYEN(1'b0           ),
    .CLKOUTPHY  (               ),
    .LOCKED     (               ),
    .PWRDWN     (1'b0           ),
    .RST        (areset         )
  );

  BUFG bufg_ce_i (
    .O(ce_clk        ),
    .I(ce_gen_clkout0)
  );


  //---------------------------------------------------------------------------
  // PPS Handling
  //---------------------------------------------------------------------------

  wire        pps_refclk;
  wire [ 1:0] pps_radioclk;
  wire [ 1:0] pps_select;
  wire        pll_sync_trigger;
  wire        pll_sync_done;
  wire [ 7:0] pll_sync_delay;
  wire [ 7:0] pps_brc_delay;
  wire [25:0] pps_prc_delay;
  wire [ 9:0] prc_rc_divider;
  wire        pps_rc_enabled;

  x4xx_pps_sync x4xx_pps_sync_i (
    .base_ref_clk     (base_ref_clk),
    .pll_ref_clk      (pll_ref_clk),
    .ctrl_clk         (clk40),
    .radio_clk        (radio_clk),
    .brc_rst          (brc_rst),
    .pps_in           (PPS_IN),
    .pps_out_brc      (pps_refclk),
    .pps_out_rc       (pps_radioclk),
    .sync             (LMK_SYNC),
    .pps_select       (pps_select),
    .pll_sync_trigger (pll_sync_trigger),
    .pll_sync_delay   (pll_sync_delay),
    .pll_sync_done    (pll_sync_done),
    .pps_brc_delay    (pps_brc_delay),
    .pps_prc_delay    (pps_prc_delay),
    .prc_rc_divider   (prc_rc_divider),
    .pps_rc_enabled   (pps_rc_enabled),
    .debug            ()
  );

  // IMPORTANT! Trigger I/O tri-sate buffer is controlled through a SW API that
  // also switches external buffers on the X410 mboard and clocking aux board.
  //
  // SW must ensure that any downstream device receiving TRIG_IO ignores or
  // re-synchronizes after enabling this port.
  wire [1:0] trig_io_select;
  assign TRIG_IO = (trig_io_select == TRIG_IO_PPS_OUTPUT) ? pps_refclk : 1'bz;
  assign PPS_LED = pps_refclk;


  //---------------------------------------------------------------------------
  // Processor System (PS) + RF Data Converter (RFDC)
  //---------------------------------------------------------------------------

  wire [ 48:0] axi_hp0_araddr;
  wire [  1:0] axi_hp0_arburst;
  wire [  3:0] axi_hp0_arcache;
  wire [  5:0] axi_hp0_arid;
  wire [  7:0] axi_hp0_arlen;
  wire         axi_hp0_arlock;
  wire [  2:0] axi_hp0_arprot;
  wire [  3:0] axi_hp0_arqos;
  wire         axi_hp0_arready;
  wire [  2:0] axi_hp0_arsize;
  wire         axi_hp0_aruser;
  wire         axi_hp0_arvalid;
  wire [ 48:0] axi_hp0_awaddr;
  wire [  1:0] axi_hp0_awburst;
  wire [  3:0] axi_hp0_awcache;
  wire [  5:0] axi_hp0_awid;
  wire [  7:0] axi_hp0_awlen;
  wire         axi_hp0_awlock;
  wire [  2:0] axi_hp0_awprot;
  wire [  3:0] axi_hp0_awqos;
  wire         axi_hp0_awready;
  wire [  2:0] axi_hp0_awsize;
  wire         axi_hp0_awuser;
  wire         axi_hp0_awvalid;
  wire         axi_hp0_bready;
  wire [  1:0] axi_hp0_bresp;
  wire         axi_hp0_bvalid;
  wire [127:0] axi_hp0_rdata;
  wire         axi_hp0_rlast;
  wire         axi_hp0_rready;
  wire [  1:0] axi_hp0_rresp;
  wire         axi_hp0_rvalid;
  wire [127:0] axi_hp0_wdata;
  wire         axi_hp0_wlast;
  wire         axi_hp0_wready;
  wire [ 15:0] axi_hp0_wstrb;
  wire         axi_hp0_wvalid;

  wire [ 48:0] axi_hp1_araddr;
  wire [  1:0] axi_hp1_arburst;
  wire [  3:0] axi_hp1_arcache;
  wire [  5:0] axi_hp1_arid;
  wire [  7:0] axi_hp1_arlen;
  wire         axi_hp1_arlock;
  wire [  2:0] axi_hp1_arprot;
  wire [  3:0] axi_hp1_arqos;
  wire         axi_hp1_arready;
  wire [  2:0] axi_hp1_arsize;
  wire         axi_hp1_aruser;
  wire         axi_hp1_arvalid;
  wire [ 48:0] axi_hp1_awaddr;
  wire [  1:0] axi_hp1_awburst;
  wire [  3:0] axi_hp1_awcache;
  wire [  5:0] axi_hp1_awid;
  wire [  7:0] axi_hp1_awlen;
  wire         axi_hp1_awlock;
  wire [  2:0] axi_hp1_awprot;
  wire [  3:0] axi_hp1_awqos;
  wire         axi_hp1_awready;
  wire [  2:0] axi_hp1_awsize;
  wire         axi_hp1_awuser;
  wire         axi_hp1_awvalid;
  wire         axi_hp1_bready;
  wire [  1:0] axi_hp1_bresp;
  wire         axi_hp1_bvalid;
  wire [127:0] axi_hp1_rdata;
  wire         axi_hp1_rlast;
  wire         axi_hp1_rready;
  wire [  1:0] axi_hp1_rresp;
  wire         axi_hp1_rvalid;
  wire [127:0] axi_hp1_wdata;
  wire         axi_hp1_wlast;
  wire         axi_hp1_wready;
  wire [ 15:0] axi_hp1_wstrb;
  wire         axi_hp1_wvalid;

  wire [ 39:0] m_axi_app_araddr;
  wire [  2:0] m_axi_app_arprot;
  wire [  0:0] m_axi_app_arready;
  wire [  0:0] m_axi_app_arvalid;
  wire [ 39:0] m_axi_app_awaddr;
  wire [  2:0] m_axi_app_awprot;
  wire [  0:0] m_axi_app_awready;
  wire [  0:0] m_axi_app_awvalid;
  wire [  0:0] m_axi_app_bready;
  wire [  1:0] m_axi_app_bresp;
  wire [  0:0] m_axi_app_bvalid;
  wire [ 31:0] m_axi_app_rdata;
  wire [  0:0] m_axi_app_rready;
  wire [  1:0] m_axi_app_rresp;
  wire [  0:0] m_axi_app_rvalid;
  wire [ 31:0] m_axi_app_wdata;
  wire [  0:0] m_axi_app_wready;
  wire [  3:0] m_axi_app_wstrb;
  wire [  0:0] m_axi_app_wvalid;
  wire [ 39:0] m_axi_mpm_ep_araddr;
  wire [  0:0] m_axi_mpm_ep_arready;
  wire [  0:0] m_axi_mpm_ep_arvalid;
  wire [ 39:0] m_axi_mpm_ep_awaddr;
  wire [  0:0] m_axi_mpm_ep_awready;
  wire [  0:0] m_axi_mpm_ep_awvalid;
  wire [  0:0] m_axi_mpm_ep_bready;
  wire [  1:0] m_axi_mpm_ep_bresp;
  wire [  0:0] m_axi_mpm_ep_bvalid;
  wire [ 31:0] m_axi_mpm_ep_rdata;
  wire [  0:0] m_axi_mpm_ep_rready;
  wire [  1:0] m_axi_mpm_ep_rresp;
  wire [  0:0] m_axi_mpm_ep_rvalid;
  wire [ 31:0] m_axi_mpm_ep_wdata;
  wire [  0:0] m_axi_mpm_ep_wready;
  wire [  3:0] m_axi_mpm_ep_wstrb;
  wire [  0:0] m_axi_mpm_ep_wvalid;

  wire                       adc_data_out_resetn_dclk;
  wire [NUM_TIMEKEEPERS-1:0] adc_enable_data_rclk;
  wire [NUM_TIMEKEEPERS-1:0] adc_rfdc_axi_resetn_rclk;

  wire                       dac_data_in_resetn_dclk;
  wire                       dac_data_in_resetn_dclk2x;
  wire [NUM_TIMEKEEPERS-1:0] dac_data_in_resetn_rclk;
  wire [NUM_TIMEKEEPERS-1:0] dac_data_in_resetn_rclk2x;

  wire fir_resetn_rclk2x;

  wire [3:0]  eth0_link_up, eth0_activity;
  wire [3:0]  eth1_link_up, eth1_activity;

  wire [63:0] gpio_0_tri_i;
  wire [63:0] gpio_0_tri_o;
  wire [63:0] gpio_0_tri_t;

  //---------------------------------------------------------------------------
  // RF Channels
  //---------------------------------------------------------------------------

  `ifdef X410
    localparam NUM_CH_PER_DB = 2;
  `elsif X440
    localparam NUM_CH_PER_DB = 4;
  `else
    localparam NUM_CH_PER_DB = 2;
  `endif

  localparam NUM_CHANNELS            = NUM_DBOARDS*NUM_CH_PER_DB;

  // RFDC AXI4-Stream interfaces
  //
  // All these signals/vectors are in the rfdc_clk domain.
  //
  // ADC:
  //
  // I/Q data comes from the RFDC in two vectors: I and Q. Each vector contains
  // up to 8 SPC depending upon the decimation performed by the RFDC. When
  // lower data rates are used (higher decimation), the LSBs will contain the
  // valid samples. The data is packed in each vector as follows:
  //
  //             ____________              ____________              _
  //  rfdc_clk _|            |____________|            |____________|
  //           _ _________________________ _________________________ _
  // *_i_tdata _X_i7,i6,i5,i4,i3,i2,i1,i0_X_______i15,...,i8________X_
  //           _ _________________________ _________________________ _
  // *_q_tdata _X_q7,q6,q5,q4,q3,q2,q1,q0_X_______q15,...,q8________X_
  //
  wire [127:0] adc_tile_dout_i_tdata  [0:NUM_CHANNELS-1]; // Up to 8 SPC (I)
  wire [127:0] adc_tile_dout_q_tdata  [0:NUM_CHANNELS-1]; // Up to 8 SPC (Q)
  wire [NUM_CHANNELS-1:0]   adc_tile_dout_i_tready;
  wire [NUM_CHANNELS-1:0]   adc_tile_dout_q_tready;
  wire [NUM_CHANNELS-1:0]   adc_tile_dout_i_tvalid;
  wire [NUM_CHANNELS-1:0]   adc_tile_dout_q_tvalid;
  //
  // DAC:
  //
  // I/Q data is interleaved to the RFDC in a single vector. This vector
  // contains up to 8 SPC depending upon the interpolation performed by the
  // RFDC. When lower data rates are used (higher interpolation), valid samples
  // need to be in the LSBs. The data is packed in the vector as follows:
  //
  //            ____________              ____________              _
  // rfdc_clk _|            |____________|            |____________|
  //          _ _________________________ _________________________ _
  //  *_tdata _X__q7,i7,q6,i6,...,q0,i0__X____q15,i15,...,q8,i8____X_
  //
  wire [255:0] dac_tile_din_tdata     [0:NUM_CHANNELS-1]; // Up to 8 SPC (I + Q)
  wire [NUM_CHANNELS-1:0]   dac_tile_din_tready;
  wire [NUM_CHANNELS-1:0]   dac_tile_din_tvalid;

  // Control/status vectors to rf_core (clk40 domain)
  wire [31:0] rf_dsp_info_clk40;
  wire [31:0] rf_rfdc_info_clk40;
  wire [31:0] rf_axi_status_clk40;
  // Invert controls to rf_core_100m (rfdc_clk_2x domain)
  wire [7:0]  invert_adc_iq_rclk2;
  wire [7:0]  invert_dac_iq_rclk2;
  wire [7:0]  swapped_invert_adc_iq_rclk2;
  wire [7:0]  swapped_invert_dac_iq_rclk2;

  // AXI4-Lite control bus in the clk40 domain
  wire [            39:0] axi_core_awaddr;
  wire                    axi_core_awvalid;
  wire                    axi_core_awready;
  wire [  REG_DWIDTH-1:0] axi_core_wdata;
  wire [REG_DWIDTH/8-1:0] axi_core_wstrb;
  wire                    axi_core_wvalid;
  wire                    axi_core_wready;
  wire [             1:0] axi_core_bresp;
  wire                    axi_core_bvalid;
  wire                    axi_core_bready;
  wire [            39:0] axi_core_araddr;
  wire                    axi_core_arvalid;
  wire                    axi_core_arready;
  wire [  REG_DWIDTH-1:0] axi_core_rdata;
  wire [             1:0] axi_core_rresp;
  wire                    axi_core_rvalid;
  wire                    axi_core_rready;

  // AXI4-Lite Ethernet internal control bus (clk40 domain)
  wire [            39:0] axi_eth_internal_awaddr;
  wire                    axi_eth_internal_awvalid;
  wire                    axi_eth_internal_awready;
  wire [  REG_DWIDTH-1:0] axi_eth_internal_wdata;
  wire [REG_DWIDTH/8-1:0] axi_eth_internal_wstrb;
  wire                    axi_eth_internal_wvalid;
  wire                    axi_eth_internal_wready;
  wire [             1:0] axi_eth_internal_bresp;
  wire                    axi_eth_internal_bvalid;
  wire                    axi_eth_internal_bready;
  wire [            39:0] axi_eth_internal_araddr;
  wire                    axi_eth_internal_arvalid;
  wire                    axi_eth_internal_arready;
  wire [  REG_DWIDTH-1:0] axi_eth_internal_rdata;
  wire [             1:0] axi_eth_internal_rresp;
  wire                    axi_eth_internal_rvalid;
  wire                    axi_eth_internal_rready;

  // Internal Ethernet xport adapter to PS (bus_clk domain)
  wire [63:0] e2h_dma_tdata;
  wire [ 7:0] e2h_dma_tkeep;
  wire        e2h_dma_tlast;
  wire        e2h_dma_tready;
  wire        e2h_dma_tvalid;
  wire [63:0] h2e_dma_tdata;
  wire [ 7:0] h2e_dma_tkeep;
  wire        h2e_dma_tlast;
  wire        h2e_dma_tready;
  wire        h2e_dma_tvalid;

  wire [3:0] eth0_rx_irq;
  wire [3:0] eth0_tx_irq;
  wire [3:0] eth1_rx_irq;
  wire [3:0] eth1_tx_irq;

  // RF reset control
  wire                       nco_reset_done;
  wire                       start_nco_reset;
  wire [NUM_TIMEKEEPERS-1:0] adc_reset_pulse;
  wire [NUM_TIMEKEEPERS-1:0] dac_reset_pulse;

  // Rear panel LEDs control
  //
  // Each LED is comprised of a green (LSB) and a red (MSB) LED which the user
  // can control through a 2-bit vector once fabric LED control is configured
  // on the X410's Linux shell.
  localparam LED_OFF   = 2'b00;
  localparam LED_GREEN = 2'b01;
  localparam LED_RED   = 2'b10;
  localparam LED_AMBER = 2'b11;

  wire [1:0] user_led_ctrl [0:2];
  assign user_led_ctrl[0] = LED_GREEN;
  assign user_led_ctrl[1] = LED_RED;
  assign user_led_ctrl[2] = LED_AMBER;

  // Unused AXI signals
  assign axi_hp0_arid   = 0;
  assign axi_hp0_aruser = 0;
  assign axi_hp0_awid   = 0;
  assign axi_hp0_awuser = 0;
  assign axi_hp1_arid   = 0;
  assign axi_hp1_aruser = 0;
  assign axi_hp1_awid   = 0;
  assign axi_hp1_awuser = 0;

  // Interrupt mapping
  wire [7:0] pl_ps_irq0;
  wire [7:2] pl_ps_irq1;

  assign pl_ps_irq0    = 8'b0;

  assign pl_ps_irq1[2] = 1'b0;
  assign pl_ps_irq1[3] = 1'b0;
  assign pl_ps_irq1[4] = eth0_rx_irq[0] || eth0_rx_irq[1] || eth0_rx_irq[2] || eth0_rx_irq[3];
  assign pl_ps_irq1[5] = eth0_tx_irq[0] || eth0_tx_irq[1] || eth0_tx_irq[2] || eth0_tx_irq[3];
  assign pl_ps_irq1[6] = eth1_rx_irq[0] || eth1_rx_irq[1] || eth1_rx_irq[2] || eth1_rx_irq[3];
  assign pl_ps_irq1[7] = eth1_tx_irq[0] || eth1_tx_irq[1] || eth1_tx_irq[2] || eth1_tx_irq[3];

  // BD DIO signals
  wire [11:0] ps_gpio_out_a;
  wire [11:0] ps_gpio_in_a;
  wire [11:0] ps_gpio_ddr_a;
  wire [11:0] ps_gpio_out_b;
  wire [11:0] ps_gpio_in_b;
  wire [11:0] ps_gpio_ddr_b;

  // GPIO inputs (assigned from 63 decreasing)
  //
  // DIO Control
  assign gpio_0_tri_i[63:56] = 8'b0;
  assign gpio_0_tri_i[55:44] = ps_gpio_in_b;
  assign gpio_0_tri_i[43:32] = ps_gpio_in_a;
  // Make the current PPS signal available to the PS.
  assign gpio_0_tri_i[31]    = pps_refclk;
  assign gpio_0_tri_i[30]    = 0; //unused
  //QSFP+ module present signals
  assign gpio_0_tri_i[29]    = QSFP1_MODPRS_n;
  assign gpio_0_tri_i[28]    = QSFP0_MODPRS_n;
  assign gpio_0_tri_i[27:24] = 4'b0; // unused
  assign gpio_0_tri_i[23]    = eth1_link_up[3];
  assign gpio_0_tri_i[22]    = eth1_link_up[2];
  assign gpio_0_tri_i[21]    = eth1_link_up[1];
  assign gpio_0_tri_i[20]    = eth1_link_up[0];
  assign gpio_0_tri_i[19]    = eth0_link_up[3];
  assign gpio_0_tri_i[18]    = eth0_link_up[2];
  assign gpio_0_tri_i[17]    = eth0_link_up[1];
  assign gpio_0_tri_i[16]    = eth0_link_up[0];
  assign gpio_0_tri_i[15:14] = 2'b0;
  assign gpio_0_tri_i[13:12] = user_led_ctrl[2];
  assign gpio_0_tri_i[11:10] = user_led_ctrl[1];
  assign gpio_0_tri_i[9:8]   = user_led_ctrl[0];
  assign gpio_0_tri_i[7:0]   = 8'b0; // unused

  // GPIO outputs (assigned from 0 increasing)
  //
  // Drive the JTAG level translator enable line (active low) with GPIO[0] from
  // the PS.
  assign CPLD_JTAG_OE_n = gpio_0_tri_o[0];
  // Drive the CPLD JTAG enable line (active high) with GPIO[1] from the PS.
  assign PL_CPLD_JTAGEN = gpio_0_tri_o[1];

  // propagate db GPIO direction and output control
  assign ps_gpio_ddr_a = gpio_0_tri_t[43:32];
  assign ps_gpio_out_a = gpio_0_tri_o[43:32];

  assign ps_gpio_ddr_b = gpio_0_tri_t[55:44];
  assign ps_gpio_out_b = gpio_0_tri_o[55:44];

  `ifdef X410
    x410_ps_rfdc_bd x410_ps_rfdc_bd_i (
      .adc_data_out_resetn_dclk      (adc_data_out_resetn_dclk),
      .adc_enable_data_rclk          (adc_enable_data_rclk),
      .adc_reset_pulse_dclk          (adc_reset_pulse),
      .adc_rfdc_axi_resetn_rclk      (adc_rfdc_axi_resetn_rclk),
      .bus_clk                       (bus_clk),
      .bus_rstn                      (bus_clk_rstn),
      .clk40                         (clk40),
      .clk40_rstn                    (clk40_rstn),
      .dac_data_in_resetn_dclk       (dac_data_in_resetn_dclk),
      .dac_data_in_resetn_dclk2x     (dac_data_in_resetn_dclk2x),
      .dac_data_in_resetn_rclk       (dac_data_in_resetn_rclk),
      .dac_data_in_resetn_rclk2x     (dac_data_in_resetn_rclk2x),
      .dac_reset_pulse_dclk          (dac_reset_pulse),
      .data_clk                      (data_clk),
      .data_clk_2x                   (data_clk_2x),
      .data_clock_locked             (),
      .enable_gated_clocks_clk40     (1'b1),
      .enable_sysref_rclk            (1'b1),
      .fir_resetn_rclk2x             (fir_resetn_rclk2x),
      .gated_base_clks_valid_clk40   (),
      .radio0_invert_adc_iq_r0clk    (invert_adc_iq_rclk2[3:0]),
      .radio0_invert_dac_iq_r0clk    (invert_dac_iq_rclk2[3:0]),
      .radio1_invert_adc_iq_r1clk    (invert_adc_iq_rclk2[7:4]),
      .radio1_invert_dac_iq_r1clk    (invert_dac_iq_rclk2[7:4]),
      .irq0_lpd_rpu_n                (1'b1),
      .irq1_lpd_rpu_n                (1'b1),
      .jtag0_tck                     (),
      .jtag0_tdi                     (),
      .jtag0_tdo                     (),
      .jtag0_tms                     (),
      .nco_reset_done_dclk           (nco_reset_done),
      .pl_clk40                      (clk40),
      .pl_clk100                     (clk100),
      .pl_clk166                     (clk166),
      .pl_clk200                     (clk200),
      .pl_ps_irq0                    (pl_ps_irq0),
      .pl_ps_irq1                    (pl_ps_irq1),
      .pl_resetn0                    (pl_resetn0),
      .pl_resetn1                    (),
      .pl_resetn2                    (),
      .pl_resetn3                    (),
      .pll_ref_clk_in                (pll_ref_clk_in),
      .pll_ref_clk_out               (pll_ref_clk),
      .rf_axi_status_clk40           (rf_axi_status_clk40),
      .rf_dsp_info_clk40             (rf_dsp_info_clk40),
      .rf_rfdc_info_clk40            (rf_rfdc_info_clk40),
      .rfdc_clk                      (rfdc_clk[0]),
      .rfdc_clk_2x                   (rfdc_clk_2x[0]),
      .rfdc_irq                      (),
      .s_axi_hp0_aclk                (clk40),
      .s_axi_hp1_aclk                (clk40),
      .s_axi_hpc0_aclk               (),
      .start_nco_reset_dclk          (start_nco_reset),
      .sysref_out_pclk               (),
      .sysref_out_rclk               (),
      .sysref_pl_in                  (sysref_pl),
    `ifdef QSFP0_0
      .s_axi_hp0_aruser              (axi_hp0_aruser),
      .s_axi_hp0_awuser              (axi_hp0_awuser),
      .s_axi_hp0_awid                (axi_hp0_awid),
      .s_axi_hp0_awaddr              (axi_hp0_awaddr),
      .s_axi_hp0_awlen               (axi_hp0_awlen),
      .s_axi_hp0_awsize              (axi_hp0_awsize),
      .s_axi_hp0_awburst             (axi_hp0_awburst),
      .s_axi_hp0_awlock              (axi_hp0_awlock),
      .s_axi_hp0_awcache             (axi_hp0_awcache),
      .s_axi_hp0_awprot              (axi_hp0_awprot),
      .s_axi_hp0_awvalid             (axi_hp0_awvalid),
      .s_axi_hp0_awready             (axi_hp0_awready),
      .s_axi_hp0_wdata               (axi_hp0_wdata),
      .s_axi_hp0_wstrb               (axi_hp0_wstrb),
      .s_axi_hp0_wlast               (axi_hp0_wlast),
      .s_axi_hp0_wvalid              (axi_hp0_wvalid),
      .s_axi_hp0_wready              (axi_hp0_wready),
      .s_axi_hp0_bid                 (),
      .s_axi_hp0_bresp               (axi_hp0_bresp),
      .s_axi_hp0_bvalid              (axi_hp0_bvalid),
      .s_axi_hp0_bready              (axi_hp0_bready),
      .s_axi_hp0_arid                (axi_hp0_arid),
      .s_axi_hp0_araddr              (axi_hp0_araddr),
      .s_axi_hp0_arlen               (axi_hp0_arlen),
      .s_axi_hp0_arsize              (axi_hp0_arsize),
      .s_axi_hp0_arburst             (axi_hp0_arburst),
      .s_axi_hp0_arlock              (axi_hp0_arlock),
      .s_axi_hp0_arcache             (axi_hp0_arcache),
      .s_axi_hp0_arprot              (axi_hp0_arprot),
      .s_axi_hp0_arvalid             (axi_hp0_arvalid),
      .s_axi_hp0_arready             (axi_hp0_arready),
      .s_axi_hp0_rid                 (),
      .s_axi_hp0_rdata               (axi_hp0_rdata),
      .s_axi_hp0_rresp               (axi_hp0_rresp),
      .s_axi_hp0_rlast               (axi_hp0_rlast),
      .s_axi_hp0_rvalid              (axi_hp0_rvalid),
      .s_axi_hp0_rready              (axi_hp0_rready),
      .s_axi_hp0_awqos               (axi_hp0_awqos),
      .s_axi_hp0_arqos               (axi_hp0_arqos),
    `else
      // Disconnect the DMA interface if the associated QSFP port is not used.
      .s_axi_hp0_awvalid             (1'b0),
      .s_axi_hp0_wvalid              (1'b0),
      .s_axi_hp0_bready              (1'b1),
      .s_axi_hp0_arvalid             (1'b0),
      .s_axi_hp0_rready              (1'b1),
    `endif
      .s_axis_eth_dma_tdata          (e2h_dma_tdata),
      .s_axis_eth_dma_tkeep          (e2h_dma_tkeep),
      .s_axis_eth_dma_tlast          (e2h_dma_tlast),
      .s_axis_eth_dma_tready         (e2h_dma_tready),
      .s_axis_eth_dma_tvalid         (e2h_dma_tvalid),
    `ifdef QSFP1_0
      .s_axi_hp1_aruser              (axi_hp1_aruser),
      .s_axi_hp1_awuser              (axi_hp1_awuser),
      .s_axi_hp1_awid                (axi_hp1_awid),
      .s_axi_hp1_awaddr              (axi_hp1_awaddr),
      .s_axi_hp1_awlen               (axi_hp1_awlen),
      .s_axi_hp1_awsize              (axi_hp1_awsize),
      .s_axi_hp1_awburst             (axi_hp1_awburst),
      .s_axi_hp1_awlock              (axi_hp1_awlock),
      .s_axi_hp1_awcache             (axi_hp1_awcache),
      .s_axi_hp1_awprot              (axi_hp1_awprot),
      .s_axi_hp1_awvalid             (axi_hp1_awvalid),
      .s_axi_hp1_awready             (axi_hp1_awready),
      .s_axi_hp1_wdata               (axi_hp1_wdata),
      .s_axi_hp1_wstrb               (axi_hp1_wstrb),
      .s_axi_hp1_wlast               (axi_hp1_wlast),
      .s_axi_hp1_wvalid              (axi_hp1_wvalid),
      .s_axi_hp1_wready              (axi_hp1_wready),
      .s_axi_hp1_bid                 (),
      .s_axi_hp1_bresp               (axi_hp1_bresp),
      .s_axi_hp1_bvalid              (axi_hp1_bvalid),
      .s_axi_hp1_bready              (axi_hp1_bready),
      .s_axi_hp1_arid                (axi_hp1_arid),
      .s_axi_hp1_araddr              (axi_hp1_araddr),
      .s_axi_hp1_arlen               (axi_hp1_arlen),
      .s_axi_hp1_arsize              (axi_hp1_arsize),
      .s_axi_hp1_arburst             (axi_hp1_arburst),
      .s_axi_hp1_arlock              (axi_hp1_arlock),
      .s_axi_hp1_arcache             (axi_hp1_arcache),
      .s_axi_hp1_arprot              (axi_hp1_arprot),
      .s_axi_hp1_arvalid             (axi_hp1_arvalid),
      .s_axi_hp1_arready             (axi_hp1_arready),
      .s_axi_hp1_rid                 (),
      .s_axi_hp1_rdata               (axi_hp1_rdata),
      .s_axi_hp1_rresp               (axi_hp1_rresp),
      .s_axi_hp1_rlast               (axi_hp1_rlast),
      .s_axi_hp1_rvalid              (axi_hp1_rvalid),
      .s_axi_hp1_rready              (axi_hp1_rready),
      .s_axi_hp1_awqos               (axi_hp1_awqos),
      .s_axi_hp1_arqos               (axi_hp1_arqos),
    `else
      // Disconnect the DMA interface if the associated QSFP port is not used.
      .s_axi_hp1_awvalid             (1'b0),
      .s_axi_hp1_wvalid              (1'b0),
      .s_axi_hp1_bready              (1'b1),
      .s_axi_hp1_arvalid             (1'b0),
      .s_axi_hp1_rready              (1'b1),
    `endif
      .s_axi_hpc0_aruser             (),
      .s_axi_hpc0_awuser             (),
      .s_axi_hpc0_awid               (),
      .s_axi_hpc0_awaddr             (),
      .s_axi_hpc0_awlen              (),
      .s_axi_hpc0_awsize             (),
      .s_axi_hpc0_awburst            (),
      .s_axi_hpc0_awlock             (),
      .s_axi_hpc0_awcache            (),
      .s_axi_hpc0_awprot             (),
      .s_axi_hpc0_awvalid            (),
      .s_axi_hpc0_awready            (),
      .s_axi_hpc0_wdata              (),
      .s_axi_hpc0_wstrb              (),
      .s_axi_hpc0_wlast              (),
      .s_axi_hpc0_wvalid             (),
      .s_axi_hpc0_wready             (),
      .s_axi_hpc0_bid                (),
      .s_axi_hpc0_bresp              (),
      .s_axi_hpc0_bvalid             (),
      .s_axi_hpc0_bready             (),
      .s_axi_hpc0_arid               (),
      .s_axi_hpc0_araddr             (),
      .s_axi_hpc0_arlen              (),
      .s_axi_hpc0_arsize             (),
      .s_axi_hpc0_arburst            (),
      .s_axi_hpc0_arlock             (),
      .s_axi_hpc0_arcache            (),
      .s_axi_hpc0_arprot             (),
      .s_axi_hpc0_arvalid            (),
      .s_axi_hpc0_arready            (),
      .s_axi_hpc0_rid                (),
      .s_axi_hpc0_rdata              (),
      .s_axi_hpc0_rresp              (),
      .s_axi_hpc0_rlast              (),
      .s_axi_hpc0_rvalid             (),
      .s_axi_hpc0_rready             (),
      .s_axi_hpc0_awqos              (),
      .s_axi_hpc0_arqos              (),
      .adc0_clk_clk_n                (ADC_CLK_N[0]),
      .adc0_clk_clk_p                (ADC_CLK_P[0]),
      .adc2_clk_clk_n                (ADC_CLK_N[2]),
      .adc2_clk_clk_p                (ADC_CLK_P[2]),
      .m_axi_app_awaddr              (m_axi_app_awaddr),
      .m_axi_app_awprot              (m_axi_app_awprot),
      .m_axi_app_awvalid             (m_axi_app_awvalid),
      .m_axi_app_awready             (m_axi_app_awready),
      .m_axi_app_wdata               (m_axi_app_wdata),
      .m_axi_app_wstrb               (m_axi_app_wstrb),
      .m_axi_app_wvalid              (m_axi_app_wvalid),
      .m_axi_app_wready              (m_axi_app_wready),
      .m_axi_app_bresp               (m_axi_app_bresp),
      .m_axi_app_bvalid              (m_axi_app_bvalid),
      .m_axi_app_bready              (m_axi_app_bready),
      .m_axi_app_araddr              (m_axi_app_araddr),
      .m_axi_app_arprot              (m_axi_app_arprot),
      .m_axi_app_arvalid             (m_axi_app_arvalid),
      .m_axi_app_arready             (m_axi_app_arready),
      .m_axi_app_rdata               (m_axi_app_rdata),
      .m_axi_app_rresp               (m_axi_app_rresp),
      .m_axi_app_rvalid              (m_axi_app_rvalid),
      .m_axi_app_rready              (m_axi_app_rready),
      .dac0_clk_clk_n                (DAC_CLK_N[0]),
      .dac0_clk_clk_p                (DAC_CLK_P[0]),
      .dac1_clk_clk_n                (DAC_CLK_N[1]),
      .dac1_clk_clk_p                (DAC_CLK_P[1]),
      .gpio_0_tri_i                  (gpio_0_tri_i),
      .gpio_0_tri_o                  (gpio_0_tri_o),
      .gpio_0_tri_t                  (gpio_0_tri_t),
      .m_axi_eth_internal_awaddr     (axi_eth_internal_awaddr),
      .m_axi_eth_internal_awprot     (),
      .m_axi_eth_internal_awvalid    (axi_eth_internal_awvalid),
      .m_axi_eth_internal_awready    (axi_eth_internal_awready),
      .m_axi_eth_internal_wdata      (axi_eth_internal_wdata),
      .m_axi_eth_internal_wstrb      (axi_eth_internal_wstrb),
      .m_axi_eth_internal_wvalid     (axi_eth_internal_wvalid),
      .m_axi_eth_internal_wready     (axi_eth_internal_wready),
      .m_axi_eth_internal_bresp      (axi_eth_internal_bresp),
      .m_axi_eth_internal_bvalid     (axi_eth_internal_bvalid),
      .m_axi_eth_internal_bready     (axi_eth_internal_bready),
      .m_axi_eth_internal_araddr     (axi_eth_internal_araddr),
      .m_axi_eth_internal_arprot     (),
      .m_axi_eth_internal_arvalid    (axi_eth_internal_arvalid),
      .m_axi_eth_internal_arready    (axi_eth_internal_arready),
      .m_axi_eth_internal_rdata      (axi_eth_internal_rdata),
      .m_axi_eth_internal_rresp      (axi_eth_internal_rresp),
      .m_axi_eth_internal_rvalid     (axi_eth_internal_rvalid),
      .m_axi_eth_internal_rready     (axi_eth_internal_rready),
      .m_axis_eth_dma_tdata          (h2e_dma_tdata),
      .m_axis_eth_dma_tkeep          (h2e_dma_tkeep),
      .m_axis_eth_dma_tlast          (h2e_dma_tlast),
      .m_axis_eth_dma_tready         (h2e_dma_tready),
      .m_axis_eth_dma_tvalid         (h2e_dma_tvalid),
      .m_axi_rpu_awaddr              (),
      .m_axi_rpu_awprot              (),
      .m_axi_rpu_awvalid             (),
      .m_axi_rpu_awready             (),
      .m_axi_rpu_wdata               (),
      .m_axi_rpu_wstrb               (),
      .m_axi_rpu_wvalid              (),
      .m_axi_rpu_wready              (),
      .m_axi_rpu_bresp               (),
      .m_axi_rpu_bvalid              (),
      .m_axi_rpu_bready              (),
      .m_axi_rpu_araddr              (),
      .m_axi_rpu_arprot              (),
      .m_axi_rpu_arvalid             (),
      .m_axi_rpu_arready             (),
      .m_axi_rpu_rdata               (),
      .m_axi_rpu_rresp               (),
      .m_axi_rpu_rvalid              (),
      .m_axi_rpu_rready              (),
      .m_axi_core_awaddr             (axi_core_awaddr),
      .m_axi_core_awprot             (),
      .m_axi_core_awvalid            (axi_core_awvalid),
      .m_axi_core_awready            (axi_core_awready),
      .m_axi_core_wdata              (axi_core_wdata),
      .m_axi_core_wstrb              (axi_core_wstrb),
      .m_axi_core_wvalid             (axi_core_wvalid),
      .m_axi_core_wready             (axi_core_wready),
      .m_axi_core_bresp              (axi_core_bresp),
      .m_axi_core_bvalid             (axi_core_bvalid),
      .m_axi_core_bready             (axi_core_bready),
      .m_axi_core_araddr             (axi_core_araddr),
      .m_axi_core_arprot             (),
      .m_axi_core_arvalid            (axi_core_arvalid),
      .m_axi_core_arready            (axi_core_arready),
      .m_axi_core_rdata              (axi_core_rdata),
      .m_axi_core_rresp              (axi_core_rresp),
      .m_axi_core_rvalid             (axi_core_rvalid),
      .m_axi_core_rready             (axi_core_rready),
      .m_axi_mpm_ep_awaddr           (m_axi_mpm_ep_awaddr),
      .m_axi_mpm_ep_awprot           (),
      .m_axi_mpm_ep_awvalid          (m_axi_mpm_ep_awvalid),
      .m_axi_mpm_ep_awready          (m_axi_mpm_ep_awready),
      .m_axi_mpm_ep_wdata            (m_axi_mpm_ep_wdata),
      .m_axi_mpm_ep_wstrb            (m_axi_mpm_ep_wstrb),
      .m_axi_mpm_ep_wvalid           (m_axi_mpm_ep_wvalid),
      .m_axi_mpm_ep_wready           (m_axi_mpm_ep_wready),
      .m_axi_mpm_ep_bresp            (m_axi_mpm_ep_bresp),
      .m_axi_mpm_ep_bvalid           (m_axi_mpm_ep_bvalid),
      .m_axi_mpm_ep_bready           (m_axi_mpm_ep_bready),
      .m_axi_mpm_ep_araddr           (m_axi_mpm_ep_araddr),
      .m_axi_mpm_ep_arprot           (),
      .m_axi_mpm_ep_arvalid          (m_axi_mpm_ep_arvalid),
      .m_axi_mpm_ep_arready          (m_axi_mpm_ep_arready),
      .m_axi_mpm_ep_rdata            (m_axi_mpm_ep_rdata),
      .m_axi_mpm_ep_rresp            (m_axi_mpm_ep_rresp),
      .m_axi_mpm_ep_rvalid           (m_axi_mpm_ep_rvalid),
      .m_axi_mpm_ep_rready           (m_axi_mpm_ep_rready),
      .adc_tile224_ch0_dout_i_tdata  (adc_tile_dout_i_tdata[0]),
      .adc_tile224_ch0_dout_i_tready (adc_tile_dout_i_tready[0]),
      .adc_tile224_ch0_dout_i_tvalid (adc_tile_dout_i_tvalid[0]),
      .adc_tile224_ch0_dout_q_tdata  (adc_tile_dout_q_tdata[0]),
      .adc_tile224_ch0_dout_q_tready (adc_tile_dout_q_tready[0]),
      .adc_tile224_ch0_dout_q_tvalid (adc_tile_dout_q_tvalid[0]),
      .adc_tile224_ch1_dout_i_tdata  (adc_tile_dout_i_tdata[1]),
      .adc_tile224_ch1_dout_i_tready (adc_tile_dout_i_tready[1]),
      .adc_tile224_ch1_dout_i_tvalid (adc_tile_dout_i_tvalid[1]),
      .adc_tile224_ch1_dout_q_tdata  (adc_tile_dout_q_tdata[1]),
      .adc_tile224_ch1_dout_q_tready (adc_tile_dout_q_tready[1]),
      .adc_tile224_ch1_dout_q_tvalid (adc_tile_dout_q_tvalid[1]),
      .adc_tile226_ch0_dout_i_tdata  (adc_tile_dout_i_tdata[2]),
      .adc_tile226_ch0_dout_i_tready (adc_tile_dout_i_tready[2]),
      .adc_tile226_ch0_dout_i_tvalid (adc_tile_dout_i_tvalid[2]),
      .adc_tile226_ch0_dout_q_tdata  (adc_tile_dout_q_tdata[2]),
      .adc_tile226_ch0_dout_q_tready (adc_tile_dout_q_tready[2]),
      .adc_tile226_ch0_dout_q_tvalid (adc_tile_dout_q_tvalid[2]),
      .adc_tile226_ch1_dout_i_tdata  (adc_tile_dout_i_tdata[3]),
      .adc_tile226_ch1_dout_i_tready (adc_tile_dout_i_tready[3]),
      .adc_tile226_ch1_dout_i_tvalid (adc_tile_dout_i_tvalid[3]),
      .adc_tile226_ch1_dout_q_tdata  (adc_tile_dout_q_tdata[3]),
      .adc_tile226_ch1_dout_q_tready (adc_tile_dout_q_tready[3]),
      .adc_tile226_ch1_dout_q_tvalid (adc_tile_dout_q_tvalid[3]),
      .dac_tile228_ch0_vout_v_n      (DB0_TX_N[0]),
      .dac_tile228_ch0_vout_v_p      (DB0_TX_P[0]),
      .dac_tile228_ch1_vout_v_n      (DB0_TX_N[1]),
      .dac_tile228_ch1_vout_v_p      (DB0_TX_P[1]),
      .dac_tile229_ch0_vout_v_n      (DB1_TX_N[0]),
      .dac_tile229_ch0_vout_v_p      (DB1_TX_P[0]),
      .dac_tile229_ch1_vout_v_n      (DB1_TX_N[1]),
      .dac_tile229_ch1_vout_v_p      (DB1_TX_P[1]),
      .dac_tile228_ch0_din_tdata     (dac_tile_din_tdata[0]),
      .dac_tile228_ch0_din_tvalid    (dac_tile_din_tvalid[0]),
      .dac_tile228_ch0_din_tready    (dac_tile_din_tready[0]),
      .dac_tile228_ch1_din_tdata     (dac_tile_din_tdata[1]),
      .dac_tile228_ch1_din_tvalid    (dac_tile_din_tvalid[1]),
      .dac_tile228_ch1_din_tready    (dac_tile_din_tready[1]),
      .dac_tile229_ch0_din_tdata     (dac_tile_din_tdata[2]),
      .dac_tile229_ch0_din_tvalid    (dac_tile_din_tvalid[2]),
      .dac_tile229_ch0_din_tready    (dac_tile_din_tready[2]),
      .dac_tile229_ch1_din_tdata     (dac_tile_din_tdata[3]),
      .dac_tile229_ch1_din_tvalid    (dac_tile_din_tvalid[3]),
      .dac_tile229_ch1_din_tready    (dac_tile_din_tready[3]),
      .s_axi_hpc1_awid               (),
      .s_axi_hpc1_awaddr             (),
      .s_axi_hpc1_awlen              (),
      .s_axi_hpc1_awsize             (),
      .s_axi_hpc1_awburst            (),
      .s_axi_hpc1_awlock             (),
      .s_axi_hpc1_awcache            (),
      .s_axi_hpc1_awprot             (),
      .s_axi_hpc1_awqos              (),
      .s_axi_hpc1_awvalid            (),
      .s_axi_hpc1_awready            (),
      .s_axi_hpc1_wdata              (),
      .s_axi_hpc1_wstrb              (),
      .s_axi_hpc1_wlast              (),
      .s_axi_hpc1_wvalid             (),
      .s_axi_hpc1_wready             (),
      .s_axi_hpc1_bid                (),
      .s_axi_hpc1_bresp              (),
      .s_axi_hpc1_bvalid             (),
      .s_axi_hpc1_bready             (),
      .s_axi_hpc1_arid               (),
      .s_axi_hpc1_araddr             (),
      .s_axi_hpc1_arlen              (),
      .s_axi_hpc1_arsize             (),
      .s_axi_hpc1_arburst            (),
      .s_axi_hpc1_arlock             (),
      .s_axi_hpc1_arcache            (),
      .s_axi_hpc1_arprot             (),
      .s_axi_hpc1_arqos              (),
      .s_axi_hpc1_arvalid            (),
      .s_axi_hpc1_arready            (),
      .s_axi_hpc1_rid                (),
      .s_axi_hpc1_rdata              (),
      .s_axi_hpc1_rresp              (),
      .s_axi_hpc1_rlast              (),
      .s_axi_hpc1_rvalid             (),
      .s_axi_hpc1_rready             (),
      .sysref_rf_in_diff_n           (SYSREF_RF_N),
      .sysref_rf_in_diff_p           (SYSREF_RF_P),
      .adc_tile224_ch0_vin_v_n       (DB0_RX_N[0]),
      .adc_tile224_ch0_vin_v_p       (DB0_RX_P[0]),
      .adc_tile224_ch1_vin_v_n       (DB0_RX_N[1]),
      .adc_tile224_ch1_vin_v_p       (DB0_RX_P[1]),
      .adc_tile226_ch0_vin_v_n       (DB1_RX_N[0]),
      .adc_tile226_ch0_vin_v_p       (DB1_RX_P[0]),
      .adc_tile226_ch1_vin_v_n       (DB1_RX_N[1]),
      .adc_tile226_ch1_vin_v_p       (DB1_RX_P[1]),
      .s_axi_hpc1_aruser             (),
      .s_axi_hpc1_awuser             ()
    );
  `elsif X440
    x440_ps_rfdc_bd x440_ps_rfdc_bd_i (
    .adc_enable_data_r0clk         (adc_enable_data_rclk[0]),
    .adc_enable_data_r1clk         (adc_enable_data_rclk[1]),
    .adc_reset_pulse_r0clk         (adc_reset_pulse[0]),
    .adc_reset_pulse_r1clk         (adc_reset_pulse[1]),
    .adc_rfdc_axi_resetn_r0clk     (adc_rfdc_axi_resetn_rclk[0]),
    .adc_rfdc_axi_resetn_r1clk     (adc_rfdc_axi_resetn_rclk[1]),
    .bus_clk                       (bus_clk),
    .bus_rstn                      (bus_clk_rstn),
    .clk40                         (clk40),
    .clk40_rstn                    (clk40_rstn),
    .dac_data_in_resetn_r0clk      (dac_data_in_resetn_rclk[0]),
    .dac_data_in_resetn_r1clk      (dac_data_in_resetn_rclk[1]),
    .dac_data_in_resetn_r0clk2x    (dac_data_in_resetn_rclk2x[0]),
    .dac_data_in_resetn_r1clk2x    (dac_data_in_resetn_rclk2x[1]),
    .dac_reset_pulse_r0clk         (dac_reset_pulse[0]),
    .dac_reset_pulse_r1clk         (dac_reset_pulse[1]),
    .data_clk                      (data_clk),
    .data_clk_2x                   (data_clk_2x),
    .data_clock_locked             (),
    .enable_gated_clocks_clk40     (1'b1),
    .enable_sysref_r0clk           (1'b1),
    .gated_base_clks_valid_clk40   (),
    .radio0_invert_adc_iq_r0clk    (invert_adc_iq_rclk2[3:0]),
    .radio0_invert_dac_iq_r0clk    (invert_dac_iq_rclk2[3:0]),
    .radio1_invert_adc_iq_r1clk    (invert_adc_iq_rclk2[7:4]),
    .radio1_invert_dac_iq_r1clk    (invert_dac_iq_rclk2[7:4]),
    .irq0_lpd_rpu_n                (1'b1),
    .irq1_lpd_rpu_n                (1'b1),
    .jtag0_tck                     (),
    .jtag0_tdi                     (),
    .jtag0_tdo                     (),
    .jtag0_tms                     (),
    .nco_reset_done_r0clk          (nco_reset_done),
    .pl_clk40                      (clk40),
    .pl_clk100                     (clk100),
    .pl_clk166                     (clk166),
    .pl_clk200                     (clk200),
    .pl_ps_irq0                    (pl_ps_irq0),
    .pl_ps_irq1                    (pl_ps_irq1),
    .pl_resetn0                    (pl_resetn0),
    .pl_resetn1                    (),
    .pl_resetn2                    (),
    .pl_resetn3                    (),
    .pll_ref_clk_in                (pll_ref_clk_in),
    .pll_ref_clk_out               (pll_ref_clk),
    .rf_axi_status_clk40           (rf_axi_status_clk40),
    .rf_dsp_info_clk40             (rf_dsp_info_clk40),
    .rf_rfdc_info_clk40            (rf_rfdc_info_clk40),
    .radio0_rfdc_clk_1x            (rfdc_clk[0]),
    .radio0_rfdc_clk_2x            (rfdc_clk_2x[0]),
    .radio1_rfdc_clk_1x            (rfdc_clk[1]),
    .radio1_rfdc_clk_2x            (rfdc_clk_2x[1]),
    .rfdc_irq                      (),
    .s_axi_hp0_aclk                (clk40),
    .s_axi_hp1_aclk                (clk40),
    .s_axi_hpc0_aclk               (),
    .start_nco_reset_r0clk         (start_nco_reset),
    .sysref_out_pclk               (),
    .sysref_out_r0clk              (),
    .sysref_pl_in                  (sysref_pl),
  `ifdef QSFP0_0
    .s_axi_hp0_aruser              (axi_hp0_aruser),
    .s_axi_hp0_awuser              (axi_hp0_awuser),
    .s_axi_hp0_awid                (axi_hp0_awid),
    .s_axi_hp0_awaddr              (axi_hp0_awaddr),
    .s_axi_hp0_awlen               (axi_hp0_awlen),
    .s_axi_hp0_awsize              (axi_hp0_awsize),
    .s_axi_hp0_awburst             (axi_hp0_awburst),
    .s_axi_hp0_awlock              (axi_hp0_awlock),
    .s_axi_hp0_awcache             (axi_hp0_awcache),
    .s_axi_hp0_awprot              (axi_hp0_awprot),
    .s_axi_hp0_awvalid             (axi_hp0_awvalid),
    .s_axi_hp0_awready             (axi_hp0_awready),
    .s_axi_hp0_wdata               (axi_hp0_wdata),
    .s_axi_hp0_wstrb               (axi_hp0_wstrb),
    .s_axi_hp0_wlast               (axi_hp0_wlast),
    .s_axi_hp0_wvalid              (axi_hp0_wvalid),
    .s_axi_hp0_wready              (axi_hp0_wready),
    .s_axi_hp0_bid                 (),
    .s_axi_hp0_bresp               (axi_hp0_bresp),
    .s_axi_hp0_bvalid              (axi_hp0_bvalid),
    .s_axi_hp0_bready              (axi_hp0_bready),
    .s_axi_hp0_arid                (axi_hp0_arid),
    .s_axi_hp0_araddr              (axi_hp0_araddr),
    .s_axi_hp0_arlen               (axi_hp0_arlen),
    .s_axi_hp0_arsize              (axi_hp0_arsize),
    .s_axi_hp0_arburst             (axi_hp0_arburst),
    .s_axi_hp0_arlock              (axi_hp0_arlock),
    .s_axi_hp0_arcache             (axi_hp0_arcache),
    .s_axi_hp0_arprot              (axi_hp0_arprot),
    .s_axi_hp0_arvalid             (axi_hp0_arvalid),
    .s_axi_hp0_arready             (axi_hp0_arready),
    .s_axi_hp0_rid                 (),
    .s_axi_hp0_rdata               (axi_hp0_rdata),
    .s_axi_hp0_rresp               (axi_hp0_rresp),
    .s_axi_hp0_rlast               (axi_hp0_rlast),
    .s_axi_hp0_rvalid              (axi_hp0_rvalid),
    .s_axi_hp0_rready              (axi_hp0_rready),
    .s_axi_hp0_awqos               (axi_hp0_awqos),
    .s_axi_hp0_arqos               (axi_hp0_arqos),
  `else
    // Disconnect the DMA interface if the associated QSFP port is not used.
    .s_axi_hp0_awvalid             (1'b0),
    .s_axi_hp0_wvalid              (1'b0),
    .s_axi_hp0_bready              (1'b1),
    .s_axi_hp0_arvalid             (1'b0),
    .s_axi_hp0_rready              (1'b1),
  `endif
    .s_axis_eth_dma_tdata          (e2h_dma_tdata),
    .s_axis_eth_dma_tkeep          (e2h_dma_tkeep),
    .s_axis_eth_dma_tlast          (e2h_dma_tlast),
    .s_axis_eth_dma_tready         (e2h_dma_tready),
    .s_axis_eth_dma_tvalid         (e2h_dma_tvalid),
  `ifdef QSFP1_0
    .s_axi_hp1_aruser              (axi_hp1_aruser),
    .s_axi_hp1_awuser              (axi_hp1_awuser),
    .s_axi_hp1_awid                (axi_hp1_awid),
    .s_axi_hp1_awaddr              (axi_hp1_awaddr),
    .s_axi_hp1_awlen               (axi_hp1_awlen),
    .s_axi_hp1_awsize              (axi_hp1_awsize),
    .s_axi_hp1_awburst             (axi_hp1_awburst),
    .s_axi_hp1_awlock              (axi_hp1_awlock),
    .s_axi_hp1_awcache             (axi_hp1_awcache),
    .s_axi_hp1_awprot              (axi_hp1_awprot),
    .s_axi_hp1_awvalid             (axi_hp1_awvalid),
    .s_axi_hp1_awready             (axi_hp1_awready),
    .s_axi_hp1_wdata               (axi_hp1_wdata),
    .s_axi_hp1_wstrb               (axi_hp1_wstrb),
    .s_axi_hp1_wlast               (axi_hp1_wlast),
    .s_axi_hp1_wvalid              (axi_hp1_wvalid),
    .s_axi_hp1_wready              (axi_hp1_wready),
    .s_axi_hp1_bid                 (),
    .s_axi_hp1_bresp               (axi_hp1_bresp),
    .s_axi_hp1_bvalid              (axi_hp1_bvalid),
    .s_axi_hp1_bready              (axi_hp1_bready),
    .s_axi_hp1_arid                (axi_hp1_arid),
    .s_axi_hp1_araddr              (axi_hp1_araddr),
    .s_axi_hp1_arlen               (axi_hp1_arlen),
    .s_axi_hp1_arsize              (axi_hp1_arsize),
    .s_axi_hp1_arburst             (axi_hp1_arburst),
    .s_axi_hp1_arlock              (axi_hp1_arlock),
    .s_axi_hp1_arcache             (axi_hp1_arcache),
    .s_axi_hp1_arprot              (axi_hp1_arprot),
    .s_axi_hp1_arvalid             (axi_hp1_arvalid),
    .s_axi_hp1_arready             (axi_hp1_arready),
    .s_axi_hp1_rid                 (),
    .s_axi_hp1_rdata               (axi_hp1_rdata),
    .s_axi_hp1_rresp               (axi_hp1_rresp),
    .s_axi_hp1_rlast               (axi_hp1_rlast),
    .s_axi_hp1_rvalid              (axi_hp1_rvalid),
    .s_axi_hp1_rready              (axi_hp1_rready),
    .s_axi_hp1_awqos               (axi_hp1_awqos),
    .s_axi_hp1_arqos               (axi_hp1_arqos),
  `else
    // Disconnect the DMA interface if the associated QSFP port is not used.
    .s_axi_hp1_awvalid             (1'b0),
    .s_axi_hp1_wvalid              (1'b0),
    .s_axi_hp1_bready              (1'b1),
    .s_axi_hp1_arvalid             (1'b0),
    .s_axi_hp1_rready              (1'b1),
  `endif
    .s_axi_hpc0_aruser             (),
    .s_axi_hpc0_awuser             (),
    .s_axi_hpc0_awid               (),
    .s_axi_hpc0_awaddr             (),
    .s_axi_hpc0_awlen              (),
    .s_axi_hpc0_awsize             (),
    .s_axi_hpc0_awburst            (),
    .s_axi_hpc0_awlock             (),
    .s_axi_hpc0_awcache            (),
    .s_axi_hpc0_awprot             (),
    .s_axi_hpc0_awvalid            (),
    .s_axi_hpc0_awready            (),
    .s_axi_hpc0_wdata              (),
    .s_axi_hpc0_wstrb              (),
    .s_axi_hpc0_wlast              (),
    .s_axi_hpc0_wvalid             (),
    .s_axi_hpc0_wready             (),
    .s_axi_hpc0_bid                (),
    .s_axi_hpc0_bresp              (),
    .s_axi_hpc0_bvalid             (),
    .s_axi_hpc0_bready             (),
    .s_axi_hpc0_arid               (),
    .s_axi_hpc0_araddr             (),
    .s_axi_hpc0_arlen              (),
    .s_axi_hpc0_arsize             (),
    .s_axi_hpc0_arburst            (),
    .s_axi_hpc0_arlock             (),
    .s_axi_hpc0_arcache            (),
    .s_axi_hpc0_arprot             (),
    .s_axi_hpc0_arvalid            (),
    .s_axi_hpc0_arready            (),
    .s_axi_hpc0_rid                (),
    .s_axi_hpc0_rdata              (),
    .s_axi_hpc0_rresp              (),
    .s_axi_hpc0_rlast              (),
    .s_axi_hpc0_rvalid             (),
    .s_axi_hpc0_rready             (),
    .s_axi_hpc0_awqos              (),
    .s_axi_hpc0_arqos              (),
    .adc0_clk_clk_n                (ADC_CLK_N[0]),
    .adc0_clk_clk_p                (ADC_CLK_P[0]),
    .adc1_clk_clk_n                (ADC_CLK_N[0]),
    .adc1_clk_clk_p                (ADC_CLK_P[0]),
    .adc2_clk_clk_n                (ADC_CLK_N[2]),
    .adc2_clk_clk_p                (ADC_CLK_P[2]),
    .adc3_clk_clk_n                (ADC_CLK_N[2]),
    .adc3_clk_clk_p                (ADC_CLK_P[2]),
    .m_axi_app_awaddr              (m_axi_app_awaddr),
    .m_axi_app_awprot              (m_axi_app_awprot),
    .m_axi_app_awvalid             (m_axi_app_awvalid),
    .m_axi_app_awready             (m_axi_app_awready),
    .m_axi_app_wdata               (m_axi_app_wdata),
    .m_axi_app_wstrb               (m_axi_app_wstrb),
    .m_axi_app_wvalid              (m_axi_app_wvalid),
    .m_axi_app_wready              (m_axi_app_wready),
    .m_axi_app_bresp               (m_axi_app_bresp),
    .m_axi_app_bvalid              (m_axi_app_bvalid),
    .m_axi_app_bready              (m_axi_app_bready),
    .m_axi_app_araddr              (m_axi_app_araddr),
    .m_axi_app_arprot              (m_axi_app_arprot),
    .m_axi_app_arvalid             (m_axi_app_arvalid),
    .m_axi_app_arready             (m_axi_app_arready),
    .m_axi_app_rdata               (m_axi_app_rdata),
    .m_axi_app_rresp               (m_axi_app_rresp),
    .m_axi_app_rvalid              (m_axi_app_rvalid),
    .m_axi_app_rready              (m_axi_app_rready),
    .dac0_clk_clk_n                (DAC_CLK_N[0]),
    .dac0_clk_clk_p                (DAC_CLK_P[0]),
    .dac1_clk_clk_n                (DAC_CLK_N[1]),
    .dac1_clk_clk_p                (DAC_CLK_P[1]),
    .gpio_0_tri_i                  (gpio_0_tri_i),
    .gpio_0_tri_o                  (gpio_0_tri_o),
    .gpio_0_tri_t                  (gpio_0_tri_t),
    .m_axi_eth_internal_awaddr     (axi_eth_internal_awaddr),
    .m_axi_eth_internal_awprot     (),
    .m_axi_eth_internal_awvalid    (axi_eth_internal_awvalid),
    .m_axi_eth_internal_awready    (axi_eth_internal_awready),
    .m_axi_eth_internal_wdata      (axi_eth_internal_wdata),
    .m_axi_eth_internal_wstrb      (axi_eth_internal_wstrb),
    .m_axi_eth_internal_wvalid     (axi_eth_internal_wvalid),
    .m_axi_eth_internal_wready     (axi_eth_internal_wready),
    .m_axi_eth_internal_bresp      (axi_eth_internal_bresp),
    .m_axi_eth_internal_bvalid     (axi_eth_internal_bvalid),
    .m_axi_eth_internal_bready     (axi_eth_internal_bready),
    .m_axi_eth_internal_araddr     (axi_eth_internal_araddr),
    .m_axi_eth_internal_arprot     (),
    .m_axi_eth_internal_arvalid    (axi_eth_internal_arvalid),
    .m_axi_eth_internal_arready    (axi_eth_internal_arready),
    .m_axi_eth_internal_rdata      (axi_eth_internal_rdata),
    .m_axi_eth_internal_rresp      (axi_eth_internal_rresp),
    .m_axi_eth_internal_rvalid     (axi_eth_internal_rvalid),
    .m_axi_eth_internal_rready     (axi_eth_internal_rready),
    .m_axis_eth_dma_tdata          (h2e_dma_tdata),
    .m_axis_eth_dma_tkeep          (h2e_dma_tkeep),
    .m_axis_eth_dma_tlast          (h2e_dma_tlast),
    .m_axis_eth_dma_tready         (h2e_dma_tready),
    .m_axis_eth_dma_tvalid         (h2e_dma_tvalid),
    .m_axi_rpu_awaddr              (),
    .m_axi_rpu_awprot              (),
    .m_axi_rpu_awvalid             (),
    .m_axi_rpu_awready             (),
    .m_axi_rpu_wdata               (),
    .m_axi_rpu_wstrb               (),
    .m_axi_rpu_wvalid              (),
    .m_axi_rpu_wready              (),
    .m_axi_rpu_bresp               (),
    .m_axi_rpu_bvalid              (),
    .m_axi_rpu_bready              (),
    .m_axi_rpu_araddr              (),
    .m_axi_rpu_arprot              (),
    .m_axi_rpu_arvalid             (),
    .m_axi_rpu_arready             (),
    .m_axi_rpu_rdata               (),
    .m_axi_rpu_rresp               (),
    .m_axi_rpu_rvalid              (),
    .m_axi_rpu_rready              (),
    .m_axi_core_awaddr             (axi_core_awaddr),
    .m_axi_core_awprot             (),
    .m_axi_core_awvalid            (axi_core_awvalid),
    .m_axi_core_awready            (axi_core_awready),
    .m_axi_core_wdata              (axi_core_wdata),
    .m_axi_core_wstrb              (axi_core_wstrb),
    .m_axi_core_wvalid             (axi_core_wvalid),
    .m_axi_core_wready             (axi_core_wready),
    .m_axi_core_bresp              (axi_core_bresp),
    .m_axi_core_bvalid             (axi_core_bvalid),
    .m_axi_core_bready             (axi_core_bready),
    .m_axi_core_araddr             (axi_core_araddr),
    .m_axi_core_arprot             (),
    .m_axi_core_arvalid            (axi_core_arvalid),
    .m_axi_core_arready            (axi_core_arready),
    .m_axi_core_rdata              (axi_core_rdata),
    .m_axi_core_rresp              (axi_core_rresp),
    .m_axi_core_rvalid             (axi_core_rvalid),
    .m_axi_core_rready             (axi_core_rready),
    .m_axi_mpm_ep_awaddr           (m_axi_mpm_ep_awaddr),
    .m_axi_mpm_ep_awprot           (),
    .m_axi_mpm_ep_awvalid          (m_axi_mpm_ep_awvalid),
    .m_axi_mpm_ep_awready          (m_axi_mpm_ep_awready),
    .m_axi_mpm_ep_wdata            (m_axi_mpm_ep_wdata),
    .m_axi_mpm_ep_wstrb            (m_axi_mpm_ep_wstrb),
    .m_axi_mpm_ep_wvalid           (m_axi_mpm_ep_wvalid),
    .m_axi_mpm_ep_wready           (m_axi_mpm_ep_wready),
    .m_axi_mpm_ep_bresp            (m_axi_mpm_ep_bresp),
    .m_axi_mpm_ep_bvalid           (m_axi_mpm_ep_bvalid),
    .m_axi_mpm_ep_bready           (m_axi_mpm_ep_bready),
    .m_axi_mpm_ep_araddr           (m_axi_mpm_ep_araddr),
    .m_axi_mpm_ep_arprot           (),
    .m_axi_mpm_ep_arvalid          (m_axi_mpm_ep_arvalid),
    .m_axi_mpm_ep_arready          (m_axi_mpm_ep_arready),
    .m_axi_mpm_ep_rdata            (m_axi_mpm_ep_rdata),
    .m_axi_mpm_ep_rresp            (m_axi_mpm_ep_rresp),
    .m_axi_mpm_ep_rvalid           (m_axi_mpm_ep_rvalid),
    .m_axi_mpm_ep_rready           (m_axi_mpm_ep_rready),
    .adc_tile224_ch0_dout_i_tdata  (adc_tile_dout_i_tdata[0]),
    .adc_tile224_ch0_dout_i_tready (adc_tile_dout_i_tready[0]),
    .adc_tile224_ch0_dout_i_tvalid (adc_tile_dout_i_tvalid[0]),
    .adc_tile224_ch0_dout_q_tdata  (adc_tile_dout_q_tdata[0]),
    .adc_tile224_ch0_dout_q_tready (adc_tile_dout_q_tready[0]),
    .adc_tile224_ch0_dout_q_tvalid (adc_tile_dout_q_tvalid[0]),
    .adc_tile224_ch1_dout_i_tdata  (adc_tile_dout_i_tdata[1]),
    .adc_tile224_ch1_dout_i_tready (adc_tile_dout_i_tready[1]),
    .adc_tile224_ch1_dout_i_tvalid (adc_tile_dout_i_tvalid[1]),
    .adc_tile224_ch1_dout_q_tdata  (adc_tile_dout_q_tdata[1]),
    .adc_tile224_ch1_dout_q_tready (adc_tile_dout_q_tready[1]),
    .adc_tile224_ch1_dout_q_tvalid (adc_tile_dout_q_tvalid[1]),
    .adc_tile225_ch0_dout_i_tdata  (adc_tile_dout_i_tdata[2]),
    .adc_tile225_ch0_dout_i_tready (adc_tile_dout_i_tready[2]),
    .adc_tile225_ch0_dout_i_tvalid (adc_tile_dout_i_tvalid[2]),
    .adc_tile225_ch0_dout_q_tdata  (adc_tile_dout_q_tdata[2]),
    .adc_tile225_ch0_dout_q_tready (adc_tile_dout_q_tready[2]),
    .adc_tile225_ch0_dout_q_tvalid (adc_tile_dout_q_tvalid[2]),
    .adc_tile225_ch1_dout_i_tdata  (adc_tile_dout_i_tdata[3]),
    .adc_tile225_ch1_dout_i_tready (adc_tile_dout_i_tready[3]),
    .adc_tile225_ch1_dout_i_tvalid (adc_tile_dout_i_tvalid[3]),
    .adc_tile225_ch1_dout_q_tdata  (adc_tile_dout_q_tdata[3]),
    .adc_tile225_ch1_dout_q_tready (adc_tile_dout_q_tready[3]),
    .adc_tile225_ch1_dout_q_tvalid (adc_tile_dout_q_tvalid[3]),
    .adc_tile226_ch0_dout_i_tdata  (adc_tile_dout_i_tdata[4]),
    .adc_tile226_ch0_dout_i_tready (adc_tile_dout_i_tready[4]),
    .adc_tile226_ch0_dout_i_tvalid (adc_tile_dout_i_tvalid[4]),
    .adc_tile226_ch0_dout_q_tdata  (adc_tile_dout_q_tdata[4]),
    .adc_tile226_ch0_dout_q_tready (adc_tile_dout_q_tready[4]),
    .adc_tile226_ch0_dout_q_tvalid (adc_tile_dout_q_tvalid[4]),
    .adc_tile226_ch1_dout_i_tdata  (adc_tile_dout_i_tdata[5]),
    .adc_tile226_ch1_dout_i_tready (adc_tile_dout_i_tready[5]),
    .adc_tile226_ch1_dout_i_tvalid (adc_tile_dout_i_tvalid[5]),
    .adc_tile226_ch1_dout_q_tdata  (adc_tile_dout_q_tdata[5]),
    .adc_tile226_ch1_dout_q_tready (adc_tile_dout_q_tready[5]),
    .adc_tile226_ch1_dout_q_tvalid (adc_tile_dout_q_tvalid[5]),
    .adc_tile227_ch0_dout_i_tdata  (adc_tile_dout_i_tdata[6]),
    .adc_tile227_ch0_dout_i_tready (adc_tile_dout_i_tready[6]),
    .adc_tile227_ch0_dout_i_tvalid (adc_tile_dout_i_tvalid[6]),
    .adc_tile227_ch0_dout_q_tdata  (adc_tile_dout_q_tdata[6]),
    .adc_tile227_ch0_dout_q_tready (adc_tile_dout_q_tready[6]),
    .adc_tile227_ch0_dout_q_tvalid (adc_tile_dout_q_tvalid[6]),
    .adc_tile227_ch1_dout_i_tdata  (adc_tile_dout_i_tdata[7]),
    .adc_tile227_ch1_dout_i_tready (adc_tile_dout_i_tready[7]),
    .adc_tile227_ch1_dout_i_tvalid (adc_tile_dout_i_tvalid[7]),
    .adc_tile227_ch1_dout_q_tdata  (adc_tile_dout_q_tdata[7]),
    .adc_tile227_ch1_dout_q_tready (adc_tile_dout_q_tready[7]),
    .adc_tile227_ch1_dout_q_tvalid (adc_tile_dout_q_tvalid[7]),
    .dac_tile228_ch0_vout_v_n      (DB0_TX_N[0]),
    .dac_tile228_ch0_vout_v_p      (DB0_TX_P[0]),
    .dac_tile228_ch1_vout_v_n      (DB0_TX_N[1]),
    .dac_tile228_ch1_vout_v_p      (DB0_TX_P[1]),
    .dac_tile228_ch2_vout_v_n      (DB0_TX_N[2]),
    .dac_tile228_ch2_vout_v_p      (DB0_TX_P[2]),
    .dac_tile228_ch3_vout_v_n      (DB0_TX_N[3]),
    .dac_tile228_ch3_vout_v_p      (DB0_TX_P[3]),
    .dac_tile229_ch0_vout_v_n      (DB1_TX_N[0]),
    .dac_tile229_ch0_vout_v_p      (DB1_TX_P[0]),
    .dac_tile229_ch1_vout_v_n      (DB1_TX_N[1]),
    .dac_tile229_ch1_vout_v_p      (DB1_TX_P[1]),
    .dac_tile229_ch2_vout_v_n      (DB1_TX_N[2]),
    .dac_tile229_ch2_vout_v_p      (DB1_TX_P[2]),
    .dac_tile229_ch3_vout_v_n      (DB1_TX_N[3]),
    .dac_tile229_ch3_vout_v_p      (DB1_TX_P[3]),
    .dac_tile228_ch0_din_tdata     (dac_tile_din_tdata[0]),
    .dac_tile228_ch0_din_tvalid    (dac_tile_din_tvalid[0]),
    .dac_tile228_ch0_din_tready    (dac_tile_din_tready[0]),
    .dac_tile228_ch1_din_tdata     (dac_tile_din_tdata[1]),
    .dac_tile228_ch1_din_tvalid    (dac_tile_din_tvalid[1]),
    .dac_tile228_ch1_din_tready    (dac_tile_din_tready[1]),
    .dac_tile228_ch2_din_tdata     (dac_tile_din_tdata[2]),
    .dac_tile228_ch2_din_tvalid    (dac_tile_din_tvalid[2]),
    .dac_tile228_ch2_din_tready    (dac_tile_din_tready[2]),
    .dac_tile228_ch3_din_tdata     (dac_tile_din_tdata[3]),
    .dac_tile228_ch3_din_tvalid    (dac_tile_din_tvalid[3]),
    .dac_tile228_ch3_din_tready    (dac_tile_din_tready[3]),
    .dac_tile229_ch0_din_tdata     (dac_tile_din_tdata[4]),
    .dac_tile229_ch0_din_tvalid    (dac_tile_din_tvalid[4]),
    .dac_tile229_ch0_din_tready    (dac_tile_din_tready[4]),
    .dac_tile229_ch1_din_tdata     (dac_tile_din_tdata[5]),
    .dac_tile229_ch1_din_tvalid    (dac_tile_din_tvalid[5]),
    .dac_tile229_ch1_din_tready    (dac_tile_din_tready[5]),
    .dac_tile229_ch2_din_tdata     (dac_tile_din_tdata[6]),
    .dac_tile229_ch2_din_tvalid    (dac_tile_din_tvalid[6]),
    .dac_tile229_ch2_din_tready    (dac_tile_din_tready[6]),
    .dac_tile229_ch3_din_tdata     (dac_tile_din_tdata[7]),
    .dac_tile229_ch3_din_tvalid    (dac_tile_din_tvalid[7]),
    .dac_tile229_ch3_din_tready    (dac_tile_din_tready[7]),
    .s_axi_hpc1_awid               (),
    .s_axi_hpc1_awaddr             (),
    .s_axi_hpc1_awlen              (),
    .s_axi_hpc1_awsize             (),
    .s_axi_hpc1_awburst            (),
    .s_axi_hpc1_awlock             (),
    .s_axi_hpc1_awcache            (),
    .s_axi_hpc1_awprot             (),
    .s_axi_hpc1_awqos              (),
    .s_axi_hpc1_awvalid            (),
    .s_axi_hpc1_awready            (),
    .s_axi_hpc1_wdata              (),
    .s_axi_hpc1_wstrb              (),
    .s_axi_hpc1_wlast              (),
    .s_axi_hpc1_wvalid             (),
    .s_axi_hpc1_wready             (),
    .s_axi_hpc1_bid                (),
    .s_axi_hpc1_bresp              (),
    .s_axi_hpc1_bvalid             (),
    .s_axi_hpc1_bready             (),
    .s_axi_hpc1_arid               (),
    .s_axi_hpc1_araddr             (),
    .s_axi_hpc1_arlen              (),
    .s_axi_hpc1_arsize             (),
    .s_axi_hpc1_arburst            (),
    .s_axi_hpc1_arlock             (),
    .s_axi_hpc1_arcache            (),
    .s_axi_hpc1_arprot             (),
    .s_axi_hpc1_arqos              (),
    .s_axi_hpc1_arvalid            (),
    .s_axi_hpc1_arready            (),
    .s_axi_hpc1_rid                (),
    .s_axi_hpc1_rdata              (),
    .s_axi_hpc1_rresp              (),
    .s_axi_hpc1_rlast              (),
    .s_axi_hpc1_rvalid             (),
    .s_axi_hpc1_rready             (),
    .sysref_rf_in_diff_n           (SYSREF_RF_N),
    .sysref_rf_in_diff_p           (SYSREF_RF_P),
    .adc_tile224_ch0_vin_v_n       (DB0_RX_N[0]),
    .adc_tile224_ch0_vin_v_p       (DB0_RX_P[0]),
    .adc_tile224_ch1_vin_v_n       (DB0_RX_N[1]),
    .adc_tile224_ch1_vin_v_p       (DB0_RX_P[1]),
    .adc_tile225_ch0_vin_v_n       (DB0_RX_N[2]),
    .adc_tile225_ch0_vin_v_p       (DB0_RX_P[2]),
    .adc_tile225_ch1_vin_v_n       (DB0_RX_N[3]),
    .adc_tile225_ch1_vin_v_p       (DB0_RX_P[3]),
    .adc_tile226_ch0_vin_v_n       (DB1_RX_N[0]),
    .adc_tile226_ch0_vin_v_p       (DB1_RX_P[0]),
    .adc_tile226_ch1_vin_v_n       (DB1_RX_N[1]),
    .adc_tile226_ch1_vin_v_p       (DB1_RX_P[1]),
    .adc_tile227_ch0_vin_v_n       (DB1_RX_N[2]),
    .adc_tile227_ch0_vin_v_p       (DB1_RX_P[2]),
    .adc_tile227_ch1_vin_v_n       (DB1_RX_N[3]),
    .adc_tile227_ch1_vin_v_p       (DB1_RX_P[3]),
    .s_axi_hpc1_aruser             (),
    .s_axi_hpc1_awuser             (),
    .clk_adc0                      ()
    );
  `endif


  //---------------------------------------------------------------------------
  // AXI Interconnect
  //---------------------------------------------------------------------------

  wire [ 39:0] axi_qsfp0_araddr;
  wire [  0:0] axi_qsfp0_arready;
  wire [  0:0] axi_qsfp0_arvalid;
  wire [ 39:0] axi_qsfp0_awaddr;
  wire [  0:0] axi_qsfp0_awready;
  wire [  0:0] axi_qsfp0_awvalid;
  wire [  0:0] axi_qsfp0_bready;
  wire [  1:0] axi_qsfp0_bresp;
  wire [  0:0] axi_qsfp0_bvalid;
  wire [ 31:0] axi_qsfp0_rdata;
  wire [  0:0] axi_qsfp0_rready;
  wire [  1:0] axi_qsfp0_rresp;
  wire [  0:0] axi_qsfp0_rvalid;
  wire [ 31:0] axi_qsfp0_wdata;
  wire [  0:0] axi_qsfp0_wready;
  wire [  3:0] axi_qsfp0_wstrb;
  wire [  0:0] axi_qsfp0_wvalid;

  wire [ 39:0] axi_qsfp1_araddr;
  wire [  0:0] axi_qsfp1_arready;
  wire [  0:0] axi_qsfp1_arvalid;
  wire [ 39:0] axi_qsfp1_awaddr;
  wire [  0:0] axi_qsfp1_awready;
  wire [  0:0] axi_qsfp1_awvalid;
  wire [  0:0] axi_qsfp1_bready;
  wire [  1:0] axi_qsfp1_bresp;
  wire [  0:0] axi_qsfp1_bvalid;
  wire [ 31:0] axi_qsfp1_rdata;
  wire [  0:0] axi_qsfp1_rready;
  wire [  1:0] axi_qsfp1_rresp;
  wire [  0:0] axi_qsfp1_rvalid;
  wire [ 31:0] axi_qsfp1_wdata;
  wire [  0:0] axi_qsfp1_wready;
  wire [  3:0] axi_qsfp1_wstrb;
  wire [  0:0] axi_qsfp1_wvalid;

  axi_interconnect_app_bd axi_interconnect_app_bd_i (
    .clk40               (clk40),
    .clk40_rstn          (clk40_rstn),
    .m_axi_qsfp0_araddr  (axi_qsfp0_araddr),
    .m_axi_qsfp0_arprot  (),
    .m_axi_qsfp0_arready (axi_qsfp0_arready),
    .m_axi_qsfp0_arvalid (axi_qsfp0_arvalid),
    .m_axi_qsfp0_awaddr  (axi_qsfp0_awaddr),
    .m_axi_qsfp0_awprot  (),
    .m_axi_qsfp0_awready (axi_qsfp0_awready),
    .m_axi_qsfp0_awvalid (axi_qsfp0_awvalid),
    .m_axi_qsfp0_bready  (axi_qsfp0_bready),
    .m_axi_qsfp0_bresp   (axi_qsfp0_bresp),
    .m_axi_qsfp0_bvalid  (axi_qsfp0_bvalid),
    .m_axi_qsfp0_rdata   (axi_qsfp0_rdata),
    .m_axi_qsfp0_rready  (axi_qsfp0_rready),
    .m_axi_qsfp0_rresp   (axi_qsfp0_rresp),
    .m_axi_qsfp0_rvalid  (axi_qsfp0_rvalid),
    .m_axi_qsfp0_wdata   (axi_qsfp0_wdata),
    .m_axi_qsfp0_wready  (axi_qsfp0_wready),
    .m_axi_qsfp0_wstrb   (axi_qsfp0_wstrb),
    .m_axi_qsfp0_wvalid  (axi_qsfp0_wvalid),
    .m_axi_qsfp1_araddr  (axi_qsfp1_araddr),
    .m_axi_qsfp1_arprot  (),
    .m_axi_qsfp1_arready (axi_qsfp1_arready),
    .m_axi_qsfp1_arvalid (axi_qsfp1_arvalid),
    .m_axi_qsfp1_awaddr  (axi_qsfp1_awaddr),
    .m_axi_qsfp1_awprot  (),
    .m_axi_qsfp1_awready (axi_qsfp1_awready),
    .m_axi_qsfp1_awvalid (axi_qsfp1_awvalid),
    .m_axi_qsfp1_bready  (axi_qsfp1_bready),
    .m_axi_qsfp1_bresp   (axi_qsfp1_bresp),
    .m_axi_qsfp1_bvalid  (axi_qsfp1_bvalid),
    .m_axi_qsfp1_rdata   (axi_qsfp1_rdata),
    .m_axi_qsfp1_rready  (axi_qsfp1_rready),
    .m_axi_qsfp1_rresp   (axi_qsfp1_rresp),
    .m_axi_qsfp1_rvalid  (axi_qsfp1_rvalid),
    .m_axi_qsfp1_wdata   (axi_qsfp1_wdata),
    .m_axi_qsfp1_wready  (axi_qsfp1_wready),
    .m_axi_qsfp1_wstrb   (axi_qsfp1_wstrb),
    .m_axi_qsfp1_wvalid  (axi_qsfp1_wvalid),
    .s_axi_app_araddr    (m_axi_app_araddr),
    .s_axi_app_arprot    (m_axi_app_arprot),
    .s_axi_app_arready   (m_axi_app_arready),
    .s_axi_app_arvalid   (m_axi_app_arvalid),
    .s_axi_app_awaddr    (m_axi_app_awaddr),
    .s_axi_app_awprot    (m_axi_app_awprot),
    .s_axi_app_awready   (m_axi_app_awready),
    .s_axi_app_awvalid   (m_axi_app_awvalid),
    .s_axi_app_bready    (m_axi_app_bready),
    .s_axi_app_bresp     (m_axi_app_bresp),
    .s_axi_app_bvalid    (m_axi_app_bvalid),
    .s_axi_app_rdata     (m_axi_app_rdata),
    .s_axi_app_rready    (m_axi_app_rready),
    .s_axi_app_rresp     (m_axi_app_rresp),
    .s_axi_app_rvalid    (m_axi_app_rvalid),
    .s_axi_app_wdata     (m_axi_app_wdata),
    .s_axi_app_wready    (m_axi_app_wready),
    .s_axi_app_wstrb     (m_axi_app_wstrb),
    .s_axi_app_wvalid    (m_axi_app_wvalid)
  );


  //---------------------------------------------------------------------------
  // RF + Control Daughterboard Cores
  //---------------------------------------------------------------------------

  // Radio timestamp
  wire [ NUM_TIMEKEEPERS*64-1:0] radio_time;
  wire [    NUM_TIMEKEEPERS-1:0] radio_time_stb;

  // User data interfaces (data_clk domain)
  //
  // ADC (note no tready signal, ADC data can't be throttled)
  wire [RADIO_SPC*32-1:0]  adc_data_out_tdata  [0:NUM_CHANNELS-1]; // 32-bit samples (I + Q)
  wire [NUM_CHANNELS-1:0]  adc_data_out_tvalid;
  // DAC
  wire [RADIO_SPC*32-1:0]  dac_data_in_tdata   [0:NUM_CHANNELS-1]; // 32-bit samples (I + Q)
  wire [NUM_CHANNELS-1:0]  dac_data_in_tready;
  wire [NUM_CHANNELS-1:0]  dac_data_in_tvalid;

  // GPIO ctrlport interface
  wire        db_ctrlport_req_rd       [0:NUM_DBOARDS-1];
  wire        db_ctrlport_req_wr       [0:NUM_DBOARDS-1];
  wire [19:0] db_ctrlport_req_addr     [0:NUM_DBOARDS-1];
  wire [31:0] db_ctrlport_req_data     [0:NUM_DBOARDS-1];
  wire [ 3:0] db_ctrlport_req_byte_en  [0:NUM_DBOARDS-1];
  wire        db_ctrlport_req_has_time [0:NUM_DBOARDS-1];
  wire [63:0] db_ctrlport_req_time     [0:NUM_DBOARDS-1];
  wire        db_ctrlport_resp_ack     [0:NUM_DBOARDS-1];
  wire [31:0] db_ctrlport_resp_data    [0:NUM_DBOARDS-1];
  wire [ 1:0] db_ctrlport_resp_status  [0:NUM_DBOARDS-1];

  // GPIO to CPLD ctrlport interface
  wire        db_to_cpld_ctrlport_req_rd       [0:NUM_DBOARDS-1];
  wire        db_to_cpld_ctrlport_req_wr       [0:NUM_DBOARDS-1];
  wire [19:0] db_to_cpld_ctrlport_req_addr     [0:NUM_DBOARDS-1];
  wire [31:0] db_to_cpld_ctrlport_req_data     [0:NUM_DBOARDS-1];
  wire [ 3:0] db_to_cpld_ctrlport_req_byte_en  [0:NUM_DBOARDS-1];
  wire        db_to_cpld_ctrlport_req_has_time [0:NUM_DBOARDS-1];
  wire [63:0] db_to_cpld_ctrlport_req_time     [0:NUM_DBOARDS-1];
  wire        db_to_cpld_ctrlport_resp_ack     [0:NUM_DBOARDS-1];
  wire [31:0] db_to_cpld_ctrlport_resp_data    [0:NUM_DBOARDS-1];
  wire [ 1:0] db_to_cpld_ctrlport_resp_status  [0:NUM_DBOARDS-1];

  // GPIO to CPLD ctrlport interface combined
  wire        db_to_cpld_comb_ctrlport_req_rd;
  wire        db_to_cpld_comb_ctrlport_req_wr;
  wire [19:0] db_to_cpld_comb_ctrlport_req_addr;
  wire [31:0] db_to_cpld_comb_ctrlport_req_data;
  wire [ 3:0] db_to_cpld_comb_ctrlport_req_byte_en;
  wire        db_to_cpld_comb_ctrlport_req_has_time;
  wire [63:0] db_to_cpld_comb_ctrlport_req_time;
  wire        db_to_cpld_comb_ctrlport_resp_ack;
  wire [31:0] db_to_cpld_comb_ctrlport_resp_data;
  wire [ 1:0] db_to_cpld_comb_ctrlport_resp_status;

  // GPIO interface
  wire [19:0] db_gpio_in_int    [0:NUM_DBOARDS-1];
  wire [19:0] db_gpio_out_int   [0:NUM_DBOARDS-1];
  wire [19:0] db_gpio_out_en_int[0:NUM_DBOARDS-1];
  wire [19:0] db_gpio_out_ext   [0:NUM_DBOARDS-1];
  wire [19:0] db_gpio_out_en_ext[0:NUM_DBOARDS-1];

  // DB TX/RX states
  wire [ NUM_CHANNELS-1:0]    rx_running;
  wire [ NUM_CHANNELS-1:0]    tx_running;
  wire [ 2*NUM_CH_PER_DB-1:0] db_state [0:NUM_DBOARDS-1];

  genvar ch_i;
  genvar db_gpio_i; // used further down but declared here
  generate
    for (db_i = 0; db_i < NUM_DBOARDS; db_i = db_i + 1) begin : gen_db_state_db
      for ( ch_i = 0; ch_i < NUM_CH_PER_DB; ch_i = ch_i + 1) begin: gen_db_state_ch
        assign db_state[db_i][ch_i*2]   = rx_running[NUM_CH_PER_DB*db_i+ch_i];
        assign db_state[db_i][ch_i*2+1] = tx_running[NUM_CH_PER_DB*db_i+ch_i];
      end
    end
  endgenerate

  // Version info
  // These wires only convey constant data.
  wire [COMPONENT_VERSIONS_SIZE-1:0] rf_core_version     [0:NUM_DBOARDS-1];
  wire [COMPONENT_VERSIONS_SIZE-1:0] db_gpio_ifc_version [0:NUM_DBOARDS-1];

  assign rf_dsp_info_clk40[FABRIC_DSP_BW_MSB:FABRIC_DSP_BW] = RF_BANDWIDTH;
  generate
    for ( db_i = 0; db_i < NUM_DBOARDS; db_i = db_i + 1) begin : gen_rf_cores
      if (RF_CORE == "100M") begin : gen_rf_core_100m
        localparam ADC_AXIS_W = 32;
        localparam DAC_AXIS_W = 64;
        rf_core_100m rf_core_100m_i (
          .rfdc_clk                  (rfdc_clk[0]),
          .rfdc_clk_2x               (rfdc_clk_2x[0]),
          .data_clk                  (data_clk),
          .data_clk_2x               (data_clk_2x),
          .s_axi_config_clk          (clk40),
          .adc_data_in_i_tdata_0     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_0    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tvalid_0    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tdata_0     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_0    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tvalid_0    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tdata_1     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_1    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_i_tvalid_1    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tdata_1     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_1    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tvalid_1    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tdata_0      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+0][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_0     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tvalid_0     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tdata_1      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+1][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_1     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tvalid_1     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tdata_0      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tvalid_0     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tdata_1      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tvalid_1     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tdata_0       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tready_0      (dac_data_in_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tvalid_0      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tdata_1       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tready_1      (dac_data_in_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tvalid_1      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+1]),
          .invert_adc_iq_rclk2       (swapped_invert_adc_iq_rclk2[4*db_i+:4]),
          .invert_dac_iq_rclk2       (swapped_invert_dac_iq_rclk2[4*db_i+:4]),
          .dsp_info_sclk             (rf_dsp_info_clk40[10*db_i+:10]),
          .rfdc_info_sclk            (rf_rfdc_info_clk40[16*db_i+:16]),
          .axi_status_sclk           (rf_axi_status_clk40[16*db_i+:16]),
          .adc_data_out_resetn_dclk  (adc_data_out_resetn_dclk),
          .adc_enable_data_rclk      (adc_enable_data_rclk),
          .adc_rfdc_axi_resetn_rclk  (adc_rfdc_axi_resetn_rclk),
          .dac_data_in_resetn_dclk   (dac_data_in_resetn_dclk),
          .dac_data_in_resetn_rclk   (dac_data_in_resetn_rclk),
          .dac_data_in_resetn_rclk2x (dac_data_in_resetn_rclk2x),
          .fir_resetn_rclk2x         (fir_resetn_rclk2x),
          .version_info              (rf_core_version[db_i])
        );
      end else if (RF_CORE == "200M") begin : gen_rf_core_200m
        localparam ADC_AXIS_W = 128;
        localparam DAC_AXIS_W = 256;
        rf_core_200m rf_core_200m_i (
          .rfdc_clk                  (rfdc_clk[0]),
          .rfdc_clk_2x               (rfdc_clk_2x[0]),
          .data_clk                  (data_clk),
          .data_clk_2x               (data_clk_2x),
          .s_axi_config_clk          (clk40),
          .adc_data_in_i_tdata_0     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_0    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tvalid_0    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tdata_0     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_0    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tvalid_0    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tdata_1     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_1    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_i_tvalid_1    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tdata_1     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_1    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tvalid_1    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tdata_0      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+0][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_0     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tvalid_0     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tdata_1      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+1][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_1     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tvalid_1     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tdata_0      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tvalid_0     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tdata_1      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tvalid_1     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tdata_0       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tready_0      (dac_data_in_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tvalid_0      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tdata_1       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tready_1      (dac_data_in_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tvalid_1      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+1]),
          .invert_adc_iq_rclk2       (swapped_invert_adc_iq_rclk2[4*db_i+:4]),
          .invert_dac_iq_rclk2       (swapped_invert_dac_iq_rclk2[4*db_i+:4]),
          .dsp_info_sclk             (rf_dsp_info_clk40[10*db_i+:10]),
          .rfdc_info_sclk            (rf_rfdc_info_clk40[16*db_i+:16]),
          .axi_status_sclk           (rf_axi_status_clk40[16*db_i+:16]),
          .adc_data_out_resetn_dclk  (adc_data_out_resetn_dclk),
          .adc_enable_data_rclk      (adc_enable_data_rclk),
          .adc_rfdc_axi_resetn_rclk  (adc_rfdc_axi_resetn_rclk),
          .dac_data_in_resetn_dclk   (dac_data_in_resetn_dclk),
          .dac_data_in_resetn_dclk2x (dac_data_in_resetn_dclk2x),
          .dac_data_in_resetn_rclk   (dac_data_in_resetn_rclk),
          .fir_resetn_rclk2x         (fir_resetn_rclk2x),
          .version_info              (rf_core_version[db_i])
        );
      end else if (RF_CORE == "400M") begin : gen_rf_core_400m
        localparam ADC_AXIS_W = 128;
        localparam DAC_AXIS_W = 256;
        rf_core_400m rf_core_400m_i (
          .rfdc_clk                  (rfdc_clk[0]),
          .rfdc_clk_2x               (rfdc_clk_2x[0]),
          .data_clk                  (data_clk),
          .data_clk_2x               (data_clk_2x),
          .s_axi_config_clk          (clk40),
          .adc_data_in_i_tdata_0     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_0    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tvalid_0    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tdata_0     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_0    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tvalid_0    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tdata_1     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_1    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_i_tvalid_1    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tdata_1     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_1    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tvalid_1    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tdata_0      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+0][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_0     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tvalid_0     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tdata_1      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+1][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_1     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tvalid_1     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tdata_0      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tvalid_0     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tdata_1      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tvalid_1     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tdata_0       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tready_0      (dac_data_in_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tvalid_0      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tdata_1       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tready_1      (dac_data_in_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tvalid_1      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+1]),
          .invert_adc_iq_rclk2       (swapped_invert_adc_iq_rclk2[4*db_i+:4]),
          .invert_dac_iq_rclk2       (swapped_invert_dac_iq_rclk2[4*db_i+:4]),
          .dsp_info_sclk             (rf_dsp_info_clk40[10*db_i+:10]),
          .rfdc_info_sclk            (rf_rfdc_info_clk40[16*db_i+:16]),
          .axi_status_sclk           (rf_axi_status_clk40[16*db_i+:16]),
          .adc_data_out_resetn_dclk  (adc_data_out_resetn_dclk),
          .adc_enable_data_rclk      (adc_enable_data_rclk),
          .adc_rfdc_axi_resetn_rclk  (adc_rfdc_axi_resetn_rclk),
          .dac_data_in_resetn_dclk   (dac_data_in_resetn_dclk),
          .dac_data_in_resetn_dclk2x (dac_data_in_resetn_dclk2x),
          .dac_data_in_resetn_rclk   (dac_data_in_resetn_rclk),
          .fir_resetn_rclk2x         (fir_resetn_rclk2x),
          .version_info              (rf_core_version[db_i])
        );
      end else if (RF_CORE == "FULL") begin : gen_rf_core_full
        localparam ADC_AXIS_W = 128;
        localparam DAC_AXIS_W = 256;
        rf_core_full #(
          .NUM_ADC_CHANNELS(NUM_CH_PER_DB),
          .NUM_DAC_CHANNELS(NUM_CH_PER_DB)
        ) rf_core_full_i (
          .rfdc_clk                  (rfdc_clk[db_i]),
          .rfdc_clk_2x               (rfdc_clk_2x[db_i]),
          .s_axi_config_clk          (clk40),
          .adc_data_in_i_tdata_0     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_0    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tvalid_0    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tdata_0     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+0][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_0    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_q_tvalid_0    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_in_i_tdata_1     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_1    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_i_tvalid_1    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tdata_1     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+1][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_1    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_q_tvalid_1    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_in_i_tdata_2     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+2][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_2    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+2]),
          .adc_data_in_i_tvalid_2    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+2]),
          .adc_data_in_q_tdata_2     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+2][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_2    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+2]),
          .adc_data_in_q_tvalid_2    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+2]),
          .adc_data_in_i_tdata_3     (adc_tile_dout_i_tdata[NUM_CH_PER_DB*db_i+3][ADC_AXIS_W-1:0]),
          .adc_data_in_i_tready_3    (adc_tile_dout_i_tready[NUM_CH_PER_DB*db_i+3]),
          .adc_data_in_i_tvalid_3    (adc_tile_dout_i_tvalid[NUM_CH_PER_DB*db_i+3]),
          .adc_data_in_q_tdata_3     (adc_tile_dout_q_tdata[NUM_CH_PER_DB*db_i+3][ADC_AXIS_W-1:0]),
          .adc_data_in_q_tready_3    (adc_tile_dout_q_tready[NUM_CH_PER_DB*db_i+3]),
          .adc_data_in_q_tvalid_3    (adc_tile_dout_q_tvalid[NUM_CH_PER_DB*db_i+3]),
          .dac_data_out_tdata_0      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+0][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_0     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tvalid_0     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_out_tdata_1      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+1][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_1     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tvalid_1     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_out_tdata_2      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+2][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_2     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+2]),
          .dac_data_out_tvalid_2     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+2]),
          .dac_data_out_tdata_3      (dac_tile_din_tdata[NUM_CH_PER_DB*db_i+3][DAC_AXIS_W-1:0]),
          .dac_data_out_tready_3     (dac_tile_din_tready[NUM_CH_PER_DB*db_i+3]),
          .dac_data_out_tvalid_3     (dac_tile_din_tvalid[NUM_CH_PER_DB*db_i+3]),
          .adc_data_out_tdata_0      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tvalid_0     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+0]),
          .adc_data_out_tdata_1      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tvalid_1     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+1]),
          .adc_data_out_tdata_2      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+2]),
          .adc_data_out_tvalid_2     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+2]),
          .adc_data_out_tdata_3      (adc_data_out_tdata[NUM_CH_PER_DB*db_i+3]),
          .adc_data_out_tvalid_3     (adc_data_out_tvalid[NUM_CH_PER_DB*db_i+3]),
          .dac_data_in_tdata_0       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tready_0      (dac_data_in_tready[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tvalid_0      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+0]),
          .dac_data_in_tdata_1       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tready_1      (dac_data_in_tready[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tvalid_1      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+1]),
          .dac_data_in_tdata_2       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+2]),
          .dac_data_in_tready_2      (dac_data_in_tready[NUM_CH_PER_DB*db_i+2]),
          .dac_data_in_tvalid_2      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+2]),
          .dac_data_in_tdata_3       (dac_data_in_tdata[NUM_CH_PER_DB*db_i+3]),
          .dac_data_in_tready_3      (dac_data_in_tready[NUM_CH_PER_DB*db_i+3]),
          .dac_data_in_tvalid_3      (dac_data_in_tvalid[NUM_CH_PER_DB*db_i+3]),
          .invert_adc_iq_rclk2       (swapped_invert_adc_iq_rclk2[4*db_i +: 4]),
          .invert_dac_iq_rclk2       (swapped_invert_dac_iq_rclk2[4*db_i +: 4]),
          .dsp_info_sclk             (rf_dsp_info_clk40[10*db_i+:10]),
          .rfdc_info_sclk            (rf_rfdc_info_clk40[16*db_i+:16]),
          .axi_status_sclk           (rf_axi_status_clk40[16*db_i +: 16]),
          .adc_enable_data_rclk      (adc_enable_data_rclk[db_i]),
          .adc_rfdc_axi_resetn_rclk  (adc_rfdc_axi_resetn_rclk[db_i]),
          .version_info              (rf_core_version[db_i])
        );
      end // gen_rf_core_full
    end // gen_rf_cores

    `ifdef X440

      wire [NUM_DBOARDS-1:0] sync_clk;

      // Import LED register offsets in MB CPLD
      `include "cpld/regmap/x440/mb_cpld_pl_regmap_utils.vh"
      `include "regmap/pl_cpld_regmap_utils.vh"

      for ( db_i = 0; db_i < (NUM_DBOARDS); db_i = db_i + 1) begin : db_gpio_gen

        localparam LED_REGISTER_ADDRESS = (db_i == 0) ? MB_CPLD + PL_DB0_LED_REGISTERS
                                        : (db_i == 1) ? MB_CPLD + PL_DB1_LED_REGISTERS
                                        : 0;

        db_gpio_interface#(
          .LED_REGISTER_ADDRESS (LED_REGISTER_ADDRESS)
        ) db_gpio_interface_i (
          .radio_clk               (radio_clk[db_i]),
          .pll_ref_clk             (pll_ref_clk),
          .db_state                (db_state[db_i]),
          .in_sync_clk             (clk5),
          .out_sync_clk            (sync_clk[db_i]),
          .ctrlport_rst            (radio_rst[db_i]),
          .s_ctrlport_req_wr       (db_ctrlport_req_wr[db_i]),
          .s_ctrlport_req_rd       (db_ctrlport_req_rd[db_i]),
          .s_ctrlport_req_addr     (db_ctrlport_req_addr[db_i]),
          .s_ctrlport_req_data     (db_ctrlport_req_data[db_i]),
          .s_ctrlport_resp_ack     (db_ctrlport_resp_ack[db_i]),
          .s_ctrlport_resp_status  (db_ctrlport_resp_status[db_i]),
          .s_ctrlport_resp_data    (db_ctrlport_resp_data[db_i]),
          .m_ctrlport_req_wr       (db_to_cpld_ctrlport_req_wr[db_i]),
          .m_ctrlport_req_rd       (db_to_cpld_ctrlport_req_rd[db_i]),
          .m_ctrlport_req_addr     (db_to_cpld_ctrlport_req_addr[db_i]),
          .m_ctrlport_req_data     (db_to_cpld_ctrlport_req_data[db_i]),
          .m_ctrlport_req_byte_en  (db_to_cpld_ctrlport_req_byte_en[db_i]),
          .m_ctrlport_resp_ack     (db_to_cpld_ctrlport_resp_ack[db_i]),
          .m_ctrlport_resp_status  (db_to_cpld_ctrlport_resp_status[db_i]),
          .m_ctrlport_resp_data    (db_to_cpld_ctrlport_resp_data[db_i]),
          .gpio_in                 (db_gpio_in_int[db_i]),
          .gpio_out                (db_gpio_out_int[db_i]),
          .gpio_out_en             (db_gpio_out_en_int[db_i]),
          .version_info            (db_gpio_ifc_version[db_i])
        );
      end

      // The sync_clk flip-flop is driven in single-ended mode. The _n side is
      // set statically via a voltage divider with DB0_GPIO[12] and
      // DB1_GPIO[16] providing the source voltages.
      assign DB0_GPIO[12] = 1'b1;
      assign DB0_GPIO[13] = sync_clk[0];
      assign DB1_GPIO[16] = 1'b1;
      assign DB1_GPIO[17] = sync_clk[1];

      // GPIO to CPLD ctrlport interface for db1 and radio_clk[0]
      wire        db1_to_cpld_radio0_ctrlport_req_rd;
      wire        db1_to_cpld_radio0_ctrlport_req_wr;
      wire [19:0] db1_to_cpld_radio0_ctrlport_req_addr;
      wire [31:0] db1_to_cpld_radio0_ctrlport_req_data;
      wire [ 3:0] db1_to_cpld_radio0_ctrlport_req_byte_en;
      wire        db1_to_cpld_radio0_ctrlport_req_has_time;
      wire [63:0] db1_to_cpld_radio0_ctrlport_req_time;
      wire        db1_to_cpld_radio0_ctrlport_resp_ack;
      wire [31:0] db1_to_cpld_radio0_ctrlport_resp_data;
      wire [ 1:0] db1_to_cpld_radio0_ctrlport_resp_status;

      // Need to cross clocks from radio_clk[1] to radio_clk[0] to combine the
      // ctrlport buses.
      ctrlport_clk_cross ctrlport_clk_cross_i (
        .rst                       (radio_rst[1]),
        .s_ctrlport_clk            (radio_clk[1]),
        .s_ctrlport_req_wr         (db_to_cpld_ctrlport_req_wr[1]),
        .s_ctrlport_req_rd         (db_to_cpld_ctrlport_req_rd[1]),
        .s_ctrlport_req_addr       (db_to_cpld_ctrlport_req_addr[1]),
        .s_ctrlport_req_portid     (),
        .s_ctrlport_req_rem_epid   (),
        .s_ctrlport_req_rem_portid (),
        .s_ctrlport_req_data       (db_to_cpld_ctrlport_req_data[1]),
        .s_ctrlport_req_byte_en    (db_to_cpld_ctrlport_req_byte_en[1]),
        .s_ctrlport_req_has_time   (),
        .s_ctrlport_req_time       (),
        .s_ctrlport_resp_ack       (db_to_cpld_ctrlport_resp_ack[1]),
        .s_ctrlport_resp_status    (db_to_cpld_ctrlport_resp_status[1]),
        .s_ctrlport_resp_data      (db_to_cpld_ctrlport_resp_data[1]),
        .m_ctrlport_clk            (radio_clk[0]),
        .m_ctrlport_req_wr         (db1_to_cpld_radio0_ctrlport_req_wr),
        .m_ctrlport_req_rd         (db1_to_cpld_radio0_ctrlport_req_rd),
        .m_ctrlport_req_addr       (db1_to_cpld_radio0_ctrlport_req_addr),
        .m_ctrlport_req_portid     (),
        .m_ctrlport_req_rem_epid   (),
        .m_ctrlport_req_rem_portid (),
        .m_ctrlport_req_data       (db1_to_cpld_radio0_ctrlport_req_data),
        .m_ctrlport_req_byte_en    (db1_to_cpld_radio0_ctrlport_req_byte_en),
        .m_ctrlport_req_has_time   (),
        .m_ctrlport_req_time       (),
        .m_ctrlport_resp_ack       (db1_to_cpld_radio0_ctrlport_resp_ack),
        .m_ctrlport_resp_status    (db1_to_cpld_radio0_ctrlport_resp_status),
        .m_ctrlport_resp_data      (db1_to_cpld_radio0_ctrlport_resp_data)
      );

      // after both ctrlport buses are on radio_clk[0], combine them to go
      // into cpld_interface
      ctrlport_combiner #(
        .NUM_MASTERS (NUM_DBOARDS),
        .PRIORITY    (0)
      ) ctrlport_combiner_db_i (
        .ctrlport_clk              (radio_clk[0]),
        .ctrlport_rst              (radio_rst[0]),
        .s_ctrlport_req_wr         ({db1_to_cpld_radio0_ctrlport_req_wr,db_to_cpld_ctrlport_req_wr[0]}),
        .s_ctrlport_req_rd         ({db1_to_cpld_radio0_ctrlport_req_rd,db_to_cpld_ctrlport_req_rd[0]}),
        .s_ctrlport_req_addr       ({db1_to_cpld_radio0_ctrlport_req_addr,db_to_cpld_ctrlport_req_addr[0]}),
        .s_ctrlport_req_portid     (),
        .s_ctrlport_req_rem_epid   (),
        .s_ctrlport_req_rem_portid (),
        .s_ctrlport_req_data       ({db1_to_cpld_radio0_ctrlport_req_data,db_to_cpld_ctrlport_req_data[0]}),
        .s_ctrlport_req_byte_en    ({db1_to_cpld_radio0_ctrlport_req_byte_en,db_to_cpld_ctrlport_req_byte_en[0]}),
        .s_ctrlport_req_has_time   (),
        .s_ctrlport_req_time       (),
        .s_ctrlport_resp_ack       ({db1_to_cpld_radio0_ctrlport_resp_ack,db_to_cpld_ctrlport_resp_ack[0]}),
        .s_ctrlport_resp_status    ({db1_to_cpld_radio0_ctrlport_resp_status,db_to_cpld_ctrlport_resp_status[0]}),
        .s_ctrlport_resp_data      ({db1_to_cpld_radio0_ctrlport_resp_data,db_to_cpld_ctrlport_resp_data[0]}),
        .m_ctrlport_req_wr         (db_to_cpld_comb_ctrlport_req_wr),
        .m_ctrlport_req_rd         (db_to_cpld_comb_ctrlport_req_rd),
        .m_ctrlport_req_addr       (db_to_cpld_comb_ctrlport_req_addr),
        .m_ctrlport_req_portid     (),
        .m_ctrlport_req_rem_epid   (),
        .m_ctrlport_req_rem_portid (),
        .m_ctrlport_req_data       (db_to_cpld_comb_ctrlport_req_data),
        .m_ctrlport_req_byte_en    (db_to_cpld_comb_ctrlport_req_byte_en),
        .m_ctrlport_req_has_time   (db_to_cpld_comb_ctrlport_req_has_time),
        .m_ctrlport_req_time       (db_to_cpld_comb_ctrlport_req_time),
        .m_ctrlport_resp_ack       (db_to_cpld_comb_ctrlport_resp_ack),
        .m_ctrlport_resp_status    (db_to_cpld_comb_ctrlport_resp_status),
        .m_ctrlport_resp_data      (db_to_cpld_comb_ctrlport_resp_data)
      );

      // on X440, some gpio are used to drive sync clk, and not tri-state.
      for (db_gpio_i = 0; db_gpio_i < 20; db_gpio_i = db_gpio_i + 1) begin: db_gpio_tristate_gen
        if (db_gpio_i != 12 || db_gpio_i != 13) begin
          assign DB0_GPIO[db_gpio_i] = (db_gpio_out_en_ext[0][db_gpio_i]) ? db_gpio_out_ext[0][db_gpio_i] : 1'bz;
        end
        if (db_gpio_i != 16 || db_gpio_i != 17) begin
          assign DB1_GPIO[db_gpio_i] = (db_gpio_out_en_ext[1][db_gpio_i]) ? db_gpio_out_ext[1][db_gpio_i] : 1'bz;
        end
      end

    `else // X410

      for ( db_i = 0; db_i < (NUM_DBOARDS); db_i = db_i + 1) begin : db_gpio_gen
        db_gpio_interface db_gpio_interface_i (
          .radio_clk               (radio_clk[db_i]),
          .pll_ref_clk             (pll_ref_clk),
          .db_state                (db_state[db_i]),
          .ctrlport_rst            (radio_rst[db_i]),
          .s_ctrlport_req_wr       (db_ctrlport_req_wr[db_i]),
          .s_ctrlport_req_rd       (db_ctrlport_req_rd[db_i]),
          .s_ctrlport_req_addr     (db_ctrlport_req_addr[db_i]),
          .s_ctrlport_req_data     (db_ctrlport_req_data[db_i]),
          .s_ctrlport_resp_ack     (db_ctrlport_resp_ack[db_i]),
          .s_ctrlport_resp_status  (db_ctrlport_resp_status[db_i]),
          .s_ctrlport_resp_data    (db_ctrlport_resp_data[db_i]),
          .gpio_in                 (db_gpio_in_int[db_i]),
          .gpio_out                (db_gpio_out_int[db_i]),
          .gpio_out_en             (db_gpio_out_en_int[db_i]),
          .version_info            (db_gpio_ifc_version[db_i])
        );

        assign db_to_cpld_comb_ctrlport_req_rd = 1'b0;
        assign db_to_cpld_comb_ctrlport_req_wr = 1'b0;
        assign db_to_cpld_comb_ctrlport_req_addr = 20'b0;
        assign db_to_cpld_comb_ctrlport_req_data = 32'b0;
        assign db_to_cpld_comb_ctrlport_req_byte_en = 4'b0;

      end

      for (db_gpio_i = 0; db_gpio_i < 20; db_gpio_i = db_gpio_i + 1) begin: db_gpio_tristate_gen
        assign DB0_GPIO[db_gpio_i] = (db_gpio_out_en_ext[0][db_gpio_i]) ? db_gpio_out_ext[0][db_gpio_i] : 1'bz;
        assign DB1_GPIO[db_gpio_i] = (db_gpio_out_en_ext[1][db_gpio_i]) ? db_gpio_out_ext[1][db_gpio_i] : 1'bz;
      end

   `endif

    db_gpio_reordering db_gpio_reordering_i (
      .db0_gpio_in_int     (db_gpio_in_int[0]),
      .db0_gpio_out_int    (db_gpio_out_int[0]),
      .db0_gpio_out_en_int (db_gpio_out_en_int[0]),
      .db1_gpio_in_int     (db_gpio_in_int[1]),
      .db1_gpio_out_int    (db_gpio_out_int[1]),
      .db1_gpio_out_en_int (db_gpio_out_en_int[1]),
      .db0_gpio_in_ext     (DB0_GPIO),
      .db0_gpio_out_ext    (db_gpio_out_ext[0]),
      .db0_gpio_out_en_ext (db_gpio_out_en_ext[0]),
      .db1_gpio_in_ext     (DB1_GPIO),
      .db1_gpio_out_ext    (db_gpio_out_ext[1]),
      .db1_gpio_out_en_ext (db_gpio_out_en_ext[1])
    );

  endgenerate

  //---------------------------------------------------------------------------
  // QSFP Interfaces
  //---------------------------------------------------------------------------

  // Misc QSFP signals are currently unused
  assign QSFP0_RESET_n  = 1'b1;   // Module reset
  assign QSFP0_LPMODE_n = 1'b0;   // Low-power Mode
  assign QSFP1_RESET_n  = 1'b1;   // Module reset
  assign QSFP1_LPMODE_n = 1'b0;   // Low-power Mode

  wire [31:0] qsfp_port_0_0_info;
  wire [31:0] qsfp_port_0_1_info;
  wire [31:0] qsfp_port_0_2_info;
  wire [31:0] qsfp_port_0_3_info;
  wire [31:0] qsfp_port_1_0_info;
  wire [31:0] qsfp_port_1_1_info;
  wire [31:0] qsfp_port_1_2_info;
  wire [31:0] qsfp_port_1_3_info;

  wire [3:0] qsfp0_tx_p;
  wire [3:0] qsfp0_tx_n;
  wire [3:0] qsfp0_rx_p;
  wire [3:0] qsfp0_rx_n;

  wire [3:0] qsfp1_tx_p;
  wire [3:0] qsfp1_tx_n;
  wire [3:0] qsfp1_rx_p;
  wire [3:0] qsfp1_rx_n;


  wire [15:0] device_id;
  wire        rx_rec_clk_out1; // output GTY on QSFP1

  // e2v and v2e are flattened arrays, where e2v_tdata[ENET_W*N +: ENET_W] is
  // the data for RFNoC port N. RFNoC ports 0-3 map to QSFP0 and ports 4-7 map
  // to QSFP1. Note that each port might use less than ENET_W bits.
  wire [ENET_W*8-1:0] e2v_tdata;
  wire [       8-1:0] e2v_tlast;
  wire [       8-1:0] e2v_tready;
  wire [       8-1:0] e2v_tvalid;

  wire [ENET_W*8-1:0] v2e_tdata;
  wire [       8-1:0] v2e_tlast;
  wire [       8-1:0] v2e_tready;
  wire [       8-1:0] v2e_tvalid;

  `ifdef QSFP0_0
    assign QSFP0_0_TX_P  = qsfp0_tx_p[0];
    assign QSFP0_0_TX_N  = qsfp0_tx_n[0];
    assign qsfp0_rx_p[0] = QSFP0_0_RX_P;
    assign qsfp0_rx_n[0] = QSFP0_0_RX_N;
  `else
    assign qsfp0_rx_p[0] = 1'b0;
    assign qsfp0_rx_n[0] = 1'b1;
  `endif
  `ifdef QSFP0_1
    assign QSFP0_1_TX_P  = qsfp0_tx_p[1];
    assign QSFP0_1_TX_N  = qsfp0_tx_n[1];
    assign qsfp0_rx_p[1] = QSFP0_1_RX_P;
    assign qsfp0_rx_n[1] = QSFP0_1_RX_N;
  `else
    assign qsfp0_rx_p[1] = 1'b0;
    assign qsfp0_rx_n[1] = 1'b1;
  `endif
  `ifdef QSFP0_2
    assign QSFP0_2_TX_P  = qsfp0_tx_p[2];
    assign QSFP0_2_TX_N  = qsfp0_tx_n[2];
    assign qsfp0_rx_p[2] = QSFP0_2_RX_P;
    assign qsfp0_rx_n[2] = QSFP0_2_RX_N;
  `else
    assign qsfp0_rx_p[2] = 1'b0;
    assign qsfp0_rx_n[2] = 1'b1;
  `endif
  `ifdef QSFP0_3
    assign QSFP0_3_TX_P  = qsfp0_tx_p[3];
    assign QSFP0_3_TX_N  = qsfp0_tx_n[3];
    assign qsfp0_rx_p[3] = QSFP0_3_RX_P;
    assign qsfp0_rx_n[3] = QSFP0_3_RX_N;
  `else
    assign qsfp0_rx_p[3] = 1'b0;
    assign qsfp0_rx_n[3] = 1'b1;
  `endif

  `ifdef QSFP1_0
    assign QSFP1_0_TX_P  = qsfp1_tx_p[0];
    assign QSFP1_0_TX_N  = qsfp1_tx_n[0];
    assign qsfp1_rx_p[0] = QSFP1_0_RX_P;
    assign qsfp1_rx_n[0] = QSFP1_0_RX_N;
  `else
    assign qsfp1_rx_p[0] = 1'b0;
    assign qsfp1_rx_n[0] = 1'b1;
  `endif
  `ifdef QSFP1_1
    assign QSFP1_1_TX_P  = qsfp1_tx_p[1];
    assign QSFP1_1_TX_N  = qsfp1_tx_n[1];
    assign qsfp1_rx_p[1] = QSFP1_1_RX_P;
    assign qsfp1_rx_n[1] = QSFP1_1_RX_N;
  `else
    assign qsfp1_rx_p[1] = 1'b0;
    assign qsfp1_rx_n[1] = 1'b1;
  `endif
  `ifdef QSFP1_2
    assign QSFP1_2_TX_P  = qsfp1_tx_p[2];
    assign QSFP1_2_TX_N  = qsfp1_tx_n[2];
    assign qsfp1_rx_p[2] = QSFP1_2_RX_P;
    assign qsfp1_rx_n[2] = QSFP1_2_RX_N;
  `else
    assign qsfp1_rx_p[2] = 1'b0;
    assign qsfp1_rx_n[2] = 1'b1;
  `endif
  `ifdef QSFP1_3
    assign QSFP1_3_TX_P  = qsfp1_tx_p[3];
    assign QSFP1_3_TX_N  = qsfp1_tx_n[3];
    assign qsfp1_rx_p[3] = QSFP1_3_RX_P;
    assign qsfp1_rx_n[3] = QSFP1_3_RX_N;
  `else
    assign qsfp1_rx_p[3] = 1'b0;
    assign qsfp1_rx_n[3] = 1'b1;
  `endif

  `ifdef QSFP0_0
    x4xx_qsfp_wrapper_temp #(
      `ifdef QSFP0_0
      .PROTOCOL0      (`QSFP0_0),
      `endif
      `ifdef QSFP0_1
      .PROTOCOL1      (`QSFP0_1),
      `endif
      `ifdef QSFP0_2
      .PROTOCOL2      (`QSFP0_2),
      `endif
      `ifdef QSFP0_3
      .PROTOCOL3      (`QSFP0_3),
      `endif
      .CHDR_W         (QSFP0_W),
      .NET_CHDR_W     (NET_CHDR_W),
      .BYTE_MTU       (BYTE_MTU),
      .PORTNUM        (0),
      .NODE_INST      (0),
      .RFNOC_PROTOVER (RFNOC_PROTOVER)
    ) x4xx_qsfp_wrapper_0 (
      .areset         (areset),
      .refclk_p       (MGT_REFCLK_LMK0_P),
      .refclk_n       (MGT_REFCLK_LMK0_N),
      .clk100         (clk100),                // IP configured for 100 MHz DClk
      .bus_rst        (bus_clk_rst),
      .bus_clk        (bus_clk),
      .clk40_rst      (clk40_rst),
      .clk40          (clk40),
      // Register Access
      .s_axi_awaddr   (axi_qsfp0_awaddr),
      .s_axi_awvalid  (axi_qsfp0_awvalid),
      .s_axi_awready  (axi_qsfp0_awready),
      .s_axi_wdata    (axi_qsfp0_wdata),
      .s_axi_wstrb    (axi_qsfp0_wstrb),
      .s_axi_wvalid   (axi_qsfp0_wvalid),
      .s_axi_wready   (axi_qsfp0_wready),
      .s_axi_bresp    (axi_qsfp0_bresp),
      .s_axi_bvalid   (axi_qsfp0_bvalid),
      .s_axi_bready   (axi_qsfp0_bready),
      .s_axi_araddr   (axi_qsfp0_araddr),
      .s_axi_arvalid  (axi_qsfp0_arvalid),
      .s_axi_arready  (axi_qsfp0_arready),
      .s_axi_rdata    (axi_qsfp0_rdata),
      .s_axi_rresp    (axi_qsfp0_rresp),
      .s_axi_rvalid   (axi_qsfp0_rvalid),
      .s_axi_rready   (axi_qsfp0_rready),
      // DMA Access
      .axi_hp_araddr  (axi_hp0_araddr),
      .axi_hp_arburst (axi_hp0_arburst),
      .axi_hp_arcache (axi_hp0_arcache),
      .axi_hp_arlen   (axi_hp0_arlen),
      .axi_hp_arlock  (axi_hp0_arlock),
      .axi_hp_arprot  (axi_hp0_arprot),
      .axi_hp_arqos   (axi_hp0_arqos),
      .axi_hp_arready (axi_hp0_arready),
      .axi_hp_arsize  (axi_hp0_arsize),
      .axi_hp_arvalid (axi_hp0_arvalid),
      .axi_hp_awaddr  (axi_hp0_awaddr),
      .axi_hp_awburst (axi_hp0_awburst),
      .axi_hp_awcache (axi_hp0_awcache),
      .axi_hp_awlen   (axi_hp0_awlen),
      .axi_hp_awlock  (axi_hp0_awlock),
      .axi_hp_awprot  (axi_hp0_awprot),
      .axi_hp_awqos   (axi_hp0_awqos),
      .axi_hp_awready (axi_hp0_awready),
      .axi_hp_awsize  (axi_hp0_awsize),
      .axi_hp_awvalid (axi_hp0_awvalid),
      .axi_hp_bready  (axi_hp0_bready),
      .axi_hp_bresp   (axi_hp0_bresp),
      .axi_hp_bvalid  (axi_hp0_bvalid),
      .axi_hp_rdata   (axi_hp0_rdata),
      .axi_hp_rlast   (axi_hp0_rlast),
      .axi_hp_rready  (axi_hp0_rready),
      .axi_hp_rresp   (axi_hp0_rresp),
      .axi_hp_rvalid  (axi_hp0_rvalid),
      .axi_hp_wdata   (axi_hp0_wdata),
      .axi_hp_wlast   (axi_hp0_wlast),
      .axi_hp_wready  (axi_hp0_wready),
      .axi_hp_wstrb   (axi_hp0_wstrb),
      .axi_hp_wvalid  (axi_hp0_wvalid),
      // Transceivers
      .tx_p           (qsfp0_tx_p),
      .tx_n           (qsfp0_tx_n),
      .rx_p           (qsfp0_rx_p),
      .rx_n           (qsfp0_rx_n),
      // Ethernet to CHDR
      .e2v_tdata      ({ e2v_tdata [3*ENET_W +: QSFP0_W],
                         e2v_tdata [2*ENET_W +: QSFP0_W],
                         e2v_tdata [1*ENET_W +: QSFP0_W],
                         e2v_tdata [0*ENET_W +: QSFP0_W] }),
      .e2v_tlast      (e2v_tlast [0 +: 4]),
      .e2v_tvalid     (e2v_tvalid[0 +: 4]),
      .e2v_tready     (e2v_tready[0 +: 4]),
      // CHDR to Ethernet
      .v2e_tdata      ({ v2e_tdata[3*ENET_W +: QSFP0_W],
                         v2e_tdata[2*ENET_W +: QSFP0_W],
                         v2e_tdata[1*ENET_W +: QSFP0_W],
                         v2e_tdata[0*ENET_W +: QSFP0_W] }),
      .v2e_tlast      (v2e_tlast [0 +: 4]),
      .v2e_tvalid     (v2e_tvalid[0 +: 4]),
      .v2e_tready     (v2e_tready[0 +: 4]),
      // Misc
      .eth_rx_irq     (eth0_rx_irq),
      .eth_tx_irq     (eth0_tx_irq),
      .device_id      (device_id),
      .rx_rec_clk_out (),
      .port_info_0    (qsfp_port_0_0_info),
      .port_info_1    (qsfp_port_0_1_info),
      .port_info_2    (qsfp_port_0_2_info),
      .port_info_3    (qsfp_port_0_3_info),
      .link_up        (eth0_link_up),
      .activity       (eth0_activity)
    );
  `endif


  `ifdef QSFP1_0
    x4xx_qsfp_wrapper_temp #(
      `ifdef QSFP1_0
      .PROTOCOL0      (`QSFP1_0),
      `endif
      `ifdef QSFP1_1
      .PROTOCOL1      (`QSFP1_1),
      `endif
      `ifdef QSFP1_2
      .PROTOCOL2      (`QSFP1_2),
      `endif
      `ifdef QSFP1_3
      .PROTOCOL3      (`QSFP1_3),
      `endif
      .CHDR_W         (QSFP1_W),
      .NET_CHDR_W     (NET_CHDR_W),
      .BYTE_MTU       (BYTE_MTU),
      .PORTNUM        (1),
      .NODE_INST      (4),
      .RFNOC_PROTOVER (RFNOC_PROTOVER)
    ) x4xx_qsfp_wrapper_1 (
      .areset         (areset),
      .refclk_p       (MGT_REFCLK_LMK3_P),
      .refclk_n       (MGT_REFCLK_LMK3_N),
      .clk100         (clk100),                // IP configured for 100 MHz DClk
      .bus_rst        (bus_clk_rst),
      .bus_clk        (bus_clk),
      .clk40_rst      (clk40_rst),
      .clk40          (clk40),
      //Register Access
      .s_axi_awaddr   (axi_qsfp1_awaddr),
      .s_axi_awvalid  (axi_qsfp1_awvalid),
      .s_axi_awready  (axi_qsfp1_awready),
      .s_axi_wdata    (axi_qsfp1_wdata),
      .s_axi_wstrb    (axi_qsfp1_wstrb),
      .s_axi_wvalid   (axi_qsfp1_wvalid),
      .s_axi_wready   (axi_qsfp1_wready),
      .s_axi_bresp    (axi_qsfp1_bresp),
      .s_axi_bvalid   (axi_qsfp1_bvalid),
      .s_axi_bready   (axi_qsfp1_bready),
      .s_axi_araddr   (axi_qsfp1_araddr),
      .s_axi_arvalid  (axi_qsfp1_arvalid),
      .s_axi_arready  (axi_qsfp1_arready),
      .s_axi_rdata    (axi_qsfp1_rdata),
      .s_axi_rresp    (axi_qsfp1_rresp),
      .s_axi_rvalid   (axi_qsfp1_rvalid),
      .s_axi_rready   (axi_qsfp1_rready),
      // DMA Access
      .axi_hp_araddr  (axi_hp1_araddr),
      .axi_hp_arburst (axi_hp1_arburst),
      .axi_hp_arcache (axi_hp1_arcache),
      .axi_hp_arlen   (axi_hp1_arlen),
      .axi_hp_arlock  (axi_hp1_arlock),
      .axi_hp_arprot  (axi_hp1_arprot),
      .axi_hp_arqos   (axi_hp1_arqos),
      .axi_hp_arready (axi_hp1_arready),
      .axi_hp_arsize  (axi_hp1_arsize),
      .axi_hp_arvalid (axi_hp1_arvalid),
      .axi_hp_awaddr  (axi_hp1_awaddr),
      .axi_hp_awburst (axi_hp1_awburst),
      .axi_hp_awcache (axi_hp1_awcache),
      .axi_hp_awlen   (axi_hp1_awlen),
      .axi_hp_awlock  (axi_hp1_awlock),
      .axi_hp_awprot  (axi_hp1_awprot),
      .axi_hp_awqos   (axi_hp1_awqos),
      .axi_hp_awready (axi_hp1_awready),
      .axi_hp_awsize  (axi_hp1_awsize),
      .axi_hp_awvalid (axi_hp1_awvalid),
      .axi_hp_bready  (axi_hp1_bready),
      .axi_hp_bresp   (axi_hp1_bresp),
      .axi_hp_bvalid  (axi_hp1_bvalid),
      .axi_hp_rdata   (axi_hp1_rdata),
      .axi_hp_rlast   (axi_hp1_rlast),
      .axi_hp_rready  (axi_hp1_rready),
      .axi_hp_rresp   (axi_hp1_rresp),
      .axi_hp_rvalid  (axi_hp1_rvalid),
      .axi_hp_wdata   (axi_hp1_wdata),
      .axi_hp_wlast   (axi_hp1_wlast),
      .axi_hp_wready  (axi_hp1_wready),
      .axi_hp_wstrb   (axi_hp1_wstrb),
      .axi_hp_wvalid  (axi_hp1_wvalid),
      // Transceivers
      .tx_p           (qsfp1_tx_p),
      .tx_n           (qsfp1_tx_n),
      .rx_p           (qsfp1_rx_p),
      .rx_n           (qsfp1_rx_n),
      // Ethernet to CHDR
      .e2v_tdata      ({ e2v_tdata[4*ENET_W + 3*ENET_W +: QSFP1_W],
                         e2v_tdata[4*ENET_W + 2*ENET_W +: QSFP1_W],
                         e2v_tdata[4*ENET_W + 1*ENET_W +: QSFP1_W],
                         e2v_tdata[4*ENET_W + 0*ENET_W +: QSFP1_W] }),
      .e2v_tlast      (e2v_tlast [4 +: 4]),
      .e2v_tvalid     (e2v_tvalid[4 +: 4]),
      .e2v_tready     (e2v_tready[4 +: 4]),
      // CHDR to Ethernet
      .v2e_tdata      ({ v2e_tdata[4*ENET_W + 3*ENET_W +: QSFP1_W],
                         v2e_tdata[4*ENET_W + 2*ENET_W +: QSFP1_W],
                         v2e_tdata[4*ENET_W + 1*ENET_W +: QSFP1_W],
                         v2e_tdata[4*ENET_W + 0*ENET_W +: QSFP1_W] }),
      .v2e_tlast      (v2e_tlast [4 +: 4]),
      .v2e_tvalid     (v2e_tvalid[4 +: 4]),
      .v2e_tready     (v2e_tready[4 +: 4]),
      // Misc
      .eth_rx_irq     (eth1_rx_irq),
      .eth_tx_irq     (eth1_tx_irq),
      .device_id      (device_id),
      .rx_rec_clk_out (rx_rec_clk_out1),
      .port_info_0    (qsfp_port_1_0_info),
      .port_info_1    (qsfp_port_1_1_info),
      .port_info_2    (qsfp_port_1_2_info),
      .port_info_3    (qsfp_port_1_3_info),
      .link_up        (eth1_link_up),
      .activity       (eth1_activity)
    );
  `endif


  //---------------------------------------------------------------------------
  // Internal Ethernet Interface
  //---------------------------------------------------------------------------

  // CHDR DMA bus (bus_clk domain)
  wire [DMA_W-1:0] e2v_dma_tdata;
  wire             e2v_dma_tlast;
  wire             e2v_dma_tready;
  wire             e2v_dma_tvalid;
  wire [DMA_W-1:0] v2e_dma_tdata;
  wire             v2e_dma_tlast;
  wire             v2e_dma_tready;
  wire             v2e_dma_tvalid;

  eth_ipv4_internal #(
    .CHDR_W         (DMA_W),
    .NET_CHDR_W     (NET_CHDR_W),
    .BYTE_MTU       (BYTE_MTU),
    .DWIDTH         (REG_DWIDTH),
    .AWIDTH         (REG_AWIDTH),
    .PORTNUM        (8'd0),
    .RFNOC_PROTOVER (RFNOC_PROTOVER),
    .NODE_INST      (5)
  ) eth_ipv4_internal_i (
    .bus_clk       (bus_clk),
    .bus_rst       (bus_clk_rst),
    .s_axi_aclk    (clk40),
    .s_axi_aresetn (clk40_rstn),
    .s_axi_awaddr  (axi_eth_internal_awaddr[REG_AWIDTH-1:0]),
    .s_axi_awvalid (axi_eth_internal_awvalid),
    .s_axi_awready (axi_eth_internal_awready),
    .s_axi_wdata   (axi_eth_internal_wdata),
    .s_axi_wstrb   (axi_eth_internal_wstrb),
    .s_axi_wvalid  (axi_eth_internal_wvalid),
    .s_axi_wready  (axi_eth_internal_wready),
    .s_axi_bresp   (axi_eth_internal_bresp),
    .s_axi_bvalid  (axi_eth_internal_bvalid),
    .s_axi_bready  (axi_eth_internal_bready),
    .s_axi_araddr  (axi_eth_internal_araddr[REG_AWIDTH-1:0]),
    .s_axi_arvalid (axi_eth_internal_arvalid),
    .s_axi_arready (axi_eth_internal_arready),
    .s_axi_rdata   (axi_eth_internal_rdata),
    .s_axi_rresp   (axi_eth_internal_rresp),
    .s_axi_rvalid  (axi_eth_internal_rvalid),
    .s_axi_rready  (axi_eth_internal_rready),
    .e2h_tdata     (e2h_dma_tdata),
    .e2h_tkeep     (e2h_dma_tkeep),
    .e2h_tlast     (e2h_dma_tlast),
    .e2h_tvalid    (e2h_dma_tvalid),
    .e2h_tready    (e2h_dma_tready),
    .h2e_tdata     (h2e_dma_tdata),
    .h2e_tkeep     (h2e_dma_tkeep),
    .h2e_tlast     (h2e_dma_tlast),
    .h2e_tvalid    (h2e_dma_tvalid),
    .h2e_tready    (h2e_dma_tready),
    .e2v_tdata     (e2v_dma_tdata),
    .e2v_tlast     (e2v_dma_tlast),
    .e2v_tvalid    (e2v_dma_tvalid),
    .e2v_tready    (e2v_dma_tready),
    .v2e_tdata     (v2e_dma_tdata),
    .v2e_tlast     (v2e_dma_tlast),
    .v2e_tvalid    (v2e_dma_tvalid),
    .v2e_tready    (v2e_dma_tready),
    .device_id     (device_id)
  );


  //---------------------------------------------------------------------------
  // CPLD Interface
  //---------------------------------------------------------------------------

  wire [COMPONENT_VERSIONS_SIZE-1:0] cpld_ifc_version;

  cpld_interface cpld_interface_i (
    .s_axi_aclk              (clk40),
    .s_axi_aresetn           (clk40_rstn),
    .pll_ref_clk             (pll_ref_clk),
    .radio_clk               (radio_clk[0]),
    .ctrlport_rst            (prc_rst),
    .s_axi_awaddr            (m_axi_mpm_ep_awaddr[16:0]),
    .s_axi_awvalid           (m_axi_mpm_ep_awvalid),
    .s_axi_awready           (m_axi_mpm_ep_awready),
    .s_axi_wdata             (m_axi_mpm_ep_wdata),
    .s_axi_wstrb             (m_axi_mpm_ep_wstrb),
    .s_axi_wvalid            (m_axi_mpm_ep_wvalid),
    .s_axi_wready            (m_axi_mpm_ep_wready),
    .s_axi_bresp             (m_axi_mpm_ep_bresp),
    .s_axi_bvalid            (m_axi_mpm_ep_bvalid),
    .s_axi_bready            (m_axi_mpm_ep_bready),
    .s_axi_araddr            (m_axi_mpm_ep_araddr[16:0]),
    .s_axi_arvalid           (m_axi_mpm_ep_arvalid),
    .s_axi_arready           (m_axi_mpm_ep_arready),
    .s_axi_rdata             (m_axi_mpm_ep_rdata),
    .s_axi_rresp             (m_axi_mpm_ep_rresp),
    .s_axi_rvalid            (m_axi_mpm_ep_rvalid),
    .s_axi_rready            (m_axi_mpm_ep_rready),
    .s_ctrlport_req_wr       (db_to_cpld_comb_ctrlport_req_wr),
    .s_ctrlport_req_rd       (db_to_cpld_comb_ctrlport_req_rd),
    .s_ctrlport_req_addr     (db_to_cpld_comb_ctrlport_req_addr),
    .s_ctrlport_req_data     (db_to_cpld_comb_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (db_to_cpld_comb_ctrlport_req_byte_en),
    .s_ctrlport_resp_ack     (db_to_cpld_comb_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (db_to_cpld_comb_ctrlport_resp_status),
    .s_ctrlport_resp_data    (db_to_cpld_comb_ctrlport_resp_data),
    .ss                      ({PL_CPLD_CS1_n, PL_CPLD_CS0_n}),
    .sclk                    (PL_CPLD_SCLK),
    .mosi                    (PL_CPLD_MOSI),
    .miso                    (PL_CPLD_MISO),
    .qsfp0_led_active        (eth0_activity),
    .qsfp0_led_link          (eth0_link_up),
    .qsfp1_led_active        (eth1_activity),
    .qsfp1_led_link          (eth1_link_up),
    .ipass_present_n         (2'b11),
    .version_info            (cpld_ifc_version)
  );


  //---------------------------------------------------------------------------
  // X4XX Core
  //---------------------------------------------------------------------------

  wire [32*RADIO_SPC*NUM_CHANNELS-1:0] rx_data_iq, rx_data_qi;
  wire [             NUM_CHANNELS-1:0] rx_stb;
  wire [32*RADIO_SPC*NUM_CHANNELS-1:0] tx_data_iq, tx_data_qi;
  wire [             NUM_CHANNELS-1:0] tx_stb;
  wire [                         11:0] gpio_out_a;
  wire [                         11:0] gpio_out_b;
  wire [                         11:0] gpio_en_a;
  wire [                         11:0] gpio_en_b;

  wire mfg_test_en_fabric_clk;
  wire mfg_test_en_gty_rcv_clk;

  // Map RFDC ports to x4xx_core ports
  `ifdef X440

    `include "regmap/x440/rfdc_mapping_regmap_utils.vh"

    assign rx_data_qi = { adc_data_out_tdata [CH7_RX_MAPPING], adc_data_out_tdata [CH6_RX_MAPPING],
                          adc_data_out_tdata [CH5_RX_MAPPING], adc_data_out_tdata [CH4_RX_MAPPING],
                          adc_data_out_tdata [CH3_RX_MAPPING], adc_data_out_tdata [CH2_RX_MAPPING],
                          adc_data_out_tdata [CH1_RX_MAPPING], adc_data_out_tdata [CH0_RX_MAPPING] };
    assign rx_stb     = { adc_data_out_tvalid[CH7_RX_MAPPING], adc_data_out_tvalid[CH6_RX_MAPPING],
                          adc_data_out_tvalid[CH5_RX_MAPPING], adc_data_out_tvalid[CH4_RX_MAPPING],
                          adc_data_out_tvalid[CH3_RX_MAPPING], adc_data_out_tvalid[CH2_RX_MAPPING],
                          adc_data_out_tvalid[CH1_RX_MAPPING], adc_data_out_tvalid[CH0_RX_MAPPING] };

    assign { dac_data_in_tdata[CH7_TX_MAPPING], dac_data_in_tdata[CH6_TX_MAPPING],
             dac_data_in_tdata[CH5_TX_MAPPING], dac_data_in_tdata[CH4_TX_MAPPING],
             dac_data_in_tdata[CH3_TX_MAPPING], dac_data_in_tdata[CH2_TX_MAPPING],
             dac_data_in_tdata[CH1_TX_MAPPING], dac_data_in_tdata[CH0_TX_MAPPING] } = tx_data_qi;
    assign { tx_stb           [CH7_TX_MAPPING], tx_stb           [CH6_TX_MAPPING],
             tx_stb           [CH5_TX_MAPPING], tx_stb           [CH4_TX_MAPPING],
             tx_stb           [CH3_TX_MAPPING], tx_stb           [CH2_TX_MAPPING],
             tx_stb           [CH1_TX_MAPPING], tx_stb           [CH0_TX_MAPPING] } = dac_data_in_tready;


    assign { swapped_invert_adc_iq_rclk2[CH7_RX_MAPPING], swapped_invert_adc_iq_rclk2[CH6_RX_MAPPING],
             swapped_invert_adc_iq_rclk2[CH5_RX_MAPPING], swapped_invert_adc_iq_rclk2[CH4_RX_MAPPING],
             swapped_invert_adc_iq_rclk2[CH3_RX_MAPPING], swapped_invert_adc_iq_rclk2[CH2_RX_MAPPING],
             swapped_invert_adc_iq_rclk2[CH1_RX_MAPPING], swapped_invert_adc_iq_rclk2[CH0_RX_MAPPING] } = invert_adc_iq_rclk2;
    assign { swapped_invert_dac_iq_rclk2[CH7_TX_MAPPING], swapped_invert_dac_iq_rclk2[CH6_TX_MAPPING],
             swapped_invert_dac_iq_rclk2[CH5_TX_MAPPING], swapped_invert_dac_iq_rclk2[CH4_TX_MAPPING],
             swapped_invert_dac_iq_rclk2[CH3_TX_MAPPING], swapped_invert_dac_iq_rclk2[CH2_TX_MAPPING],
             swapped_invert_dac_iq_rclk2[CH1_TX_MAPPING], swapped_invert_dac_iq_rclk2[CH0_TX_MAPPING] } = invert_dac_iq_rclk2;
  `else // X410
    // IMPORTANT! For ZBX RevB, there is a RX channel swap in layout
    //            that we need to correct for here in HDL.
    assign rx_data_qi = { adc_data_out_tdata [2], adc_data_out_tdata [3],
                          adc_data_out_tdata [0], adc_data_out_tdata [1] };
    assign rx_stb     = { adc_data_out_tvalid[2], adc_data_out_tvalid[3],
                          adc_data_out_tvalid[0], adc_data_out_tvalid[1] };

    assign { dac_data_in_tdata[3], dac_data_in_tdata[2],
             dac_data_in_tdata[1], dac_data_in_tdata[0] } = tx_data_qi;
    assign tx_stb             = dac_data_in_tready;

    // invert_adc_iq_rclk2[3:0] relate to DB0, whilst invert_adc_iq_rclk2[7:4] relate to DB1.
    // use this consideration to match the re-mapping above.
    assign swapped_invert_adc_iq_rclk2 = { 2'b00, invert_adc_iq_rclk2[4], invert_adc_iq_rclk2[5],
                                           2'b00, invert_adc_iq_rclk2[0], invert_adc_iq_rclk2[1] };
    assign swapped_invert_dac_iq_rclk2 = invert_dac_iq_rclk2;
  `endif

  // Tie flow control signals (not existent in downstream logic). TX chain
  // always provides valid data when the rf_core is ready to receive.
  assign dac_data_in_tvalid = {NUM_CHANNELS{1'b1}};

  // DIO tristate buffers
  genvar dio_i;
  generate for ( dio_i = 0; dio_i < 12; dio_i = dio_i + 1) begin: dio_tristate_gen
    assign DIOA_FPGA[dio_i] = (gpio_en_a[dio_i]) ? gpio_out_a[dio_i] : 1'bz;
    assign DIOB_FPGA[dio_i] = (gpio_en_b[dio_i]) ? gpio_out_b[dio_i] : 1'bz;
  end endgenerate

  // The RFNoC HDL assumes the data to be ordered with I in the MSBs and Q in
  // the LSBs, whereas the interface of rf_core assumes that Q is in MSBs and I
  // is in the LSBs. Here we swap I and Q to match the ordering of each
  // interface.
  genvar iq_i;
  generate for ( iq_i = 0; iq_i < RADIO_SPC*NUM_CHANNELS; iq_i = iq_i + 1) begin : gen_iq_swap
    assign rx_data_iq[iq_i*32 +: 32] = { rx_data_qi[iq_i*32 +: 16], rx_data_qi[iq_i*32+16 +: 16] };
    assign tx_data_qi[iq_i*32 +: 32] = { tx_data_iq[iq_i*32 +: 16], tx_data_iq[iq_i*32+16 +: 16] };
  end endgenerate

  // Version information mapping
  // Each component consists of a 96-bit vector (refer to versioning_utils.vh)
  //
  // Build FPGA version
  wire [COMPONENT_VERSIONS_SIZE-1:0] fpga_version;
  assign fpga_version = build_component_versions(
    FPGA_VERSION_LAST_MODIFIED_TIME,
    build_version(
      FPGA_OLDEST_COMPATIBLE_VERSION_MAJOR,
      FPGA_OLDEST_COMPATIBLE_VERSION_MINOR,
      FPGA_OLDEST_COMPATIBLE_VERSION_BUILD
    ),
    build_version(
      FPGA_CURRENT_VERSION_MAJOR,
      FPGA_CURRENT_VERSION_MINOR,
      FPGA_CURRENT_VERSION_BUILD
    )
  );
  //
  wire [64*COMPONENT_VERSIONS_SIZE-1:0] x4xx_core_version_info;
  assign x4xx_core_version_info[COMPONENT_VERSIONS_SIZE*FPGA_VERSION_INDEX  +: COMPONENT_VERSIONS_SIZE] = fpga_version;
  assign x4xx_core_version_info[COMPONENT_VERSIONS_SIZE*CPLD_IFC_INDEX      +: COMPONENT_VERSIONS_SIZE] = cpld_ifc_version;
  assign x4xx_core_version_info[COMPONENT_VERSIONS_SIZE*DB0_RF_CORE_INDEX   +: COMPONENT_VERSIONS_SIZE] = rf_core_version[0];
  assign x4xx_core_version_info[COMPONENT_VERSIONS_SIZE*DB1_RF_CORE_INDEX   +: COMPONENT_VERSIONS_SIZE] = rf_core_version[1];
  assign x4xx_core_version_info[COMPONENT_VERSIONS_SIZE*DB0_GPIO_IFC_INDEX  +: COMPONENT_VERSIONS_SIZE] = db_gpio_ifc_version[0];
  assign x4xx_core_version_info[COMPONENT_VERSIONS_SIZE*DB1_GPIO_IFC_INDEX  +: COMPONENT_VERSIONS_SIZE] = db_gpio_ifc_version[1];

  x4xx_core #(
    .NUM_DBOARDS     (NUM_DBOARDS),
    .REG_DWIDTH      (REG_DWIDTH),
    .REG_AWIDTH      (REG_AWIDTH),
    .CHDR_CLK_RATE   (CHDR_CLK_RATE),
    .NUM_CH_PER_DB   (NUM_CH_PER_DB),
    .NUM_CHANNELS    (NUM_CHANNELS),
    .NUM_TIMEKEEPERS (NUM_TIMEKEEPERS),
    .CHDR_W          (CHDR_W),
    .DMA_W           (DMA_W),
    .NET_CHDR_W      (NET_CHDR_W),
    .ENET_W          (ENET_W),
    .ENET_WIDTHS     (ENET_WIDTHS),
    .MTU             (CHDR_MTU),
    .RFNOC_PROTOVER  (RFNOC_PROTOVER),
    .RADIO_SPC       (RADIO_SPC),
    .RF_BANDWIDTH    (RF_BANDWIDTH)
  ) x4xx_core_i (
    .radio_clk                     (radio_clk),
    .radio_rst                     (radio_rst),
    .radio_clk_2x                  (radio_clk_2x),
    .rfnoc_chdr_clk                (bus_clk),
    .rfnoc_chdr_rst                (bus_clk_rst),
    .rfnoc_ctrl_clk                (clk40),
    .rfnoc_ctrl_rst                (clk40_rst),
    .ce_clk                        (ce_clk),
    .dram0_sys_clk_p               (DRAM0_REFCLK_P),
    .dram0_sys_clk_n               (DRAM0_REFCLK_N),
    .dram0_ck_t                    (DRAM0_CLK_P),
    .dram0_ck_c                    (DRAM0_CLK_N),
    .dram0_cs_n                    (DRAM0_CS_n),
    .dram0_act_n                   (DRAM0_ACT_n),
    .dram0_adr                     (DRAM0_ADDR),
    .dram0_ba                      (DRAM0_BA),
    .dram0_bg                      (DRAM0_BG),
    .dram0_cke                     (DRAM0_CKE),
    .dram0_odt                     (DRAM0_ODT),
    .dram0_reset_n                 (DRAM0_RESET_n),
    .dram0_dm_dbi_n                (DRAM0_DM_n),
    .dram0_dq                      (DRAM0_DQ),
    .dram0_dqs_t                   (DRAM0_DQS_p),
    .dram0_dqs_c                   (DRAM0_DQS_n),
    .dram1_sys_clk_p               (DRAM1_REFCLK_P),
    .dram1_sys_clk_n               (DRAM1_REFCLK_N),
    .dram1_ck_t                    (DRAM1_CLK_P),
    .dram1_ck_c                    (DRAM1_CLK_N),
    .dram1_cs_n                    (DRAM1_CS_n),
    .dram1_act_n                   (DRAM1_ACT_n),
    .dram1_adr                     (DRAM1_ADDR),
    .dram1_ba                      (DRAM1_BA),
    .dram1_bg                      (DRAM1_BG),
    .dram1_cke                     (DRAM1_CKE),
    .dram1_odt                     (DRAM1_ODT),
    .dram1_reset_n                 (DRAM1_RESET_n),
    .dram1_dm_dbi_n                (DRAM1_DM_n),
    .dram1_dq                      (DRAM1_DQ),
    .dram1_dqs_t                   (DRAM1_DQS_p),
    .dram1_dqs_c                   (DRAM1_DQS_n),
    .s_axi_aclk                    (clk40),
    .s_axi_aresetn                 (clk40_rstn),
    .s_axi_awaddr                  (axi_core_awaddr[REG_AWIDTH-1:0]),
    .s_axi_awvalid                 (axi_core_awvalid),
    .s_axi_awready                 (axi_core_awready),
    .s_axi_wdata                   (axi_core_wdata),
    .s_axi_wstrb                   (axi_core_wstrb),
    .s_axi_wvalid                  (axi_core_wvalid),
    .s_axi_wready                  (axi_core_wready),
    .s_axi_bresp                   (axi_core_bresp),
    .s_axi_bvalid                  (axi_core_bvalid),
    .s_axi_bready                  (axi_core_bready),
    .s_axi_araddr                  (axi_core_araddr[REG_AWIDTH-1:0]),
    .s_axi_arvalid                 (axi_core_arvalid),
    .s_axi_arready                 (axi_core_arready),
    .s_axi_rdata                   (axi_core_rdata),
    .s_axi_rresp                   (axi_core_rresp),
    .s_axi_rvalid                  (axi_core_rvalid),
    .s_axi_rready                  (axi_core_rready),
    .pps_radioclk                  (pps_radioclk),
    .pps_select                    (pps_select),
    .trig_io_select                (trig_io_select),
    .pll_sync_trigger              (pll_sync_trigger),
    .pll_sync_delay                (pll_sync_delay),
    .pll_sync_done                 (pll_sync_done),
    .pps_brc_delay                 (pps_brc_delay),
    .pps_prc_delay                 (pps_prc_delay),
    .prc_rc0_divider               (prc_rc_divider[4:0]),
    .prc_rc1_divider               (prc_rc_divider[9:5]),
    .pps_rc_enabled                (pps_rc_enabled),
    .rx_data                       (rx_data_iq),
    .rx_stb                        (rx_stb),
    .rx_running                    (rx_running),
    .tx_data                       (tx_data_iq),
    .tx_stb                        (tx_stb),
    .tx_running                    (tx_running),
    .dmao_tdata                    (v2e_dma_tdata),
    .dmao_tlast                    (v2e_dma_tlast),
    .dmao_tvalid                   (v2e_dma_tvalid),
    .dmao_tready                   (v2e_dma_tready),
    .dmai_tdata                    (e2v_dma_tdata),
    .dmai_tlast                    (e2v_dma_tlast),
    .dmai_tvalid                   (e2v_dma_tvalid),
    .dmai_tready                   (e2v_dma_tready),
    .e2v_tdata                     (e2v_tdata),
    .e2v_tlast                     (e2v_tlast),
    .e2v_tvalid                    (e2v_tvalid),
    .e2v_tready                    (e2v_tready),
    .v2e_tdata                     (v2e_tdata),
    .v2e_tlast                     (v2e_tlast),
    .v2e_tvalid                    (v2e_tvalid),
    .v2e_tready                    (v2e_tready),
    .gpio_in_a                     (DIOA_FPGA),
    .gpio_in_b                     (DIOB_FPGA),
    .gpio_out_a                    (gpio_out_a),
    .gpio_out_b                    (gpio_out_b),
    .gpio_en_a                     (gpio_en_a),
    .gpio_en_b                     (gpio_en_b),
    .ps_gpio_out_a                 (ps_gpio_out_a),
    .ps_gpio_in_a                  (ps_gpio_in_a),
    .ps_gpio_ddr_a                 (ps_gpio_ddr_a),
    .ps_gpio_out_b                 (ps_gpio_out_b),
    .ps_gpio_in_b                  (ps_gpio_in_b),
    .ps_gpio_ddr_b                 (ps_gpio_ddr_b),
    .qsfp_port_0_0_info            (qsfp_port_0_0_info),
    .qsfp_port_0_1_info            (qsfp_port_0_1_info),
    .qsfp_port_0_2_info            (qsfp_port_0_2_info),
    .qsfp_port_0_3_info            (qsfp_port_0_3_info),
    .qsfp_port_1_0_info            (qsfp_port_1_0_info),
    .qsfp_port_1_1_info            (qsfp_port_1_1_info),
    .qsfp_port_1_2_info            (qsfp_port_1_2_info),
    .qsfp_port_1_3_info            (qsfp_port_1_3_info),
    .radio_time                    (radio_time),
    .radio_time_stb                (radio_time_stb),
    .device_id                     (device_id),
    .mfg_test_en_fabric_clk        (mfg_test_en_fabric_clk),
    .mfg_test_en_gty_rcv_clk       (mfg_test_en_gty_rcv_clk),
    .fpga_aux_ref                  (FPGA_AUX_REF),
    .m_ctrlport_radio_req_wr       ({ db_ctrlport_req_wr       [1], db_ctrlport_req_wr       [0] }),
    .m_ctrlport_radio_req_rd       ({ db_ctrlport_req_rd       [1], db_ctrlport_req_rd       [0] }),
    .m_ctrlport_radio_req_addr     ({ db_ctrlport_req_addr     [1], db_ctrlport_req_addr     [0] }),
    .m_ctrlport_radio_req_data     ({ db_ctrlport_req_data     [1], db_ctrlport_req_data     [0] }),
    .m_ctrlport_radio_resp_ack     ({ db_ctrlport_resp_ack     [1], db_ctrlport_resp_ack     [0] }),
    .m_ctrlport_radio_resp_status  ({ db_ctrlport_resp_status  [1], db_ctrlport_resp_status  [0] }),
    .m_ctrlport_radio_resp_data    ({ db_ctrlport_resp_data    [1], db_ctrlport_resp_data    [0] }),
    .start_nco_reset               (start_nco_reset),
    .nco_reset_done                (nco_reset_done),
    .adc_reset_pulse               (adc_reset_pulse),
    .dac_reset_pulse               (dac_reset_pulse),
    .version_info                  (x4xx_core_version_info)
  );


  //---------------------------------------------------------------------------
  // eCPRI Clock Output Test
  //---------------------------------------------------------------------------

  wire fabric_clk_oddr;

  wire mfg_test_en_fabric_clk_dc;
  wire mfg_test_en_gty_rcv_clk_dc;

  synchronizer #(
    .STAGES      (2),
    .WIDTH       (1),
    .INITIAL_VAL (1'h0)
  ) synchronizer_mfg_test_en_fabric_clk (
    .clk (radio_clk[0]),
    .rst (1'b0),
    .in  (mfg_test_en_fabric_clk),
    .out (mfg_test_en_fabric_clk_dc)
  );

  synchronizer #(
    .STAGES      (2),
    .WIDTH       (1),
    .INITIAL_VAL (1'h0)
  ) synchronizer_mfg_test_en_gty_rcv_clk (
    .clk (radio_clk[0]),
    .rst (1'b0),
    .in  (mfg_test_en_gty_rcv_clk),
    .out (mfg_test_en_gty_rcv_clk_dc)
  );

  ODDRE1 #(
    .SRVAL (1'b0)  // Initializes the ODDRE1 Flip-Flops to 1'b0
  ) oddre1_fabric_clk (
    .Q  (fabric_clk_oddr),           // 1-bit output: Data output to IOB
    .C  (radio_clk[0]),                 // 1-bit input: High-speed clock input
    .D1 (1'b0),                      // 1-bit input: Parallel data input 1
    .D2 (mfg_test_en_fabric_clk_dc), // 1-bit input: Parallel data input 2
    .SR (1'b0)                       // 1-bit input: Active High Async Reset
  );

  OBUFDS obufds_fabric_clk (
    .O  (FABRIC_CLK_OUT_P), // 1-bit output: Diff_p output (connect directly to top-level port)
    .OB (FABRIC_CLK_OUT_N), // 1-bit output: Diff_n output (connect directly to top-level port)
    .I  (fabric_clk_oddr)   // 1-bit input: Buffer input
  );

  // Requires QSFP1_0 because of limited input options for this buffer.
  // This output on (MGTREFCLK1 128)(QUAD128 is QSFP1).
  // The input is MGT_REFCLK_LMK3_P (MGT_REFCLK1 129)(QUAD 129 not used).
  `ifdef QSFP1_0
  OBUFDS_GTE4 #(
    .REFCLK_EN_TX_PATH (1'b1),
    .REFCLK_ICNTL_TX   (5'b00111)
  ) gty_rcv_clk_OBUFDS (
    .O   (GTY_RCV_CLK_P),               // 1-bit output: Diff_p output (connect directly to top-level port)
    .OB  (GTY_RCV_CLK_N),               // 1-bit output: Diff_n output (connect directly to top-level port)
    .I   (rx_rec_clk_out1),             // 1-bit input: Buffer input
    .CEB (!mfg_test_en_gty_rcv_clk_dc)  // 1-bit input: Clock Enable
  );
  `endif

endmodule


`default_nettype wire


//XmlParse xml_on
//<top name="X410_FPGA">
//  <info>
//    This documentation provides a description of the different register spaces available
//    for the USRP X4xx Open-Source FPGA target implementation, accessible through the
//    embedded ARM A53 processor in the RFSoC chip, and other UHD hosts.
//  </info>
//</top>
//
//<top name="X440_FPGA">
//  <info>
//    This map contains register windows for controlling the different sources
//    that drive the state of FBX control lines.
//  </info>
//</top>
//
//<regmap name="VERSIONING_REGS_REGMAP">
//  <group name="VERSIONING_CONSTANTS">
//    <enumeratedtype name="FPGA_VERSION" showhex="true">
//      <info>
//        FPGA version.{BR/}
//        For guidance on when to update these revision numbers,
//        please refer to the register map documentation accordingly:
//        <li> Current version: @.VERSIONING_REGS_REGMAP..CURRENT_VERSION
//        <li> Oldest compatible version: @.VERSIONING_REGS_REGMAP..OLDEST_COMPATIBLE_VERSION
//        <li> Version last modified: @.VERSIONING_REGS_REGMAP..VERSION_LAST_MODIFIED
//      </info>
//      <value name="FPGA_CURRENT_VERSION_MAJOR"           integer="8"/>
//      <value name="FPGA_CURRENT_VERSION_MINOR"           integer="2"/>
//      <value name="FPGA_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="FPGA_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="8"/>
//      <value name="FPGA_OLDEST_COMPATIBLE_VERSION_MINOR" integer="2"/>
//      <value name="FPGA_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="FPGA_VERSION_LAST_MODIFIED_TIME"      integer="0x23102410"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//XmlParse xml_off
