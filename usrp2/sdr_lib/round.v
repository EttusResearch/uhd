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

module round
  #(parameter bits_in=0,
    parameter bits_out=0,
    parameter round_to_zero=0,       // original behavior
    parameter round_to_nearest=1,    // lowest noise
    parameter trunc=0)               // round to negative infinity
    (input [bits_in-1:0] in,
     output [bits_out-1:0] out,
     output [bits_in-bits_out:0] err);

   wire 			 round_corr,round_corr_trunc,round_corr_rtz,round_corr_nearest,round_corr_nearest_safe;
   
   assign 			 round_corr_trunc = 0;
   assign 			 round_corr_rtz = (in[bits_in-1] & |in[bits_in-bits_out-1:0]);
   assign 			 round_corr_nearest = in[bits_in-bits_out-1];

   generate
      if(bits_in-bits_out > 1)
	assign 			 round_corr_nearest_safe = (~in[bits_in-1] & (&in[bits_in-2:bits_out])) ? 0 :
				 round_corr_nearest;
      else
	assign round_corr_nearest_safe = round_corr_nearest;
   endgenerate
   
      
   assign round_corr = round_to_nearest ? round_corr_nearest_safe :
		       trunc ? round_corr_trunc : 
		       round_to_zero ? round_corr_rtz :
		       0;  // default to trunc
      
   assign out = in[bits_in-1:bits_in-bits_out] + round_corr;
   
   assign err = in - {out,{(bits_in-bits_out){1'b0}}};
   
endmodule // round
