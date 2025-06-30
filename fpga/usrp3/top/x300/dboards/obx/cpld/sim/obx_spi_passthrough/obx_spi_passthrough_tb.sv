//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: obx_spi_passthrough_tb
//
// Description:
//   Testbench for the OBX CPLD SPI passthrough functionality.
//   It verifies that all LO SPI endpoints can be accessed
//

`define NUM_RUNS_PER_TEST_CASE 30

module obx_spi_passthrough_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgRandom::*;

  //----------------------------------------------------
  // General test setup
  //----------------------------------------------------

  // Clocks and reset
  bit clk, reset;

  sim_clock_gen #(.PERIOD(5.0)) clk_gen (.clk(clk), .rst(reset));

  // SPI passthrough signals
  wire mRXLO1_DATA;
  wire mRXLO1_DCLK;
  wire mRXLO1_DLE;
  wire mRXLO2_DATA;
  wire mRXLO2_DCLK;
  wire mRXLO2_DLE;
  wire mTXLO1_DATA;
  wire mTXLO1_DCLK;
  wire mTXLO1_DLE;
  wire mTXLO2_DATA;
  wire mTXLO2_DCLK;
  wire mTXLO2_DLE;

  logic reset_n;
  // Master engine access signals
  logic set_stb;
  logic [7:0] set_addr;
  logic [31:0] set_data;
  logic spi_ready;
  logic [31:0] readback;
  logic readback_stb;
  // CPLD SPI signals
  logic [2:0] spi_endpoint;
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


  obx_cpld dut (
    .aMCTRL_RESET_N  (reset_n),       //input wire
    .mMCTRL_SEN      (sen),           //input wire
    .mMCTRL_SDI      (mosi),          //input wire
    .MCTRL_SCLK      (sclk),          //input wire
    .aMCTRL_ATR_RX_N (1'b0),          //input wire
    .aMCTRL_ATR_TX_N (1'b0),          //input wire
    .aMCTRL_RX2_EN   (1'b0),          //input wire
    .aMCTRL_ADDR     (spi_endpoint),  //input wire[2:0]
    .mMCTRL_SDO      (miso),          //output wire
    .aTXLO1_LD       (),              //input wire
    .aTXLO2_LD       (),              //input wire
    .aRXLO1_LD       (),              //input wire
    .aRXLO2_LD       (),              //input wire
    .aLED_LOCKD      (),              //output wire
    .aLED_TXD        (),              //output wire
    .aLED_RXD1       (),              //output wire
    .aLED_RXD2       (),              //output wire
    .aLOCKD_RX       (),              //output wire
    .aLOCKD_TX       (),              //output wire
    .mTXLO1_DATA     (mTXLO1_DATA),   //output wire
    .mTXLO1_DCLK     (mTXLO1_DCLK),   //output wire
    .mTXLO1_DLE      (mTXLO1_DLE),    //output wire
    .aTXLO1_NPDRF    (),              //output wire
    .mTXLO2_DATA     (mTXLO2_DATA),   //output wire
    .mTXLO2_DCLK     (mTXLO2_DCLK),   //output wire
    .mTXLO2_DLE      (mTXLO2_DLE),    //output wire
    .aTXLO2_NPDRF    (),              //output wire
    .mRXLO1_DATA     (mRXLO1_DATA),   //output wire
    .mRXLO1_DCLK     (mRXLO1_DCLK),   //output wire
    .mRXLO1_DLE      (mRXLO1_DLE),    //output wire
    .aRXLO1_NPDRF    (),              //output wire
    .mRXLO2_DATA     (mRXLO2_DATA),   //output wire
    .mRXLO2_DCLK     (mRXLO2_DCLK),   //output wire
    .mRXLO2_DLE      (mRXLO2_DLE),    //output wire
    .aRXLO2_NPDRF    (),              //output wire
    .aTXLO1_FSEL1    (),              //output wire
    .aTXLO1_FSEL2    (),              //output wire
    .aTXLO1_FSEL3    (),              //output wire
    .aTXLO1_FSEL4    (),              //output wire
    .aRXLO1_FSEL1    (),              //output wire
    .aRXLO1_FSEL2    (),              //output wire
    .aRXLO1_FSEL3    (),              //output wire
    .aRXLO1_FSEL4    (),              //output wire
    .aTXLB_SEL       (),              //output wire
    .aTXLB_SEL2      (),              //output wire
    .aTXHB_SEL       (),              //output wire
    .aTXHB_SEL2      (),              //output wire
    .aRXLB_SEL       (),              //output wire
    .aRXLB_SEL2      (),              //output wire
    .aRXHB_SEL       (),              //output wire
    .aRXHB_SEL2      (),              //output wire
    .aPWREN_TXDRV    (),              //output wire
    .aPWREN_TXMOD_N  (),              //output wire
    .aPWREN_TXLBMIX  (),              //output wire
    .aPWREN_RXDEMOD_N(),              //output wire
    .aPWREN_RXLBMIX  (),              //output wire
    .aPWREN_RXAMP    (),              //output wire
    .aPWREN_LNA1     (),              //output wire
    .aPWREN_LNA2     (),              //output wire
    .aFE_SEL_CAL_RX2 (),              //output wire
    .aFE_SEL_CAL_TX2 (),              //output wire
    .aPWEN_RX_DOUBLER(),              //output wire
    .aPWEN_TX_DOUBLER(),              //output wire
    .aFE_SEL_RX_LNA1 (),              //output wire
    .aFE_SEL_TRX_RX2 (),              //output wire
    .aFE_SEL_TRX_TX  ()               //output wire
  );

  // RXLO1 SPI slave model
  //-----------------------------------------------------
  spi_slave_model #(
    .Tp(1)  //integer:=1
  ) slave_rx_lo1 (
    .rst ((!reset_n)),  //input wire
    .ss  (mRXLO1_DLE),   //input wire
    .sclk(mRXLO1_DCLK),  //input wire
    .mosi(mRXLO1_DATA),  //input wire
    .miso()             //output reg
  );

  wire [23:0] rx_lo1_data_int = slave_rx_lo1.data; // Internal data for RXLO1

  // RXLO2 SPI slave model
  //-----------------------------------------------------
  spi_slave_model #(
    .Tp(1)  //integer:=1
  ) slave_rx_lo2 (
    .rst ((!reset_n)),  //input wire
    .ss  (mRXLO2_DLE),   //input wire
    .sclk(mRXLO2_DCLK),  //input wire
    .mosi(mRXLO2_DATA),  //input wire
    .miso()             //output reg
  );

  wire [23:0] rx_lo2_data_int = slave_rx_lo2.data; // Internal data for RXLO2

  // TXLO1 SPI slave model
  //-----------------------------------------------------
  spi_slave_model #(
    .Tp(1)  //integer:=1
  ) slave_tx_lo1 (
    .rst ((!reset_n)),  //input wire
    .ss  (mTXLO1_DLE),   //input wire
    .sclk(mTXLO1_DCLK),  //input wire
    .mosi(mTXLO1_DATA),  //input wire
    .miso()             //output reg
  );

  wire [23:0] tx_lo1_data_int = slave_tx_lo1.data; // Internal data for TXLO1

  // TXLO2 SPI slave model
  //-----------------------------------------------------
  spi_slave_model #(
    .Tp(1)  //integer:=1
  ) slave_tx_lo2 (
    .rst ((!reset_n)),  //input wire
    .ss  (mTXLO2_DLE),   //input wire
    .sclk(mTXLO2_DCLK),  //input wire
    .mosi(mTXLO2_DATA),  //input wire
    .miso()             //output reg
  );

  wire [23:0] tx_lo2_data_int = slave_tx_lo2.data; // Internal data for TXLO2

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

  // This task writes a CPLD register at the specified address with the given data.
  // It assumes the CPLD SPI interface is already set up.
  task write_passthrough(input logic [23:0] data);
    @(posedge clk);
    set_stb  <= 1;
    set_addr <= 8'h02;
    set_data <= {8'b0, data};
    @(posedge clk);
    set_stb <= 0;
  endtask

  // Validates that the appropriate SPI endpoint is receives the
  // SPI transaction
  task verify_bypass_access;

    logic [23:0] random_data;
    logic  [2:0] slave_endpoint;
    logic [23:0] rx_lo1_data_expected;
    logic [23:0] rx_lo2_data_expected;
    logic [23:0] tx_lo1_data_expected;
    logic [23:0] tx_lo2_data_expected;

    // Save the current state of the registers
    rx_lo1_data_expected = rx_lo1_data_int;
    rx_lo2_data_expected = rx_lo2_data_int;
    tx_lo1_data_expected = tx_lo1_data_int;
    tx_lo2_data_expected = tx_lo2_data_int;

    // Randomly select a slave endpoint
    slave_endpoint = $urandom_range(0, CPLD_INTERNAL_SPI); // Randomly select a slave endpoint

    if (slave_endpoint == CPLD_INTERNAL_SPI) begin
      test_scratch_register(); // Test the scratch register if the endpoint is CPLD_INTERNAL_SPI
    end
    else begin
      random_data = $urandom_range(0, 24'hFFFFFF);
      // Update the expected data based on the selected slave
      case (slave_endpoint)
        RX_LO1_PASSTHROUGH: begin
          rx_lo1_data_expected = random_data;
        end
        RX_LO2_PASSTHROUGH: begin
          rx_lo2_data_expected = random_data;
        end
        TX_LO1_PASSTHROUGH: begin
          tx_lo1_data_expected = random_data;
        end
        TX_LO2_PASSTHROUGH: begin
          tx_lo2_data_expected = random_data;
        end
      endcase

      // Write to selected slave
      spi_endpoint <= slave_endpoint;
      write_passthrough(random_data);

      while (sen) @(posedge clk);
      while (!spi_ready) @(posedge clk);
      @(posedge clk);
    end

    //Verify current values of the registers
    `ASSERT_ERROR(rx_lo1_data_int == rx_lo1_data_expected,
                  "RXLO1 data did not match expected value");
    `ASSERT_ERROR(rx_lo2_data_int == rx_lo2_data_expected,
                  "RXLO2 data did not match expected value");
    `ASSERT_ERROR(tx_lo1_data_int == tx_lo1_data_expected,
                  "TXLO1 data did not match expected value");
    `ASSERT_ERROR(tx_lo2_data_int == tx_lo2_data_expected,
                  "TXLO2 data did not match expected value");

  endtask

  task test_scratch_register;

    logic [15:0] random_data;
    logic [23:0] readback_data;

    spi_endpoint <= CPLD_INTERNAL_SPI; // Enable CPLD internal SPI access
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

    begin
      spi_endpoint <= CPLD_INTERNAL_SPI; // Enable CPLD internal SPI access
      read_cpld_register(REVISION_REGISTER, readback_data);
      `ASSERT_ERROR(readback_data == {8'b0,
                                      REVISION_MAJOR_VAL[REVISION_MAJOR_SIZE-1:0],
                                      REVISION_MINOR_VAL[REVISION_MINOR_SIZE-1:0]},
                    "Revision register did not match expected value");
    end
  endtask

  initial begin : tb_main

    // Display testbench start message
    test.start_tb("obx_spi_passthrough_tb");

    test.start_test("Wait for Reset", 10us);
    do_reset();
    test.end_test();

    slave_rx_lo1.rx_negedge = 1'b0; // Negedge for RXLO1
    slave_rx_lo2.rx_negedge = 1'b0; // Negedge for RXLO2
    slave_tx_lo1.rx_negedge = 1'b0; // Negedge for TXLO1
    slave_tx_lo2.rx_negedge = 1'b0; // Negedge for TXLO2

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

    test.start_test("Test bypass access");
    repeat (`NUM_RUNS_PER_TEST_CASE) verify_bypass_access();
    test.end_test();

    // All done!
    //-------------------------------------------------------------------------
    // End the testbench
    test.end_tb(0);
    clk_gen.kill();

  end : tb_main

endmodule : obx_spi_passthrough_tb
