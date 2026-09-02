//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_utils_pkg
//
// Description:
//
//   Utility functions for CIC stages that consume multiple samples per clock cycle.
//

package cic_utils_pkg;

  function automatic int barrel_shift_width(
    input int in_width,
    input int out_width
  );
    return $clog2(in_width/2 - out_width/2 + 1);
  endfunction

  // The latency of the barrel shift stage is the number of stages in the barrel
  // shifter plus one cycle for the output register.
  function automatic int barrel_shift_latency(input int in_width, input int out_width);
    return barrel_shift_width(in_width, out_width) + 1;
  endfunction

  function automatic int integrator_dsp_latency(input int spc, input int order);
    return order * (spc + 1) + spc + 1;
  endfunction

  // The latency of the integrator is the DSP latency plus one cycle for the output register.
  function automatic int integrator_latency(input int spc, input int order);
    return integrator_dsp_latency(spc, order) + 1;
  endfunction

  // The decimator has a registered compute_output stage and a 1-cycle
  // output FIFO.
  function automatic int decimator_latency();
    return 2;
  endfunction

  // Each comb stage contains one registered output FIFO stage.
  function automatic int comb_filter_latency(input int order);
    return order;
  endfunction

  class cic_utils #(
    parameter int SPC         = 8,   // Number of samples processed per clock cycle
    parameter int SAMP_W      = 48   // Input/output data width
  );
    //---------------------------------------------------------------------------
    // Parameter declarations and Type definitions
    //---------------------------------------------------------------------------
    localparam int SPC_LOG2 = (SPC == 1) ? 1 : $clog2(SPC);
    localparam bit VERBOSE = 0;

    typedef logic        [SAMP_W-1   : 0] sample_t;
    typedef logic signed [SAMP_W/2-1 : 0] comp_t;
    typedef sample_t     [SPC-1      : 0] word_t;
    typedef int                           adder_conn_array_t[SPC_LOG2][SPC];

  endclass : cic_utils

endpackage : cic_utils_pkg
