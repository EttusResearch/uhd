//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: cic_filter_test_pkg
//
// Description:
//
//   Package containing utilities and simulation models for CIC multisample filter TBs.
//
// Parameters:
//
//   SPC          : Number of samples per clock cycle on DUT
//   ACCUM_W      : Width of a DUT input sample. Equivalent to the accumulator width.
//   COMP_W       : Width of I and Q components in the input samples. This is
//                  used to generate test inputs and should be less than or equal to
//                  ACCUM_W/2.
//

`default_nettype none

package cic_filter_test_pkg;

  import cic_utils_pkg::*;
  import cic_test_pkg::*;
  import cic_comb_filter_test_pkg::*;

  //---------------------------------------------------------------------------
  // Bit-true simulation model of a full Nth-order CIC decimation filter.
  //
  // Chains ORDER integrators -> decimate by R -> ORDER combs (D=1),
  // processing one sample at a time (SPC-agnostic). All stages operate at
  // ACCUM_W width with modular (wrapping) two's complement arithmetic,
  // matching the RTL which performs no internal clipping or rounding.
  //
  // State is created fresh on each call to process() (zero initial
  // conditions), so there is no need to explicitly clear between runs.
  //---------------------------------------------------------------------------
  class cic_filter_decim_model #(
    int ACCUM_W = 96,
    int COMP_W  = 16,
    int ORDER   = 4
  );

    typedef cic_utils#(.SAMP_W(ACCUM_W)) util_c;
    typedef util_c::sample_t sample_t;
    typedef sample_t         sample_queue_t[$];

    function new();
    endfunction

    //-------------------------------------------------------------------------
    // process: run samples through the full CIC decimation chain.
    //
    // ORDER integrators (full rate) -> decimate by R -> ORDER combs (D=1).
    // All stages operate at ACCUM_W width with wrapping two's complement
    // arithmetic. State is created fresh each call (zero initial conditions).
    //
    //   samples_in   : flat queue of input samples (full rate).
    //   decim_factor : decimation ratio R (keep every R-th sample).
    //   samples_out  : flat queue of output samples (decimated rate).
    //-------------------------------------------------------------------------
    function void process(
      input  sample_queue_t samples_in,
      input  int            decim_factor,
      output sample_queue_t samples_out
    );
      sample_queue_t stage_data, next_data;

      stage_data = samples_in;

      // ORDER integrator stages (full input rate).
      // SAMP_W == ACCUM_W so the saturate is a no-op between stages.
      begin
        automatic cic_integrator_model #(
          .SAMP_W (ACCUM_W),
          .ACCUM_W(ACCUM_W),
          .ORDER  (ORDER)
        ) integ = new();
        integ.process_packet(stage_data, next_data);
        stage_data = next_data;
      end

      // Decimate by R: keep every R-th sample.
      next_data = {};
      foreach (stage_data[i]) begin
        if (i % decim_factor == 0)
          next_data.push_back(stage_data[i]);
      end
      stage_data = next_data;

      // ORDER comb stages (decimated rate, fixed delay D=1).
      repeat (ORDER) begin
        automatic cic_comb_filter_test_model #(
          .ACCUM_W(ACCUM_W)
        ) comb = new();
        comb.process_samples(stage_data, next_data);
        stage_data = next_data;
      end

      samples_out = stage_data;
    endfunction

  endclass : cic_filter_decim_model


  //---------------------------------------------------------------------------
  // Bit-true simulation model of a full Nth-order CIC interpolation filter.
  //
  // Chains ORDER combs (D=1, input rate) -> upsample by R -> ORDER integrators
  // (full output rate), processing one sample at a time (SPC-agnostic). All
  // stages operate at ACCUM_W width with modular (wrapping) two's complement
  // arithmetic, matching the RTL which performs no internal clipping or
  // rounding.
  //
  // State is created fresh on each call to process() (zero initial
  // conditions), so there is no need to explicitly clear between runs.
  //---------------------------------------------------------------------------
  class cic_filter_interp_model #(
    int ACCUM_W = 96,
    int COMP_W  = 16,
    int ORDER   = 4
  );

    typedef cic_utils#(.SAMP_W(ACCUM_W)) util_c;
    typedef util_c::sample_t sample_t;
    typedef sample_t         sample_queue_t[$];

    function new();
    endfunction

    //-------------------------------------------------------------------------
    // process: run samples through the full CIC interpolation chain.
    //
    // ORDER combs (D=1, input rate) -> upsample by R -> ORDER integrators
    // (full output rate). All stages operate at ACCUM_W width with wrapping
    // two's complement arithmetic. State is created fresh each call (zero
    // initial conditions).
    //
    //   samples_in    : flat queue of input samples (lower rate).
    //   interp_factor : interpolation ratio R (produce R outputs per input).
    //   samples_out   : flat queue of output samples (higher rate).
    //-------------------------------------------------------------------------
    function void process(
      input  sample_queue_t samples_in,
      input  int            interp_factor,
      output sample_queue_t samples_out
    );
      sample_queue_t stage_data, next_data;

      stage_data = samples_in;

      // ORDER comb stages (input rate, fixed delay D=1).
      repeat (ORDER) begin
        automatic cic_comb_filter_test_model #(
          .ACCUM_W(ACCUM_W)
        ) comb = new();
        comb.process_samples(stage_data, next_data);
        stage_data = next_data;
      end

      // Upsample by R: each input sample is followed by R-1 zeros.
      next_data = {};
      foreach (stage_data[i]) begin
        next_data.push_back(stage_data[i]);
        repeat (interp_factor - 1) begin
          next_data.push_back('0);
        end
      end
      stage_data = next_data;

      // ORDER integrator stages (full output rate).
      // SAMP_W == ACCUM_W so the saturate is a no-op between stages.
      begin
        automatic cic_integrator_model #(
          .SAMP_W (ACCUM_W),
          .ACCUM_W(ACCUM_W),
          .ORDER  (ORDER)
        ) integ = new();
        integ.process_packet(stage_data, next_data);
        stage_data = next_data;
      end

      samples_out = stage_data;
    endfunction

  endclass : cic_filter_interp_model

endpackage : cic_filter_test_pkg

`default_nettype wire
