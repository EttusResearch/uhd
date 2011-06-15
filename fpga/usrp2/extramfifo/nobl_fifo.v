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

// Since this FIFO uses a ZBT/NoBL SRAM for its storage which is a since port 
// device it can only sustain data throughput at half the RAM clock rate.
// Fair arbitration to ensure this occurs is included in this logic and
// requests for transactions that can not be completed are held off.
// This FIFO requires a an external signal driving read_strobe that assures space for at least 6
// reads since this the theopretical maximum number in flight due to pipeling.

module nobl_fifo
  #(parameter WIDTH=18,RAM_DEPTH=19,FIFO_DEPTH=19)
    (   
	input clk,
	input rst,
	input [WIDTH-1:0] RAM_D_pi,
	output [WIDTH-1:0] RAM_D_po,
	output RAM_D_poe,
	output [RAM_DEPTH-1:0] RAM_A,
	output RAM_WEn,
	output RAM_CENn,
	output RAM_LDn,
	output RAM_OEn,
	output RAM_CE1n,
	input [WIDTH-1:0] write_data,
	input write_strobe,
	output reg space_avail,
	output  [WIDTH-1:0] read_data,
	input read_strobe,                    // Triggers a read, result in approximately 6 cycles.
	output  data_avail,                    // Qulaifys read data available this cycle on read_data.
	output reg [FIFO_DEPTH-1:0] capacity
	);

   //reg [FIFO_DEPTH-1:0] capacity;
   reg [FIFO_DEPTH-1:0] wr_pointer;
   reg [FIFO_DEPTH-1:0] rd_pointer;
   wire [RAM_DEPTH-1:0] address;
   reg 			data_avail_int;  // Internal not empty flag.
   
   assign 	    read = read_strobe && data_avail_int;
   assign 	    write = write_strobe && space_avail;

   // When a read and write collision occur, supress the space_avail flag next cycle
   // and complete write followed by read over 2 cycles. This forces balanced arbitration
   // and makes for a simple logic design.

   always @(posedge clk)
     if (rst)
       begin
	  capacity <= (1 << FIFO_DEPTH) - 1;
	  wr_pointer <= 0;
	  rd_pointer <= 0;
	  space_avail <= 1;
	  data_avail_int <= 0;
       end
     else	  
       begin
	  // No space available if:
	  // Capacity is already zero; Capacity is 1 and write is asserted (lookahead); both read and write are asserted (collision)
	  space_avail <= ~((capacity == 0) || (read&&write) || ((capacity == 1) && write) );
	  // Capacity has 1 cycle delay so look ahead here for corner case of read of last item in FIFO.
	  data_avail_int <= ~((capacity == ((1 << FIFO_DEPTH)-1))  || ((capacity == ((1 << FIFO_DEPTH)-2)) && (~write && read))  );
	  wr_pointer <= wr_pointer + write;
	  rd_pointer <= rd_pointer + (~write && read); 
	  capacity <= capacity - write + (~write && read) ;
       end // else: !if(rst)

   assign address = write ? wr_pointer : rd_pointer;
   assign enable = write || read; 


   //
   // Simple NoBL SRAM interface, 4 cycle read latency.
   // Read/Write arbitration via temprary application of empty/full flags.
   //
   nobl_if #(.WIDTH(WIDTH),.DEPTH(RAM_DEPTH))
     nobl_if_i1
       (
	.clk(clk),
	.rst(rst),
	.RAM_D_pi(RAM_D_pi),
	.RAM_D_po(RAM_D_po),
	.RAM_D_poe(RAM_D_poe),
	.RAM_A(RAM_A),
	.RAM_WEn(RAM_WEn),
	.RAM_CENn(RAM_CENn),
	.RAM_LDn(RAM_LDn),
	.RAM_OEn(RAM_OEn),
	.RAM_CE1n(RAM_CE1n),
	.address(address),
	.data_out(write_data),
	.data_in(read_data),
	.data_in_valid(data_avail),
	.write(write),
	.enable(enable)
	);

   

endmodule // nobl_fifo
