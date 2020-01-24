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

// Final halfband decimator 
// Implements impulse responses of the form [A 0 B 0 C .. 0 H 0.5 H 0 .. C 0 B 0 A]
// Strobe in cannot come faster than every 2nd clock cycle
// These taps designed by halfgen4 from ldoolittle
// myfilt = round(2^18 * halfgen4(.7/4,8))

module hb_dec
  #(parameter WIDTH=24)
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
   localparam ACCWIDTH = 30;
   
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
     (.clk(clk),.write(write_odd),.in(data_rnd),.addr(addr_odd_a),.out(data_odd_a));
   srl #(.WIDTH(INTWIDTH)) srl_odd_b
     (.clk(clk),.write(write_odd),.in(data_rnd),.addr(addr_odd_b),.out(data_odd_b));
   srl #(.WIDTH(INTWIDTH)) srl_odd_c
     (.clk(clk),.write(write_odd),.in(data_rnd),.addr(addr_odd_c),.out(data_odd_c));
   srl #(.WIDTH(INTWIDTH)) srl_odd_d
     (.clk(clk),.write(write_odd),.in(data_rnd),.addr(addr_odd_d),.out(data_odd_d));

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
     (.clk(clk),.write(write_even),.in(data_rnd),.addr(addr_even),.out(data_even));

   MULT18X18S mult1(.C(clk), .CE(do_mult), .R(rst), .P(prod1), .A(coeff1), .B(sum1) );
   MULT18X18S mult2(.C(clk), .CE(do_mult), .R(rst), .P(prod2), .A(coeff2), .B(sum2) );

   reg [35:0] 	     sum_of_prod;
   always @(posedge clk) sum_of_prod <= prod1 + prod2;   // Can't overflow

   wire [35:0]  acc_out;
   acc #(.IWIDTH(36),.OWIDTH(36))
     acc (.clk(clk),.clear(clear),.acc(do_acc),.in(sum_of_prod),.out(acc_out));

   wire [WIDTH:0] acc_out_rnd;
   round #(.bits_in(36),.bits_out(WIDTH+1)) round_acc
     (.in(acc_out), .out(acc_out_rnd));

   wire [WIDTH:0]     data_even_signext;

   localparam SHIFT_FACTOR = 17 - (36 - (WIDTH+1));

   sign_extend #(.bits_in(INTWIDTH),.bits_out(WIDTH+1-SHIFT_FACTOR)) signext_data_even
     (.in(data_even),.out(data_even_signext[WIDTH:SHIFT_FACTOR]));
   assign       data_even_signext[SHIFT_FACTOR-1:0] = 0;

   always @(posedge clk) final_sum <= acc_out_rnd + data_even_signext;

   clip #(.bits_in(WIDTH+1),.bits_out(WIDTH)) clip_finalsum
     (.in(final_sum), .out(final_sum_clip));

   // Output MUX to allow for bypass
   wire 		selected_stb = bypass ? stb_in : stb_out_pre[8];
   
   always @(posedge clk)
     begin
	stb_out  <= selected_stb;
	if(selected_stb)
	  data_out <= bypass ? data_in : final_sum_clip;
     end
   
endmodule // hb_dec
