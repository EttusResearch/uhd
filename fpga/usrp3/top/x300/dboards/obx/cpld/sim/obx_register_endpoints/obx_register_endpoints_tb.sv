//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: obx_register_endpoints_tb
//
// Description:
//   Testbench for the obx_register_endpoints module.
//   This testbench simulates SPI transactions to the CPLD's register endpoints,
//   specifically the TX and RX control registers. It verifies that the registers
//   can be written to correctly, and that invalid writes do not change the register
//   values.
//

`define NUM_RUNS_PER_TEST_CASE 10


module obx_register_endpoints_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgRandom::*;

  //----------------------------------------------------
  // General test setup
  //----------------------------------------------------

  // Clocks and reset
  bit clk, reset;

  sim_clock_gen #(.PERIOD(5.0)) clk_gen (.clk(clk), .rst(reset));

  logic reset_n;
  // Master engine access signals
  logic set_stb;
  logic [7:0] set_addr;
  logic [31:0] set_data;
  logic spi_ready;
  logic [31:0] readback;
  logic readback_stb;
  // CPLD SPI signals
  logic internal_spi_access;
  logic mosi;
  logic miso;
  logic sclk;
  logic sen;


  //----------------------------------------------------
  // Topology
  //----------------------------------------------------

  // Instantiate the SPI engine
  // only one slave is needed for this master engine,
  // which is the CPLD SPI interface.
  simple_spi_core #(
    .WIDTH(1)
  )spi_engine (
    .reset(reset),
    .clock(clk),
    .set_stb(set_stb),
    .set_addr(set_addr),
    .set_data(set_data),
    .ready(spi_ready),
    .readback(readback),
    .readback_stb(readback_stb),
    .sen(sen),
    .mosi(mosi),
    .miso(miso),
    .sclk(sclk),
    .debug()
  );

  assign reset_n = ~reset;

  // CPLD control registers
  logic [23:0] tx_fe_ctrl;
  logic [23:0] rx_fe_ctrl;

  obx_register_endpoints dut (
    .reset_n(reset_n),
    .spi_cs(sen),
    .spi_sdi(mosi),
    .spi_sdo(miso),
    .spi_clk(sclk),
    .internal_spi_access(internal_spi_access),
    .tx_fe_ctrl(tx_fe_ctrl),
    .rx_fe_ctrl(rx_fe_ctrl)
  );

  `include "../../regmap/obx_cpld_regs_utils.vh"

  // Local reset function
  task do_reset (input int rst_cyc = 100);
    clk_gen.clk_wait_f();
    clk_gen.reset();
    wait(reset == 0);
    repeat (rst_cyc) clk_gen.clk_wait_f();
  endtask : do_reset

  // This task sets up the SPI engine with the necessary clock divider and SPI settings
  // to communicate with the CPLD.
  task set_up_engine;
    // Set up clock divider
    @(posedge clk);
    set_stb   <= 1;
    set_addr  <= 8'h00; // CLK_DIVIDER
    set_data  <= 32'd30;
    @(posedge clk);
    set_stb <= 0;
    // Set up clock divider
    @(posedge clk);
    set_stb   <= 1;
    set_addr  <= 8'h01; // SPI_SETTINGS
    set_data  <= {
      1'b0,   // dataout_edge
      1'b0,   // datain_edge
      6'd32,  // num_bits
      24'b1   // slave select
    };
    @(posedge clk);
    set_stb <= 0;
  endtask

  // This task writes a CPLD register at the specified address with the given data.
  // It assumes the CPLD SPI interface is already set up.
  task write_cpld_register(input logic [23:0] data,
                           input logic [6:0] addr);
    @(posedge clk);
    set_stb   <= 1;
    set_addr  <= 8'h02;
    set_data  <= {
      1'b0, // RW
      addr,
      data
    };
    @(posedge clk);
    set_stb <= 0;
  endtask

  task write_tx_control(input logic [23:0] data);

    write_cpld_register(data, TX_CONTROL);

  endtask

  task write_rx_control(input logic [23:0] data);

    write_cpld_register(data, RX_CONTROL);

  endtask

  task write_scratch_register(input logic [15:0] data);

    write_cpld_register({8'h00, data}, SCRATCH_REGISTER);

  endtask

  // This task reads a CPLD register at the specified address and returns the readback data.
  task read_cpld_register(input  logic [6:0]  addr,
                          output logic [23:0] readback_data);

    @(posedge clk);
    set_stb   <= 1;
    set_addr  <= 8'h02;
    set_data  <= {
      1'b1,
      addr,
      24'b0}; // RW=1 for read
    @(posedge clk);
    set_stb <= 0;

    // Wait for the SPI engine to be ready
    while (!readback_stb) @(posedge clk);
    // Read the data from the SPI engine
    readback_data = readback[23:0];

  endtask

  // This task tests writing to the TX control register with a random value
  // and verifies that the register is updated correctly.
  task test_random_tx;

    logic [23:0] random_data;
    logic [23:0] readback_data;

    internal_spi_access <= 1; // Enable CPLD internal SPI access
    random_data = $urandom_range(0, 24'hFFFFFF);
    write_tx_control(random_data);
    while (sen) @(posedge clk);
    while (!spi_ready) @(posedge clk);
    @(posedge clk);
    `ASSERT_ERROR(tx_fe_ctrl == random_data, "TX control register did not match expected value");
    // Read back the TX control register to verify
    read_cpld_register(TX_CONTROL, readback_data);
    `ASSERT_ERROR(readback_data == random_data, "TX control register readback did not match expected value");

  endtask

  // This task tests writing to the RX control register with a random value
  // and verifies that the register is updated correctly.
  task test_random_rx;

    logic [23:0] random_data;
    logic [23:0] readback_data;

    internal_spi_access <= 1; // Enable CPLD internal SPI access
    random_data = $urandom_range(0, 24'hFFFFFF);
    write_rx_control(random_data);
    while (sen) @(posedge clk);
    while (!spi_ready) @(posedge clk);
    @(posedge clk);
    `ASSERT_ERROR(rx_fe_ctrl == random_data, "RX control register did not match expected value");
    // Read back the RX control register to verify
    read_cpld_register(RX_CONTROL, readback_data);
    `ASSERT_ERROR(readback_data == random_data, "RX control register readback did not match expected value");

  endtask

  // This task tests validates that each register keeps its value when a write that does not
  // match the expected address is attempted.
  task verify_invalid_address;

    logic [23:0] random_data;
    logic [23:0] tx_backup;
    logic [23:0] rx_backup;

    random_data = $urandom_range(0, 24'hFFFFFF);
    test_random_tx();
    tx_backup = tx_fe_ctrl;
    test_random_rx();
    rx_backup = rx_fe_ctrl;

    // Write to an invalid address
    write_cpld_register(random_data, 7'h7F); // Invalid address

    while (sen) @(posedge clk);
    while (!spi_ready) @(posedge clk);
    @(posedge clk);

    // Verify that the registers were not changed
    `ASSERT_ERROR(tx_fe_ctrl == tx_backup, "TX control register changed on invalid write");
    `ASSERT_ERROR(rx_fe_ctrl == rx_backup, "RX control register changed on invalid write");

  endtask

  // This task tests validates that each register keeps its value when a write that does not
  // target the CPLD internal endpoints is attempted.
  task verify_bypass_access;

    logic [23:0] random_data;
    logic [23:0] tx_backup;
    logic [23:0] rx_backup;

    // Load new data onto the registers
    random_data = $urandom_range(0, 24'hFFFFFF);
    test_random_tx();
    tx_backup = tx_fe_ctrl;
    test_random_rx();
    rx_backup = rx_fe_ctrl;

    // Write to a different slave
    internal_spi_access <= 0; // Disable CPLD internal SPI access
    write_cpld_register(random_data, TX_CONTROL);
    write_cpld_register(random_data, RX_CONTROL);

    while (sen) @(posedge clk);
    while (!spi_ready) @(posedge clk);
    @(posedge clk);

    // Verify that the registers were not changed
    `ASSERT_ERROR(tx_fe_ctrl == tx_backup, "TX control register changed on invalid write");
    `ASSERT_ERROR(rx_fe_ctrl == rx_backup, "RX control register changed on invalid write");

  endtask

  task test_scratch_register;

    logic [15:0] random_data;
    logic [23:0] readback_data;

    internal_spi_access <= 1; // Enable CPLD internal SPI access
    random_data = $urandom_range(0, 16'hFFFF);
    write_scratch_register(random_data);
    while (sen) @(posedge clk);
    while (!spi_ready) @(posedge clk);
    @(posedge clk);
    // Read back the scratch register to verify
    read_cpld_register(SCRATCH_REGISTER, readback_data);
    `ASSERT_ERROR(readback_data == {8'b0, random_data}, "Scratch register readback did not match expected value");

  endtask

  task check_revision_register;

    logic [23:0] readback_data;

    internal_spi_access <= 1; // Enable CPLD internal SPI access
    read_cpld_register(REVISION_REGISTER, readback_data);
    `ASSERT_ERROR(readback_data == {8'b0,
                                    REVISION_MAJOR_VAL[REVISION_MAJOR_SIZE-1:0],
                                    REVISION_MINOR_VAL[REVISION_MINOR_SIZE-1:0]},
                  "Revision register did not match expected value");

  endtask

  initial begin : tb_main

    // Display testbench start message
    test.start_tb("obx_register_endpoints_tb");

    test.start_test("Wait for Reset", 10us);
    do_reset();
    test.end_test();

    repeat (10) @(posedge clk); // Wait for reset to settle
    test.start_test("Set up SPI engine");
    set_up_engine();
    test.end_test();

    test.start_test("Test CPLD revision register");
    check_revision_register();
    test.end_test();

    test.start_test("Test scratch register");
    repeat (`NUM_RUNS_PER_TEST_CASE) test_scratch_register();
    test.end_test();

    test.start_test("Test TX register endpoint");
    repeat (`NUM_RUNS_PER_TEST_CASE) test_random_tx();
    test.end_test();

    test.start_test("Test RX register endpoint");
    repeat (`NUM_RUNS_PER_TEST_CASE) test_random_rx();
    test.end_test();

    test.start_test("Test invalid address write");
    repeat (`NUM_RUNS_PER_TEST_CASE) verify_invalid_address();
    test.end_test();

    test.start_test("Test bypass access");
    repeat (`NUM_RUNS_PER_TEST_CASE) verify_bypass_access();
    test.end_test();

    // All done!
    //-------------------------------------------------------------------------
    // End the testbench
    test.end_tb(0);
    clk_gen.kill();

  end : tb_main

endmodule : obx_register_endpoints_tb
