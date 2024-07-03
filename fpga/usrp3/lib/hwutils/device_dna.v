//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Module: device_dna
//
// Description:
//
//   Read back the PL DNA.
//   When reset, this module will shift the PL DNA into an output register.
//
// Parameters:
//
//   DNA_WIDTH: The width of the DNA register. UltraScale(+) devices have a 96-bit
//              DNA, 7-series have a 57-bit DNA. If DNA_WIDTH is smaller than
//              that, only LSBs will be output. If it's larger, then the DNA
//              will be zero-padded.
//   DEVICE_TYPE: Either ULTRASCALE or 7SERIES.
//
// Signals:
//
//   dna:   This register will hold the full DNA value.
//   valid: This is low while the DNA value register is being populated. Only
//          when high is the value in `dna' valid.
//

`default_nettype none

module device_dna #(
  // Number of bits of DNA to output
  parameter DNA_WIDTH   = 96,
  // For future use: Different FPGA types have different primitives for reading DNA
  parameter DEVICE_TYPE = "ULTRASCALE"
)(
  input wire                 clk,
  input wire                 rst,

  // The device DNA
  output reg [DNA_WIDTH-1:0] dna,
  // This output is low while the dna register is being populated
  output wire                valid
);

  localparam ST_RESET = 2'd0;
  localparam ST_READ  = 2'd1;
  localparam ST_SHIFT = 2'd2;
  localparam ST_DONE  = 2'd3;

  localparam MAX_SHIFT = (DEVICE_TYPE == "7SERIES") ? 57 : DNA_WIDTH;

  reg [1:0]                   state   = ST_RESET;
  reg [$clog2(DNA_WIDTH)-1:0] bit_cnt = 0;

  wire read;
  wire shift;
  wire dout0;


  always @(posedge clk) begin
    if (rst) begin
      dna   <= {DNA_WIDTH{1'b0}};
      state <= ST_READ;
    end else if (state == ST_READ) begin
      state <= ST_SHIFT;
      bit_cnt <= MAX_SHIFT-1;
    end else if (state == ST_SHIFT) begin
      // Ultrascale shifts the DNA out LSB first, 7-series shifts the DNA out
      // MSB first (cf. UG-470 and UG-570 for 7-series and Ultrascale).
      if (DEVICE_TYPE == "ULTRASCALE") begin
        dna     <= { dout0, dna[DNA_WIDTH-1:1] };
      end else if (DEVICE_TYPE == "7SERIES") begin
        dna     <= { dna[DNA_WIDTH-2:0], dout0 };
      end
      state   <= (bit_cnt == 0) ? ST_DONE : ST_SHIFT;
      bit_cnt <= bit_cnt - 1;
    end else begin
      // Nothing in ST_DONE
    end
  end

  assign read  = state == ST_READ;
  assign shift = state == ST_SHIFT;
  assign valid = state == ST_DONE;

  if (DEVICE_TYPE == "ULTRASCALE") begin : gen_ultrascale_dna

    DNA_PORTE2 #(
      .SIM_DNA_VALUE(96'h12F1110_C0D111A0_11C0FFEE)
    ) dna_inst (
      .CLK  (clk),
      .DIN  (1'b0),
      .READ (read),
      .SHIFT(shift),
      .DOUT (dout0)
    );

  end else if (DEVICE_TYPE == "7SERIES") begin : gen_7series_dna

    DNA_PORT #(
      .SIM_DNA_VALUE(57'h0D111A0_C0DE00FF)
    ) dna_inst (
      .CLK  (clk),
      .DIN  (1'b0),
      .READ (read),
      .SHIFT(shift),
      .DOUT (dout0)
    );

  end else begin : gen_assert
    ERROR_invalid_device_type();
  end

endmodule

`default_nettype wire
