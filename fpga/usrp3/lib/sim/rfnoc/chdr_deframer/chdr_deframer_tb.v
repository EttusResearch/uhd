//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

//`timescale 1ns
module chdr_deframer_tb();
   
   reg clk, reset;
   always
     #100 clk = ~clk;

   initial $dumpfile("chdr_deframer_tb.vcd");
   initial $dumpvars(0,chdr_deframer_tb);

   reg [63:0] i_tdata;
   reg i_tlast, i_tvalid;
   wire i_tready;

   wire [31:0] o_tdata;
   wire [127:0] o_tuser;
   wire        o_tlast, o_tvalid;

   reg 	       o_tready = 1;

   wire [63:0] int_tdata;
   wire        int_tlast, int_tvalid, int_tready;
   
   axi_fifo #(.SIZE(10), .WIDTH(65)) fifo
     (.clk(clk), .reset(reset), .clear(0),
      .i_tdata({i_tlast, i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata({int_tlast, int_tdata}), .o_tvalid(int_tvalid), .o_tready(int_tready));
   
   chdr_deframer chdr_deframer
     (.clk(clk), .reset(reset), .clear(0),
      .i_tdata(int_tdata), .i_tlast(int_tlast), .i_tvalid(int_tvalid), .i_tready(int_tready),
      .o_tdata(o_tdata), .o_tuser(o_tuser), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

   reg [63:0]  hdr, vtime, data;
   
   initial
     begin
	clk = 0;
	reset = 1;
	i_tlast = 0;
	i_tvalid = 0;
	hdr = 64'hFF00_AAB9_BEEF_0000;
	vtime = 64'h8888_7777_6666_0000;
	data = 64'hEEEE_0000_FFFF_0001;
	#1000 reset = 0;
	repeat (10)
	  @(posedge clk);
	repeat (6)
	  begin
	     #1 i_tdata = hdr;
	     #1 i_tlast = 0;
	     #1 i_tvalid = 1;
	     @(posedge clk);
	     #1 i_tdata = vtime;
	     @(posedge clk);
	     #1 hdr = hdr + 1;
	     #1 vtime = vtime + 1;
	     repeat (10)
	       begin
		  #1 i_tdata = data;
		  #1 data = data + 64'h0000_0002_0000_0002;
		  @(posedge clk);
	       end
	     #1 i_tdata = data;
	     #1 data = data + 64'h0000_0002_0000_0002;
	     #1 i_tlast <= 1;
	     @(posedge clk);
	  end // repeat (20)
	#1 i_tvalid <= 0;
	#200000 $finish;
     end

   always @(posedge clk)
     if(o_tvalid & o_tready)
       if(o_tlast)
	 $display("%x    LAST",o_tdata);
       else
	 $display("%x",o_tdata);
   
endmodule // chdr_deframer_tb
