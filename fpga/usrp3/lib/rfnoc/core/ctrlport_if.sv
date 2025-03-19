//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Interface: ctrlport_if
//
// Description:
//
//   Defines a SystemVerilog interface for the control-port bus.
//

interface ctrlport_if (
  input logic clk,
  input logic rst
);

  import ctrlport_pkg::*;

  // Consisting of a request and a response moving in different directions.
  ctrlport_request_t  req;
  ctrlport_response_t resp;

  // Master driving the request and consuming the response.
  modport master (
    input  clk,
    input  rst,

    output req,
    input  resp
  );

  // Slave consuming the request and driving the response.
  modport slave (
    input  clk,
    input  rst,

    input  req,
    output resp
  );

endinterface : ctrlport_if
