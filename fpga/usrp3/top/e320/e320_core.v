/////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0
//
// Module: e320_core
// Description:
//  - Motherboard Registers
//  - DRAM Interconnect
//  - Radio Front End control
//  - Timekeeper
//  - RFNoC Image Core
//
/////////////////////////////////////////////////////////////////////

`default_nettype none
module e320_core #(
  parameter REG_DWIDTH   = 32,        // Width of the AXI4-Lite data bus (must be 32 or 64)
  parameter REG_AWIDTH   = 32,        // Width of the address bus
  parameter BUS_CLK_RATE = 200000000, // bus_clk rate
  parameter NUM_RADIOS = 1,
  parameter NUM_CHANNELS = 2,
  parameter NUM_DBOARDS = 1,
  parameter NUM_CHANNELS_PER_DBOARD = 2,
  parameter FP_GPIO_WIDTH = 8,  // Front panel GPIO width
  parameter DB_GPIO_WIDTH = 16,  // Daughterboard GPIO width
  parameter CHDR_WIDTH  = 16'd64 ,
  parameter RFNOC_PROTOVER  = {8'd1, 8'd0}
)(
  // Clocks and resets
  input wire radio_clk,
  input wire radio_rst,
  input wire bus_clk,
  input wire bus_rst,
  input wire ddr3_dma_clk,
  input wire clk40,

  // Motherboard Registers: AXI lite interface
  input wire                    s_axi_aclk,
  input wire                    s_axi_aresetn,
  input wire [REG_AWIDTH-1:0]   s_axi_awaddr,
  input wire                    s_axi_awvalid,
  output wire                   s_axi_awready,

  input wire [REG_DWIDTH-1:0]   s_axi_wdata,
  input wire [REG_DWIDTH/8-1:0] s_axi_wstrb,
  input wire                    s_axi_wvalid,
  output wire                   s_axi_wready,

  output wire [1:0]             s_axi_bresp,
  output wire                   s_axi_bvalid,
  input wire                    s_axi_bready,

  input wire [REG_AWIDTH-1:0]   s_axi_araddr,
  input wire                    s_axi_arvalid,
  output wire                   s_axi_arready,

  output wire [REG_DWIDTH-1:0]  s_axi_rdata,
  output wire [1:0]             s_axi_rresp,
  output wire                   s_axi_rvalid,
  input wire                    s_axi_rready,

  // PPS and Clock Control
  input wire            pps_refclk,
  input wire            refclk_locked,
  output reg [1:0]      pps_select,
  output reg            ref_select,

  // PS GPIO source
  input wire  [FP_GPIO_WIDTH-1:0]  ps_gpio_out,
  input wire  [FP_GPIO_WIDTH-1:0]  ps_gpio_tri,
  output wire [FP_GPIO_WIDTH-1:0]  ps_gpio_in,

  // Front Panel GPIO
  input wire  [FP_GPIO_WIDTH-1:0] fp_gpio_in,
  output wire [FP_GPIO_WIDTH-1:0] fp_gpio_tri,
  output wire [FP_GPIO_WIDTH-1:0] fp_gpio_out,

  // Radio GPIO control
  output wire [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_out_flat,
  output wire [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_ddr_flat,
  input wire  [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_in_flat,
  input wire  [DB_GPIO_WIDTH*NUM_CHANNELS-1:0] db_gpio_fab_flat,

  // TX/RX LEDs
  output wire [32*NUM_CHANNELS-1:0] leds_flat,

  // Radio ATR
  output wire [NUM_CHANNELS-1:0] rx_atr,
  output wire [NUM_CHANNELS-1:0] tx_atr,

  // Radio Data
  input wire  [NUM_CHANNELS-1:0]    rx_stb,
  input wire  [NUM_CHANNELS-1:0]    tx_stb,
  input wire  [32*NUM_CHANNELS-1:0] rx,
  output wire [32*NUM_CHANNELS-1:0] tx,

  // AXI4 DDR3 Interface
  input wire          ddr3_axi_clk,
  input wire          ddr3_axi_rst,
  input wire          ddr3_running,
  // Write Address Ports
  output wire [3:0]   ddr3_axi_awid,
  output wire [31:0]  ddr3_axi_awaddr,
  output wire [7:0]   ddr3_axi_awlen,
  output wire [2:0]   ddr3_axi_awsize,
  output wire [1:0]   ddr3_axi_awburst,
  output wire [0:0]   ddr3_axi_awlock,
  output wire [3:0]   ddr3_axi_awcache,
  output wire [2:0]   ddr3_axi_awprot,
  output wire [3:0]   ddr3_axi_awqos,
  output wire         ddr3_axi_awvalid,
  input wire          ddr3_axi_awready,
  // Write Data Ports
  output wire [255:0] ddr3_axi_wdata,
  output wire [31:0]  ddr3_axi_wstrb,
  output wire         ddr3_axi_wlast,
  output wire         ddr3_axi_wvalid,
  input wire          ddr3_axi_wready,
  // Write Response Ports
  output wire         ddr3_axi_bready,
  input wire [3:0]    ddr3_axi_bid,
  input wire [1:0]    ddr3_axi_bresp,
  input wire          ddr3_axi_bvalid,
  // Read Address Ports
  output wire [3:0]   ddr3_axi_arid,
  output wire [31:0]  ddr3_axi_araddr,
  output wire [7:0]   ddr3_axi_arlen,
  output wire [2:0]   ddr3_axi_arsize,
  output wire [1:0]   ddr3_axi_arburst,
  output wire [0:0]   ddr3_axi_arlock,
  output wire [3:0]   ddr3_axi_arcache,
  output wire [2:0]   ddr3_axi_arprot,
  output wire [3:0]   ddr3_axi_arqos,
  output wire         ddr3_axi_arvalid,
  input wire          ddr3_axi_arready,
  // Read Data Ports
  output wire         ddr3_axi_rready,
  input wire [3:0]    ddr3_axi_rid,
  input wire [255:0]  ddr3_axi_rdata,
  input wire [1:0]    ddr3_axi_rresp,
  input wire          ddr3_axi_rlast,
  input wire          ddr3_axi_rvalid,



  // DMA xport adapter to PS
  input wire  [63:0] s_dma_tdata,
  input wire  [3:0]  s_dma_tuser,
  input wire         s_dma_tlast,
  output wire        s_dma_tready,
  input wire         s_dma_tvalid,

  output wire [63:0] m_dma_tdata,
  output wire [3:0]  m_dma_tdest,
  output wire        m_dma_tlast,
  input wire         m_dma_tready,
  output wire        m_dma_tvalid,

  // e2v (Ethernet to Vita) and v2e (Vita to Ethernet)
  output wire [63:0] v2e_tdata,
  output wire        v2e_tvalid,
  output wire        v2e_tlast,
  input wire         v2e_tready,

  input wire  [63:0] e2v_tdata,
  input wire         e2v_tlast,
  input wire         e2v_tvalid,
  output wire        e2v_tready,

  // Misc
  input wire [31:0] build_datestamp,
  input wire [31:0] sfp_ports_info,
  input wire [31:0] gps_status,
  output reg [31:0] gps_ctrl,
  input wire [31:0] dboard_status,
  input wire [31:0] xadc_readback,
  output reg [31:0] fp_gpio_ctrl,
  output reg [31:0] dboard_ctrl,
  output reg [15:0] device_id
);

  /////////////////////////////////////////////////////////////////////////////////
  //
  // FPGA Compatibility Number
  //   Rules for modifying compat number:
  //   - Major is updated when the FPGA is changed and requires a software
  //     change as a result.
  //   - Minor is updated when a new feature is added to the FPGA that does not
  //     break software compatibility.
  //
  /////////////////////////////////////////////////////////////////////////////////

  localparam [15:0] COMPAT_MAJOR = 16'd5;
  localparam [15:0] COMPAT_MINOR = 16'd0;

  /////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////
  //
  // Motherboard Registers
  //
  /////////////////////////////////////////////////////////////////////////////////

  // Register base
  localparam REG_BASE_MISC         = 14'h0;
  localparam REG_BASE_TIMEKEEPER   = 14'h1000;

  // Misc Registers
  localparam REG_COMPAT_NUM        = REG_BASE_MISC + 14'h00;
  localparam REG_DATESTAMP         = REG_BASE_MISC + 14'h04;
  localparam REG_GIT_HASH          = REG_BASE_MISC + 14'h08;
  localparam REG_SCRATCH           = REG_BASE_MISC + 14'h0C;
  localparam REG_DEVICE_ID         = REG_BASE_MISC + 14'h10;
  localparam REG_RFNOC_INFO        = REG_BASE_MISC + 14'h14;
  localparam REG_CLOCK_CTRL        = REG_BASE_MISC + 14'h18;
  localparam REG_XADC_READBACK     = REG_BASE_MISC + 14'h1C;
  localparam REG_BUS_CLK_RATE      = REG_BASE_MISC + 14'h20;
  localparam REG_BUS_CLK_COUNT     = REG_BASE_MISC + 14'h24;
  localparam REG_SFP_PORT_INFO     = REG_BASE_MISC + 14'h28;
  localparam REG_FP_GPIO_CTRL      = REG_BASE_MISC + 14'h2C;
  localparam REG_FP_GPIO_MASTER    = REG_BASE_MISC + 14'h30;
  localparam REG_FP_GPIO_RADIO_SRC = REG_BASE_MISC + 14'h34;
  localparam REG_GPS_CTRL          = REG_BASE_MISC + 14'h38;
  localparam REG_GPS_STATUS        = REG_BASE_MISC + 14'h3C;
  localparam REG_DBOARD_CTRL       = REG_BASE_MISC + 14'h40;
  localparam REG_DBOARD_STATUS     = REG_BASE_MISC + 14'h44;
  localparam REG_NUM_TIMEKEEPERS   = REG_BASE_MISC + 14'h48;

  localparam NUM_TIMEKEEPERS = 16'd1;

  wire                  m_ctrlport_req_wr;
  wire                  m_ctrlport_req_rd;
  wire [19:0]           m_ctrlport_req_addr;
  wire [31:0]           m_ctrlport_req_data;
  wire                  m_ctrlport_req_has_time;
  wire [63:0]           m_ctrlport_req_time;
  wire                  m_ctrlport_resp_ack;
  wire [31:0]           m_ctrlport_resp_data;

  reg  [31:0]           fp_gpio_master_reg = 32'h0;
  reg  [31:0]           fp_gpio_src_reg    = 32'h0;

  wire                  reg_wr_req;
  wire [REG_AWIDTH-1:0] reg_wr_addr;
  wire [REG_DWIDTH-1:0] reg_wr_data;
  wire                  reg_rd_req;
  wire [REG_AWIDTH-1:0] reg_rd_addr;
  wire                  reg_rd_resp;
  wire [REG_DWIDTH-1:0] reg_rd_data;

  reg                   reg_rd_resp_glob;
  reg  [REG_DWIDTH-1:0] reg_rd_data_glob;
  wire                  reg_rd_resp_tk;
  wire [REG_DWIDTH-1:0] reg_rd_data_tk;

  reg [31:0] scratch_reg = 32'h0;
  reg [31:0] bus_counter = 32'h0;

  always @(posedge bus_clk) begin
     if (bus_rst)
        bus_counter <= 32'd0;
     else
        bus_counter <= bus_counter + 32'd1;
  end

  // Regport Master to convert AXI4-Lite to regport
  axil_regport_master #(
    .DWIDTH   (REG_DWIDTH), // Width of the AXI4-Lite data bus (must be 32 or 64)
    .AWIDTH   (REG_AWIDTH), // Width of the address bus
    .WRBASE   (0),          // Write address base
    .RDBASE   (0),          // Read address base
    .TIMEOUT  (10)          // log2(timeout). Read will timeout after (2^TIMEOUT - 1) cycles
  ) core_regport_master_i (
    // Clock and reset
    .s_axi_aclk    (s_axi_aclk),
    .s_axi_aresetn (s_axi_aresetn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr  (s_axi_awaddr),
    .s_axi_awvalid (s_axi_awvalid),
    .s_axi_awready (s_axi_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata   (s_axi_wdata),
    .s_axi_wstrb   (s_axi_wstrb),
    .s_axi_wvalid  (s_axi_wvalid),
    .s_axi_wready  (s_axi_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp   (s_axi_bresp),
    .s_axi_bvalid  (s_axi_bvalid),
    .s_axi_bready  (s_axi_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr  (s_axi_araddr),
    .s_axi_arvalid (s_axi_arvalid),
    .s_axi_arready (s_axi_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata   (s_axi_rdata),
    .s_axi_rresp   (s_axi_rresp),
    .s_axi_rvalid  (s_axi_rvalid),
    .s_axi_rready  (s_axi_rready),
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

  //--------------------------------------------------------------------
  // Global Registers
  // -------------------------------------------------------------------

  // Write Registers
  always @ (posedge bus_clk) begin
    if (bus_rst) begin
      scratch_reg    <= 32'h0;
      pps_select     <= 2'b01; // Default to internal
      ref_select     <= 1'b0;  // Default to internal
      fp_gpio_ctrl   <= 32'h9; // Default to OFF - 4'b1001
      gps_ctrl       <= 32'h3; // Default to gps_en, out of reset
      dboard_ctrl    <= 32'h1; // Default to mimo
      device_id      <= 16'h0;
    end else if (reg_wr_req) begin
      case (reg_wr_addr)
        REG_DEVICE_ID: begin
          device_id <= reg_wr_data[15:0];
        end
        REG_FP_GPIO_MASTER: begin
          fp_gpio_master_reg <= reg_wr_data;
        end
        REG_FP_GPIO_RADIO_SRC: begin
          fp_gpio_src_reg <= reg_wr_data;
        end
        REG_SCRATCH: begin
          scratch_reg <= reg_wr_data;
        end
        REG_CLOCK_CTRL: begin
          pps_select  <= reg_wr_data[1:0];
          ref_select  <= reg_wr_data[2];
        end
        REG_FP_GPIO_CTRL: begin
          fp_gpio_ctrl <= reg_wr_data;
        end
        REG_GPS_CTRL: begin
          gps_ctrl    <= reg_wr_data;
        end
        REG_DBOARD_CTRL: begin
          dboard_ctrl <= reg_wr_data;
        end
      endcase
    end
  end

  // Read Registers
  always @ (posedge bus_clk) begin
    if (bus_rst) begin
      reg_rd_resp_glob <= 1'b0;
    end
    else begin

      if (reg_rd_req) begin
        reg_rd_resp_glob <= 1'b1;

        case (reg_rd_addr)
        REG_DEVICE_ID:
          reg_rd_data_glob <= device_id;

        REG_RFNOC_INFO:
          reg_rd_data_glob <= {CHDR_WIDTH[15:0], RFNOC_PROTOVER[15:0]};

        REG_COMPAT_NUM:
          reg_rd_data_glob <= {COMPAT_MAJOR[15:0], COMPAT_MINOR[15:0]};

        REG_FP_GPIO_CTRL:
          reg_rd_data_glob <= fp_gpio_ctrl;

        REG_FP_GPIO_MASTER:
          reg_rd_data_glob <= fp_gpio_master_reg;

        REG_FP_GPIO_RADIO_SRC:
          reg_rd_data_glob <= fp_gpio_src_reg;

        REG_DATESTAMP:
          reg_rd_data_glob <= build_datestamp;

        REG_GIT_HASH:
          `ifndef GIT_HASH
          `define GIT_HASH 32'h0BADC0DE
          `endif
          reg_rd_data_glob <= `GIT_HASH;

        REG_SCRATCH:
          reg_rd_data_glob <= scratch_reg;

        REG_CLOCK_CTRL: begin
          reg_rd_data_glob      <= 32'b0;
          reg_rd_data_glob[1:0] <= pps_select;
          reg_rd_data_glob[2]   <= ref_select;
          reg_rd_data_glob[3]   <= refclk_locked;
        end

        REG_XADC_READBACK:
          reg_rd_data_glob <= xadc_readback;

        REG_BUS_CLK_RATE:
          reg_rd_data_glob <= BUS_CLK_RATE;

        REG_BUS_CLK_COUNT:
          reg_rd_data_glob <= bus_counter;

        REG_SFP_PORT_INFO:
          reg_rd_data_glob <= sfp_ports_info;

        REG_GPS_CTRL:
          reg_rd_data_glob <= gps_ctrl;

        REG_GPS_STATUS:
          reg_rd_data_glob <= gps_status;

        REG_DBOARD_CTRL:
          reg_rd_data_glob <= dboard_ctrl;

        REG_DBOARD_STATUS:
          reg_rd_data_glob <= dboard_status;

        REG_NUM_TIMEKEEPERS:
          reg_rd_data_glob <= NUM_TIMEKEEPERS;

        default:
          reg_rd_resp_glob <= 1'b0;
        endcase
      end
      else if (reg_rd_resp_glob) begin
          reg_rd_resp_glob <= 1'b0;
      end
    end
  end

  wire pps_radioclk;

  // Synchronize the PPS signal to the radio clock domain
  synchronizer pps_radio_sync (
    .clk(radio_clk), .rst(1'b0), .in(pps_refclk), .out(pps_radioclk)
  );


  /////////////////////////////////////////////////////////////////////////////
  // 
  // DRAM
  //
  /////////////////////////////////////////////////////////////////////////////

  localparam NUM_DRAM_FIFOS = 2;

  wire ddr3_dma_rst;

  synchronizer #(
    .INITIAL_VAL(1'b1)
  ) ddr3_dma_rst_sync_i (
    .clk(ddr3_dma_clk), .rst(1'b0), .in(ddr3_axi_rst), .out(ddr3_dma_rst)
  );

  // AXI4 MM buses
  wire [0:0]  dram_axi_awid     [0:NUM_DRAM_FIFOS-1];
  wire [31:0] dram_axi_awaddr   [0:NUM_DRAM_FIFOS-1];
  wire [7:0]  dram_axi_awlen    [0:NUM_DRAM_FIFOS-1];
  wire [2:0]  dram_axi_awsize   [0:NUM_DRAM_FIFOS-1];
  wire [1:0]  dram_axi_awburst  [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_awlock   [0:NUM_DRAM_FIFOS-1];
  wire [3:0]  dram_axi_awcache  [0:NUM_DRAM_FIFOS-1];
  wire [2:0]  dram_axi_awprot   [0:NUM_DRAM_FIFOS-1];
  wire [3:0]  dram_axi_awqos    [0:NUM_DRAM_FIFOS-1];
  wire [3:0]  dram_axi_awregion [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_awuser   [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_awvalid  [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_awready  [0:NUM_DRAM_FIFOS-1];
  wire [63:0] dram_axi_wdata    [0:NUM_DRAM_FIFOS-1];
  wire [7:0]  dram_axi_wstrb    [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_wlast    [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_wuser    [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_wvalid   [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_wready   [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_bid      [0:NUM_DRAM_FIFOS-1];
  wire [1:0]  dram_axi_bresp    [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_buser    [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_bvalid   [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_bready   [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_arid     [0:NUM_DRAM_FIFOS-1];
  wire [31:0] dram_axi_araddr   [0:NUM_DRAM_FIFOS-1];
  wire [7:0]  dram_axi_arlen    [0:NUM_DRAM_FIFOS-1];
  wire [2:0]  dram_axi_arsize   [0:NUM_DRAM_FIFOS-1];
  wire [1:0]  dram_axi_arburst  [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_arlock   [0:NUM_DRAM_FIFOS-1];
  wire [3:0]  dram_axi_arcache  [0:NUM_DRAM_FIFOS-1];
  wire [2:0]  dram_axi_arprot   [0:NUM_DRAM_FIFOS-1];
  wire [3:0]  dram_axi_arqos    [0:NUM_DRAM_FIFOS-1];
  wire [3:0]  dram_axi_arregion [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_aruser   [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_arvalid  [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_arready  [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_rid      [0:NUM_DRAM_FIFOS-1];
  wire [63:0] dram_axi_rdata    [0:NUM_DRAM_FIFOS-1];
  wire [1:0]  dram_axi_rresp    [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_rlast    [0:NUM_DRAM_FIFOS-1];
  wire [0:0]  dram_axi_ruser    [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_rvalid   [0:NUM_DRAM_FIFOS-1];
  wire        dram_axi_rready   [0:NUM_DRAM_FIFOS-1];

  axi_intercon_4x64_256_bd_wrapper axi_intercon_2x64_256_bd_i (
    .S00_AXI_ACLK     (ddr3_dma_clk        ),
    .S00_AXI_ARESETN  (~ddr3_dma_rst       ),
    .S00_AXI_AWID     (dram_axi_awid    [0]),
    .S00_AXI_AWADDR   (dram_axi_awaddr  [0]),
    .S00_AXI_AWLEN    (dram_axi_awlen   [0]),
    .S00_AXI_AWSIZE   (dram_axi_awsize  [0]),
    .S00_AXI_AWBURST  (dram_axi_awburst [0]),
    .S00_AXI_AWLOCK   (dram_axi_awlock  [0]),
    .S00_AXI_AWCACHE  (dram_axi_awcache [0]),
    .S00_AXI_AWPROT   (dram_axi_awprot  [0]),
    .S00_AXI_AWQOS    (dram_axi_awqos   [0]),
    .S00_AXI_AWREGION (dram_axi_awregion[0]),
    .S00_AXI_AWVALID  (dram_axi_awvalid [0]),
    .S00_AXI_AWREADY  (dram_axi_awready [0]),
    .S00_AXI_WDATA    (dram_axi_wdata   [0]),
    .S00_AXI_WSTRB    (dram_axi_wstrb   [0]),
    .S00_AXI_WLAST    (dram_axi_wlast   [0]),
    .S00_AXI_WVALID   (dram_axi_wvalid  [0]),
    .S00_AXI_WREADY   (dram_axi_wready  [0]),
    .S00_AXI_BID      (dram_axi_bid     [0]),
    .S00_AXI_BRESP    (dram_axi_bresp   [0]),
    .S00_AXI_BVALID   (dram_axi_bvalid  [0]),
    .S00_AXI_BREADY   (dram_axi_bready  [0]),
    .S00_AXI_ARID     (dram_axi_arid    [0]),
    .S00_AXI_ARADDR   (dram_axi_araddr  [0]),
    .S00_AXI_ARLEN    (dram_axi_arlen   [0]),
    .S00_AXI_ARSIZE   (dram_axi_arsize  [0]),
    .S00_AXI_ARBURST  (dram_axi_arburst [0]),
    .S00_AXI_ARLOCK   (dram_axi_arlock  [0]),
    .S00_AXI_ARCACHE  (dram_axi_arcache [0]),
    .S00_AXI_ARPROT   (dram_axi_arprot  [0]),
    .S00_AXI_ARQOS    (dram_axi_arqos   [0]),
    .S00_AXI_ARREGION (dram_axi_arregion[0]),
    .S00_AXI_ARVALID  (dram_axi_arvalid [0]),
    .S00_AXI_ARREADY  (dram_axi_arready [0]),
    .S00_AXI_RID      (dram_axi_rid     [0]),
    .S00_AXI_RDATA    (dram_axi_rdata   [0]),
    .S00_AXI_RRESP    (dram_axi_rresp   [0]),
    .S00_AXI_RLAST    (dram_axi_rlast   [0]),
    .S00_AXI_RVALID   (dram_axi_rvalid  [0]),
    .S00_AXI_RREADY   (dram_axi_rready  [0]),
    //
    .S01_AXI_ACLK     (ddr3_dma_clk        ),
    .S01_AXI_ARESETN  (~ddr3_dma_rst       ),
    .S01_AXI_AWID     (dram_axi_awid    [1]),
    .S01_AXI_AWADDR   (dram_axi_awaddr  [1]),
    .S01_AXI_AWLEN    (dram_axi_awlen   [1]),
    .S01_AXI_AWSIZE   (dram_axi_awsize  [1]),
    .S01_AXI_AWBURST  (dram_axi_awburst [1]),
    .S01_AXI_AWLOCK   (dram_axi_awlock  [1]),
    .S01_AXI_AWCACHE  (dram_axi_awcache [1]),
    .S01_AXI_AWPROT   (dram_axi_awprot  [1]),
    .S01_AXI_AWQOS    (dram_axi_awqos   [1]),
    .S01_AXI_AWREGION (dram_axi_awregion[1]),
    .S01_AXI_AWVALID  (dram_axi_awvalid [1]),
    .S01_AXI_AWREADY  (dram_axi_awready [1]),
    .S01_AXI_WDATA    (dram_axi_wdata   [1]),
    .S01_AXI_WSTRB    (dram_axi_wstrb   [1]),
    .S01_AXI_WLAST    (dram_axi_wlast   [1]),
    .S01_AXI_WVALID   (dram_axi_wvalid  [1]),
    .S01_AXI_WREADY   (dram_axi_wready  [1]),
    .S01_AXI_BID      (dram_axi_bid     [1]),
    .S01_AXI_BRESP    (dram_axi_bresp   [1]),
    .S01_AXI_BVALID   (dram_axi_bvalid  [1]),
    .S01_AXI_BREADY   (dram_axi_bready  [1]),
    .S01_AXI_ARID     (dram_axi_arid    [1]),
    .S01_AXI_ARADDR   (dram_axi_araddr  [1]),
    .S01_AXI_ARLEN    (dram_axi_arlen   [1]),
    .S01_AXI_ARSIZE   (dram_axi_arsize  [1]),
    .S01_AXI_ARBURST  (dram_axi_arburst [1]),
    .S01_AXI_ARLOCK   (dram_axi_arlock  [1]),
    .S01_AXI_ARCACHE  (dram_axi_arcache [1]),
    .S01_AXI_ARPROT   (dram_axi_arprot  [1]),
    .S01_AXI_ARQOS    (dram_axi_arqos   [1]),
    .S01_AXI_ARREGION (dram_axi_arregion[1]),
    .S01_AXI_ARVALID  (dram_axi_arvalid [1]),
    .S01_AXI_ARREADY  (dram_axi_arready [1]),
    .S01_AXI_RID      (dram_axi_rid     [1]),
    .S01_AXI_RDATA    (dram_axi_rdata   [1]),
    .S01_AXI_RRESP    (dram_axi_rresp   [1]),
    .S01_AXI_RLAST    (dram_axi_rlast   [1]),
    .S01_AXI_RVALID   (dram_axi_rvalid  [1]),
    .S01_AXI_RREADY   (dram_axi_rready  [1]),
    //
   .S02_AXI_ACLK     (ddr3_dma_clk        ),
   .S02_AXI_ARESETN  (~ddr3_dma_rst       ),
   .S02_AXI_AWID     (dram_axi_awid    [2]),
   .S02_AXI_AWADDR   (dram_axi_awaddr  [2]),
   .S02_AXI_AWLEN    (dram_axi_awlen   [2]),
   .S02_AXI_AWSIZE   (dram_axi_awsize  [2]),
   .S02_AXI_AWBURST  (dram_axi_awburst [2]),
   .S02_AXI_AWLOCK   (dram_axi_awlock  [2]),
   .S02_AXI_AWCACHE  (dram_axi_awcache [2]),
   .S02_AXI_AWPROT   (dram_axi_awprot  [2]),
   .S02_AXI_AWQOS    (dram_axi_awqos   [2]),
   .S02_AXI_AWREGION (dram_axi_awregion[2]),
   .S02_AXI_AWVALID  (dram_axi_awvalid [2]),
   .S02_AXI_AWREADY  (dram_axi_awready [2]),
   .S02_AXI_WDATA    (dram_axi_wdata   [2]),
   .S02_AXI_WSTRB    (dram_axi_wstrb   [2]),
   .S02_AXI_WLAST    (dram_axi_wlast   [2]),
   .S02_AXI_WVALID   (dram_axi_wvalid  [2]),
   .S02_AXI_WREADY   (dram_axi_wready  [2]),
   .S02_AXI_BID      (dram_axi_bid     [2]),
   .S02_AXI_BRESP    (dram_axi_bresp   [2]),
   .S02_AXI_BVALID   (dram_axi_bvalid  [2]),
   .S02_AXI_BREADY   (dram_axi_bready  [2]),
   .S02_AXI_ARID     (dram_axi_arid    [2]),
   .S02_AXI_ARADDR   (dram_axi_araddr  [2]),
   .S02_AXI_ARLEN    (dram_axi_arlen   [2]),
   .S02_AXI_ARSIZE   (dram_axi_arsize  [2]),
   .S02_AXI_ARBURST  (dram_axi_arburst [2]),
   .S02_AXI_ARLOCK   (dram_axi_arlock  [2]),
   .S02_AXI_ARCACHE  (dram_axi_arcache [2]),
   .S02_AXI_ARPROT   (dram_axi_arprot  [2]),
   .S02_AXI_ARQOS    (dram_axi_arqos   [2]),
   .S02_AXI_ARREGION (dram_axi_arregion[2]),
   .S02_AXI_ARVALID  (dram_axi_arvalid [2]),
   .S02_AXI_ARREADY  (dram_axi_arready [2]),
   .S02_AXI_RID      (dram_axi_rid     [2]),
   .S02_AXI_RDATA    (dram_axi_rdata   [2]),
   .S02_AXI_RRESP    (dram_axi_rresp   [2]),
   .S02_AXI_RLAST    (dram_axi_rlast   [2]),
   .S02_AXI_RVALID   (dram_axi_rvalid  [2]),
   .S02_AXI_RREADY   (dram_axi_rready  [2]),
   //
   .S03_AXI_ACLK     (ddr3_dma_clk        ),
   .S03_AXI_ARESETN  (~ddr3_dma_rst       ),
   .S03_AXI_AWID     (dram_axi_awid    [3]),
   .S03_AXI_AWADDR   (dram_axi_awaddr  [3]),
   .S03_AXI_AWLEN    (dram_axi_awlen   [3]),
   .S03_AXI_AWSIZE   (dram_axi_awsize  [3]),
   .S03_AXI_AWBURST  (dram_axi_awburst [3]),
   .S03_AXI_AWLOCK   (dram_axi_awlock  [3]),
   .S03_AXI_AWCACHE  (dram_axi_awcache [3]),
   .S03_AXI_AWPROT   (dram_axi_awprot  [3]),
   .S03_AXI_AWQOS    (dram_axi_awqos   [3]),
   .S03_AXI_AWREGION (dram_axi_awregion[3]),
   .S03_AXI_AWVALID  (dram_axi_awvalid [3]),
   .S03_AXI_AWREADY  (dram_axi_awready [3]),
   .S03_AXI_WDATA    (dram_axi_wdata   [3]),
   .S03_AXI_WSTRB    (dram_axi_wstrb   [3]),
   .S03_AXI_WLAST    (dram_axi_wlast   [3]),
   .S03_AXI_WVALID   (dram_axi_wvalid  [3]),
   .S03_AXI_WREADY   (dram_axi_wready  [3]),
   .S03_AXI_BID      (dram_axi_bid     [3]),
   .S03_AXI_BRESP    (dram_axi_bresp   [3]),
   .S03_AXI_BVALID   (dram_axi_bvalid  [3]),
   .S03_AXI_BREADY   (dram_axi_bready  [3]),
   .S03_AXI_ARID     (dram_axi_arid    [3]),
   .S03_AXI_ARADDR   (dram_axi_araddr  [3]),
   .S03_AXI_ARLEN    (dram_axi_arlen   [3]),
   .S03_AXI_ARSIZE   (dram_axi_arsize  [3]),
   .S03_AXI_ARBURST  (dram_axi_arburst [3]),
   .S03_AXI_ARLOCK   (dram_axi_arlock  [3]),
   .S03_AXI_ARCACHE  (dram_axi_arcache [3]),
   .S03_AXI_ARPROT   (dram_axi_arprot  [3]),
   .S03_AXI_ARQOS    (dram_axi_arqos   [3]),
   .S03_AXI_ARREGION (dram_axi_arregion[3]),
   .S03_AXI_ARVALID  (dram_axi_arvalid [3]),
   .S03_AXI_ARREADY  (dram_axi_arready [3]),
   .S03_AXI_RID      (dram_axi_rid     [3]),
   .S03_AXI_RDATA    (dram_axi_rdata   [3]),
   .S03_AXI_RRESP    (dram_axi_rresp   [3]),
   .S03_AXI_RLAST    (dram_axi_rlast   [3]),
   .S03_AXI_RVALID   (dram_axi_rvalid  [3]),
   .S03_AXI_RREADY   (dram_axi_rready  [3]),
    //
    .M00_AXI_ACLK     (ddr3_axi_clk        ),
    .M00_AXI_ARESETN  (~ddr3_axi_rst       ),
    .M00_AXI_AWID     (ddr3_axi_awid       ),
    .M00_AXI_AWADDR   (ddr3_axi_awaddr     ),
    .M00_AXI_AWLEN    (ddr3_axi_awlen      ),
    .M00_AXI_AWSIZE   (ddr3_axi_awsize     ),
    .M00_AXI_AWBURST  (ddr3_axi_awburst    ),
    .M00_AXI_AWLOCK   (ddr3_axi_awlock     ),
    .M00_AXI_AWCACHE  (ddr3_axi_awcache    ),
    .M00_AXI_AWPROT   (ddr3_axi_awprot     ),
    .M00_AXI_AWQOS    (ddr3_axi_awqos      ),
    .M00_AXI_AWREGION (                    ),
    .M00_AXI_AWVALID  (ddr3_axi_awvalid    ),
    .M00_AXI_AWREADY  (ddr3_axi_awready    ),
    .M00_AXI_WDATA    (ddr3_axi_wdata      ),
    .M00_AXI_WSTRB    (ddr3_axi_wstrb      ),
    .M00_AXI_WLAST    (ddr3_axi_wlast      ),
    .M00_AXI_WVALID   (ddr3_axi_wvalid     ),
    .M00_AXI_WREADY   (ddr3_axi_wready     ),
    .M00_AXI_BID      (ddr3_axi_bid        ),
    .M00_AXI_BRESP    (ddr3_axi_bresp      ),
    .M00_AXI_BVALID   (ddr3_axi_bvalid     ),
    .M00_AXI_BREADY   (ddr3_axi_bready     ),
    .M00_AXI_ARID     (ddr3_axi_arid       ),
    .M00_AXI_ARADDR   (ddr3_axi_araddr     ),
    .M00_AXI_ARLEN    (ddr3_axi_arlen      ),
    .M00_AXI_ARSIZE   (ddr3_axi_arsize     ),
    .M00_AXI_ARBURST  (ddr3_axi_arburst    ),
    .M00_AXI_ARLOCK   (ddr3_axi_arlock     ),
    .M00_AXI_ARCACHE  (ddr3_axi_arcache    ),
    .M00_AXI_ARPROT   (ddr3_axi_arprot     ),
    .M00_AXI_ARQOS    (ddr3_axi_arqos      ),
    .M00_AXI_ARREGION (                    ),
    .M00_AXI_ARVALID  (ddr3_axi_arvalid    ),
    .M00_AXI_ARREADY  (ddr3_axi_arready    ),
    .M00_AXI_RID      (ddr3_axi_rid        ),
    .M00_AXI_RDATA    (ddr3_axi_rdata      ),
    .M00_AXI_RRESP    (ddr3_axi_rresp      ),
    .M00_AXI_RLAST    (ddr3_axi_rlast      ),
    .M00_AXI_RVALID   (ddr3_axi_rvalid     ),
    .M00_AXI_RREADY   (ddr3_axi_rready     )
  );


  /////////////////////////////////////////////////////////////////////////////
  //
  // DMA Transport Adapter
  //
  /////////////////////////////////////////////////////////////////////////////
  wire [63:0] dmao_tdata;
  wire        dmao_tlast;
  wire        dmao_tvalid;
  wire        dmao_tready;

  wire [63:0] dmai_tdata;
  wire        dmai_tlast;
  wire        dmai_tvalid;
  wire        dmai_tready;

  liberio_chdr64_adapter #(
    .DMA_ID_WIDTH                    (4)
  ) dma_xport_adapter (
    .clk                             (bus_clk),
    .rst                             (bus_rst),
    .device_id                       (device_id),
    // From DMA engine to core
    .s_dma_tdata                     (s_dma_tdata),
    .s_dma_tuser                     (s_dma_tuser),
    .s_dma_tlast                     (s_dma_tlast),
    .s_dma_tvalid                    (s_dma_tvalid),
    .s_dma_tready                    (s_dma_tready),
    // From core to DMA engine
    .m_dma_tdata                     (m_dma_tdata),
    .m_dma_tuser                     (m_dma_tdest),
    .m_dma_tlast                     (m_dma_tlast),
    .m_dma_tvalid                    (m_dma_tvalid),
    .m_dma_tready                    (m_dma_tready),
    // CHDR buses
    .s_chdr_tdata                    (dmao_tdata),
    .s_chdr_tlast                    (dmao_tlast),
    .s_chdr_tvalid                   (dmao_tvalid),
    .s_chdr_tready                   (dmao_tready),
    .m_chdr_tdata                    (dmai_tdata),
    .m_chdr_tlast                    (dmai_tlast),
    .m_chdr_tvalid                   (dmai_tvalid),
    .m_chdr_tready                   (dmai_tready)
  );

  /////////////////////////////////////////////////////////////////////////////
  //
  // Radio Daughter board and Front End Control
  //
  /////////////////////////////////////////////////////////////////////////////

  // Radio Daughter board GPIO
  wire [DB_GPIO_WIDTH-1:0] db_gpio_in[0:NUM_CHANNELS-1];
  wire [DB_GPIO_WIDTH-1:0] db_gpio_out[0:NUM_CHANNELS-1];
  wire [DB_GPIO_WIDTH-1:0] db_gpio_ddr[0:NUM_CHANNELS-1];
  wire [DB_GPIO_WIDTH-1:0] db_gpio_fab[0:NUM_CHANNELS-1];
  wire [31:0] radio_gpio_out[0:NUM_CHANNELS-1];
  wire [31:0] radio_gpio_ddr[0:NUM_CHANNELS-1];
  wire [31:0] radio_gpio_in[0:NUM_CHANNELS-1];
  wire [31:0] leds[0:NUM_CHANNELS-1];

  // Daughter board I/O
  wire        rx_running[0:NUM_CHANNELS-1], tx_running[0:NUM_CHANNELS-1];
  wire [31:0] rx_int[0:NUM_CHANNELS-1], rx_data[0:NUM_CHANNELS-1], tx_int[0:NUM_CHANNELS-1], tx_data[0:NUM_CHANNELS-1];
  //wire        rx_stb[0:NUM_CHANNELS-1], tx_stb[0:NUM_CHANNELS-1];
  wire        db_fe_set_stb[0:NUM_CHANNELS-1];
  wire [7:0]  db_fe_set_addr[0:NUM_CHANNELS-1];
  wire [31:0] db_fe_set_data[0:NUM_CHANNELS-1];
  wire        db_fe_rb_stb[0:NUM_CHANNELS-1];
  wire [7:0]  db_fe_rb_addr[0:NUM_CHANNELS-1];
  wire [63:0] db_fe_rb_data[0:NUM_CHANNELS-1];

  wire [NUM_RADIOS-1:0] sync_out;

  genvar i;
  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin
      assign rx_atr[i] = rx_running[i];
      assign tx_atr[i] = tx_running[i];
    end
  endgenerate


  //------------------------------------
  // Daughterboard Control
  // -----------------------------------

  localparam [7:0] SR_DB_BASE = 8'd160;
  localparam [7:0] RB_DB_BASE = 8'd16;

  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin: gen_db_control
      db_control #(
        .USE_SPI_CLK(0),
        .SR_BASE(SR_DB_BASE),
        .RB_BASE(RB_DB_BASE)
      ) db_control_i (
        .clk(radio_clk), .reset(radio_rst),
        .set_stb(db_fe_set_stb[i]), .set_addr(db_fe_set_addr[i]), .set_data(db_fe_set_data[i]),
        .rb_stb(db_fe_rb_stb[i]), .rb_addr(db_fe_rb_addr[i]), .rb_data(db_fe_rb_data[i]),
        .run_rx(rx_running[i]), .run_tx(tx_running[i]),
        .misc_ins(32'h0), .misc_outs(),
        .fp_gpio_in(radio_gpio_in[i]), .fp_gpio_out(radio_gpio_out[i]), .fp_gpio_ddr(radio_gpio_ddr[i]), .fp_gpio_fab(32'h0),
        .db_gpio_in(db_gpio_in[i]), .db_gpio_out(db_gpio_out[i]), .db_gpio_ddr(db_gpio_ddr[i]), .db_gpio_fab(),
        .leds(leds[i]),
        .spi_clk(1'b0), .spi_rst(1'b0), .sen(), .sclk(), .mosi(), .miso(1'b0)
      );
    end
  endgenerate

  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin: gen_gpio_control
      // Radio Data
      assign rx_data[i] = rx[32*i+31:32*i];
      assign tx[32*i+31:32*i] = tx_data[i];
      // GPIO
      assign db_gpio_out_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH] = db_gpio_out[i];
      assign db_gpio_ddr_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH] = db_gpio_ddr[i];
      assign db_gpio_in[i] = db_gpio_in_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH];
      assign db_gpio_fab[i] = db_gpio_fab_flat[DB_GPIO_WIDTH*i +: DB_GPIO_WIDTH];
      // LEDs
      assign leds_flat[32*i+31:32*i] = leds[i];
    end
  endgenerate

  /////////////////////////////////////////////////////////////////////////////
  //
  // Front-panel GPIO
  //
  /////////////////////////////////////////////////////////////////////////////

  wire [FP_GPIO_WIDTH-1:0] radio_gpio_in_sync;
  wire [FP_GPIO_WIDTH-1:0] radio_gpio_src_out;
  reg  [FP_GPIO_WIDTH-1:0] radio_gpio_src_out_reg;
  wire [FP_GPIO_WIDTH-1:0] radio_gpio_src_ddr;
  reg  [FP_GPIO_WIDTH-1:0] radio_gpio_src_ddr_reg = ~0;

  // Double-synchronize the inputs to the PS
  synchronizer #(
    .INITIAL_VAL(1'b0), .WIDTH(FP_GPIO_WIDTH)
    ) ps_gpio_in_sync_i (
    .clk(bus_clk), .rst(1'b0), .in(fp_gpio_in), .out(ps_gpio_in)
  );

  // Double-synchronize the inputs to the radio
  synchronizer #(
    .INITIAL_VAL(1'b0), .WIDTH(FP_GPIO_WIDTH)
    ) radio_gpio_in_sync_i (
    .clk(radio_clk), .rst(1'b0), .in(fp_gpio_in), .out(radio_gpio_in_sync)
  );

  // Map the double-synchronized inputs to all radio channels
  generate
    for (i=0; i<NUM_CHANNELS; i=i+1) begin: gen_fp_gpio_in_sync
      assign radio_gpio_in[i][FP_GPIO_WIDTH-1:0] = radio_gpio_in_sync;
    end
  endgenerate

  // For each of the FP GPIO bits, implement four control muxes
  generate
    for (i=0; i<FP_GPIO_WIDTH; i=i+1) begin: gpio_muxing_gen

      // 1) Select which radio drives the output
      assign radio_gpio_src_out[i] = radio_gpio_out[fp_gpio_src_reg[2*i+1:2*i]][i];
      always @ (posedge radio_clk) begin
        if (radio_rst) begin
          radio_gpio_src_out_reg <= 0;
        end else begin
          radio_gpio_src_out_reg <= radio_gpio_src_out;
        end
      end

      // 2) Select which radio drives the direction
      assign radio_gpio_src_ddr[i] = radio_gpio_ddr[fp_gpio_src_reg[2*i+1:2*i]][i];
      always @ (posedge radio_clk) begin
        if (radio_rst) begin
          radio_gpio_src_ddr_reg <= ~0;
        end else begin
          radio_gpio_src_ddr_reg <= radio_gpio_src_ddr;
        end
      end

      // 3) Select if the radio or the ps drives the output
      //
      // The following implements a 2:1 mux in a LUT explicitly to avoid
      // glitches that can be introduced by unexpected Vivado synthesis.
      //
      (* dont_touch = "TRUE" *) LUT3 #(
        .INIT(8'hCA) // Specify LUT Contents. O = ~I2&I0 | I2&I1
      ) mux_out_i (
        .O(fp_gpio_out[i]),             // LUT general output. Mux output
        .I0(radio_gpio_src_out_reg[i]), // LUT input. Input 1
        .I1(ps_gpio_out[i]),            // LUT input. Input 2
        .I2(fp_gpio_master_reg[i])      // LUT input. Select bit
      );

      // 4) Select if the radio or the PS drives the direction
      //
      (* dont_touch = "TRUE" *) LUT3 #(
        .INIT(8'hC5) // Specify LUT Contents. O = ~I2&I0 | I2&~I1
      ) mux_ddr_i (
        .O(fp_gpio_tri[i]),             // LUT general output. Mux output
        .I0(radio_gpio_src_ddr_reg[i]), // LUT input. Input 1
        .I1(ps_gpio_tri[i]),            // LUT input. Input 2
        .I2(fp_gpio_master_reg[i])      // LUT input. Select bit
      );

    end
  endgenerate

  // Regport Mux for response
  regport_resp_mux #(
    .WIDTH      (32),
    .NUM_SLAVES (2)
  ) reg_resp_mux_i (
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp({reg_rd_resp_tk, reg_rd_resp_glob}),
    .sla_rd_data({reg_rd_data_tk, reg_rd_data_glob}),
    .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
  );

   // Timekeeper
   wire [63:0] radio_time;

   timekeeper #(
     .BASE_ADDR      (REG_BASE_TIMEKEEPER),
     .TIME_INCREMENT (1'b1)
   ) timekeeper_i (
     .tb_clk                (radio_clk),
     .tb_rst                (radio_rst),
     .s_ctrlport_clk        (bus_clk),
     .s_ctrlport_req_wr     (reg_wr_req),
     .s_ctrlport_req_rd     (reg_rd_req),
     .s_ctrlport_req_addr   (reg_wr_req ? reg_wr_addr: reg_rd_addr),
     .s_ctrlport_req_data   (reg_wr_data),
     .s_ctrlport_resp_ack   (reg_rd_resp_tk),
     .s_ctrlport_resp_data  (reg_rd_data_tk),
     .sample_rx_stb         (rx_stb[0]),
     .pps                   (pps_radioclk),
     .tb_timestamp          (radio_time),
     .tb_timestamp_last_pps (),
     .tb_period_ns_q32      ()
   );


  /////////////////////////////////////////////////////////////////////////////
  //
  // RFNoC Image Core
  //
  /////////////////////////////////////////////////////////////////////////////

  // Unused memory AXI ports
  for (i = 0; i < NUM_DRAM_FIFOS; i = i+1) begin : gen_unused_ram_signals
    assign dram_axi_buser[i] = 4'b0;
    assign dram_axi_ruser[i] = 4'b0;
  end

  rfnoc_image_core #(
    .PROTOVER(RFNOC_PROTOVER)
  ) rfnoc_sandbox_i (
    .chdr_aclk               (bus_clk    ),
    .ctrl_aclk               (clk40      ),
    .core_arst               (bus_rst    ),
    .device_id               (device_id  ),
    .radio_clk               (radio_clk  ),
    .dram_clk                (ddr3_dma_clk),
    .m_ctrlport_req_wr       (m_ctrlport_req_wr      ),
    .m_ctrlport_req_rd       (m_ctrlport_req_rd      ),
    .m_ctrlport_req_addr     (m_ctrlport_req_addr    ),
    .m_ctrlport_req_data     (m_ctrlport_req_data    ),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (m_ctrlport_req_has_time),
    .m_ctrlport_req_time     (m_ctrlport_req_time    ),
    .m_ctrlport_resp_ack     (m_ctrlport_resp_ack    ),
    .m_ctrlport_resp_status  (2'b0),
    .m_ctrlport_resp_data    (m_ctrlport_resp_data   ),
    .axi_rst                 (ddr3_dma_rst),
    .m_axi_awid              ({dram_axi_awid    [1], dram_axi_awid    [0]}),
    .m_axi_awaddr            ({dram_axi_awaddr  [1], dram_axi_awaddr  [0]}),
    .m_axi_awlen             ({dram_axi_awlen   [1], dram_axi_awlen   [0]}),
    .m_axi_awsize            ({dram_axi_awsize  [1], dram_axi_awsize  [0]}),
    .m_axi_awburst           ({dram_axi_awburst [1], dram_axi_awburst [0]}),
    .m_axi_awlock            ({dram_axi_awlock  [1], dram_axi_awlock  [0]}),
    .m_axi_awcache           ({dram_axi_awcache [1], dram_axi_awcache [0]}),
    .m_axi_awprot            ({dram_axi_awprot  [1], dram_axi_awprot  [0]}),
    .m_axi_awqos             ({dram_axi_awqos   [1], dram_axi_awqos   [0]}),
    .m_axi_awregion          ({dram_axi_awregion[1], dram_axi_awregion[0]}),
    .m_axi_awuser            ({dram_axi_awuser  [1], dram_axi_awuser  [0]}),
    .m_axi_awvalid           ({dram_axi_awvalid [1], dram_axi_awvalid [0]}),
    .m_axi_awready           ({dram_axi_awready [1], dram_axi_awready [0]}),
    .m_axi_wdata             ({dram_axi_wdata   [1], dram_axi_wdata   [0]}),
    .m_axi_wstrb             ({dram_axi_wstrb   [1], dram_axi_wstrb   [0]}),
    .m_axi_wlast             ({dram_axi_wlast   [1], dram_axi_wlast   [0]}),
    .m_axi_wuser             ({dram_axi_wuser   [1], dram_axi_wuser   [0]}),
    .m_axi_wvalid            ({dram_axi_wvalid  [1], dram_axi_wvalid  [0]}),
    .m_axi_wready            ({dram_axi_wready  [1], dram_axi_wready  [0]}),
    .m_axi_bid               ({dram_axi_bid     [1], dram_axi_bid     [0]}),
    .m_axi_bresp             ({dram_axi_bresp   [1], dram_axi_bresp   [0]}),
    .m_axi_buser             ({dram_axi_buser   [1], dram_axi_buser   [0]}),
    .m_axi_bvalid            ({dram_axi_bvalid  [1], dram_axi_bvalid  [0]}),
    .m_axi_bready            ({dram_axi_bready  [1], dram_axi_bready  [0]}),
    .m_axi_arid              ({dram_axi_arid    [1], dram_axi_arid    [0]}),
    .m_axi_araddr            ({dram_axi_araddr  [1], dram_axi_araddr  [0]}),
    .m_axi_arlen             ({dram_axi_arlen   [1], dram_axi_arlen   [0]}),
    .m_axi_arsize            ({dram_axi_arsize  [1], dram_axi_arsize  [0]}),
    .m_axi_arburst           ({dram_axi_arburst [1], dram_axi_arburst [0]}),
    .m_axi_arlock            ({dram_axi_arlock  [1], dram_axi_arlock  [0]}),
    .m_axi_arcache           ({dram_axi_arcache [1], dram_axi_arcache [0]}),
    .m_axi_arprot            ({dram_axi_arprot  [1], dram_axi_arprot  [0]}),
    .m_axi_arqos             ({dram_axi_arqos   [1], dram_axi_arqos   [0]}),
    .m_axi_arregion          ({dram_axi_arregion[1], dram_axi_arregion[0]}),
    .m_axi_aruser            ({dram_axi_aruser  [1], dram_axi_aruser  [0]}),
    .m_axi_arvalid           ({dram_axi_arvalid [1], dram_axi_arvalid [0]}),
    .m_axi_arready           ({dram_axi_arready [1], dram_axi_arready [0]}),
    .m_axi_rid               ({dram_axi_rid     [1], dram_axi_rid     [0]}),
    .m_axi_rdata             ({dram_axi_rdata   [1], dram_axi_rdata   [0]}),
    .m_axi_rresp             ({dram_axi_rresp   [1], dram_axi_rresp   [0]}),
    .m_axi_rlast             ({dram_axi_rlast   [1], dram_axi_rlast   [0]}),
    .m_axi_ruser             ({dram_axi_ruser   [1], dram_axi_ruser   [0]}),
    .m_axi_rvalid            ({dram_axi_rvalid  [1], dram_axi_rvalid  [0]}),
    .m_axi_rready            ({dram_axi_rready  [1], dram_axi_rready  [0]}),
    .radio_time              (radio_time             ),
    .radio_rx_stb            ({rx_stb[1],     rx_stb[0]    }),
    .radio_rx_data           ({rx_data[1],    rx_data[0]   }),
    .radio_rx_running        ({rx_running[1], rx_running[0]}),
    .radio_tx_stb            ({tx_stb[1],     tx_stb[0]    }),
    .radio_tx_data           ({tx_data[1],    tx_data[0]   }),
    .radio_tx_running        ({tx_running[1], tx_running[0]}),
    .s_eth_tdata             (e2v_tdata ),
    .s_eth_tlast             (e2v_tlast ),
    .s_eth_tvalid            (e2v_tvalid),
    .s_eth_tready            (e2v_tready),
    .m_eth_tdata             (v2e_tdata ),
    .m_eth_tlast             (v2e_tlast ),
    .m_eth_tvalid            (v2e_tvalid),
    .m_eth_tready            (v2e_tready),
    .s_dma_tdata             (dmai_tdata),
    .s_dma_tlast             (dmai_tlast),
    .s_dma_tvalid            (dmai_tvalid),
    .s_dma_tready            (dmai_tready),
    .m_dma_tdata             (dmao_tdata),
    .m_dma_tlast             (dmao_tlast),
    .m_dma_tvalid            (dmao_tvalid),
    .m_dma_tready            (dmao_tready)
  );

  //---------------------------------------------------------------------------
  // Convert Control Port to Settings Bus
  //---------------------------------------------------------------------------

  ctrlport_to_settings_bus # (
    .NUM_PORTS (2),
    .USE_TIME  (1)
  ) ctrlport_to_settings_bus_i (
    .ctrlport_clk             (radio_clk),
    .ctrlport_rst             (radio_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr),
    .s_ctrlport_req_data      (m_ctrlport_req_data),
    .s_ctrlport_req_has_time  (m_ctrlport_req_has_time),
    .s_ctrlport_req_time      (m_ctrlport_req_time),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data),
    .set_data                 ({db_fe_set_data[1], db_fe_set_data[0]}),
    .set_addr                 ({db_fe_set_addr[1], db_fe_set_addr[0]}),
    .set_stb                  ({db_fe_set_stb[1],  db_fe_set_stb[0] }),
    .set_time                 (),
    .set_has_time             (),
    .rb_stb                   ({db_fe_rb_stb[1],   db_fe_rb_stb[0]  }),
    .rb_addr                  ({db_fe_rb_addr[1],  db_fe_rb_addr[0] }),
    .rb_data                  ({db_fe_rb_data[1],  db_fe_rb_data[0] }),
    .timestamp                (radio_time)
  );

endmodule //e320_core
`default_nettype wire

