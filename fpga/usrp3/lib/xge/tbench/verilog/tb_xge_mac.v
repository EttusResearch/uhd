//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "tb_xge_mac.v"                                    ////
////                                                              ////
////  This file is part of the "10GE MAC" project                 ////
////  http://www.opencores.org/cores/xge_mac/                     ////
////                                                              ////
////  Author(s):                                                  ////
////      - A. Tanguay (antanguay@opencores.org)                  ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2008 AUTHORS. All rights reserved.             ////
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
 
 
`include "timescale.v"
`include "defines.v"
 
//`define GXB
//`define XIL
 
module tb;
 
 
/*AUTOREG*/
 
reg [7:0]     tx_buffer[0:10000];
integer       tx_length;
 
reg           clk_156m25;
reg           clk_312m50;
reg           clk_xgmii_rx;
reg           clk_xgmii_tx;
 
reg           reset_156m25_n;
reg           reset_xgmii_rx_n;
reg           reset_xgmii_tx_n;
 
reg           pkt_rx_ren;
 
reg  [63:0]   pkt_tx_data;
reg           pkt_tx_val;
reg           pkt_tx_sop;
reg           pkt_tx_eop;
reg  [2:0]    pkt_tx_mod;
 
/*AUTOWIRE*/
// Beginning of automatic wires (for undeclared instantiated-module outputs)
wire                    pkt_rx_avail;           // From dut of xge_mac.v
wire [63:0]             pkt_rx_data;            // From dut of xge_mac.v
wire                    pkt_rx_eop;             // From dut of xge_mac.v
wire                    pkt_rx_err;             // From dut of xge_mac.v
wire [2:0]              pkt_rx_mod;             // From dut of xge_mac.v
wire                    pkt_rx_sop;             // From dut of xge_mac.v
wire                    pkt_rx_val;             // From dut of xge_mac.v
wire                    pkt_tx_full;            // From dut of xge_mac.v
wire                    wb_ack_o;               // From dut of xge_mac.v
wire [31:0]             wb_dat_o;               // From dut of xge_mac.v
wire                    wb_int_o;               // From dut of xge_mac.v
wire [7:0]              xgmii_txc;              // From dut of xge_mac.v
wire [63:0]             xgmii_txd;              // From dut of xge_mac.v
// End of automatics
 
wire  [7:0]   wb_adr_i;
wire  [31:0]  wb_dat_i;
 
wire [7:0]              xgmii_rxc;
wire [63:0]             xgmii_rxd;
 
wire [3:0]              tx_dataout;
 
wire                    xaui_tx_l0_n;
wire                    xaui_tx_l0_p;
wire                    xaui_tx_l1_n;
wire                    xaui_tx_l1_p;
wire                    xaui_tx_l2_n;
wire                    xaui_tx_l2_p;
wire                    xaui_tx_l3_n;
wire                    xaui_tx_l3_p;
 
