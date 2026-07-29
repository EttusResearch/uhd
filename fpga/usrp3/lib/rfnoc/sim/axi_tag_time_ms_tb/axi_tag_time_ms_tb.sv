//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_tag_time_ms_tb
//
// Description:
//   Testbench for axi_tag_time_ms.  Drives random sample data through the
//   DUT (no downstream block) and verifies:
//     1. Sample data (tdata) passes through unchanged.
//     2. m_axis_dout_tags is zero for all untimed words and for timed words
//        whose timestamp precedes the command timestamp.
//     3. m_axis_dout_tags pulses non-zero for exactly one word — the first
//        output word for which current_timestamp >= fifo_set_time — then
//        returns to zero after the FIFO entry is consumed.
//
//   TUSER layout for to_dut (AXI-Stream input):
//     [TEOB_POS_IN]                            = End of Burst (EOB)
//     [THASTIME_POS_IN]                        = Has time
//     [TIMESTAMP_POS_IN +: CHDR_TIMESTAMP_W]   = Packet timestamp
//
//   TUSER layout for from_dut (AXI-Stream output from axi_tag_time_ms):
//     [0]          = teob  (m_axis_dout_teob)
//     [SPC:1]      = tags  (m_axis_dout_tags[SPC-1:0])
//     [USER_W-1 : SPC+1] = 0 (unused)
//

