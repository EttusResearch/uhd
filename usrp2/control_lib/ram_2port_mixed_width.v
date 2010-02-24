

module ram_2port_mixed_width
  #(parameter AWIDTH=9)
    (input clk16,
     input en16,
     input we16,
     input [10:0] addr16,
     input [15:0] di16,
     output reg [15:0] do16,
     
     input clk32,
     input en32,
     input we32,
     input [9:0] addr32,
     input [31:0] di32,
     output reg [31:0] do32);
   
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
