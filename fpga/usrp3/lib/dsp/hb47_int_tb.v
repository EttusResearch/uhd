//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`timescale 1ns/1ps

module hb47_int_tb();

   wire GSR, GTS;
   glbl glbl( );

   reg 	clk = 0;
   reg 	rst;
   reg 	bypass;
   reg 	run;

   wire stb_in;
   reg 	stb_out;
   reg [7:0] cpo;

   reg [17:0] data_in;

   wire [17:0] data_out_dsp48a; 
   wire [17:0] data_out_dsp48e;
   
   reg [15:0]  freq = 500;

   integer     x = 0, y =0;



   always #100 clk = ~clk;

   // SPARTAN6 / DSP48A
    hb47_int
      #(.WIDTH(18),
	.DEVICE("SPARTAN6"))
	hb47_int_i0
	(
	 .clk(clk),
	 .rst(rst),
	 .bypass(bypass),
	 .stb_in(stb_in),
	 .data_in(data_in),
	 .output_rate(cpo),
	 .stb_out(stb_out),
	 .data_out(data_out_dsp48a)
	 );

   // SERIES7 / DSP48E
    hb47_int
      #(.WIDTH(18),
	.DEVICE("7SERIES"))
	hb47_int_i1
	(
	 .clk(clk),
	 .rst(rst),
	 .bypass(bypass),
	 .stb_in(stb_in),
	 .data_in(data_in),
	 .output_rate(cpo),
	 .stb_out(stb_out),
	 .data_out(data_out_dsp48e)
	 );

   always @(negedge clk)
     if (data_out_dsp48a !== data_out_dsp48e)
       $display("Output missmatch at %t",$time);
   

   
  cic_strober #(.WIDTH(2))
     hb1_strober(.clock(clk),.reset(rst),.enable(run),.rate(2'd2),
		 .strobe_fast(stb_out),.strobe_slow(stb_in) );

   initial
     begin
	rst <= 1;
	bypass <= 0;
	run <= 0;
	data_in <= 0;
	stb_out <= 1;
	cpo <= 1;

	repeat(10) @(posedge clk);
	rst <= 0;
	run <= 1;

	for (x=0; x<100000; x=x+1) begin
	   for (y=0; y<10000; y=y+1)
	     begin
		while (stb_in == 1'b0) @(posedge clk);
		triangle_wave(freq,data_in);
		@(posedge clk);

	     end
	   freq = freq * 2;
	end

	$finish;
     end // initial begin




   reg signed [18:0] phase_acc = 0;
   reg 	      direction = 0;


   task triangle_wave;
      input [15:0] inc;
      output [17:0] data_out;

      begin
	 if (direction) begin
	    phase_acc = phase_acc - inc;
	    if (phase_acc < -19'sh20000) begin
	       direction = 0;
	       phase_acc = phase_acc + inc;
	    end
	 end else begin
	    phase_acc = phase_acc + inc;
	    if (phase_acc > 19'sh1ffff) begin
	       direction = 1;
	       phase_acc = phase_acc - inc;
	    end
	 end
	 data_out = phase_acc[17:0];
      end

   endtask // triangle_wave



endmodule // hb47_int_tb
