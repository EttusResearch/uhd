//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgMovingAverage
//
// Description: This package contains the MovingAverage class, which models
// the expected behavior of the moving average RFNoC block.
//


package PkgMovingAverage;

  // This class implements the same moving-average computation as the DUT
  class MovingAverage;

    local int sum;          // Current running sum
    local int sum_length;   // Number of samples to sum or average
    local int divisor;      // Divisor value to use for averaging
    local int history[$];   // History of previous values

    function new();
      // Initialize all values
      set_sum_length(0);
    endfunction : new

    // Set the number of vales to sum
    function void set_sum_length(int value);
      sum = 0;
      sum_length = value;
      history = {};
    endfunction : set_sum_length

    // Set the divisor value
    function void set_divisor(int value);
      divisor = value;
    endfunction : set_divisor

    // Add a value to the history of values used in the sum computation
    function void add_value(int value);
      history.push_back(value);
      sum += value;

      // Check if we have a full history
      if (history.size() > sum_length) begin
        sum -= history.pop_front();
      end
    endfunction : add_value

    // Return the current running sum
    function int get_sum();
      return sum;
    endfunction : get_sum

    // Return the current running average
    function int get_average();
      int result;

      // Round to nearest integer, the same way the IP does.
      result = $floor(real'(get_sum())/real'(divisor) + 0.5);

      // Saturate to 16-bit signed
      if (result > 32767) result = 32767;
      if (result < -32768) result = 32768;

      return result;
    endfunction : get_average

  endclass : MovingAverage

endpackage
