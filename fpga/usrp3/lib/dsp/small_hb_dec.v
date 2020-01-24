//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// NOTE: This module uses Xilinx IP that is not available in Spartan3 and older FPGA's

// Short halfband decimator (intended to be followed by another stage)
// Implements impulse responses of the form [A 0 B 0.5 B 0 A]
//
// These taps designed by halfgen4 from ldoolittle:
//   2 * 131072 * halfgen4(.75/8,2)
module small_hb_dec
  #(parameter WIDTH=18,
    parameter DEVICE = "SPARTAN6")
    (input clk,
     input rst,
     input bypass,
     input run,
     input stb_in,
     input [WIDTH-1:0] data_in,
     output reg stb_out,
     output reg [WIDTH-1:0] data_out);

   // Round off inputs to 17 bits because of 18 bit multipliers
   localparam INTWIDTH = 17;
   wire [INTWIDTH-1:0] 	data_rnd;
   wire 		stb_rnd;

   round_sd #(.WIDTH_IN(WIDTH),.WIDTH_OUT(INTWIDTH)) round_in
     (.clk(clk),.reset(rst),.in(data_in),.strobe_in(stb_in),.out(data_rnd),.strobe_out(stb_rnd));


   reg 			stb_rnd_d1;
   reg [INTWIDTH-1:0] 	data_rnd_d1;
   always @(posedge clk) stb_rnd_d1 <= stb_rnd;
   always @(posedge clk) data_rnd_d1 <= data_rnd;

   wire 		go;
   reg 			phase, go_d1, go_d2, go_d3, go_d4;
   always @(posedge clk)
     if(rst | ~run)
       phase <= 0;
     else if(stb_rnd_d1)
       phase <= ~phase;
   assign 		go = stb_rnd_d1 & phase;
   always @(posedge clk)
     if(rst | ~run)
       begin
	  go_d1 <= 0;
	  go_d2 <= 0;
	  go_d3 <= 0;
	  go_d4 <= 0;
       end
     else
       begin
	  go_d1 <= go;
	  go_d2 <= go_d1;
	  go_d3 <= go_d2;
	  go_d4 <= go_d3;
       end

   wire [17:0] 		coeff_a = -10690;
   wire [17:0] 		coeff_b = 75809;

   reg [INTWIDTH-1:0] 	d1, d2, d3, d4 , d5, d6;
   always @(posedge clk)
     if(stb_rnd_d1 | rst)
       begin
	  d1 <= data_rnd_d1;
	  d2 <= d1;
	  d3 <= d2;
	  d4 <= d3;
	  d5 <= d4;
	  d6 <= d5;
       end

   reg [17:0] sum_a, sum_b, middle, middle_d1;

   always @(posedge clk)
     if(go)
       begin
	  sum_a <= {data_rnd_d1[INTWIDTH-1],data_rnd_d1} + {d6[INTWIDTH-1],d6};
	  sum_b <= {d2[INTWIDTH-1],d2} + {d4[INTWIDTH-1],d4};
	  //middle <= {d3[INTWIDTH-1],d3};
	  middle <= {d3,1'b0};
       end

   always @(posedge clk)
     if(go_d1)
       middle_d1 <= middle;

   wire [17:0] sum = go_d1 ? sum_b : sum_a;
   wire [17:0] coeff = go_d1 ? coeff_b : coeff_a;
   wire [35:0] 	 prod;

    MULT_MACRO #(.DEVICE(DEVICE),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(18),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
      mult (.P(prod),     // Multiplier output bus, width determined by WIDTH_P parameter
	       .A(coeff),     // Multiplier input A bus, width determined by WIDTH_A parameter
	       .B(sum),                    // Multiplier input B bus, width determined by WIDTH_B parameter
	       .CE(go_d1 | go_d2),   // 1-bit active high input clock enable
	       .CLK(clk),              // 1-bit positive edge clock input
	       .RST(rst));             // 1-bit input active high reset

   localparam ACCWIDTH = 30;
   reg [ACCWIDTH-1:0] 	 accum;

   always @(posedge clk)
     if(rst)
       accum <= 0;
     else if(go_d2)
       accum <= {middle_d1[17],middle_d1[17],middle_d1,{(16+ACCWIDTH-36){1'b0}}} + {prod[35:36-ACCWIDTH]};
     else if(go_d3)
       accum <= accum + {prod[35:36-ACCWIDTH]};

   wire [WIDTH:0] 	 accum_rnd;
   wire [WIDTH-1:0] 	 accum_rnd_clip;

   wire 	 stb_round;

   round_sd #(.WIDTH_IN(ACCWIDTH),.WIDTH_OUT(WIDTH+1)) round_acc
     (.clk(clk), .reset(rst), .in(accum), .strobe_in(go_d4), .out(accum_rnd), .strobe_out(stb_round));

   clip #(.bits_in(WIDTH+1),.bits_out(WIDTH)) clip (.in(accum_rnd), .out(accum_rnd_clip));

   // Output
   always @(posedge clk)
     begin
	stb_out  <= bypass ? stb_in : stb_round;
	data_out <= bypass ? data_in : accum_rnd_clip;
     end


endmodule // small_hb_dec
