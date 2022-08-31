/////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0
//
// Module: e31x_core
// Description:
//  - Motherboard Registers
//  - RFNoC Image Core + Blocks
//  - Radio Front End control
//
/////////////////////////////////////////////////////////////////////

`default_nettype none
module e31x_core #(
  parameter REG_DWIDTH   = 32,        // Width of the AXI4-Lite data bus (must be 32 or 64)
  parameter REG_AWIDTH   = 32,        // Width of the address bus
  parameter BUS_CLK_RATE = 200000000, // bus_clk rate
  parameter NUM_SFP_PORTS = 0,        // Number of SFP Ports
  parameter NUM_RADIOS = 1,
  parameter NUM_CHANNELS_PER_RADIO = 2,
  parameter NUM_CHANNELS = 2,
  parameter NUM_DBOARDS = 1,
  parameter NUM_CHANNELS_PER_DBOARD = 2,
  parameter FP_GPIO_WIDTH = 8,  // Front panel GPIO width
  parameter DB_GPIO_WIDTH = 16,  // Daughterboard GPIO width
  parameter CHDR_W = 64,
  parameter BYTE_MTU = $clog2(8192),
  parameter RFNOC_PROTOVER  = {8'd1, 8'd0}
)(
  // Clocks and resets
  input wire radio_clk,
  input wire radio_rst,
  input wire bus_clk,
  input wire bus_rst,
  input wire clk40,
  input wire clk200,

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

  // DRAM Controller Clock
  input wire         ddr3_sys_clk,

  // DRAM Chip Interface
  output wire [14:0] ddr3_addr,
  output wire [ 2:0] ddr3_ba,
  output wire        ddr3_cas_n,
  output wire [ 0:0] ddr3_ck_n,
  output wire [ 0:0] ddr3_ck_p,
  output wire [ 0:0] ddr3_cke,
  output wire        ddr3_ras_n,
  output wire        ddr3_reset_n,
  output wire        ddr3_we_n,
  inout  wire [15:0] ddr3_dq,
  inout  wire [ 1:0] ddr3_dqs_n,
  inout  wire [ 1:0] ddr3_dqs_p,
  output wire [ 1:0] ddr3_dm,
  output wire [ 0:0] ddr3_odt,

  // DMA xport adapter to PS
  input wire  [63:0] s_dma_tdata,
  input wire         s_dma_tlast,
  output wire        s_dma_tready,
  input wire         s_dma_tvalid,

  output wire [63:0] m_dma_tdata,
  output wire        m_dma_tlast,
  input wire         m_dma_tready,
  output wire        m_dma_tvalid,

  // Misc
  input wire [31:0] build_datestamp,
  input wire [31:0] sfp_ports_info,
  input wire [31:0] dboard_status,
  input wire [31:0] xadc_readback,
  output reg [31:0] fp_gpio_ctrl,
  output reg [31:0] dboard_ctrl,
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

  localparam [15:0] COMPAT_MAJOR = 16'd6;
  localparam [15:0] COMPAT_MINOR = 16'd0;

  /////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////
  //
  // Motherboard Registers
  //
  /////////////////////////////////////////////////////////////////////////////////

  // Register base
  localparam [CTRLPORT_ADDR_W-1:0] REG_BASE_MISC       = 20'h0;
  localparam [CTRLPORT_ADDR_W-1:0] REG_BASE_TIMEKEEPER = 20'h1000;

  // Register region sizes. Give each register region a 256-byte window.
  localparam REG_GLOB_ADDR_W       = 8;
  localparam REG_TIMEKEEPER_ADDR_W = 8;

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
  localparam REG_DBOARD_CTRL       = REG_BASE_MISC + 14'h40;
  localparam REG_DBOARD_STATUS     = REG_BASE_MISC + 14'h44;
  localparam REG_NUM_TIMEKEEPERS   = REG_BASE_MISC + 14'h48;

  localparam NUM_TIMEKEEPERS = 1;

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

  reg [31:0] scratch_reg = 32'h0;
  reg [31:0] bus_counter = 32'h0;

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

  // We need a really long timeout for CtrlPort transactions, since the
  // radio_clk for the timekeeper might be really slow compared to s_axi_aclk.
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

  wire                       cp_tk_req_wr;
  wire                       cp_tk_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] cp_tk_req_addr;
  wire [CTRLPORT_DATA_W-1:0] cp_tk_req_data;
  wire                       cp_tk_resp_ack;
  wire [CTRLPORT_DATA_W-1:0] cp_tk_resp_data;

  // Split the CtrlPort for the global registers and the timekeeper registers
  ctrlport_decoder_param #(
    .NUM_SLAVES  (2),
    .PORT_BASE   ({   REG_BASE_TIMEKEEPER,   REG_BASE_MISC }),
    .PORT_ADDR_W ({ REG_TIMEKEEPER_ADDR_W, REG_GLOB_ADDR_W })
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
    .m_ctrlport_req_wr       ({    cp_tk_req_wr,    cp_glob_req_wr }),
    .m_ctrlport_req_rd       ({    cp_tk_req_rd,    cp_glob_req_rd }),
    .m_ctrlport_req_addr     ({  cp_tk_req_addr,  cp_glob_req_addr }),
    .m_ctrlport_req_data     ({  cp_tk_req_data,  cp_glob_req_data }),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({  cp_tk_resp_ack,  cp_glob_resp_ack }),
    .m_ctrlport_resp_status  ({2{CTRL_STS_OKAY}}),
    .m_ctrlport_resp_data    ({ cp_tk_resp_data, cp_glob_resp_data })
  );


  /////////////////////////////////////////////////////////////////////////////
  //
  // Global Registers
  //
  /////////////////////////////////////////////////////////////////////////////

  // Write Registers
  always @ (posedge bus_clk) begin
    if (bus_rst) begin
      scratch_reg       <= 32'h0;
      pps_select        <= 2'b01; // Default to internal
      fp_gpio_ctrl      <= 32'h9; // Default to OFF - 4'b1001
      dboard_ctrl       <= 32'h1; // Default to mimo
      device_id         <= 16'h0;
      cp_glob_resp_ack  <= 1'b0;
      cp_glob_resp_data <= 'bX;
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
            fp_gpio_src_reg  <= cp_glob_req_data;

          REG_SCRATCH:
            scratch_reg <= cp_glob_req_data;

          REG_CLOCK_CTRL:
            pps_select <= cp_glob_req_data[1:0];

          REG_FP_GPIO_CTRL:
            fp_gpio_ctrl <= cp_glob_req_data;

          REG_DBOARD_CTRL:
            dboard_ctrl <= cp_glob_req_data;

          default : begin
            // Don't acknowledge if the address doesn't match
            cp_glob_resp_ack <= 1'b0;
          end
        endcase
      end

      if (cp_glob_req_rd) begin
        cp_glob_resp_data <= 0;  // Unused bits will read 0
        cp_glob_resp_ack  <= 1'b1;

        case (cp_glob_req_addr[REG_GLOB_ADDR_W-1:0])
          REG_DEVICE_ID:
            cp_glob_resp_data <= {16'd0, device_id};

          REG_RFNOC_INFO:
            cp_glob_resp_data <= {CHDR_W[15:0], RFNOC_PROTOVER[15:0]};

          REG_COMPAT_NUM:
            cp_glob_resp_data <= {COMPAT_MAJOR[15:0], COMPAT_MINOR[15:0]};

          REG_FP_GPIO_CTRL:
            cp_glob_resp_data <= fp_gpio_ctrl;

          REG_FP_GPIO_MASTER:
            cp_glob_resp_data <= fp_gpio_master_reg;

          REG_FP_GPIO_RADIO_SRC:
            cp_glob_resp_data <= fp_gpio_src_reg;

          REG_DATESTAMP:
            cp_glob_resp_data <= build_datestamp;

          REG_GIT_HASH: begin
            `ifndef GIT_HASH
            `define GIT_HASH 32'h0BADC0DE
            `endif
            cp_glob_resp_data <= `GIT_HASH;
          end

          REG_SCRATCH:
            cp_glob_resp_data <= scratch_reg;

          REG_CLOCK_CTRL: begin
            cp_glob_resp_data      <= 32'b0;
            cp_glob_resp_data[1:0] <= pps_select;
            cp_glob_resp_data[3]   <= refclk_locked;
          end

          REG_XADC_READBACK:
            cp_glob_resp_data <= xadc_readback;

          REG_BUS_CLK_RATE:
            cp_glob_resp_data <= BUS_CLK_RATE;

          REG_BUS_CLK_COUNT:
            cp_glob_resp_data <= bus_counter;

          REG_SFP_PORT_INFO:
            cp_glob_resp_data <= sfp_ports_info;

          REG_DBOARD_CTRL:
            cp_glob_resp_data <= dboard_ctrl;

          REG_DBOARD_STATUS:
            cp_glob_resp_data <= dboard_status;

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

  wire pps_radioclk;

  // Synchronize the PPS signal to the radio clock domain
  synchronizer pps_radio_sync (
    .clk(radio_clk), .rst(1'b0), .in(pps_refclk), .out(pps_radioclk)
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
   .pps                   (pps_radioclk),
   .tb_timestamp          (radio_time),
   .tb_timestamp_last_pps (),
   .tb_period_ns_q32      ()
  );

  /////////////////////////////////////////////////////////////////////////////
  // PL DDR3 Memory Interface
  /////////////////////////////////////////////////////////////////////////////

  localparam NUM_PORTS  = 2;
  localparam AXI_DWIDTH = 64;

  wire [          32*NUM_PORTS-1:0] dram_axi_araddr;
  wire [           2*NUM_PORTS-1:0] dram_axi_arburst;
  wire [           4*NUM_PORTS-1:0] dram_axi_arcache;
  wire [           1*NUM_PORTS-1:0] dram_axi_arid;
  wire [           8*NUM_PORTS-1:0] dram_axi_arlen;
  wire [           1*NUM_PORTS-1:0] dram_axi_arlock;
  wire [           3*NUM_PORTS-1:0] dram_axi_arprot;
  wire [           4*NUM_PORTS-1:0] dram_axi_arqos;
  wire [           1*NUM_PORTS-1:0] dram_axi_arready;
  wire [           4*NUM_PORTS-1:0] dram_axi_arregion;
  wire [           3*NUM_PORTS-1:0] dram_axi_arsize;
  wire [           1*NUM_PORTS-1:0] dram_axi_arvalid;
  wire [          32*NUM_PORTS-1:0] dram_axi_awaddr;
  wire [           2*NUM_PORTS-1:0] dram_axi_awburst;
  wire [           4*NUM_PORTS-1:0] dram_axi_awcache;
  wire [           1*NUM_PORTS-1:0] dram_axi_awid;
  wire [           8*NUM_PORTS-1:0] dram_axi_awlen;
  wire [           1*NUM_PORTS-1:0] dram_axi_awlock;
  wire [           3*NUM_PORTS-1:0] dram_axi_awprot;
  wire [           4*NUM_PORTS-1:0] dram_axi_awqos;
  wire [           1*NUM_PORTS-1:0] dram_axi_awready;
  wire [           4*NUM_PORTS-1:0] dram_axi_awregion;
  wire [           3*NUM_PORTS-1:0] dram_axi_awsize;
  wire [           1*NUM_PORTS-1:0] dram_axi_awvalid;
  wire [           1*NUM_PORTS-1:0] dram_axi_bid;
  wire [           1*NUM_PORTS-1:0] dram_axi_bready;
  wire [           2*NUM_PORTS-1:0] dram_axi_bresp;
  wire [           1*NUM_PORTS-1:0] dram_axi_bvalid;
  wire [  AXI_DWIDTH*NUM_PORTS-1:0] dram_axi_rdata;
  wire [           1*NUM_PORTS-1:0] dram_axi_rid;
  wire [           1*NUM_PORTS-1:0] dram_axi_rlast;
  wire [           1*NUM_PORTS-1:0] dram_axi_rready;
  wire [           2*NUM_PORTS-1:0] dram_axi_rresp;
  wire [           1*NUM_PORTS-1:0] dram_axi_rvalid;
  wire [  AXI_DWIDTH*NUM_PORTS-1:0] dram_axi_wdata;
  wire [           1*NUM_PORTS-1:0] dram_axi_wlast;
  wire [           1*NUM_PORTS-1:0] dram_axi_wready;
  wire [AXI_DWIDTH/8*NUM_PORTS-1:0] dram_axi_wstrb;
  wire [           1*NUM_PORTS-1:0] dram_axi_wvalid;

  wire dram_clk;
  wire dram_rst;

  `ifdef ENABLE_DRAM
  begin : gen_dram
    wire clk200_rst;

    synchronizer synchronizer_clk200_rst (
      .clk(clk200  ),
      .rst(1'b0    ),
      .in (bus_rst ),
      .out(clk200_rst)
    );

    e31x_dram #(
      .AXI_DWIDTH(AXI_DWIDTH),
      .NUM_PORTS (NUM_PORTS )
    ) e31x_dram_i (
      .sys_rst          (bus_rst          ),
      .ddr3_sys_clk     (ddr3_sys_clk     ),
      .clk200           (clk200           ),
      .clk200_rst       (clk200_rst       ),
      .ddr3_addr        (ddr3_addr        ),
      .ddr3_ba          (ddr3_ba          ),
      .ddr3_cas_n       (ddr3_cas_n       ),
      .ddr3_ck_n        (ddr3_ck_n        ),
      .ddr3_ck_p        (ddr3_ck_p        ),
      .ddr3_cke         (ddr3_cke         ),
      .ddr3_ras_n       (ddr3_ras_n       ),
      .ddr3_reset_n     (ddr3_reset_n     ),
      .ddr3_we_n        (ddr3_we_n        ),
      .ddr3_dq          (ddr3_dq          ),
      .ddr3_dqs_n       (ddr3_dqs_n       ),
      .ddr3_dqs_p       (ddr3_dqs_p       ),
      .ddr3_dm          (ddr3_dm          ),
      .ddr3_odt         (ddr3_odt         ),
      .dram_clk         (dram_clk         ),
      .dram_rst         (dram_rst         ),
      .dram_axi_araddr  (dram_axi_araddr  ),
      .dram_axi_arburst (dram_axi_arburst ),
      .dram_axi_arcache (dram_axi_arcache ),
      .dram_axi_arid    (dram_axi_arid    ),
      .dram_axi_arlen   (dram_axi_arlen   ),
      .dram_axi_arlock  (dram_axi_arlock  ),
      .dram_axi_arprot  (dram_axi_arprot  ),
      .dram_axi_arqos   (dram_axi_arqos   ),
      .dram_axi_arready (dram_axi_arready ),
      .dram_axi_arregion(dram_axi_arregion),
      .dram_axi_arsize  (dram_axi_arsize  ),
      .dram_axi_arvalid (dram_axi_arvalid ),
      .dram_axi_awaddr  (dram_axi_awaddr  ),
      .dram_axi_awburst (dram_axi_awburst ),
      .dram_axi_awcache (dram_axi_awcache ),
      .dram_axi_awid    (dram_axi_awid    ),
      .dram_axi_awlen   (dram_axi_awlen   ),
      .dram_axi_awlock  (dram_axi_awlock  ),
      .dram_axi_awprot  (dram_axi_awprot  ),
      .dram_axi_awqos   (dram_axi_awqos   ),
      .dram_axi_awready (dram_axi_awready ),
      .dram_axi_awregion(dram_axi_awregion),
      .dram_axi_awsize  (dram_axi_awsize  ),
      .dram_axi_awvalid (dram_axi_awvalid ),
      .dram_axi_bid     (dram_axi_bid     ),
      .dram_axi_bready  (dram_axi_bready  ),
      .dram_axi_bresp   (dram_axi_bresp   ),
      .dram_axi_bvalid  (dram_axi_bvalid  ),
      .dram_axi_rdata   (dram_axi_rdata   ),
      .dram_axi_rid     (dram_axi_rid     ),
      .dram_axi_rlast   (dram_axi_rlast   ),
      .dram_axi_rready  (dram_axi_rready  ),
      .dram_axi_rresp   (dram_axi_rresp   ),
      .dram_axi_rvalid  (dram_axi_rvalid  ),
      .dram_axi_wdata   (dram_axi_wdata   ),
      .dram_axi_wlast   (dram_axi_wlast   ),
      .dram_axi_wready  (dram_axi_wready  ),
      .dram_axi_wstrb   (dram_axi_wstrb   ),
      .dram_axi_wvalid  (dram_axi_wvalid  )
    );
  end  // gen_dram
  `endif

  /////////////////////////////////////////////////////////////////////////////
  //
  // RFNoC Image Core
  //
  /////////////////////////////////////////////////////////////////////////////

  rfnoc_image_core #(
    .CHDR_W   (CHDR_W),
    .MTU      (BYTE_MTU - $clog2(CHDR_W/8)),
    .PROTOVER (RFNOC_PROTOVER)
  ) rfnoc_image_core_i (
    .chdr_aclk               (bus_clk    ),
    .ctrl_aclk               (clk40      ),
    .core_arst               (bus_rst    ),
    .device_id               (device_id  ),
    .radio_clk               (radio_clk  ),
    .dram_clk                (dram_clk   ),
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
    .radio_time              (radio_time             ),
    .radio_rx_stb            ({rx_stb[1],     rx_stb[0]    }),
    .radio_rx_data           ({rx_data[1],    rx_data[0]   }),
    .radio_rx_running        ({rx_running[1], rx_running[0]}),
    .radio_tx_stb            ({tx_stb[1],     tx_stb[0]    }),
    .radio_tx_data           ({tx_data[1],    tx_data[0]   }),
    .radio_tx_running        ({tx_running[1], tx_running[0]}),
    .axi_rst                 (dram_rst),
    .m_axi_awid              (dram_axi_awid),
    .m_axi_awaddr            (dram_axi_awaddr),
    .m_axi_awlen             (dram_axi_awlen),
    .m_axi_awsize            (dram_axi_awsize),
    .m_axi_awburst           (dram_axi_awburst),
    .m_axi_awlock            (dram_axi_awlock),
    .m_axi_awcache           (dram_axi_awcache),
    .m_axi_awprot            (dram_axi_awprot),
    .m_axi_awqos             (dram_axi_awqos),
    .m_axi_awregion          (dram_axi_awregion),
    .m_axi_awuser            (),
    .m_axi_awvalid           (dram_axi_awvalid),
    .m_axi_awready           (dram_axi_awready),
    .m_axi_wdata             (dram_axi_wdata),
    .m_axi_wstrb             (dram_axi_wstrb),
    .m_axi_wlast             (dram_axi_wlast),
    .m_axi_wuser             (),
    .m_axi_wvalid            (dram_axi_wvalid),
    .m_axi_wready            (dram_axi_wready),
    .m_axi_bid               (dram_axi_bid),
    .m_axi_bresp             (dram_axi_bresp),
    .m_axi_buser             (4'b0),
    .m_axi_bvalid            (dram_axi_bvalid),
    .m_axi_bready            (dram_axi_bready),
    .m_axi_arid              (dram_axi_arid),
    .m_axi_araddr            (dram_axi_araddr),
    .m_axi_arlen             (dram_axi_arlen),
    .m_axi_arsize            (dram_axi_arsize),
    .m_axi_arburst           (dram_axi_arburst),
    .m_axi_arlock            (dram_axi_arlock),
    .m_axi_arcache           (dram_axi_arcache),
    .m_axi_arprot            (dram_axi_arprot),
    .m_axi_arqos             (dram_axi_arqos),
    .m_axi_arregion          (dram_axi_arregion),
    .m_axi_aruser            (),
    .m_axi_arvalid           (dram_axi_arvalid),
    .m_axi_arready           (dram_axi_arready),
    .m_axi_rid               (dram_axi_rid),
    .m_axi_rdata             (dram_axi_rdata),
    .m_axi_rresp             (dram_axi_rresp),
    .m_axi_rlast             (dram_axi_rlast),
    .m_axi_ruser             (4'b0),
    .m_axi_rvalid            (dram_axi_rvalid),
    .m_axi_rready            (dram_axi_rready),
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

endmodule //e31x_core
`default_nettype wire

