

module crossbar36
  (input clk, input reset, input clear,
   input cross,
   input [35:0] data0_i, input src0_rdy_i, output dst0_rdy_o,
   input [35:0] data1_i, input src1_rdy_i, output dst1_rdy_o,
   output [35:0] data0_o, output src0_rdy_o, input dst0_rdy_i,
   output [35:0] data1_o, output src1_rdy_o, input dst1_rdy_i);

   reg 		 cross_int, active0, active1;
   wire active0_next = (src0_rdy_i & dst0_rdy_o)? ~data0_i[33] : active0;
   wire active1_next = (src1_rdy_i & dst1_rdy_o)? ~data1_i[33] : active1;

   assign data0_o = cross_int ? data1_i : data0_i;
   assign data1_o = cross_int ? data0_i : data1_i;

   assign src0_rdy_o = cross_int ? src1_rdy_i : src0_rdy_i;
   assign src1_rdy_o = cross_int ? src0_rdy_i : src1_rdy_i;

   assign dst0_rdy_o = cross_int ? dst1_rdy_i : dst0_rdy_i;
   assign dst1_rdy_o = cross_int ? dst0_rdy_i : dst1_rdy_i;
   
   always @(posedge clk)
     if(reset | clear)
       active0 <= 0;
     else
       active0 <= active0_next;
   
   always @(posedge clk)
     if(reset | clear)
       active1 <= 0;
     else
       active1 <= active1_next;

   always @(posedge clk)
     if(reset | clear)
       cross_int <= 0;
     else if(~active0_next & ~active1_next)
       cross_int <= cross;
   
endmodule // crossbar36
