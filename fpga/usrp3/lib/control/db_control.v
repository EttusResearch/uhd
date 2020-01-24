//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module db_control #(
  // Drive SPI core with input spi_clk instead of ce_clk. This is useful if ce_clk is very slow which
  // would cause spi transactions to take a long time. WARNING: This adds a clock crossing FIFO!
  parameter USE_SPI_CLK = 0,
  parameter SR_BASE     = 160,
  parameter RB_BASE     = 16,
  parameter NUM_SPI_SEN = 8
)(
  // Commands from Radio Core
  input clk, input reset,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  output reg rb_stb, input [7:0] rb_addr, output reg [63:0] rb_data,
  input run_rx, input run_tx,
  // Frontend / Daughterboard I/O
  input [31:0] misc_ins, output [31:0] misc_outs,
  input [31:0] fp_gpio_in, output [31:0] fp_gpio_out, output [31:0] fp_gpio_ddr, input [31:0] fp_gpio_fab,
  input [31:0] db_gpio_in, output [31:0] db_gpio_out, output [31:0] db_gpio_ddr, input [31:0] db_gpio_fab,
  output [31:0] leds,
  input spi_clk, input spi_rst, output [NUM_SPI_SEN-1:0] sen, output sclk, output mosi, input miso
);

  localparam [7:0] SR_MISC_OUTS = SR_BASE + 8'd0;
  localparam [7:0] SR_SPI       = SR_BASE + 8'd8;
  localparam [7:0] SR_LEDS      = SR_BASE + 8'd16;
  localparam [7:0] SR_FP_GPIO   = SR_BASE + 8'd24;
  localparam [7:0] SR_DB_GPIO   = SR_BASE + 8'd32;

  localparam [7:0] RB_MISC_IO   = RB_BASE + 0;
  localparam [7:0] RB_SPI       = RB_BASE + 1;
  localparam [7:0] RB_LEDS      = RB_BASE + 2;
  localparam [7:0] RB_DB_GPIO   = RB_BASE + 3;
  localparam [7:0] RB_FP_GPIO   = RB_BASE + 4;

  /********************************************************
  ** Settings registers
  ********************************************************/
  setting_reg #(.my_addr(SR_MISC_OUTS), .width(32)) sr_misc_outs (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(misc_outs), .changed());

  // Readback
  reg spi_readback_stb_hold;
  reg [31:0] spi_readback_hold;
  wire [31:0] spi_readback_sync;
  wire [31:0] fp_gpio_readback, db_gpio_readback;
  always @* begin
    case(rb_addr)
      // Use a latched spi readback stobe so additional readbacks after a SPI transaction will work
      RB_MISC_IO  : {rb_stb, rb_data} <= {spi_readback_stb_hold, {misc_ins, misc_outs}};
      RB_SPI      : {rb_stb, rb_data} <= {spi_readback_stb_hold, {32'd0, spi_readback_hold}};
      RB_LEDS     : {rb_stb, rb_data} <= {spi_readback_stb_hold, {32'd0, leds}};
      RB_DB_GPIO  : {rb_stb, rb_data} <= {spi_readback_stb_hold, {32'd0, db_gpio_readback}};
      RB_FP_GPIO  : {rb_stb, rb_data} <= {spi_readback_stb_hold, {32'd0, fp_gpio_readback}};
      default     : {rb_stb, rb_data} <= {spi_readback_stb_hold, {64'h0BADC0DE0BADC0DE}};
    endcase
  end

  /********************************************************
  ** GPIO
  ********************************************************/
  gpio_atr #(.BASE(SR_LEDS), .WIDTH(32), .FAB_CTRL_EN(0), .DEFAULT_DDR(32'hFFFF_FFFF), .DEFAULT_IDLE(32'd0)) leds_gpio_atr (
    .clk(clk), .reset(reset),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .rx(run_rx), .tx(run_tx),
    .gpio_in(32'd0), .gpio_out(leds), .gpio_ddr(/*unused, assumed output only*/),
    .gpio_out_fab(32'h00000000 /*LEDs don't have fabric control*/), .gpio_sw_rb());

  gpio_atr #(.BASE(SR_FP_GPIO), .WIDTH(32), .FAB_CTRL_EN(1), .DEFAULT_DDR(32'hFFFF_FFFF), .DEFAULT_IDLE(32'd0)) fp_gpio_atr (
    .clk(clk), .reset(reset),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .rx(run_rx), .tx(run_tx),
    .gpio_in(fp_gpio_in), .gpio_out(fp_gpio_out), .gpio_ddr(fp_gpio_ddr),
    .gpio_out_fab(fp_gpio_fab), .gpio_sw_rb(fp_gpio_readback));

  gpio_atr #(.BASE(SR_DB_GPIO), .WIDTH(32), .FAB_CTRL_EN(1), .DEFAULT_DDR(32'hFFFF_FFFF), .DEFAULT_IDLE(32'd0)) db_gpio_atr (
    .clk(clk), .reset(reset),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .rx(run_rx), .tx(run_tx),
    .gpio_in(db_gpio_in), .gpio_out(db_gpio_out), .gpio_ddr(db_gpio_ddr),
    .gpio_out_fab(db_gpio_fab), .gpio_sw_rb(db_gpio_readback));

  /********************************************************
  ** SPI
  ********************************************************/
  wire spi_set_stb;
  wire [7:0] spi_set_addr;
  wire [31:0] spi_set_data;
  wire spi_readback_stb, spi_readback_stb_sync;
  wire [31:0] spi_readback;
  wire spi_clk_int, spi_rst_int;
  generate
    if (USE_SPI_CLK) begin
      axi_fifo_2clk #(.WIDTH(8 + 32), .SIZE(0)) set_2clk_i (
        .reset(reset),
        .i_aclk(clk), .i_tdata({set_addr, set_data}), .i_tvalid(set_stb), .i_tready(),
        .o_aclk(spi_clk), .o_tdata({spi_set_addr, spi_set_data}), .o_tvalid(spi_set_stb), .o_tready(spi_set_stb));

      axi_fifo_2clk #(.WIDTH(32), .SIZE(0)) rb_2clk_i (
        .reset(reset),
        .i_aclk(spi_clk), .i_tdata(spi_readback), .i_tvalid(spi_readback_stb), .i_tready(),
        .o_aclk(clk), .o_tdata(spi_readback_sync), .o_tvalid(spi_readback_stb_sync), .o_tready(spi_readback_stb_sync));

      assign spi_clk_int = spi_clk;
      assign spi_rst_int = spi_rst;
    end else begin
      assign spi_set_stb           = set_stb;
      assign spi_set_addr          = set_addr;
      assign spi_set_data          = set_data;
      assign spi_readback_stb_sync = spi_readback_stb;
      assign spi_readback_sync     = spi_readback;
      assign spi_clk_int           = clk;
      assign spi_rst_int           = reset;
    end
  endgenerate

  // Need to latch spi_readback_stb in case of additional readbacks
  // after the initial spi transaction.
  always @(posedge clk) begin
    if (reset) begin
      spi_readback_stb_hold       <= 1'b1;
    end else begin
      if (set_stb & (set_addr == SR_SPI+2 /* Trigger address */)) begin
        spi_readback_stb_hold     <= 1'b0;
      end else if (spi_readback_stb_sync) begin
        spi_readback_hold         <= spi_readback_sync;
        spi_readback_stb_hold     <= 1'b1;
      end
    end
  end

  // SPI Core instantiation
  // Note: We don't use "ready" because we use readback_stb to backpressure the settings bus
  simple_spi_core #(.BASE(SR_SPI), .WIDTH(NUM_SPI_SEN), .CLK_IDLE(0), .SEN_IDLE(8'hFF)) simple_spi_core (
    .clock(spi_clk_int), .reset(spi_rst_int),
    .set_stb(spi_set_stb), .set_addr(spi_set_addr), .set_data(spi_set_data),
    .readback(spi_readback), .readback_stb(spi_readback_stb), .ready(/* Unused */),
    .sen(sen), .sclk(sclk), .mosi(mosi), .miso(miso),
    .debug());

endmodule
