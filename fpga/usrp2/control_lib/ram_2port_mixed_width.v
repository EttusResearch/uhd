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


module ram_2port_mixed_width
  (input clk16,
   input en16,
   input we16,
   input [10:0] addr16,
   input [15:0] di16,
   output [15:0] do16,
   input clk32,
   input en32,
   input we32,
   input [9:0] addr32,
   input [31:0] di32,
   output [31:0] do32);

   wire 	 en32a = en32 & ~addr32[9];
   wire 	 en32b = en32 & addr32[9];
   wire 	 en16a = en16 & ~addr16[10];
   wire 	 en16b = en16 & addr16[10];

   wire [31:0] 	 do32a, do32b;
   wire [15:0] 	 do16a, do16b;
   
   assign do32 = addr32[9] ? do32b : do32a;
   assign do16 = addr16[10] ? do16b : do16a;
   
   RAMB16BWE_S36_S18 #(.INIT_A(36'h000000000),
		       .INIT_B(18'h00000),
		       .SIM_COLLISION_CHECK("ALL"), // "NONE", "WARNING_ONLY", "GENERATE_X_ONLY", "ALL"
		       .SRVAL_A(36'h000000000), // Port A output value upon SSR assertion
		       .SRVAL_B(18'h00000),      // Port B output value upon SSR assertion
		       .WRITE_MODE_A("WRITE_FIRST"), // WRITE_FIRST, READ_FIRST or NO_CHANGE
		       .WRITE_MODE_B("WRITE_FIRST") // WRITE_FIRST, READ_FIRST or NO_CHANGE
		       ) 
   RAMB16BWE_S36_S18_0 (.DOA(do32a),       // Port A 32-bit Data Output
			.DOB(do16a),       // Port B 16-bit Data Output
			.DOPA(),     // Port A 4-bit Parity Output
			.DOPB(),     // Port B 2-bit Parity Output
			.ADDRA(addr32[8:0]),   // Port A 9-bit Address Input
			.ADDRB(addr16[9:0]),   // Port B 10-bit Address Input
			.CLKA(clk32),     // Port A 1-bit Clock
			.CLKB(clk16),     // Port B 1-bit Clock
			.DIA(di32),       // Port A 32-bit Data Input
			.DIB(di16),       // Port B 16-bit Data Input
			.DIPA(0),     // Port A 4-bit parity Input
			.DIPB(0),     // Port-B 2-bit parity Input
			.ENA(en32a),       // Port A 1-bit RAM Enable Input
			.ENB(en16a),       // Port B 1-bit RAM Enable Input
			.SSRA(0),     // Port A 1-bit Synchronous Set/Reset Input
			.SSRB(0),     // Port B 1-bit Synchronous Set/Reset Input
			.WEA({4{we32}}),       // Port A 4-bit Write Enable Input
			.WEB({2{we16}})        // Port B 2-bit Write Enable Input
			);

   RAMB16BWE_S36_S18 #(.INIT_A(36'h000000000),
		       .INIT_B(18'h00000),
		       .SIM_COLLISION_CHECK("ALL"), // "NONE", "WARNING_ONLY", "GENERATE_X_ONLY", "ALL"
		       .SRVAL_A(36'h000000000), // Port A output value upon SSR assertion
		       .SRVAL_B(18'h00000),      // Port B output value upon SSR assertion
		       .WRITE_MODE_A("WRITE_FIRST"), // WRITE_FIRST, READ_FIRST or NO_CHANGE
		       .WRITE_MODE_B("WRITE_FIRST") // WRITE_FIRST, READ_FIRST or NO_CHANGE
		       ) 
   RAMB16BWE_S36_S18_1 (.DOA(do32b),       // Port A 32-bit Data Output
			.DOB(do16b),       // Port B 16-bit Data Output
			.DOPA(),     // Port A 4-bit Parity Output
			.DOPB(),     // Port B 2-bit Parity Output
			.ADDRA(addr32[8:0]),   // Port A 9-bit Address Input
			.ADDRB(addr16[9:0]),   // Port B 10-bit Address Input
			.CLKA(clk32),     // Port A 1-bit Clock
			.CLKB(clk16),     // Port B 1-bit Clock
			.DIA(di32),       // Port A 32-bit Data Input
			.DIB(di16),       // Port B 16-bit Data Input
			.DIPA(0),     // Port A 4-bit parity Input
			.DIPB(0),     // Port-B 2-bit parity Input
			.ENA(en32b),       // Port A 1-bit RAM Enable Input
			.ENB(en16b),       // Port B 1-bit RAM Enable Input
			.SSRA(0),     // Port A 1-bit Synchronous Set/Reset Input
			.SSRB(0),     // Port B 1-bit Synchronous Set/Reset Input
			.WEA({4{we32}}),       // Port A 4-bit Write Enable Input
			.WEB({2{we16}})        // Port B 2-bit Write Enable Input
			);

endmodule // ram_2port_mixed_width



   
// ISE 10.1.03 chokes on the following
   
/*
   
   reg [31:0] 	       ram [(1<<AWIDTH)-1:0];
   integer 	       i;
   initial
     for(i=0;i<512;i=i+1)
       ram[i] <= 32'b0;
   
   always @(posedge clk16)
     if (en16)
       begin
          if (we16)
            if(addr16[0])
	      ram[addr16[10:1]][15:0] <= di16;
	    else
	      ram[addr16[10:1]][31:16] <= di16;
	  do16 <= addr16[0] ? ram[addr16[10:1]][15:0] : ram[addr16[10:1]][31:16];
       end

   always @(posedge clk32)
     if (en32)
       begin
          if (we32)
            ram[addr32] <= di32;
          do32 <= ram[addr32];
       end

endmodule // ram_2port_mixed_width

 
 */
