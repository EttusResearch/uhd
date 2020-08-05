//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_keep_one_in_n_tb
//
// Description: Testbench for the keep_one_in_n RFNoC block.
//

`default_nettype none

module rfnoc_block_keep_one_in_n_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam [31:0] NOC_ID          = 32'h02460000;
  localparam int    CHDR_W          = 64;
  localparam int    ITEM_W          = 32;
  localparam int    NUM_PORTS       = 2;
  localparam int    MTU             = 13;
  localparam int    SPP             = 64;
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 50;      // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;     // 200 MHz
  localparam real   CTRL_CLK_PER    = 25.0;    // 40 MHz
  localparam real   CE_CLK_PER      = 5.0;  // 200 MHz

  localparam        WIDTH_N     = 8;

  localparam bit    SAMPLE_MODE = 0;
  localparam bit    PACKET_MODE = 1;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER) ce_clk_gen (.clk(ce_clk), .rst());

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

  // AXIS-CHDR Interfaces
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  // Block Controller BFM
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // CHDR word and item/sample data types
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_input_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
    end
  end
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_output_connections
    initial begin
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  // DUT Slave (Input) Port Signals
  logic [CHDR_W*NUM_PORTS-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tready;

  // DUT Master (Output) Port Signals
  logic [CHDR_W*NUM_PORTS-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT connections
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_dut_input_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];
  end
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_dut_output_connections
    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end

  rfnoc_block_keep_one_in_n #(
    .WIDTH_N             (WIDTH_N),
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk              (ce_clk),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready)
  );

  //---------------------------------------------------------------------------
  // Helper Logic
  //---------------------------------------------------------------------------

  // Translate the desired register access to a ctrlport write request.
  task automatic write_reg(input logic [$clog2(NUM_PORTS)-1:0] port, input ctrl_address_t addr, input logic [31:0] value);
    blk_ctrl.reg_write(256*8*port + addr*8, value);
  endtask : write_reg

  // Translate the desired register access to a ctrlport read request.
  task automatic read_user_reg(input logic [$clog2(NUM_PORTS)-1:0] port, input ctrl_address_t addr, output logic [63:0] value);
    blk_ctrl.reg_read(256*8*port + addr*8 + 0, value[31: 0]);
    blk_ctrl.reg_read(256*8*port + addr*8 + 4, value[63:32]);
  endtask : read_user_reg

  typedef struct {
    item_t        samples[$];
    chdr_word_t   mdata[$];
    packet_info_t pkt_info;
  } test_packet_t;

  // Check if input test packets are identical
  function automatic void compare_test_packets(const ref test_packet_t a, b);
    string str;

    // Packet payload
    $sformat(str,
      "Packet payload size incorrect! Expected: %4d, Received: %4d",
      a.samples.size(), b.samples.size());
    `ASSERT_ERROR(a.samples.size() == b.samples.size(), str);

    for (int i = 0; i < a.samples.size(); i++) begin
      $sformat(str,
        "Packet payload word %4d incorrect! Expected: 0x%8X, Received: 0x%8X",
        i, a.samples[i], b.samples[i]);
      `ASSERT_ERROR(a.samples[i] == b.samples[i], str);
    end

    // Packet metadata
    $sformat(str,
      "Packet metadata size incorrect! Expected: %4d, Received: %4d",
      a.mdata.size(), b.mdata.size());
    `ASSERT_ERROR(a.mdata.size() == b.mdata.size(), str);

    for (int i = 0; i < a.mdata.size(); i++) begin
      $sformat(str,
        "Packet metadata word %04d incorrect! Expected: 0x%8X, Received: 0x%8X",
        i, a.mdata[i], b.mdata[i]);
      `ASSERT_ERROR(a.mdata[i] == b.mdata[i], str);
    end

    // Packet info
    $sformat(str,
      "Packet info field 'vc' incorrect! Expected: %2d, Received: %2d",
      a.pkt_info.vc, b.pkt_info.vc);
    `ASSERT_ERROR(a.pkt_info.vc == b.pkt_info.vc, str);

    $sformat(str,
      "Packet info field 'eob' incorrect! Expected: %1d, Received: %1d",
      a.pkt_info.eob, b.pkt_info.eob);
    `ASSERT_ERROR(a.pkt_info.eob == b.pkt_info.eob, str);

    $sformat(str,
      "Packet info field 'eov' incorrect! Expected: %1d, Received: %1d",
      a.pkt_info.eov, b.pkt_info.eov);
    `ASSERT_ERROR(a.pkt_info.eov == b.pkt_info.eov, str);

    $sformat(str,
      "Packet info field 'has_time' incorrect! Expected: %1d, Received: %1d",
      a.pkt_info.has_time, b.pkt_info.has_time);
    `ASSERT_ERROR(a.pkt_info.has_time == b.pkt_info.has_time, str);

    $sformat(str,
      "Packet info field 'timestamp' incorrect! Expected: 0x%16X, Received: 0x%16X",
      a.pkt_info.timestamp, b.pkt_info.timestamp);
    `ASSERT_ERROR(a.pkt_info.timestamp == b.pkt_info.timestamp, str);

  endfunction

  //---------------------------------------------------------------------------
  // Test Tasks
  //---------------------------------------------------------------------------

  task automatic test_keep_one_in_n (
    input bit mode,
    input int n,
    input int num_packets,
    input int port        = 0,
    input int spp         = SPP,
    input int stall_prob  = STALL_PROB
  );

    mailbox #(test_packet_t) tb_send_packets = new();

    blk_ctrl.set_master_stall_prob(port, stall_prob);
    blk_ctrl.set_slave_stall_prob(port, stall_prob);

    $display("N = %3d, Number of Packets = %3d, Port Number = %1d", n, num_packets, port);

    begin
      logic [63:0] readback;
      string str;

      write_reg(port, dut.REG_MODE, mode);
      read_user_reg(port, dut.REG_MODE, readback);
      $sformat(str,
        "Mode incorrect! Expected: %1d, Received: %1d",
        mode, readback[0]);
      `ASSERT_ERROR(readback[0] == mode, str);

      write_reg(port, dut.REG_N, n);
      read_user_reg(port, dut.REG_N, readback);
      $sformat(str,
        "N incorrect! Expected: %5d, Received: %5d",
        n, readback);
      `ASSERT_ERROR(readback == n, str);
    end

    fork
      // TX
      begin
        for (int i = 0; i < num_packets; i++) begin
          test_packet_t tb_send_pkt;

          for (int k = 0; k < spp; k++) begin
            tb_send_pkt.samples.push_back($urandom());
          end
          tb_send_pkt.mdata = {};
          tb_send_pkt.pkt_info = '{
            vc:        0,
            eob:       (i == num_packets-1),
            eov:       bit'($urandom()),
            has_time:  1'b1,
            timestamp: {$urandom(),$urandom()}};

          blk_ctrl.send_items(port, tb_send_pkt.samples, tb_send_pkt.mdata, tb_send_pkt.pkt_info);

          tb_send_packets.put(tb_send_pkt);
        end
      end
      // RX
      begin
        int l = 0;
        mailbox #(test_packet_t) tb_recv_packets = new();
        int num_packets_expected = int'($ceil(real'(num_packets)/real'(n)));

        for (int i = 0; i < num_packets_expected; i++) begin
          test_packet_t tb_recv_pkt;

          blk_ctrl.recv_items_adv(port, tb_recv_pkt.samples, tb_recv_pkt.mdata, tb_recv_pkt.pkt_info);

          tb_recv_packets.put(tb_recv_pkt);
        end

        for (int i = 0; i < num_packets; i = i + n) begin
          test_packet_t tb_recv_pkt, tb_send_pkt, tb_dropped_pkt;

          // Packet mode keeps first packet, drops n-1
          if (mode) begin
            tb_send_packets.get(tb_send_pkt);
            for (int k = 0; k < n-1; k++) begin
              if (!tb_send_packets.try_get(tb_dropped_pkt)) break;
              tb_send_pkt.pkt_info.eob = tb_send_pkt.pkt_info.eob | tb_dropped_pkt.pkt_info.eob;
              tb_send_pkt.pkt_info.eov = tb_send_pkt.pkt_info.eov | tb_dropped_pkt.pkt_info.eov;
            end
          // Sample mode loops through n packets, keeps 1 in n samples
          // from all the packets
          end else begin
            item_t samples_pruned[$];

            // Peek first packet to grab mdata and packet info
            tb_send_packets.peek(tb_send_pkt);

            // Loop through n packets, grabbing 1 in n samples.
            for (int k = 0; k < n; k++) begin

              if (!tb_send_packets.try_get(tb_dropped_pkt)) break;
              tb_send_pkt.pkt_info.eob = tb_send_pkt.pkt_info.eob | tb_dropped_pkt.pkt_info.eob;
              tb_send_pkt.pkt_info.eov = tb_send_pkt.pkt_info.eov | tb_dropped_pkt.pkt_info.eov;
              while (l < tb_dropped_pkt.samples.size()) begin
                samples_pruned.push_back(tb_dropped_pkt.samples[l]);
                l = l + n;
              end
              // Account for wrap around when dropping samples between packet boundaries
              l = l - tb_dropped_pkt.samples.size();
            end

            // Replace packet samples with 1 in n samples of n packets
            tb_send_pkt.samples = samples_pruned;
          end

          tb_recv_packets.get(tb_recv_pkt);
          compare_test_packets(tb_send_pkt, tb_recv_pkt);
        end

        begin
          string str;
          $sformat(str,
            "Sent packets queue not empty! Number of extra items: %2d",
            tb_send_packets.num);
          `ASSERT_ERROR(tb_send_packets.num == 0, str);
          $sformat(str,
            "Receive packets queue not empty! Number of extra items: %2d",
            tb_recv_packets.num);
          `ASSERT_ERROR(tb_recv_packets.num == 0, str);
        end
      end
    join

  endtask;

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_keep_one_in_n_tb");

    // Start the BFMs running
    blk_ctrl.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Flush block then reset it", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();

    //--------------------------------
    // Verify Block Info
    //--------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    // Packet mode
    test.start_test("Test packet mode", NUM_PORTS*1100us);
    for (int port = 0; port < NUM_PORTS; port++) begin
      test_keep_one_in_n(PACKET_MODE,  1,  1, port); // mode, n, num_packets, port
      test_keep_one_in_n(PACKET_MODE,  1,  2, port);
      test_keep_one_in_n(PACKET_MODE,  1, 10, port);
      test_keep_one_in_n(PACKET_MODE,  2,  1, port);
      test_keep_one_in_n(PACKET_MODE,  2,  2, port);
      test_keep_one_in_n(PACKET_MODE,  2,  3, port);
      test_keep_one_in_n(PACKET_MODE,  2, 10, port);
      test_keep_one_in_n(PACKET_MODE,  2, 51, port);
      test_keep_one_in_n(PACKET_MODE,  3,  1, port);
      test_keep_one_in_n(PACKET_MODE,  3,  2, port);
      test_keep_one_in_n(PACKET_MODE,  3,  3, port);
      test_keep_one_in_n(PACKET_MODE,  3,  4, port);
      test_keep_one_in_n(PACKET_MODE,  3, 10, port);
      test_keep_one_in_n(PACKET_MODE,  3, 53, port);
      test_keep_one_in_n(PACKET_MODE, 11,  1, port);
      test_keep_one_in_n(PACKET_MODE, 11,  7, port);
      test_keep_one_in_n(PACKET_MODE, 11, 10, port);
      test_keep_one_in_n(PACKET_MODE, 11, 11, port);
      test_keep_one_in_n(PACKET_MODE, 11, 12, port);
      test_keep_one_in_n(PACKET_MODE, 11, 13, port);
      test_keep_one_in_n(PACKET_MODE, 11, 20, port);
      test_keep_one_in_n(PACKET_MODE, 11, 21, port);
      test_keep_one_in_n(PACKET_MODE, 11, 32, port);
      test_keep_one_in_n(PACKET_MODE, 11, 33, port);
      test_keep_one_in_n(PACKET_MODE, 11, 34, port);
      test_keep_one_in_n(PACKET_MODE, 2**WIDTH_N-1, 2**WIDTH_N-2, port);
      test_keep_one_in_n(PACKET_MODE, 2**WIDTH_N-1, 2**WIDTH_N-1, port);
      test_keep_one_in_n(PACKET_MODE, 2**WIDTH_N-1, 2**WIDTH_N  , port);
      test_keep_one_in_n(PACKET_MODE, 2**WIDTH_N-1, 2*(2**WIDTH_N-3), port);
      test_keep_one_in_n(PACKET_MODE, 2**WIDTH_N-1, 2*(2**WIDTH_N-2), port);
      test_keep_one_in_n(PACKET_MODE, 2**WIDTH_N-1, 2*(2**WIDTH_N-1), port);
    end
    test.end_test();

    // Packet mode
    test.start_test("Test sample mode", NUM_PORTS*1100us);
    for (int port = 0; port < NUM_PORTS; port++) begin
      test_keep_one_in_n(SAMPLE_MODE,  1,  1, port);
      test_keep_one_in_n(SAMPLE_MODE,  1,  2, port);
      test_keep_one_in_n(SAMPLE_MODE,  1, 10, port);
      test_keep_one_in_n(SAMPLE_MODE,  2,  1, port);
      test_keep_one_in_n(SAMPLE_MODE,  2,  2, port);
      test_keep_one_in_n(SAMPLE_MODE,  2,  3, port);
      test_keep_one_in_n(SAMPLE_MODE,  2, 10, port);
      test_keep_one_in_n(SAMPLE_MODE,  2, 51, port);
      test_keep_one_in_n(SAMPLE_MODE,  3,  1, port);
      test_keep_one_in_n(SAMPLE_MODE,  3,  2, port);
      test_keep_one_in_n(SAMPLE_MODE,  3,  3, port);
      test_keep_one_in_n(SAMPLE_MODE,  3,  4, port);
      test_keep_one_in_n(SAMPLE_MODE,  3, 10, port);
      test_keep_one_in_n(SAMPLE_MODE,  3, 53, port);
      test_keep_one_in_n(SAMPLE_MODE, 11,  1, port);
      test_keep_one_in_n(SAMPLE_MODE, 11,  7, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 10, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 11, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 12, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 13, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 20, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 21, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 32, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 33, port);
      test_keep_one_in_n(SAMPLE_MODE, 11, 34, port);
      test_keep_one_in_n(SAMPLE_MODE, 2**WIDTH_N-1, 2**WIDTH_N-2, port);
      test_keep_one_in_n(SAMPLE_MODE, 2**WIDTH_N-1, 2**WIDTH_N-1, port);
      test_keep_one_in_n(SAMPLE_MODE, 2**WIDTH_N-1, 2**WIDTH_N  , port);
      test_keep_one_in_n(SAMPLE_MODE, 2**WIDTH_N-1, 2*(2**WIDTH_N-3), port);
      test_keep_one_in_n(SAMPLE_MODE, 2**WIDTH_N-1, 2*(2**WIDTH_N-2), port);
      test_keep_one_in_n(SAMPLE_MODE, 2**WIDTH_N-1, 2*(2**WIDTH_N-1), port);
    end
    test.end_test();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_keep_one_in_n_tb

`default_nettype wire
