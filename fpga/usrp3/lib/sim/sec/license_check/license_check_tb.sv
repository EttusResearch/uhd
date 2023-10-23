//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//
//   Testbench for license_check and the sha256 module
//

`default_nettype none

module license_check_tb ();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;

  localparam real CLK_PERIOD = 10.0; // ns
  localparam [95:0] SERIAL   = 96'h012345678ABCDEF00C0DE0BB;
  localparam int PKEY_W      = 312;
  localparam [PKEY_W-1:0] PRIVATE_KEY = 312'h5EC;
  localparam int FEATURE0    = 32'hC0DE;
  localparam int FEATURE1    = 32'hF00D;

  localparam FEATURE_ADDR = 0;
  localparam KEY_ADDR     = 4;
  localparam RB_ADDR      = 8;

  // Note: hashes are calculated with gen_sha256.py
  // Feature 0 hash: 256'h80f122aad504a1ec7ae8f37b4eee414cf1cf32ecc543ee2e1d4e1d4ae59eb41b
  // Individual words:
  localparam hash0_0 = 32'h80f122aa;
  localparam hash0_1 = 32'hd504a1ec;
  localparam hash0_2 = 32'h7ae8f37b;
  localparam hash0_3 = 32'h4eee414c;
  localparam hash0_4 = 32'hf1cf32ec;
  localparam hash0_5 = 32'hc543ee2e;
  localparam hash0_6 = 32'h1d4e1d4a;
  localparam hash0_7 = 32'he59eb41b;
  // Feature 1 hash: 256'hefd81f78a5fdee0a16ed039cfa51db769a8da088328f8ed73d6af85ba1ce57d4
  // Individual words:
  localparam hash1_0 = 32'hefd81f78;
  localparam hash1_1 = 32'ha5fdee0a;
  localparam hash1_2 = 32'h16ed039c;
  localparam hash1_3 = 32'hfa51db76;
  localparam hash1_4 = 32'h9a8da088;
  localparam hash1_5 = 32'h328f8ed7;
  localparam hash1_6 = 32'h3d6af85b;
  localparam hash1_7 = 32'ha1ce57d4;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;
  bit rst_dut = 0;

  sim_clock_gen #(.PERIOD(CLK_PERIOD))
    clk_gen (.clk(clk), .rst(rst));

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic           i_ctrlport_req_rd   = 1'b0;
  logic           i_ctrlport_req_wr   = 1'b0;
  logic [19:0]    i_ctrlport_req_addr = 20'h0;
  logic [31:0]    i_ctrlport_req_data = 32'h0;
  logic           o_ctrlport_resp_ack;
  logic [1:0]     o_ctrlport_resp_status;
  logic [31:0]    o_ctrlport_resp_data;

  logic [1:0]     feature_enabled;

  license_check #(
    .BASE_ADDR    (0),
    .PKEY_W       (PKEY_W),
    .SERIAL_W     (96),
    .NUM_FEATURES (2),
    .FEATURE_IDS  ({FEATURE1, FEATURE0}),
    .PRIVATE_KEY  (PRIVATE_KEY)
  ) license_check_dut (
    .clk(clk),
    .rst(rst | rst_dut),

    .serial(SERIAL),

    .s_ctrlport_req_wr      (i_ctrlport_req_wr),
    .s_ctrlport_req_rd      (i_ctrlport_req_rd),
    .s_ctrlport_req_addr    (i_ctrlport_req_addr),
    .s_ctrlport_req_data    (i_ctrlport_req_data),
    .s_ctrlport_resp_ack    (o_ctrlport_resp_ack),
    .s_ctrlport_resp_status (o_ctrlport_resp_status),
    .s_ctrlport_resp_data   (o_ctrlport_resp_data),

    .feature_enabled(feature_enabled)
  );


  //--------------------------------
  // Tasks
  //--------------------------------

  // Write a data word via ctrlport transaction. Will run into timeout if no
  // ACK is returned.
  task automatic ctrlport_poke32(int addr, int data);
    i_ctrlport_req_addr <= addr;
    i_ctrlport_req_data <= data;
    i_ctrlport_req_wr   <= 1'b1;
    @(posedge clk);
    i_ctrlport_req_wr   <= 1'b0;
    while (!o_ctrlport_resp_ack) @(posedge clk);
    test.assert_error(
      !o_ctrlport_resp_status,
      $sformatf("CtrlPort write response status not zero (%d)!",
                o_ctrlport_resp_status));
  endtask

  task automatic ctrlport_poke32_status(int addr, int data, int exp_status);
    i_ctrlport_req_addr <= addr;
    i_ctrlport_req_data <= data;
    i_ctrlport_req_wr   <= 1'b1;
    @(posedge clk);
    i_ctrlport_req_wr   <= 1'b0;
    while (!o_ctrlport_resp_ack) @(posedge clk);
    test.assert_error(
      o_ctrlport_resp_status == exp_status,
      $sformatf("CtrlPort write response status not %d (%d)!",
                exp_status, o_ctrlport_resp_status));
  endtask

  task automatic ctrlport_peek32(int addr, int expected_data);
    i_ctrlport_req_addr <= addr;
    i_ctrlport_req_rd   <= 1'b1;
    @(posedge clk);
    i_ctrlport_req_rd   <= 1'b0;
    while (!o_ctrlport_resp_ack) @(posedge clk);
    test.assert_error(
      !o_ctrlport_resp_status,
      $sformatf("CtrlPort read response status not zero (%d)!",
                o_ctrlport_resp_status));
    test.assert_error(
      o_ctrlport_resp_data == expected_data,
      $sformatf("Incorrect peek value: %x! Expected %x.",
                o_ctrlport_resp_data, expected_data));
  endtask



  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    tb_name = $sformatf("device_dna_ctrlport");
    test.start_tb(tb_name, 10ms);

    //-------------------------------------------------------
    // Reset clock generator and wait for reset to complete
    //-------------------------------------------------------

    test.start_test("Reset", 100us);
    clk_gen.reset();
    if (rst) @rst;
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    // Verify that all features are disabled after reset
    test.start_test("Checking reset state", 2us);
    test.assert_error(
      feature_enabled == 0,
      $sformatf("Features enabled after reset: %x", feature_enabled));
    ctrlport_peek32(RB_ADDR, 32'b00000000000000000000000000000000);
    test.end_test();

    /// Regular feature unlock
    test.start_test("Checking feature 0 unlock", 100us);
    ctrlport_poke32(FEATURE_ADDR, FEATURE0);
    ctrlport_poke32(KEY_ADDR, hash0_0);
    ctrlport_poke32(KEY_ADDR, hash0_1);
    ctrlport_poke32(KEY_ADDR, hash0_2);
    ctrlport_poke32(KEY_ADDR, hash0_3);
    ctrlport_poke32(KEY_ADDR, hash0_4);
    ctrlport_poke32(KEY_ADDR, hash0_5);
    ctrlport_poke32(KEY_ADDR, hash0_6);
    ctrlport_poke32(KEY_ADDR, hash0_7);
    test.assert_error(feature_enabled == 2'b1,
                      $sformatf("Feature enable is not 1 (%x)", feature_enabled));
    ctrlport_peek32(RB_ADDR, 32'b10000000000000000000000000000000);
    test.end_test();

    test.start_test("Checking feature 1 unlock", 100us);
    ctrlport_poke32(FEATURE_ADDR, FEATURE1);
    ctrlport_poke32(KEY_ADDR, hash1_0);
    ctrlport_poke32(KEY_ADDR, hash1_1);
    ctrlport_poke32(KEY_ADDR, hash1_2);
    ctrlport_poke32(KEY_ADDR, hash1_3);
    ctrlport_poke32(KEY_ADDR, hash1_4);
    ctrlport_poke32(KEY_ADDR, hash1_5);
    ctrlport_poke32(KEY_ADDR, hash1_6);
    ctrlport_poke32(KEY_ADDR, hash1_7);
    test.assert_error(feature_enabled == 2'b11,
                      $sformatf("Feature enable is not 0b11 (%x)", feature_enabled));
    ctrlport_peek32(RB_ADDR, 32'b10000000000000000000000000000001);
    test.end_test();

    /// Reset and make sure features are no longer enabled
    test.start_test("Testing reset after unlock", 100us);
    rst_dut <= 1;
    @(posedge clk);
    rst_dut <= 0;
    @(posedge clk);
    test.assert_error(feature_enabled == 2'b00,
                      $sformatf("Feature enable is not 0b00 (%x)", feature_enabled));
    ctrlport_peek32(RB_ADDR, 32'h0);
    test.end_test();

    /// Writing a hash value now is invalid
    test.start_test("Checking error case: No FID loaded", 100us);
    ctrlport_poke32_status(KEY_ADDR, 32'h1234ABCD, 2'b1);
    test.end_test();

    /// Try uploading invalid key
    test.start_test("Checking feature 0 unlock with invalid key", 100us);
    ctrlport_poke32(FEATURE_ADDR, FEATURE0);
    ctrlport_poke32(KEY_ADDR, hash0_0);
    ctrlport_poke32(KEY_ADDR, hash0_1);
    ctrlport_poke32(KEY_ADDR, hash0_2);
    ctrlport_poke32(KEY_ADDR, hash0_3);
    ctrlport_poke32(KEY_ADDR, hash0_4);
    ctrlport_poke32(KEY_ADDR, hash0_5);
    ctrlport_poke32(KEY_ADDR, hash0_6);
    ctrlport_poke32(KEY_ADDR, 32'h0);
    test.assert_error(feature_enabled == 2'b0,
                      $sformatf("Feature enable is not 0 (%x)", feature_enabled));
    test.end_test();

    /// Now again valid key after failure
    test.start_test("Checking feature 0 unlock with valid key again after fail", 100us);
    ctrlport_poke32(FEATURE_ADDR, FEATURE0);
    ctrlport_poke32(KEY_ADDR, hash0_0);
    ctrlport_poke32(KEY_ADDR, hash0_1);
    ctrlport_poke32(KEY_ADDR, hash0_2);
    ctrlport_poke32(KEY_ADDR, hash0_3);
    ctrlport_poke32(KEY_ADDR, hash0_4);
    ctrlport_poke32(KEY_ADDR, hash0_5);
    ctrlport_poke32(KEY_ADDR, hash0_6);
    ctrlport_poke32(KEY_ADDR, hash0_7);
    test.assert_error(feature_enabled == 2'b1,
                      $sformatf("Feature enable is not 0b1 (%x)", feature_enabled));
    test.end_test();

    /// Upload part of feature 0, interrupt, then feature 1
    test.start_test("Checking feature 1 unlock after reset", 100us);
    ctrlport_poke32(FEATURE_ADDR, FEATURE0);
    ctrlport_poke32(KEY_ADDR, hash0_0);
    ctrlport_poke32(KEY_ADDR, hash0_1);
    ctrlport_poke32(KEY_ADDR, hash0_2);
    ctrlport_poke32(FEATURE_ADDR, FEATURE1);
    ctrlport_poke32(KEY_ADDR, hash1_0);
    ctrlport_poke32(KEY_ADDR, hash1_1);
    ctrlport_poke32(KEY_ADDR, hash1_2);
    ctrlport_poke32(KEY_ADDR, hash1_3);
    ctrlport_poke32(KEY_ADDR, hash1_4);
    ctrlport_poke32(KEY_ADDR, hash1_5);
    ctrlport_poke32(KEY_ADDR, hash1_6);
    ctrlport_poke32(KEY_ADDR, hash1_7);
    test.assert_error(feature_enabled == 2'b11,
                      $sformatf("Feature enable is not 0b11 (%x)", feature_enabled));
    ctrlport_peek32(RB_ADDR, 32'b10000000000000000000000000000001);
    test.end_test();


    //--------------------------------
    // Finish Up
    //--------------------------------

    test.end_tb();

  end : tb_main

endmodule : license_check_tb

`default_nettype wire
