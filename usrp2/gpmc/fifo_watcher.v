

module fifo_watcher
  (input clk, input reset, input clear,
   input src_rdy, input dst_rdy, input sof, input eof,
   output have_packet, output [15:0] length, input next);

   wire   write = src_rdy & dst_rdy & eof;
   
   fifo_short #(.WIDTH(16)) frame_lengths
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(counter), .src_rdy_i(write), .dst_rdy_o(),
      .dataout(length), .src_rdy_o(have_packet), .dst_rdy_i(next) );

   reg [15:0] counter;
   always @(posedge clk)
     if(reset | clear)
       counter <= 1;   // Start at 1
     else if(src_rdy & dst_rdy)
       if(eof)
	 counter <= 1;
       else
	 counter <= counter + 1;
   

endmodule // fifo_watcher
