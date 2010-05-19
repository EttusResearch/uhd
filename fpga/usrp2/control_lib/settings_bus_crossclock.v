

// This module takes the settings bus on one clock domain and crosses it over to another domain
// Typically it will be used with the input settings bus on the wishbone clock, and either 
// the system or dsp clock on the output side

module settings_bus_crossclock
  (input clk_i, input rst_i, input set_stb_i, input [7:0] set_addr_i, input [31:0] set_data_i,
   input clk_o, input rst_o, output set_stb_o, output [7:0] set_addr_o, output [31:0] set_data_o);

   wire  full, empty;
   
   fifo_xlnx_16x40_2clk settings_fifo
     (.rst(rst_i),
      .wr_clk(clk_i), .din({set_addr_i,set_data_i}), .wr_en(set_stb_i & ~full), .full(full),
      .rd_clk(clk_o), .dout({set_addr_o,set_data_o}), .rd_en(~empty), .empty(empty));

   assign set_stb_o = ~empty;

endmodule // settings_bus_crossclock
