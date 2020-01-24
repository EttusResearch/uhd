//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
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

module hb47_int
  #(parameter WIDTH=18,
    parameter DEVICE="7SERIES")
    (input clk,
     input rst,
     input bypass,
     input stb_in,
     input [WIDTH-1:0] data_in,
     input [7:0] output_rate,
     input stb_out,
     output reg [WIDTH-1:0] data_out);

   // Input data Pipeline
   reg [WIDTH-1:0] data_in_pipe[0:23];

   reg 		   stb_pipe0, stb_pipe1, stb_pipe2, stb_pipe3, stb_pipe4 ;
   reg 		   stb_pipe5, stb_pipe6, stb_pipe7, stb_pipe8, stb_pipe9 ;
   wire [WIDTH-1:0] sample_a[0:5], sample_b[0:5];
   wire [17:0] coeff[0:5];
   wire [47:0] 	    accumulator_out[0:5];
   reg 	[47:0]	    partial_result_a, partial_result_b;
   reg [47:0] 	    result;


   // Implicit coeff23 of 131071
   wire [17:0] 	   coeff_a[0:5];
   wire [17:0] 	   coeff_b[0:5];

   assign 	   coeff_a[0] = -62;
   assign 	   coeff_b[0] = 194;
   assign 	   coeff_a[1] = -440;
   assign 	   coeff_b[1] = 855;
   assign 	   coeff_a[2] = -1505;
   assign 	   coeff_b[2] = 2478;
   assign 	   coeff_a[3] = -3900;
   assign 	   coeff_b[3] = 5990;
   assign 	   coeff_a[4] = -9187;
   assign 	   coeff_b[4] = 14632;
   assign 	   coeff_a[5] = -26536;
   assign 	   coeff_b[5] = 83009;

   genvar 	   i;

   always @(posedge clk)
     if (rst)
       data_in_pipe[0] <= 18'h0;
     else if (stb_in)
       data_in_pipe[0] <= data_in;

   generate
      for (i=0; i<23; i=i+1) begin: sample_pipeline
   	 always @(posedge clk) if (rst)
	   data_in_pipe[i+1] <= 0;
	 else if (stb_in)
	   data_in_pipe[i+1] <= data_in_pipe[i];
      end
   endgenerate

   generate
      for (i=0; i<6; i=i+1) begin: filter_core

         assign sample_a[i] = stb_pipe0 ? data_in_pipe[(i*2)] : data_in_pipe[(2*i)+1];
         assign sample_b[i] = stb_pipe0 ? data_in_pipe[23-(i*2)] : data_in_pipe[23-(2*i)-1];
         // Coeffs are 1 pipeline downstream of sample input
         assign coeff[i] = stb_pipe1 ? coeff_a[i] : coeff_b[i];

	 add_then_mac
	   #(.DEVICE(DEVICE))
	     add_then_mac_i
	       (
		.acc(accumulator_out[i]),
		.carryin(1'b0),
		.ce(1'b1),
		.clk(clk),
		.b(coeff[i]),
		.load(stb_pipe2),
		.c(48'h0),
		.a(sample_a[i]),
		.d(sample_b[i]),
		.rst(rst)
		);

     end // block: filter_core
   endgenerate

   //
   // Dual 3:1 compressors
   //
   always @(posedge clk) if (stb_pipe5) begin
      partial_result_a[47:0] <= accumulator_out[0] + accumulator_out[1] + accumulator_out[2];
      partial_result_b[47:0] <= accumulator_out[3] + accumulator_out[4] + accumulator_out[5];
   end

   //
   // Final Result Adder
   //
   always @(posedge clk) if (stb_pipe6) begin
      result[47:0] <= partial_result_a[47:0] + partial_result_b[47:0];
   end

   //
   // Now round unneed precision from accumulator result, and clip the unused dynamic range
   //
   wire [47:17] 	 result_rnd;
   round_reg #(.bits_in(48),.bits_out(31))
     final_round (.clk(clk),.in(result[47:0]),.out(result_rnd[47:17]),.err());

   wire [34:17] 	 result_clip;
   clip_reg #(.bits_in(31),.bits_out(18)) final_clip
     (.clk(clk),.in(result_rnd[47:17]),.strobe_in(1'b1), .out(result_clip[34:17]), .strobe_out());

   //
   // Data enters the sample pipeline when stb_in is asserted.
   // The clock cycle after is phase=0 with pipeline delay=0
   //
   always @(posedge clk)
     if (rst) begin
	stb_pipe0 <= 1'b0;   // New sample loaded into sample pipeline, setup on reg a
	stb_pipe1 <= 1'b0;   // Sample n+0 presented to pre-adder, result to reg ad, coeff_a setup on input reg b
	stb_pipe2 <= 1'b0;   // Sample n+1 presented to pre-adder, product of sample n+0 and coeff_a setup on reg m
   	stb_pipe3 <= 1'b0;   // product of sample n+1 and coeff_b setup on reg m, previous product loaded into accumulator.
	stb_pipe4 <= 1'b0;   // Add both products into accumulator
   	stb_pipe5 <= 1'b0;   // Add 3 accumulator values into one partial result
	stb_pipe6 <= 1'b0;   // Add both partial results into final full precision result.
	stb_pipe7 <= 1'b0;   // Round result
	stb_pipe8 <= 1'b0;   // Clip Result
	stb_pipe9 <= 1'b0;

     end else begin
	stb_pipe0 <= stb_in;      // New sample loaded into sample pipeline, setup on reg a
	stb_pipe1 <= stb_pipe0;   // Sample n+0 presented to pre-adder, result to reg ad, coeff_a setup on input reg b
	stb_pipe2 <= stb_pipe1;   // Sample n+1 presented to pre-adder, product of sample n+0 and coeff_a setup on reg m
   	stb_pipe3 <= stb_pipe2;   // product of sample n+1 and coeff_b setup on reg m, previous product loaded into accumulator.
	stb_pipe4 <= stb_pipe3;   // Add both products into accumulator
   	stb_pipe5 <= stb_pipe4;   // Add 3 accumulator values into one partial result
	stb_pipe6 <= stb_pipe5;   // Add both partial results into final result.
	stb_pipe7 <= stb_pipe6;   // Round result
	stb_pipe8 <= stb_pipe7;   // Clip Result
	stb_pipe9 <= stb_pipe8;

     end // else: !if(rst)

   //
   // Interleave newly interpolated samples (odd taps) with raw input samples (even taps - All zero except center tap)
   // Account for differences caused by various CPO settings and the pipeline advancing.
   //
   always @(posedge clk)
     if (bypass)
       data_out <= data_in;
     else if (stb_in & stb_out)
        data_out <= result_clip[34:17];
      else if(stb_out)
       case(output_rate)
	 1: data_out <= data_in_pipe[16]; // Four input pipeline shifts since we calculated odd taps
	 2: data_out <= data_in_pipe[14]; // Three input pipeline shifts since we calculated odd taps
	 3,4: data_out <= data_in_pipe[13]; // Two input pipeline shifts since we calculated odd taps
	 default: data_out <= data_in_pipe[12]; // One input pipeline shift since we calculated odd taps
       endcase // case(output_rate)

endmodule // hb47_int
