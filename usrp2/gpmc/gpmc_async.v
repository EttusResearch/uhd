//////////////////////////////////////////////////////////////////////////////////

module gpmc_async
  (// GPMC signals
   input arst,
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,
   
   // GPIOs for FIFO signalling
   output rx_have_data, output tx_have_space, output bus_error, input bus_reset,
   
   // Wishbone signals
   input wb_clk, input wb_rst,
   output [10:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
   output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,

   // FIFO interface
   input fifo_clk, input fifo_rst,
   output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
   input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,

   input [15:0] tx_frame_len, output [15:0] rx_frame_len,
   
   output [31:0] debug
   );

   wire 	EM_output_enable = (~EM_NOE & (~EM_NCS4 | ~EM_NCS6));
   wire [15:0] 	EM_D_fifo;
   wire [15:0] 	EM_D_wb;

   assign EM_D = ~EM_output_enable ? 16'bz : ~EM_NCS4 ? EM_D_fifo : EM_D_wb;

   wire 	bus_error_tx, bus_error_rx;
   assign bus_error = bus_error_tx | bus_error_rx;
   
   // CS4 is RAM_2PORT for DATA PATH (high-speed data)
   //    Writes go into one RAM, reads come from the other
   // CS6 is for CONTROL PATH (wishbone)

   // ////////////////////////////////////////////
   // TX Data Path

   wire [17:0] 	tx18_data, tx18b_data;
   wire 	tx18_src_rdy, tx18_dst_rdy, tx18b_src_rdy, tx18b_dst_rdy;
   wire [15:0] 	tx_fifo_space;
   
   gpmc_to_fifo_async gpmc_to_fifo_async
     (.EM_D(EM_D), .EM_NBE(EM_NBE), .EM_NCS(EM_NCS4), .EM_NWE(EM_NWE),
      .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
      .data_o(tx18_data), .src_rdy_o(tx18_src_rdy), .dst_rdy_i(tx18_dst_rdy),
      .frame_len(tx_frame_len), .fifo_space(tx_fifo_space), .fifo_ready(tx_have_space),
      .bus_error(bus_error_tx) );
   
   fifo_cascade #(.WIDTH(18), .SIZE(10)) tx_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(tx18_data), .src_rdy_i(tx18_src_rdy), .dst_rdy_o(tx18_dst_rdy), .space(tx_fifo_space),
      .dataout(tx18b_data), .src_rdy_o(tx18b_src_rdy), .dst_rdy_i(tx18b_dst_rdy), .occupied());

   fifo19_to_fifo36 f19_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f19_datain({1'b0,tx18b_data}), .f19_src_rdy_i(tx18b_src_rdy), .f19_dst_rdy_o(tx18b_dst_rdy),
      .f36_dataout(tx_data_o), .f36_src_rdy_o(tx_src_rdy_o), .f36_dst_rdy_i(tx_dst_rdy_i));
   
   // ////////////////////////////////////////////
   // RX Data Path

   wire [17:0] 	rx18_data, rx18b_data;
   wire 	rx18_src_rdy, rx18_dst_rdy, rx18b_src_rdy, rx18b_dst_rdy;
   wire [15:0] 	rx_fifo_space;
   
   fifo36_to_fifo18 f18_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f36_datain(rx_data_i), .f36_src_rdy_i(rx_src_rdy_i), .f36_dst_rdy_o(rx_dst_rdy_o),
      .f18_dataout(rx18_data), .f18_src_rdy_o(rx18_src_rdy), .f18_dst_rdy_i(rx18_dst_rdy) );

   fifo_cascade #(.WIDTH(18), .SIZE(10)) rx_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(rx18_data), .src_rdy_i(rx18_src_rdy), .dst_rdy_o(rx18_dst_rdy), .space(rx_fifo_space),
      .dataout(rx18b_data), .src_rdy_o(rx18b_src_rdy), .dst_rdy_i(rx18b_dst_rdy), .occupied());

   fifo_to_gpmc_async fifo_to_gpmc_async
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(rx18b_data), .src_rdy_i(rx18b_src_rdy), .dst_rdy_o(rx18b_dst_rdy),
      .EM_D(EM_D_fifo), .EM_NCS(EM_NCS4), .EM_NOE(EM_NOE),
      .frame_len(rx_frame_len) );

   fifo_watcher fifo_watcher
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .src_rdy1(rx18_src_rdy), .dst_rdy1(rx18_dst_rdy), .sof1(rx18_data[16]), .eof1(rx18_data[17]),
      .src_rdy2(rx18b_src_rdy), .dst_rdy2(rx18b_dst_rdy), .sof2(rx18b_data[16]), .eof2(rx18b_data[17]),
      .have_packet(rx_have_data), .length(rx_frame_len), .bus_error(bus_error_rx) );

   // ////////////////////////////////////////////
   // Control path on CS6
   
   gpmc_wb gpmc_wb
     (.EM_CLK(EM_CLK), .EM_D_in(EM_D), .EM_D_out(EM_D_wb), .EM_A(EM_A), .EM_NBE(EM_NBE),
      .EM_NCS(EM_NCS6), .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
      .wb_clk(wb_clk), .wb_rst(wb_rst),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso),
      .wb_sel_o(wb_sel_o), .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o),
      .wb_ack_i(wb_ack_i) );
   
      assign debug = 0;
   
endmodule // gpmc_async
