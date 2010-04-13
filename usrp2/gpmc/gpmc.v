//////////////////////////////////////////////////////////////////////////////////

module gpmc
  (// GPMC signals
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,

   // GPIOs for FIFO signalling
   output rx_have_data, output tx_have_space,
   
   // Wishbone signals
   input wb_clk, input wb_rst,
   output [10:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
   output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,

   // FIFO interface
   input fifo_clk, input fifo_rst,
   output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
   input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,
   
   output [31:0] debug
   );

   wire 	EM_output_enable = (~EM_NOE & (~EM_NCS4 | ~EM_NCS6));
   wire [15:0] 	EM_D_ram;
   wire [15:0] 	EM_D_wb;

   assign EM_D = ~EM_output_enable ? 16'bz : ~EM_NCS4 ? EM_D_ram : EM_D_wb;

   // CS4 is RAM_2PORT for DATA PATH (high-speed data)
   //    Writes go into one RAM, reads come from the other
   // CS6 is for CONTROL PATH (wishbone)

   // ////////////////////////////////////////////
   // TX Data Path

   wire [17:0] 	tx18_data, tx18b_data;
   wire 	tx18_src_rdy, tx18_dst_rdy, tx18b_src_rdy, tx18b_dst_rdy;
   wire [15:0] 	tx_fifo_space, tx_frame_len;

   assign tx_frame_len = 10;
   
   gpmc_to_fifo gpmc_to_fifo
     (.EM_CLK(EM_CLK), .EM_D(EM_D), .EM_NBE(EM_NBE), .EM_NCS(EM_NCS4), .EM_NWE(EM_NWE),
      .fifo_clk(fifo_clk), .fifo_rst(fifo_rst),
      .data_o(tx18_data), .src_rdy_o(tx18_src_rdy), .dst_rdy_i(tx18_dst_rdy),
      .frame_len(tx_frame_len), .fifo_space(tx_fifo_space), .fifo_ready(tx_have_space));
   
   fifo_cascade #(.WIDTH(18), .SIZE(10)) tx_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(tx18_data), .src_rdy_i(tx18_src_rdy), .dst_rdy_o(tx18_dst_rdy), .space(tx_fifo_space),
      .dataout(tx18b_data), .src_rdy_o(tx18b_src_rdy), .dst_rdy_i(tx18b_dst_rdy));

   fifo19_to_fifo36 f19_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f19_datain({1'b0,tx18b_data}), .f19_src_rdy_i(tx18b_src_rdy), .f19_dst_rdy_o(tx18b_dst_rdy),
      .f36_dataout(tx_data_o), .f36_src_rdy_o(tx_src_rdy_o), .f36_dst_rdy_i(tx_dst_rdy_i));
   

   // ////////////////////////////////////////////
   // RX Data Path
   wire 	read_sel_rx, write_sel_rx, clear_rx;
   wire 	read_done_rx;
      
   edge_sync #(.POSEDGE(0)) 
   edge_sync_rx(.clk(wb_clk), .rst(wb_rst), 
		.sig(~EM_NCS4 & ~EM_NOE & (EM_A == 10'h3FF)), .trig(read_done_rx));
   
   ram_2port_mixed_width buffer_rx
     (.clk16(wb_clk), .en16(~EM_NCS4), .we16(0), .addr16({read_sel_rx,EM_A}), .di16(0), .do16(EM_D_ram),
      .clk32(ram_clk), .en32(write_en), .we32(write_en), .addr32({write_sel_rx,write_addr}), .di32(write_data), .do32());

   dbsm dbsm_rx(.clk(wb_clk), .reset(wb_rst), .clear(clear_rx),
		 .read_sel(read_sel_rx), .read_ready(rx_have_data), .read_done(read_done_rx),
		 .write_sel(write_sel_rx), .write_ready(write_ready), .write_done(write_done));

   // ////////////////////////////////////////////
   // Control path on CS6
   
   gpmc_wb gpmc_wb
     (.EM_CLK(EM_CLK), .EM_D(EM_D_wb), .EM_A(EM_A), .EM_NBE(EM_NBE),
      .EM_NCS(EM_NCS6), .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
      .wb_clk(wb_clk), .wb_rst(wb_rst),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso),
      .wb_sel_o(wb_sel_o), .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o),
      .wb_ack_i(wb_ack_i) );
   
      assign debug = 0;
   
endmodule // gpmc