xge_mac dut(/*AUTOINST*/
            // Outputs
            .pkt_rx_avail               (pkt_rx_avail),
            .pkt_rx_data                (pkt_rx_data[63:0]),
            .pkt_rx_eop                 (pkt_rx_eop),
            .pkt_rx_err                 (pkt_rx_err),
            .pkt_rx_mod                 (pkt_rx_mod[2:0]),
            .pkt_rx_sop                 (pkt_rx_sop),
            .pkt_rx_val                 (pkt_rx_val),
            .pkt_tx_full                (pkt_tx_full),
            .wb_ack_o                   (wb_ack_o),
            .wb_dat_o                   (wb_dat_o[31:0]),
            .wb_int_o                   (wb_int_o),
            .xgmii_txc                  (xgmii_txc[7:0]),
            .xgmii_txd                  (xgmii_txd[63:0]),
            // Inputs
            .clk_156m25                 (clk_156m25),
            .clk_xgmii_rx               (clk_xgmii_rx),
            .clk_xgmii_tx               (clk_xgmii_tx),
            .pkt_rx_ren                 (pkt_rx_ren),
            .pkt_tx_data                (pkt_tx_data[63:0]),
            .pkt_tx_eop                 (pkt_tx_eop),
            .pkt_tx_mod                 (pkt_tx_mod[2:0]),
            .pkt_tx_sop                 (pkt_tx_sop),
            .pkt_tx_val                 (pkt_tx_val),
            .reset_156m25_n             (reset_156m25_n),
            .reset_xgmii_rx_n           (reset_xgmii_rx_n),
            .reset_xgmii_tx_n           (reset_xgmii_tx_n),
            .wb_adr_i                   (wb_adr_i[7:0]),
            .wb_clk_i                   (wb_clk_i),
            .wb_cyc_i                   (wb_cyc_i),
            .wb_dat_i                   (wb_dat_i[31:0]),
            .wb_rst_i                   (wb_rst_i),
            .wb_stb_i                   (wb_stb_i),
            .wb_we_i                    (wb_we_i),
            .xgmii_rxc                  (xgmii_rxc[7:0]),
            .xgmii_rxd                  (xgmii_rxd[63:0]));
 
`ifdef GXB
// Example of transceiver instance
gxb gxb(// Outputs
        .rx_ctrldetect                  ({xgmii_rxc[7],
                                          xgmii_rxc[5],
                                          xgmii_rxc[3],
                                          xgmii_rxc[1],
                                          xgmii_rxc[6],
                                          xgmii_rxc[4],
                                          xgmii_rxc[2],
                                          xgmii_rxc[0]}),
        .rx_dataout                     ({xgmii_rxd[63:56],
                                          xgmii_rxd[47:40],
                                          xgmii_rxd[31:24],
                                          xgmii_rxd[15:8],
                                          xgmii_rxd[55:48],
                                          xgmii_rxd[39:32],
                                          xgmii_rxd[23:16],
                                          xgmii_rxd[7:0]}),
        .tx_dataout                     (tx_dataout[3:0]),
        // Inputs
        .pll_inclk                      (clk_156m25),
        .rx_analogreset                 (~reset_156m25_n),
        .rx_cruclk                      ({clk_156m25, clk_156m25, clk_156m25, clk_156m25}),
        .rx_datain                      (tx_dataout[3:0]),
        .rx_digitalreset                (~reset_156m25_n),
        .tx_ctrlenable                  ({xgmii_txc[7],
                                          xgmii_txc[5],
                                          xgmii_txc[3],
                                          xgmii_txc[1],
                                          xgmii_txc[6],
                                          xgmii_txc[4],
                                          xgmii_txc[2],
                                          xgmii_txc[0]}),
        .tx_datain                      ({xgmii_txd[63:56],
                                          xgmii_txd[47:40],
                                          xgmii_txd[31:24],
                                          xgmii_txd[15:8],
                                          xgmii_txd[55:48],
                                          xgmii_txd[39:32],
                                          xgmii_txd[23:16],
                                          xgmii_txd[7:0]}),
        .tx_digitalreset                (~reset_156m25_n));
`endif
 
