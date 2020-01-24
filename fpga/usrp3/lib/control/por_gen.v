//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//



module por_gen
  (input clk,
   output reset_out);

   reg 		por_rst;
   reg [7:0] 	por_counter = 8'h0;

   always @(posedge clk)
     if (por_counter != 8'h55)
       begin
          por_counter <= por_counter + 8'h1;
          por_rst <= 1'b1;
       end
     else
       por_rst <= 1'b0;

   assign reset_out = por_rst;

endmodule // por_gen
