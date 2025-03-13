//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_channel_mux_all_tb.sv
//
// Description:
//
// Top level testbench for the chdr_channel_mux module. The testbench instantiates
// the chdr_channel_mux_tb for different combinations of channels/ports and CHDR_W.
//

`default_nettype none

module chdr_channel_mux_all_tb;

  // Run test for all combinations of CHDR_W and NUM_PORTS
  for (genvar chdr_w = 64; chdr_w <= 512; chdr_w *= 2) begin : chdr_w_loop
    // Test common even number of ports
    for (genvar num_ports = 1; num_ports <= 4; num_ports++) begin : num_ports_loop
      chdr_channel_mux_tb #(
        .NUM_PORTS(num_ports),
        .CHDR_W(chdr_w)
      ) tb_i ();
    end
  end

endmodule

`default_nettype wire
