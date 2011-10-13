// -*- verilog -*-
//
//  USRP - Universal Software Radio Peripheral
//
//  Copyright (C) 2011 Matt Ettus
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301  USA
//

// Rounding "macro"
// Keeps the topmost bits, does proper 2s comp round to zero (unbiased truncation)

module round_tb();

   localparam IW=8;
   localparam OW=4;
   localparam EW=IW-OW+1;
   
   reg signed [IW-1:0] in;
   wire signed [OW-1:0] out;
   wire signed [EW-1:0] err;

   round #(.bits_in(IW),
	   .bits_out(OW),
	   .round_to_zero(0),       // original behavior
	   .round_to_nearest(1),    // lowest noise
	   .trunc(0))               // round to negative infinity
   round (.in(in),.out(out),.err(err));

   initial $dumpfile("round_tb.vcd");
   initial $dumpvars(0,round_tb);

   wire signed [IW-1:0] out_round = {out,{IW-OW{1'b0}}};
   
   initial
     begin
	in <= -129;
	#1;
	repeat (260)
	  begin
	     in <= in + 1;
	     #1;
	     $display("In %d, out %d, out_rnd %d, err %d, real err %d",in,out,out_round,-err,out_round-in);
	     #1;
	  end
	$finish;
     end
     
endmodule // round
