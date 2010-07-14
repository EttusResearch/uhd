
// 18 bit interface means we either can't handle errors or can't handle odd lengths
//   unless we go to heroic measures

module fifo_extram36
  (input clk, input reset, input clear,
   input [35:0] datain, input src_rdy_i, output dst_rdy_o, output [15:0] space,
   output [35:0] dataout, output src_rdy_o, input dst_rdy_i, output [15:0] occupied,
   input sram_clk, output [18:0] sram_a, inout [17:0] sram_d, output sram_we,
   output [1:0] sram_bw, output sram_adv, output sram_ce, output sram_oe, output sram_mode,
   output sram_zz);

   wire [17:0] f18_data_1, f18_data_2, f18_data_3, f18_data_4;
   wire        f18_src_rdy_1, f18_dst_rdy_1, f18_src_rdy_2, f18_dst_rdy_2;
   wire        f18_src_rdy_3, f18_dst_rdy_3, f18_src_rdy_4, f18_dst_rdy_4;
   
   fifo36_to_fifo18 f36_to_f18
     (.clk(clk), .reset(reset), .clear(clear),
      .f36_datain(datain), .f36_src_rdy_i(src_rdy_i), .f36_dst_rdy_o(dst_rdy_o),
      .f18_dataout(f18_data_1), .f18_src_rdy_o(f18_src_rdy_1), .f18_dst_rdy_i(f18_dst_rdy_1) );

   wire [15:0] f1_occ, f2_space;
   
   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) fifo_2clock_in
     (.wclk(clk), .datain(f18_data_1), .src_rdy_i(f18_src_rdy_1), .dst_rdy_o(f18_dst_rdy_1), .space(),
      .rclk(sram_clk), .dataout(f18_data_2), .src_rdy_o(f18_src_rdy_2), .dst_rdy_i(f18_dst_rdy_2), .short_occupied(f1_occ),
      .arst(reset) );

   fifo_extram fifo_extram
     (.reset(reset), .clear(clear),
      .datain(f18_data_2), .src_rdy_i(f18_src_rdy_2), .dst_rdy_o(f18_dst_rdy_2), .space(), .occ_in(f1_occ),
      .dataout(f18_data_3), .src_rdy_o(f18_src_rdy_3), .dst_rdy_i(f18_dst_rdy_3), .occupied(), .space_in(f2_space),
      .sram_clk(sram_clk), .sram_a(sram_a), .sram_d(sram_d), .sram_we(sram_we),
      .sram_bw(sram_bw), .sram_adv(sram_adv), .sram_ce(sram_ce), .sram_oe(sram_oe),
      .sram_mode(sram_mode), .sram_zz(sram_zz));
   
   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) fifo_2clock_out
     (.wclk(sram_clk), .datain(f18_data_3), .src_rdy_i(f18_src_rdy_3), .dst_rdy_o(f18_dst_rdy_3), .short_space(f2_space),
      .rclk(clk), .dataout(f18_data_4), .src_rdy_o(f18_src_rdy_4), .dst_rdy_i(f18_dst_rdy_4), .occupied(),
      .arst(reset) );

   fifo18_to_fifo36 f18_to_f36
     (.clk(clk), .reset(reset), .clear(clear),
      .f18_datain(f18_data_4), .f18_src_rdy_i(f18_src_rdy_4), .f18_dst_rdy_o(f18_dst_rdy_4),
      .f36_dataout(dataout), .f36_src_rdy_o(src_rdy_o), .f36_dst_rdy_i(dst_rdy_i) );
   
endmodule // fifo_extram36
