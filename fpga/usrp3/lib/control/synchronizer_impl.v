//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module synchronizer_impl #(
   parameter WIDTH       = 1,
   parameter STAGES      = 2,
   parameter INITIAL_VAL = 0
)(
   input              clk,
   input              rst,
   input  [WIDTH-1:0] in,
   output [WIDTH-1:0] out
);

   (* ASYNC_REG = "TRUE" *) reg [WIDTH-1:0] value[0:STAGES-1];

   integer k;
   initial begin
     for (k = 0; k < STAGES; k = k + 1) begin
       value[k] = INITIAL_VAL;
     end
   end

   genvar i;
   generate
      for (i=0; i<STAGES; i=i+1) begin: stages
         always @(posedge clk) begin
            if (rst) begin
               value[i] <= INITIAL_VAL;
            end else begin
               if (i == 0) begin
                 value[i] <= in;
               end else begin
                 value[i] <= value[i-1];
               end
            end
         end
      end
   endgenerate

   assign out = value[STAGES-1];

endmodule   //synchronizer_impl
