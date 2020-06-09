//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_null_src_sink_tb
//

`default_nettype none


module rfnoc_block_null_src_sink_tb #(
  parameter TEST_NAME  = "rfnoc_block_null_src_sink_tb",
  parameter CHDR_W     = 64
)(
  /* no IO */
);

  // Include macros and time declarations for use with PkgTestExec
  `define TEST_EXEC_OBJ test  
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  // Parameters
  localparam        NOC_ID      = 32'h0000_0001;
  localparam [9:0]  THIS_PORTID = 10'h17;
  localparam [15:0] THIS_EPID   = 16'hDEAD;
  localparam int    ITEM_W      = 32; 
  localparam int    NIPC        = CHDR_W/ITEM_W; // Expected Data generation only works for full words
  localparam int    SPP         = 201;
  localparam int    LPP         = SPP % NIPC == 0 ? SPP/NIPC : SPP/NIPC+1;
  localparam int    NUM_PKTS    = 50;

  localparam int    PORT_SRCSNK = 0;
  localparam int    PORT_LOOP   = 1;

  // Clock and Reset Definition
  bit rfnoc_chdr_clk;
  sim_clock_gen #(2.5) rfnoc_chdr_clk_gen (rfnoc_chdr_clk); // 400 MHz

  // ----------------------------------------
  // Instantiate DUT
  // ----------------------------------------

  // Connections to DUT as interfaces:
  RfnocBackendIf        backend (rfnoc_chdr_clk, rfnoc_chdr_clk); // Required backend iface
  AxiStreamIf #(32)     m_ctrl  (rfnoc_chdr_clk);                 // Required control iface
  AxiStreamIf #(32)     s_ctrl  (rfnoc_chdr_clk);                 // Required control iface
  AxiStreamIf #(CHDR_W) m0_chdr (rfnoc_chdr_clk);                 // Optional data iface
  AxiStreamIf #(CHDR_W) m1_chdr (rfnoc_chdr_clk);                 // Optional data iface
  AxiStreamIf #(CHDR_W) s0_chdr (rfnoc_chdr_clk);                 // Optional data iface
  AxiStreamIf #(CHDR_W) s1_chdr (rfnoc_chdr_clk);                 // Optional data iface

  TestExec test = new();  
  
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(.CHDR_W(CHDR_W)) blk_ctrl;

  // DUT
  rfnoc_block_null_src_sink #(
    .THIS_PORTID        (THIS_PORTID),
    .CHDR_W             (CHDR_W),
    .ITEM_W             (ITEM_W),    
    .NIPC               (NIPC),
    .MTU                (10)
  ) dut (
    .rfnoc_chdr_clk     (backend.chdr_clk),
    .rfnoc_ctrl_clk     (backend.ctrl_clk),
    .rfnoc_core_config  (backend.cfg),
    .rfnoc_core_status  (backend.sts),
    .s_rfnoc_chdr_tdata ({m1_chdr.tdata  , m0_chdr.tdata  }),
    .s_rfnoc_chdr_tlast ({m1_chdr.tlast  , m0_chdr.tlast  }),
    .s_rfnoc_chdr_tvalid({m1_chdr.tvalid , m0_chdr.tvalid }),
    .s_rfnoc_chdr_tready({m1_chdr.tready , m0_chdr.tready }),
    .m_rfnoc_chdr_tdata ({s1_chdr.tdata , s0_chdr.tdata }),
    .m_rfnoc_chdr_tlast ({s1_chdr.tlast , s0_chdr.tlast }),
    .m_rfnoc_chdr_tvalid({s1_chdr.tvalid, s0_chdr.tvalid}),
    .m_rfnoc_chdr_tready({s1_chdr.tready, s0_chdr.tready}),
    .s_rfnoc_ctrl_tdata (m_ctrl.tdata  ),
    .s_rfnoc_ctrl_tlast (m_ctrl.tlast  ),
    .s_rfnoc_ctrl_tvalid(m_ctrl.tvalid ),
    .s_rfnoc_ctrl_tready(m_ctrl.tready ),
    .m_rfnoc_ctrl_tdata (s_ctrl.tdata ),
    .m_rfnoc_ctrl_tlast (s_ctrl.tlast ),
    .m_rfnoc_ctrl_tvalid(s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready(s_ctrl.tready)
  );

  // ----------------------------------------
  // Test Process
  // ----------------------------------------

  initial begin
    // Shared Variables
    // ----------------------------------------
    timeout_t    timeout;
    ctrl_word_t  rvalue;

    // Initialize
    // ----------------------------------------
    test.start_tb({TEST_NAME,"rfnoc_block_null_src_sink_tb"});

    // Start the stream endpoint BFM
    blk_ctrl = new(backend, m_ctrl, s_ctrl);
    blk_ctrl.add_master_data_port(m0_chdr);
    blk_ctrl.add_slave_data_port(s0_chdr);
    blk_ctrl.add_master_data_port(m1_chdr);
    blk_ctrl.add_slave_data_port(s1_chdr);
    blk_ctrl.run();

    // Startup block (Software initialization)
    // ----------------------------------------
    test.start_test({TEST_NAME,"Flush block then reset it"});
    begin
      test.start_timeout(timeout, 10us, "Waiting for flush_and_reset");
      #100;  //Wait for GSR to deassert
      blk_ctrl.flush_and_reset();
      test.end_timeout(timeout);
    end
    test.end_test();

    // Run Tests
    // ----------------------------------------
    test.start_test({TEST_NAME,"Read Block Info"});
    begin
      test.start_timeout(timeout, 1us, "Waiting for block info response");
      // Get static block info and validate it
      `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect noc_id Value");
      `ASSERT_ERROR(blk_ctrl.get_num_data_i() == 2, "Incorrect num_data_i Value");
      `ASSERT_ERROR(blk_ctrl.get_num_data_o() == 2, "Incorrect num_data_o Value");
      `ASSERT_ERROR(blk_ctrl.get_ctrl_fifosize() == 5, "Incorrect ctrl_fifosize Value");
      `ASSERT_ERROR(blk_ctrl.get_mtu() == 10, "Incorrect mtu Value");

      // Read status register and validate it
      blk_ctrl.reg_read(dut.REG_CTRL_STATUS, rvalue);
      `ASSERT_ERROR(rvalue[31:24] == NIPC, "Incorrect NIPC Value");
      `ASSERT_ERROR(rvalue[23:16] == ITEM_W, "Incorrect ITEM_W Value");
      test.end_timeout(timeout);
    end
    test.end_test();

    test.start_test({TEST_NAME,"Stream Data Through Loopback Port m1->s1"});
    begin
      // Send and receive packets
      repeat (NUM_PKTS) begin
        chdr_word_t rx_data[$];
        int rx_bytes;
        automatic ItemDataBuff #(logic[ITEM_W-1:0],CHDR_W) tx_dbuff = new;
        automatic ItemDataBuff #(logic[ITEM_W-1:0],CHDR_W) rx_dbuff = new;
        for (int i = 0; i < SPP; i++)
          tx_dbuff.put($urandom());
        test.start_timeout(timeout, 5us, "Waiting for pkt to loop back");
        blk_ctrl.send(PORT_LOOP, tx_dbuff.to_chdr_payload(), tx_dbuff.get_bytes());
        blk_ctrl.recv(PORT_LOOP, rx_data, rx_bytes);
        rx_dbuff.from_chdr_payload(rx_data, rx_bytes);
        `ASSERT_ERROR(rx_dbuff.equal(tx_dbuff), "Data mismatch");
        test.end_timeout(timeout);
      end

      // Read item and packet counts on loopback port
      blk_ctrl.reg_read(dut.REG_LOOP_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == (LPP*NUM_PKTS), "Incorrect REG_LOOP_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_LOOP_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == NUM_PKTS, "Incorrect REG_LOOP_PKT_CNT_LO value");

      // Read item and packet counts on source port
      blk_ctrl.reg_read(dut.REG_SRC_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SRC_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SRC_PKT_CNT_LO value");

      // Read item and packet counts on sink port
      blk_ctrl.reg_read(dut.REG_SNK_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SNK_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_SNK_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SNK_PKT_CNT_LO value");
    end
    test.end_test();

    test.start_test({TEST_NAME,"Stream Data To Sink Port m0"});
    begin
      // Send packets
      repeat (NUM_PKTS) begin
        chdr_word_t rx_data[$];
        int rx_bytes;
        automatic ItemDataBuff #(logic[ITEM_W-1:0],CHDR_W) tx_dbuff = new;
        for (int i = 0; i < SPP; i++)
          tx_dbuff.put($urandom());
        test.start_timeout(timeout, 5us, "Waiting to send packet");
        blk_ctrl.send(PORT_SRCSNK, tx_dbuff.to_chdr_payload(), tx_dbuff.get_bytes());
        test.end_timeout(timeout);
      end
      repeat (NUM_PKTS * SPP * 2) @(posedge rfnoc_chdr_clk);

      // Read item and packet counts on loopback port
      blk_ctrl.reg_read(dut.REG_LOOP_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == (LPP*NUM_PKTS), "Incorrect REG_LOOP_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_LOOP_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == NUM_PKTS, "Incorrect REG_LOOP_PKT_CNT_LO value");

      // Read item and packet counts on source port
      blk_ctrl.reg_read(dut.REG_SRC_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SRC_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SRC_PKT_CNT_LO value");

      // Read item and packet counts on sink port
      blk_ctrl.reg_read(dut.REG_SNK_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == (LPP*NUM_PKTS), "Incorrect REG_SNK_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_SNK_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == NUM_PKTS, "Incorrect REG_SNK_PKT_CNT_LO value");
    end
    test.end_test();

    test.start_test({TEST_NAME,"Stream Data From Source Port s0"});
    begin
      // Turn on the source for some time then stop it
      blk_ctrl.reg_write(dut.REG_SRC_LINES_PER_PKT, LPP-1);
      // A line is generated as NIPC Items
      blk_ctrl.reg_write(dut.REG_SRC_BYTES_PER_PKT, (LPP+1)*ITEM_W/8*NIPC);
      blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 2'b10);
      repeat ((NUM_PKTS / 10) * LPP) @(posedge rfnoc_chdr_clk);
      blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 2'b00);
      blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);
      repeat (rvalue * LPP * 2) @(posedge rfnoc_chdr_clk);
      blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);

      // Gather the accumulated packets and verify contents
      for (int p = 0; p < rvalue; p++) begin
        chdr_word_t exp_data[$];
        chdr_word_t rx_data[$];
        int rx_bytes;
        test.start_timeout(timeout, 5us, "Waiting for pkt to arrive");
        exp_data.delete();
        for (int i = p*LPP; i < (p+1)*LPP; i++)
          exp_data.push_back({NIPC{{~i[ITEM_W/2-1:0], i[ITEM_W/2-1:0]}}});
        blk_ctrl.recv(PORT_SRCSNK, rx_data, rx_bytes);
        `ASSERT_ERROR(blk_ctrl.compare_data(exp_data, rx_data), "Data mismatch");
        test.end_timeout(timeout);
      end
    end
    test.end_test();

    test.start_test({TEST_NAME,"Clear Counts"});
    begin
      test.start_timeout(timeout, 1us, "Waiting for clear and readbacks");
      // Clear
      blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 2'b01);

      // Read item and packet counts on loopback port
      blk_ctrl.reg_read(dut.REG_LOOP_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_LOOP_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_LOOP_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_LOOP_PKT_CNT_LO value");

      // Read item and packet counts on source port
      blk_ctrl.reg_read(dut.REG_SRC_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SRC_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SRC_PKT_CNT_LO value");

      // Read item and packet counts on sink port
      blk_ctrl.reg_read(dut.REG_SNK_LINE_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SNK_LINE_CNT_LO value");
      blk_ctrl.reg_read(dut.REG_SNK_PKT_CNT_LO, rvalue);
      `ASSERT_ERROR(rvalue == 0, "Incorrect REG_SNK_PKT_CNT_LO value");
      test.end_timeout(timeout);
    end
    test.end_test();

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    test.end_tb(.finish(0));
  end

endmodule
