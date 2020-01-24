//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: pulse_stretch_min
//
// Description:
//
//   Pulse stretcher, to guarantee a minimum pulse width. Takes a short input 
//   pulse and outputs a pulse that is LENGTH clock cycles long. If the input 
//   pulse is longer than LENGTH then the output pulse will be the same length 
//   as the input pulse. The output is registered so the output is delayed by 
//   one clock cycle relative to the input. If more than one pulse is input 
//   within LENGTH+1 clock cycles, then the extra input pulses will not 
//   generate output pulses.
//
// Examples:  (LENGTH = 3)
//
//   Clock      _/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_
//
//
//   pulse_in   _/‾‾‾\_______________/‾‾‾‾‾‾‾\___________/‾‾‾‾‾‾‾‾‾‾‾\_______
//
//   pulse_out  _____/‾‾‾‾‾‾‾‾‾‾‾\_______/‾‾‾‾‾‾‾‾‾‾‾\_______/‾‾‾‾‾‾‾‾‾‾‾\___
//
//
//   pulse_in   _/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\______________/‾‾‾\_______/‾‾‾\______
//
//   pulse_out  ______/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\__
//
//
// Parameters:
//
//   LENGTH : Length of the minimum pulse to output, in clock cycles.
//

module pulse_stretch_min #(
  parameter LENGTH = 4
) (
  input  wire clk,
  input  wire rst,
  input  wire pulse_in,
  output reg  pulse_out = 0
);

  reg [$clog2(LENGTH)-1:0] count = 0;
  reg                      state = 0;

  always @ (posedge clk)
    if (rst) begin
      state     <= 0;
      count     <= 0;
      pulse_out <= 0;
    end
    else begin
      case (state)

      1'b0: begin
        if (pulse_in) begin
          state     <= 1;
          pulse_out <= 1;
          count     <= 0;
        end
      end

      1'b1: begin
        if (count == LENGTH-1) begin
          if (!pulse_in) begin
            state     <= 0;
            pulse_out <= 0;
          end
        end else
          count <= count + 1;
      end
      endcase
    end

endmodule