`ifdef XIL
// Example of transceiver instance
xaui_block xaui(// Outputs
                .txoutclk               (),
                .xgmii_rxd              (xgmii_rxd[63:0]),
                .xgmii_rxc              (xgmii_rxc[7:0]),
                .xaui_tx_l0_p           (xaui_tx_l0_p),
                .xaui_tx_l0_n           (xaui_tx_l0_n),
                .xaui_tx_l1_p           (xaui_tx_l1_p),
                .xaui_tx_l1_n           (xaui_tx_l1_n),
                .xaui_tx_l2_p           (xaui_tx_l2_p),
                .xaui_tx_l2_n           (xaui_tx_l2_n),
                .xaui_tx_l3_p           (xaui_tx_l3_p),
                .xaui_tx_l3_n           (xaui_tx_l3_n),
                .txlock                 (),
                .align_status           (),
                .sync_status            (),
                .mgt_tx_ready           (),
                .drp_o                  (),
                .drp_rdy                (),
                .status_vector          (),
                // Inputs
                .dclk                   (clk_156m25),
                .clk156                 (clk_156m25),
                .clk312                 (clk_312m50),
                .refclk                 (clk_156m25),
                .reset                  (~reset_156m25_n),
                .reset156               (~reset_156m25_n),
                .xgmii_txd              (xgmii_txd[63:0]),
                .xgmii_txc              (xgmii_txc[7:0]),
                .xaui_rx_l0_p           (xaui_tx_l0_p),
                .xaui_rx_l0_n           (xaui_tx_l0_n),
                .xaui_rx_l1_p           (xaui_tx_l1_p),
                .xaui_rx_l1_n           (xaui_tx_l1_n),
                .xaui_rx_l2_p           (xaui_tx_l2_p),
                .xaui_rx_l2_n           (xaui_tx_l2_n),
                .xaui_rx_l3_p           (xaui_tx_l3_p),
                .xaui_rx_l3_n           (xaui_tx_l3_n),
                .signal_detect          (4'b1111),
                .drp_addr               (7'b0),
                .drp_en                 (2'b0),
                .drp_i                  (16'b0),
                .drp_we                 (2'b0),
                .configuration_vector   (7'b0));
 
glbl glbl();
`endif
 
//---
// Unused for this testbench
 
assign wb_adr_i = 8'b0;
assign wb_clk_i = 1'b0;
assign wb_cyc_i = 1'b0;
assign wb_dat_i = 32'b0;
assign wb_rst_i = 1'b1;
assign wb_stb_i = 1'b0;
assign wb_we_i = 1'b0;
 
 
//---
// XGMII Loopback
// This test is done with loopback on XGMII or using one of the tranceiver examples
 
`ifndef GXB
  `ifndef XIL
    assign xgmii_rxc = xgmii_txc;
    assign xgmii_rxd = xgmii_txd;
  `endif
`endif
 
//---
// Clock generation
 
initial begin
    clk_156m25 = 1'b0;
    clk_xgmii_rx = 1'b0;
    clk_xgmii_tx = 1'b0;
    forever begin
        WaitPS(3200);
        clk_156m25 = ~clk_156m25;
        clk_xgmii_rx = ~clk_xgmii_rx;
        clk_xgmii_tx = ~clk_xgmii_tx;
    end
end
 
initial begin
    clk_312m50 = 1'b0;
    forever begin
        WaitPS(1600);
        clk_312m50 = ~clk_312m50;
    end
end
 
//---
// Reset Generation
 
initial begin
    reset_156m25_n = 1'b0;
    reset_xgmii_rx_n = 1'b0;
    reset_xgmii_tx_n = 1'b0;
    WaitNS(20);
    reset_156m25_n = 1'b1;
    reset_xgmii_rx_n = 1'b1;
    reset_xgmii_tx_n = 1'b1;
end
 
 
//---
// Init signals
 
initial begin
 
    for (tx_length = 0; tx_length <= 1000; tx_length = tx_length + 1) begin
        tx_buffer[tx_length] = 0;
    end
 
    pkt_rx_ren = 1'b0;
 
    pkt_tx_data = 64'b0;
    pkt_tx_val = 1'b0;
    pkt_tx_sop = 1'b0;
    pkt_tx_eop = 1'b0;
    pkt_tx_mod = 3'b0;
 
end
 
task WaitNS;
  input [31:0] delay;
    begin
        #(1000*delay);
    end
endtask
 
task WaitPS;
  input [31:0] delay;
    begin
        #(delay);
    end
endtask
 
 
//---
// Task to send a single packet
 
