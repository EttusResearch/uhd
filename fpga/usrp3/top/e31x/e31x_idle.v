/////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: e31x
// Description:
//   E31x Top Level Idle
//
/////////////////////////////////////////////////////////////////////

module e31x (

  // PS Connections
  inout [53:0]  MIO,
  input         PS_SRSTB,
  input         PS_CLK,
  input         PS_PORB,
  inout         DDR_CLK,
  inout         DDR_CLK_N,
  inout         DDR_CKE,
  inout         DDR_CS_N,
  inout         DDR_RAS_N,
  inout         DDR_CAS_N,
  inout         DDR_WEB,
  inout [2:0]   DDR_BANKADDR,
  inout [14:0]  DDR_ADDR,
  inout         DDR_ODT,
  inout         DDR_DRSTB,
  inout [31:0]  DDR_DQ,
  inout [3:0]   DDR_DM,
  inout [3:0]   DDR_DQS,
  inout [3:0]   DDR_DQS_N,
  inout         DDR_VRP,
  inout         DDR_VRN,

  //AVR SPI IO
  input         AVR_CS_R,
  output        AVR_IRQ,
  output        AVR_MISO_R,
  input         AVR_MOSI_R,
  input         AVR_SCK_R,

  input         ONSWITCH_DB,

  // pps connections
  input         GPS_PPS,
  input         PPS_EXT_IN,

  // gpios, change to inout somehow
  inout [5:0]   PL_GPIO,

  // RF Board connections
  inout [99:0] DB_IO
);

  // Clocks
  wire bus_clk;
  wire radio_clk;
  wire reg_clk;
  wire clk40;
  wire FCLK_CLK0;
  wire FCLK_CLK1;

  // Resets
  wire global_rst;
  wire bus_rst;
  wire radio_rst;
  wire reg_rstn;
  wire clk40_rst;
  wire clk40_rstn;
  wire FCLK_RESET0_N;

  // PMU
  wire [31:0] m_axi_pmu_araddr;
  wire [2:0]  m_axi_pmu_arprot;
  wire        m_axi_pmu_arready;
  wire        m_axi_pmu_arvalid;
  wire [31:0] m_axi_pmu_awaddr;
  wire [2:0]  m_axi_pmu_awprot;
  wire        m_axi_pmu_awready;
  wire        m_axi_pmu_awvalid;
  wire        m_axi_pmu_bready;
  wire [1:0]  m_axi_pmu_bresp;
  wire        m_axi_pmu_bvalid;
  wire [31:0] m_axi_pmu_rdata;
  wire        m_axi_pmu_rready;
  wire [1:0]  m_axi_pmu_rresp;
  wire        m_axi_pmu_rvalid;
  wire [31:0] m_axi_pmu_wdata;
  wire        m_axi_pmu_wready;
  wire [3:0]  m_axi_pmu_wstrb;
  wire        m_axi_pmu_wvalid;

  /////////////////////////////////////////////////////////////////////
  //
  // Resets:
  //  - PL - Bus Reset
  //         Radio Reset
  //  - PS - FCLK_RESET0_N --> clk40_rst(n)
  //
  //////////////////////////////////////////////////////////////////////

  // Synchronous reset for the bus_clk domain
  reset_sync bus_reset_gen (
    .clk(bus_clk),
    .reset_in(~FCLK_RESET0_N),
    //.reset_in(~clocks_locked),
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

  wire [1:0] pps_select;

  assign clk40   = FCLK_CLK1;   // 40 MHz
  assign bus_clk = FCLK_CLK0;   // 100 MHz
  assign reg_clk = clk40;

  reg [2:0] pps_reg;

  wire pps_ext = PPS_EXT_IN;
  wire gps_pps = GPS_PPS;

  // connect PPS input to GPIO so ntpd can use it
  always @ (posedge bus_clk)
    pps_reg <= bus_rst ? 3'b000 : {pps_reg[1:0], GPS_PPS};
  assign ps_gpio_in[8] = pps_reg[2]; // 62

  /////////////////////////////////////////////////////////////////////
  //
  // Power Button
  //
  //////////////////////////////////////////////////////////////////////

  // register the debounced onswitch signal to detect edges,
  // Note: ONSWITCH_DB is low active
  reg [1:0] onswitch_edge;
  always @ (posedge bus_clk)
    onswitch_edge <= bus_rst ? 2'b00 : {onswitch_edge[0], ONSWITCH_DB};

  wire button_press = ~ONSWITCH_DB & onswitch_edge[0] & onswitch_edge[1];
  wire button_release = ONSWITCH_DB & ~onswitch_edge[0] & ~onswitch_edge[1];

  // stretch the pulse so IRQs don't get lost
  reg [7:0] button_press_reg, button_release_reg;
  always @ (posedge bus_clk)
    if (bus_rst) begin
      button_press_reg <= 8'h00;
      button_release_reg <= 8'h00;
    end else begin
      button_press_reg <= {button_press_reg[6:0], button_press};
      button_release_reg <= {button_release_reg[6:0], button_release};
    end

  wire button_press_irq = |button_press_reg;
  wire button_release_irq = |button_release_reg;

  /////////////////////////////////////////////////////////////////////
  //
  // Interrupts Fabric to PS
  //
  //////////////////////////////////////////////////////////////////////

  wire [15:0] IRQ_F2P;
  wire pmu_irq;
  assign IRQ_F2P = {12'b0,
                    pmu_irq,            // Interrupt 32
                    button_release_irq, // Interrupt 31
                    button_press_irq,   // Interrupt 30
                    1'b0};

  /////////////////////////////////////////////////////////////////////
  //
  // PS Connections
  //
  //////////////////////////////////////////////////////////////////////

  wire [63:0] ps_gpio_in;
  wire [63:0] ps_gpio_out;
  wire [63:0] ps_gpio_tri;

  e31x_ps_bd e31x_ps_bd_inst (

    // DDR Interface
    .DDR_VRN(DDR_VRN),
    .DDR_VRP(DDR_VRP),
    .DDR_addr(DDR_ADDR),
    .DDR_ba(DDR_BANKADDR),
    .DDR_cas_n(DDR_CAS_N),
    .DDR_ck_n(DDR_CLK_N),
    .DDR_ck_p(DDR_CLK),
    .DDR_cke(DDR_CKE),
    .DDR_cs_n(DDR_CS_N),
    .DDR_dm(DDR_DM),
    .DDR_dq(DDR_DQ),
    .DDR_dqs_n(DDR_DQS_N),
    .DDR_dqs_p(DDR_DQS),
    .DDR_odt(DDR_ODT),
    .DDR_ras_n(DDR_RAS_N),
    .DDR_reset_n(DDR_RESET_N),
    .DDR_we_n(DDR_WE_N),

    // Clocks
    .FCLK_CLK0(FCLK_CLK0),
    .FCLK_CLK1(FCLK_CLK1),
    .FCLK_CLK2(),
    .FCLK_CLK3(),

    // Resets
    .FCLK_RESET0_N(FCLK_RESET0_N),

    // GPIO
    .GPIO_0_tri_i(ps_gpio_in),
    .GPIO_0_tri_o(ps_gpio_out),
    .GPIO_0_tri_t(ps_gpio_tri),

    // Interrupts
    .IRQ_F2P(IRQ_F2P),

    // MIO
    .MIO(MIO),

    .PS_CLK(PS_CLK),
    .PS_PORB(PS_PORB),
    .PS_SRSTB(PS_SRSTB),

    // SPI
    .SPI0_MISO_I(),
    .SPI0_MISO_O(),
    .SPI0_MISO_T(),
    .SPI0_MOSI_I(),
    .SPI0_MOSI_O(),
    .SPI0_MOSI_T(),
    .SPI0_SCLK_I(),
    .SPI0_SCLK_O(),
    .SPI0_SCLK_T(),
    .SPI0_SS1_O(),
    .SPI0_SS2_O(),
    .SPI0_SS_I(),
    .SPI0_SS_O(),
    .SPI0_SS_T(),

    .SPI1_MISO_I(),
    .SPI1_MISO_O(),
    .SPI1_MISO_T(),
    .SPI1_MOSI_I(),
    .SPI1_MOSI_O(),
    .SPI1_MOSI_T(),
    .SPI1_SCLK_I(),
    .SPI1_SCLK_O(),
    .SPI1_SCLK_T(),
    .SPI1_SS1_O(),
    .SPI1_SS2_O(),
    .SPI1_SS_I(),
    .SPI1_SS_O(),
    .SPI1_SS_T(),

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

    // XBAR Regport
    .m_axi_xbar_araddr(),
    .m_axi_xbar_arprot(),
    .m_axi_xbar_arready(),
    .m_axi_xbar_arvalid(),
    .m_axi_xbar_awaddr(),
    .m_axi_xbar_awprot(),
    .m_axi_xbar_awready(),
    .m_axi_xbar_awvalid(),
    .m_axi_xbar_bready(),
    .m_axi_xbar_bresp(),
    .m_axi_xbar_bvalid(),
    .m_axi_xbar_rdata(),
    .m_axi_xbar_rready(),
    .m_axi_xbar_rresp(),
    .m_axi_xbar_rvalid(),
    .m_axi_xbar_wdata(),
    .m_axi_xbar_wready(),
    .m_axi_xbar_wstrb(),
    .m_axi_xbar_wvalid(),

    // PMU
    .m_axi_pmu_araddr(m_axi_pmu_araddr),
    .m_axi_pmu_arprot(m_axi_pmu_arprot),
    .m_axi_pmu_arready(m_axi_pmu_arready),
    .m_axi_pmu_arvalid(m_axi_pmu_arvalid),
    .m_axi_pmu_awaddr(m_axi_pmu_awaddr),
    .m_axi_pmu_awprot(m_axi_pmu_awprot),
    .m_axi_pmu_awready(m_axi_pmu_awready),
    .m_axi_pmu_awvalid(m_axi_pmu_awvalid),
    .m_axi_pmu_bready(m_axi_pmu_bready),
    .m_axi_pmu_bresp(m_axi_pmu_bresp),
    .m_axi_pmu_bvalid(m_axi_pmu_bvalid),
    .m_axi_pmu_rdata(m_axi_pmu_rdata),
    .m_axi_pmu_rready(m_axi_pmu_rready),
    .m_axi_pmu_rresp(m_axi_pmu_rresp),
    .m_axi_pmu_rvalid(m_axi_pmu_rvalid),
    .m_axi_pmu_wdata(m_axi_pmu_wdata),
    .m_axi_pmu_wready(m_axi_pmu_wready),
    .m_axi_pmu_wstrb(m_axi_pmu_wstrb),
    .m_axi_pmu_wvalid(m_axi_pmu_wvalid),

    // DMA
    .s_axis_dma_tdata(),
    .s_axis_dma_tdest(),
    .s_axis_dma_tlast(),
    .s_axis_dma_tready(),
    .s_axis_dma_tvalid(1'b0),
    .m_axis_dma_tdata(),
    .m_axis_dma_tuser(),
    .m_axis_dma_tlast(),
    .m_axis_dma_tready(1'b1),
    .m_axis_dma_tvalid()
  );

  /////////////////////////////////////////////////////////////////////
  //
  // PMU
  //
  //////////////////////////////////////////////////////////////////////

  axi_pmu inst_axi_pmu (
    .s_axi_aclk(clk40),  // TODO: Original design used bus_clk
    .s_axi_areset(clk40_rst),

    .ss(AVR_CS_R),
    .mosi(AVR_MOSI_R),
    .sck(AVR_SCK_R),
    .miso(AVR_MISO_R),

    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr(m_axi_pmu_awaddr),
    .s_axi_awvalid(m_axi_pmu_awvalid),
    .s_axi_awready(m_axi_pmu_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata(m_axi_pmu_wdata),
    .s_axi_wstrb(m_axi_pmu_wstrb),
    .s_axi_wvalid(m_axi_pmu_wvalid),
    .s_axi_wready(m_axi_pmu_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp(m_axi_pmu_bresp),
    .s_axi_bvalid(m_axi_pmu_bvalid),
    .s_axi_bready(m_axi_pmu_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr(m_axi_pmu_araddr),
    .s_axi_arvalid(m_axi_pmu_arvalid),
    .s_axi_arready(m_axi_pmu_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata(m_axi_pmu_rdata),
    .s_axi_rresp(m_axi_pmu_rresp),
    .s_axi_rvalid(m_axi_pmu_rvalid),
    .s_axi_rready(m_axi_pmu_rready),

    .s_axi_irq(pmu_irq)
  );

  assign AVR_IRQ = 1'b0;

  localparam DB_E31X_IDLE_OUT = {
    1'b0,              /* DB_EXP_18_24 (99) */
    13'b0000000000000, /* leds & rx bandsels */
    1'b0,              /* CAT_FB_CLK (85) */
    1'b0,              /* CAT_TX_FRAME (84) */
    1'b0,              /* CAT_RX_DATA_CLK (83) */
    25'b0000_0000_0000_0000_0000_0000_0, /* CAT_RX_FRAME(81), CAT_P0 & CAT_P1 (58)*/
    1'b0,              /* CAT_SYNC(57) */
    4'b0000,           /* CAT_ENAGC(56), CAT_BBCLK_OUT (55), CAT_ENABLE(54), CAT_TXNRX(53) */
    3'b000,            /* DB_EXP_1_8_V_{33,34,32} */
    2'b00,             /* CAT_CTRL_IN[1:0] (49,48) */
    1'b0,              /* CAT_MISO (47) */
    4'b0000,           /* CAT_{MOSI,SCLK,CS,RESETn) (46,45, 44, 43) */
    1'b0,              /* DB_1_8V_31 (42) */
    8'h00,             /* CAT_CTRL_OUT (41:34) */
    1'b0,              /* DB_1_8V_11 (33) */
    1'b0,              /* CAT_CTRL_IN3 (32) */
    1'b0,              /* DB_1_8V_10 (31) */
    1'b0,              /* CAT_CTRL_IN2 (30) */
    1'b0,              /* DB_1_8V_9 (29) */
    1'b0,              /* VCRX2_V2 (28) */
    1'b0,              /* DB_1_8V_8 (25) */
    1'b0,              /* VCRX2_V1 (26) */
    1'b0,              /* DB_1_8V_7 (25) */
    1'b0,              /* VCRX1_V2 (24) */
    1'b0,              /* DB_1_8V_6 (23) */
    1'b0,              /* VCRX1_V1 (22) */
    1'b0,              /* DB_1_8V_5 (21) */
    1'b0,              /* VCTXRX1_V2 (20) */
    1'b0,              /* DB_1_8V_4 (19) */
    1'b0,              /* VCTXRX1_V1 (18) */
    1'b0,              /* DB_1_8V_3 (17) */
    1'b0,              /* VCTXRX2_V1 (16) */
    1'b1,              /* DB_1_8V_2 (15) */
    15'd0};

  localparam DB_E31X_IDLE_DDR = {
    1'b0,              /* DB_EXP_18_24 (99) */
    13'b0101010101010, /* leds & rx bandsels */
    1'b0,              /* CAT_FB_CLK (85) */
    1'b0,              /* CAT_TX_FRAME (84) */
    1'b0,              /* CAT_RX_DATA_CLK (83) */
    25'b0000_0000_0000_0000_0000_0000_0, /* CAT_RX_FRAME(81), CAT_P0 & CAT_P1 (58) */
    1'b0,              /* CAT_SYNC(57) */
    4'b0001,           /* CAT_ENAGC(56), CAT_BBCLK_OUT (55), CAT_ENABLE(54), CAT_TXNRX(53) */
    3'b000,            /* DB_EXP_1_8_V_{32,33,34,} (52, 51, 50) */
    2'b00,             /* CAT_CTRL_IN[1:0] (49,48) */
    1'b0,              /* CAT_MISO (47) */
    4'b0111,           /* CAT_{MOSI,SCLK,CS,RESETn) (46, 45, 44, 43) */
    1'b0,              /* DB_1_8V_31 (42) */
    8'h00,             /* CAT_CTRL_OUT (41:34) */
    1'b0,              /* DB_1_8V_11 (33) */
    1'b0,              /* CAT_CTRL_IN3 (32) */
    1'b0,              /* DB_1_8V_10 (31) */
    1'b0,              /* CAT_CTRL_IN2 (30) */
    1'b0,              /* DB_1_8V_9 (29) */
    1'b0,              /* VCRX2_V2 (28) */
    1'b0,              /* DB_1_8V_8 (25) */
    1'b0,              /* VCRX2_V1 (26) */
    1'b0,              /* DB_1_8V_7 (24) */
    1'b0,              /* VCRX1_V2 (24) */
    1'b0,              /* DB_1_8V_6 (23) */
    1'b0,              /* VCRX1_V1 (22) */
    1'b0,              /* DB_1_8V_5 (21) */
    1'b0,              /* VCTXRX1_V1 (20) */
    1'b0,              /* DB_1_8V_4 (19) */
    1'b0,              /* VCTXRX1_V2 (18) */
    1'b1,              /* DB_1_8V_3 (17) */
    1'b0,              /* VCTXRX2_V1 (16) */
    1'b1,              /* DB_1_8V_2 (15) */
    15'd0};

  localparam NUM_DB_IO_PINS   = 100;

  wire [NUM_DB_IO_PINS-1:0] db_ddr = DB_E31X_IDLE_DDR;
  wire [NUM_DB_IO_PINS-1:0] db_out = DB_E31X_IDLE_OUT;
  wire [NUM_DB_IO_PINS-1:0] db_in;


  genvar k;
  generate
    for (k = 0; k < NUM_DB_IO_PINS; k = k+1) begin
      IOBUF db_io_i(.O(db_in[k]), .IO(DB_IO[k]), .I(db_out[k]), .T(~db_ddr[k]));
    end
  endgenerate

endmodule // e31x
