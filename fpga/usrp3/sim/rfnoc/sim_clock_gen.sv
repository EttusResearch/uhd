//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: sim_clock_gen
//
// Description: This module generates a clock and reset signal for the purposes 
// of simulation. Both clock and reset are configurable at run time for 
// software-based simulation control.
//



module sim_clock_gen #(
  parameter realtime PERIOD     = 10.0,  // Period in ns
  parameter real     DUTY_CYCLE = 0.5,   // Duty cycle, in the range (0.0, 1.0)
  parameter bit      AUTOSTART  = 1      // Start clock automatically at time 0
) (
  output bit clk,
  output bit rst
);
  timeunit      1ns;
  timeprecision 1ps;

  realtime period       = PERIOD;
  real     duty         = DUTY_CYCLE;
  realtime low_time     = PERIOD * (1.0 - DUTY_CYCLE);
  realtime high_time    = PERIOD * DUTY_CYCLE;
  bit      toggle       = AUTOSTART;
  bit      alive        = 1;


  //-----------------------
  // Clock and Reset Tasks
  //-----------------------

  // Set the period and duty cycle for the clock
  function void set_clock(real new_period, real new_duty);
    low_time  = new_period * (1.0 - new_duty);
    high_time = new_period * new_duty;
  endfunction

  // Set the period, only, for the clock
  function void set_period(real new_period);
    set_clock(new_period, duty);
  endfunction

  // Set the duty cycle, only, for the clock
  function void set_duty(real new_duty);
    set_clock(period, new_duty);
  endfunction

  // Start toggling the clock
  function void start();
    toggle = 1;
  endfunction

  // Stop toggling the clock
  function void stop();
    toggle = 0;
  endfunction

  // Stop running the clock loop (no new simulation events will be created)
  function void kill();
    alive = 0;
  endfunction

  // Start running the clock loop (new simulation events will be created)
  function void revive();
    alive = 1;
  endfunction

  // Asynchronously assert the reset signal and synchronously deassert it after 
  // "length" clock cycles.
  task reset(int length = 8);
    fork
      begin
        rst <= 1;
        repeat (length) @(posedge clk);
        rst <= 0;
      end
    join_none

    // Make sure rst asserts before we return
    wait(rst);
  endtask : reset

  // Assert reset
  task set_reset();
    rst <= 1'b1;
  endtask : set_reset

  // Deassert reset
  task clr_reset();
    rst <= 1'b0;
  endtask : clr_reset

  // Wait for num rising edges of the clock
  task clk_wait_r(int num = 1);
    repeat(num) @(posedge clk);
  endtask

  // Wait for num falling edges of the clock
  task clkd_wait_f(int num = 1);
    repeat(num) @(negedge clk);
  endtask


  //--------------------------
  // Clock Generation Process
  //--------------------------

  initial begin : clock_block
    // Toggle the clock in a loop
    forever begin : clock_loop
      #(low_time);
      if (toggle) clk = 0;
      
      #(high_time);
      if (toggle) clk = 1;

      wait (alive);
    end
  end

endmodule : sim_clock_gen