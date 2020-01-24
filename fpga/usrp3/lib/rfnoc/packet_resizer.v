//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Not necessarily that useful in general, but a good test block

module packet_resizer
  #(parameter SR_PKT_SIZE=1)
   (input clk, input reset,
    input [15:0] next_dst_sid,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [31:0] i_tdata, input [127:0] i_tuser, input i_tlast, input i_tvalid, output i_tready,
    output [31:0] o_tdata, output [127:0] o_tuser, output o_tlast, output o_tvalid, input o_tready);

   wire [15:0] 	  pkt_size;
   reg [15:0] 	  count;
   reg 		  first_packet_in_burst = 1'b1;
   
   setting_reg #(.my_addr(SR_PKT_SIZE), .width(16)) reg_pkt_size
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(pkt_size));

   // Parse i_tuser
   wire [1:0] 	  TYPE_in = i_tuser[127:126];
   wire 	  TSI_in = i_tuser[125];
   wire 	  EOB_in = i_tuser[124];
   wire [11:0] 	  SEQ_in = i_tuser[123:112];
   wire [15:0] 	  LEN_in = i_tuser[111:96];
   wire [15:0] 	  SRC_in = i_tuser[95:80];
   wire [15:0] 	  DST_in = i_tuser[79:64];
   wire [63:0] 	  TIME_in = i_tuser[63:0];
   
   // Generate o_tuser
   wire [1:0] 	  TYPE_out = TYPE_in;
   wire 	  TSI_out = TSI_in & first_packet_in_burst;
   wire 	  EOB_out = EOB_in & i_tlast;
   wire [11:0] 	  SEQ_out = SEQ_in;     // Doesn't actually matter, it gets overwritten by chdr_framer
   wire [15:0] 	  LEN_out = LEN_in;     // Only the bottom 2 bits actually matter, rest gets overwritten
   wire [15:0] 	  SRC_out = DST_in;
   wire [15:0] 	  DST_out = next_dst_sid;
   wire [63:0] 	  TIME_out = TIME_in;

   // Pass nearly everything through unchanged
   assign o_tdata = i_tdata;
   assign o_tlast = (count == pkt_size) | EOB_out;   
   assign o_tuser = { TYPE_out, TSI_out, EOB_out, SEQ_out, LEN_out, SRC_out, DST_out, TIME_out };

   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready;

   always @(posedge clk)
     if(reset)
       count <= 16'd4;
     else
       if(o_tvalid & o_tready)
	 if(o_tlast)
	   count <= 16'd4;
	 else
	   count <= count + 16'd4;

   always @(posedge clk)
     if(reset)
       first_packet_in_burst <= 1'b1;
     else
       if(o_tvalid & o_tready & o_tlast)
	 first_packet_in_burst <= EOB_out;
   
endmodule // packet_resizer
