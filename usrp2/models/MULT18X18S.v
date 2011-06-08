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


// Model of the Xilinx mult18x18s for signed 18x18 bit multiplies,
// As in the Spartan 3 series

module MULT18X18S
  (output reg signed [35:0] P,
   input signed [17:0] A,
   input signed [17:0] B,
   input C,    // Clock
   input CE,   // Clock Enable
   input R     // Synchronous Reset
   );
   
   always @(posedge C)
     if(R)
       P <= 36'sd0;
     else if(CE)
       P <= A * B;

endmodule // MULT18X18S
