//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: strobe_data_if
//
// Description:
//
//   Package defining a Radio Data interface.
//
//   The radio interface uses a simple strobe/enable interface:
//     - data: Radio data word
//     - strobe: Indicates that the data is valid
//

interface strobe_data_if #(
    parameter int NSPC   = 1,  // Number of samples per clock cycle
    parameter int SAMP_W = 32  // Length of each radio sample

) (
    input logic strobe_clk,
    input logic strobe_rst
);
  import rfnoc_chdr_utils_pkg::*;

  logic [     SAMP_W*NSPC-1:0] data;
  logic                        strobe;
  logic [CHDR_TIMESTAMP_W-1:0] timebase;

  modport master(input strobe_clk, input strobe_rst, output data, output strobe, output timebase);

  modport slave(input strobe_clk, input strobe_rst, input data, input strobe);

endinterface : strobe_data_if
