

module fifo_pacer
  (input clk,
   input reset,
   input [7:0] rate,
   input enable,
   input src1_rdy_i, output dst1_rdy_o,
   output src2_rdy_o, input dst2_rdy_i,
   output underrun, overrun);

   wire   strobe;
   
   cic_strober strober (.clock(clk), .reset(reset), .enable(enable),
			.rate(rate), .strobe_fast(1), .strobe_slow(strobe));

   wire   all_ready = src1_rdy_i & dst2_rdy_i;
   assign dst1_rdy_o = all_ready & strobe;
   assign src2_rdy_o = dst1_rdy_o;

   assign underrun = strobe & ~src1_rdy_i;
   assign overrun = strobe & ~dst2_rdy_i;
   
endmodule // fifo_pacer
