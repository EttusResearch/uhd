//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module n3xx_db_fe_core #(
  // Drive SPI core with input spi_clk instead of ce_clk. This is useful if ce_clk is very slow which
  // would cause spi transactions to take a long time. WARNING: This adds a clock crossing FIFO!
  parameter USE_SPI_CLK = 0,
  parameter [7:0] SR_DB_FE_BASE = 160,
  parameter [7:0] RB_DB_FE_BASE = 16,
  parameter WIDTH = 32,
  parameter NUM_SPI_SEN = 8,
  parameter USE_CORRECTION = 0
)(
  input clk, input reset,
  // Commands from Radio Core
  input  set_stb, input [7:0] set_addr, input  [31:0] set_data,
  output rb_stb,  input [7:0] rb_addr,  output [63:0] rb_data,
  // Radio datapath
  input  tx_stb, input [WIDTH-1:0] tx_data_in, output [WIDTH-1:0] tx_data_out, input tx_running,
  input  rx_stb, input [WIDTH-1:0] rx_data_in, output [WIDTH-1:0] rx_data_out, input rx_running,
  // Frontend / Daughterboard I/O
  input [31:0] misc_ins, output [31:0] misc_outs,
  input [31:0] fp_gpio_in, output [31:0] fp_gpio_out, output [31:0] fp_gpio_ddr, input [31:0] fp_gpio_fab,
  input [31:0] db_gpio_in, output [31:0] db_gpio_out, output [31:0] db_gpio_ddr, input [31:0] db_gpio_fab,
  output [31:0] leds,
  input spi_clk, input spi_rst, output [NUM_SPI_SEN-1:0] sen, output sclk, output mosi, input miso
);

  localparam [7:0] SR_TX_FE_BASE = SR_DB_FE_BASE + 8'd48;
  localparam [7:0] SR_RX_FE_BASE = SR_DB_FE_BASE + 8'd64;

  db_control #(
    .USE_SPI_CLK(USE_SPI_CLK), .SR_BASE(SR_DB_FE_BASE), .RB_BASE(RB_DB_FE_BASE),
    .NUM_SPI_SEN(NUM_SPI_SEN)
  ) db_control_i (
    .clk(clk), .reset(reset),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .rb_stb(rb_stb), .rb_addr(rb_addr), .rb_data(rb_data),
    .run_rx(rx_running), .run_tx(tx_running),
    .misc_ins(misc_ins), .misc_outs(misc_outs),
    .fp_gpio_in(fp_gpio_in), .fp_gpio_out(fp_gpio_out), .fp_gpio_ddr(fp_gpio_ddr), .fp_gpio_fab(fp_gpio_fab),
    .db_gpio_in(db_gpio_in), .db_gpio_out(db_gpio_out), .db_gpio_ddr(db_gpio_ddr), .db_gpio_fab(db_gpio_fab),
    .leds(leds),
    .spi_clk(spi_clk), .spi_rst(spi_rst), .sen(sen), .sclk(sclk), .mosi(mosi), .miso(miso)
  );

  generate
    if (USE_CORRECTION == 1) begin
      tx_frontend_gen3 #(
        .SR_OFFSET_I(SR_TX_FE_BASE + 0), .SR_OFFSET_Q(SR_TX_FE_BASE + 1),.SR_MAG_CORRECTION(SR_TX_FE_BASE + 2),
        .SR_PHASE_CORRECTION(SR_TX_FE_BASE + 3), .SR_MUX(SR_TX_FE_BASE + 4),
        .BYPASS_DC_OFFSET_CORR(0), .BYPASS_IQ_COMP(0),
        .DEVICE("7SERIES")
      ) tx_fe_corr_i (
        .clk(clk), .reset(reset),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .tx_stb(tx_stb), .tx_i(tx_data_in[31:16]), .tx_q(tx_data_in[15:0]),
        .dac_stb(), .dac_i(tx_data_out[31:16]), .dac_q(tx_data_out[15:0])
      );

      rx_frontend_gen3 #(
        .SR_MAG_CORRECTION(SR_RX_FE_BASE + 0), .SR_PHASE_CORRECTION(SR_RX_FE_BASE + 1), .SR_OFFSET_I(SR_RX_FE_BASE + 2),
        .SR_OFFSET_Q(SR_RX_FE_BASE + 3), .SR_IQ_MAPPING(SR_RX_FE_BASE + 4), .SR_HET_PHASE_INCR(SR_RX_FE_BASE + 5),
        .BYPASS_DC_OFFSET_CORR(0), .BYPASS_IQ_COMP(0), .BYPASS_REALMODE_DSP(1),
        .DEVICE("7SERIES")
      ) rx_fe_corr_i (
        .clk(clk), .reset(reset), .sync_in(),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .adc_stb(rx_stb), .adc_i(rx_data_in[31:16]), .adc_q(rx_data_in[15:0]),
        .rx_stb(), .rx_i(rx_data_out[31:16]), .rx_q(rx_data_out[15:0])
      );
    end else begin
      //TODO: Flesh out this module
      assign tx_data_out = tx_data_in;
      assign rx_data_out = rx_data_in;
    end
  endgenerate

endmodule
