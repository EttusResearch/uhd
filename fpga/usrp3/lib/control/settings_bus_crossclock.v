//
// Copyright 2011-2012 Ettus Research LLC
//




// This module takes the settings bus on one clock domain and crosses it over to another domain
// Typically it will be used with the input settings bus on the wishbone clock, and either 
// the system or dsp clock on the output side

module settings_bus_crossclock
  #(parameter FLOW_CTRL=0, parameter AWIDTH=8, parameter DWIDTH=32)
  (input clk_i, input rst_i, input set_stb_i, input [AWIDTH-1:0] set_addr_i, input [DWIDTH-1:0] set_data_i,
   input clk_o, input rst_o, output set_stb_o, output [AWIDTH-1:0] set_addr_o, output [DWIDTH-1:0] set_data_o, input blocked);

   wire  nfull, nempty;
   
   axi_fifo_2clk #(.WIDTH(AWIDTH + DWIDTH), .SIZE(0)) settings_fifo
     (.reset(rst_i),
      .i_aclk(clk_i), .i_tdata({set_addr_i,set_data_i}), .i_tvalid(set_stb_i), .i_tready(nfull),
      .o_aclk(clk_o), .o_tdata({set_addr_o,set_data_o}), .o_tready(set_stb_o), .o_tvalid(nempty));

   assign set_stb_o = nempty & (~blocked | ~FLOW_CTRL);

endmodule // settings_bus_crossclock
