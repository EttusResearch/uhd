//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  pulse_stretch
//
// Description:
//
//   Pulse stretcher. Takes any input pulse that is SCALE+2 clock cycles or 
//   less and outputs a pulse that is SCALE+1 clock cycles. However, if an 
//   input pulse is longer than SCALE+2 clock cycles then the output pulse 
//   repeats. If more than one pulse is input within SCALE+2 clock cycles then 
//   additional pulses will be ignored.
//
// Examples (SCALE = 2):
//
//   Clock            _/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_/‾\_
//
//
//   pulse            _/‾‾‾\_______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\___________________
//
//   pulse_stretched  _____/‾‾‾‾‾‾‾‾‾‾‾\_______/‾‾‾‾‾‾‾‾‾‾‾\___________________
//
//
//   pulse            _/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\___________________________________
//
//   pulse_stretched  _____/‾‾‾‾‾‾‾‾‾‾‾\___/‾‾‾‾‾‾‾‾‾‾‾\_______________________
//
//
//   pulse            _/‾‾‾\_______/‾‾‾\___/‾‾‾\_______/‾‾‾‾‾‾‾\_______________
//
//   pulse_stretched  _____/‾‾‾‾‾‾‾‾‾‾‾\_______/‾‾‾‾‾‾‾‾‾‾‾\___/‾‾‾‾‾‾‾‾‾‾‾\___

// Parameters:
//
//   SCALE : The number of clock cycles to add to a single cycle pulse. Or, the
//           number of clock cycles, minus 1, for the output pulse.
//

module pulse_stretch #(
  parameter SCALE = 64'd12_500_000
)(
  input clk,
  input rst,
  input pulse,
  output pulse_stretched
);

  reg [$clog2(SCALE+1)-1:0] count = 'd0;
  reg             state = 1'b0;

  always @ (posedge clk)
    if (rst) begin
      state <= 1'b0;
      count <= 'd0;
    end
    else begin
      case (state)

      1'b0: begin
        if (pulse) begin
          state <= 1'b1;
          count <= 'd0;
        end
      end

      1'b1: begin
        if (count == SCALE)
          state <= 1'b0;
        else
          count <= count + 1'b1;
      end
      endcase
    end

  assign pulse_stretched = (state == 1'b1);

endmodule
