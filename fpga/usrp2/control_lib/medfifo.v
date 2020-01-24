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


module medfifo
  #(parameter WIDTH=32,
    parameter DEPTH=1)
    (input clk, input rst,
     input [WIDTH-1:0] datain,
     output [WIDTH-1:0] dataout,
     input read,
     input write,
     input clear,
     output full,
     output empty,
     output [7:0] space,
     output [7:0] occupied);

   localparam 		NUM_FIFOS = (1<<DEPTH);
   
   wire [WIDTH-1:0] 	dout [0:NUM_FIFOS-1];
   wire [0:NUM_FIFOS-1] full_x;
   wire [0:NUM_FIFOS-1] empty_x;

   shortfifo #(.WIDTH(WIDTH))
     head (.clk(clk),.rst(rst),
	   .datain(datain),.write(write),.full(full),
	   .dataout(dout[0]),.read(~empty_x[0] & ~full_x[1]),.empty(empty_x[0]),
	   .clear(clear),.space(space[4:0]),.occupied() );
   
   shortfifo #(.WIDTH(WIDTH))
     tail (.clk(clk),.rst(rst),
	   .datain(dout[NUM_FIFOS-2]),.write(~empty_x[NUM_FIFOS-2] & ~full_x[NUM_FIFOS-1]),.full(full_x[NUM_FIFOS-1]),
	   .dataout(dataout),.read(read),.empty(empty),
	   .clear(clear),.space(),.occupied(occupied[4:0]) );

   genvar 		i;
   generate
      for(i = 1; i < NUM_FIFOS-1 ; i = i + 1)
	begin : gen_depth
	   shortfifo #(.WIDTH(WIDTH))
	     shortfifo (.clk(clk),.rst(rst),
			.datain(dout[i-1]),.write(~full_x[i] & ~empty_x[i-1]),.full(full_x[i]),
			.dataout(dout[i]),.read(~full_x[i+1] & ~empty_x[i]),.empty(empty_x[i]),
			.clear(clear),.space(),.occupied() );
	end
   endgenerate

   assign space[7:5] = 0;
   assign occupied[7:5] = 0;
   
endmodule // medfifo
