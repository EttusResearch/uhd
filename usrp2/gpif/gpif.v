//////////////////////////////////////////////////////////////////////////////////

module gpif
  #(parameter TXFIFOSIZE = 11, parameter RXFIFOSIZE = 11)
   (// GPIF signals
    input gpif_clk,
    inout [15:0] gpif_d, input [3:0] gpif_ctl, output [3:0] gpif_rdy,
    input [2:0] gpif_misc,
    
    // Wishbone signals
    input wb_clk, input wb_rst,
    output [10:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    
    // FIFO interface
    input fifo_clk, input fifo_rst,
    output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
    input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,
    
    output [31:0] debug0, output [31:0] debug1
    );

   wire 	  WR = gpif_ctl[0];
   wire 	  RD = gpif_ctl[1];
   wire 	  OE = gpif_ctl[2];
   wire 	  have_space, have_pkt_rdy;
   wire [15:0] 	  gpif_dat_out;
   assign gpif_dat = OE ? gpif_dat_out : 16'bz;

   assign gpif_rdy[0] = have_space;
   assign gpif_rdy[1] = have_pkt_rdy;

   wire [15:0] 	  gpif_d_copy = gpif_d;
   
   assign debug0 = {11'd0, WR, RD, OE, have_space, have_pkt_rdy, gpif_d_copy};
   assign debug1 = 32'd0;
   
   // ////////////////////////////////////////////////////////////////////
   // TX Side
   
   wire [18:0] 	  tx19_data;
   wire 	  tx19_src_rdy, tx19_dst_rdy;
   wire [35:0] 	  tx36_data, tx36b_data, tx36c_data;
   wire 	  tx36_src_rdy, tx36_dst_rdy, tx36b_src_rdy, tx36b_dst_rdy, tx36c_src_rdy, tx36c_dst_rdy;
   wire [35:0] 	  ctrl_data;
   wire 	  ctrl_src_rdy, ctrl_dst_rdy;
   
   gpif_wr gpif_wr
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst), 
      .gpif_data(gpif_d), .gpif_wr(WR), .have_space(have_space),
      .sys_clk(fifo_clk), .sys_rst(fifo_rst),
      .data_o(tx19_data), .src_rdy_o(tx19_src_rdy), .dst_rdy_i(tx_19_dst_rdy),
      .debug() );

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f19_datain(tx19_data), .f19_src_rdy_i(tx19_src_rdy), .f19_dst_rdy_o(tx19_dst_rdy),
      .f36_dataout(tx36_data), .f36_src_rdy_o(tx36_src_rdy), .f36_dst_rdy_i(tx36_dst_rdy));
   
   fifo_short #(.WIDTH(36)) tx_sfifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(tx36_data), .src_rdy_i(tx36_src_rdy), .dst_rdy_o(tx36_dst_rdy),
      .dataout(tx36b_data), .src_rdy_o(tx36b_src_rdy), .dst_rdy_i(tx36b_dst_rdy));

   fifo36_demux #(.match_data(32'h1000_0000), .match_mask(32'hF000_0000)) tx_demux
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(tx36b_data), .src_rdy_i(tx36b_src_rdy), .dst_rdy_o(tx36b_dst_rdy),
      .data0_o(ctrl_data), .src0_rdy_o(ctrl_src_rdy), .dst0_rdy_i(ctrl_dst_rdy),
      .data1_o(tx36c_data), .src1_rdy_o(tx36c_src_rdy), .dst1_rdy_i(tx36c_dst_rdy));
   
   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(tx36c_data), .src_rdy_i(tx36c_src_rdy), .dst_rdy_o(tx36c_dst_rdy),
      .dataout(tx_data_o), .src_rdy_o(tx_src_rdy_o), .dst_rdy_i(tx_dst_rdy_i));

   // ////////////////////////////////////////////////////////////////////
   // RX Side

   wire [35:0] 	  rx36_data, rx36b_data, rx36c_data;
   wire 	  rx36_src_rdy, rx36_dst_rdy, rx36b_src_rdy, rx36b_dst_rdy, rx36c_src_rdy, rx36c_dst_rdy;
   wire [18:0] 	  rx19_data;
   wire 	  rx19_src_rdy, rx19_dst_rdy;
   wire [35:0] 	  resp_data;
   wire 	  resp_src_rdy, resp_dst_rdy;
   
   fifo_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(rx_data_i), .src_rdy_i(rx_src_rdy_i), .dst_rdy_o(rx_dst_rdy_o),
      .dataout(rx36c_data), .src_rdy_o(rx36c_src_rdy), .dst_rdy_i(rx36c_dst_rdy));

   fifo36_mux #(.prio(1)) rx_mux
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data0_i(resp_data), .src0_rdy_i(resp_src_rdy), .dst0_rdy_o(resp_dst_rdy),
      .data1_i(rx36c_data), .src1_rdy_i(rx36c_src_rdy), .dst1_rdy_o(rx36c_dst_rdy),
      .data_o(rx36b_data), .src_rdy_o(rx36b_src_rdy), .dst_rdy_i(rx36b_dst_rdy));
   
   fifo_short #(.WIDTH(36)) rx_sfifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(rx36b_data), .src_rdy_i(rx36b_src_rdy), .dst_rdy_o(rx36b_dst_rdy),
      .dataout(rx36_data), .src_rdy_o(rx36_src_rdy), .dst_rdy_i(rx36_dst_rdy));
   
   fifo36_to_fifo19 #(.LE(1)) f36_to_f19   // FIXME Endianness?
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f36_datain(rx36_data), .f36_src_rdy_i(rx36_src_rdy), .f36_dst_rdy_o(rx36_dst_rdy),
      .f19_dataout(rx19_data), .f19_src_rdy_o(rx19_src_rdy), .f19_dst_rdy_i(rx19_dst_rdy) );

   gpif_rd gpif_rd
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
      .gpif_data(gpif_dat_out), .gpif_rd(RD), .have_pkt_rdy(have_pkt_rdy),
      .sys_clk(fifo_clk), .sys_rst(fifo_rst),
      .data_i(rx19_data), .src_rdy_i(rx19_src_rdy), .dst_rdy_o(rx19_dst_rdy));

   // ////////////////////////////////////////////////////////////////////
   // FIFO to Wishbone interface

   fifo_to_wb fifo_to_wb
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(ctrl_data), .src_rdy_i(ctrl_src_rdy), .dst_rdy_o(ctrl_dst_rdy),
      .data_o(resp_data), .src_rdy_o(resp_src_rdy), .dst_rdy_i(resp_dst_rdy),
      .wb_adr_o(), .wb_dat_mosi(), .wb_dat_miso(),
      .wb_sel_o(), .wb_cyc_o(), .wb_stb_o(), .wb_we_o(), .wb_ack_i(),
      .debug0(), .debug1());
   
endmodule // gpif
