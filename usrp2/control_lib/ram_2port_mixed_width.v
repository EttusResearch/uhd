module ram_2port_mixed_width
  #(parameter AWIDTH=9)
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

   RAMB16BWER #(.DATA_WIDTH_A(18),  // Valid values are 0, 1, 2, 4, 9, 18, or 36
		.DATA_WIDTH_B(36),  // Valid values are 0, 1, 2, 4, 9, 18, or 36
		.DOA_REG(0), // Specifies to enable=1/disable=0 port A output registers
		.DOB_REG(0), // Specifies to enable=1/disable=0 port B output registers
		.INIT_A(36'h000000000),  // Initial values on A output port
		.INIT_B(36'h000000000),  // Initial values on B output port
		.RSTTYPE("SYNC"),  // Specifes reset type to be "SYNC" or "ASYNC" 
		.SIM_COLLISION_CHECK("ALL"),  // Collision check enable "ALL", "WARNING_ONLY", 
                //   "GENERATE_X_ONLY" or "NONE" 
		.SRVAL_A(36'h000000000), // Set/Reset value for A port output
		.SRVAL_B(36'h000000000),  // Set/Reset value for B port output
		.WRITE_MODE_A("WRITE_FIRST"),  // "WRITE_FIRST", "READ_FIRST", or "NO_CHANGE" 
		.WRITE_MODE_B("WRITE_FIRST"))  // "WRITE_FIRST", "READ_FIRST", or "NO_CHANGE" 
   RAMB16BWER_inst (.DOA(do16),      // 32-bit A port data output
		    .DOB(do32),      // 32-bit B port data output
		    .DOPA(),    // 4-bit A port parity data output
		    .DOPB(),    // 4-bit B port parity data output
		    .ADDRA(addr16),  // 14-bit A port address input
		    .ADDRB(addr32),  // 14-bit B port address input
		    .CLKA(clk16),    // 1-bit A port clock input
		    .CLKB(clk32),    // 1-bit B port clock input
		    .DIA(di16),      // 32-bit A port data input
		    .DIB(di32),      // 32-bit B port data input
		    .DIPA(0),    // 4-bit A port parity data input
		    .DIPB(0),    // 4-bit B port parity data input
		    .ENA(en16),      // 1-bit A port enable input
		    .ENB(en32),      // 1-bit B port enable input
		    .REGCEA(0), // 1-bit A port output register enable input
		    .REGCEB(0), // 1-bit B port output register enable input
		    .RSTA(0),    // 1-bit A port reset input
		    .RSTB(0),    // 1-bit B port reset input
		    .WEA({2{we16}}),      // 4-bit A port write enable input
		    .WEB({4{we32}})       // 4-bit B port write enable input
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
