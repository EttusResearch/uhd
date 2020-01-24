
//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module gpio_atr_io #(
  parameter WIDTH = 32
) (
  input               clk,
  input  [WIDTH-1:0]  gpio_ddr,
  input  [WIDTH-1:0]  gpio_out,
  output [WIDTH-1:0]  gpio_in,
  inout  [WIDTH-1:0]  gpio_pins
);

  //Instantiate registers in the IOB
  (* IOB = "true" *) reg [WIDTH-1:0] gpio_in_iob, gpio_out_iob;
  always @(posedge clk) begin
    gpio_in_iob   <= gpio_pins;
    gpio_out_iob  <= gpio_out;
  end
  assign gpio_in = gpio_in_iob;

  //Pipeline the data direction bus
  reg [WIDTH-1:0] gpio_ddr_reg;
  always @(posedge clk)
    gpio_ddr_reg <= gpio_ddr;

  //Tristate buffers
  genvar i;
  generate for (i=0; i<WIDTH; i=i+1) begin: io_tristate_gen
    assign gpio_pins[i] = gpio_ddr_reg[i] ? gpio_out_iob[i] : 1'bz;
  end endgenerate

endmodule // gpio_atr_io
