

module fifo_watcher
  (input clk, input reset, input clear,
   input src_rdy1, input dst_rdy1, input sof1, input eof1,
   input src_rdy2, input dst_rdy2, input sof2, input eof2,
   output have_packet, output [15:0] length, output reg bus_error);

   wire   write = src_rdy1 & dst_rdy1 & eof1;
   wire   read = src_rdy2 & dst_rdy2 & eof2;
   wire   have_packet_int;
   reg [15:0] counter;
   
   fifo_short #(.WIDTH(16)) frame_lengths
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(counter), .src_rdy_i(write), .dst_rdy_o(),
      .dataout(length), .src_rdy_o(have_packet_int), .dst_rdy_i(read) );

   always @(posedge clk)
     if(reset | clear)
       counter <= 1;   // Start at 1
     else if(src_rdy1 & dst_rdy1)
       if(eof1)
	 counter <= 1;
       else
	 counter <= counter + 1;

   always @(posedge clk)
     if(reset | clear)
       bus_error <= 0;
     else if(dst_rdy2 & ~src_rdy2)
       bus_error <= 1;
     else if(read & ~have_packet_int)
       bus_error <= 1;

   reg 	      in_packet;
   assign have_packet = have_packet_int & ~in_packet;
   
   always @(posedge clk)
     if(reset | clear)
       in_packet <= 0;
     else if(src_rdy2 & dst_rdy2)
       if(eof2)
	 in_packet <= 0;
       else
	 in_packet <= 1;
   
endmodule // fifo_watcher
