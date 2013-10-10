
// Adapts from internal VITA to ethernet packets.  Also handles ZPU and ethernet crossover interfaces.

module eth_interface
  #(parameter BASE=0,
    parameter XO_FIFOSIZE=10,
    parameter ZPU_FIFOSIZE=10,
    parameter VITA_FIFOSIZE=10,
    parameter ETHOUT_FIFOSIZE=10)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    // Eth ports
    output [63:0] eth_tx_tdata, output [3:0] eth_tx_tuser, output eth_tx_tlast, output eth_tx_tvalid, input eth_tx_tready,
    input [63:0] eth_rx_tdata, input [3:0] eth_rx_tuser, input eth_rx_tlast, input eth_rx_tvalid, output eth_rx_tready,
    // Vita router interface
    output [63:0] e2v_tdata, output e2v_tlast, output e2v_tvalid, input e2v_tready,
    input [63:0] v2e_tdata, input v2e_tlast, input v2e_tvalid, output v2e_tready,
    // Ethernet crossover
    output [63:0] xo_tdata, output [3:0] xo_tuser, output xo_tlast, output xo_tvalid, input xo_tready,
    input [63:0] xi_tdata, input [3:0] xi_tuser, input xi_tlast, input xi_tvalid, output xi_tready,
    // ZPU
    output [63:0] e2z_tdata, output [3:0] e2z_tuser, output e2z_tlast, output e2z_tvalid, input e2z_tready,
    input [63:0] z2e_tdata, input [3:0] z2e_tuser, input z2e_tlast, input z2e_tvalid, output z2e_tready,
    
    output [31:0] debug
    );

   wire [63:0] 	  v2ef_tdata;
   wire [3:0] 	  v2ef_tuser;
   wire 	  v2ef_tlast, v2ef_tvalid, v2ef_tready;

   // //////////////////////////////////////////////////////////////
   // Incoming Ethernet path
   //  Includes FIFO on the output going to ZPU

   wire [63:0] 	  epg_tdata_int;
   wire [3:0] 	  epg_tuser_int;
   wire 	  epg_tlast_int, epg_tvalid_int, epg_tready_int;

   axi_packet_gate #(.WIDTH(68), .SIZE(10)) packet_gater //holds 8K pkts
     (.clk(clk), .reset(reset), .clear(clear),

      .i_tdata({eth_rx_tuser, eth_rx_tdata}), .i_tlast(eth_rx_tlast),
      .i_terror(eth_rx_tuser[3]), //top bit of user bus is error
      .i_tvalid(eth_rx_tvalid), .i_tready(eth_rx_tready),

      .o_tdata({epg_tuser_int, epg_tdata_int}), .o_tlast(epg_tlast_int),
      .o_tvalid(epg_tvalid_int), .o_tready(epg_tready_int));

   wire [63:0] 	  e2z_tdata_int;
   wire [3:0] 	  e2z_tuser_int;
   wire 	  e2z_tlast_int, e2z_tvalid_int, e2z_tready_int;

   eth_dispatch #(.BASE(BASE+8)) eth_dispatch
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb), .set_addr(set_addr) , .set_data(set_data),
      .in_tdata(epg_tdata_int), .in_tuser(epg_tuser_int), .in_tlast(epg_tlast_int), .in_tvalid(epg_tvalid_int), .in_tready(epg_tready_int),
      .vita_tdata(e2v_tdata), .vita_tlast(e2v_tlast), .vita_tvalid(e2v_tvalid), .vita_tready(e2v_tready),
      .zpu_tdata(e2z_tdata_int), .zpu_tuser(e2z_tuser_int), .zpu_tlast(e2z_tlast_int), .zpu_tvalid(e2z_tvalid_int), .zpu_tready(e2z_tready_int),
      .xo_tdata(xo_tdata), .xo_tuser(xo_tuser), .xo_tlast(xo_tlast), .xo_tvalid(xo_tvalid), .xo_tready(xo_tready), // to other eth port
      .debug(debug));

   axi_fifo #(.WIDTH(69),.SIZE(ZPU_FIFOSIZE)) zpu_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({e2z_tlast_int,e2z_tuser_int,e2z_tdata_int}), .i_tvalid(e2z_tvalid_int), .i_tready(e2z_tready_int),
      .o_tdata({e2z_tlast,e2z_tuser,e2z_tdata}), .o_tvalid(e2z_tvalid), .o_tready(e2z_tready));

   // //////////////////////////////////////////////////////////////
   // Outgoing Ethernet path
   //  Includes FIFOs on path from VITA router, from ethernet crossover, and on the overall output

   wire [63:0] 	  eth_tx_tdata_int;
   wire [3:0] 	  eth_tx_tuser_int;
   wire 	  eth_tx_tlast_int, eth_tx_tvalid_int, eth_tx_tready_int;

   wire [63:0] 	  xi_tdata_int;
   wire [3:0] 	  xi_tuser_int;
   wire 	  xi_tlast_int, xi_tvalid_int, xi_tready_int;

   wire [63:0] 	  v2e_tdata_int;
   wire 	  v2e_tlast_int, v2e_tvalid_int, v2e_tready_int;
   
   axi_fifo #(.WIDTH(65),.SIZE(VITA_FIFOSIZE)) vitaout_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({v2e_tlast,v2e_tdata}), .i_tvalid(v2e_tvalid), .i_tready(v2e_tready),
      .o_tdata({v2e_tlast_int,v2e_tdata_int}), .o_tvalid(v2e_tvalid_int), .o_tready(v2e_tready_int));

   chdr_eth_framer #(.BASE(BASE)) my_eth_framer
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb), .set_addr(set_addr) , .set_data(set_data),
      .in_tdata(v2e_tdata_int), .in_tlast(v2e_tlast_int), .in_tvalid(v2e_tvalid_int), .in_tready(v2e_tready_int),
      .out_tdata(v2ef_tdata), .out_tuser(v2ef_tuser), .out_tlast(v2ef_tlast), .out_tvalid(v2ef_tvalid), .out_tready(v2ef_tready),
      .debug());

   axi_fifo #(.WIDTH(69),.SIZE(XO_FIFOSIZE)) xo_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({xi_tlast,xi_tuser,xi_tdata}), .i_tvalid(xi_tvalid), .i_tready(xi_tready),
      .o_tdata({xi_tlast_int,xi_tuser_int,xi_tdata_int}), .o_tvalid(xi_tvalid_int), .o_tready(xi_tready_int));

   axi_mux4 #(.PRIO(0), .WIDTH(68)) eth_mux
     (.clk(clk), .reset(reset), .clear(clear),
      .i0_tdata({z2e_tuser,z2e_tdata}), .i0_tlast(z2e_tlast), .i0_tvalid(z2e_tvalid), .i0_tready(z2e_tready),
      .i1_tdata({v2ef_tuser,v2ef_tdata}), .i1_tlast(v2ef_tlast), .i1_tvalid(v2ef_tvalid), .i1_tready(v2ef_tready),
      .i2_tdata({xi_tuser_int,xi_tdata_int}), .i2_tlast(xi_tlast_int), .i2_tvalid(xi_tvalid_int), .i2_tready(xi_tready_int),
      .i3_tdata(), .i3_tlast(), .i3_tvalid(1'b0), .i3_tready(),
      .o_tdata({eth_tx_tuser_int,eth_tx_tdata_int}), .o_tlast(eth_tx_tlast_int), .o_tvalid(eth_tx_tvalid_int), .o_tready(eth_tx_tready_int));

   axi_fifo #(.WIDTH(69),.SIZE(ETHOUT_FIFOSIZE)) ethout_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({eth_tx_tlast_int,eth_tx_tuser_int,eth_tx_tdata_int}), .i_tvalid(eth_tx_tvalid_int), .i_tready(eth_tx_tready_int),
      .o_tdata({eth_tx_tlast,eth_tx_tuser,eth_tx_tdata}), .o_tvalid(eth_tx_tvalid), .o_tready(eth_tx_tready));

endmodule // eth_interface
