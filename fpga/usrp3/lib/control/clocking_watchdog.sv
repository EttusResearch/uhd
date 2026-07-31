//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: clocking_watchdog
//
// Description:
//   Monitors clocking-health indicators and asserts clock_fault if any
//   indicator is out of spec. Two indicators are checked:
//
//   1. Lock status (clock_locked)
//      clock_fault is asserted in the monitor_clk domain whenever clock_locked
//      is not asserted (after synchronization into monitor_clk). This covers
//      PLL unlock events regardless of the actual frequency of watched_clk.
//
//   2. Frequency of watched_clk
//      The rate of watched_clk is measured by dividing it down to ~1 kHz
//      with clock_div (N = CLK_DIV_1KHZ, chosen to be even), synchronising
//      the divided clock into the monitor_clk domain, and counting how many
//      monitor_clk cycles pass between two consecutive rising edges of the
//      divided clock. A count below (MS_CYCLES_MON_CLK - 1) means the
//      watched_clk edge arrived sooner than expected, i.e. watched_clk is
//      running faster than MAX_CLOCK_RATE, and clock_fault_mon is asserted.
//      The counter saturates at MS_CYCLES_MON_CLK to prevent rollover when
//      watched_clk is slower than the maximum rate.
//
//   clock_fault_mon (monitor_clk domain) is synchronised back into the
//   watched_clk domain before being driven out as clock_fault so that
//   downstream consumers can safely sample it on either clock.
//
// Clock domains:
//   monitor_clk : measurement and fault evaluation domain
//   watched_clk : clock being monitored; only clock_div runs here
//
// Reset:
//   rst is synchronous to monitor_clk. For logic in the watched_clk domain
//   (clock_div and the clock_fault synchronizer), rst is synchronized into the
//   watched_clk domain before use.
//

