//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// NOTE: This module uses Xilinx IP that is not available in Spartan3 and older FPGA's

// Final halfband decimator
// Implements impulse responses of the form [A 0 B 0 C .. 0 H 0.5 H 0 .. C 0 B 0 A]
// Strobe in cannot come faster than every 2nd clock cycle
// These taps designed by halfgen4 from ldoolittle
// myfilt = round(2^18 * halfgen4(.7/4,8))

module hb_dec
  #(parameter WIDTH=24,
    parameter DEVICE = "SPARTAN6")
    (input clk,
     input rst,
     input bypass,
     input run,
     input [8:0] cpi,  // Clocks per input -- equal to the decimation ratio ahead of this block
     input stb_in,
     input [WIDTH-1:0] data_in,
     output reg stb_out,
     output reg [WIDTH-1:0] data_out);

   localparam INTWIDTH = 17;
   localparam ACCWIDTH = WIDTH + 3;

   // Round off inputs to 17 bits because of 18 bit multipliers
   wire [INTWIDTH-1:0] 	     data_rnd;
   wire 		     stb_rnd;

   round_sd #(.WIDTH_IN(WIDTH),.WIDTH_OUT(INTWIDTH)) round_in
     (.clk(clk),.reset(rst),.in(data_in),.strobe_in(stb_in),.out(data_rnd),.strobe_out(stb_rnd));

   // Control
   reg [3:0] 		addr_odd_a, addr_odd_b, addr_odd_c, addr_odd_d;
   wire 		write_odd, write_even, do_mult;
   reg 			odd;
   reg [2:0] 		phase, phase_d1;
   reg 			stb_out_int;
   wire 		clear, do_acc;
   assign 		do_mult = 1;

   always @(posedge clk)
     if(rst | ~run)
       odd <= 0;
     else if(stb_rnd)
       odd <= ~odd;

   assign 		write_odd = stb_rnd & odd;
   assign 		write_even = stb_rnd & ~odd;

   always @(posedge clk)
     if(rst | ~run)
       phase <= 0;
     else if(stb_rnd & odd)
       phase <= 1;
     else if(phase == 4)
       phase <= 0;
     else if(phase != 0)
       phase <= phase + 1;

   always @(posedge clk)
     phase_d1 <= phase;

   reg [15:0] 		stb_out_pre;
   always @(posedge clk)
     if(rst)
       stb_out_pre <= 0;
     else
       stb_out_pre <= {stb_out_pre[14:0],(stb_rnd & odd)};

   always @*
     case(phase)
       1 : begin addr_odd_a = 0; addr_odd_b = 15; end
       2 : begin addr_odd_a = 1; addr_odd_b = 14; end
       3 : begin addr_odd_a = 2; addr_odd_b = 13; end
       4 : begin addr_odd_a = 3; addr_odd_b = 12; end
       default : begin addr_odd_a = 0; addr_odd_b = 15; end
     endcase // case(phase)

   always @*
     case(phase)
       1 : begin addr_odd_c = 4; addr_odd_d = 11; end
       2 : begin addr_odd_c = 5; addr_odd_d = 10; end
       3 : begin addr_odd_c = 6; addr_odd_d = 9; end
       4 : begin addr_odd_c = 7; addr_odd_d = 8; end
       default : begin addr_odd_c = 4; addr_odd_d = 11; end
     endcase // case(phase)

   assign do_acc = |stb_out_pre[6:3];
   assign clear = stb_out_pre[3];

   // Data
   wire [INTWIDTH-1:0] data_odd_a, data_odd_b, data_odd_c, data_odd_d;
   reg [INTWIDTH:0]    sum1, sum2;    // these are 18-bit inputs to mult
   reg [WIDTH:0]       final_sum;
   wire [WIDTH-1:0]    final_sum_clip;
   reg [17:0] 	       coeff1, coeff2;
   wire [35:0] 	       prod1, prod2;

   always @*            // Outer coeffs
     case(phase_d1)
       1 : coeff1 = -107;
       2 : coeff1 = 445;
       3 : coeff1 = -1271;
       4 : coeff1 = 2959;
       default : coeff1 = -107;
     endcase // case(phase)

   always @*            //  Inner coeffs
     case(phase_d1)
       1 : coeff2 = -6107;
       2 : coeff2 = 11953;
       3 : coeff2 = -24706;
       4 : coeff2 = 82359;
       default : coeff2 = -6107;
     endcase // case(phase)

   srl #(.WIDTH(INTWIDTH)) srl_odd_a
     (.clk(clk),.rst(rst),.write(write_odd),.in(data_rnd),.addr(addr_odd_a),.out(data_odd_a));
   srl #(.WIDTH(INTWIDTH)) srl_odd_b
     (.clk(clk),.rst(rst),.write(write_odd),.in(data_rnd),.addr(addr_odd_b),.out(data_odd_b));
   srl #(.WIDTH(INTWIDTH)) srl_odd_c
     (.clk(clk),.rst(rst),.write(write_odd),.in(data_rnd),.addr(addr_odd_c),.out(data_odd_c));
   srl #(.WIDTH(INTWIDTH)) srl_odd_d
     (.clk(clk),.rst(rst),.write(write_odd),.in(data_rnd),.addr(addr_odd_d),.out(data_odd_d));

   always @(posedge clk) sum1 <= {data_odd_a[INTWIDTH-1],data_odd_a} + {data_odd_b[INTWIDTH-1],data_odd_b};
   always @(posedge clk) sum2 <= {data_odd_c[INTWIDTH-1],data_odd_c} + {data_odd_d[INTWIDTH-1],data_odd_d};

   wire [INTWIDTH-1:0] data_even;
   reg [3:0] 	     addr_even;

   always @(posedge clk)
     case(cpi)
       //  1 is an error
       2 : addr_even <= 9;  // Maximum speed (overall decim by 4)
       3, 4, 5, 6, 7 : addr_even <= 8;
       default : addr_even <= 7;
     endcase // case(cpi)

   srl #(.WIDTH(INTWIDTH)) srl_even
     (.clk(clk),.rst(rst),.write(write_even),.in(data_rnd),.addr(addr_even),.out(data_even));


    MULT_MACRO #(.DEVICE(DEVICE),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(18),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
      mult1 (.P(prod1),     // Multiplier output bus, width determined by WIDTH_P parameter
	       .A(coeff1),     // Multiplier input A bus, width determined by WIDTH_A parameter
	       .B(sum1),                    // Multiplier input B bus, width determined by WIDTH_B parameter
	       .CE(do_mult),   // 1-bit active high input clock enable
	       .CLK(clk),              // 1-bit positive edge clock input
	       .RST(rst));             // 1-bit input active high reset

    MULT_MACRO #(.DEVICE(DEVICE),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(18),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
      mult2 (.P(prod2),     // Multiplier output bus, width determined by WIDTH_P parameter
	       .A(coeff2),     // Multiplier input A bus, width determined by WIDTH_A parameter
	       .B(sum2),                    // Multiplier input B bus, width determined by WIDTH_B parameter
	       .CE(do_mult),   // 1-bit active high input clock enable
	       .CLK(clk),              // 1-bit positive edge clock input
	       .RST(rst));             // 1-bit input active high reset


   reg [35:0] 	     sum_of_prod;
   always @(posedge clk) sum_of_prod <= prod1 + prod2;   // Can't overflow

   wire [ACCWIDTH-1:0] 	acc_out;
   acc #(.IWIDTH(ACCWIDTH-2),.OWIDTH(ACCWIDTH))
     acc (.clk(clk),.clear(clear),.acc(do_acc),.in(sum_of_prod[35:38-ACCWIDTH]),.out(acc_out));

   wire [ACCWIDTH-1:0] 	data_even_signext;

   localparam SHIFT_FACTOR = 6;

   sign_extend #(.bits_in(INTWIDTH),.bits_out(ACCWIDTH-SHIFT_FACTOR)) signext_data_even
     (.in(data_even),.out(data_even_signext[ACCWIDTH-1:SHIFT_FACTOR]));
   assign 		data_even_signext[SHIFT_FACTOR-1:0] = 0;

   always @(posedge clk) final_sum <= acc_out + data_even_signext;

   clip #(.bits_in(WIDTH+1), .bits_out(WIDTH)) clip (.in(final_sum), .out(final_sum_clip));

   // Output MUX to allow for bypass
   wire 		selected_stb = bypass ? stb_in : stb_out_pre[8];

   always @(posedge clk)
     begin
	stb_out  <= selected_stb;
	if(selected_stb)
	  data_out <= bypass ? data_in : final_sum_clip;
     end

endmodule // hb_dec
