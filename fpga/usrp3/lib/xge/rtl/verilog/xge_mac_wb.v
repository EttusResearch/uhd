//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "xge_mac_wb.v"                                       ////
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


module xge_mac_wb (
  // Outputs
  xgmii_txd, xgmii_txc, pkt_tx_full,
  pkt_rx_val, pkt_rx_sop, pkt_rx_mod, pkt_rx_err, pkt_rx_eop,
  pkt_rx_data, pkt_rx_avail,
  wb_int_o, wb_dat_o, wb_ack_o,
  mdc, mdio_out, mdio_tri, xge_gpo,

  // Inputs
  xgmii_rxd, xgmii_rxc,
  wb_we_i, wb_stb_i, wb_rst_i, wb_dat_i,
  wb_cyc_i, wb_clk_i, wb_adr_i,
  reset_xgmii_tx_n, reset_xgmii_rx_n, reset_156m25_n,
  pkt_tx_val, pkt_tx_sop, pkt_tx_mod, pkt_tx_eop,
  pkt_tx_data, pkt_rx_ren,
  clk_xgmii_tx, clk_xgmii_rx, clk_156m25,
  mdio_in, xge_gpi
);

  input                   clk_156m25;             // To rx_dq0 of rx_dequeue.v, ...
  input                   clk_xgmii_rx;           // To rx_eq0 of rx_enqueue.v, ...
  input                   clk_xgmii_tx;           // To tx_dq0 of tx_dequeue.v, ...
  input                   pkt_rx_ren;             // To rx_dq0 of rx_dequeue.v
  input [63:0]            pkt_tx_data;            // To tx_eq0 of tx_enqueue.v
  input                   pkt_tx_eop;             // To tx_eq0 of tx_enqueue.v
  input [2:0]             pkt_tx_mod;             // To tx_eq0 of tx_enqueue.v
  input                   pkt_tx_sop;             // To tx_eq0 of tx_enqueue.v
  input                   pkt_tx_val;             // To tx_eq0 of tx_enqueue.v
  input                   reset_156m25_n;         // To rx_dq0 of rx_dequeue.v, ...
  input                   reset_xgmii_rx_n;       // To rx_eq0 of rx_enqueue.v, ...
  input                   reset_xgmii_tx_n;       // To tx_dq0 of tx_dequeue.v, ...
  input [7:0]             wb_adr_i;               // To wishbone_if0 of wishbone_if.v
  input                   wb_clk_i;               // To sync_clk_wb0 of sync_clk_wb.v, ...
  input                   wb_cyc_i;               // To wishbone_if0 of wishbone_if.v
  input [31:0]            wb_dat_i;               // To wishbone_if0 of wishbone_if.v
  input                   wb_rst_i;               // To sync_clk_wb0 of sync_clk_wb.v, ...
  input                   wb_stb_i;               // To wishbone_if0 of wishbone_if.v
  input                   wb_we_i;                // To wishbone_if0 of wishbone_if.v
  input [7:0]             xgmii_rxc;              // To rx_eq0 of rx_enqueue.v
  input [63:0]            xgmii_rxd;              // To rx_eq0 of rx_enqueue.v
  input 		          mdio_in;
  input [7:0]		      xge_gpi;

  output                  pkt_rx_avail;           // From rx_dq0 of rx_dequeue.v
  output [63:0]           pkt_rx_data;            // From rx_dq0 of rx_dequeue.v
  output                  pkt_rx_eop;             // From rx_dq0 of rx_dequeue.v
  output                  pkt_rx_err;             // From rx_dq0 of rx_dequeue.v
  output [2:0]            pkt_rx_mod;             // From rx_dq0 of rx_dequeue.v
  output                  pkt_rx_sop;             // From rx_dq0 of rx_dequeue.v
  output                  pkt_rx_val;             // From rx_dq0 of rx_dequeue.v
  output                  pkt_tx_full;            // From tx_eq0 of tx_enqueue.v
  output                  wb_ack_o;               // From wishbone_if0 of wishbone_if.v
  output [31:0]           wb_dat_o;               // From wishbone_if0 of wishbone_if.v
  output                  wb_int_o;               // From wishbone_if0 of wishbone_if.v
  output [7:0]            xgmii_txc;              // From tx_dq0 of tx_dequeue.v
  output [63:0]           xgmii_txd;              // From tx_dq0 of tx_dequeue.v
  output                  mdc;
  output                  mdio_out;
  output                  mdio_tri;               // Assert to tristate driver.
  output [7:0] 	          xge_gpo;

  wire                    ctrl_tx_enable;         // From wishbone_if0 of wishbone_if.v
  wire                    ctrl_tx_enable_ctx;     // From sync_clk_xgmii_tx0 of sync_clk_xgmii_tx.v
  wire                    status_local_fault_ctx; // From sync_clk_xgmii_tx0 of sync_clk_xgmii_tx.v
  wire                    status_remote_fault_ctx;// From sync_clk_xgmii_tx0 of sync_clk_xgmii_tx.v
  wire                    status_crc_error;       // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_crc_error_tog;   // From rx_eq0 of rx_enqueue.v
  wire                    status_fragment_error;  // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_fragment_error_tog;// From rx_eq0 of rx_enqueue.v
  wire                    status_local_fault;     // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_local_fault_crx; // From fault_sm0 of fault_sm.v
  wire                    status_pause_frame_rx;  // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_pause_frame_rx_tog;// From rx_eq0 of rx_enqueue.v
  wire                    status_remote_fault;    // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_remote_fault_crx;// From fault_sm0 of fault_sm.v
  wire                    status_rxdfifo_ovflow;  // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_rxdfifo_ovflow_tog;// From rx_eq0 of rx_enqueue.v
  wire                    status_rxdfifo_udflow;  // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_rxdfifo_udflow_tog;// From rx_dq0 of rx_dequeue.v
  wire                    status_txdfifo_ovflow;  // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_txdfifo_ovflow_tog;// From tx_eq0 of tx_enqueue.v
  wire                    status_txdfifo_udflow;  // From sync_clk_wb0 of sync_clk_wb.v
  wire                    status_txdfifo_udflow_tog;// From tx_dq0 of tx_dequeue.v

  xge_mac xge_mac (
    // Clocks and Resets
    .clk_156m25             (clk_156m25),
    .clk_xgmii_rx           (clk_xgmii_rx),
    .clk_xgmii_tx           (clk_xgmii_tx),
    .reset_156m25_n         (reset_156m25_n),
    .reset_xgmii_rx_n       (reset_xgmii_rx_n),
    .reset_xgmii_tx_n       (reset_xgmii_tx_n),
    // XGMII
    .xgmii_txc              (xgmii_txc[7:0]),
    .xgmii_txd              (xgmii_txd[63:0]),
    .xgmii_rxc              (xgmii_rxc[7:0]),
    .xgmii_rxd              (xgmii_rxd[63:0]),
    // Packet interface
    .pkt_rx_avail           (pkt_rx_avail),
    .pkt_rx_data            (pkt_rx_data),
    .pkt_rx_eop             (pkt_rx_eop),
    .pkt_rx_err             (pkt_rx_err),
    .pkt_rx_mod             (pkt_rx_mod),
    .pkt_rx_sop             (pkt_rx_sop),
    .pkt_rx_val             (pkt_rx_val),
    .pkt_tx_full            (pkt_tx_full),
    // Inputs
    .pkt_rx_ren             (pkt_rx_ren),
    .pkt_tx_data            (pkt_tx_data),
    .pkt_tx_eop             (pkt_tx_eop),
    .pkt_tx_mod             (pkt_tx_mod),
    .pkt_tx_sop             (pkt_tx_sop),
    .pkt_tx_val             (pkt_tx_val),
    // Control and Status
    .ctrl_tx_enable         (ctrl_tx_enable),
    .status_crc_error       (status_crc_error_tog),
    .status_fragment_error  (status_fragment_error_tog),
    .status_txdfifo_ovflow  (status_txdfifo_ovflow_tog),
    .status_txdfifo_udflow  (status_txdfifo_udflow_tog),
    .status_rxdfifo_ovflow  (status_rxdfifo_ovflow_tog),
    .status_rxdfifo_udflow  (status_rxdfifo_udflow_tog),
    .status_pause_frame_rx  (status_pause_frame_rx_tog),
    .status_local_fault     (status_local_fault_crx),
    .status_remote_fault    (status_remote_fault_crx)
  );

  sync_clk_wb sync_clk_wb0 (
    // Outputs
    .status_crc_error      (status_crc_error),
    .status_fragment_error (status_fragment_error),
    .status_txdfifo_ovflow (status_txdfifo_ovflow),
    .status_txdfifo_udflow (status_txdfifo_udflow),
    .status_rxdfifo_ovflow (status_rxdfifo_ovflow),
    .status_rxdfifo_udflow (status_rxdfifo_udflow),
    .status_pause_frame_rx (status_pause_frame_rx),
    .status_local_fault    (status_local_fault),
    .status_remote_fault   (status_remote_fault),
    // Inputs
    .wb_clk_i              (wb_clk_i),
    .wb_rst_i              (wb_rst_i),
    .status_crc_error_tog  (status_crc_error_tog),
    .status_fragment_error_tog(status_fragment_error_tog),
    .status_txdfifo_ovflow_tog(status_txdfifo_ovflow_tog),
    .status_txdfifo_udflow_tog(status_txdfifo_udflow_tog),
    .status_rxdfifo_ovflow_tog(status_rxdfifo_ovflow_tog),
    .status_rxdfifo_udflow_tog(status_rxdfifo_udflow_tog),
    .status_pause_frame_rx_tog(status_pause_frame_rx_tog),
    .status_local_fault_crx(status_local_fault_crx),
    .status_remote_fault_crx(status_remote_fault_crx)
  );


