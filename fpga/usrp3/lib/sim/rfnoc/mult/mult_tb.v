//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module mult_tb();
   
   xlnx_glbl glbl (.GSR(),.GTS());
   
   localparam STR_SINK_FIFOSIZE = 9;
      
   reg clk, reset;
   always
     #100 clk = ~clk;

   initial clk = 0;
   initial reset = 1;
   initial #1000 reset = 0;
   
   initial $dumpfile("mult_tb.vcd");
   initial $dumpvars(0,mult_tb);

   initial #1000000 $finish;

   localparam AW=25;
   localparam BW=18;
   localparam PW=48;
   
   wire [AW-1:0] a_tdata;
   wire [BW-1:0] b_tdata;
   wire [PW-1:0] p_tdata;
   
   wire 	 a_tlast, a_tvalid, a_tready, b_tlast, b_tvalid, b_tready;
   wire 	 p_tlast, p_tvalid;
   reg 		 p_tready = 0;

   reg 		 ai_tvalid = 0;
   reg 		 bi_tvalid = 0;
   
   counter #(.WIDTH(AW)) ca
     (.clk(clk), .reset(reset), .clear(0),
      .max(20),
      .i_tlast(0), .i_tvalid(ai_tvalid), .i_tready(),
      .o_tdata(a_tdata), .o_tlast(a_tlast), .o_tvalid(a_tvalid), .o_tready(a_tready));
   
   counter #(.WIDTH(BW)) cb
     (.clk(clk), .reset(reset), .clear(0),
      .max(20),
      .i_tlast(0), .i_tvalid(bi_tvalid), .i_tready(),
      .o_tdata(b_tdata), .o_tlast(b_tlast), .o_tvalid(b_tvalid), .o_tready(b_tready));
   
   mult #(.WIDTH_A(AW), .WIDTH_B(BW), .WIDTH_P(PW), .LATENCY(4)) mult
     (.clk(clk), .reset(reset),
      .a_tdata(a_tdata), .a_tlast(a_tlast), .a_tvalid(a_tvalid), .a_tready(a_tready),
      .b_tdata(b_tdata), .b_tlast(b_tlast), .b_tvalid(b_tvalid), .b_tready(b_tready),
      .p_tdata(p_tdata), .p_tlast(p_tlast), .p_tvalid(p_tvalid), .p_tready(p_tready));

   initial
     begin
	@(negedge reset);
	repeat (100)
	  @(posedge clk);
	ai_tvalid <= 1;
	repeat (10)
	  @(posedge clk);
	bi_tvalid <= 1;
	repeat (10)
	  @(posedge clk);
	p_tready <= 1;
     end
      
endmodule // mult_tb
