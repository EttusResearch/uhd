//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module add2_and_clip_reg
  #(parameter WIDTH=16)
    (input clk,
     input rst,
     input [WIDTH-1:0] in1,
     input [WIDTH-1:0] in2,
     input strobe_in,
     output reg [WIDTH-1:0] sum,
     output reg strobe_out);

   wire [WIDTH-1:0] sum_int;

   add2_and_clip #(.WIDTH(WIDTH)) add2_and_clip (.in1(in1),.in2(in2),.sum(sum_int));

   always @(posedge clk)
     if(rst)
       sum <= 0;
     else if(strobe_in)
       sum <= sum_int;

   always @(posedge clk) strobe_out <= rst ? 1'b0 : strobe_in;

endmodule // add2_and_clip_reg
