
//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Module: device_dna_ctrlport
//
// Description:
//
//   Read back the PL DNA via CtrlPort transactions, 32-bit at a time.
//
//   This module will return the PL DNA via a 32-bit ctrlport transaction. This
//   means the DNA value is split up into multiple registers. For example, if a
//   96 bit DNA width is selected, there will be three consecutive registers
//   holding the DNA value.
//
//   Note the DNA value width is chip-dependent. For example, Ultrascale+ devices
//   like the RFSoC have a 96-bit DNA value. Everything above the 96 bits would be
//   zero-padded.
//
//   After resetting, it takes some clock cycles to load the DNA value. During
//   this time, transactions will return an error code.
//
// Parameters:
//
//   BASE_ADDR: Readback address for the 32 LSBs of the device DNA. The next
//              32 bits will be addressable at BASE_ADDR+4, and so on.
//   DNA_WIDTH: The width of the DNA register. UltraScale(+) devices have a 96-bit
//              DNA, 7-series have a 57-bit DNA. If DNA_WIDTH is smaller than
//              that, only LSBs will be output. If it's larger, then the DNA
//              will be zero-padded.
//   DEVICE_TYPE: Either ULTRASCALE or 7SERIES.
//

`default_nettype none

module device_dna_ctrlport #(
  parameter BASE_ADDR   = 0,
  // Number of bits of DNA to output
  parameter DNA_WIDTH   = 96,
  // For future use: Different FPGA types have different primitives for reading DNA
  parameter DEVICE_TYPE = "ULTRASCALE"
)(
  input wire         ctrlport_clk,
  input wire         reset,

  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data
);

  `include "../rfnoc/core/ctrlport.vh"

  if (DEVICE_TYPE != "ULTRASCALE" && DEVICE_TYPE != "7SERIES") begin : gen_assertion
    ERROR_only_ultrascale_and_7series_supported();
  end

  wire [DNA_WIDTH-1:0] device_dna_value;
  wire                 device_dna_valid;
  wire [1:0]           reg_ro_status;

  device_dna #(
    .DNA_WIDTH(DNA_WIDTH),
    .DEVICE_TYPE(DEVICE_TYPE)
  ) device_dna_i (
    .clk  (ctrlport_clk),
    .rst  (reset),
    .dna  (device_dna_value),
    .valid(device_dna_valid)
  );

  ctrlport_reg_ro #(
    .ADDR  (BASE_ADDR),
    .WIDTH (DNA_WIDTH)
    // Don't need to assert COHERENT, because device_dna_value won't change unless
    // device_dna_valid is also deasserted
  ) dna_ctrlport_reg_ro_i (
    .ctrlport_clk           (ctrlport_clk        ),
    .s_ctrlport_req_rd      (s_ctrlport_req_rd   ),
    .s_ctrlport_req_addr    (s_ctrlport_req_addr ),
    .s_ctrlport_resp_ack    (s_ctrlport_resp_ack ),
    .s_ctrlport_resp_status (reg_ro_status       ),
    .s_ctrlport_resp_data   (s_ctrlport_resp_data),
    .value_in               (device_dna_value    )
  );

  // If we don't have a valid timestamp yet, we finish transaction,
  // but with an error code
  assign s_ctrlport_resp_status = device_dna_valid ? reg_ro_status : CTRL_STS_CMDERR;

endmodule

`default_nettype wire
