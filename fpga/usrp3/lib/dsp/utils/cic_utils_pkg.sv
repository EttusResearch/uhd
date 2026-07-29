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
