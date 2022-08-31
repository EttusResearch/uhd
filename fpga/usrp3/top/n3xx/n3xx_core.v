/////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: n3xx_core
// Description:
// - Motherboard Registers
// - Crossbar
// - Noc Block Radios
// - Noc Block Dram fifo
// - Radio Front End control
//
/////////////////////////////////////////////////////////////////////

module n3xx_core #(
  parameter REG_DWIDTH  = 32, // Width of the AXI4-Lite data bus (must be 32 or 64)
  parameter REG_AWIDTH  = 32,  // Width of the address bus
  parameter BUS_CLK_RATE = 200000000, // BUS_CLK rate
  parameter CHANNEL_WIDTH = 32,
  parameter NUM_CHANNELS_PER_RADIO = 1,
  parameter NUM_CHANNELS = 4,
  parameter NUM_DBOARDS = 2,
  parameter NUM_SPI_PER_DBOARD = 8,
  parameter USE_CORRECTION = 0,
  parameter FP_GPIO_WIDTH = 12, // Front panel GPIO width
  parameter CHDR_W = 64,
  parameter BYTE_MTU = $clog2(8192),
  parameter RFNOC_PROTOVER = {8'd1, 8'd0}
)(
  // Clocks and resets
  input         radio_clk,
  input         radio_rst,
  input         bus_clk,
  input         bus_rst,
  input         ddr3_dma_clk,
  input         clk40,

  // Clocking and PPS Controls/Indicators
  input            pps,
  output reg[3:0]  pps_select = 4'h1,
  output reg       pps_out_enb,
  output reg[1:0]  pps_select_sfp = 2'b0,
  output reg       ref_clk_reset,
  output reg       meas_clk_reset,
  input            ref_clk_locked,
  input            meas_clk_locked,
  output reg       enable_ref_clk_async,

  // Motherboard Registers: AXI lite interface
  input                    s_axi_aclk,
  input                    s_axi_aresetn,
  input [REG_AWIDTH-1:0]   s_axi_awaddr,
  input                    s_axi_awvalid,
  output                   s_axi_awready,

  input [REG_DWIDTH-1:0]   s_axi_wdata,
  input [REG_DWIDTH/8-1:0] s_axi_wstrb,
  input                    s_axi_wvalid,
  output                   s_axi_wready,

  output [1:0]             s_axi_bresp,
  output                   s_axi_bvalid,
  input                    s_axi_bready,

  input [REG_AWIDTH-1:0]   s_axi_araddr,
  input                    s_axi_arvalid,
  output                   s_axi_arready,

  output [REG_DWIDTH-1:0]  s_axi_rdata,
  output [1:0]             s_axi_rresp,
  output                   s_axi_rvalid,
  input                    s_axi_rready,

  // PS GPIO source
  input  [FP_GPIO_WIDTH-1:0]  ps_gpio_out,
  input  [FP_GPIO_WIDTH-1:0]  ps_gpio_tri,
  output [FP_GPIO_WIDTH-1:0]  ps_gpio_in,

  // Front Panel GPIO
  inout  [FP_GPIO_WIDTH-1:0] fp_gpio_inout,

  // Radio GPIO control for DSA
  output [16*NUM_CHANNELS-1:0] db_gpio_out_flat,
  output [16*NUM_CHANNELS-1:0] db_gpio_ddr_flat,
  input  [16*NUM_CHANNELS-1:0] db_gpio_in_flat,
  input  [16*NUM_CHANNELS-1:0] db_gpio_fab_flat,

  // Radio ATR
  output [NUM_CHANNELS-1:0] rx_atr,
  output [NUM_CHANNELS-1:0] tx_atr,

  // Radio Data
  input  [NUM_CHANNELS-1:0]    rx_stb,
  input  [NUM_CHANNELS-1:0]    tx_stb,
  input  [CHANNEL_WIDTH*NUM_CHANNELS-1:0] rx,
  output [CHANNEL_WIDTH*NUM_CHANNELS-1:0] tx,

  // CPLD
  output [NUM_SPI_PER_DBOARD*NUM_DBOARDS-1:0] sen_flat,
  output [NUM_DBOARDS-1:0]   sclk_flat,
  output [NUM_DBOARDS-1:0]   mosi_flat,
  input  [NUM_DBOARDS-1:0]   miso_flat,

  // DMA xport adapter to PS
  input wire  [63:0] s_dma_tdata,
  input wire         s_dma_tlast,
  output wire        s_dma_tready,
  input wire         s_dma_tvalid,

  output wire [63:0] m_dma_tdata,
  output wire        m_dma_tlast,
  input wire         m_dma_tready,
  output wire        m_dma_tvalid,

  // AXI4 (256b@200MHz) interface to DDR3 controller
  input          ddr3_axi_clk,
  input          ddr3_axi_rst,
  input          ddr3_running,
  // Write Address Ports
  output [3:0]   ddr3_axi_awid,
  output [31:0]  ddr3_axi_awaddr,
  output [7:0]   ddr3_axi_awlen,
  output [2:0]   ddr3_axi_awsize,
  output [1:0]   ddr3_axi_awburst,
  output [0:0]   ddr3_axi_awlock,
  output [3:0]   ddr3_axi_awcache,
  output [2:0]   ddr3_axi_awprot,
  output [3:0]   ddr3_axi_awqos,
  output         ddr3_axi_awvalid,
  input          ddr3_axi_awready,
  // Write Data Ports
  output [255:0] ddr3_axi_wdata,
  output [31:0]  ddr3_axi_wstrb,
  output         ddr3_axi_wlast,
  output         ddr3_axi_wvalid,
  input          ddr3_axi_wready,
  // Write Response Ports
  output         ddr3_axi_bready,
  input [3:0]    ddr3_axi_bid,
  input [1:0]    ddr3_axi_bresp,
  input          ddr3_axi_bvalid,
  // Read Address Ports
  output [3:0]   ddr3_axi_arid,
  output [31:0]  ddr3_axi_araddr,
  output [7:0]   ddr3_axi_arlen,
  output [2:0]   ddr3_axi_arsize,
  output [1:0]   ddr3_axi_arburst,
  output [0:0]   ddr3_axi_arlock,
  output [3:0]   ddr3_axi_arcache,
  output [2:0]   ddr3_axi_arprot,
  output [3:0]   ddr3_axi_arqos,
  output         ddr3_axi_arvalid,
  input          ddr3_axi_arready,
  // Read Data Ports
  output         ddr3_axi_rready,
  input [3:0]    ddr3_axi_rid,
  input [255:0]  ddr3_axi_rdata,
  input [1:0]    ddr3_axi_rresp,
  input          ddr3_axi_rlast,
  input          ddr3_axi_rvalid,

  // v2e (vita to ethernet) and e2v (eth to vita)
  output [63:0] v2e0_tdata,
  output        v2e0_tvalid,
  output        v2e0_tlast,
  input         v2e0_tready,

  output [63:0] v2e1_tdata,
  output        v2e1_tlast,
  output        v2e1_tvalid,
  input         v2e1_tready,

  input  [63:0] e2v0_tdata,
  input         e2v0_tlast,
  input         e2v0_tvalid,
  output        e2v0_tready,

  input  [63:0] e2v1_tdata,
  input         e2v1_tlast,
  input         e2v1_tvalid,
  output        e2v1_tready,

  // RegPort interface to NPIO
  output                  reg_wr_req_npio,
  output [REG_AWIDTH-1:0] reg_wr_addr_npio,
  output [REG_DWIDTH-1:0] reg_wr_data_npio,
  output                  reg_rd_req_npio,
  output [REG_AWIDTH-1:0] reg_rd_addr_npio,
  input                   reg_rd_resp_npio,
  input  [REG_DWIDTH-1:0] reg_rd_data_npio,

  // Misc
  input  [31:0]   build_datestamp,
  input  [31:0]   xadc_readback,
  input  [63:0]   sfp_ports_info,
  output reg [15:0] device_id
);
  `include "../../lib/rfnoc/core/ctrlport.vh"

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

  localparam [15:0] COMPAT_MAJOR = 16'd8;
  localparam [15:0] COMPAT_MINOR = 16'd1;
  /////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////
  // Motherboard Registers
  /////////////////////////////////////////////////////////////////////////////////

  // Register base
  localparam [CTRLPORT_ADDR_W-1:0] REG_BASE_MISC       = 20'h0;
  localparam [CTRLPORT_ADDR_W-1:0] REG_BASE_NPIO       = 20'h200;
  localparam [CTRLPORT_ADDR_W-1:0] REG_BASE_TIMEKEEPER = 20'h1000;

  // Register region sizes. Give each register region a 256-byte window.
  localparam REG_GLOB_ADDR_W       = 8;
  localparam REG_NPIO_ADDR_W       = 8;
  localparam REG_TIMEKEEPER_ADDR_W = 8;

  // Misc Motherboard Registers
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
  localparam REG_SFP_PORT0_INFO    = REG_BASE_MISC + 14'h28;
  localparam REG_SFP_PORT1_INFO    = REG_BASE_MISC + 14'h2C;
  localparam REG_FP_GPIO_MASTER    = REG_BASE_MISC + 14'h30;
  localparam REG_FP_GPIO_RADIO_SRC = REG_BASE_MISC + 14'h34;
  localparam REG_NUM_TIMEKEEPERS   = REG_BASE_MISC + 14'h48;

  localparam NUM_TIMEKEEPERS = 1;

  wire                 m_ctrlport_req_wr_radio0;
  wire                 m_ctrlport_req_rd_radio0;
  wire [19:0]          m_ctrlport_req_addr_radio0;
  wire [31:0]          m_ctrlport_req_data_radio0;
  wire [3:0]           m_ctrlport_req_byte_en_radio0;
  wire                 m_ctrlport_req_has_time_radio0;
  wire [63:0]          m_ctrlport_req_time_radio0;
  wire                 m_ctrlport_resp_ack_radio0;
  wire [1:0]           m_ctrlport_resp_status_radio0;
  wire [31:0]          m_ctrlport_resp_data_radio0;
  `ifndef N300
    wire                 m_ctrlport_req_wr_radio1;
    wire                 m_ctrlport_req_rd_radio1;
    wire [19:0]          m_ctrlport_req_addr_radio1;
    wire [31:0]          m_ctrlport_req_data_radio1;
    wire [3:0]           m_ctrlport_req_byte_en_radio1;
    wire                 m_ctrlport_req_has_time_radio1;
    wire [63:0]          m_ctrlport_req_time_radio1;
    wire                 m_ctrlport_resp_ack_radio1;
    wire [1:0]           m_ctrlport_resp_status_radio1;
    wire [31:0]          m_ctrlport_resp_data_radio1;
  `endif

  reg [31:0] scratch_reg = 32'b0;
  reg [31:0] bus_counter = 32'h0;
  reg [31:0] fp_gpio_master_reg = 32'h0;
  reg [31:0] fp_gpio_src_reg = 32'h0;

  always @(posedge bus_clk) begin
     if (bus_rst)
        bus_counter <= 32'd0;
     else
        bus_counter <= bus_counter + 32'd1;
  end


  /////////////////////////////////////////////////////////////////////////////
  //
  // Bus Bridge
  //
  /////////////////////////////////////////////////////////////////////////////

  // Specify timeout for CtrlPort transactions that don't complete.
  localparam CTRLPORT_TIMEOUT = 20;

  wire                       cp_req_wr_aclk;
  wire                       cp_req_rd_aclk;
  wire [CTRLPORT_ADDR_W-1:0] cp_req_addr_aclk;
  wire [CTRLPORT_DATA_W-1:0] cp_req_data_aclk;
  wire                       cp_resp_ack_aclk;
  wire [ CTRLPORT_STS_W-1:0] cp_resp_status_aclk;
  wire [CTRLPORT_DATA_W-1:0] cp_resp_data_aclk;

  // Convert the AXI4-Lite transactions to CtrlPort
  axil_ctrlport_master #(
    .TIMEOUT         (CTRLPORT_TIMEOUT),
    .AXI_AWIDTH      (REG_AWIDTH),
    .CTRLPORT_AWIDTH (CTRLPORT_ADDR_W)
  ) axil_ctrlport_master_i (
    .s_axi_aclk                (s_axi_aclk),
    .s_axi_aresetn             (s_axi_aresetn),
    .s_axi_awaddr              (s_axi_awaddr),
    .s_axi_awvalid             (s_axi_awvalid),
    .s_axi_awready             (s_axi_awready),
    .s_axi_wdata               (s_axi_wdata),
    .s_axi_wstrb               (s_axi_wstrb),
    .s_axi_wvalid              (s_axi_wvalid),
    .s_axi_wready              (s_axi_wready),
    .s_axi_bresp               (s_axi_bresp),
    .s_axi_bvalid              (s_axi_bvalid),
    .s_axi_bready              (s_axi_bready),
    .s_axi_araddr              (s_axi_araddr),
    .s_axi_arvalid             (s_axi_arvalid),
    .s_axi_arready             (s_axi_arready),
    .s_axi_rdata               (s_axi_rdata),
    .s_axi_rresp               (s_axi_rresp),
    .s_axi_rvalid              (s_axi_rvalid),
    .s_axi_rready              (s_axi_rready),
    .m_ctrlport_req_wr         (cp_req_wr_aclk),
    .m_ctrlport_req_rd         (cp_req_rd_aclk),
    .m_ctrlport_req_addr       (cp_req_addr_aclk),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (cp_req_data_aclk),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (cp_resp_ack_aclk),
    .m_ctrlport_resp_status    (cp_resp_status_aclk),
    .m_ctrlport_resp_data      (cp_resp_data_aclk)
  );

  wire                       cp_req_wr;
  wire                       cp_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] cp_req_addr;
  wire [CTRLPORT_DATA_W-1:0] cp_req_data;
  wire                       cp_resp_ack;
  wire [ CTRLPORT_STS_W-1:0] cp_resp_status;
  wire [CTRLPORT_DATA_W-1:0] cp_resp_data;

  // Cross transactions from s_axi_clk to bus_clk domain
  ctrlport_clk_cross ctrlport_clk_cross_i (
    .rst                       (~s_axi_aresetn),
    .s_ctrlport_clk            (s_axi_aclk),
    .s_ctrlport_req_wr         (cp_req_wr_aclk),
    .s_ctrlport_req_rd         (cp_req_rd_aclk),
    .s_ctrlport_req_addr       (cp_req_addr_aclk),
    .s_ctrlport_req_portid     ({CTRLPORT_PORTID_W{1'b0}}),
    .s_ctrlport_req_rem_epid   ({CTRLPORT_REM_EPID_W{1'b0}}),
    .s_ctrlport_req_rem_portid ({CTRLPORT_PORTID_W{1'b0}}),
    .s_ctrlport_req_data       (cp_req_data_aclk),
    .s_ctrlport_req_byte_en    ({CTRLPORT_BYTE_EN_W{1'b1}}),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       ({CTRLPORT_TIME_W{1'b0}}),
    .s_ctrlport_resp_ack       (cp_resp_ack_aclk),
    .s_ctrlport_resp_status    (cp_resp_status_aclk),
    .s_ctrlport_resp_data      (cp_resp_data_aclk),
    .m_ctrlport_clk            (bus_clk),
    .m_ctrlport_req_wr         (cp_req_wr),
    .m_ctrlport_req_rd         (cp_req_rd),
    .m_ctrlport_req_addr       (cp_req_addr),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (cp_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (cp_resp_ack),
    .m_ctrlport_resp_status    (cp_resp_status),
    .m_ctrlport_resp_data      (cp_resp_data)
  );

  wire                       cp_glob_req_wr;
  wire                       cp_glob_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] cp_glob_req_addr;
  wire [CTRLPORT_DATA_W-1:0] cp_glob_req_data;
  reg                        cp_glob_resp_ack  = 1'b0;
  reg  [CTRLPORT_DATA_W-1:0] cp_glob_resp_data = 1'bX;

  wire                       cp_npio_req_wr;
  wire                       cp_npio_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] cp_npio_req_addr;
  wire [CTRLPORT_DATA_W-1:0] cp_npio_req_data;
  wire                       cp_npio_resp_ack;
  wire [CTRLPORT_DATA_W-1:0] cp_npio_resp_data;

  wire                       cp_tk_req_wr;
  wire                       cp_tk_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] cp_tk_req_addr;
  wire [CTRLPORT_DATA_W-1:0] cp_tk_req_data;
  wire                       cp_tk_resp_ack;
  wire [CTRLPORT_DATA_W-1:0] cp_tk_resp_data;

  // Split the CtrlPort for the global registers and the timekeeper registers
  ctrlport_decoder_param #(
    .NUM_SLAVES  (3),
    .PORT_BASE   ({   REG_BASE_TIMEKEEPER,   REG_BASE_NPIO,   REG_BASE_MISC }),
    .PORT_ADDR_W ({ REG_TIMEKEEPER_ADDR_W, REG_NPIO_ADDR_W, REG_GLOB_ADDR_W })
  ) ctrlport_decoder_param_i (
    .ctrlport_clk            (bus_clk),
    .ctrlport_rst            (bus_rst),
    .s_ctrlport_req_wr       (cp_req_wr),
    .s_ctrlport_req_rd       (cp_req_rd),
    .s_ctrlport_req_addr     (cp_req_addr),
    .s_ctrlport_req_data     (cp_req_data),
    .s_ctrlport_req_byte_en  ({CTRLPORT_BYTE_EN_W{1'b1}}),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     ({CTRLPORT_TIME_W{1'b0}}),
    .s_ctrlport_resp_ack     (cp_resp_ack),
    .s_ctrlport_resp_status  (cp_resp_status),
    .s_ctrlport_resp_data    (cp_resp_data),
    .m_ctrlport_req_wr       ({ cp_tk_req_wr,    cp_npio_req_wr,    cp_glob_req_wr }),
    .m_ctrlport_req_rd       ({ cp_tk_req_rd,    cp_npio_req_rd,    cp_glob_req_rd }),
    .m_ctrlport_req_addr     ({ cp_tk_req_addr,  cp_npio_req_addr,  cp_glob_req_addr }),
    .m_ctrlport_req_data     ({ cp_tk_req_data,  cp_npio_req_data,  cp_glob_req_data }),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({ cp_tk_resp_ack,  cp_npio_resp_ack,  cp_glob_resp_ack }),
    .m_ctrlport_resp_status  ({2{CTRL_STS_OKAY}}),
    .m_ctrlport_resp_data    ({ cp_tk_resp_data, cp_npio_resp_data, cp_glob_resp_data })
  );

  // Convert NPIO from CtrlPort to RegPort
  ctrlport_to_regport #(
    .REG_AWIDTH (REG_AWIDTH),
    .REG_DWIDTH (REG_DWIDTH)
  ) ctrlport_to_regport_i (
    .clk                  (bus_clk),
    .rst                  (bus_rst),
    .s_ctrlport_req_wr    (cp_npio_req_wr),
    .s_ctrlport_req_rd    (cp_npio_req_rd),
    .s_ctrlport_req_addr  (cp_npio_req_addr),
    .s_ctrlport_req_data  (cp_npio_req_data),
    .s_ctrlport_resp_ack  (cp_npio_resp_ack),
    .s_ctrlport_resp_data (cp_npio_resp_data),
    .reg_wr_req           (reg_wr_req_npio),
    .reg_wr_addr          (reg_wr_addr_npio),
    .reg_wr_data          (reg_wr_data_npio),
    .reg_rd_req           (reg_rd_req_npio),
    .reg_rd_addr          (reg_rd_addr_npio),
    .reg_rd_resp          (reg_rd_resp_npio),
    .reg_rd_data          (reg_rd_data_npio)
  );


  /////////////////////////////////////////////////////////////////////////////
  //
  // Global Registers
  //
  /////////////////////////////////////////////////////////////////////////////

  reg b_ref_clk_locked_ms;
  reg b_ref_clk_locked;
  reg b_meas_clk_locked_ms;
  reg b_meas_clk_locked;

  always @ (posedge bus_clk) begin
    if (bus_rst) begin
      cp_glob_resp_ack     <= 1'b0;
      cp_glob_resp_data    <= 'bX;
      scratch_reg          <= 32'h0;
      fp_gpio_master_reg   <= 32'h0;
      fp_gpio_src_reg      <= 32'h0;
      pps_select           <= 4'h1;
      pps_select_sfp       <= 2'h0;
      pps_out_enb          <= 1'b0;
      ref_clk_reset        <= 1'b0;
      meas_clk_reset       <= 1'b0;
      enable_ref_clk_async <= 1'b1;
      device_id            <= 16'h0;
      b_ref_clk_locked_ms  <= 1'b0;
      b_ref_clk_locked     <= 1'b0;
      b_meas_clk_locked_ms <= 1'b0;
      b_meas_clk_locked    <= 1'b0;
    end else begin
      cp_glob_resp_ack <= 1'b0;

      if (cp_glob_req_wr) begin
        cp_glob_resp_ack <= 1'b1;
        
        case (cp_glob_req_addr[REG_GLOB_ADDR_W-1:0])
          REG_DEVICE_ID:
            device_id <= cp_glob_req_data[15:0];

          REG_FP_GPIO_MASTER:
            fp_gpio_master_reg <= cp_glob_req_data;

          REG_FP_GPIO_RADIO_SRC:
            fp_gpio_src_reg <= cp_glob_req_data;

          REG_SCRATCH:
            scratch_reg <= cp_glob_req_data;

          REG_CLOCK_CTRL: begin
            pps_select           <= cp_glob_req_data[3:0];
            pps_out_enb          <= cp_glob_req_data[4];
            pps_select_sfp       <= cp_glob_req_data[6:5];
            ref_clk_reset        <= cp_glob_req_data[8];
            meas_clk_reset       <= cp_glob_req_data[12];
            // This bit is defined as "to disable, write '1' to bit 16" for backwards
            // compatibility.
            enable_ref_clk_async <= ~cp_glob_req_data[16];
          end
          default: begin
            // Don't acknowledge if the address doesn't match
            cp_glob_resp_ack <= 1'b0;
          end
        endcase
      end

      // double-sync the locked bits into the bus_clk domain before using them
      b_ref_clk_locked_ms  <= ref_clk_locked;
      b_ref_clk_locked     <= b_ref_clk_locked_ms;
      b_meas_clk_locked_ms <= meas_clk_locked;
      b_meas_clk_locked    <= b_meas_clk_locked_ms;

      if (cp_glob_req_rd) begin
        cp_glob_resp_data <= 0;  // Unused bits will read 0
        cp_glob_resp_ack  <= 1'b1;

        case (cp_glob_req_addr[REG_GLOB_ADDR_W-1:0])
          REG_DEVICE_ID:
            cp_glob_resp_data <= {16'd0, device_id};

          REG_RFNOC_INFO:
            cp_glob_resp_data <= {CHDR_W[15:0], RFNOC_PROTOVER[15:0]};

          REG_COMPAT_NUM:
            cp_glob_resp_data <= {COMPAT_MAJOR, COMPAT_MINOR};

          REG_DATESTAMP:
            cp_glob_resp_data <= build_datestamp;

          REG_GIT_HASH:
            `ifndef GIT_HASH
            `define GIT_HASH 32'h0BADC0DE
            `endif
            cp_glob_resp_data <= `GIT_HASH;

          REG_FP_GPIO_MASTER:
            cp_glob_resp_data <= fp_gpio_master_reg;

          REG_FP_GPIO_RADIO_SRC:
            cp_glob_resp_data <= fp_gpio_src_reg;

          REG_SCRATCH:
            cp_glob_resp_data <= scratch_reg;

          REG_CLOCK_CTRL: begin
            cp_glob_resp_data      <= 32'b0;
            cp_glob_resp_data[3:0] <= pps_select;
            cp_glob_resp_data[4]   <= pps_out_enb;
            cp_glob_resp_data[6:5] <= pps_select_sfp;
            cp_glob_resp_data[8]   <= ref_clk_reset;
            cp_glob_resp_data[9]   <= b_ref_clk_locked;
            cp_glob_resp_data[12]  <= meas_clk_reset;
            cp_glob_resp_data[13]  <= b_meas_clk_locked;
            cp_glob_resp_data[16]  <= ~enable_ref_clk_async;
          end

          REG_XADC_READBACK:
            cp_glob_resp_data <= xadc_readback;

          REG_BUS_CLK_RATE:
            cp_glob_resp_data <= BUS_CLK_RATE;

          REG_BUS_CLK_COUNT:
            cp_glob_resp_data <= bus_counter;

          REG_SFP_PORT0_INFO:
            cp_glob_resp_data <= sfp_ports_info[31:0];

          REG_SFP_PORT1_INFO:
            cp_glob_resp_data <= sfp_ports_info[63:32];

          REG_NUM_TIMEKEEPERS:
            cp_glob_resp_data <= NUM_TIMEKEEPERS;

          default: begin
            // Don't acknowledge if the address doesn't match
            cp_glob_resp_ack <= 1'b0;
          end
        endcase
      end
    end
  end


  /////////////////////////////////////////////////////////////////////
  //
  // AXI Interconnect
  //
  /////////////////////////////////////////////////////////////////////

  localparam NUM_DRAM_FIFOS = 4;
  localparam DRAM_FIFO_INPUT_BUFF_SIZE = 8'd13;

  wire ddr3_dma_rst;
  synchronizer #(
   .INITIAL_VAL(1'b1)
  ) ddr3_dma_rst_sync_i (
   .clk(ddr3_dma_clk), .rst(1'b0), .in(ddr3_axi_rst), .out(ddr3_dma_rst)
  );

  // AXI4 MM buses
  wire [0:0]  dram_axi_awid     [0:NUM_DRAM_FIFOS-1];
  wire [30:0] dram_axi_awaddr   [0:NUM_DRAM_FIFOS-1];
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
  wire [30:0] dram_axi_araddr   [0:NUM_DRAM_FIFOS-1];
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
   .S00_AXI_AWADDR   ({1,b0, dram_axi_awaddr[0]}),
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
   .S00_AXI_ARADDR   ({1,b0, dram_axi_araddr[0]}),
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
   .S01_AXI_AWADDR   ({1,b0, dram_axi_awaddr[1]}),
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
   .S01_AXI_ARADDR   ({1,b0, dram_axi_araddr[1]}),
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
   .S02_AXI_AWADDR   ({1,b0, dram_axi_awaddr[2]}),
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
   .S02_AXI_ARADDR   ({1,b0, dram_axi_araddr[2]}),
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
   .S03_AXI_AWADDR   ({1,b0, dram_axi_awaddr[3]}),
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
   .S03_AXI_ARADDR   ({1,b0, dram_axi_araddr[3]}),
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
  // Radios
  //
  /////////////////////////////////////////////////////////////////////////////


  wire [NUM_SPI_PER_DBOARD-1:0]  sen[0:NUM_CHANNELS-1];
  wire        sclk[0:NUM_CHANNELS-1], mosi[0:NUM_CHANNELS-1], miso[0:NUM_CHANNELS-1];
  // Data
  wire [CHANNEL_WIDTH-1:0] rx_int[0:NUM_CHANNELS-1], tx_int[0:NUM_CHANNELS-1];
  wire [CHANNEL_WIDTH-1:0] rx_data[0:NUM_CHANNELS-1], tx_data[0:NUM_CHANNELS-1];
  wire        db_fe_set_stb[0:NUM_CHANNELS-1];
  wire [7:0]  db_fe_set_addr[0:NUM_CHANNELS-1];
  wire [31:0] db_fe_set_data[0:NUM_CHANNELS-1];
  wire        db_fe_rb_stb[0:NUM_CHANNELS-1];
  wire [7:0]  db_fe_rb_addr[0:NUM_CHANNELS-1];
  wire [63:0] db_fe_rb_data[0:NUM_CHANNELS-1];
  wire        rx_running[0:NUM_CHANNELS-1], tx_running[0:NUM_CHANNELS-1];

  genvar i;
  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin: gen_gpio_control
      assign rx_atr[i] = rx_running[i];
      assign tx_atr[i] = tx_running[i];
    end
  endgenerate


  /////////////////////////////////////////////////////////////////////////////////
  //
  // TX/RX FrontEnd
  //
  /////////////////////////////////////////////////////////////////////////////////

  wire [15:0] db_gpio_in[0:NUM_CHANNELS-1];
  wire [15:0] db_gpio_out[0:NUM_CHANNELS-1];
  wire [15:0] db_gpio_ddr[0:NUM_CHANNELS-1];
  wire [15:0] db_gpio_fab[0:NUM_CHANNELS-1];

  wire [31:0] radio_gpio_out[0:NUM_CHANNELS-1];
  wire [31:0] radio_gpio_ddr[0:NUM_CHANNELS-1];
  wire [31:0] radio_gpio_in[0:NUM_CHANNELS-1];
  wire [FP_GPIO_WIDTH-1:0] radio_gpio_src_out;
  reg  [FP_GPIO_WIDTH-1:0] radio_gpio_src_out_reg;
  wire [FP_GPIO_WIDTH-1:0] radio_gpio_src_ddr;
  reg  [FP_GPIO_WIDTH-1:0] radio_gpio_src_ddr_reg;
  reg  [FP_GPIO_WIDTH-1:0] radio_gpio_src_in;
  wire [FP_GPIO_WIDTH-1:0] radio_gpio_sync;
  wire [FP_GPIO_WIDTH-1:0] fp_gpio_in_int;
  wire [FP_GPIO_WIDTH-1:0] fp_gpio_out_int;
  wire [FP_GPIO_WIDTH-1:0] fp_gpio_ddr_int;

  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin
      // Radio Data
      assign rx_int[i] = rx[CHANNEL_WIDTH*i +: CHANNEL_WIDTH];
      assign tx[CHANNEL_WIDTH*i +: CHANNEL_WIDTH] = tx_int[i];
      // GPIO
      assign db_gpio_out_flat[16*i+15:16*i] = db_gpio_out[i];
      assign db_gpio_ddr_flat[16*i+15:16*i] = db_gpio_ddr[i];
      assign db_gpio_in[i] = db_gpio_in_flat[16*i+15:16*i];
      assign db_gpio_fab[i] = db_gpio_fab_flat[16*i+15:16*i];
    end
  endgenerate

  generate if (NUM_CHANNELS_PER_RADIO == 1)
    begin
      for (i = 0; i < NUM_DBOARDS; i = i + 1) begin
        // SPI
        assign miso[i] = miso_flat[i];
        assign sclk_flat[i] = sclk[i];
        assign sen_flat[NUM_SPI_PER_DBOARD*i +: NUM_SPI_PER_DBOARD] = sen[i];
        assign mosi_flat[i] = mosi[i];
      end
    end else if (NUM_CHANNELS_PER_RADIO == 2)
    begin
      for (i = 0; i < NUM_DBOARDS; i = i + 1) begin
        // SPI
        assign miso[2*i] = miso_flat[i];
        assign sclk_flat[i] = sclk[2*i];
        assign sen_flat[NUM_SPI_PER_DBOARD*i +: NUM_SPI_PER_DBOARD] = sen[2*i];
        assign mosi_flat[i] = mosi[2*i];
      end
    end
  endgenerate

  generate
    for (i = 0; i < NUM_CHANNELS; i = i + 1) begin
      n3xx_db_fe_core #(
        .USE_CORRECTION(USE_CORRECTION),
        .NUM_SPI_SEN(NUM_SPI_PER_DBOARD),
        .WIDTH(CHANNEL_WIDTH)
      ) db_fe_core_i (
        .clk(radio_clk),
        .reset(radio_rst),
        .set_stb(db_fe_set_stb[i]),
        .set_addr(db_fe_set_addr[i]),
        .set_data(db_fe_set_data[i]),
        .rb_stb(db_fe_rb_stb[i]),
        .rb_addr(db_fe_rb_addr[i]),
        .rb_data(db_fe_rb_data[i]),
        .tx_stb(tx_stb[i]),
        .tx_data_in(tx_data[i]),
        .tx_data_out(tx_int[i]),
        .tx_running(tx_running[i]),
        .rx_stb(rx_stb[i]),
        .rx_data_in(rx_int[i]),
        .rx_data_out(rx_data[i]),
        .rx_running(rx_running[i]),
        .misc_ins(32'h0),
        .misc_outs(),
        .fp_gpio_in(radio_gpio_in[i]),
        .fp_gpio_out(radio_gpio_out[i]),
        .fp_gpio_ddr(radio_gpio_ddr[i]),
        .fp_gpio_fab(32'h0),
        .db_gpio_in(db_gpio_in[i]),
        .db_gpio_out(db_gpio_out[i]),
        .db_gpio_ddr(db_gpio_ddr[i]),
        .db_gpio_fab(db_gpio_fab[i]),
        .leds(),
        .spi_clk(radio_clk),
        .spi_rst(radio_rst),
        .sen(sen[i]),
        .sclk(sclk[i]),
        .mosi(mosi[i]),
        .miso(miso[i])
      );
    end
  endgenerate

  // Front panel GPIOs logic
  // Double-sync for the GPIO inputs to the PS and to the Radio blocks.
  synchronizer #(
    .INITIAL_VAL(1'b0), .WIDTH(FP_GPIO_WIDTH)
    ) ps_gpio_in_sync_i (
    .clk(bus_clk), .rst(1'b0), .in(fp_gpio_in_int), .out(ps_gpio_in)
  );
  synchronizer #(
    .INITIAL_VAL(1'b0), .WIDTH(FP_GPIO_WIDTH)
    ) radio_gpio_in_sync_i (
    .clk(radio_clk), .rst(1'b0), .in(fp_gpio_in_int), .out(radio_gpio_sync)
  );

  generate
    for (i=0; i<NUM_CHANNELS; i=i+1) begin: gen_fp_gpio_in_sync
      assign radio_gpio_in[i][FP_GPIO_WIDTH-1:0] = radio_gpio_sync;
    end
  endgenerate

  // For each of the FP GPIO bits, implement four control muxes, then the IO buffer.
  generate
    for (i=0; i<FP_GPIO_WIDTH; i=i+1) begin: gpio_muxing_gen

      // 1) Select which radio drives the output
      assign radio_gpio_src_out[i] = radio_gpio_out[fp_gpio_src_reg[2*i+1:2*i]][i];
      always @ (posedge radio_clk) begin
        if (radio_rst) begin
          radio_gpio_src_out_reg <= 'b0;
        end else begin
          radio_gpio_src_out_reg <= radio_gpio_src_out;
        end
      end

      // 2) Select which radio drives the direction
      assign radio_gpio_src_ddr[i] = radio_gpio_ddr[fp_gpio_src_reg[2*i+1:2*i]][i];
      always @ (posedge radio_clk) begin
        if (radio_rst) begin
          radio_gpio_src_ddr_reg <= 'b0;
        end else begin
          radio_gpio_src_ddr_reg <= radio_gpio_src_ddr;
        end
      end

      // 3) Select if the radio or the ps drives the output
      // The following is implementing a 2:1 mux in a LUT6 explicitly to avoid
      // glitching behavior that is introduced by unexpected Vivado synthesis.
      (* dont_touch = "TRUE" *) LUT3 #(
        .INIT(8'hCA) // Specify LUT Contents. O = ~I2&I0 | I2&I1
      ) mux_out_i (
        .O(fp_gpio_out_int[i]), // LUT general output. Mux output
        .I0(radio_gpio_src_out_reg[i]), // LUT input. Input 1
        .I1(ps_gpio_out[i]), // LUT input. Input 2
        .I2(fp_gpio_master_reg[i])// LUT input. Select bit
      );
      // 4) Select if the radio or the ps drives the direction
      (* dont_touch = "TRUE" *) LUT3 #(
        .INIT(8'hC5) // Specify LUT Contents. O = ~I2&I0 | I2&~I1
      ) mux_ddr_i (
        .O(fp_gpio_ddr_int[i]), // LUT general output. Mux output
        .I0(radio_gpio_src_ddr_reg[i]), // LUT input. Input 1
        .I1(ps_gpio_tri[i]), // LUT input. Input 2
        .I2(fp_gpio_master_reg[i]) // LUT input. Select bit
      );

      // Infer the IOBUFT
      assign fp_gpio_inout[i] = fp_gpio_ddr_int[i] ? 1'bz : fp_gpio_out_int[i];
      assign fp_gpio_in_int[i] = fp_gpio_inout[i];
    end
  endgenerate


  /////////////////////////////////////////////////////////////////////////////
  //
  // Timekeeper
  //
  /////////////////////////////////////////////////////////////////////////////

  wire [63:0] radio_time;

  timekeeper #(
   .BASE_ADDR      (0),      // ctrlport_decoder removes the base offset
   .TIME_INCREMENT (1'b1)
  ) timekeeper_i (
   .tb_clk                (radio_clk),
   .tb_rst                (radio_rst),
   .s_ctrlport_clk        (bus_clk),
   .s_ctrlport_req_wr     (cp_tk_req_wr),
   .s_ctrlport_req_rd     (cp_tk_req_rd),
   .s_ctrlport_req_addr   (cp_tk_req_addr),
   .s_ctrlport_req_data   (cp_tk_req_data),
   .s_ctrlport_resp_ack   (cp_tk_resp_ack),
   .s_ctrlport_resp_data  (cp_tk_resp_data),
   .sample_rx_stb         (rx_stb[0]),
   .pps                   (pps),
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
    .CHDR_W   (CHDR_W),
    .MTU      (BYTE_MTU - $clog2(CHDR_W/8)),
    .PROTOVER (RFNOC_PROTOVER)
  ) rfnoc_sandbox_i (
    .chdr_aclk               (bus_clk    ),
    .ctrl_aclk               (clk40      ),
    .core_arst               (bus_rst    ),
    .device_id               (device_id  ),
    .radio_clk               (radio_clk  ),
    .dram_clk                (ddr3_dma_clk),
    `ifndef N300
      .m_ctrlport_radio1_req_wr       (m_ctrlport_req_wr_radio1      ),
      .m_ctrlport_radio1_req_rd       (m_ctrlport_req_rd_radio1      ),
      .m_ctrlport_radio1_req_addr     (m_ctrlport_req_addr_radio1    ),
      .m_ctrlport_radio1_req_data     (m_ctrlport_req_data_radio1    ),
      .m_ctrlport_radio1_req_byte_en  (m_ctrlport_req_byte_en_radio1 ),
      .m_ctrlport_radio1_req_has_time (m_ctrlport_req_has_time_radio1),
      .m_ctrlport_radio1_req_time     (m_ctrlport_req_time_radio1    ),
      .m_ctrlport_radio1_resp_ack     (m_ctrlport_resp_ack_radio1    ),
      .m_ctrlport_radio1_resp_status  (m_ctrlport_resp_status_radio1 ),
      .m_ctrlport_radio1_resp_data    (m_ctrlport_resp_data_radio1   ),
    `endif
    .m_ctrlport_radio0_req_wr       (m_ctrlport_req_wr_radio0      ),
    .m_ctrlport_radio0_req_rd       (m_ctrlport_req_rd_radio0      ),
    .m_ctrlport_radio0_req_addr     (m_ctrlport_req_addr_radio0    ),
    .m_ctrlport_radio0_req_data     (m_ctrlport_req_data_radio0    ),
    .m_ctrlport_radio0_req_byte_en  (m_ctrlport_req_byte_en_radio0 ),
    .m_ctrlport_radio0_req_has_time (m_ctrlport_req_has_time_radio0),
    .m_ctrlport_radio0_req_time     (m_ctrlport_req_time_radio0    ),
    .m_ctrlport_radio0_resp_ack     (m_ctrlport_resp_ack_radio0    ),
    .m_ctrlport_radio0_resp_status  (m_ctrlport_resp_status_radio0 ),
    .m_ctrlport_radio0_resp_data    (m_ctrlport_resp_data_radio0   ),
    .axi_rst        (ddr3_dma_rst),
    .m_axi_awid     ({dram_axi_awid    [3], dram_axi_awid    [2], dram_axi_awid    [1], dram_axi_awid    [0]}),
    .m_axi_awaddr   ({dram_axi_awaddr  [3], dram_axi_awaddr  [2], dram_axi_awaddr  [1], dram_axi_awaddr  [0]}),
    .m_axi_awlen    ({dram_axi_awlen   [3], dram_axi_awlen   [2], dram_axi_awlen   [1], dram_axi_awlen   [0]}),
    .m_axi_awsize   ({dram_axi_awsize  [3], dram_axi_awsize  [2], dram_axi_awsize  [1], dram_axi_awsize  [0]}),
    .m_axi_awburst  ({dram_axi_awburst [3], dram_axi_awburst [2], dram_axi_awburst [1], dram_axi_awburst [0]}),
    .m_axi_awlock   ({dram_axi_awlock  [3], dram_axi_awlock  [2], dram_axi_awlock  [1], dram_axi_awlock  [0]}),
    .m_axi_awcache  ({dram_axi_awcache [3], dram_axi_awcache [2], dram_axi_awcache [1], dram_axi_awcache [0]}),
    .m_axi_awprot   ({dram_axi_awprot  [3], dram_axi_awprot  [2], dram_axi_awprot  [1], dram_axi_awprot  [0]}),
    .m_axi_awqos    ({dram_axi_awqos   [3], dram_axi_awqos   [2], dram_axi_awqos   [1], dram_axi_awqos   [0]}),
    .m_axi_awregion ({dram_axi_awregion[3], dram_axi_awregion[2], dram_axi_awregion[1], dram_axi_awregion[0]}),
    .m_axi_awuser   ({dram_axi_awuser  [3], dram_axi_awuser  [2], dram_axi_awuser  [1], dram_axi_awuser  [0]}),
    .m_axi_awvalid  ({dram_axi_awvalid [3], dram_axi_awvalid [2], dram_axi_awvalid [1], dram_axi_awvalid [0]}),
    .m_axi_awready  ({dram_axi_awready [3], dram_axi_awready [2], dram_axi_awready [1], dram_axi_awready [0]}),
    .m_axi_wdata    ({dram_axi_wdata   [3], dram_axi_wdata   [2], dram_axi_wdata   [1], dram_axi_wdata   [0]}),
    .m_axi_wstrb    ({dram_axi_wstrb   [3], dram_axi_wstrb   [2], dram_axi_wstrb   [1], dram_axi_wstrb   [0]}),
    .m_axi_wlast    ({dram_axi_wlast   [3], dram_axi_wlast   [2], dram_axi_wlast   [1], dram_axi_wlast   [0]}),
    .m_axi_wuser    ({dram_axi_wuser   [3], dram_axi_wuser   [2], dram_axi_wuser   [1], dram_axi_wuser   [0]}),
    .m_axi_wvalid   ({dram_axi_wvalid  [3], dram_axi_wvalid  [2], dram_axi_wvalid  [1], dram_axi_wvalid  [0]}),
    .m_axi_wready   ({dram_axi_wready  [3], dram_axi_wready  [2], dram_axi_wready  [1], dram_axi_wready  [0]}),
    .m_axi_bid      ({dram_axi_bid     [3], dram_axi_bid     [2], dram_axi_bid     [1], dram_axi_bid     [0]}),
    .m_axi_bresp    ({dram_axi_bresp   [3], dram_axi_bresp   [2], dram_axi_bresp   [1], dram_axi_bresp   [0]}),
    .m_axi_buser    ({dram_axi_buser   [3], dram_axi_buser   [2], dram_axi_buser   [1], dram_axi_buser   [0]}),
    .m_axi_bvalid   ({dram_axi_bvalid  [3], dram_axi_bvalid  [2], dram_axi_bvalid  [1], dram_axi_bvalid  [0]}),
    .m_axi_bready   ({dram_axi_bready  [3], dram_axi_bready  [2], dram_axi_bready  [1], dram_axi_bready  [0]}),
    .m_axi_arid     ({dram_axi_arid    [3], dram_axi_arid    [2], dram_axi_arid    [1], dram_axi_arid    [0]}),
    .m_axi_araddr   ({dram_axi_araddr  [3], dram_axi_araddr  [2], dram_axi_araddr  [1], dram_axi_araddr  [0]}),
    .m_axi_arlen    ({dram_axi_arlen   [3], dram_axi_arlen   [2], dram_axi_arlen   [1], dram_axi_arlen   [0]}),
    .m_axi_arsize   ({dram_axi_arsize  [3], dram_axi_arsize  [2], dram_axi_arsize  [1], dram_axi_arsize  [0]}),
    .m_axi_arburst  ({dram_axi_arburst [3], dram_axi_arburst [2], dram_axi_arburst [1], dram_axi_arburst [0]}),
    .m_axi_arlock   ({dram_axi_arlock  [3], dram_axi_arlock  [2], dram_axi_arlock  [1], dram_axi_arlock  [0]}),
    .m_axi_arcache  ({dram_axi_arcache [3], dram_axi_arcache [2], dram_axi_arcache [1], dram_axi_arcache [0]}),
    .m_axi_arprot   ({dram_axi_arprot  [3], dram_axi_arprot  [2], dram_axi_arprot  [1], dram_axi_arprot  [0]}),
    .m_axi_arqos    ({dram_axi_arqos   [3], dram_axi_arqos   [2], dram_axi_arqos   [1], dram_axi_arqos   [0]}),
    .m_axi_arregion ({dram_axi_arregion[3], dram_axi_arregion[2], dram_axi_arregion[1], dram_axi_arregion[0]}),
    .m_axi_aruser   ({dram_axi_aruser  [3], dram_axi_aruser  [2], dram_axi_aruser  [1], dram_axi_aruser  [0]}),
    .m_axi_arvalid  ({dram_axi_arvalid [3], dram_axi_arvalid [2], dram_axi_arvalid [1], dram_axi_arvalid [0]}),
    .m_axi_arready  ({dram_axi_arready [3], dram_axi_arready [2], dram_axi_arready [1], dram_axi_arready [0]}),
    .m_axi_rid      ({dram_axi_rid     [3], dram_axi_rid     [2], dram_axi_rid     [1], dram_axi_rid     [0]}),
    .m_axi_rdata    ({dram_axi_rdata   [3], dram_axi_rdata   [2], dram_axi_rdata   [1], dram_axi_rdata   [0]}),
    .m_axi_rresp    ({dram_axi_rresp   [3], dram_axi_rresp   [2], dram_axi_rresp   [1], dram_axi_rresp   [0]}),
    .m_axi_rlast    ({dram_axi_rlast   [3], dram_axi_rlast   [2], dram_axi_rlast   [1], dram_axi_rlast   [0]}),
    .m_axi_ruser    ({dram_axi_ruser   [3], dram_axi_ruser   [2], dram_axi_ruser   [1], dram_axi_ruser   [0]}),
    .m_axi_rvalid   ({dram_axi_rvalid  [3], dram_axi_rvalid  [2], dram_axi_rvalid  [1], dram_axi_rvalid  [0]}),
    .m_axi_rready   ({dram_axi_rready  [3], dram_axi_rready  [2], dram_axi_rready  [1], dram_axi_rready  [0]}),
    .radio_time                     (radio_time      ),
  `ifdef N320
    .radio_rx_stb_radio0            (                rx_stb [0]    ),
    .radio_rx_data_radio0           (                rx_data[0]    ),
    .radio_rx_running_radio0        (                rx_running[0] ),
    .radio_tx_stb_radio0            (                tx_stb [0]    ),
    .radio_tx_data_radio0           (                tx_data[0]    ),
    .radio_tx_running_radio0        (                tx_running[0] ),
    .radio_rx_stb_radio1            (                rx_stb [1]    ),
    .radio_rx_data_radio1           (                rx_data[1]    ),
    .radio_rx_running_radio1        (                rx_running[1] ),
    .radio_tx_stb_radio1            (                tx_stb [1]    ),
    .radio_tx_data_radio1           (                tx_data[1]    ),
    .radio_tx_running_radio1        (                tx_running[1] ),
  `endif
  `ifndef N320
    .radio_rx_stb_radio0            ({rx_stb[1],     rx_stb [0]   }),
    .radio_rx_data_radio0           ({rx_data[1],    rx_data[0]   }),
    .radio_rx_running_radio0        ({rx_running[1], rx_running[0]}),
    .radio_tx_stb_radio0            ({tx_stb[1],     tx_stb [0]   }),
    .radio_tx_data_radio0           ({tx_data[1],    tx_data[0]   }),
    .radio_tx_running_radio0        ({tx_running[1], tx_running[0]}),
  `endif
  `ifdef N310
    .radio_rx_stb_radio1            ({rx_stb[3],     rx_stb [2]   }),
    .radio_rx_data_radio1           ({rx_data[3],    rx_data[2]   }),
    .radio_rx_running_radio1        ({rx_running[3], rx_running[2]}),
    .radio_tx_stb_radio1            ({tx_stb[3],     tx_stb [2]   }),
    .radio_tx_data_radio1           ({tx_data[3],    tx_data[2]   }),
    .radio_tx_running_radio1        ({tx_running[3], tx_running[2]}),
  `endif
    .s_eth0_tdata            (e2v0_tdata ),
    .s_eth0_tlast            (e2v0_tlast ),
    .s_eth0_tvalid           (e2v0_tvalid),
    .s_eth0_tready           (e2v0_tready),
    .m_eth0_tdata            (v2e0_tdata ),
    .m_eth0_tlast            (v2e0_tlast ),
    .m_eth0_tvalid           (v2e0_tvalid),
    .m_eth0_tready           (v2e0_tready),
    .s_eth1_tdata            (e2v1_tdata ),
    .s_eth1_tlast            (e2v1_tlast ),
    .s_eth1_tvalid           (e2v1_tvalid),
    .s_eth1_tready           (e2v1_tready),
    .m_eth1_tdata            (v2e1_tdata ),
    .m_eth1_tlast            (v2e1_tlast ),
    .m_eth1_tvalid           (v2e1_tvalid),
    .m_eth1_tready           (v2e1_tready),
    .s_dma_tdata             (s_dma_tdata),
    .s_dma_tlast             (s_dma_tlast),
    .s_dma_tvalid            (s_dma_tvalid),
    .s_dma_tready            (s_dma_tready),
    .m_dma_tdata             (m_dma_tdata),
    .m_dma_tlast             (m_dma_tlast),
    .m_dma_tvalid            (m_dma_tvalid),
    .m_dma_tready            (m_dma_tready)
  );


  //---------------------------------------------------------------------------
  // Convert Control Port to Settings Bus
  //---------------------------------------------------------------------------
`ifdef N320
  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_CHANNELS_PER_RADIO),
    .USE_TIME  (1)
  ) ctrlport0_to_settings_bus_i (
    .ctrlport_clk             (radio_clk),
    .ctrlport_rst             (radio_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr_radio0),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd_radio0),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr_radio0),
    .s_ctrlport_req_data      (m_ctrlport_req_data_radio0),
    .s_ctrlport_req_has_time  (m_ctrlport_req_has_time_radio0),
    .s_ctrlport_req_time      (m_ctrlport_req_time_radio0),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack_radio0),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data_radio0),
    .set_data                 (db_fe_set_data[0]),
    .set_addr                 (db_fe_set_addr[0]),
    .set_stb                  (db_fe_set_stb[0]),
    .set_time                 (),
    .set_has_time             (),
    .rb_stb                   (db_fe_rb_stb[0]),
    .rb_addr                  (db_fe_rb_addr[0]),
    .rb_data                  (db_fe_rb_data[0]),
    .timestamp                (radio_time)
  );

  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_CHANNELS_PER_RADIO),
    .USE_TIME  (1)
  ) ctrlport1_to_settings_bus_i (
    .ctrlport_clk             (radio_clk),
    .ctrlport_rst             (radio_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr_radio1),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd_radio1),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr_radio1),
    .s_ctrlport_req_data      (m_ctrlport_req_data_radio1),
    .s_ctrlport_req_has_time  (m_ctrlport_req_has_time_radio1),
    .s_ctrlport_req_time      (m_ctrlport_req_time_radio1),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack_radio1),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data_radio1),
    .set_data                 (db_fe_set_data[1]),
    .set_addr                 (db_fe_set_addr[1]),
    .set_stb                  (db_fe_set_stb[1]),
    .set_time                 (),
    .set_has_time             (),
    .rb_stb                   (db_fe_rb_stb[1]),
    .rb_addr                  (db_fe_rb_addr[1]),
    .rb_data                  (db_fe_rb_data[1]),
    .timestamp                (radio_time)
  );
