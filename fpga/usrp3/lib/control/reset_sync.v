//
// Copyright 2011 Ettus Research LLC
//




module reset_sync
  (input clk,
   input reset_in,
   output  reset_out);

   reg 	   reset_int;

   reg 	   reset_out_tmp;

   //synthesis attribute async_reg of reset_out_tmp is "true";
   //synthesis attribute async_reg of reset_int is "true";
   always @(posedge clk or posedge reset_in)
     if(reset_in)
       {reset_out_tmp,reset_int} <= 2'b11;
     else
       {reset_out_tmp,reset_int} <= {reset_int,1'b0};

   assign reset_out = reset_out_tmp;


endmodule // reset_sync
