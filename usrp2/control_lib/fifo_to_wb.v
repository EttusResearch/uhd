
module fifo_to_wb
  (input clk, input reset, input clear,
   input [35:0] data_i, input src_rdy_i, output dst_rdy_o,
   output [35:0] data_o, output src_rdy_o, input dst_rdy_i,
   output [10:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
   output wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
   output [31:0] debug0, output [31:0] debug1);

   wire [35:0] 	 ctrl_data, resp_data;
   wire 	 ctrl_src_rdy, ctrl_dst_rdy, resp_src_rdy, resp_dst_rdy;
   
   fifo_short #(.WIDTH(36)) ctrl_sfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o),
      .dataout(ctrl_data), .src_rdy_o(ctrl_src_rdy), .dst_rdy_i(ctrl_dst_rdy));

   fifo_short #(.WIDTH(36)) resp_sfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(resp_data), .src_rdy_i(resp_src_rdy), .dst_rdy_o(resp_dst_rdy),
      .dataout(data_o), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));
   
   // Loopback control packets
   
   assign resp_data = ctrl_data;
   assign resp_src_rdy = ctrl_src_rdy;
   assign ctrl_dst_rdy = resp_dst_rdy;

   assign debug0 = ctrl_data[31:0];
   assign debug1 = { ctrl_src_rdy, ctrl_dst_rdy, resp_src_rdy, resp_dst_rdy, ctrl_data[35:31] };
   		  
endmodule // fifo_to_wb

   
