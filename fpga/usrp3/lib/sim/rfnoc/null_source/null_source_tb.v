//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module null_source_tb();
   
   reg clk, reset;
   always
     #100 clk = ~clk;

   initial clk = 0;
   initial reset = 1;
   initial #1000 reset = 0;
   
   initial $dumpfile("null_source_tb.vcd");
   initial $dumpvars(0,null_source_tb);

   //initial #10000000 $finish;

   reg [31:0] set_data;
   reg [7:0]  set_addr;
   reg        set_stb=0;

   wire [63:0] src_tdata;
   wire	       src_tlast, src_tvalid;
   wire        src_tready;

   assign src_tready = 1'b1;
   
   localparam PORTS = 4;

   null_source #(.BASE(0)) axi_wrapper_ce1
     (.clk(clk), .reset(reset),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .o_tdata(src_tdata), .o_tlast(src_tlast), .o_tvalid(src_tvalid), .o_tready(src_tready)     );

   initial
     begin

	@(negedge reset);
	@(posedge clk);
	#100000;
	@(posedge clk);
	set_stb <= 1;
	set_addr <= 0;
	set_data <= 32'hDEADBEEF;
	@(posedge clk);
	set_stb <= 1;
	set_addr <= 1;  // Len
	set_data <= 32'h8;
	@(posedge clk);
	set_stb <= 1;
	set_addr <= 2;
	set_data <= 32'h20; // Rate
	@(posedge clk);
	set_stb <= 1;
	set_addr <= 3;
	set_data <= 1;     // enable
	@(posedge clk);
	set_stb <= 0;
	@(posedge clk);
	#1000000;
	@(posedge clk);
	set_stb <= 1;
	set_addr <= 3;
	set_data <= 0;     // disable
	@(posedge clk);
	set_stb <= 0;
	#1000000;

	$finish;
	
	
     end

endmodule // null_source_tb
