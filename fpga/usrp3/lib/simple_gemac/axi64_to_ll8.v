//
// Copyright 2014 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module axi64_to_ll8
  #(parameter START_BYTE=6)
   (input clk, input reset, input clear,
    input [63:0] axi64_tdata, input axi64_tlast, input [3:0] axi64_tuser, input axi64_tvalid, output axi64_tready,
    output [7:0] ll_data, output ll_eof, output ll_src_rdy, input ll_dst_rdy);

   reg [7:0] 	 data_int;
   wire 	 eof_int, valid_int, ready_int;

   reg [2:0] 	 state = START_BYTE;
   reg 		 eof, done;
   reg [3:0] 	 occ;

   wire [63:0] axi64_tdata_gated;
   wire        axi64_tlast_gated;
   wire [3:0]  axi64_tuser_gated;
   wire        axi64_tvalid_gated;
   wire        axi64_tready_gated;

   axi_packet_gate #(
     .WIDTH(68),
     .SIZE(10)
   ) axi64_packet_gater (
     .clk(clk), .reset(reset), .clear(clear),
     .i_tdata({axi64_tuser, axi64_tdata}), .i_tlast(axi64_tlast),
     .i_tvalid(axi64_tvalid), .i_tready(axi64_tready),
     .o_tdata({axi64_tuser_gated, axi64_tdata_gated}), .o_tlast(axi64_tlast_gated),
     .o_tvalid(axi64_tvalid_gated), .o_tready(axi64_tready_gated)
   );

   always @(posedge clk)
     if(reset | clear)
       state <= START_BYTE;
     else
       if(valid_int & ready_int)
	 if(eof_int)
	   state <= START_BYTE;
	 else
	   state <= state + 3'd1;

   assign valid_int = axi64_tvalid_gated;
   assign axi64_tready_gated = ready_int & (eof_int | state == 7);
   assign eof_int = axi64_tlast_gated & (axi64_tuser_gated[2:0] == (state + 3'd1));

   always @*
     data_int <= axi64_tdata_gated[state*8 +: 8];

   axi_fifo_short #(.WIDTH(9)) ll8_fifo
     (.clk(clk), .reset(reset), .clear(0),
      .i_tdata({eof_int, data_int}), .i_tvalid(valid_int), .i_tready(ready_int),
      .o_tdata({ll_eof, ll_data}), .o_tvalid(ll_src_rdy), .o_tready(ll_dst_rdy),
      .space(), .occupied());

endmodule // axi64_to_ll8