task TxPacket;
  integer        i;
    begin
 
        $display("Transmit packet with length: %d", tx_length);
 
        @(posedge clk_156m25);
        WaitNS(1);
        pkt_tx_val = 1'b1;
 
        for (i = 0; i < tx_length; i = i + 8) begin
 
            pkt_tx_sop = 1'b0;
            pkt_tx_eop = 1'b0;
            pkt_tx_mod = 2'b0;
 
            if (i == 0) pkt_tx_sop = 1'b1;
 
            if (i + 8 >= tx_length) begin
                pkt_tx_eop = 1'b1;
                pkt_tx_mod = tx_length % 8;
            end
 
            pkt_tx_data[`LANE7] = tx_buffer[i];
            pkt_tx_data[`LANE6] = tx_buffer[i+1];
            pkt_tx_data[`LANE5] = tx_buffer[i+2];
            pkt_tx_data[`LANE4] = tx_buffer[i+3];
            pkt_tx_data[`LANE3] = tx_buffer[i+4];
            pkt_tx_data[`LANE2] = tx_buffer[i+5];
            pkt_tx_data[`LANE1] = tx_buffer[i+6];
            pkt_tx_data[`LANE0] = tx_buffer[i+7];
 
            @(posedge clk_156m25);
            WaitNS(1);
 
        end
 
        pkt_tx_val = 1'b0;
        pkt_tx_eop = 1'b0;
        pkt_tx_mod = 3'b0;
 
    end
 
endtask
 
 
//---
// Task to read a single packet from command file and transmit
 
task CmdTxPacket;
  input [31:0] file;
  integer count;
  integer data;
  integer i;
    begin
 
        count = $fscanf(file, "%2d", tx_length);
 
        if (count == 1) begin
 
            for (i = 0; i < tx_length; i = i + 1) begin
 
                count = $fscanf(file, "%2X", data);
                if (count) begin
                    tx_buffer[i] = data;
                end
 
            end
 
            TxPacket();
 
        end
    end
 
endtask
 
 
//---
// Task to read commands from file and stop when complete
 
task ProcessCmdFile;
  integer    file_cmd;
  integer  count;
  reg [8*8-1:0] str;
    begin
 
        file_cmd = $fopen("../../tbench/verilog/packets_tx.txt", "r");
        if (!file_cmd) $stop;
 
        while (!$feof(file_cmd)) begin
 
            count = $fscanf(file_cmd, "%s", str);
       // IJB. WTF!    if (count != 1) $continue;
	   if (count === 1) // IJB. I think this is what's intended.
	     begin
	   
		$display("CMD %s", str);
 
		case (str)
 
		  "SEND_PKT":
                    begin
                       CmdTxPacket(file_cmd);
                    end
 
		endcase // case(str)
	     end
 
        end
 
        $fclose(file_cmd);
 
        WaitNS(50000);
        $stop;
 
    end
endtask
 
initial begin
    WaitNS(5000);
`ifdef XIL
    WaitNS(200000);
`endif
    ProcessCmdFile();
end
 
 
//---
// Task to read a single packet from receive interface and display
 
task RxPacket;
  reg done;
    begin
 
        done = 0;
 
        pkt_rx_ren <= 1'b1;
        @(posedge clk_156m25);
 
        while (!done) begin
 
            if (pkt_rx_val) begin
 
                if (pkt_rx_sop) begin
                    $display("\n\n------------------------");
                end
 
                $display("%x", pkt_rx_data);
 
                if (pkt_rx_eop) begin
                    done <= 1;
                    pkt_rx_ren <= 1'b0;
                end
 
                if (pkt_rx_eop) begin
                    $display("------------------------\n\n");
                end
 
            end
 
            @(posedge clk_156m25);
 
        end
 
    end
endtask
 
initial begin
 
    forever begin
 
        if (pkt_rx_avail) begin
            RxPacket();
        end
 
        @(posedge clk_156m25);
 
    end
 
end
 
endmodule
 