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



// AD9510 Register Map (from datasheet Rev. A)

/* INSTRUCTION word format (16 bits)
 * 15       Read = 1, Write = 0
 * 14:13    W1/W0,  Number of bytes 00 - 1, 01 - 2, 10 - 3, 11 - stream
 * 12:0     Address
 */

/* ADDR     Contents             Value (hex)
 * 00       Serial Config Port   10 (def) -- MSB first, SDI/SDO separate
 * 04       A Counter
 * 05-06    B Counter
 * 07-0A    PLL Control
 * 0B-0C    R Divider
 * 0D       PLL Control
 * 34-3A    Fine Delay
 * 3C-3F    LVPECL Outs
 * 40-43    LVDS/CMOS Outs
 * 45       Clock select, power down
 * 48-57    Dividers
 * 58       Func and Sync
 * 5A       Update regs
 */


module spi
  (input reset,
   input clk,

   // SPI signals
   output sen, 
   output sclk,
   input sdi,
   output sdo,

   // Interfaces
   input read_1,
   input write_1,
   input [15:0] command_1,
   input [15:0] wdata_1,
   output [15:0] rdata_1,
   output reg done_1,
   input msb_first_1,
   input [5:0] command_width_1,
   input [5:0] data_width_1,
   input [7:0] clkdiv_1
   
   );

   reg [15:0]  command, wdata, rdata;
   reg 	       done;

   always @(posedge clk)
     if(reset)
       done_1 <= #1 1'b0;
   
   always @(posedge clk)
     if(reset)
       begin
	  counter <= #1 7'd0;
	  command <= #1 20'd0;
       end
     else if(start)
       begin
	  counter <= #1 7'd1;
	  command <= #1 {read,w,addr_data};
       end
     else if( |counter && ~done )
       begin
	  counter <= #1 counter + 7'd1;
	  if(~counter[0])
	    command <= {command[22:0],1'b0};
       end

   wire done = (counter == 8'd49);
   
   assign sen = (done | counter == 8'd0);  // CSB is high when we're not doing anything
   assign sclk = ~counter[0];
   assign sdo = command[23];
   

endmodule // clock_control
