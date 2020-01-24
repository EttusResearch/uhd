`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 4

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_cvita_lib.svh"
`include "sim_axis_lib.svh"
`include "sim_set_rb_lib.svh"

module axi_rate_change_tb();
  `TEST_BENCH_INIT("axi_rate_change_tb",`NUM_TEST_CASES,`NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e9/166.67e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(reset, 0, 100);

  localparam SR_N_ADDR      = 0;
  localparam SR_M_ADDR      = 1;
  localparam SR_CONFIG_ADDR = 2;
  localparam MAX_N = 16;
  localparam MAX_M = 16;

  logic [15:0] src_sid = 16'h0000;
  logic [15:0] dst_sid = 16'h0010;
  settings_bus_master sb (.clk(clk));
  axis_master #(.DWIDTH(32+128)) m_axis (.clk(clk));
  axis_slave #(.DWIDTH(32+128)) s_axis (.clk(clk));
  logic clear, clear_user;
  logic [31:0] m_axis_data_tdata, s_axis_data_tdata;
  logic m_axis_data_tlast, m_axis_data_tvalid, m_axis_data_tready;
  logic s_axis_data_tlast, s_axis_data_tvalid, s_axis_data_tready;
  logic warning_long_throttle, error_extra_outputs, error_drop_pkt_lockup;
  axi_rate_change #(
    .WIDTH(32),
    .MAX_N(MAX_N),
    .MAX_M(MAX_M),
    .SR_N_ADDR(SR_N_ADDR),
    .SR_M_ADDR(SR_M_ADDR),
    .SR_CONFIG_ADDR(SR_CONFIG_ADDR))
  axi_rate_change (
    .clk(clk), .reset(reset), .clear(clear), .clear_user(clear_user),
    .src_sid(src_sid), .dst_sid(dst_sid),
    .set_stb(sb.settings_bus.set_stb), .set_addr(sb.settings_bus.set_addr), .set_data(sb.settings_bus.set_data),
    .i_tdata(m_axis.axis.tdata[31:0]), .i_tlast(m_axis.axis.tlast), .i_tvalid(m_axis.axis.tvalid), .i_tready(m_axis.axis.tready), .i_tuser(m_axis.axis.tdata[159:32]),
    .o_tdata(s_axis.axis.tdata[31:0]), .o_tlast(s_axis.axis.tlast), .o_tvalid(s_axis.axis.tvalid), .o_tready(s_axis.axis.tready), .o_tuser(s_axis.axis.tdata[159:32]),
    .m_axis_data_tdata(m_axis_data_tdata), .m_axis_data_tlast(m_axis_data_tlast),
    .m_axis_data_tvalid(m_axis_data_tvalid), .m_axis_data_tready(m_axis_data_tready),
    .s_axis_data_tdata(s_axis_data_tdata), .s_axis_data_tlast(s_axis_data_tlast),
    .s_axis_data_tvalid(s_axis_data_tvalid), .s_axis_data_tready(s_axis_data_tready),
    .warning_long_throttle(warning_long_throttle),
    .error_extra_outputs(error_extra_outputs),
    .error_drop_pkt_lockup(error_drop_pkt_lockup));

  // Simulate user logic that can handle various decimation / interpolation rates
  // - Generates a word count sequence that is checked in the test_rate() task.
  // - Introduces a single clock cycle delay which is useful for testing that
  //   the DUT is not reliant on user logic having a large built in delay.
  integer rate_n, rate_m, count_n, count_m, count_in, count_out;
  always @(posedge clk) begin
    if (reset | clear_user | clear) begin
      s_axis_data_tlast  <= 1'b0;
      count_n            <= 1;
      count_m            <= 1;
      count_in           <= 0;
      count_out          <= 0;
    end else begin
      // Rate change = N/M
      if (m_axis_data_tvalid & m_axis_data_tready) begin
        if (count_n == rate_n) begin
          count_n        <= 1;
          count_in       <= count_in + 1;
        end else begin
          count_n        <= count_n + 1;
        end
      end
      if (count_in != count_out) begin
        if (s_axis_data_tvalid & s_axis_data_tready) begin
          if (count_m == rate_m) begin
            count_m          <= 1;
            count_out        <= count_out + 1;
          end else begin
            count_m          <= count_m + 1;
          end
        end
      end
    end
  end

  assign s_axis_data_tdata  = {count_out[15:0], count_m[15:0]};
  assign s_axis_data_tvalid = (count_in != count_out);
  assign m_axis_data_tready = s_axis_data_tready;

  // Used with test 3 to count output clock cycles
  int clock_cnt;
  logic clock_cnt_en = 1'b0;
  logic clock_cnt_start = 1'b0;
  always @(posedge clk) begin
    if (clock_cnt_en == 1'b1) begin
      // Wait until output data starts
      if (s_axis.axis.tvalid & ~clock_cnt_start) begin
        clock_cnt_start <= 1'b1;
        clock_cnt       <= clock_cnt + 1;
      end else if (clock_cnt_start) begin
        clock_cnt       <= clock_cnt + 1;
      end
    end else begin
      clock_cnt_start <= 1'b0;
      clock_cnt       <= 0;
    end
  end

  /********************************************************
  ** Verification
  ********************************************************/
  task random_wait(int unsigned min_cycles, int unsigned max_cycles);
    begin
      int unsigned num_cycles;
      do begin
        num_cycles = $random() & (2**($clog2(max_cycles))-1);
      end while ((num_cycles < min_cycles) || (num_cycles > max_cycles));

      if (num_cycles != 0) begin
        for (int unsigned i = 0; i < num_cycles; i++) begin
          @(posedge clk);
        end
        @(negedge clk); // Realign with negedge
      end
    end
  endtask

  task test_rate(int n, int m, int num_words, int spp = 16, bit rand_delay_in = 0, bit rand_delay_out = 0);
    begin
      clear = 1'b1;
      @(posedge clk);
      @(posedge clk);
      clear = 1'b0;
      @(posedge clk);
      @(posedge clk);
      rate_n = n;
      rate_m = m;
      sb.write(SR_N_ADDR, n);
      sb.write(SR_M_ADDR, m);
      @(posedge clk);
      fork
      begin
        cvita_hdr_t send_header;
        int words_left_to_send, words_to_send;
        logic [31:0] words_sent;
        real timestamp;
        timestamp = 0.0;
        words_sent = 0;
        words_left_to_send = num_words;

        while (words_left_to_send > 0) begin
          // Setup header
          send_header = '{default:0};
          send_header.has_time = 1;
          send_header.timestamp = longint'(timestamp);
          send_header.eob = (words_left_to_send <= spp);
          if (words_left_to_send >= spp) begin
            send_header.length = 16+4*spp;
            words_to_send = spp;
            words_left_to_send -= spp;
          end else begin
            send_header.length = 16+4*words_left_to_send;
            words_to_send = words_left_to_send;
            words_left_to_send = 0;
          end
          // Send packet
          for (int i = 0; i < words_to_send; i++) begin
            if (rand_delay_in) random_wait(0,2*spp);
            m_axis.push_word({send_header,words_sent + i},i == words_to_send-1);
          end
          words_sent += words_to_send;
          // Update seq num, timestamp
          send_header.seqnum++;
          timestamp += (1.0*m/n)*words_to_send;
        end
      end
      begin
        string s;
        cvita_hdr_t recv_header;
        logic last, expected_eob;
        real timestamp;
        logic [63:0] expected_timestamp;
        logic [31:0] word;
        logic [15:0] word_cnt_div_m, word_cnt_div_m_frac;
        logic [15:0] word_cnt_div_spp_frac;
        int words_left_to_recv, words_recvd;
        timestamp = 0.0;
        expected_timestamp = 0; // Timestamp starts at 0
        word_cnt_div_spp_frac = 0;
        word_cnt_div_m = 0;
        word_cnt_div_m_frac = 0;
        words_recvd = 0;
        words_left_to_recv = $floor(num_words/n)*m; // Order matters!

        while (words_left_to_recv > 0) begin
          s_axis.pull_word({recv_header,word},last);
          word_cnt_div_spp_frac++;
          word_cnt_div_m_frac++;
          words_recvd++;
          words_left_to_recv--;
          timestamp += 1.0*n;
          // Check packet length
          if ((word_cnt_div_spp_frac == spp) || (words_left_to_recv == 0)) begin
            `ASSERT_FATAL(last == 1, "Incorrect packet length! Last not asserted!");
            word_cnt_div_spp_frac = 0;
          end else begin
            $sformat(s, "Incorrect packet length! Expected: %0d, Actual %0d", spp, words_recvd);
            `ASSERT_FATAL(last == 0, s);
          end
          // Check for EOB
          if (last) begin
            words_recvd = 0;
            expected_eob = (words_left_to_recv == 0);
            $sformat(s, "Incorrect EOB state! Expected: %0d, Actual %0d", expected_eob, recv_header.eob);
            `ASSERT_FATAL(recv_header.eob == expected_eob, s);
          end
          // Check timestamp
          if (last) begin
            $sformat(s, "Incorrect timestamp! Expected: %0d, Actual %0d", expected_timestamp, recv_header.timestamp);
            `ASSERT_FATAL(recv_header.timestamp == expected_timestamp, s);
            expected_timestamp = longint'(timestamp);
          end
          // Check word
          $sformat(s, "Incorrect packet data! Expected: 0x%08h, Actual: 0x%08h", {word_cnt_div_m, word_cnt_div_m_frac}, word);
          `ASSERT_FATAL(((word[31:16] == word_cnt_div_m) && (word[15:0] == word_cnt_div_m_frac)), s);
          // Track number of received words, do at end of loop
          if (word_cnt_div_m_frac == m) begin
            word_cnt_div_m_frac = 0;
            word_cnt_div_m++;
          end
        end
      end
      join
    end
  endtask

  initial begin : tb_main
    string s;
    cvita_hdr_t tmp_header;
    logic [63:0] word;
    logic last;
    integer spp, number_words;
    spp = 16;

    /********************************************************
    ** Test 1 -- Reset
    ********************************************************/
    `TEST_CASE_START("Wait for Reset");
    sb.reset();
    m_axis.reset();
    s_axis.reset();
    while (reset) @(posedge clk);
    `TEST_CASE_DONE(~reset);

    /********************************************************
    ** Test 2 -- Test various rates
    ** - Try many decimation / interpolation rates (including 
    **   fractional rates) and use randomized delays.
    ********************************************************/
    `TEST_CASE_START("Check various rates");
    for (int _n = 1; _n <= MAX_N; _n++) begin
      for (int _m = 1; _m <= MAX_M; _m++) begin
        $display("Testing rate %0d:%0d", _n, _m);
        test_rate(_n, _m, _n*spp*3, spp, 1, 1);
      end
    end
    `TEST_CASE_DONE(1);

    #2000; // Delay to make the tests visually distinct in waveform viewer

    /********************************************************
    ** Test 3 -- Test partial packets
    ** - Send packets with extra data that is less than SPP.
    **   Module should output the maximum number of samples
    **   possible while dropping the "partial" sample.
    ********************************************************/
    `TEST_CASE_START("Test partial packets");
    for (int _n = 1; _n <= MAX_N; _n++) begin
      for (int _m = 1; _m <= MAX_M; _m++) begin
        $display("Testing rate %0d:%0d", _n, _m);
        test_rate(_n, _m, _n*spp + spp-1, spp, 1, 1);
      end
    end
    `TEST_CASE_DONE(1);

    #2000;

    /********************************************************
    ** Test 3 -- Test for bubble states
    ** - Send many packets at full rate and make sure number
    **   of input clock cycles == output clock cycles. If
    **   they do not match, then the module likely has
    **   bubble states that would prevent it from running
    **   continuously at full rate.
    ********************************************************/
    `TEST_CASE_START("Test for bubble states");
    clock_cnt_en = 1'b1;
    number_words = 100000;
    test_rate(1, 1, number_words, spp, 0, 0);
    $sformat(s, "Incorrect number of clock cycles -- Possible bubble states detected! Expected: %0d, Actual: %0d", number_words, clock_cnt);
    `ASSERT_FATAL(clock_cnt == number_words, s);
    clock_cnt_en = 1'b0;
    `TEST_CASE_DONE(1);

    `TEST_BENCH_DONE;

  end

  // The warning, error signals should never assert.
  initial begin
    while (reset) @(posedge clk);
    forever begin
      @(posedge clk);
      `ASSERT_FATAL(~warning_long_throttle,    "Throttle state deadlock!");
      `ASSERT_FATAL(~error_extra_outputs,      "Extra outputs detected!");
      `ASSERT_FATAL(~error_drop_pkt_lockup,    "Drop packet deadlock!");
    end
  end

endmodule
