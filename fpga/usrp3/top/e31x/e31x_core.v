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
//  - Crossbar
//  - Noc Block Radio
//  - Noc Block Dram Fifo
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
  parameter CHDR_WIDTH  = 16'd64 ,
  parameter RFNOC_PROTOVER  = {8'd1, 8'd0}
)(
  // Clocks and resets
  input wire radio_clk,
  input wire radio_rst,
  input wire bus_clk,
  input wire bus_rst,

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

  // Misc
  input wire [31:0] build_datestamp,
  input wire [31:0] sfp_ports_info,
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
      fp_gpio_ctrl   <= 32'h9; // Default to OFF - 4'b1001
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
        end
        REG_FP_GPIO_CTRL: begin
          fp_gpio_ctrl <= reg_wr_data;
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
          reg_rd_data_glob <= `GIT_HASH;

        REG_SCRATCH:
          reg_rd_data_glob <= scratch_reg;

        REG_CLOCK_CTRL: begin
          reg_rd_data_glob      <= 32'b0;
          reg_rd_data_glob[1:0] <= pps_select;
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


  rfnoc_image_core #(
    .PROTOVER(RFNOC_PROTOVER)
  ) rfnoc_image_core_i (
    .chdr_aclk               (bus_clk    ),
    .ctrl_aclk               (bus_clk    ), //TODO: X310 uses bus_clk_div2. we can also reduce it here.
    .core_arst               (bus_rst    ),
    .device_id               (device_id  ),
    .radio_clk               (radio_clk  ),
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

endmodule //e31x_core
`default_nettype wire

