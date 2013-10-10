//
// Copyright 2011-2012 Ettus Research LLC
//



// Grab settings off the wishbone bus, send them out to our simpler bus on the fast clock

module settings_bus
  #(parameter AWIDTH=16, parameter DWIDTH=32, parameter SWIDTH=8)
    (input wb_clk, 
     input wb_rst, 
     input [AWIDTH-1:0] wb_adr_i,
     input [DWIDTH-1:0] wb_dat_i,
     input wb_stb_i,
     input wb_we_i,
     output reg wb_ack_o,
     output reg strobe,
     output reg [SWIDTH-1:0] addr,
     output reg [31:0] data);

   reg 	    stb_int, stb_int_d1;
   
   always @(posedge wb_clk)
     if(wb_rst)
       begin
	  strobe <= 1'b0;
	  addr <= {SWIDTH{1'b0}};
	  data <= 32'd0;
       end
     else if(wb_we_i & wb_stb_i & ~wb_ack_o)
       begin
	  strobe <= 1'b1;
	  addr <= wb_adr_i[SWIDTH+1:2];
	  data <= wb_dat_i;
       end
     else
       strobe <= 1'b0;

   always @(posedge wb_clk)
     if(wb_rst)
       wb_ack_o <= 0;
     else
       wb_ack_o <= wb_stb_i & ~wb_ack_o;

endmodule // settings_bus
