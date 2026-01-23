//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_null_src_sink_tb
//

`default_nettype none


module rfnoc_block_null_src_sink_tb #(
  parameter int CHDR_W = 64
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrData::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  // Parameters
  localparam bit        NOC_ID      = 32'h0000_0001;
  localparam bit [9:0]  THIS_PORTID = 10'h17;
  localparam bit [15:0] THIS_EPID   = 16'hDEAD;
  localparam int        ITEM_W      = 32;
  localparam int        NIPC        = CHDR_W/ITEM_W;
  localparam int        NUM_PKTS    = 10;
  localparam int        PORT_SRCSNK = 0;
  localparam int        PORT_LOOP   = 1;

  localparam realtime CLK_PERIOD = 10.0ns;

  // Clock and Reset Definition
  bit rfnoc_chdr_clk;
  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0))
    clk_gen (.clk(rfnoc_chdr_clk), .rst());

  // ----------------------------------------
  // Instantiate DUT
  // ----------------------------------------

  // Connections to DUT as interfaces:
  RfnocBackendIf        backend (rfnoc_chdr_clk, rfnoc_chdr_clk); // Required backend iface
  AxiStreamIf #(32)     m_ctrl  (rfnoc_chdr_clk,             '0); // Required control iface
  AxiStreamIf #(32)     s_ctrl  (rfnoc_chdr_clk,             '0); // Required control iface
  AxiStreamIf #(CHDR_W) m0_chdr (rfnoc_chdr_clk,             '0); // Optional data iface
  AxiStreamIf #(CHDR_W) m1_chdr (rfnoc_chdr_clk,             '0); // Optional data iface
  AxiStreamIf #(CHDR_W) s0_chdr (rfnoc_chdr_clk,             '0); // Optional data iface
  AxiStreamIf #(CHDR_W) s1_chdr (rfnoc_chdr_clk,             '0); // Optional data iface

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

    // Packet sizes to test
    localparam int ITEMS_PER_CHDR_WORD = CHDR_W/ITEM_W;
    automatic int test_spp_values[] = '{
      1,
      ITEMS_PER_CHDR_WORD - 1,
      ITEMS_PER_CHDR_WORD,
      ITEMS_PER_CHDR_WORD + 1,
      2*ITEMS_PER_CHDR_WORD - 1,
      2*ITEMS_PER_CHDR_WORD,
      2*ITEMS_PER_CHDR_WORD + 1,
      7*ITEMS_PER_CHDR_WORD - 1,
      7*ITEMS_PER_CHDR_WORD,
      7*ITEMS_PER_CHDR_WORD + 1
    };

    // Number of packets to test
    automatic int test_num_pkts[] = '{1, 2, 3, 7};

    // Throttle cycles to test
    automatic int test_throttle_cycles[] = '{0, 3};

    // Initialize
    // ----------------------------------------
    test.start_tb($sformatf("%m"));

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    clk_gen.start();

    // Start the stream endpoint BFM
    blk_ctrl = new(backend, m_ctrl, s_ctrl);
    void'(blk_ctrl.add_master_data_port(m0_chdr));
    void'(blk_ctrl.add_slave_data_port(s0_chdr));
    void'(blk_ctrl.add_master_data_port(m1_chdr));
    void'(blk_ctrl.add_slave_data_port(s1_chdr));
    blk_ctrl.run();

    // Startup block (Software initialization)
    // ----------------------------------------
    test.start_test("Flush block then reset it", 10us);
    begin
      blk_ctrl.flush_and_reset();
    end
    test.end_test();

    // Run Tests
    // ----------------------------------------
    test.start_test("Read Block Info", 1us);
    begin
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
    end
    test.end_test();

    test.start_test("Stream Data Through Loopback Port m1->s1", 500us);
    foreach (test_num_pkts[npkt_idx]) begin : test_loopback_npkts
      automatic int num_test_pkts = test_num_pkts[npkt_idx];

      foreach (test_spp_values[spp_idx]) begin : test_loopback_spp
        automatic int spp = test_spp_values[spp_idx];
        automatic int lpp = int'($ceil(real'(spp)/NIPC));
        automatic int total_lines = 0;
        automatic int total_pkts = 0;

        $display("Testing loopback with NUM_PKTS=%0d, SPP=%0d, LPP=%0d",
          num_test_pkts, spp, lpp);

        // Clear counters before this test iteration
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b01);
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b00);

        // Send and receive packets
        repeat (num_test_pkts) begin
          chdr_word_t rx_data[$];
          int rx_bytes;
          automatic ItemDataBuff #(logic[ITEM_W-1:0],CHDR_W) tx_dbuff = new;
          automatic ItemDataBuff #(logic[ITEM_W-1:0],CHDR_W) rx_dbuff = new;
          repeat (spp) tx_dbuff.put($urandom());
          test.start_timeout(timeout, 5us, "Waiting for pkt to loop back");
          blk_ctrl.send(PORT_LOOP, tx_dbuff.to_chdr_payload(), tx_dbuff.get_bytes());
          blk_ctrl.recv(PORT_LOOP, rx_data, rx_bytes);
          rx_dbuff.from_chdr_payload(rx_data, rx_bytes);
          `ASSERT_ERROR(rx_dbuff.equal(tx_dbuff), "Data mismatch");
          test.end_timeout(timeout);
          total_lines += lpp;
          total_pkts += 1;
        end

        // Read item and packet counts on loopback port
        blk_ctrl.reg_read(dut.REG_LOOP_LINE_CNT_LO, rvalue);
        `ASSERT_ERROR(rvalue == total_lines,
          $sformatf("Incorrect REG_LOOP_LINE_CNT_LO: exp %0d, got %0d",
          total_lines, rvalue));
        blk_ctrl.reg_read(dut.REG_LOOP_PKT_CNT_LO, rvalue);
        `ASSERT_ERROR(rvalue == total_pkts,
          $sformatf("Incorrect REG_LOOP_PKT_CNT_LO: exp %0d, got %0d",
          total_pkts, rvalue));

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
      end : test_loopback_spp
    end : test_loopback_npkts
    test.end_test();

    test.start_test("Stream Data To Sink Port m0", 500us);
    foreach (test_num_pkts[npkt_idx]) begin : test_sink_npkts
      automatic int num_test_pkts = test_num_pkts[npkt_idx];

      foreach (test_spp_values[spp_idx]) begin : test_sink_spp
        automatic int spp = test_spp_values[spp_idx];
        automatic int lpp = int'($ceil(real'(spp)/NIPC));
        automatic int total_lines = 0;
        automatic int total_pkts = 0;

        $display("Testing sink with NUM_PKTS=%0d, SPP=%0d, LPP=%0d",
          num_test_pkts, spp, lpp);

        // Clear counters before this test iteration
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b01);
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b00);

        // Send packets
        repeat (num_test_pkts) begin
          chdr_word_t rx_data[$];
          int rx_bytes;
          automatic ItemDataBuff #(logic[ITEM_W-1:0],CHDR_W) tx_dbuff = new;
          repeat (spp) tx_dbuff.put($urandom());
          test.start_timeout(timeout, 5us, "Waiting to send packet");
          blk_ctrl.send(PORT_SRCSNK, tx_dbuff.to_chdr_payload(), tx_dbuff.get_bytes());
          test.end_timeout(timeout);
          total_lines += lpp;
          total_pkts += 1;
        end
        blk_ctrl.wait_complete(PORT_SRCSNK);

        // Read item and packet counts on loopback port (should be unchanged)
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
        `ASSERT_ERROR(rvalue == total_lines,
          $sformatf("Incorrect REG_SNK_LINE_CNT_LO: expected %0d, got %0d",
          total_lines, rvalue));
        blk_ctrl.reg_read(dut.REG_SNK_PKT_CNT_LO, rvalue);
        `ASSERT_ERROR(rvalue == total_pkts,
          $sformatf("Incorrect REG_SNK_PKT_CNT_LO: expected %0d, got %0d",
          total_pkts, rvalue));
      end : test_sink_spp
    end : test_sink_npkts
    test.end_test();

    test.start_test("Stream Data From Source Port s0", 500us);
    foreach (test_num_pkts[npkt_idx]) begin : test_source_num_pkts
      automatic int num_test_pkts = test_num_pkts[npkt_idx];

      foreach (test_spp_values[spp_idx]) begin : test_source_spp
        automatic int spp = test_spp_values[spp_idx];
        automatic int lpp = int'($ceil(real'(spp)/NIPC));

        $display("Testing source with NUM_PKTS=%0d, SPP=%0d, LPP=%0d",
          num_test_pkts, spp, lpp);

        // Clear counters before each test iteration
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b01);
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b00);

        // Turn on the source for some time then stop it
        blk_ctrl.reg_write(dut.REG_SRC_LINES_PER_PKT, lpp-1);
        // A line is generated as NIPC Items
        blk_ctrl.reg_write(dut.REG_SRC_BYTES_PER_PKT, (lpp+1)*ITEM_W/8*NIPC);
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b10);
        repeat ((num_test_pkts / 10) * lpp) @(posedge rfnoc_chdr_clk);
        blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b00);
        blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);
        repeat (rvalue * lpp * 2) @(posedge rfnoc_chdr_clk);
        blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);

        // Gather the accumulated packets and verify contents
        for (int p = 0; p < rvalue; p++) begin
          chdr_word_t exp_data[$];
          chdr_word_t rx_data[$];
          chdr_word_t   metadata[$];
          packet_info_t pkt_info;
          int rx_bytes;
          test.start_timeout(timeout, 5us, "Waiting for pkt to arrive");
          exp_data.delete();
          for (int i = p*lpp; i < (p+1)*lpp; i++)
            exp_data.push_back({NIPC{{~i[ITEM_W/2-1:0], i[ITEM_W/2-1:0]}}});
          blk_ctrl.recv_adv(PORT_SRCSNK, rx_data, rx_bytes, metadata, pkt_info);
          `ASSERT_ERROR(blk_ctrl.compare_data(exp_data, rx_data), "Data mismatch");
          if (p == rvalue-1) begin
            `ASSERT_ERROR(pkt_info.eob == 1, "EOB was not set on last packet from source");
          end else begin
            `ASSERT_ERROR(pkt_info.eob == 0, "EOB was set on middle packet from source");
          end
          test.end_timeout(timeout);
        end
      end : test_source_spp
    end : test_source_num_pkts
    test.end_test();

    test.start_test("Clear Counts", 10us);
    begin
      // Clear
      blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b01);

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
    end
    test.end_test();

    test.start_test("Test Finite Source Packet Generation", 4ms);
    begin : test_finite_source
      // Iterate through all packet sizes
      foreach (test_spp_values[spp_idx]) begin : test_finite_spp
        automatic int spp = test_spp_values[spp_idx];
        automatic int lpp = int'($ceil(real'(spp)/NIPC));
        automatic int pkt_count_lo, pkt_count_hi;
        automatic int total_pkt_count_lo, total_pkt_count_hi;
        automatic longint total_pkt_count;

        $display("Testing finite source with SPP=%0d, LPP=%0d", spp, lpp);

        // Test multiple packet count values with different throttle settings
        foreach (test_throttle_cycles[throttle_idx]) begin : test_throttle_loop
          automatic int throttle_cyc = test_throttle_cycles[throttle_idx];

          foreach (test_num_pkts[test_idx]) begin : test_num_pkts_loop
            automatic int num_pkts = test_num_pkts[test_idx];
            $display("Testing %0d packets, %0d throttle cycles",
              num_pkts, throttle_cyc);

            // Clear counters
            blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 'b01);

            // Configure source parameters
            blk_ctrl.reg_write(dut.REG_SRC_LINES_PER_PKT, lpp-1);
            blk_ctrl.reg_write(dut.REG_SRC_BYTES_PER_PKT, CHDR_W/8 + spp*ITEM_W/8);

            // Set throttle cycles and number of packets for this test
            blk_ctrl.reg_write(dut.REG_SRC_THROTTLE_CYC, throttle_cyc);
            blk_ctrl.reg_write(dut.REG_SRC_NUM_PKT_LO, num_pkts);
            blk_ctrl.reg_write(dut.REG_SRC_NUM_PKT_HI, 0);

            // Verify the register updated as expected
            blk_ctrl.reg_read(dut.REG_SRC_NUM_PKT_LO, rvalue);
            `ASSERT_ERROR(rvalue == num_pkts, "Packet limit LO was not set correctly");
            blk_ctrl.reg_read(dut.REG_SRC_NUM_PKT_HI, rvalue);
            `ASSERT_ERROR(rvalue == 0, "Packet limit HI was not set correctly");

            // Start the source. It should automatically stop after num_pkts
            // packets.
            blk_ctrl.reg_write(dut.REG_CTRL_STATUS, 3'b110);

            // Collect and verify the packets that were generated
            for (int pkt_idx = 0; pkt_idx < num_pkts; pkt_idx++) begin
              chdr_word_t exp_data[$];
              chdr_word_t rx_data[$];
              chdr_word_t metadata[$];
              packet_info_t pkt_info;
              int rx_bytes;

              // Generate expected data pattern
              exp_data.delete();
              for (int idx = pkt_idx*lpp; idx < (pkt_idx+1)*lpp; idx++)
                exp_data.push_back({NIPC{{~idx[ITEM_W/2-1:0], idx[ITEM_W/2-1:0]}}});

              // Receive and verify packet
              blk_ctrl.recv_adv(PORT_SRCSNK, rx_data, rx_bytes, metadata, pkt_info);
              `ASSERT_ERROR(blk_ctrl.compare_data(exp_data, rx_data),
                          $sformatf("Data mismatch in packet %0d", pkt_idx));

              // Check EOB on the final packet only
              if (pkt_idx == num_pkts-1) begin
                `ASSERT_ERROR(pkt_info.eob == 1, "EOB not set on final packet");
              end else begin
                `ASSERT_ERROR(pkt_info.eob == 0,
                  "EOB incorrectly set on intermediate packet");
              end
            end

            // Read final packet count for test
            blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_LO, rvalue);
            total_pkt_count_lo = rvalue;
            blk_ctrl.reg_read(dut.REG_SRC_PKT_CNT_HI, rvalue);
            total_pkt_count_hi = rvalue;
            total_pkt_count = {total_pkt_count_hi, total_pkt_count_lo};
            `ASSERT_ERROR(total_pkt_count == num_pkts,
              $sformatf("Hardware generated %0d packets but expected %0d",
              total_pkt_count, num_pkts));

            // Read the status and make sure the src_en bit cleared
            blk_ctrl.reg_read(dut.REG_CTRL_STATUS, rvalue);
            `ASSERT_ERROR(rvalue[1] == 1'b0,
              $sformatf("Status register reports 0x%X but expected bit 1 to be 0",
              rvalue));

            // Verify no additional packets are available
            // Wait a bit to make sure no extra packets arrive
            #(1000*CLK_PERIOD);

            // Check if there are any unexpected packets waiting
            begin
              bit status;
              ChdrPacket #(CHDR_W) peeked_packet;
              status = blk_ctrl.try_peek_chdr(PORT_SRCSNK, peeked_packet);
              `ASSERT_ERROR(status == 0,
                "Unexpected extra packet available after finite generation completed");
            end
          end : test_num_pkts_loop
        end : test_throttle_loop
      end : test_finite_spp
    end : test_finite_source
    test.end_test();

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    test.end_tb(.finish(0));

    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();
  end

endmodule