module clocking_watchdog #(
  // Maximum allowed frequency of watched_clk in Hz. A watched_clk rate
  // strictly above this value causes clock_fault to be asserted.
  parameter int MAX_CLOCK_RATE   = 125_000_000,
  // Frequency of monitor_clk in Hz. Used to derive the 1 ms measurement
  // window (MS_CYCLES_MON_CLK = MONITOR_CLK_FREQ / 1000).
  parameter int MONITOR_CLK_FREQ = 100_000_000
) (
  // Free-running reference clock used for all measurement logic.
  input  logic monitor_clk,
  // Synchronous reset (monitor_clk domain). Also drives clk_in_rst of
  // clock_div so the watched_clk-domain divider is reset as well.
  input  logic rst,
  // PLL/MMCM lock indicator for watched_clk. Active high; when low,
  // clock_fault is asserted immediately regardless of the measured rate.
  input  logic clock_locked,
  // Clock whose frequency is being monitored.
  input  logic watched_clk,
  // Asserted (1) when a clocking fault is detected, deasserted (0) when
  // clock_locked is high and watched_clk is within [0, MAX_CLOCK_RATE].
  // Registered in the watched_clk domain.
  output logic clock_fault
);
  
  // Number of monitor_clk cycles in 1 ms; defines the measurement window.
  localparam int unsigned MS_CYCLES_MON_CLK = MONITOR_CLK_FREQ / 1000;
  // Number of watched_clk cycles in 1 ms at MAX_CLOCK_RATE.
  localparam int unsigned MAX_COUNT_PER_MS = MAX_CLOCK_RATE / 1000;
  // Worst-case tolerance of each clock source in ppm. The frequency margin
  // assumes two independent clocks, so their relative error is 2 * CLOCK_PPM.
  localparam int unsigned CLOCK_PPM = 100;
  // Divider ratio for clock_div. clock_div requires an even divisor, so
  // MAX_COUNT_PER_MS is rounded up to the nearest even integer when needed.
  localparam int unsigned CLK_DIV_1KHZ = ((MAX_COUNT_PER_MS % 2) != 0) ?
    (MAX_COUNT_PER_MS + 1) : MAX_COUNT_PER_MS;
  // Worst-case relative error from two independent CLOCK_PPM clocks over a
  // 1 ms measurement window. Ceiling rounds any fractional cycle up into the
  // safety margin.
  localparam int unsigned FREQ_MARGIN_CYCLES =
    int'($ceil(real'(MS_CYCLES_MON_CLK * (2 * CLOCK_PPM)) / 1_000_000.0));
  // One extra monitor_clk cycle for synchronizer/edge-detection quantization
  // when the divided watched clock is sampled into the monitor_clk domain.
  localparam int unsigned SYNC_MARGIN_CYCLES = 1;

  // Counts monitor_clk cycles since the last rising edge of the divided
  // clock. One extra bit to hold MS_CYCLES_MON_CLK without wrap-around.
  logic unsigned [$clog2(MS_CYCLES_MON_CLK+1)-1:0] ms_tick_count;

  logic clock_locked_mon;           // clock_locked synchronised into monitor_clk domain
  logic watched_clk_div1k;          // watched_clk divided by CLK_DIV_1KHZ (~1 kHz)
  logic watched_clk_div1k_mon;      // divided clock synchronised into monitor_clk domain
  logic watched_clk_div1k_mon_prev; // one-cycle delay for rising-edge detection
  logic clock_fault_mon = 1'b1;     // fault flag evaluated in monitor_clk domain
  logic rst_watched_clk;            // rst synchronized into watched_clk domain

  synchronizer #(
    .WIDTH(1),
    .STAGES(2),
    .INITIAL_VAL(0),
    .FALSE_PATH_TO_IN(1)
  ) sync_data_clock_locked_i (
    .clk(monitor_clk),
    .rst(rst),
    .in(clock_locked),
    .out(clock_locked_mon)
  );

  synchronizer #(
    .WIDTH(1),
    .STAGES(2),
    .INITIAL_VAL(1),
    .FALSE_PATH_TO_IN(1)
  ) sync_rst_watched_clk_i (
    .clk(watched_clk),
    .rst(1'b0),
    .in(rst),
    .out(rst_watched_clk)
  );

  clock_div #(
    .N(CLK_DIV_1KHZ)
  ) clock_div_1khz_i (
    .clk_in(watched_clk),
    .clk_in_rst(rst_watched_clk),
    .clk_out(watched_clk_div1k)
  );

  synchronizer #(
    .WIDTH(1),
    .STAGES(2),
    .INITIAL_VAL(0),
    .FALSE_PATH_TO_IN(1)
  ) sync_watched_clk_1khz_i (
    .clk(monitor_clk),
    .rst(rst),
    .in(watched_clk_div1k),
    .out(watched_clk_div1k_mon)
  );

  // Measurement and fault evaluation (monitor_clk domain).
  //
  // On each monitor_clk cycle ms_tick_count increments, saturating at
  // MS_CYCLES_MON_CLK to prevent wrap-around for slow watched_clk rates.
  //
  // On each rising edge of the synchronised divided clock:
  //   - clock_fault_mon is set if ms_tick_count is below the nominal 1 ms
  //     count minus both the frequency-tolerance margin and the
  //     synchronizer quantization margin, meaning the interval completed
  //     too early and watched_clk is running above MAX_CLOCK_RATE.
  //   - ms_tick_count is reset to start the next measurement window.
  //
  // During reset or when clock_locked is not asserted, clock_fault_mon is
  // held high (fault asserted) and the counter is held at zero.
  always_ff @(posedge monitor_clk) begin
    if (rst || !clock_locked_mon) begin
      ms_tick_count <= '0;
      watched_clk_div1k_mon_prev <= '0;
      clock_fault_mon <= 1'b1;
    end else begin
      watched_clk_div1k_mon_prev <= watched_clk_div1k_mon;
      // Saturate to avoid rollover for slower-than-max watched clocks.
      if (ms_tick_count < MS_CYCLES_MON_CLK) begin
        ms_tick_count <= ms_tick_count + 1'b1;
      end
      if (watched_clk_div1k_mon && !watched_clk_div1k_mon_prev) begin
        clock_fault_mon <= ms_tick_count
          < MS_CYCLES_MON_CLK - FREQ_MARGIN_CYCLES - SYNC_MARGIN_CYCLES;
        ms_tick_count <= '0;
      end
    end
  end

  synchronizer #(
    .WIDTH(1),
    .STAGES(2),
    .INITIAL_VAL(1),
    .FALSE_PATH_TO_IN(1)
  ) sync_clock_fault_i (
    .clk(watched_clk),
    .rst(rst_watched_clk),
    .in(clock_fault_mon),
    .out(clock_fault)
  );
  
endmodule
