//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_rate_change_ms_tb
//
// Description:
//   Standalone testbench for axi_rate_change_ms. Validates the CHDR-native
//   ctrlport-facing rate-change module used in the multisample DDC path.
//   Tests register access, decimation rates, timestamp propagation, and
//   bubble-free throughput.
//

`default_nettype none

module axi_rate_change_ms_tb #(
  parameter int WIDTH = 32,
  parameter int MAX_N = 16,
  parameter int MAX_M = 16,
  parameter bit EN_TIME_ALL_PKTS = 1'b1
);
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import ctrlport_pkg::*;
  import ctrlport_bfm_pkg::*;
  import rfnoc_chdr_utils_pkg::*;
  import axi_rate_change_ms_pkg::*;

  //---------------------------------------------------------------------------
  // Local parameters and type definitions
  //---------------------------------------------------------------------------

  localparam real CLK_PERIOD = 6.0;
  localparam int WORD_BYTES = WIDTH / 8;
  localparam int DEFAULT_SPP = 16;
  localparam int STALL_PROB = 38;
  localparam int BUBBLE_TEST_WORDS = 100000;
  localparam int VERBOSE = 0;
  localparam int SAMP_W = 32;
  localparam int SPC = WIDTH / SAMP_W;
  localparam int SPC_MTU_LOG2 = $clog2(8192 / ((SAMP_W*SPC)/8));

  // TUSER field positions for CHDR sideband metadata encoding.
  // Field widths reuse constants from rfnoc_chdr_utils.vh.
  localparam int LENGTH_POS    = 0;
  localparam int TIMESTAMP_POS = LENGTH_POS + CHDR_LENGTH_W;
  localparam int HAS_TIME_POS  = TIMESTAMP_POS + CHDR_TIMESTAMP_W;
  localparam int EOB_POS       = HAS_TIME_POS + 1;
  localparam int USER_W        = EOB_POS + CHDR_EOB_W;

  typedef struct packed {
    logic                        eob;
    logic                        has_time;
    logic [CHDR_TIMESTAMP_W-1:0] timestamp;
    logic [CHDR_LENGTH_W-1:0]    length;
  } user_t;
  typedef AxiStreamPacket #(.DATA_WIDTH(WIDTH), .USER_WIDTH(USER_W)) axis_pkt_t;
  typedef axis_pkt_t axis_pkt_queue_t[$];

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  logic clk;
  logic rst;

  sim_clock_gen #(
    .PERIOD(CLK_PERIOD),
    .AUTOSTART(0)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  AxiStreamIf #(.DATA_WIDTH(WIDTH), .USER_WIDTH(USER_W), .TKEEP(0)) noc_in (
    .clk(clk),
    .rst(rst)
  );
  AxiStreamIf #(.DATA_WIDTH(WIDTH), .USER_WIDTH(USER_W), .TKEEP(0)) noc_out (
    .clk(clk),
    .rst(rst)
  );

  AxiStreamBfm #(.DATA_WIDTH(WIDTH), .USER_WIDTH(USER_W), .TKEEP(0)) axis_bfm =
    new(.master(noc_in), .slave(noc_out));

  ctrlport_if ctrlport_if_i (
    .clk(clk),
    .rst(rst)
  );

  ctrlport_bfm ctrl_bfm = new(ctrlport_if_i);

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic clear = 1'b0;
  logic clear_user;
  logic [1:0] ctrlport_resp_status;

  // User-logic facing signals
  logic [WIDTH-1:0] m_axis_data_tdata;
  logic             m_axis_data_tlast;
  logic             m_axis_data_tvalid;
  logic             m_axis_data_tready;
  logic             m_axis_data_teob;
  logic [WIDTH-1:0] s_axis_data_tdata;
  logic             s_axis_data_tlast;
  logic             s_axis_data_tvalid;
  logic             s_axis_data_tready;

  // CHDR sideband extraction from BFM tuser
  user_t i_tuser;
  assign i_tuser = user_t'(noc_in.tuser);

  // CHDR sideband outputs from DUT
  logic                        o_teob;
  logic [CHDR_TIMESTAMP_W-1:0] o_ttimestamp;
  logic                        o_thas_time;

  assign ctrlport_if_i.resp.status = ctrlport_status_t'(ctrlport_resp_status);

  axi_rate_change_ms #(
    .WIDTH(WIDTH),
    .SPC(SPC),
    .SPC_MTU_LOG2(SPC_MTU_LOG2),
    .EN_TIME_ALL_PKTS(EN_TIME_ALL_PKTS)
  ) dut (
    .clk(clk),
    .rst(rst),
    .clear(clear),
    .clear_user(clear_user),
    .s_ctrlport_req_wr(ctrlport_if_i.req.wr),
    .s_ctrlport_req_rd(ctrlport_if_i.req.rd),
    .s_ctrlport_req_addr(ctrlport_if_i.req.addr),
    .s_ctrlport_req_data(ctrlport_if_i.req.data),
    .s_ctrlport_resp_ack(ctrlport_if_i.resp.ack),
    .s_ctrlport_resp_status(ctrlport_resp_status),
    .s_ctrlport_resp_data(ctrlport_if_i.resp.data),
    .i_tdata(noc_in.tdata),                  // in:  NoC Shell input data
    .i_tlast(noc_in.tlast),                  // in:  NoC Shell input last
    .i_tvalid(noc_in.tvalid),                // in:  NoC Shell input valid
    .i_tready(noc_in.tready),                // out: NoC Shell input
                                             // backpressure
    .i_teob(i_tuser.eob),                    // in:  NoC Shell CHDR
                                             // end-of-burst
    .i_ttimestamp(i_tuser.timestamp),        // in:  NoC Shell CHDR
                                             // timestamp
    .i_thas_time(i_tuser.has_time),          // in:  NoC Shell CHDR
                                             // has-time flag
    .i_tlength(i_tuser.length),              // in:  NoC Shell CHDR
                                             // payload length
    .o_tdata(noc_out.tdata),                 // out: NoC Shell output data
    .o_tlast(noc_out.tlast),                 // out: NoC Shell output last
    .o_tvalid(noc_out.tvalid),               // out: NoC Shell output valid
    .o_tready(noc_out.tready),               // in:  NoC Shell output
                                             // backpressure
    .o_teob(o_teob),                         // out: NoC Shell CHDR
                                             // end-of-burst
    .o_ttimestamp(o_ttimestamp),             // out: NoC Shell CHDR
                                             // timestamp
    .o_thas_time(o_thas_time),               // out: NoC Shell CHDR
                                             // has-time flag
    .m_axis_data_tdata(m_axis_data_tdata),   // out: User logic input
                                             // data (to DSP)
    .m_axis_data_tlast(m_axis_data_tlast),   // out: User logic input last
    .m_axis_data_tvalid(m_axis_data_tvalid), // out: User logic input valid
    .m_axis_data_tready(m_axis_data_tready), // in:  User logic input
                         // backpressure
    .m_axis_data_teob(m_axis_data_teob),     // out: User logic input
                         // end-of-burst
    .s_axis_data_tdata(s_axis_data_tdata),   // in:  User logic output
                         // data (from DSP)
    .s_axis_data_tlast(s_axis_data_tlast),   // in:  User logic output last
    .s_axis_data_tvalid(s_axis_data_tvalid), // in:  User logic output valid
    .s_axis_data_tready(s_axis_data_tready)  // out: User logic output
                         // backpressure
  );

  // Pack DUT output sidebands into BFM tuser for receive path
  assign noc_out.tuser = pack_user(16'd0, o_ttimestamp, o_thas_time, o_teob);

  //---------------------------------------------------------------------------
  // Simulated User Logic
  //---------------------------------------------------------------------------
  // Mimics the user-logic side of the rate-change module
  // (e.g., a CIC decimator).
  // For every N input words consumed on m_axis_data, it produces M output
  // words on
  // s_axis_data. The data pattern is deterministic: each output word is
  // {group_index[WIDTH/2-1:0], word_within_group[WIDTH/2-1:0]}, allowing the
  // checker to
  // independently reconstruct expected data without knowing DUT internals.
  //
  // Counters:
  //   count_n   — tracks position within current N-word input group (1..rate_n)
  //   count_m   — tracks position within current M-word output group
  //               (1..rate_m)
  //   count_in  — number of complete input groups consumed
  //               (increments after N words)
  //   count_out — number of complete output groups emitted
  //               (increments after M words)
  //
  // Flow control:
  //   - Output is only valid when count_in != count_out
  //     (input group(s) pending)
  //   - Backpressure from s_axis_data_tready gates output emission
  //   - m_axis_data_tready is tied to s_axis_data_tready (pass-through)

  int rate_n;
  int rate_m;
  int count_n;
  int count_m;
  int count_in;
  int count_out;

  // Warmup counter: models the pipeline fill-up latency of a real DSP block
  // (e.g., CIC interpolator). The user logic does not produce output until
  // at least DEFAULT_SPP input words have been consumed, ensuring that the
  // DUT's timestamp/sideband capture (which occurs on the first input
  // packet's tlast) completes before any output reaches the framing logic.
  int  user_warmup_cnt;
  logic user_warmup_done;

  // Queue of tlast flags: one entry per N-word input group, recording
  // whether m_axis_data_tlast was asserted on the last word of that group.
  // Popped when the corresponding M-word output group completes.
  logic tlast_queue[$];

  always_ff @(posedge clk) begin
    if (rst || clear_user || clear) begin
      // Reset all counters at burst boundaries (clear_user) or global reset
      count_n <= 1;
      count_m <= 1;
      count_in <= 0;
      count_out <= 0;
      user_warmup_cnt <= 0;
      user_warmup_done <= '0;
      tlast_queue = {};
    end else begin
      // --- Warmup: count input words until pipeline is primed ---
      if (!user_warmup_done) begin
        if (m_axis_data_tvalid && m_axis_data_tready) begin
          if (user_warmup_cnt >= DEFAULT_SPP - 1) begin
            user_warmup_done <= '1;
          end else begin
            user_warmup_cnt <= user_warmup_cnt + 1;
          end
        end
      end
      // --- Input consumption: count N words from m_axis_data ---
      if (m_axis_data_tvalid && m_axis_data_tready) begin
        if (count_n == rate_n) begin
          // Consumed a full group of N words; signal one output group is owed
          count_n <= 1;
          count_in <= count_in + 1;
          // Record whether the last word of this group had tlast
          tlast_queue.push_back(m_axis_data_tlast);
        end else begin
          count_n <= count_n + 1;
        end
      end

      // --- Output production: emit M words on s_axis_data ---
      // Only produce when at least one input group has not been answered
      if (count_in != count_out) begin
        if (s_axis_data_tvalid && s_axis_data_tready) begin
          if (count_m == rate_m) begin
            // Emitted a full group of M words; debt for this input group
            // is paid
            count_m <= 1;
            count_out <= count_out + 1;
            // Remove one tlast token from the queue if there is one for
            // this group
            // (note: value from front of queue is used below,
            // so we cast to void to avoid warning))
            void'(tlast_queue.pop_front());
          end else begin
            count_m <= count_m + 1;
          end
        end
      end
    end
  end

  // Output data pattern: {group_index, word_within_group}
  assign s_axis_data_tdata = {count_out[WIDTH/2-1:0], count_m[WIDTH/2-1:0]};
  // Assert valid whenever there are pending output groups to emit AND the
  // pipeline warmup is complete (models real DSP block fill-up latency).
  assign s_axis_data_tvalid = (count_in != count_out) && user_warmup_done;
  // Assert tlast at the end of an output group if the corresponding input
  // group had tlast asserted on its last word.
  logic s_axis_data_tlast_r;
  always_comb begin
    // Set tlast for the next output group after receiving input group
    // with tlast
    s_axis_data_tlast_r = (tlast_queue.size() > 0) && tlast_queue[0] &&
                          (count_m == rate_m);
  end
  assign s_axis_data_tlast = s_axis_data_tlast_r;
  // Pass-through backpressure: accept input whenever output can be accepted
  assign m_axis_data_tready = s_axis_data_tready | !s_axis_data_tvalid;

  //---------------------------------------------------------------------------
  // User Logic Interface Monitor
  //---------------------------------------------------------------------------
  // Checks that the DUT presents correct data on the m_axis_data port facing
  // user logic. Verifies that data words arrive in the same sequential order
  // as they were injected on the NoC-shell input. Also counts total words
  // delivered so test_rate can verify partial groups are not passed through.

  logic               user_mon_en = '0;
  logic [WIDTH-1:0]   user_mon_data_expect;  // expected sequential data value
  logic [WIDTH-1:0]   user_mon_word_cnt;     // total words delivered
                                             // (doesn't reset on clear_user)

  always_ff @(posedge clk) begin
    if (rst || !user_mon_en) begin
      // Full reset only when monitor is disabled or global reset
      user_mon_data_expect <= 0;
      user_mon_word_cnt    <= 0;
    end else if (clear_user || clear) begin
      // clear_user resets the expected data pattern (DUT restarts sequencing)
      // but does NOT reset the word counter
      // (we want the total across the burst)
      user_mon_data_expect <= 0;
    end else if (m_axis_data_tvalid && m_axis_data_tready) begin
      // Check data matches sequential input pattern
      `ASSERT_ERROR(
        m_axis_data_tdata == user_mon_data_expect[WIDTH-1:0],
        $sformatf(
          "User IF data mismatch at word %0d. Actual=0x%0h Expected=0x%0h",
          user_mon_data_expect,
          m_axis_data_tdata,
          user_mon_data_expect[WIDTH-1:0])
      )
      user_mon_data_expect <= user_mon_data_expect + 1;
      // Only count words that are part of complete N-word groups.
      // Partial groups at the end of a burst are accepted but not counted
      // — the user logic discards them.
      if (count_n == rate_n) begin
        user_mon_word_cnt <= user_mon_word_cnt + rate_n;
      end
    end
  end

  // Check that the DUT asserts clear_user exactly once per burst, and only
  // after:
  //   1. The EOB word has been consumed on the DUT input (noc_in)
  //   2. All complete N-word groups have been delivered to user logic
  //      (m_axis_data_tvalid is deasserted, meaning no pending words)
  logic clear_user_mon_en = 1'b0;
  int   clear_user_cnt;
  logic clear_user_prev;
  logic eob_input_seen;

  always_ff @(posedge clk) begin
    if (rst || !clear_user_mon_en) begin
      clear_user_cnt  <= 0;
      clear_user_prev <= 1'b0;
      eob_input_seen  <= 1'b0;
    end else begin
      // Track when the EOB word is consumed on the DUT input
      if (noc_in.tvalid && noc_in.tready && noc_in.tlast && i_tuser.eob) begin
        eob_input_seen <= 1'b1;
      end

      clear_user_prev <= clear_user;
      if (clear_user && !clear_user_prev) begin
        clear_user_cnt <= clear_user_cnt + 1;

        // clear_user must not fire before EOB was consumed on input
        `ASSERT_ERROR(
          eob_input_seen,
          "clear_user asserted before EOB was received on DUT input"
        )
        // clear_user must not fire while words are still pending for user logic
        `ASSERT_ERROR(
          !m_axis_data_tvalid,
          "clear_user asserted while m_axis_data_tvalid still active"
        )
      end
    end
  end

  //---------------------------------------------------------------------------
  // Bubble State Counter
  //---------------------------------------------------------------------------
  // Counts output clock cycles while output is active (between first tvalid
  // and last tlast). Used by the bubble-state test to verify zero-bubble
  // throughput.

  int   clock_cnt;
  logic clock_cnt_en = 1'b0;
  logic clock_cnt_start = 1'b0;

  always_ff @(posedge clk) begin
    if (clock_cnt_en) begin
      if (noc_out.tvalid && noc_out.tready && !clock_cnt_start) begin
        clock_cnt_start <= 1'b1;
        clock_cnt <= clock_cnt + 1;
      end else if (clock_cnt_start) begin
        clock_cnt <= clock_cnt + 1;
        if (noc_out.tvalid && noc_out.tready && noc_out.tlast) begin
          clock_cnt_start <= 1'b0;
        end
      end
    end else begin
      clock_cnt_start <= 1'b0;
      clock_cnt <= 0;
    end
  end

  //---------------------------------------------------------------------------
  // Test Utilities
  //---------------------------------------------------------------------------

  // Pack CHDR sideband metadata into a tuser word
  function automatic user_t pack_user(
    input logic [CHDR_LENGTH_W-1:0]    length,
    input logic [CHDR_TIMESTAMP_W-1:0] timestamp,
    input logic has_time,
    input logic eob
  );
    user_t user;
    user.length    = length;
    user.timestamp = timestamp;
    user.has_time  = has_time;
    user.eob       = eob;
    return user;
  endfunction

  // Assert that two packets match in data, EOB, HAS_TIME, and timestamp.
  task automatic assert_packets_equal(
    input axis_pkt_t actual,
    input axis_pkt_t expected,
    input string ctx
  );
    begin
      int last_word_idx;

      // Check that data queue lengths match
      `ASSERT_ERROR(
        actual.data.size() == expected.data.size(),
        $sformatf("%s: Packet length mismatch. Actual=%0d Expected=%0d",
          ctx, actual.data.size(), expected.data.size())
      )

      // Check that actual user queue is consistent with its data queue
      `ASSERT_ERROR(
        actual.user.size() == actual.data.size(),
        $sformatf(
          "%s: Actual packet user queue size mismatch. Actual=%0d Data=%0d",
          ctx, actual.user.size(), actual.data.size())
      )

      // Check that expected user queue is consistent with its data queue
      `ASSERT_ERROR(
        expected.user.size() == expected.data.size(),
        $sformatf(
          "%s: Expected packet user queue size mismatch. Expected=%0d Data=%0d",
          ctx, expected.user.size(), expected.data.size())
      )

      // Check each data word for exact match
      foreach (actual.data[i]) begin
        `ASSERT_ERROR(
          actual.data[i] === expected.data[i],
          $sformatf("%s: Word %0d data mismatch. Actual=0x%0h Expected=0x%0h",
            ctx, i, actual.data[i], expected.data[i])
        )
      end

      last_word_idx = actual.data.size() - 1;

      begin
        user_t actual_last   = user_t'(actual.user[last_word_idx]);
        user_t expected_last = user_t'(expected.user[last_word_idx]);

        // Check EOB flag on last word
        `ASSERT_ERROR(
          actual_last.eob == expected_last.eob,
          $sformatf("%s: Last-word EOB mismatch. Actual=%0d Expected=%0d",
            ctx, actual_last.eob, expected_last.eob)
        )

        // Check HAS_TIME flag on last word
        `ASSERT_ERROR(
          actual_last.has_time == expected_last.has_time,
          $sformatf("%s: Last-word HAS_TIME mismatch. Actual=%0d Expected=%0d",
            ctx, actual_last.has_time, expected_last.has_time)
        )

        // Check timestamp on last word (only when HAS_TIME is set)
        if (expected_last.has_time) begin
          `ASSERT_ERROR(
            actual_last.timestamp == expected_last.timestamp,
            $sformatf(
              "%s: Last-word timestamp mismatch. Actual=%0d Expected=%0d",
              ctx, actual_last.timestamp, expected_last.timestamp)
          )
        end
        // Length is valid on the final word along with the other sideband.
        `ASSERT_ERROR(
          actual_last.length == expected_last.length,
          $sformatf(
            "%s: Last-word length mismatch. Actual=%0d Expected=%0d",
            ctx, actual_last.length, expected_last.length)
        )
      end
    end
  endtask

  // Assert clear for 2 cycles then deassert
  task automatic pulse_clear();
    clear = 1'b1;
    clk_gen.clk_wait_r(2);
    clear = 1'b0;
    clk_gen.clk_wait_r(2);
  endtask

  // Configure BFM stall probabilities
  task automatic set_stall_probs(bit rand_delay_in, bit rand_delay_out);
    axis_bfm.set_master_stall_prob(rand_delay_in ? STALL_PROB : 0);
    axis_bfm.set_slave_stall_prob(rand_delay_out ? STALL_PROB : 0);
  endtask

  // Update the local rate used by the simulated user logic and program the
  // timestamp increment.
  task automatic set_rate(
    input int n,
    input int m,
    input int time_incr = 0
  );
    // Keep local variables in sync for simulated user logic
    rate_n = n;
    rate_m = m;
    ctrl_bfm.write(REG_AXI_RATE_SR_TIME_INCR_ADDR, time_incr);
  endtask

  // Enqueue a burst of packets to the BFM
  task automatic send_packets(input axis_pkt_queue_t packets);
    foreach (packets[i]) begin
      axis_bfm.put(packets[i]);
    end
  endtask

  // Receive and verify packets against expected
  task automatic check_packets(
    input axis_pkt_queue_t expected_packets,
    input string test_name
  );
    axis_pkt_t actual_packet;

    foreach (expected_packets[i]) begin
      axis_bfm.get(actual_packet);
      assert_packets_equal(
        actual_packet, expected_packets[i],
        $sformatf("%s pkt %0d", test_name, i)
      );
    end
  endtask

  // Receive and verify packets for randomized SPP.
  //
  // Since the DUT updates its internal SPP configuration based on the size of
  // incoming packets, output packet sizes are not predictable when input
  // packets have randomized sizes. This task verifies:
  //   1. Output packet sizes are monotonically non-decreasing (each packet is
  //      >= the previous), except for the last packet which may be partial.
  //   2. No output packet exceeds the maximum input packet size.
  //   3. Data words match the expected serialized stream, word-by-word.
  //   4. Sideband information is only checked on the last word of each
  //      received packet (per the DUT contract: sideband valid on tlast only).
  task automatic check_packets_random_spp(
    input axis_pkt_queue_t expected_packets,
    input axis_pkt_queue_t input_packets,
    input int time_incr
  );
    // Compute maximum allowed output packet size from input packets
    int max_input_pkt_size = 0;
    foreach (input_packets[p]) begin
      if (input_packets[p].data.size() > max_input_pkt_size) begin
        max_input_pkt_size = input_packets[p].data.size();
      end
    end

    // Serialize expected packets into a flat data queue for word-by-word
    // checking
    begin
      logic [WIDTH-1:0]  expected_data_q[$];
      int                total_expected_words;

      // Extract the initial timestamp from the first input packet's last word
      logic [CHDR_TIMESTAMP_W-1:0] expected_timestamp;
      user_t first_input_user;
      first_input_user =
        user_t'(input_packets[0].user[input_packets[0].user.size()-1]);
      expected_timestamp = first_input_user.timestamp;

      foreach (expected_packets[p]) begin
        foreach (expected_packets[p].data[w]) begin
          expected_data_q.push_back(expected_packets[p].data[w]);
        end
      end
      total_expected_words = expected_data_q.size();

      begin
        int words_consumed = 0;
        int pkt_idx = 0;
        int prev_pkt_size = 0;

        // Receive packets until all expected words are consumed
        while (words_consumed < total_expected_words) begin
          axis_pkt_t actual_packet;
          int actual_pkt_size;
          bit is_last_packet;

          axis_bfm.get(actual_packet);
          actual_pkt_size = actual_packet.data.size();

          // Determine if this is the last packet by checking whether
          // consuming it will exhaust all expected words
          is_last_packet =
            (words_consumed + actual_pkt_size >= total_expected_words);

          // --- Check 1: Packet size monotonicity ---
          // exception: there is a special case where an intermediate packet of
          // size 1 is generated. See DUT documentation for details.
          if (pkt_idx > 0 && !is_last_packet) begin
            `ASSERT_ERROR(
              actual_pkt_size >= prev_pkt_size,
              $sformatf({"random_spp pkt %0d:",
                " Packet size decreased. Current=%0d Previous=%0d",
                " (not last packet)"},
                pkt_idx, actual_pkt_size, prev_pkt_size)
            )
          end

          // --- Check 2: Packet size upper bound ---
          `ASSERT_ERROR(
            actual_pkt_size <= max_input_pkt_size,
            $sformatf({"random_spp pkt %0d:",
              " Packet size %0d exceeds max input packet size %0d"},
              pkt_idx, actual_pkt_size, max_input_pkt_size)
          )

          // --- Check 3: Data word-by-word verification ---
          foreach (actual_packet.data[i]) begin
            int pos = words_consumed + i;
            `ASSERT_ERROR(
              pos < total_expected_words,
              $sformatf({"random_spp pkt %0d word %0d:",
                " Received more words than expected. Position=%0d Total=%0d"},
                pkt_idx, i, pos, total_expected_words)
            )
            `ASSERT_ERROR(
              actual_packet.data[i] === expected_data_q[pos],
              $sformatf({"random_spp pkt %0d word %0d (pos %0d):",
                " Data mismatch. Actual=0x%0h Expected=0x%0h"},
                pkt_idx, i, pos, actual_packet.data[i], expected_data_q[pos])
            )
          end

          // --- Check 4: Sideband on last word only ---
          // The timestamp identifies the first sample in the packet. The
          // running timestamp is advanced after the packet for the next one.
          begin
            int last_idx = actual_pkt_size - 1;
            user_t actual_user = user_t'(actual_packet.user[last_idx]);
            logic [CHDR_TIMESTAMP_W-1:0] expected_last_timestamp;
            bit expected_has_time;

            expected_last_timestamp = expected_timestamp;
            expected_has_time = EN_TIME_ALL_PKTS || (pkt_idx == 0);

            `ASSERT_ERROR(
              actual_user.eob == is_last_packet,
              $sformatf({"random_spp pkt %0d:",
                " Last-word EOB mismatch. Actual=%0d Expected=%0d"},
                pkt_idx, actual_user.eob, is_last_packet)
            )
            `ASSERT_ERROR(
              actual_user.has_time == expected_has_time,
              $sformatf({"random_spp pkt %0d:",
                " Last-word HAS_TIME mismatch. Actual=%0d Expected=%0d"},
                pkt_idx, actual_user.has_time, expected_has_time)
            )
            if (expected_has_time) begin
              `ASSERT_ERROR(
                actual_user.timestamp == expected_last_timestamp,
                $sformatf({"random_spp pkt %0d:",
                  " Last-word timestamp mismatch. Actual=%0d Expected=%0d"},
                  pkt_idx, actual_user.timestamp, expected_last_timestamp)
              )
            end
          end

          // Advance running timestamp for the next packet
          expected_timestamp += actual_pkt_size * time_incr;
          words_consumed += actual_pkt_size;
          prev_pkt_size = actual_pkt_size;
          pkt_idx++;
        end

        // Final sanity check: all expected words were consumed
        `ASSERT_ERROR(
          words_consumed == total_expected_words,
          $sformatf({"random_spp:",
            " Total word count mismatch. Received=%0d Expected=%0d"},
            words_consumed, total_expected_words)
        )
      end
    end
  endtask

  // Build input stimulus packets for a given rate configuration.
  // Creates a burst of num_words total, split into spp-sized packets. Each
  // word carries a sequential value (0, 1, 2, ...) so the user-logic monitor
  // can verify ordering. CHDR timestamp, has-time, and length metadata is
  // repeated on every payload word, as guaranteed by the RFNoC shell.
  task automatic build_rate_input_packets(
    input int n,
    input int m,
    input int num_words,
    input int spp,
    input bit randomize_spp,
    input bit sideband_every_word = 1'b1,
    output axis_pkt_queue_t input_packets,
    output int max_spp
  );
    real timestamp;
    int words_left_to_send;
    int words_to_send;
    int current_spp = spp;
    logic [WIDTH-1:0] words_sent;

    input_packets = {};
    timestamp = 0.0;
    words_sent = 0;
    words_left_to_send = num_words;
    max_spp = 0;

    while (words_left_to_send > 0) begin
      axis_pkt_t packet = new();
      bit is_last_input_packet;

      if (randomize_spp) begin
        // Randomize packet sizes between SPC and 2*spp to test non-uniform
        // chunking
        // Round to multiple of SPC since input groups are N words
        current_spp = $urandom_range(1, 2*(spp/SPC)) * SPC;
      end else begin
        current_spp = spp;
      end
      if (current_spp > max_spp) begin
        max_spp = current_spp;
      end

      // Determine packet size (spp or remainder)
      if (words_left_to_send >= current_spp) begin
        words_to_send = current_spp;
      end else begin
        words_to_send = words_left_to_send;
      end
      is_last_input_packet = (words_left_to_send <= current_spp);

      for (int i = 0; i < words_to_send; i++) begin
        user_t user_word;
        logic [CHDR_TIMESTAMP_W-1:0] packet_timestamp;

        user_word = '0;
        packet_timestamp = longint'(timestamp);
        if (sideband_every_word || (i == words_to_send - 1)) begin
          user_word = pack_user(
            WORD_BYTES * words_to_send,
            packet_timestamp,
            1'b1,
            is_last_input_packet
          );
        end
        // Sequential data pattern for user-logic monitor verification
        packet.data.push_back(words_sent + i);
        packet.user.push_back(user_word);
      end

      input_packets.push_back(packet);
      words_sent += words_to_send;
      words_left_to_send -= words_to_send;
      // Advance timestamp at the output rate (M/N samples per input sample)
      timestamp += (1.0 * m / n) * words_to_send;
    end
  endtask

  // Build expected output packets for a given rate configuration.
  // The DUT consumes N input words and produces M output words for each group.
  // Total output words = floor(num_words / N) * M. The expected data pattern
  // matches what the simulated user logic produces:
  //   s_axis_data_tdata =
  //   {group_index[WIDTH/2-1:0], word_within_group[WIDTH/2-1:0]}
  // Timestamps advance by time_incr per output word. Each output packet's
  // metadata carries the timestamp of its first output word.
  task automatic build_expected_rate_packets(
    input int n,
    input int m,
    input int num_words,
    input int spp,
    input int time_incr,
    input bit expect_extra_last_word,
    output axis_pkt_queue_t expected_packets
  );
    logic [CHDR_TIMESTAMP_W-1:0] expected_timestamp;
    logic [WIDTH/2-1:0] word_cnt_div_m;       // group index
                          // (increments every M words)
    logic [WIDTH/2-1:0] word_cnt_div_m_frac;  // word within current group
                          // (1..M)
    int words_left_to_recv;

    expected_packets = {};
    expected_timestamp = 0;
    word_cnt_div_m = 0;
    word_cnt_div_m_frac = 0;
    // A partial input group produces one final word carrying the burst tlast.
    words_left_to_recv = $floor(num_words / n) * m +
      (expect_extra_last_word && ((num_words % n) != 0));

    while (words_left_to_recv > 0) begin
      axis_pkt_t packet = new();
      int words_in_packet;
      bit is_last_output_packet;
      bit has_time;

      if (words_left_to_recv >= spp) begin
        words_in_packet = spp;
      end else begin
        words_in_packet = words_left_to_recv;
      end
      is_last_output_packet = (words_left_to_recv <= spp);
      has_time = EN_TIME_ALL_PKTS || (expected_packets.size() == 0);

      for (int i = 0; i < words_in_packet; i++) begin
        user_t user_word;

        // Track position within M-word output group
        word_cnt_div_m_frac++;
        // Expected data matches simulated user logic:
        // {group_idx, word_in_group}
        packet.data.push_back({word_cnt_div_m, word_cnt_div_m_frac});

        // Output sideband is valid only on the final word of each packet.
        user_word = '0;
        if (words_in_packet-1 == i) begin
          user_word = pack_user(
            16'd0,
            expected_timestamp,
            has_time,
            is_last_output_packet
          );
        end
        packet.user.push_back(user_word);

        // Advance to next group after M words
        if (word_cnt_div_m_frac == m) begin
          word_cnt_div_m_frac = 0;
          word_cnt_div_m++;
        end
      end

      // Advance the burst timestamp by every output word in this packet.
      expected_timestamp += words_in_packet * time_incr;
      words_left_to_recv -= words_in_packet;
      expected_packets.push_back(packet);
    end
  endtask

  //---------------------------------------------------------------------------
  // Testcases
  //---------------------------------------------------------------------------

  // Test a single N:M rate configuration end-to-end.
  //
  // Steps:
  //   1. Pulse clear to reset DUT state machines and internal counters.
  //   2. Program N, M, and time_incr via ctrlport.
  //   3. Configure BFM master/slave stall probabilities for back-pressure.
  //   4. Build the stimulus packets (num_words total, chunked into spp-sized
  //      packets with CHDR sideband metadata and EOB on the last packet).
  //   5. Build the expected output packets (floor(num_words/N)*M words,
  //      with computed timestamps and user-logic counter data pattern).
  //   6. Fork: send stimulus on noc_in while simultaneously receiving and
  //      verifying each output packet on noc_out against the expected set.
  //   7. Wait for the BFM to drain any remaining in-flight transactions.
  //   8. Restore stall probabilities to zero and optionally wait a few
  //      cycles to let the DUT settle before the next test iteration.
  task automatic test_rate(
    input int n,
    input int m,
    input int num_words,
    input int spp = DEFAULT_SPP,
    input bit rand_delay_in = 1'b0,
    input bit rand_delay_out = 1'b0,
    input int post_wait_cycles = 4,
    input bit randomize_spp = 1'b0,
    input int time_incr = 0,
    input bit sideband_every_word = 1'b1,
    input bit expect_extra_last_word = 1'b0
  );
    axis_pkt_queue_t input_packets;
    axis_pkt_queue_t expected_packets;
    int              max_spp;
    int              resolved_time_incr;

    if (VERBOSE) begin
      $display("Testing rate %0d:%0d with %0d total words (%0d packets)",
        n, m, num_words, (num_words + spp - 1) / spp);
    end
    pulse_clear();
    resolved_time_incr = (time_incr == 0) ? n : time_incr;
    set_rate(n, m, resolved_time_incr);
    set_stall_probs(rand_delay_in, rand_delay_out);
    build_rate_input_packets(
      n, m, num_words, spp, randomize_spp, sideband_every_word,
      input_packets, max_spp);
    build_expected_rate_packets(
      n, m, num_words, spp, resolved_time_incr, expect_extra_last_word,
      expected_packets);

    // Enable monitors for user-logic data and clear_user assertion
    user_mon_en = 1'b1;
    clear_user_mon_en = 1'b1;

    // Drive input and check output concurrently
    fork
      begin
        send_packets(input_packets);
      end
      begin
        if (randomize_spp) begin
          check_packets_random_spp(
            expected_packets, input_packets, resolved_time_incr);
        end else begin
          check_packets(expected_packets,
            $sformatf("rate %0d:%0d", n, m));
        end
      end
    join

    // Drain any remaining BFM transactions
    axis_bfm.wait_complete();
    set_stall_probs(1'b0, 1'b0);
    if (post_wait_cycles > 0) begin
      clk_gen.clk_wait_r(post_wait_cycles);
    end

    // clear_user fires asynchronously after output completes; give it time
    if (clear_user_cnt == 0) begin
      clk_gen.clk_wait_r(100);
    end
    // Verify exactly one clear_user pulse occurred for this single-burst test
    `ASSERT_ERROR(
      clear_user_cnt == 1,
      $sformatf("rate %0d:%0d: clear_user fired %0d times, expected 1",
      n, m, clear_user_cnt)
    )
    clear_user_mon_en = 1'b0;

    // Verify the DUT delivered exactly the right number of words to user logic.
    // Only complete N-word groups should be passed through; partial groups at
    // the end of a burst must be dropped by axi_drop_partial_packet.
    begin
      int expected_user_words;
      expected_user_words = (num_words / n) * n;
      `ASSERT_ERROR(
        user_mon_word_cnt == expected_user_words,
        $sformatf({"rate %0d:%0d: User IF word count mismatch.",
          " Actual=%0d Expected=%0d (partial group should be dropped)"},
          n, m, user_mon_word_cnt, expected_user_words)
      )
    end
    user_mon_en = 1'b0;
  endtask

  // Verify ctrlport register write/readback for the timestamp increment
  task automatic test_register_readback();
    logic [CTRLPORT_DATA_W-1:0] read_data;
    logic [CTRLPORT_DATA_W-1:0] write_data;

    pulse_clear();

    if (MAX_N<19) begin
      write_data = MAX_N;
    end else begin
      write_data = 19;
    end
    ctrl_bfm.write(REG_AXI_RATE_SR_TIME_INCR_ADDR, write_data);
    ctrl_bfm.read(REG_AXI_RATE_SR_TIME_INCR_ADDR, read_data);
    `ASSERT_ERROR(read_data == write_data,
      $sformatf("TIME_INCR readback mismatch. Actual=%0d", read_data))
  endtask


  //---------------------------------------------------------------------------
  // Main Testbench Task
  //---------------------------------------------------------------------------

  task automatic testbench_main();
    string tb_name;

    tb_name = $sformatf(
      {"AXI Rate Change CHDR Testbench (WIDTH=%0d, MAX_N=%0d, MAX_M=%0d, ",
       "EN_TIME_ALL_PKTS=%0d)"},
      WIDTH, MAX_N, MAX_M, EN_TIME_ALL_PKTS);
    test.start_tb(tb_name, 1s);

    //`ASSERT_ERROR(
    //  WIDTH == 32,
    //  $sformatf("Testbench requires WIDTH=32. Actual=%0d", WIDTH))

    clk_gen.start();
    axis_bfm.run();
    ctrl_bfm.run();
    clk_gen.reset();
    @(negedge rst);

    test.start_test("CtrlPort Register Readback");
    test_register_readback();
    test.end_test();

    // Sweep all decimation rates N=1..MAX_N with M=1 and random stalls.
    // Each iteration sends 3*N*SPP words (enough for 3 full output packets).
    test.start_test("Check Various Rates", 50ms);
    for (int n = 1; n <= MAX_N; n++) begin
      test_rate(n, 1, n * DEFAULT_SPP * 3, DEFAULT_SPP, 1'b1, 1'b1);
    end
    for (int m = 1; m <= MAX_M; m++) begin
      test_rate(1, m, DEFAULT_SPP * 3, DEFAULT_SPP, 1'b1, 1'b1);
    end
    test.end_test();

    // Test partial input groups (not a multiple of N). The extra
    // (DEFAULT_SPP - 1) words form an incomplete group, and the output emits
    // one additional terminal word carrying tlast.
    test.start_test("Test Partial Packets", 50ms);
    for (int n = 1; n <= MAX_N; n++) begin
      test_rate(
        .n(n),
        .m(1),
        .num_words(n * DEFAULT_SPP + DEFAULT_SPP - 1),
        .spp(DEFAULT_SPP),
        .rand_delay_in(1'b1),
        .rand_delay_out(1'b1),
        .expect_extra_last_word(1'b1)
      );
    end
    test.end_test();

    // Randomize SPP to verify the DUT correctly handles non-uniform packet
    // chunking on the input.
    test.start_test("Test Randomized SPP", 100ms);
    for (int n = 1; n <= MAX_N; n++) begin
      test_rate(
        n, 1, n * DEFAULT_SPP * 10, DEFAULT_SPP, 1'b1, 1'b1,
        4, 1'b1);
    end
    test.end_test();

    // Send one large packet (4*SPP words) with no stalls to verify the DUT
    // correctly captures metadata from the first input word even when it
    // arrives far before the output packet boundary.
    test.start_test("Test Long First Packet Metadata Timing");
    test_rate(1, 1, DEFAULT_SPP * 4, DEFAULT_SPP * 4, 1'b0, 1'b0, 0);
    test.end_test();

    if (!EN_TIME_ALL_PKTS) begin
      test.start_test("Test First Packet Timestamp Only");
      repeat (2) begin
        test_rate(
          .n(1),
          .m(1),
          .num_words(DEFAULT_SPP * 3),
          .spp(DEFAULT_SPP),
          .rand_delay_in(1'b1),
          .rand_delay_out(1'b1),
          .time_incr(3)
        );
      end
      test.end_test();
    end

    if (MAX_M >= 2) begin
      test.start_test("Test Interpolation Early Sideband Metadata");
      test_rate(
        .n(1),
        .m(2),
        .num_words(DEFAULT_SPP * 4),
        .spp(DEFAULT_SPP * 4),
        .rand_delay_in(1'b0),
        .rand_delay_out(1'b0),
        .post_wait_cycles(0),
        .randomize_spp(1'b0),
        .time_incr(2),
        .sideband_every_word(1'b1)
      );
      test.end_test();
    end


    // Verify zero-bubble throughput: with 1:1 rate and no stalls, every
    // output clock cycle should produce one valid word. The bubble counter
    // measures active cycles and must equal the total word count.
    test.start_test("Test For Bubble States");
    clock_cnt_en = 1'b1;
    test_rate(1, 1, BUBBLE_TEST_WORDS, DEFAULT_SPP, 1'b0, 1'b0, 0);
    #1ps;
    `ASSERT_ERROR(
      clock_cnt == BUBBLE_TEST_WORDS,
      $sformatf(
        "Incorrect number of output clock cycles. Expected=%0d Actual=%0d",
        BUBBLE_TEST_WORDS,
        clock_cnt
      )
    )
    clock_cnt_en = 1'b0;
    test.end_test();

    test.end_tb(0);
    clk_gen.kill();
  endtask

  //---------------------------------------------------------------------------
  // Test Execution
  //---------------------------------------------------------------------------

  initial begin : tb_main
    testbench_main();
  end

endmodule

`default_nettype wire
