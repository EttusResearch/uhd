

module valve36
  (input clk, input reset, input clear,
   input shutoff,
   input [35:0] data_i, input src_rdy_i, output dst_rdy_o,
   output [35:0] data_o, output src_rdy_o, input dst_rdy_i);
   
   reg 		 shutoff_int, active;
   wire active_next = (src_rdy_i & dst_rdy_o)? ~data_i[33] : active;

   assign data_o = data_i;

   assign dst_rdy_o = shutoff_int ? 1'b1 : dst_rdy_i;
   assign src_rdy_o = shutoff_int ? 1'b0 : src_rdy_i;
   
   always @(posedge clk)
     if(reset | clear)
       active <= 0;
     else
       active <= active_next;
   
   always @(posedge clk)
     if(reset | clear)
       shutoff_int <= 0;
     else if(~active_next)
       shutoff_int <= shutoff;
   
endmodule // valve36
