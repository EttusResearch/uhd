//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Dual ported ram attached to a FIFO for readout
//   Most useful for storing coefficients for windows, filters, etc.
//   Config port is used for writing in order
//   i_* (address in) and o_* (data out) ports are for streams, and can read out in arbitrary order

module ram_to_fifo
  #(parameter DWIDTH=32,
    parameter AWIDTH=10)
   (input clk, input reset, input clear,
    // FIXME add writing port
    input [DWIDTH-1:0] config_tdata, input config_tlast, input config_tvalid, output config_tready,
    input [AWIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [DWIDTH-1:0] o_tdata, output reg o_tlast, output reg o_tvalid, input o_tready);

   // Write side
   reg [AWIDTH-1:0] 	write_addr;
   
   assign config_tready = 1'b1;

   always @(posedge clk)
     if(reset | clear)
       write_addr <= 0;
     else
       if(config_tvalid & config_tready)
	 if(config_tlast)
	   write_addr <= 0;
	 else
	   write_addr <= write_addr + 1;
      
   ram_2port #(.DWIDTH(DWIDTH), .AWIDTH(AWIDTH)) ram_2port
     (.clka(clk), .ena(1'b1), .wea(config_tvalid), .addra(write_addr), .dia(config_tdata), .doa(), // Write port
      .clkb(clk), .enb(i_tready & i_tvalid), .web(1'b0), .addrb(i_tdata), .dib({DWIDTH{1'b1}}), .dob(o_tdata)); // Read port

   // Read side
   assign i_tready = ~o_tvalid | o_tready;

   always @(posedge clk)
     if(reset | clear)
       begin
	  o_tvalid <= 1'b0;
	  o_tlast <= 1'b0;
       end
     else
       begin
	  o_tvalid <= (i_tready & i_tvalid) | (o_tvalid & ~o_tready);
	  if(i_tready & i_tvalid)
	    o_tlast <= i_tlast;
       end
   
endmodule // ram_to_fifo