// IJB. This module has only inputs and is treated as a black box by XST which causes a fatal error.
// Commented out. Original pupose/intent unknown.
//sync_clk_core sync_clk_core0(/*AUTOINST*/
//                             // Inputs
//                            .clk_xgmii_tx      (clk_xgmii_tx),
//                             .reset_xgmii_tx_n  (reset_xgmii_tx_n));
  wishbone_if wishbone_if0 (
    // Outputs
    .wb_dat_o              (wb_dat_o[31:0]),
    .wb_ack_o              (wb_ack_o),
    .wb_int_o              (wb_int_o),
    .ctrl_tx_enable        (ctrl_tx_enable),
    // Inputs
    .wb_clk_i              (wb_clk_i),
    .wb_rst_i              (wb_rst_i),
    .wb_adr_i              (wb_adr_i[7:0]),
    .wb_dat_i              (wb_dat_i[31:0]),
    .wb_we_i               (wb_we_i),
    .wb_stb_i              (wb_stb_i),
    .wb_cyc_i              (wb_cyc_i),
    .status_crc_error      (status_crc_error),
    .status_fragment_error (status_fragment_error),
    .status_txdfifo_ovflow (status_txdfifo_ovflow),
    .status_txdfifo_udflow (status_txdfifo_udflow),
    .status_rxdfifo_ovflow (status_rxdfifo_ovflow),
    .status_rxdfifo_udflow (status_rxdfifo_udflow),
    .status_pause_frame_rx (status_pause_frame_rx),
    .status_local_fault    (status_local_fault),
    .status_remote_fault   (status_remote_fault),
    // MDIO
    .mdc(mdc),
    .mdio_in(mdio_in),
    .mdio_out(mdio_out),
    .mdio_tri(mdio_tri),
    .xge_gpo(xge_gpo),
    .xge_gpi(xge_gpi)
  );

endmodule