`default_nettype none

module axi_tag_time_ms_tb #(
  int SPC = 8
);
  `include "test_exec.svh"

  import axi_tag_time_ms_pkg::*;
  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import rfnoc_chdr_utils_pkg::*;
  import ctrlport_pkg::*;
  import ctrlport_bfm_pkg::*;
  import PkgRandom::*;

  //---------------------------------------------------------------------------
  // Testbench Parameters
  //---------------------------------------------------------------------------

  localparam real CLK_PERIOD       = 4.0;
  localparam int  SAMP_W           = 32;
  localparam int  DATA_W           = SPC * SAMP_W;
  localparam int  TICK_RATE_W      = 16;
  // Max FIFO depth = 2^CMD_FIFO_SIZE
  localparam int  CMD_FIFO_DEPTH   = 1 << 5;
  // cycles from ctrlport write to FIFO output
  localparam int  CMD_LATENCY      = 4;

  // One tick per sample; one word = SPC ticks
  localparam logic [TICK_RATE_W-1:0] TICKS_PER_SAMPLE = 1;

  // Timestamp increment per word
  localparam longint TS_INCR      = TICKS_PER_SAMPLE * SPC;

  // Input TUSER bit positions
  localparam int TEOB_POS_IN      = 0;
  localparam int THASTIME_POS_IN  = 1;
  localparam int TIMESTAMP_POS_IN = 2;
  // USER_W = 2 + CHDR_TIMESTAMP_W; large enough for both input and output
  // (output uses only SPC+1 lower bits, always <= 66)
  localparam int USER_W           = 2 + CHDR_TIMESTAMP_W;

  // Output TUSER bit positions
  localparam int TEOB_POS_OUT = 0;
  localparam int TAGS_POS_OUT = 1;   // tags[SPC-1:0] in bits [SPC:1]

  localparam logic VERBOSE    = 1'b0;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk, rst;
  sim_clock_gen #(CLK_PERIOD) clk_gen (clk, rst);

  //---------------------------------------------------------------------------
  // AXI-Stream BFM
  //---------------------------------------------------------------------------

  AxiStreamIf #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W), .TKEEP(0))
    to_dut (clk, rst);
  AxiStreamIf #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W), .TKEEP(0))
    from_dut (clk, rst);
  AxiStreamIf #(.DATA_WIDTH(CTRLPORT_DATA_W), .USER_WIDTH(1), .TKEEP(0))
    cmd_out (clk, rst);

  AxiStreamBfm #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W), .TKEEP(0))
    axis_bfm = new(to_dut, from_dut);

  typedef AxiStreamPacket #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W)) axis_pkt_t;

  //---------------------------------------------------------------------------
  // CtrlPort BFM
  //---------------------------------------------------------------------------

  ctrlport_if tag_ctrlport_if (.clk(clk), .rst(rst));
  ctrlport_bfm tag_ctrlport_bfm = new(tag_ctrlport_if);

  //---------------------------------------------------------------------------
  // Typedefs
  //---------------------------------------------------------------------------
  // Sideband info packed into tuser
  typedef struct packed {
    logic [CHDR_TIMESTAMP_W-1:0] timestamp;
    logic                        has_time;
    logic                        eob;
  } axis_tuser_t;

  axis_tuser_t DEFAULT_TUSER = '{timestamp: '0, has_time: 0, eob: 0};

  // Packet config for testing
  typedef struct packed {
    int num_bursts;
    int num_pkts;
    int pkt_words;
    int num_tags;
  } pkt_cfg_t;

  pkt_cfg_t DEFAULT_PKT_CFG = '{num_bursts: 1, num_pkts: 5,
                              pkt_words: 16, num_tags: 1};

  //---------------------------------------------------------------------------
  // DUT Output Wires
  //---------------------------------------------------------------------------

  logic                        clear;
  logic [SPC-1:0][SAMP_W-1:0]  dout_tdata;
  logic [SPC-1:0]              dout_tags;
  logic                        dout_tlast;
  logic                        dout_tvalid;
  logic                        dout_thastime;
  logic [CHDR_TIMESTAMP_W-1:0] dout_ttimestamp;
  logic                        dout_teob;
  logic                        dout_tready;
  logic                        cmd_fifo_full;

  // Route DUT outputs to from_dut BFM interface.
  // TUSER packs: tags in bits [SPC:1], teob in bit 0; upper bits zero.
  assign from_dut.tdata  = {dout_tdata};
  assign from_dut.tlast  = dout_tlast;
  assign from_dut.tvalid = dout_tvalid;
  assign dout_tready     = from_dut.tready;
  assign from_dut.tuser  = USER_W'({dout_tags, dout_teob});
  // Pop cmd FIFO when tagged sample arrives
  assign cmd_out.tready  = from_dut.tready;

  //---------------------------------------------------------------------------
  // DUT: axi_tag_time_ms
  //---------------------------------------------------------------------------

  axi_tag_time_ms #(
    .SPC          (SPC),
    .SAMP_W       (SAMP_W),
    .TICK_RATE_W  (TICK_RATE_W),
    .CMD_DATA_W   (CTRLPORT_DATA_W),
    .MSB_ALIGN    (0)
  ) dut (
    .clk                    (clk),
    .rst                    (rst),
    .cmd_fifo_full          (cmd_fifo_full),
    .s_axis_din_tdata       (to_dut.tdata),
    .s_axis_din_tlast       (to_dut.tlast),
    .s_axis_din_tvalid      (to_dut.tvalid),
    .s_axis_din_thas_time   (to_dut.tuser[THASTIME_POS_IN]),
    .s_axis_din_ttimestamp  (to_dut.tuser[TIMESTAMP_POS_IN +: CHDR_TIMESTAMP_W]),
    .s_axis_din_teob        (to_dut.tuser[TEOB_POS_IN]),
    .s_axis_din_tlength     ('0),
    .s_axis_din_tready      (to_dut.tready),
    .m_axis_dout_tdata      (dout_tdata),
    .m_axis_dout_ttags      (dout_tags),
    .m_axis_dout_tlast      (dout_tlast),
    .m_axis_dout_tvalid     (dout_tvalid),
    .m_axis_dout_thas_time  (dout_thastime),
    .m_axis_dout_ttimestamp (dout_ttimestamp),
    .m_axis_dout_teob       (dout_teob),
    .m_axis_dout_tlength    (),
    .m_axis_dout_tready     (dout_tready),
    .m_axis_cmd_tdata       (cmd_out.tdata),
    .m_axis_cmd_tvalid      (cmd_out.tvalid),
    .m_axis_cmd_tuser       (cmd_out.tuser),
    .m_axis_cmd_tready      (cmd_out.tready),
    .ctrlport_req_wr        (tag_ctrlport_if.req.wr),
    .ctrlport_req_rd        (tag_ctrlport_if.req.rd),
    .ctrlport_req_addr      (tag_ctrlport_if.req.addr),
    .ctrlport_req_data      (tag_ctrlport_if.req.data),
    .ctrlport_req_has_time  (tag_ctrlport_if.req.has_time),
    .ctrlport_req_time      (tag_ctrlport_if.req.timestamp),
    .ctrlport_resp_ack      (tag_ctrlport_if.resp.ack),
    .ctrlport_resp_data     (tag_ctrlport_if.resp.data),
    .ctrlport_resp_status   (tag_ctrlport_if.resp.status)
  );

  //---------------------------------------------------------------------------
  // Helper Functions and Tasks
  //---------------------------------------------------------------------------

  // Generate a random packet.
  // length: number of words in packet
  // tuser : packs timestamp, has_time, eob for each word;
  //         default is no time, no eob, timestamp 0
  function automatic axis_pkt_t gen_rand_pkt(
    int     length,
    axis_tuser_t tuser = DEFAULT_TUSER
  );
    axis_pkt_t pkt;
    pkt = new();
    repeat (length) begin
      pkt.data.push_back(Rand#(.WIDTH(DATA_W))::rand_bit());
      pkt.user.push_back(tuser);
    end
    return pkt;
  endfunction : gen_rand_pkt

  // Do untimed write to REG_TAG_ADDR
  task automatic write_reg(logic [CTRLPORT_DATA_W-1:0] data);
    tag_ctrlport_bfm.write(REG_TAG_ADDR, data);
  endtask : write_reg

  // Do timed write to REG_TAG_ADDR
  task automatic timed_write_reg(
    logic [CTRLPORT_DATA_W-1:0] data,
    logic [CTRLPORT_TIME_W-1:0] ts
  );
    tag_ctrlport_bfm.write_timed(REG_TAG_ADDR, ts, data);
  endtask : timed_write_reg

  //---------------------------------------------------------------------------
  // Command Output Monitor
  //
  // Captures every m_axis_cmd transaction (tvalid && tready) into a queue.
  // Tests call clear_cmd_events() before each verification window.
  // Each entry records the data value (tdata) and has_time flag (tuser).
  //---------------------------------------------------------------------------

  typedef struct packed {
    logic [CTRLPORT_DATA_W-1:0] tdata;
    logic                       tuser;
  } cmd_event_t;

  cmd_event_t cmd_events[$];

  always @(posedge clk) begin
    if (cmd_out.tvalid && cmd_out.tready)
      cmd_events.push_back({cmd_out.tdata, cmd_out.tuser[0]});
  end

  task automatic clear_cmd_events();
    cmd_events.delete();
  endtask : clear_cmd_events

  //---------------------------------------------------------------------------
  // Ctrlport read/write test
  //
  // Case 1: write a random value and read it back.
  // Case 2: write 2^CMD_FIFO_SIZE (= 32) timed commands to saturate the
  //         cmd FIFO; after the last write cmd_fifo_full must assert (no FIFO
  //         ack for that write); a read must return the last written value.
  //---------------------------------------------------------------------------
  task automatic ctrlport_rw_test();
    logic [CTRLPORT_DATA_W-1:0] rw_val, readback;

    // ---- Case 1: write a random value then read it back ----
    test.start_test("CtrlPort R/W: write-readback");
    rw_val = $urandom();
    write_reg(rw_val);
    clk_gen.clk_wait_r(1);
    tag_ctrlport_bfm.read(REG_TAG_ADDR, readback);
    `ASSERT_ERROR(readback == '0,
      $sformatf("Readback not supported but got %0h", readback));
    test.end_test();

    // ---- Case 2: fill FIFO with 2^CMD_FIFO_SIZE timed commands; verify
    //              cmd_fifo_full asserts on the last write (no FIFO ack),
    //              then verify read returns the last written value ----
    test.start_test("CtrlPort R/W: FIFO full no-ack on last write till FIFO has space");
    for (int i = 0; i <= CMD_FIFO_DEPTH; i++) begin
      rw_val = $urandom();
      // Far-future timestamp: entries stay queued without incoming sample data.
      timed_write_reg(rw_val, {CTRLPORT_TIME_W{1'b1}});
      clk_gen.clk_wait_r(2);
      if (i == CMD_FIFO_DEPTH) begin
        `ASSERT_ERROR(cmd_fifo_full == 1'b1,
          "cmd_fifo_full should be asserted after writing 2^CMD_FIFO_SIZE timed commands");
      end else begin
      `ASSERT_ERROR(cmd_fifo_full == 1'b0,
        $sformatf("cmd_fifo_full should not be asserted yet, after timed write %0d", i));
      end
    end
    fork
      begin : fifo_full_late_ack
      // Ack for this write should arrive once the FIFO has space
      // The BFM waits for ack and timesout if not received.
        timed_write_reg(rw_val, {CTRLPORT_TIME_W{1'b1}});
        if (VERBOSE) $display("Received FIFO ack for last write after FIFO was popped");
      end
      begin : send_pkt
      // Send single packet of 4 words to pop the FIFO and allow the last write to complete.
        axis_tuser_t tuser =
        '{timestamp:'0,
          has_time : 0,
          eob      : 1};
        axis_pkt_t pkt = gen_rand_pkt(4, .tuser(tuser));
        `ASSERT_ERROR(cmd_fifo_full == 1'b1,
          "cmd_fifo_full should still be asserted before FIFO is popped");
        clk_gen.clk_wait_r(4);
        axis_bfm.put(pkt);
        axis_bfm.get(pkt);
      end
      begin : pop_fifo
        @(posedge clk iff (dout_tvalid && dout_tready));
        // In this test: first command in FIFO is untimed
        `ASSERT_ERROR(cmd_out.tvalid,
          "cmd_out.tvalid must be asserted on first output word after untimed write");
      end
    join
    // Flush the command FIFO so subsequent tests start from a clean state.
    clk_gen.reset(1);
    @(negedge rst);
    test.end_test();
  endtask : ctrlport_rw_test

  //---------------------------------------------------------------------------
  // Test: Data Passthrough
  //
  // Untimed command; send random untimed data with configurable stall
  // probabilities. Verify that data passes through unchanged and that
  // tags are zero for every word and command streams on the first word of
  // output packet.
  //---------------------------------------------------------------------------
  task automatic data_passthrough_test(
    pkt_cfg_t pkt_cfg           = DEFAULT_PKT_CFG,
    int       master_stall_prob = 0,
    int       slave_stall_prob  = 0
  );
    axis_pkt_t   sent[$], recv_pkt;
    axis_tuser_t tuser;

    logic [CTRLPORT_DATA_W-1:0] write_value = $urandom();

    test.start_test($sformatf("Data Passthrough (master stall=%0d slave stall=%0d)",
                              master_stall_prob, slave_stall_prob));
    clear_cmd_events();

    axis_bfm.set_master_stall_prob(master_stall_prob);
    axis_bfm.set_slave_stall_prob(slave_stall_prob);

    write_reg(write_value);
    clk_gen.clk_wait_r(2);

    // Check that cmd fires (tvalid=1, tdata correct, tuser=0) on the first
    // output word — the first data_valid after the untimed ctrlport write.
    fork
      begin : first_word_cmd_check
        @(posedge clk iff (dout_tvalid && dout_tready));
        `ASSERT_ERROR(cmd_out.tvalid,
          "cmd_out.tvalid must be asserted on first output word after untimed write");
        `ASSERT_ERROR(cmd_out.tdata == CTRLPORT_DATA_W'(write_value),
          $sformatf("Untimed cmd tdata: expected %0h, got %0h",
                    write_value, cmd_out.tdata));
        `ASSERT_ERROR(cmd_out.tuser[0] == 1'b0,
          "Untimed cmd tuser should be 0 (no has_time)");
      end
    join_none

    for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
      axis_tuser_t tuser =
        '{timestamp: '0,
          has_time: 0,
          eob: (pkt_num == pkt_cfg.num_pkts - 1)};
      axis_pkt_t pkt = gen_rand_pkt(pkt_cfg.pkt_words, .tuser(tuser));
      sent.push_back(pkt);
      axis_bfm.put(pkt);
    end

    for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
      axis_bfm.get(recv_pkt);
      `ASSERT_ERROR(recv_pkt.data.size() == sent[pkt_num].data.size(),
        $sformatf("Pkt %0d: expected %0d words, got %0d",
                  pkt_num, sent[pkt_num].data.size(), recv_pkt.data.size()));
      foreach (recv_pkt.data[word_idx]) begin
        `ASSERT_ERROR(recv_pkt.data[word_idx] == sent[pkt_num].data[word_idx],
          $sformatf("Pkt %0d, word %0d: tdata mismatch: sent %0h, got %0h",
                    pkt_num, word_idx, sent[pkt_num].data[word_idx], recv_pkt.data[word_idx]));
        `ASSERT_ERROR(recv_pkt.user[word_idx][TAGS_POS_OUT +: SPC] == '0,
          $sformatf("Pkt %0d, word %0d: tags should be 0, got %0b",
                    pkt_num, word_idx, recv_pkt.user[word_idx][TAGS_POS_OUT +: SPC]));
      end
    end

    // Post-hoc: exactly one cmd_event, correct data, tuser=0
    `ASSERT_ERROR(cmd_events.size() == 1,
      $sformatf("Expected 1 cmd_event for untimed write, got %0d", cmd_events.size()));
    `ASSERT_ERROR(cmd_events[0].tuser == 1'b0,
      "Untimed cmd_event: tuser should be 0");
    `ASSERT_ERROR(cmd_events[0].tdata == CTRLPORT_DATA_W'(write_value),
      $sformatf("Untimed cmd_event tdata: expected %0h, got %0h",
                write_value, cmd_events[0].tdata));

    test.end_test();
  endtask : data_passthrough_test

  //---------------------------------------------------------------------------
  // Test: Timed Tag Transition
  //
  // Issues a timed ctrlport write every odd burst, then
  // sends num_pkts timed packets starting at timestamp 0.  Verifies:
  //   - All data passes through unchanged.
  //   - Tags are 0 for all words before tagged word index.
  //   - Tags are non-zero for exactly the one word at tagged word index (the
  //     first output word where current_timestamp >= command time fires the pulse).
  //     FIFO entry is consumed on the tagged transfer.
  //   - Tags return to 0 for all subsequent words.
  //   - cmd_fifo_full is deasserted after the command fires.
  //
  //---------------------------------------------------------------------------
  task automatic timed_tag_test(
    pkt_cfg_t pkt_cfg           = DEFAULT_PKT_CFG,
    int       master_stall_prob = 0,
    int       slave_stall_prob  = 0
  );
    axis_pkt_t   sent[$], recv_pkt;
    longint      cmd_ts;
    int          tag_word[$];
    bit          write_cmd = 1'b1;

    // Test tags first sample of word always
    logic [SPC-1:0]             expected_tag = 1;
    logic [CTRLPORT_DATA_W-1:0] timed_data;

    int     idx    = 0;
    longint pkt_ts = 0;

    test.start_test($sformatf("Timed Tag Transition (master stall=%0d slave stall=%0d)",
                    master_stall_prob, slave_stall_prob));
    // Remove residual timed commands from previous tests (if any)
    clk_gen.reset(1);
    @(negedge rst);
    clear_cmd_events();
    for (int tag_idx = 0; tag_idx < pkt_cfg.num_tags; tag_idx++) begin : gen_sample_tags
      // Randomize command timestamp
      tag_word.push_back(
        $urandom_range(pkt_cfg.num_pkts * pkt_cfg.pkt_words, CMD_LATENCY));
      cmd_ts     = tag_word[tag_idx] * TS_INCR;
      if (VERBOSE)
        $display ("Timed command at timestamp %0d, (tag_word=%0d)", cmd_ts, tag_word[tag_idx]);
    end
    axis_bfm.set_master_stall_prob(master_stall_prob);
    axis_bfm.set_slave_stall_prob(slave_stall_prob);

    repeat (pkt_cfg.num_bursts) begin
      if (write_cmd) begin
        timed_data = $urandom();
        timed_write_reg(timed_data, CTRLPORT_TIME_W'(pkt_ts + tag_word[idx] * TS_INCR));
      end

      fork
        begin : send_pkts
          for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
            axis_tuser_t tuser =
              '{timestamp: (pkt_num == 0) ? pkt_ts: '0,
              has_time: (pkt_num == 0),
              eob: (pkt_num == pkt_cfg.num_pkts - 1)};
            axis_pkt_t pkt = gen_rand_pkt(pkt_cfg.pkt_words, .tuser(tuser));
            pkt_ts += pkt_cfg.pkt_words * TS_INCR;
            sent.push_back(pkt);
            axis_bfm.put(pkt);
          end
        end

        begin : recv_and_check_tag
          for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
            axis_bfm.get(recv_pkt);
            `ASSERT_ERROR(recv_pkt.data.size() == sent[pkt_num].data.size(),
              $sformatf("Pkt %0d: expected %0d words, got %0d",
                        pkt_num, sent[pkt_num].data.size(), recv_pkt.data.size()));
            foreach (recv_pkt.data[word_idx]) begin
              int             abs_word = pkt_num * pkt_cfg.pkt_words + word_idx;
              logic [SPC-1:0] got_tags = recv_pkt.user[word_idx][TAGS_POS_OUT +: SPC];

              `ASSERT_ERROR(recv_pkt.data[word_idx] == sent[pkt_num].data[word_idx],
                $sformatf("Pkt %0d, word %0d: data mismatch: sent %0h, got %0h",
                          pkt_num, word_idx, sent[pkt_num].data[word_idx],
                          recv_pkt.data[word_idx]));

              if (write_cmd && (abs_word < tag_word[idx])) begin
                `ASSERT_ERROR(got_tags == '0,
                  $sformatf({"Pkt %0d, word %0d (word in burst=%0d):",
                            "expected tags=0 before cmd, got %0b"},
                            pkt_num, word_idx, abs_word, got_tags));
              end else if (write_cmd && (abs_word == tag_word[idx])) begin
                `ASSERT_ERROR(got_tags == expected_tag,
                  $sformatf({"Pkt %0d, word %0d (word in burst=%0d):",
                            "expected non-zero tag at cmd ts, got 0"},
                            pkt_num, word_idx, abs_word));
                // cmd_out must fire simultaneously with the tag
                `ASSERT_ERROR(cmd_events.size() > 0,
                  $sformatf({"Pkt %0d, word %0d: expected cmd_event at tagged word,",
                            " none captured"}, pkt_num, word_idx));
                `ASSERT_ERROR(cmd_events[0].tuser == 1'b1,
                  $sformatf({"Pkt %0d, word %0d: timed cmd tuser should be 1, got %0b"},
                            pkt_num, word_idx, cmd_events[0].tuser));
                `ASSERT_ERROR(cmd_events[0].tdata == timed_data,
                  $sformatf({"Pkt %0d, word %0d: timed cmd tdata expected %0h, got %0h"},
                            pkt_num, word_idx, timed_data, cmd_events[0].tdata));
                void'(cmd_events.pop_front());
              end else begin
                `ASSERT_ERROR(got_tags == '0,
                  $sformatf({"Pkt %0d, word %0d (word in burst=%0d): ",
                            "expected tags=0 after one-word pulse, got %0b"},
                            pkt_num, word_idx, abs_word, got_tags));
              end
            end
          end

          `ASSERT_ERROR(cmd_fifo_full == 1'b0,
            "cmd_fifo_full should be deasserted after command fires");
          `ASSERT_ERROR(cmd_events.size() == 0,
            $sformatf("Unexpected extra cmd_events after timed test: %0d remaining",
                      cmd_events.size()));
          sent.delete();
        end
      join
      if (write_cmd && (idx < tag_word.size() - 1)) idx++;
      // Alternate between writing a timed command and not writing one for each burst
      // Ensure data passes through without tags when no command is written.
      write_cmd = ~write_cmd;
    end
    test.end_test();
  endtask : timed_tag_test

  //---------------------------------------------------------------------------
  // Test: Multiple Timed Commands
  //
  // Issues multiple timed ctrlport writes whose timestamps are spread across the
  // burst (each at a randomly chosen word boundary plus an intra-word sample offset).
  // Sends num_pkts timed packets starting at a random base timestamp.  Verifies:
  //   - All data passes through unchanged.
  //   - Tags are non-zero for exactly the word that contains each command's
  //     timestamp (one-word pulse per command).
  //   - Tags are zero for all other words.
  //   - cmd_fifo_full is deasserted after all commands have fired.
  //
  //---------------------------------------------------------------------------
  task automatic multiple_timed_cmds_test(
    pkt_cfg_t pkt_cfg           = DEFAULT_PKT_CFG,
    int       master_stall_prob = 0,
    int       slave_stall_prob  = 0
  );
    axis_pkt_t   sent[$], recv_pkt;
    longint      cmd_ts;
    axis_tuser_t tuser;
    longint      pkt_ts = $urandom();

    int                         tag_sample;
    int                         tag_word[$];
    logic [CTRLPORT_DATA_W-1:0] tag_data[$];
    logic [SPC-1:0]             expected_tag[$];

    int timed_cmd_idx = 0;

    test.start_test($sformatf({"Multiple Timed Commands (start time: %0h('h), ",
                    "master stall=%0d slave stall=%0d)"},
                    pkt_ts, master_stall_prob, slave_stall_prob));
    // Remove residual timed commands from previous tests (if any)
    clk_gen.reset(1);
    @(negedge rst);
    // Commands spread across the burst
    for (int tag = 0; tag < pkt_cfg.num_tags; tag++) begin
      int total    = pkt_cfg.num_bursts * pkt_cfg.num_pkts * pkt_cfg.pkt_words;
      // Ensure  tags are 4 words apart due to module pipeline latency
      int seg_lo   = (tag==0) ? CMD_LATENCY : (tag * total) / pkt_cfg.num_tags;
      int prev_min = (tag==0) ? 0 : tag_word[tag-1] + CMD_LATENCY;
      // Ensure tags are in increasing order
      int lo       = (seg_lo > prev_min) ? seg_lo : prev_min;
      int hi       = ((tag + 1) * total) / pkt_cfg.num_tags - 1;
      tag_word.push_back($urandom_range(hi, lo));
      tag_sample        = $urandom_range(SPC-1);
      expected_tag.push_back(1 << tag_sample);

      cmd_ts = pkt_ts + (tag_word[tag] * TS_INCR) + (tag_sample * TICKS_PER_SAMPLE);
      if (VERBOSE) 
        $display ("Timed command at timestamp %0h('h), (tag_word=%0d, tag_sample=%0d)",
                  cmd_ts, tag_word[tag], tag_sample);

      tag_data.push_back($urandom());

      timed_write_reg(tag_data[tag], CTRLPORT_TIME_W'(cmd_ts));
    end

    axis_bfm.set_master_stall_prob(master_stall_prob);
    axis_bfm.set_slave_stall_prob(slave_stall_prob);

    begin : sample_stream
      int burst_word_offset = 0;
      repeat (pkt_cfg.num_bursts) begin
      fork
        begin : send_pkts
          for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
            axis_tuser_t tuser =
              '{timestamp: pkt_ts,
                has_time: (pkt_num == 0),
                eob: (pkt_num == pkt_cfg.num_pkts - 1)};
            axis_pkt_t pkt = gen_rand_pkt(pkt_cfg.pkt_words, .tuser(tuser));
            pkt_ts += pkt_cfg.pkt_words * TS_INCR;
            sent.push_back(pkt);
            axis_bfm.put(pkt);
          end
        end
        begin : recv_and_check_tag
          for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
            axis_bfm.get(recv_pkt);
            `ASSERT_ERROR(recv_pkt.data.size() == sent[pkt_num].data.size(),
              $sformatf("Pkt %0d: expected %0d words, got %0d",
                        pkt_num, sent[pkt_num].data.size(), recv_pkt.data.size()));
            foreach (recv_pkt.data[word_idx]) begin
              int             abs_word = burst_word_offset +
                                         pkt_num * pkt_cfg.pkt_words + word_idx;
              logic [SPC-1:0] got_tags = recv_pkt.user[word_idx][TAGS_POS_OUT +: SPC];

              `ASSERT_ERROR(recv_pkt.data[word_idx] == sent[pkt_num].data[word_idx],
                $sformatf("Pkt %0d, word %0d: data mismatch: sent %0h, got %0h",
                          pkt_num, word_idx, sent[pkt_num].data[word_idx], recv_pkt.data[word_idx]));

              begin : check_tags
                bit is_tag_word = 0;
                logic [SPC-1:0] check_tag = '0;
                foreach (tag_word[i]) begin
                  is_tag_word = (abs_word == tag_word[i]);
                  if (is_tag_word) begin
                    check_tag = expected_tag[i];
                    break;
                  end
                end
                if (is_tag_word) begin
                  `ASSERT_ERROR(got_tags == check_tag,
                    $sformatf({"Pkt %0d, word %0d (word in burst=%0d):",
                              "expected non-zero tag at cmd ts, got 0"},
                              pkt_num, word_idx, abs_word));
                  // cmd fires simultaneously with the tag; events arrive in word order
                  `ASSERT_ERROR(cmd_events.size() > 0,
                    $sformatf({"Pkt %0d, word %0d: expected cmd_event at tagged word,",
                              " none captured"}, pkt_num, word_idx));
                  `ASSERT_ERROR(cmd_events[0].tuser == 1'b1,
                    $sformatf({"Pkt %0d, word %0d: timed cmd tuser should be 1, got %0b"},
                              pkt_num, word_idx, cmd_events[0].tuser));
                  `ASSERT_ERROR(cmd_events[0].tdata == tag_data[timed_cmd_idx],
                    $sformatf({"Pkt %0d, word %0d: timed cmd tdata expected %0h, got %0h"},
                              pkt_num, word_idx, tag_data[timed_cmd_idx], cmd_events[0].tdata));
                  void'(cmd_events.pop_front());
                  timed_cmd_idx++;
                end else begin
                  `ASSERT_ERROR(got_tags == '0,
                    $sformatf({"Pkt %0d, word %0d (word in burst=%0d): ",
                              "expected tags=0 for non-tag word, got %0b"},
                              pkt_num, word_idx, abs_word, got_tags));
                end
              end
            end
          end
          `ASSERT_ERROR(cmd_fifo_full == 1'b0,
            "cmd_fifo_full should be deasserted after command fires");
        end
      join
      burst_word_offset += pkt_cfg.num_pkts * pkt_cfg.pkt_words;
      sent.delete();
      end
      `ASSERT_ERROR(cmd_events.size() == 0,
        $sformatf("Unexpected extra cmd_events after multiple-timed test: %0d remaining",
                  cmd_events.size()));
    end
    test.end_test();
  endtask : multiple_timed_cmds_test

  //---------------------------------------------------------------------------
  // Test: Mixed Timed and Untimed Commands (Sequential Issuance)
  //
  // Sends a single timed burst while issuing timed and untimed commands at
  // separate points in time - not all upfront.  The second batch of commands
  // is issued mid-burst so that command issuance genuinely interleaves with 
  // the ongoing data stream.
  //
  // Command sequence:
  //   Phase 1 (before data starts):
  //     a) write_reg(untimed_val[0]) - untimed; no tag expected.
  //     b) timed_write_reg(timed_val[0], cmd_ts[0]) - queued for tag_word[0]
  //        in the first half of the burst.
  //
  //   Phase 2 (mid-burst, concurrent with receiver):
  //     c) write_reg(untimed_val[1]) - another untimed register update,
  //        no tag expected
  //     d) timed_write_reg(timed_val[1], cmd_ts[1]) - queued for tag_word[1]
  //        in the second half of the burst.
  //
  // Verifies:
  //   - All data passes through unchanged.
  //   - m_axis_dout_tags is non-zero for words tag_word[0] and
  //     tag_word[1]; zero for every other word.
  //
  // tag_word[0] is chosen from the first half of the burst; tag_word[1] from
  // the second half with enough margin (> 4 packet) so the timed command
  // issued mid-burst propagates through the FIFO before its timestamp arrives.
  //---------------------------------------------------------------------------
  task automatic mixed_timed_untimed_test(
    pkt_cfg_t pkt_cfg           = DEFAULT_PKT_CFG,
    int       master_stall_prob = 0,
    int       slave_stall_prob  = 0
  );
    int mid_word  = (pkt_cfg.num_pkts / 2) * pkt_cfg.pkt_words;
    int mid2_lo   = mid_word + pkt_cfg.pkt_words + CMD_LATENCY + 1;

    axis_pkt_t      sent[$], recv_pkt;
    int             tag_word[2];
    longint         cmd_ts[2];
    logic [SPC-1:0] expected_tag[2];
    longint         pkt_ts = 0;

    logic [CTRLPORT_DATA_W-1:0] untimed_val[2], timed_val[2];

    test.start_test($sformatf({"Mixed Timed/Untimed Commands",
                              "(master stall=%0d slave stall=%0d)"},
                              master_stall_prob, slave_stall_prob));
    // Remove residual timed commands from previous tests (if any)
    clk_gen.reset(1);
    @(negedge rst);

    repeat (pkt_cfg.num_bursts) begin
      int tag_sample;
      clear_cmd_events();
      // tag_word[0]: first half of the burst
      // 4 cycles latency between command issuance
      // t0: untimed cmd, t0+4: timed cmd
      tag_word[0] = $urandom_range(mid_word - 1, CMD_LATENCY+1);
      tag_sample  = $urandom_range(SPC-1);
      expected_tag[0] = 1 << tag_sample;
      cmd_ts[0]   = longint'(tag_word[0]) * TS_INCR + longint'(tag_sample) * TICKS_PER_SAMPLE;
      // tag_word[1]: second half of the burst
      tag_word[1] = $urandom_range(pkt_cfg.num_pkts * pkt_cfg.pkt_words - 1, mid2_lo);
      tag_sample  = $urandom_range(SPC-1);
      expected_tag[1] = 1 << tag_sample;
      cmd_ts[1]   = longint'(tag_word[1]) * TS_INCR + longint'(tag_sample) * TICKS_PER_SAMPLE;
      foreach (untimed_val[i]) untimed_val[i] = $urandom();
      foreach (timed_val[i])   timed_val[i]   = $urandom();

      if (VERBOSE)
        $display($sformatf({"Interleaved timed/untimed test:",
                "tag_word[0]=%0d (ts=%0d), tag_word[1]=%0d (ts=%0d)"},
                tag_word[0], cmd_ts[0], tag_word[1], cmd_ts[1]));

      axis_bfm.set_master_stall_prob(master_stall_prob);
      axis_bfm.set_slave_stall_prob(slave_stall_prob);

      // ---- Issue first untimed then first timed command ----
      // Untimed write: No tags.
      write_reg(untimed_val[0]);
      clk_gen.clk_wait_r(2);
      // Timed write for tag_word[0]: queued in FIFO.
      timed_write_reg(timed_val[0], CTRLPORT_TIME_W'(cmd_ts[0]));

      // Queue all packets into the BFM send queue (non-blocking)
      for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
        axis_tuser_t tuser = '{
          timestamp: '0,
          has_time:  (pkt_num == 0),
          eob:       (pkt_num == pkt_cfg.num_pkts - 1)
        };
        axis_pkt_t pkt = gen_rand_pkt(pkt_cfg.pkt_words, .tuser(tuser));
        sent.push_back(pkt);
        axis_bfm.put(pkt);
      end

      // ---- Receive output while issuing second command batch ----
      fork
        // Thread A: collect and verify all packets
        begin : recv_and_check
          for (int pkt_num = 0; pkt_num < pkt_cfg.num_pkts; pkt_num++) begin
            axis_bfm.get(recv_pkt);
            `ASSERT_ERROR(recv_pkt.data.size() == pkt_cfg.pkt_words,
              $sformatf("Pkt %0d: expected %0d words, got %0d",
                        pkt_num, pkt_cfg.pkt_words, recv_pkt.data.size()));
            foreach (recv_pkt.data[word_idx]) begin
              int             abs_word = pkt_num * pkt_cfg.pkt_words + word_idx;
              logic [SPC-1:0] got_tags = recv_pkt.user[word_idx][TAGS_POS_OUT +: SPC];

              `ASSERT_ERROR(recv_pkt.data[word_idx] == sent[pkt_num].data[word_idx],
                $sformatf("Pkt %0d, word %0d: data mismatch: sent %0h, got %0h",
                          pkt_num, word_idx,
                          sent[pkt_num].data[word_idx], recv_pkt.data[word_idx]));

              begin : check_tags
                bit is_tag_word = 0;
                logic [SPC-1:0] check_tag = '0;
                foreach(tag_word[i]) begin
                  is_tag_word = (abs_word == tag_word[i]);
                  if (is_tag_word) begin
                    check_tag = expected_tag[i];
                    break;
                  end
                end
                if (is_tag_word) begin
                  `ASSERT_ERROR(got_tags == check_tag,
                    $sformatf({"Pkt %0d, word %0d (abs=%0d): ",
                              "expected non-zero tag at cmd ts, got 0"},
                              pkt_num, word_idx, abs_word));
                end else begin
                  `ASSERT_ERROR(got_tags == '0,
                    $sformatf({"Pkt %0d, word %0d (abs=%0d): ",
                              "expected tags=0 for non-cmd word, got %0b"},
                              pkt_num, word_idx, abs_word, got_tags));
                end
              end
            end
          end
        end : recv_and_check

        // Thread B: once MID_WORD output words have transferred, issue the
        // second untimed command (no tag) then the second timed command.
        // Waiting for actual output transfers — not just BFM queuing — ensures
        // the timestamp has advanced far enough to safely issue cmd_ts[1].
        begin : mid_burst_cmds
          repeat (mid_word) begin
            @(posedge clk iff (dout_tvalid && dout_tready));
          end
          // Second untimed write
          write_reg(untimed_val[1]);
          clk_gen.clk_wait_r(1);
          // Second timed write for tag_word[1]: queued in FIFO.
          timed_write_reg(timed_val[1], CTRLPORT_TIME_W'(cmd_ts[1]));
        end : mid_burst_cmds
      join

      // Verify cmd_events: Untimed events (tuser=0) must match untimed_val[] in
      // order; timed events (tuser=1) must match timed_val[] in order.
      begin : check_cmd_events_mixed
        cmd_event_t untimed_got[$], timed_got[$];
        int n_untimed, n_timed;
        foreach (cmd_events[i]) begin
          if (cmd_events[i].tuser == 1'b0) untimed_got.push_back(cmd_events[i]);
          else                              timed_got.push_back(cmd_events[i]);
        end
        n_untimed = $size(untimed_val);
        n_timed   = $size(timed_val);
        `ASSERT_ERROR(untimed_got.size() == n_untimed,
          $sformatf("Expected %0d untimed cmd_events, got %0d",
                    n_untimed, untimed_got.size()));
        `ASSERT_ERROR(timed_got.size() == n_timed,
          $sformatf("Expected %0d timed cmd_events, got %0d",
                    n_timed, timed_got.size()));
        for (int i = 0; i < n_untimed && i < int'(untimed_got.size()); i++)
          `ASSERT_ERROR(untimed_got[i].tdata == untimed_val[i],
            $sformatf("Untimed cmd_event[%0d]: expected tdata=%0h, got %0h",
                      i, untimed_val[i], untimed_got[i].tdata));
        for (int i = 0; i < n_timed && i < int'(timed_got.size()); i++)
          `ASSERT_ERROR(timed_got[i].tdata == timed_val[i],
            $sformatf("Timed cmd_event[%0d]: expected tdata=%0h, got %0h",
                      i, timed_val[i], timed_got[i].tdata));
      end : check_cmd_events_mixed
      sent.delete();
    end
    test.end_test();
  endtask : mixed_timed_untimed_test

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : main
    test.start_tb($sformatf("axi_tag_time_ms Testbench (SPC=%0d)", SPC));
    tag_ctrlport_bfm.run();
    axis_bfm.run();

    clk_gen.reset(2);
    @(negedge rst);

    ctrlport_rw_test();

    // No backpressure: verify data passthrough and tag behavior
    data_passthrough_test(.master_stall_prob(0),  .slave_stall_prob(0));
    // single word burst, untimed cmd
    data_passthrough_test(.pkt_cfg('{num_bursts: 1, num_pkts: 1,
                                     pkt_words: 1, num_tags: 0}),
                          .master_stall_prob(0), .slave_stall_prob(0));

    // Minimum of 3 word burst for timed cmd due to CMD_LATENCY
    timed_tag_test(.pkt_cfg('{num_bursts: 1, num_pkts: 1, pkt_words: 4, num_tags: 1}));
    timed_tag_test(.pkt_cfg('{num_bursts: 1, num_pkts: 1, pkt_words: 16, num_tags: 1}));
    timed_tag_test(.pkt_cfg('{num_bursts: 5, num_pkts: 5, pkt_words: 16, num_tags: 3}));

    // Stall robustness (tag timing varies under stalls)
    timed_tag_test(.master_stall_prob(20), .slave_stall_prob(80));
    timed_tag_test(.master_stall_prob(80), .slave_stall_prob(20));
    timed_tag_test(.master_stall_prob(50), .slave_stall_prob(50));

    // Queue multiple timed commands in a single burst
    multiple_timed_cmds_test(.pkt_cfg('{num_bursts: 1, num_pkts: 5,
                             pkt_words: 16, num_tags: 3}));
    // Queue max timed commands and check tags over multiple bursts
    multiple_timed_cmds_test(.pkt_cfg('{num_bursts: 3, num_pkts: 5,
                             pkt_words: 64, num_tags: CMD_FIFO_DEPTH}));

    // Interleaved timed and untimed commands issued sequentially mid-burst
    mixed_timed_untimed_test(.pkt_cfg('{num_bursts: 5, num_pkts: 5,
                                        pkt_words: 16, num_tags: 4}));
    mixed_timed_untimed_test(.master_stall_prob(20), .slave_stall_prob(80));
    mixed_timed_untimed_test(.master_stall_prob(80), .slave_stall_prob(20));

    test.end_tb(0);
    clk_gen.kill();
  end

endmodule : axi_tag_time_ms_tb

`default_nettype wire
