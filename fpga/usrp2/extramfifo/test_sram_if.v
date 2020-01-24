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

// Instantiate this block at the core level to conduct closed 
// loop testing of the AC performance of the USRP2 SRAM interface


`define WIDTH 18
`define DEPTH 19

module   test_sram_if
  (
   input clk,
   input rst,
   input [`WIDTH-1:0] RAM_D_pi,
   output [`WIDTH-1:0] RAM_D_po,
   output RAM_D_poe,
   output [`DEPTH-1:0] RAM_A,
   output RAM_WEn,
   output RAM_CENn,
   output RAM_LDn,
   output RAM_OEn,
   output RAM_CE1n,
   output reg correct
   );
   
   reg [`DEPTH-1:0] write_count;
   reg [`DEPTH-1:0] read_count;
   reg 		    enable;
   reg 		    write;
   reg 		    write_cycle;
   reg 		    read_cycle;
   reg 		    enable_reads;
   reg [18:0] 	    address;
   reg [17:0] 	    data_out;
   wire [17:0] 	    data_in;
   wire 	    data_in_valid;
   
   reg [17:0] 	    check_data;
   reg [17:0] 	    check_data_old;
   reg [17:0] 	    check_data_old2;
    
  //
  // Create counter that generates both external modulo 2^19 address and modulo 2^18 data to test RAM.
  //

   always @(posedge clk)
     if (rst)
       begin
	  write_count <= 19'h0;
	  read_count <= 19'h0; 
       end
     else if (write_cycle) // Write cycle
       if (write_count == 19'h7FFFF)
	 begin
	    write_count <= 19'h0;
	 end
       else
	 begin
	    write_count <= write_count + 1'b1;
	 end
     else if (read_cycle) // Read cycle
       if (read_count == 19'h7FFFF)
	 begin
	    read_count <= 19'h0;
	 end
       else
	 begin
	    read_count <= read_count + 1'b1;
	 end
   
   always @(posedge clk)
     if (rst)
       begin
	  enable_reads <= 0;
	  read_cycle <= 0;
	  write_cycle <= 0;
       end
     else
       begin
	  write_cycle <= ~write_cycle;
	  if (enable_reads)
	    read_cycle <= write_cycle;
	  if (write_count == 15) // Enable reads 15 writes after reset terminates.
	    enable_reads <= 1;
       end // else: !if(rst)
   
   always @(posedge clk)
     if (rst)
       begin
	  enable <= 0;
       end
     else if (write_cycle)
       begin
	  address <= write_count;
	  data_out <= write_count[17:0];
	  enable <= 1;
	  write <= 1;
       end
     else if (read_cycle)
       begin
	  address <= read_count;
	  check_data <= read_count[17:0];
	  check_data_old <= check_data;
	  check_data_old2 <= check_data_old;
	  enable <= 1;
	  write <= 0;
       end
     else
       enable <= 0;

   always @(posedge clk)
     if (data_in_valid)
       begin
	  correct <= (data_in == check_data_old2);
       end
	  
    
   nobl_if nobl_if_i1
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
      .data_out(data_out),
      .data_in(data_in),
      .data_in_valid(data_in_valid),
      .write(write),
      .enable(enable)
      );


   wire [35:0] CONTROL0;
   reg [7:0]   data_in_reg, data_out_reg, address_reg;
   reg 	       data_in_valid_reg,write_reg,enable_reg,correct_reg;

   always @(posedge clk)
     begin
	data_in_reg <= data_in[7:0];
	data_out_reg <= data_out[7:0];
	data_in_valid_reg <= data_in_valid;
	write_reg <= write;
	enable_reg <= enable;
	correct_reg <= correct;
	address_reg <= address;
	
     end
   
   
   icon icon_i1
     (
      .CONTROL0(CONTROL0)
      );
   
   ila ila_i1
     (    
	  .CLK(clk), 
	  .CONTROL(CONTROL0), 
	  //    .TRIG0(address_reg), 
	  .TRIG0(data_in_reg[7:0]), 
	  .TRIG1(data_out_reg[7:0]),
	  .TRIG2(address_reg[7:0]),
	  .TRIG3({data_in_valid_reg,write_reg,enable_reg,correct_reg})
	  );
 

   
endmodule // test_sram_if

   