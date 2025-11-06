//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_license_check_tb
//
// Description: Testbench for the license_check RFNoC block.
//

`default_nettype none


module rfnoc_block_license_check_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'h11C0CECC;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = 64;    // CHDR size in bits
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_FEATURES    = 2;
  localparam [(32*NUM_FEATURES)-1:0] FEATURE_IDS = {32'hF00D, 32'hC0DE};
  localparam int    NUM_PORTS_I     = 0;
  localparam int    NUM_PORTS_O     = 0;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam int    SERIAL_W        = 96;
  localparam int    PKEY_W          = 312;
  localparam [SERIAL_W-1:0] SERIAL  = 96'h012345678ABCDEF00C0DE0BB;
  localparam [PKEY_W-1:0] PRIVATE_KEY = 312'h5EC;

  localparam COMPAT_ADDR  = 0;
  localparam FID_ADDR     = 4;
  localparam KEY_ADDR     = 8;
  localparam FID_RB_ADDR  = 12;
  localparam LIST_RB_ADDR = 16;
  localparam int FEATURE0 = 32'hC0DE;
  localparam int FEATURE1 = 32'hF00D;

  localparam hash0_0 = 32'h80f122aa;
  localparam hash0_1 = 32'hd504a1ec;
  localparam hash0_2 = 32'h7ae8f37b;
  localparam hash0_3 = 32'h4eee414c;
  localparam hash0_4 = 32'hf1cf32ec;
  localparam hash0_5 = 32'hc543ee2e;
  localparam hash0_6 = 32'h1d4e1d4a;
  localparam hash0_7 = 32'he59eb41b;
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

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;

  sim_clock_gen #(.PERIOD(CHDR_CLK_PER))
    rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(.PERIOD(CHDR_CLK_PER))
    rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

  // Block Controller BFM
  RfnocBlockCtrlBfmCtrlOnly blk_ctrl = new(backend, m_ctrl, s_ctrl);

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic [NUM_FEATURES-1:0] feature_enable;

  rfnoc_block_license_check #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_FEATURES        (NUM_FEATURES),
    .FEATURE_IDS         (FEATURE_IDS),
    .SERIAL_W            (SERIAL_W),
    .PRIVATE_KEY         (312'h5EC)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready),

    .serial              (SERIAL),
    .feature_enable      (feature_enable)
  );

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_license_check_tb");

    // Start the BFMs running
    blk_ctrl.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset block", 10us);
    blk_ctrl.reset_ctrl();
    test.end_test();

    //--------------------------------
    // Verify Block Info
    //--------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS_I, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS_O, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    begin
      logic [31:0] rb_val;
      blk_ctrl.reg_read(COMPAT_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'h00000000,
                    $sformatf("Compat readback incorrect! (0x%X)", rb_val));
    end
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    begin
      logic [31:0] rb_val;
      test.start_test("Unlock feature0", 100us);
      blk_ctrl.reg_write(FID_ADDR, FEATURE0);
      blk_ctrl.reg_write(KEY_ADDR, hash0_0);
      blk_ctrl.reg_write(KEY_ADDR, hash0_1);
      blk_ctrl.reg_write(KEY_ADDR, hash0_2);
      blk_ctrl.reg_write(KEY_ADDR, hash0_3);
      blk_ctrl.reg_write(KEY_ADDR, hash0_4);
      blk_ctrl.reg_write(KEY_ADDR, hash0_5);
      blk_ctrl.reg_write(KEY_ADDR, hash0_6);
      blk_ctrl.reg_write(KEY_ADDR, hash0_7);
      `ASSERT_ERROR(feature_enable[0], "Feature 0 was not unlocked!");
      blk_ctrl.reg_read(FID_RB_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'h80000000,
                    $sformatf("Feature enable readback incorrect! (0x%X)", rb_val));
      test.end_test();
    end


    begin
      logic [31:0] rb_val;
      test.start_test("Unlock feature1", 100us);
      blk_ctrl.reg_write(FID_ADDR, FEATURE1);
      blk_ctrl.reg_write(KEY_ADDR, hash1_0);
      blk_ctrl.reg_write(KEY_ADDR, hash1_1);
      blk_ctrl.reg_write(KEY_ADDR, hash1_2);
      blk_ctrl.reg_write(KEY_ADDR, hash1_3);
      blk_ctrl.reg_write(KEY_ADDR, hash1_4);
      blk_ctrl.reg_write(KEY_ADDR, hash1_5);
      blk_ctrl.reg_write(KEY_ADDR, hash1_6);
      blk_ctrl.reg_write(KEY_ADDR, hash1_7);
      `ASSERT_ERROR(feature_enable[1], "Feature 1 was not unlocked!");
      blk_ctrl.reg_read(FID_RB_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'h80000001,
                    $sformatf("Feature enable readback incorrect! (0x%X)", rb_val));
      test.end_test();
    end

    begin
      logic [31:0] rb_val;
      test.start_test("Reading back feature IDs", 100us);
      blk_ctrl.reg_read(LIST_RB_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'hC0DE,
                    $sformatf("Feature ID readback incorrect! (0x%X)", rb_val));
      blk_ctrl.reg_read(LIST_RB_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'hF00D,
                    $sformatf("Feature ID readback incorrect! (0x%X)", rb_val));
      blk_ctrl.reg_read(LIST_RB_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'hC0DE,
                    $sformatf("Feature ID readback incorrect! (0x%X)", rb_val));
      blk_ctrl.reg_read(FID_RB_ADDR, rb_val);
      `ASSERT_ERROR(rb_val == 32'h80000001,
                    $sformatf("Feature enable readback incorrect! (0x%X)", rb_val));
      test.end_test();
    end

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_license_check_tb

`default_nettype wire
