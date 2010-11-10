
// Modified 2010 by Matt Ettus to remove old verilog style and
// allow 16-bit operation

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  spi_top.v                                                   ////
////                                                              ////
////  This file is part of the SPI IP core project                ////
////  http://www.opencores.org/projects/spi/                      ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Srot (simons@opencores.org)                     ////
////                                                              ////
////  All additional information is avaliable in the Readme.txt   ////
////  file.                                                       ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2002 Authors                                   ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////


`include "spi_defines.v"

module spi_top16
  (input wb_clk_i, input wb_rst_i, 
   input [4:0] wb_adr_i, 
   input [15:0] wb_dat_i, 
   output reg [15:0] wb_dat_o, 
   input [1:0] wb_sel_i,
   input wb_we_i, input wb_stb_i, input wb_cyc_i, 
   output reg wb_ack_o, output wb_err_o, output reg wb_int_o,
   
   // SPI signals
   output [15:0] ss_pad_o, output sclk_pad_o, output mosi_pad_o, input miso_pad_i);
   
   // Internal signals
   reg [15:0] divider;          // Divider register
   reg [`SPI_CTRL_BIT_NB-1:0] ctrl;             // Control and status register
   reg [15:0] 		      ss;               // Slave select register
   reg [31:0] 		      wb_dat;           // wb data out
   wire [31:0] 		      rx;               // Rx register
   wire 		      rx_negedge;       // miso is sampled on negative edge
   wire 		      tx_negedge;       // mosi is driven on negative edge
   wire [`SPI_CHAR_LEN_BITS-1:0] char_len;         // char len
   wire 			 go;               // go
   wire 			 lsb;              // lsb first on line
   wire 			 ie;               // interrupt enable
   wire 			 ass;              // automatic slave select
   wire 			 spi_divider_sel;  // divider register select
   wire 			 spi_ctrl_sel;     // ctrl register select
   wire [3:0] 			 spi_tx_sel;       // tx_l register select
   wire 			 spi_ss_sel;       // ss register select
   wire 			 tip;              // transfer in progress
   wire 			 pos_edge;         // recognize posedge of sclk
   wire 			 neg_edge;         // recognize negedge of sclk
   wire 			 last_bit;         // marks last character bit
   
   // Address decoder
   assign spi_divider_sel = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_DIVIDE);
   assign spi_ctrl_sel    = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_CTRL);
   assign spi_tx_sel[0]   = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_TX_0);
   assign spi_tx_sel[1]   = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_TX_1);
   assign spi_tx_sel[2]   = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_TX_2);
   assign spi_tx_sel[3]   = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_TX_3);
   assign spi_ss_sel      = wb_cyc_i & wb_stb_i & (wb_adr_i[4:2] == `SPI_SS);
   
   always @(wb_adr_i or rx or ctrl or divider or ss)
     case (wb_adr_i[4:2])   
       `SPI_RX_0:    wb_dat = rx[31:0];
       `SPI_CTRL:    wb_dat = {{32-`SPI_CTRL_BIT_NB{1'b0}}, ctrl};
       `SPI_DIVIDE:  wb_dat = {16'b0, divider};
       `SPI_SS:      wb_dat = {16'b0, ss};
       default : wb_dat = 32'd0;
     endcase // case (wb_adr_i[4:2])
   
   always @(posedge wb_clk_i)
     if (wb_rst_i)
       wb_dat_o <= 32'b0;
     else
       wb_dat_o <= wb_adr_i[1] ? wb_dat[31:16] : wb_dat[15:0];
   
   always @(posedge wb_clk_i)
     if (wb_rst_i)
       wb_ack_o <= 1'b0;
     else
       wb_ack_o <= wb_cyc_i & wb_stb_i & ~wb_ack_o;
   
   assign wb_err_o = 1'b0;
   
   // Interrupt
   always @(posedge wb_clk_i)
     if (wb_rst_i)
       wb_int_o <= 1'b0;
     else if (ie && tip && last_bit && pos_edge)
       wb_int_o <= 1'b1;
     else if (wb_ack_o)
       wb_int_o <= 1'b0;
   
   // Divider register
   always @(posedge wb_clk_i)
     if (wb_rst_i)
       divider <= 16'b0;
     else if (spi_divider_sel && wb_we_i && !tip && ~wb_adr_i[1])
       divider <= wb_dat_i;
   
   // Ctrl register
   always @(posedge wb_clk_i)
     if (wb_rst_i)
       ctrl <= {`SPI_CTRL_BIT_NB{1'b0}};
     else if(spi_ctrl_sel && wb_we_i && !tip && ~wb_adr_i[1])
       begin
          if (wb_sel_i[0])
            ctrl[7:0] <= wb_dat_i[7:0] | {7'b0, ctrl[0]};
          if (wb_sel_i[1])
            ctrl[`SPI_CTRL_BIT_NB-1:8] <= wb_dat_i[`SPI_CTRL_BIT_NB-1:8];
       end
     else if(tip && last_bit && pos_edge)
       ctrl[`SPI_CTRL_GO] <= 1'b0;
   
   assign rx_negedge = ctrl[`SPI_CTRL_RX_NEGEDGE];
   assign tx_negedge = ctrl[`SPI_CTRL_TX_NEGEDGE];
   assign go         = ctrl[`SPI_CTRL_GO];
   assign char_len   = ctrl[`SPI_CTRL_CHAR_LEN];
   assign lsb        = ctrl[`SPI_CTRL_LSB];
   assign ie         = ctrl[`SPI_CTRL_IE];
   assign ass        = ctrl[`SPI_CTRL_ASS];
   
   // Slave select register
   always @(posedge wb_clk_i)
     if (wb_rst_i)
       ss <= 16'b0;
     else if(spi_ss_sel && wb_we_i && !tip & ~wb_adr_i[1])
       begin
          if (wb_sel_i[0])
            ss[7:0] <= wb_dat_i[7:0];
          if (wb_sel_i[1])
            ss[15:8] <= wb_dat_i[15:8];
       end
   
   assign ss_pad_o = ~((ss & {16{tip & ass}}) | (ss & {16{!ass}}));
   
   spi_clgen clgen (.clk_in(wb_clk_i), .rst(wb_rst_i), .go(go), .enable(tip), .last_clk(last_bit),
                    .divider(divider[`SPI_DIVIDER_LEN-1:0]), .clk_out(sclk_pad_o), .pos_edge(pos_edge), 
                    .neg_edge(neg_edge));

   wire [3:0] new_sels = { (wb_adr_i[1] & wb_sel_i[1]), (wb_adr_i[1] & wb_sel_i[0]), 
			   (~wb_adr_i[1] & wb_sel_i[1]), (~wb_adr_i[1] & wb_sel_i[0]) };
   
   
   spi_shift shift (.clk(wb_clk_i), .rst(wb_rst_i), .len(char_len[`SPI_CHAR_LEN_BITS-1:0]),
                    .latch(spi_tx_sel[3:0] & {4{wb_we_i}}), .byte_sel(new_sels), .lsb(lsb), 
                    .go(go), .pos_edge(pos_edge), .neg_edge(neg_edge), 
                    .rx_negedge(rx_negedge), .tx_negedge(tx_negedge),
                    .tip(tip), .last(last_bit), 
                    .p_in({wb_dat_i,wb_dat_i}), .p_out(rx), 
                    .s_clk(sclk_pad_o), .s_in(miso_pad_i), .s_out(mosi_pad_o));

endmodule // spi_top16
