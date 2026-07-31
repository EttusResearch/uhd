//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: clocking_watchdog_tb
//
// Description:
//   Self-checking testbench for clocking_watchdog.
//
// Requirements verified:
//   1) clock_fault is asserted when clock_locked is deasserted.
//   2) clock_fault is asserted when watched_clk is strictly above MAX_CLOCK_RATE.
//   3) clock_fault is deasserted when watched_clk is at or below MAX_CLOCK_RATE.
//
// Clock model:
//   Both monitor_clk and watched_clk are driven by sim_clock_gen instances.
//   sim_clock_gen rounds all periods to 1 ps, so the "above threshold" stimulus
//   must be a frequency whose 1 ps-quantized period is strictly shorter than the
//   8.000 ns period of the max-rate clock. ABOVE_CLOCK_RATE is derived so that
//   1e12 / ABOVE_CLOCK_RATE rounds to 7.999 ns.
//
// Settling time:
//   Each test case waits wait_settle_ms() of monitor_clk time. The watchdog
//   measures a 1 ms window in the monitor_clk domain, so 2 ms of settle time
//   is sufficient for the fault to propagate through the synchronizer chain.
//

`default_nettype none

module clocking_watchdog_tb;

  `include "test_exec.svh"
  import PkgTestExec::*;

  timeunit 1ns/1ps; 
  // Monitor clock frequency used to evaluate the 1 ms measurement window.
  localparam int MONITOR_CLK_FREQ = 100_000_000;
  // Maximum allowed watched_clk rate; matches the DUT parameter.
  localparam int MAX_CLOCK_RATE   = 125_000_000;
  // One Hz below the limit; the watchdog must remain clear.
  localparam int BELOW_CLOCK_RATE = MAX_CLOCK_RATE - 1;
  // First whole-hertz frequency whose 1 ps-quantized period (7.999 ns) is
  // strictly shorter than the 8.000 ns period of MAX_CLOCK_RATE.
  // Derivation: smallest f such that floor(1e12 / f) <= 7_999 ps, i.e.
  //   f >= ceil(1e12 / 7_999) = (1e12 + 7_998) / 7_999 using integer ceiling.
  // The 64-bit literal prevents a 32-bit overflow during elaboration.
  localparam int ABOVE_CLOCK_RATE =
    int'(((64'd1000000000000) + 7_998) / 7_999);

  localparam real MONITOR_CLK_PER_NS = 1.0e9 / MONITOR_CLK_FREQ;

  logic monitor_clk  = 1'b0;
  logic monitor_rst;
  logic rst          = 1'b1;
  logic clock_locked = 1'b0;
  logic watched_clk  = 1'b0;
  logic watched_rst;
  logic clock_fault;

  sim_clock_gen #(
    .PERIOD    (MONITOR_CLK_PER_NS),
    .AUTOSTART  (0)
  ) monitor_clk_gen (
    .clk (monitor_clk),
    .rst (monitor_rst)
  );

  sim_clock_gen #(
    .PERIOD    (1.0e9 / MAX_CLOCK_RATE),
    .AUTOSTART  (0)
  ) watched_clk_gen (
    .clk (watched_clk),
    .rst (watched_rst)
  );

  clocking_watchdog #(
    .MAX_CLOCK_RATE  (MAX_CLOCK_RATE),
    .MONITOR_CLK_FREQ(MONITOR_CLK_FREQ)
  ) dut (
    .monitor_clk (monitor_clk),
    .rst         (rst),
    .clock_locked(clock_locked),
    .watched_clk (watched_clk),
    .clock_fault (clock_fault)
  );

  // Set the watched_clk frequency at run time by reprogramming the clock
  // generator period. freq_hz is in Hz; the generator expects nanoseconds.
  task automatic set_watched_clk_freq(input int freq_hz);
    watched_clk_gen.set_period(1.0e9 / freq_hz);
    $display("[%0t] watched_clk=%0.3f Hz", $time, freq_hz);
  endtask

  // Sample clock_fault and compare against expected. Uses strict inequality
  // ("!==" rather than "!=") so X/Z values are treated as failures.
  task automatic check_fault(input logic expected, input string test_name);
    test.start_test(test_name, 10ms);

    assert (clock_fault == expected) begin
      $display("PASS (%s): clock_fault=%0b at time %0t", test_name, clock_fault, $time);
      test.end_test();
    end else begin
      $error("FAIL (%s): clock_fault=%0b expected=%0b at time %0t", test_name, clock_fault, expected, $time);
      test.end_test(0);
    end

  endtask

  // Wait settle_ms milliseconds, counted in monitor_clk cycles. Using the
  // monitor clock rather than $realtime keeps the TB independent of the
  // watched_clk rate and avoids relying on DUT-internal timing.
  task automatic wait_settle_ms(input int settle_ms);
    localparam int MON_CYCLES_PER_MS = MONITOR_CLK_FREQ / 1000;
    repeat (settle_ms * MON_CYCLES_PER_MS) @(posedge monitor_clk);
  endtask

  // Assert reset long enough to initialize both clock domains.
  // The monitor-domain registers reset asynchronously; 8 monitor cycles is
  // sufficient for those. clock_div resets synchronously to watched_clk, so
  // rst must remain asserted through at least 2 watched_clk rising edges to
  // guarantee its internal counter is cleared before measurement begins.
  task automatic startup_reset();
    rst <= 1'b1;
    repeat (8) @(posedge monitor_clk);
    // clock_div reset is synchronous to watched_clk, so keep rst asserted
    // through watched_clk edges to initialize its internal counter.
    repeat (2) @(posedge watched_clk);
    rst <= 1'b0;
    repeat (4) @(posedge monitor_clk);
  endtask

  initial begin : tb_main

    test.start_tb("clocking_watchdog", 100ms);

    monitor_clk_gen.start();
    watched_clk_gen.start();

    set_watched_clk_freq(MAX_CLOCK_RATE);
    startup_reset();

    // Requirement 1: If clock_locked is not asserted, clock_fault must be 1.
    clock_locked <= 1'b0;
    wait_settle_ms(1);
    check_fault(1'b1, "clock_locked deasserted");

    // Assert lock, keep watched clock at max rate. Fault must clear.
    clock_locked <= 1'b1;
    wait_settle_ms(2);
    check_fault(1'b0, "watched_clk exactly max_clock_rate");

    // Requirement 2 (lower): At lower than max, fault must remain clear.
    set_watched_clk_freq(BELOW_CLOCK_RATE);
    wait_settle_ms(2);
    check_fault(1'b0, "watched_clk lower than max_clock_rate");

    // Requirement 2 (higher): Use the nearest detectable above-threshold rate.
    $display("Nearest above-threshold rate used: %0d Hz (offset +%0d Hz)",
      ABOVE_CLOCK_RATE, ABOVE_CLOCK_RATE - MAX_CLOCK_RATE);
    set_watched_clk_freq(ABOVE_CLOCK_RATE);
    wait_settle_ms(2);
    check_fault(1'b1, "watched_clk above max_clock_rate");

    test.end_tb();
  end

endmodule

`default_nettype wire
