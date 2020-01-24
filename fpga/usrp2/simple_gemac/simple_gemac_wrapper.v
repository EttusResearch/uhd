//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


module simple_gemac_wrapper
  #(parameter RXFIFOSIZE=9,
    parameter TXFIFOSIZE=9,
    parameter RX_FLOW_CTRL=0)
   (input clk125, input reset,
    // GMII
    output GMII_GTX_CLK, output GMII_TX_EN, output GMII_TX_ER, output [7:0] GMII_TXD,
    input GMII_RX_CLK, input GMII_RX_DV, input GMII_RX_ER, input [7:0] GMII_RXD,
    
    // Client FIFO Interfaces
    input sys_clk,
    output [35:0] rx_f36_data, output rx_f36_src_rdy, input rx_f36_dst_rdy,
    input [35:0] tx_f36_data, input tx_f36_src_rdy, output tx_f36_dst_rdy,
    
    // Wishbone Interface
    input wb_clk, input wb_rst, input wb_stb, input wb_cyc, output wb_ack, input wb_we,
    input [7:0] wb_adr, input [31:0] wb_dat_i, output [31:0] wb_dat_o,
    
    // MIIM
    inout mdio, output mdc,
    output [31:0] debug);

   wire 	  clear = 0;
   wire [7:0] 	  rx_data, tx_data;
   wire 	  tx_clk, tx_valid, tx_error, tx_ack;
   wire 	  rx_clk, rx_valid, rx_error, rx_ack;
   
   wire [47:0] 	  ucast_addr, mcast_addr;
   wire 	  pass_ucast, pass_mcast, pass_bcast, pass_pause, pass_all;
   wire 	  pause_req;
   wire 	  pause_request_en, pause_respect_en;
   wire [15:0] 	  pause_time, pause_thresh, pause_time_req, rx_fifo_space;

   wire [31:0] 	  debug_state;
      
   wire 	  tx_reset, rx_reset;
   reset_sync reset_sync_tx (.clk(tx_clk),.reset_in(reset),.reset_out(tx_reset));
   reset_sync reset_sync_rx (.clk(rx_clk),.reset_in(reset),.reset_out(rx_reset));
   
   simple_gemac simple_gemac
     (.clk125(clk125),  .reset(reset),
      .GMII_GTX_CLK(GMII_GTX_CLK), .GMII_TX_EN(GMII_TX_EN),  
      .GMII_TX_ER(GMII_TX_ER), .GMII_TXD(GMII_TXD),
      .GMII_RX_CLK(GMII_RX_CLK), .GMII_RX_DV(GMII_RX_DV),  
      .GMII_RX_ER(GMII_RX_ER), .GMII_RXD(GMII_RXD),
      .pause_req(RX_FLOW_CTRL ? pause_req : 1'b0), .pause_time_req(RX_FLOW_CTRL ? pause_time_req : 16'd0), 
      .pause_respect_en(pause_respect_en),
      .ucast_addr(ucast_addr), .mcast_addr(mcast_addr),
      .pass_ucast(pass_ucast), .pass_mcast(pass_mcast), .pass_bcast(pass_bcast), 
      .pass_pause(pass_pause), .pass_all(pass_all),
      .rx_clk(rx_clk), .rx_data(rx_data),
      .rx_valid(rx_valid), .rx_error(rx_error), .rx_ack(rx_ack),
      .tx_clk(tx_clk), .tx_data(tx_data), 
      .tx_valid(tx_valid), .tx_error(tx_error), .tx_ack(tx_ack),
      .debug(debug_state)
      );
   
   simple_gemac_wb simple_gemac_wb
     (.wb_clk(wb_clk), .wb_rst(wb_rst),
      .wb_cyc(wb_cyc), .wb_stb(wb_stb), .wb_ack(wb_ack), .wb_we(wb_we),
      .wb_adr(wb_adr), .wb_dat_i(wb_dat_i), .wb_dat_o(wb_dat_o),
      .mdio(mdio), .mdc(mdc),
      .ucast_addr(ucast_addr), .mcast_addr(mcast_addr),
      .pass_ucast(pass_ucast), .pass_mcast(pass_mcast), .pass_bcast(pass_bcast), 
      .pass_pause(pass_pause), .pass_all(pass_all), 
      .pause_respect_en(pause_respect_en), .pause_request_en(pause_request_en),
      .pause_time(pause_time), .pause_thresh(pause_thresh) );

   // RX FIFO Chain
   wire 	  rx_ll_sof, rx_ll_eof, rx_ll_src_rdy, rx_ll_dst_rdy;
   wire [7:0] 	  rx_ll_data;
   
   wire [18:0] 	  rx_f19_data_int1, rx_f19_data_int2;
   wire 	  rx_f19_src_rdy_int1, rx_f19_dst_rdy_int1, rx_f19_src_rdy_int2, rx_f19_dst_rdy_int2;
   wire [35:0] 	  rx_f36_data_int;
   wire 	  rx_f36_src_rdy_int, rx_f36_dst_rdy_int;
   
   rxmac_to_ll8 rx_adapt
     (.clk(rx_clk), .reset(rx_reset), .clear(0),
      .rx_data(rx_data), .rx_valid(rx_valid), .rx_error(rx_error), .rx_ack(rx_ack),
      .ll_data(rx_ll_data), .ll_sof(rx_ll_sof), .ll_eof(rx_ll_eof), .ll_error(),  // error also encoded in sof/eof
      .ll_src_rdy(rx_ll_src_rdy), .ll_dst_rdy(rx_ll_dst_rdy));

   ll8_to_fifo19 ll8_to_fifo19
     (.clk(rx_clk), .reset(rx_reset), .clear(0),
      .ll_data(rx_ll_data), .ll_sof(rx_ll_sof), .ll_eof(rx_ll_eof),
      .ll_src_rdy(rx_ll_src_rdy), .ll_dst_rdy(rx_ll_dst_rdy),
      .f19_data(rx_f19_data_int1), .f19_src_rdy_o(rx_f19_src_rdy_int1), .f19_dst_rdy_i(rx_f19_dst_rdy_int1));

   fifo19_rxrealign fifo19_rxrealign
     (.clk(rx_clk), .reset(rx_reset), .clear(0),
      .datain(rx_f19_data_int1), .src_rdy_i(rx_f19_src_rdy_int1), .dst_rdy_o(rx_f19_dst_rdy_int1),
      .dataout(rx_f19_data_int2), .src_rdy_o(rx_f19_src_rdy_int2), .dst_rdy_i(rx_f19_dst_rdy_int2) );

   fifo19_to_fifo36 rx_fifo19_to_fifo36
     (.clk(rx_clk), .reset(rx_reset), .clear(0),
      .f19_datain(rx_f19_data_int2),  .f19_src_rdy_i(rx_f19_src_rdy_int2), .f19_dst_rdy_o(rx_f19_dst_rdy_int2),
      .f36_dataout(rx_f36_data_int), .f36_src_rdy_o(rx_f36_src_rdy_int), .f36_dst_rdy_i(rx_f36_dst_rdy_int) );

   fifo_2clock_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_2clk_fifo
     (.wclk(rx_clk), .datain(rx_f36_data_int), 
      .src_rdy_i(rx_f36_src_rdy_int), .dst_rdy_o(rx_f36_dst_rdy_int), .space(rx_fifo_space),
      .rclk(sys_clk), .dataout(rx_f36_data), 
      .src_rdy_o(rx_f36_src_rdy), .dst_rdy_i(rx_f36_dst_rdy), .occupied(), .arst(reset));
   
   // TX FIFO Chain
   wire 	  tx_ll_sof, tx_ll_eof, tx_ll_src_rdy, tx_ll_dst_rdy;
   wire [7:0] 	  tx_ll_data;
   wire [35:0] 	  tx_f36_data_int1, tx_f36_data_int2;
   wire 	  tx_f36_src_rdy_int1, tx_f36_dst_rdy_int1, tx_f36_src_rdy_int2, tx_f36_dst_rdy_int2;
   
   fifo_2clock_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_2clk_fifo
     (.wclk(sys_clk), .datain(tx_f36_data), .src_rdy_i(tx_f36_src_rdy), .dst_rdy_o(tx_f36_dst_rdy), .space(),
      .rclk(tx_clk), .dataout(tx_f36_data_int1), .src_rdy_o(tx_f36_src_rdy_int1), .dst_rdy_i(tx_f36_dst_rdy_int1), .occupied(), 
      .arst(reset));

   ethtx_realign ethtx_realign
     (.clk(tx_clk), .reset(tx_reset), .clear(clear),
      .datain(tx_f36_data_int1), .src_rdy_i(tx_f36_src_rdy_int1), .dst_rdy_o(tx_f36_dst_rdy_int1),
      .dataout(tx_f36_data_int2), .src_rdy_o(tx_f36_src_rdy_int2), .dst_rdy_i(tx_f36_dst_rdy_int2) );
     
   fifo36_to_ll8 fifo36_to_ll8
     (.clk(tx_clk), .reset(tx_reset), .clear(clear),
      .f36_data(tx_f36_data_int2), .f36_src_rdy_i(tx_f36_src_rdy_int2), .f36_dst_rdy_o(tx_f36_dst_rdy_int2),
      .ll_data(tx_ll_data), .ll_sof(tx_ll_sof), .ll_eof(tx_ll_eof),
      .ll_src_rdy(tx_ll_src_rdy), .ll_dst_rdy(tx_ll_dst_rdy));

   ll8_to_txmac ll8_to_txmac
     (.clk(tx_clk), .reset(tx_reset), .clear(clear),
      .ll_data(tx_ll_data), .ll_sof(tx_ll_sof), .ll_eof(tx_ll_eof),
      .ll_src_rdy(tx_ll_src_rdy), .ll_dst_rdy(tx_ll_dst_rdy),
      .tx_data(tx_data), .tx_valid(tx_valid), .tx_error(tx_error), .tx_ack(tx_ack));

   // Flow Control
   generate
      if(RX_FLOW_CTRL==1)
	flow_ctrl_rx flow_ctrl_rx
	  (.pause_request_en(pause_request_en), .pause_time(pause_time), .pause_thresh(pause_thresh),
	   .rx_clk(rx_clk), .rx_reset(rx_reset), .rx_fifo_space(rx_fifo_space),
	   .tx_clk(tx_clk), .tx_reset(tx_reset), .pause_req(pause_req), .pause_time_req(pause_time_req));
   endgenerate
   
   wire [31:0] 	  debug_tx, debug_rx;

   assign debug_tx  = { { tx_ll_data },
			{ tx_ll_sof, tx_ll_eof, tx_ll_src_rdy, tx_ll_dst_rdy, 4'b0 },
			{ tx_valid, tx_error, tx_ack, tx_f36_src_rdy_int1, tx_f36_dst_rdy_int1, tx_f36_data_int1[34:32]},
			{ tx_data} };
   assign debug_rx  = { { rx_ll_data },
			{ rx_ll_sof, rx_ll_eof, rx_ll_src_rdy, rx_ll_dst_rdy, 4'b0 },
			{ rx_valid, rx_error, rx_ack, rx_f36_src_rdy_int, rx_f36_dst_rdy_int, rx_f36_data_int[34:32]},
			{ rx_data} };

   assign debug  = debug_rx;
   
endmodule // simple_gemac_wrapper