`endif


`ifndef N320
  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_CHANNELS_PER_RADIO),
    .USE_TIME  (1)
  ) ctrlport0_to_settings_bus_i (
    .ctrlport_clk             (radio_clk),
    .ctrlport_rst             (radio_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr_radio0),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd_radio0),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr_radio0),
    .s_ctrlport_req_data      (m_ctrlport_req_data_radio0),
    .s_ctrlport_req_has_time  (m_ctrlport_req_has_time_radio0),
    .s_ctrlport_req_time      (m_ctrlport_req_time_radio0),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack_radio0),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data_radio0),
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

  `ifndef N300
    ctrlport_to_settings_bus # (
      .NUM_PORTS (NUM_CHANNELS_PER_RADIO),
      .USE_TIME  (1)
    ) ctrlport1_to_settings_bus_i (
      .ctrlport_clk             (radio_clk),
      .ctrlport_rst             (radio_rst),
      .s_ctrlport_req_wr        (m_ctrlport_req_wr_radio1),
      .s_ctrlport_req_rd        (m_ctrlport_req_rd_radio1),
      .s_ctrlport_req_addr      (m_ctrlport_req_addr_radio1),
      .s_ctrlport_req_data      (m_ctrlport_req_data_radio1),
      .s_ctrlport_req_has_time  (m_ctrlport_req_has_time_radio1),
      .s_ctrlport_req_time      (m_ctrlport_req_time_radio1),
      .s_ctrlport_resp_ack      (m_ctrlport_resp_ack_radio1),
      .s_ctrlport_resp_data     (m_ctrlport_resp_data_radio1),
      .set_data                 ({db_fe_set_data[3], db_fe_set_data[2]}),
      .set_addr                 ({db_fe_set_addr[3], db_fe_set_addr[2]}),
      .set_stb                  ({db_fe_set_stb[3],  db_fe_set_stb[2] }),
      .set_time                 (),
      .set_has_time             (),
      .rb_stb                   ({db_fe_rb_stb[3],   db_fe_rb_stb[2]  }),
      .rb_addr                  ({db_fe_rb_addr[3],  db_fe_rb_addr[2] }),
      .rb_data                  ({db_fe_rb_data[3],  db_fe_rb_data[2] }),
      .timestamp                (radio_time)
    );
  `endif
`endif

endmodule //n3xx_core
