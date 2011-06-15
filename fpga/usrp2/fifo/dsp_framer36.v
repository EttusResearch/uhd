//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


// This has 3 functions:
//   Correct the VITA packet length
//   [optional] Frame DSP packets with an header line to be handled by the protocol machine
//   Hold on to the packet until there is a complete one before allowing to leave

module dsp_framer36
  #(parameter BUF_SIZE = 9, 
    parameter PORT_SEL = 0,
    parameter PROT_ENG_FLAGS = 1)
   (input clk, input reset, input clear,
    input [35:0] data_i, input src_rdy_i, output dst_rdy_o,
    output [35:0] data_o, output src_rdy_o, input dst_rdy_i);

   wire 	  dfifo_in_dst_rdy, dfifo_in_src_rdy, dfifo_out_dst_rdy, dfifo_out_src_rdy;
   wire 	  tfifo_in_dst_rdy, tfifo_in_src_rdy, tfifo_out_dst_rdy, tfifo_out_src_rdy;

   wire 	  do_xfer_in = dfifo_in_src_rdy & dfifo_in_dst_rdy;
   wire 	  do_xfer_out = src_rdy_o & dst_rdy_i;
   
   wire 	  have_space = dfifo_in_dst_rdy & tfifo_in_dst_rdy;
   reg [15:0] 	  pkt_len_in, pkt_len_out;
   wire [15:0] 	  tfifo_data;
   wire [35:0] 	  dfifo_out_data;
   
   assign dst_rdy_o        = have_space;
   assign dfifo_in_src_rdy = src_rdy_i & have_space;
   
   fifo_cascade #(.WIDTH(36), .SIZE(BUF_SIZE)) dfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_i), .src_rdy_i(dfifo_in_src_rdy), .dst_rdy_o(dfifo_in_dst_rdy),
      .dataout(dfifo_out_data), .src_rdy_o(dfifo_out_src_rdy),  .dst_rdy_i(dfifo_out_dst_rdy) );

   fifo_short #(.WIDTH(16)) tfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(pkt_len_in),  .src_rdy_i(tfifo_in_src_rdy), .dst_rdy_o(tfifo_in_dst_rdy),
      .dataout(tfifo_data), .src_rdy_o(tfifo_out_src_rdy), .dst_rdy_i(tfifo_out_dst_rdy),
      .space(), .occupied() );

   // FIXME won't handle single-line packets, will show wrong length
   always @(posedge clk)
     if(reset | clear)
       pkt_len_in <= 0;
     else if(do_xfer_in)
       if(data_i[32])   // sof
	 pkt_len_in <= 2;  // fixes off by one since number is stored before increment
       else
	 pkt_len_in <= pkt_len_in + 1;

   assign tfifo_in_src_rdy = do_xfer_in & data_i[33]; // store length when at eof in
   assign tfifo_out_dst_rdy = do_xfer_out & data_o[33]; // remove length from list at eof out

   always @(posedge clk)
     if(reset | clear)
       pkt_len_out <= (PROT_ENG_FLAGS ? 1'b0 : 1'b1);
     else if(do_xfer_out)
       if(dfifo_out_data[33]) // eof
	 pkt_len_out <= (PROT_ENG_FLAGS ? 1'b0 : 1'b1);
       else
	 pkt_len_out <= pkt_len_out + 1;
   
   assign dfifo_out_dst_rdy = do_xfer_out & (pkt_len_out != 0);

   wire [1:0] 	  port_sel_bits = PORT_SEL;
   
   assign data_o = (pkt_len_out == 0) ? {3'b000, 1'b1, 13'b0, port_sel_bits, 1'b1, tfifo_data[13:0],2'b00} :
		   (pkt_len_out == 1) ? {3'b000, (PROT_ENG_FLAGS ? 1'b0: 1'b1), dfifo_out_data[31:16],tfifo_data} : 
		   {dfifo_out_data[35:33], 1'b0, dfifo_out_data[31:0] };

   assign src_rdy_o = dfifo_out_src_rdy & tfifo_out_src_rdy;
   
endmodule // dsp_framer36
