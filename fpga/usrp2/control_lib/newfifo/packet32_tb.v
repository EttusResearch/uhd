

module packet32_tb();

   wire [35:0] data;
   wire       src_rdy, dst_rdy;

   wire       clear = 0;
   reg 	      clk = 0;
   reg 	      reset = 1;

   always #10 clk <= ~clk;
   initial #1000 reset <= 0;

   initial $dumpfile("packet32_tb.vcd");
   initial $dumpvars(0,packet32_tb);

   wire [31:0] total, crc_err, seq_err, len_err;
   
   packet_generator32 pkt_gen (.clk(clk), .reset(reset), .clear(clear),
			       .data_o(data), .src_rdy_o(src_rdy), .dst_rdy_i(dst_rdy));

   packet_verifier32 pkt_ver (.clk(clk), .reset(reset), .clear(clear),
			      .data_i(data), .src_rdy_i(src_rdy), .dst_rdy_o(dst_rdy),
			      .total(total), .crc_err(crc_err), .seq_err(seq_err), .len_err(len_err));

endmodule // packet32_tb
