

module packet_verifier32
  (input clk, input reset, input clear,
   input [35:0] data_i, input src_rdy_i, output dst_rdy_o,
   output [31:0] total, output [31:0] crc_err, output [31:0] seq_err, output [31:0] len_err);

   wire [7:0] 	     ll_data;
   wire 	     ll_sof_n, ll_eof_n, ll_src_rdy_n, ll_dst_rdy;
   wire [35:0] 	     data_int;
   wire 	     src_rdy_int, dst_rdy_int;
   
   fifo_short #(.WIDTH(36)) fifo_short
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o),
      .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int));
   
   fifo36_to_ll8 f36_to_ll8
     (.clk(clk), .reset(reset), .clear(clear),
      .f36_data(data_int), .f36_src_rdy_i(src_rdy_int), .f36_dst_rdy_o(dst_rdy_int),
      .ll_data(ll_data), .ll_sof_n(ll_sof_n), .ll_eof_n(ll_eof_n),
      .ll_src_rdy_n(ll_src_rdy_n), .ll_dst_rdy_n(~ll_dst_rdy));
   
   packet_verifier pkt_ver
     (.clk(clk), .reset(reset), .clear(clear),
      .data_i(ll_data), .sof_i(~ll_sof_n), .eof_i(~ll_eof_n),
      .src_rdy_i(~ll_src_rdy_n), .dst_rdy_o(ll_dst_rdy),
      .total(total), .crc_err(crc_err), .seq_err(seq_err), .len_err(len_err));

endmodule // packet_verifier32
