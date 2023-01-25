//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: setting_reg
//
// Description:
//
//   A settings register is a peripheral for the settings register bus. When
//   the settings register sees the strobe and a matching address, the outputs
//   will be become registered to the given input bus.
//

module setting_reg #(
  parameter my_addr  = 0,
  parameter awidth   = 8,
  parameter width    = 32,
  parameter at_reset = 0
) (
  input  wire              clk,
  input  wire              rst,
  input  wire              strobe,
  input  wire [awidth-1:0] addr,
  input  wire [      31:0] in,
  output reg  [ width-1:0] out,
  output reg               changed
);
  always @(posedge clk)
    if(rst) begin
      out <= at_reset;
      changed <= 1'b0;
    end else begin
      if(strobe && (my_addr == addr)) begin
        out     <= in[width-1:0];
        changed <= 1'b1;
      end else begin
        changed <= 1'b0;
      end
    end
endmodule // setting_reg
