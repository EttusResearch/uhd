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



module rssi (input clock, input reset, input enable,
	     input [11:0] adc, output [15:0] rssi, output [15:0] over_count);

   wire 		  over_hi = (adc == 12'h7FF);
   wire 		  over_lo = (adc == 12'h800);
   wire 		  over = over_hi | over_lo;

   reg [25:0] 		  over_count_int;
   always @(posedge clock)
     if(reset | ~enable)
       over_count_int <= #1 26'd0;
     else
       over_count_int <= #1 over_count_int + (over ? 26'd65535 : 26'd0) - over_count_int[25:10];
   
   assign      over_count = over_count_int[25:10];
   
   wire [11:0] abs_adc = adc[11] ? ~adc : adc;

   reg [25:0]  rssi_int;
   always @(posedge clock)
     if(reset | ~enable)
       rssi_int <= #1 26'd0;
     else
       rssi_int <= #1 rssi_int + abs_adc - rssi_int[25:10];

   assign      rssi = rssi_int[25:10];
   
endmodule // rssi
