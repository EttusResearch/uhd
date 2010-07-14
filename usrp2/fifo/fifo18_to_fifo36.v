
// For now just assume FIFO18 is same as FIFO19 without occupancy bit

module fifo18_to_fifo36
  (input clk, input reset, input clear,
   input [17:0] f18_datain,
   input f18_src_rdy_i,
   output f18_dst_rdy_o,

   output [35:0] f36_dataout,
   output f36_src_rdy_o,
   input f36_dst_rdy_i
   );

   fifo19_to_fifo36 fifo19_to_fifo36
     (.clk(clk), .reset(reset), .clear(clear),
      .f19_datain({1'b0,f18_datain}), .f19_src_rdy_i(f18_src_rdy_i), .f19_dst_rdy_o(f18_dst_rdy_o),
      .f36_dataout(f36_dataout), .f36_src_rdy_o(f36_src_rdy_o), .f36_dst_rdy_i(f36_dst_rdy_i) );

endmodule // fifo18_to_fifo36
