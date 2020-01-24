`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 7

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_axis_lib.svh"

module axi_packet_gate_tb();
  `TEST_BENCH_INIT("axi_packet_gate_tb", `NUM_TEST_CASES, `NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e9/166.67e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(reset, 0, 100);

  localparam MAX_PKT_SIZE = 64;

  // +1 for terror
  axis_master #(.DWIDTH(32+1)) m_axis (.clk(clk));
  axis_slave #(.DWIDTH(32)) s_axis (.clk(clk));

  axi_packet_gate #(
    .WIDTH(32),
    .SIZE($clog2(MAX_PKT_SIZE)),
    .USE_AS_BUFF(1)
  ) dut (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i_tdata(m_axis.axis.tdata[31:0]),
    .i_tvalid(m_axis.axis.tvalid),
    .i_tlast(m_axis.axis.tlast),
    .i_terror(m_axis.axis.tdata[32]), // Use MSB as terror
    .i_tready(m_axis.axis.tready),
    .o_tdata(s_axis.axis.tdata),
    .o_tvalid(s_axis.axis.tvalid),
    .o_tlast(s_axis.axis.tlast),
    .o_tready(s_axis.axis.tready)
  );

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

  initial begin : tb_main
    string s;
    logic error, last;
    int cnt, check;

    /********************************************************
    ** Test 1 -- Reset
    ********************************************************/
    `TEST_CASE_START("Wait for Reset");
    m_axis.reset();
    s_axis.reset();
    while (reset) @(posedge clk);
    `TEST_CASE_DONE(~reset);

    /********************************************************
    ** Test 2 -- Fill / Empty FIFO
    ********************************************************/
    `TEST_CASE_START("Fill and empty FIFO");
    error = 0;
    last = 0;
    cnt = 0;
    $display("Write %0d words to FIFO", MAX_PKT_SIZE);
    for (int i = 0; i < MAX_PKT_SIZE; i++) begin
      $sformat(s, "FIFO prematurely full at %0d (tready not asserted!)", i);
      `ASSERT_FATAL(m_axis.axis.tready, s);
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE-1);
      cnt++;
    end
    $display("Done writing to FIFO");
    if (m_axis.axis.tready) begin
      $display("FIFO not full, fill remaining space");
      // Continue filling until FIFO is full
      while (m_axis.axis.tready) begin
        m_axis.push_word({error,cnt}, 0);
        cnt++;
      end
      $display("Done filling remaining space");
      $display("Expected FIFO size: %0d, Actual: %0d",MAX_PKT_SIZE,cnt);
    end
    // On the first packet, output is held off until a full packet is received
    repeat(MAX_PKT_SIZE) @(posedge clk);
    $display("Empty FIFO and check output");
    for (int i = 0; i < cnt; i++) begin
      $sformat(s, "FIFO prematurely empty at %0d (tvalid not asserted!)", i);
      `ASSERT_FATAL(s_axis.axis.tvalid, s);
      s_axis.pull_word(check, last);
      $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
      `ASSERT_FATAL(check == i, s);
      if (i == cnt-1) begin
        `ASSERT_FATAL(last, "tlast not asserted on final word!");
      end else begin
        `ASSERT_FATAL(~last, "tlast asserted prematurely!");
      end
    end
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 3 -- Check gating
    ********************************************************/
    `TEST_CASE_START("Check gating");
    error = 0;
    last = 0;
    cnt = 0;

    $display("Write %0d words to FIFO and check out valid", MAX_PKT_SIZE);
    for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
      m_axis.push_word({error,cnt}, 0);
      cnt++;
    end
    // On the first packet, output is held off until a full packet is received
    repeat(10) @(posedge clk);
    `ASSERT_FATAL(~s_axis.axis.tvalid, "Saw output before a full packet input");
    for (int i = MAX_PKT_SIZE/2; i < MAX_PKT_SIZE; i++) begin
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE-1);
      cnt++;
    end
    repeat(10) @(posedge clk);
    `ASSERT_FATAL(s_axis.axis.tvalid, "Did not see output even after full packet input");
    $display("Empty FIFO and check output");
    for (int i = 0; i < cnt; i++) begin
      $sformat(s, "FIFO prematurely empty at %0d (tvalid not asserted!)", i);
      `ASSERT_FATAL(s_axis.axis.tvalid, s);
      s_axis.pull_word(check, last);
      $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
      `ASSERT_FATAL(check == i, s);
      if (i == cnt-1) begin
        `ASSERT_FATAL(last, "tlast not asserted on final word!");
      end else begin
        `ASSERT_FATAL(~last, "tlast asserted prematurely!");
      end
    end
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 4 -- Ensure no bleed
    ********************************************************/
    `TEST_CASE_START("Ensure no bleed");
    error = 0;
    last = 0;
    cnt = 0;

    $display("Write %0d words to FIFO (full packet)", MAX_PKT_SIZE/2);
    for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
      m_axis.push_word({error,cnt}, i == (MAX_PKT_SIZE/2)-1);
      cnt++;
    end
    $display("Write %0d words to FIFO (partial packet)", MAX_PKT_SIZE/4);
    for (int i = 0; i < MAX_PKT_SIZE/4; i++) begin
      m_axis.push_word({error,cnt}, 0);
      cnt++;
    end
    // On the first packet, output is held off until a full packet is received
    repeat(10) @(posedge clk);
    for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
      $sformat(s, "FIFO prematurely empty at %0d (tvalid not asserted!)", i);
      `ASSERT_FATAL(s_axis.axis.tvalid, s);
      s_axis.pull_word(check, last);
      $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
      `ASSERT_FATAL(check == i, s);
      if (i == (MAX_PKT_SIZE/2)-1) begin
        `ASSERT_FATAL(last, "tlast not asserted on final word!");
      end else begin
        `ASSERT_FATAL(~last, "tlast asserted prematurely!");
      end
    end
    repeat(10) @(posedge clk);
    `ASSERT_FATAL(~s_axis.axis.tvalid, "Partial packet bled through with full packet");
    for (int i = MAX_PKT_SIZE/4; i < MAX_PKT_SIZE/2; i++) begin
      m_axis.push_word({error,cnt}, i == (MAX_PKT_SIZE/2)-1);
      cnt++;
    end
    repeat(10) @(posedge clk);
    `ASSERT_FATAL(s_axis.axis.tvalid, "Did not see output even after full packet input");
    $display("Empty FIFO and check output");
    for (int i = MAX_PKT_SIZE/2; i < cnt; i++) begin
      $sformat(s, "FIFO prematurely empty at %0d (tvalid not asserted!)", i);
      `ASSERT_FATAL(s_axis.axis.tvalid, s);
      s_axis.pull_word(check, last);
      $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
      `ASSERT_FATAL(check == i, s);
      if (i == cnt-1) begin
        `ASSERT_FATAL(last, "tlast not asserted on final word!");
      end else begin
        `ASSERT_FATAL(~last, "tlast asserted prematurely!");
      end
    end
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 5 -- Back to back small packets
    ********************************************************/
    `TEST_CASE_START("Back to back small packets");
    error = 0;
    last = 0;
    cnt = 0;
    for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
      $sformat(s, "FIFO prematurely full at %0d (tready not asserted!)", i);
      `ASSERT_FATAL(m_axis.axis.tready, s);
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE/2-1);
      cnt++;
    end
    for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
      $sformat(s, "FIFO prematurely full at %0d (tready not asserted!)", i);
      `ASSERT_FATAL(m_axis.axis.tready, s);
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE/2-1);
      cnt++;
    end
    fork
    begin
      for (int k = 0; k < 8; k++) begin
        for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
          m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE/2-1);
          cnt++;
        end
      end
    end
    begin
      @(posedge clk);
      @(posedge clk);
      @(posedge clk);
      for (int k = 0; k < 10; k++) begin
        for (int i = 0; i < MAX_PKT_SIZE/2; i++) begin
          `ASSERT_FATAL(s_axis.axis.tvalid, "tvalid not asserted!");
          s_axis.pull_word(check, last);
          $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
          `ASSERT_FATAL(check == (i+k*MAX_PKT_SIZE/2), s);
          if (i == MAX_PKT_SIZE/2-1) begin
            `ASSERT_FATAL(last, "tlast not asserted on final word!");
          end else begin
            `ASSERT_FATAL(~last, "tlast asserted prematurely!");
          end
        end
      end
    end
    join
    `TEST_CASE_DONE(1);

    #2000; // Delay to make the tests visually distinct in waveform viewer

    /********************************************************
    ** Test 6 -- Drop error packet
    ** - Send packet, drop a packet, send another packet
    ********************************************************/
    `TEST_CASE_START("Drop error packet");
    cnt = 0;
    $display("Write packet with %0d words to FIFO", MAX_PKT_SIZE/4);
    error = 0;
    for (int i = 0; i < MAX_PKT_SIZE/4; i++) begin
      $sformat(s, "FIFO prematurely full at %0d (tready not asserted!)", i);
      `ASSERT_FATAL(m_axis.axis.tready, s);
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE/4-1);
      cnt++;
    end
    $display("Write error packet with %0d words to FIFO", MAX_PKT_SIZE/4);
    error = 1;
    for (int i = 0; i < MAX_PKT_SIZE/4; i++) begin
      $sformat(s, "FIFO prematurely full at %0d (tready not asserted!)", i);
      `ASSERT_FATAL(m_axis.axis.tready, s);
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE/4-1);
      cnt++;
    end
    $display("Write packet with %0d words to FIFO", MAX_PKT_SIZE/4);
    error = 0;
    for (int i = 0; i < MAX_PKT_SIZE/4; i++) begin
      $sformat(s, "FIFO prematurely full at %0d (tready not asserted!)", i);
      `ASSERT_FATAL(m_axis.axis.tready, s);
      m_axis.push_word({error,cnt}, i == MAX_PKT_SIZE/4-1);
      cnt++;
    end
    for (int i = 0; i < MAX_PKT_SIZE/4; i++) begin
      $sformat(s, "FIFO prematurely empty at %0d (tvalid not asserted!)", i);
      `ASSERT_FATAL(s_axis.axis.tvalid, s);
      s_axis.pull_word(check, last);
      $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
      `ASSERT_FATAL(check == i, s);
      if (i == MAX_PKT_SIZE/4-1) begin
        `ASSERT_FATAL(last, "tlast not asserted on final word!");
      end else begin
        `ASSERT_FATAL(~last, "tlast asserted prematurely!");
      end
    end
    for (int i = 0; i < MAX_PKT_SIZE/4; i++) begin
      $sformat(s, "FIFO prematurely empty at %0d (tvalid not asserted!)", i);
      `ASSERT_FATAL(s_axis.axis.tvalid, s);
      s_axis.pull_word(check, last);
      $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", i, check);
      `ASSERT_FATAL(check == (i+MAX_PKT_SIZE/2), s);
      if (i == MAX_PKT_SIZE/4-1) begin
        `ASSERT_FATAL(last, "tlast not asserted on final word!");
      end else begin
        `ASSERT_FATAL(~last, "tlast asserted prematurely!");
      end
    end
    `TEST_CASE_DONE(1);

    #2000;

    /********************************************************
    ** Test 7 -- Random read / writes
    ********************************************************/
    `TEST_CASE_START("Random read / writes");
    error = 0;
    last = 0;
    cnt = 0;
    fork
    begin
      for (int k = 1; k <= 5000*MAX_PKT_SIZE; k++) begin
        m_axis.push_word({error,k}, (k % 16) == 0);
        random_wait(0,16);
      end
    end
    begin
      for (int k = 1; k <= 5000*MAX_PKT_SIZE; k++) begin
        random_wait(0,16);
        s_axis.pull_word(check, last);
        $sformat(s, "FIFO output incorrect! Expected: %0d, Actual: %0d", k, check);
        `ASSERT_FATAL(check == k, s);
        if ((k % 16) == 0) begin
          `ASSERT_FATAL(last, "tlast not asserted!");
        end else begin
          `ASSERT_FATAL(~last, "tlast asserted prematurely!");
        end
      end
    end
    join
    `TEST_CASE_DONE(1);

    `TEST_BENCH_DONE;

  end
endmodule
