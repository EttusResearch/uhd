//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

//`timescale 1ns
module chdr_framer_tb();
   
   reg clk, reset;
   always
     #100 clk = ~clk;

   initial $dumpfile("chdr_framer_tb.vcd");
   initial $dumpvars(0,chdr_framer_tb);

   reg [31:0] i_tdata;
   reg [127:0] i_tuser;
   reg i_tlast, i_tvalid;
   wire i_tready;

   wire [63:0] o_tdata;
   wire        o_tlast, o_tvalid;

   reg 	       o_tready = 0;
   
   chdr_framer #(.SIZE(10)) chdr_framer
     (.clk(clk), .reset(reset), .clear(0),
      .i_tdata(i_tdata), .i_tuser(i_tuser), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

   always 
     begin
	#1 o_tready = 1;
	repeat (200)
	  @(posedge clk);
	#1 o_tready = 0;
	repeat (120)
	  @(posedge clk);
     end
		   
	    
   initial
     begin
	clk = 0;
	reset = 1;
	i_tlast = 0;
	i_tvalid = 0;
	i_tdata = 32'hBEEF_0000;
	i_tuser = 128'hF123_4567_89ab_cdef_0011_2233_4455_0000;
	#1000 reset = 0;
	repeat (10)
	  @(posedge clk);
	#1 i_tvalid = 1;
	repeat (400)
	  begin
	     #1 i_tlast = 0;
	     repeat (22)
	       begin
		  #1 i_tdata = i_tdata + 1;
		  @(posedge clk);
	       end
	     #1 i_tdata = i_tdata + 1;
	     #1 i_tlast = 1;
	     @(posedge clk);
	     #1 i_tuser <= i_tuser + 1;
	  end // repeat (20)
	#1 i_tvalid <= 0;
	#100000 $finish;
     end

   always @(posedge clk)
     if(o_tvalid & o_tready)
       if(o_tlast)
	 $display("%x    LAST",o_tdata);
       else
	 $display("%x",o_tdata);
   
endmodule // chdr_framer_tb
