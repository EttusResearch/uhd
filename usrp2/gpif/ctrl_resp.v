

module ctrl_resp
  (input clk, input reset,
   input [17:0] ctrl_i, input ctrl_src_rdy_i, output ctrl_dst_rdy_o,
   output [17:0] resp_i, output resp_src_rdy_o, input resp_dst_rdy_i,

   output [15:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
   output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,

   input [3:0] trigger_i
   );

   

endmodule // ctrl_resp
