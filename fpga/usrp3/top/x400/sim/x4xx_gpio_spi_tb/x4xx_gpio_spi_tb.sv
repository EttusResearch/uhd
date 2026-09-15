//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_gpio_spi_tb
//
// Description: X4xx GPIO SPI transaction testbench for four peripheral slots.
//
// Validates GPIO pin configuration, CS idle state, and SPI write/readback for
// 20-, 32-, 40-, and 64-bit transactions. Tests cover consecutive transfers,
// switching peripheral slots and clock dividers, randomized payloads, the
// status and extended response registers, and SPI timing checks.
//
// This testbench does not cover invalid register configurations, unsupported
// transaction lengths, concurrent transactions.
// The DUT does not properly handle these unsupported scenarios.
//
// For testing X410, run with VLOG_ARGS='+define+X410' SVLOG_ARGS='+define+X410'
//

`default_nettype none

module x4xx_gpio_spi_tb;
  `include "test_exec.svh"
  import PkgTestExec::*;
  import ctrlport_pkg::*;
  import ctrlport_bfm_pkg::*;
  import spi_slave_bfm_pkg::*;

  import XmlSvPkgDIG_IFC_REGMAP::*;

  localparam int unsigned NUM_PERIPHERALS = 4;

  // Compile-time BFM widths for the peripheral slots.
  localparam int unsigned TRANSACTION_LENGTH_0 = 64;
  localparam int unsigned TRANSACTION_LENGTH_1 = 40;
  localparam int unsigned TRANSACTION_LENGTH_2 = 32;
  localparam int unsigned TRANSACTION_LENGTH_3 = 20;


  localparam time CLK_2X_PERIOD = 12ns;
  `ifdef X410
    localparam bit DOUBLE_CLOCK = 1;
  `else
    localparam bit DOUBLE_CLOCK = 0;
  `endif
  localparam int unsigned CLK_DIV_1 = 2;
  localparam int unsigned CLK_DIV_2 = 6;
  // SPI edge toggle: 0 updates on falling and samples on rising; 1 reverses it.
  localparam bit SPI_UPDATE_ON_RISING = 1;
  localparam int unsigned SDI_PIN = 1;
  localparam int unsigned SDO_PIN = 2;
  localparam int unsigned CLK_PIN = 3;
  localparam int unsigned CS_PINS [0:NUM_PERIPHERALS-1] = '{0, 4, 5, 6};
  localparam int unsigned TRANSACTION_LENGTHS [0:NUM_PERIPHERALS-1] = '{64, 40, 32, 20};

  // The DUT uses a control clock and a 2x SPI engine clock.
  logic clk, clkx2, reset;
  sim_clock_gen #(.PERIOD(CLK_2X_PERIOD*2), .AUTOSTART(0)) clk_gen (
    .clk(clk), .rst(reset));
  sim_clock_gen #(.PERIOD(CLK_2X_PERIOD), .AUTOSTART(0)) clk_gen_x2 (
    .clk(clkx2), .rst());

  // GPIO interface signals
  logic [31:0] gpio_out;
  logic [31:0] gpio_ddr;
  wire logic [31:0] gpio_in;

  // SPI interfaces for the peripheral slots.
  spi_if spi_if [0:NUM_PERIPHERALS-1] ();

  // All slots share SCK, MOSI, and MISO GPIO pins; each has its own CS pin.
  for (genvar spi_idx = 0; spi_idx < NUM_PERIPHERALS; spi_idx++) begin : gen_spi_wiring
    assign spi_if[spi_idx].cs_n = gpio_out[CS_PINS[spi_idx]];
    assign spi_if[spi_idx].sck  = gpio_out[CLK_PIN];
    assign spi_if[spi_idx].mosi = gpio_out[SDO_PIN];
    assign gpio_in[SDI_PIN]     = spi_if[spi_idx].miso;
  end

  // The BFMs must remain explicit because length is a class specialization parameter.
  spi_slave_bfm #(.TRANSACTION_LENGTH(TRANSACTION_LENGTH_0)) spi_bfm_0 = new(spi_if[0]);
  spi_slave_bfm #(.TRANSACTION_LENGTH(TRANSACTION_LENGTH_1)) spi_bfm_1 = new(spi_if[1]);
  spi_slave_bfm #(.TRANSACTION_LENGTH(TRANSACTION_LENGTH_2)) spi_bfm_2 = new(spi_if[2]);
  spi_slave_bfm #(.TRANSACTION_LENGTH(TRANSACTION_LENGTH_3)) spi_bfm_3 = new(spi_if[3]);

  // Each slot has an independent monitor so timing expectations follow the
  // selected transaction clock divider.
  spi_timing_monitor_bfm spi_timing_mon [0:NUM_PERIPHERALS-1];

  for (genvar spi_idx = 0; spi_idx < NUM_PERIPHERALS; spi_idx++) begin : gen_spi_timing_monitor
    initial begin
      spi_timing_mon[spi_idx] = new(
        .iface(spi_if[spi_idx]), .half_per_time(get_half_per_time(CLK_DIV_1)),
        .cs_hold_time(0), .cs_to_first_rise_time(0), .cs_guard_time(0));
    end
  end

  // Control-port connection to the GPIO SPI DUT.
  logic [1:0] spi_ctrlport_resp_status;
  ctrlport_if gpio_ctrlport_if (.clk(clk), .rst(reset));
  ctrlport_bfm gpio_ctrlport_bfm = new(gpio_ctrlport_if);

  x4xx_gpio_spi dut (
    .ctrlport_clk          (clk),
    .ctrlport_clk_2x       (clkx2),
    .ctrlport_rst          (reset),
    .s_ctrlport_req_wr     (gpio_ctrlport_if.req.wr),
    .s_ctrlport_req_rd     (gpio_ctrlport_if.req.rd),
    .s_ctrlport_req_addr   (gpio_ctrlport_if.req.addr),
    .s_ctrlport_req_data   (gpio_ctrlport_if.req.data),
    .s_ctrlport_resp_ack   (gpio_ctrlport_if.resp.ack),
    .s_ctrlport_resp_status(spi_ctrlport_resp_status),
    .s_ctrlport_resp_data  (gpio_ctrlport_if.resp.data),
    .gpio_out              (gpio_out),
    .gpio_ddr              (gpio_ddr),
    .gpio_in               (gpio_in)
  );

  assign gpio_ctrlport_if.resp.status = ctrlport_status_t'(spi_ctrlport_resp_status);

  // Return the SPI half-period expected by the timing monitor. X410 runs the
  // SPI engine at twice the clock rate used by the other X4xx variants.
  function automatic time get_half_per_time(input int unsigned clk_div);
    return (clk_div + 1) * CLK_2X_PERIOD * (DOUBLE_CLOCK ? 1 : 2);
  endfunction

  function automatic logic [63:0] get_response_mask(
    input int unsigned transaction_length
  );
    // SPI_FULL_RESPONSE_MSB exposes bits 62:31; bit 63 is not returned.
    return (64'h1 << ((transaction_length < 63) ? transaction_length : 63)) - 1;
  endfunction

  function automatic logic [31:0] get_status_mask(
    input int unsigned transaction_length
  );
    // SPI_STATUS exposes at most 31 response bits below SPI_READY.
    return (32'h1 << ((transaction_length < 31) ? transaction_length : 31)) - 1;
  endfunction

  // Program the pin mapping and fixed transaction length for one SPI slot.
  task automatic configure_peripheral(input int unsigned selected_peripheral);
    SPI_SETUP peripheral_config_fields;

    peripheral_config_fields = '0;
    peripheral_config_fields.MOSI_EDGE  = SPI_UPDATE_ON_RISING;
    peripheral_config_fields.MISO_EDGE  = !SPI_UPDATE_ON_RISING;
    peripheral_config_fields.SPI_LENGTH = TRANSACTION_LENGTHS[selected_peripheral];
    peripheral_config_fields.SLAVE_CS   = CS_PINS[selected_peripheral];
    peripheral_config_fields.SLAVE_MISO = SDI_PIN;
    peripheral_config_fields.SLAVE_MOSI = SDO_PIN;
    peripheral_config_fields.SLAVE_CLK  = CLK_PIN;
    gpio_ctrlport_bfm.write(kSPI_SLAVE_CONFIG(selected_peripheral), peripheral_config_fields);
  endtask

  task automatic configure_transaction(
    input int unsigned selected_peripheral,
    input int unsigned clk_div
  );
    SPI_TRANSACTION_CONFIG transaction_config_fields;

    // The selected selected_peripheral and divider apply to the next transaction.
    transaction_config_fields = '0;
    transaction_config_fields.SPI_CLK_DIV      = clk_div;
    transaction_config_fields.SPI_SLAVE_SELECT = selected_peripheral;
    gpio_ctrlport_bfm.write(kSPI_TRANSACTION_CONFIG, transaction_config_fields);
    spi_timing_mon[selected_peripheral].set_timing(
      .half_per_time(get_half_per_time(clk_div)),
      .cs_hold_time(0), .cs_to_first_rise_time(0), .cs_guard_time(0));
  endtask

  task automatic queue_bfm_transaction(
    input int unsigned selected_peripheral,
    input logic [63:0] write_data,
    input logic [63:0] expected_response
  );
    // Keep the typed BFM dispatch in one place for simulator portability.
    case (selected_peripheral)
      0: spi_bfm_0.async_write_read(write_data, expected_response);
      1: spi_bfm_1.async_write_read(write_data, expected_response);
      2: spi_bfm_2.async_write_read(write_data, expected_response);
      3: spi_bfm_3.async_write_read(write_data, expected_response);
      default: $fatal(1, "Invalid SPI peripheral index: %0d", selected_peripheral);
    endcase
  endtask

  task automatic wait_for_cs(input int unsigned selected_peripheral);
    // Dynamic bit-select event controls are avoided for Vivado XSim.
    case (selected_peripheral)
      0: @(posedge gpio_out[CS_PINS[0]]);
      1: @(posedge gpio_out[CS_PINS[1]]);
      2: @(posedge gpio_out[CS_PINS[2]]);
      3: @(posedge gpio_out[CS_PINS[3]]);
      default: $fatal(1, "Invalid SPI peripheral index: %0d", selected_peripheral);
    endcase
  endtask

  task automatic get_bfm_error_count(input int unsigned selected_peripheral, output int count);
    case (selected_peripheral)
      0: count = spi_bfm_0.get_error_count();
      1: count = spi_bfm_1.get_error_count();
      2: count = spi_bfm_2.get_error_count();
      3: count = spi_bfm_3.get_error_count();
      default: $fatal(1, "Invalid SPI peripheral index: %0d", selected_peripheral);
    endcase
  endtask

  task automatic start_bfms();
    // Start all models, including inactive slots. Inactive CS lines remain
    // high and therefore do not consume queued transactions.
    spi_bfm_0.run();
    spi_bfm_1.run();
    spi_bfm_2.run();
    spi_bfm_3.run();
    for (int unsigned spi_idx = 0; spi_idx < NUM_PERIPHERALS; spi_idx++) begin
      spi_timing_mon[spi_idx].run();
    end
  endtask

  task automatic start_transaction(
    input int unsigned transaction_length,
    input logic [63:0] write_data
  );
    if (transaction_length > 32) begin
      gpio_ctrlport_bfm.write(
        kSPI_TRANSACTION_LSB,
        (write_data << (64 - transaction_length)) & kSPI_TRANSACTION_LSBMask);
      gpio_ctrlport_bfm.write(
        kSPI_TRANSACTION_GO,
        (write_data >> (transaction_length - 32)) & kSPI_TRANSACTION_GOMask);
    end else begin
      gpio_ctrlport_bfm.write(
        kSPI_TRANSACTION_GO,
        (write_data << (32 - transaction_length)) & kSPI_TRANSACTION_GOMask);
    end
  endtask

  task automatic execute_transaction(
    input int unsigned selected_peripheral,
    input logic [63:0] write_data,
    input logic [63:0] expected_response
  );
    int unsigned spi_status_read;
    int unsigned spi_response_msb;
    int bfm_error_count;
    int timing_error_count;
    int unsigned transaction_length;
    logic [62:0] full_response;
    logic [63:0] response_mask;
    logic [31:0] status_mask;


    transaction_length = TRANSACTION_LENGTHS[selected_peripheral];
    response_mask      = get_response_mask(transaction_length);
    status_mask        = get_status_mask(transaction_length);
    queue_bfm_transaction(selected_peripheral, write_data, expected_response);
    start_transaction(transaction_length, write_data);
    wait_for_cs(selected_peripheral);
    repeat (20) @(posedge clk);
    gpio_ctrlport_bfm.read(kSPI_STATUS, spi_status_read);

    test.assert_error(
      spi_status_read[kSPI_READY],
      $sformatf("SPI%0d transaction not complete", transaction_length));
    test.assert_error(
      (spi_status_read & status_mask) == (expected_response & status_mask),
      $sformatf("SPI%0d status response failed. Expected 0x%0h, got 0x%0h",
        transaction_length, expected_response & status_mask,
        spi_status_read & status_mask));

    full_response = {32'b0, spi_status_read[30:0]};
    if (transaction_length > 31) begin
      gpio_ctrlport_bfm.read(kSPI_FULL_RESPONSE_MSB, spi_response_msb);
      full_response[62:31] = spi_response_msb;
    end
    test.assert_error(
      (full_response & response_mask) == (expected_response & response_mask),
      $sformatf("SPI%0d full response failed. Expected 0x%0h, got 0x%0h, mask 0x%0h",
        transaction_length, expected_response, full_response, response_mask));

    get_bfm_error_count(selected_peripheral, bfm_error_count);
    test.assert_error(
      bfm_error_count == 0,
      $sformatf("SPI%0d BFM reported %0d errors", transaction_length, bfm_error_count));
    timing_error_count = spi_timing_mon[selected_peripheral].get_error_count();
    test.assert_error(
      timing_error_count == 0,
      $sformatf("SPI%0d timing monitor reported %0d errors",
        transaction_length, timing_error_count));
  endtask

  initial begin : main
    string tb_name;
    int unsigned selected_peripheral;
    int unsigned previous_peripheral;

    `ifdef X410
      tb_name = "x410_gpio_spi_tb";
    `else
      tb_name = "x4xx_gpio_spi_tb";
    `endif
    test.start_tb(tb_name);
    gpio_ctrlport_bfm.run();
    clk_gen.start();
    clk_gen_x2.start();
    clk_gen.reset(50);
    //-------------------------------------------------------------------------

    test.start_test("Full configuration and transaction", 100us);

    for (int unsigned selected_peripheral = 0; selected_peripheral < NUM_PERIPHERALS; selected_peripheral++) begin
      configure_peripheral(selected_peripheral);
      test.assert_error(
        gpio_out[CS_PINS[selected_peripheral]] === 1'b1,
        $sformatf("SPI%0d CS is not high after configuration", TRANSACTION_LENGTHS[selected_peripheral]));
      test.assert_error(
        gpio_ddr[CS_PINS[selected_peripheral]] === 1'b1,
        $sformatf("SPI%0d CS is not configured as an output", TRANSACTION_LENGTHS[selected_peripheral]));
    end
    test.assert_error(gpio_ddr[CLK_PIN] === 1'b1, "SPI clock is not configured as an output");
    test.assert_error(gpio_ddr[SDO_PIN] === 1'b1, "SPI SDO is not configured as an output");
    test.assert_error(gpio_ddr[SDI_PIN] === 1'b0, "SPI SDI is not configured as an input");

    // Only start SPI BFM after configuration is done (CS is high)
    start_bfms();

    configure_transaction(0, CLK_DIV_1);
    execute_transaction(0, 64'hFAFAFAFA00000001, 64'h0123456789ABCDEF);
    //Consecutive transaction with new divider
    configure_transaction(0, CLK_DIV_2);
    execute_transaction(0, 64'hFAFAFAFA00000001, 64'h0123456789ABCDEF);
    //Transaction on different peripheral and new divider
    configure_transaction(2, CLK_DIV_1);
    execute_transaction(2, 64'hBA4B0EB4, 64'h98765432);
    //Transaction on different peripheral and new divider
    configure_transaction(1, CLK_DIV_1);
    execute_transaction(1, 64'h0000000A010FA7BF, 64'h000000DCBA987654);
    //Consecutive transaction without changing the clock divider
    configure_transaction(3, CLK_DIV_1);
    execute_transaction(3, 64'h00000000000FA7BF, 64'h0000000000054321);
    execute_transaction(3, 64'h00000000000FA00F, 64'h0000000000054001);

    test.end_test();
    //-------------------------------------------------------------------------

    test.start_test("Random transactions", 2ms);
    previous_peripheral = NUM_PERIPHERALS;
    for (int unsigned transaction = 0; transaction < 100; transaction++) begin
      selected_peripheral = $urandom_range(NUM_PERIPHERALS - 1, 0);
      if (selected_peripheral != previous_peripheral) begin
        configure_transaction(selected_peripheral, CLK_DIV_2);
      end
      execute_transaction(selected_peripheral, {$urandom(), $urandom()}, {$urandom(), $urandom()});
      previous_peripheral = selected_peripheral;
    end
    test.end_test();

    test.end_tb();
  end

endmodule

`default_nettype wire
