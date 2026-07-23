//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: pcie_lossy_samp_gate
//
// Description:
//
//   This module implements a gate that can be used to drop samples from a
//   stream. When 'drop' is high, the module will drop all incoming samples by
//   not asserting o_tvalid and by asserting i_tready.
//
// Parameters:
//   DATA_WIDTH : Width of the data bus
//

`default_nettype none

module pcie_lossy_samp_gate
#(
  parameter DATA_WIDTH = 64
)
(
  input  wire  [DATA_WIDTH-1:0]  i_tdata,
  input  wire                    i_tvalid,
  output wire                    i_tready,

  output wire [DATA_WIDTH-1:0]  o_tdata,
  output wire                   o_tvalid,
  input  wire                   o_tready,

  input  wire                  drop,
  output wire                  dropping
);

  assign o_tdata    = i_tdata;
  assign o_tvalid   = i_tvalid & ~drop;
  assign i_tready   = o_tready | drop;

  assign dropping   = drop & i_tvalid;

endmodule // pcie_lossy_samp_gate

`default_nettype wire
