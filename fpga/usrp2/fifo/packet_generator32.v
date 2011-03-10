

module packet_generator32
  (input clk, input reset, input clear,
   input [127:0] header,
   output [35:0] data_o, output src_rdy_o, input dst_rdy_i);

   wire [7:0] 	     ll_data;
   wire 	     ll_sof, ll_eof, ll_src_rdy, ll_dst_rdy_n;
   
   packet_generator pkt_gen
     (.clk(clk), .reset(reset), .clear(clear),
      .data_o(ll_data), .sof_o(ll_sof), .eof_o(ll_eof),
      .header(header),
      .src_rdy_o(ll_src_rdy), .dst_rdy_i(~ll_dst_rdy_n));

   ll8_to_fifo36 ll8_to_f36
     (.clk(clk), .reset(reset), .clear(clear),
      .ll_data(ll_data), .ll_sof_n(~ll_sof), .ll_eof_n(~ll_eof),
      .ll_src_rdy_n(~ll_src_rdy), .ll_dst_rdy_n(ll_dst_rdy_n),
      .f36_data(data_o), .f36_src_rdy_o(src_rdy_o), .f36_dst_rdy_i(dst_rdy_i));
   
endmodule // packet_generator32
