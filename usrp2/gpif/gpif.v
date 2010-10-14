//////////////////////////////////////////////////////////////////////////////////

module gpif
  #(parameter TXFIFOSIZE = 11, parameter RXFIFOSIZE = 11)
   (// GPIF signals
    input gpif_clk, input gpif_rst,
    inout [15:0] gpif_d, input [3:0] gpif_ctl, output [3:0] gpif_rdy,
    input [2:0] gpif_misc,
    
    // Wishbone signals
    input wb_clk, input wb_rst,
    output [15:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    input [7:0] triggers,
    
    // FIFO interface
    input fifo_clk, input fifo_rst,
    output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
    input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,
    
    output [31:0] debug0, output [31:0] debug1
    );

   wire 	  WR = gpif_ctl[0];
   wire 	  RD = gpif_ctl[1];
   wire 	  OE = gpif_ctl[2];
   wire 	  EP = gpif_ctl[3];

   wire 	  CF, CE, DF, DE;
   
   assign gpif_rdy = { CF, CE, DF, DE };
   
   wire [15:0] 	  gpif_d_out;
   assign gpif_d = OE ? gpif_d_out : 16'bz;

   wire [15:0] 	  gpif_d_copy = gpif_d;

   wire [31:0] 	  debug_rd, debug_wr;
   
   // ////////////////////////////////////////////////////////////////////
   // TX Side
   
   wire [17:0] 	  tx18_data;
   wire 	  tx18_src_rdy, tx18_dst_rdy;
   wire [35:0] 	  tx36_data;
   wire 	  tx36_src_rdy, tx36_dst_rdy;

   wire [17:0] 	  ctrl_data;
   wire 	  ctrl_src_rdy, ctrl_dst_rdy;
   
   gpif_wr gpif_wr
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst), 
      .gpif_data(gpif_d), .gpif_wr(WR), .gpif_ep(EP),
      .gpif_full_d(DF), .gpif_full_c(CF),
      
      .sys_clk(fifo_clk), .sys_rst(fifo_rst),
      .data_o(tx18_data), .src_rdy_o(tx18_src_rdy), .dst_rdy_i(tx18_dst_rdy),
      .ctrl_o(ctrl_data), .ctrl_src_rdy_o(ctrl_src_rdy), .ctrl_dst_rdy_i(ctrl_dst_rdy),
      .debug(debug_wr) );

   fifo19_to_fifo36 #(.LE(1)) f18_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f19_datain({1'b0,tx18_data}), .f19_src_rdy_i(tx18_src_rdy), .f19_dst_rdy_o(tx18_dst_rdy),
      .f36_dataout(tx36_data), .f36_src_rdy_o(tx36_src_rdy), .f36_dst_rdy_i(tx36_dst_rdy));
   
   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(tx36_data), .src_rdy_i(tx36_src_rdy), .dst_rdy_o(tx36_dst_rdy),
      .dataout(tx_data_o), .src_rdy_o(tx_src_rdy_o), .dst_rdy_i(tx_dst_rdy_i));

   // ////////////////////////////////////////////////////////////////////
   // RX Side

   wire [35:0] 	  rx36_data;
   wire 	  rx36_src_rdy, rx36_dst_rdy;
   wire [17:0] 	  rx18_data;
   wire 	  rx18_src_rdy, rx18_dst_rdy;
   wire [17:0] 	  resp_data;
   wire 	  resp_src_rdy, resp_dst_rdy;
   
   fifo_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .datain(rx_data_i), .src_rdy_i(rx_src_rdy_i), .dst_rdy_o(rx_dst_rdy_o),
      .dataout(rx36_data), .src_rdy_o(rx36_src_rdy), .dst_rdy_i(rx36_dst_rdy));

   fifo36_to_fifo19 #(.LE(1)) f36_to_f18   // FIXME Endianness?
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f36_datain(rx36_data), .f36_src_rdy_i(rx36_src_rdy), .f36_dst_rdy_o(rx36_dst_rdy),
      .f19_dataout(rx18_data), .f19_src_rdy_o(rx18_src_rdy), .f19_dst_rdy_i(rx18_dst_rdy) );

   gpif_rd gpif_rd
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
      .gpif_data(gpif_d_out), .gpif_rd(RD), .gpif_ep(EP),
      .gpif_empty_d(DE), .gpif_empty_c(CE),
      
      .sys_clk(fifo_clk), .sys_rst(fifo_rst),
      .data_i(rx18_data), .src_rdy_i(rx18_src_rdy), .dst_rdy_o(rx18_dst_rdy),
      .resp_i(resp_data), .resp_src_rdy_i(resp_src_rdy), .resp_dst_rdy_o(resp_dst_rdy),
      .debug(debug_rd) );

   // ////////////////////////////////////////////////////////////////////
   // FIFO to Wishbone interface

   fifo_to_wb fifo_to_wb
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(ctrl_data), .src_rdy_i(ctrl_src_rdy), .dst_rdy_o(ctrl_dst_rdy),
      .data_o(resp_data), .src_rdy_o(resp_src_rdy), .dst_rdy_i(resp_dst_rdy),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso), .wb_sel_o(wb_sel_o), 
      .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o), .wb_ack_i(wb_ack_i),
      .triggers(triggers),
      .debug0(), .debug1());

   // ////////////////////////////////////////////
   //    DEBUG
   
   // Loopback for testing
   //assign resp_data = ctrl_data;
   //assign resp_src_rdy = ctrl_src_rdy;
   //assign ctrl_dst_rdy = resp_dst_rdy;
   
   assign debug0 = { 5'd0, gpif_misc[2:0], gpif_ctl[3:0], gpif_rdy[3:0], gpif_d_copy[15:0] };
   assign debug1 = { { debug_rd[15:8] },
		     { debug_rd[7:0] },
		     { rx_src_rdy_i, rx_dst_rdy_o, rx36_src_rdy, rx36_dst_rdy, rx18_src_rdy, rx18_dst_rdy, resp_src_rdy, resp_dst_rdy},
		     { tx_src_rdy_o, tx_dst_rdy_i, tx18_src_rdy, tx18_dst_rdy, tx36_src_rdy, tx36_dst_rdy, ctrl_src_rdy, ctrl_dst_rdy} };
   
endmodule // gpif
