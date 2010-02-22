
// Grab settings off the wishbone bus, send them out to settings bus
// 16 bits little endian, but all registers need to be written 32 bits at a time.
// This means that you write the low 16 bits first and then the high 16 bits.
// The setting regs are strobed when the high 16 bits are written

module settings_bus_16LE
  #(parameter AWIDTH=16)
    (input wb_clk, 
     input wb_rst, 
     input [AWIDTH-1:0] wb_adr_i,
     input [15:0] wb_dat_i,
     input wb_stb_i,
     input wb_we_i,
     output reg wb_ack_o,
     output strobe,
     output reg [7:0] addr,
     output reg [31:0] data);

   reg 		       stb_int;
   
   always @(posedge wb_clk)
     if(wb_rst)
       begin
	  stb_int <= 1'b0;
	  addr <= 8'd0;
	  data <= 32'd0;
       end
     else if(wb_we_i & wb_stb_i)
       begin
	  addr <= wb_adr_i[9:2];
	  if(wb_adr_i[1])
	    begin
	       stb_int <= 1'b1;     // We now have both halves
	       data[31:16] <= wb_dat_i;
	    end
	  else
	    begin
	       stb_int <= 1'b0;     // Don't strobe, we need other half
	       data[15:0] <= wb_dat_i;
	    end
       end
     else
       stb_int <= 1'b0;

   always @(posedge wb_clk)
     if(wb_rst)
       wb_ack_o <= 0;
     else
       wb_ack_o <= wb_stb_i & ~wb_ack_o;

   assign strobe = stb_int & wb_ack_o;
          
endmodule // settings_bus_16LE
