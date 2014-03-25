// -- (c) Copyright 2011 Xilinx, Inc. All rights reserved.
// --
// -- This file contains confidential and proprietary information
// -- of Xilinx, Inc. and is protected under U.S. and 
// -- international copyright and other intellectual property
// -- laws.
// --
// -- DISCLAIMER
// -- This disclaimer is not a license and does not grant any
// -- rights to the materials distributed herewith. Except as
// -- otherwise provided in a valid license issued to you by
// -- Xilinx, and to the maximum extent permitted by applicable
// -- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// -- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// -- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// -- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// -- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// -- (2) Xilinx shall not be liable (whether in contract or tort,
// -- including negligence, or under any other theory of
// -- liability) for any loss or damage of any kind or nature
// -- related to, arising under or in connection with these
// -- materials, including for any direct, or any indirect,
// -- special, incidental, or consequential loss or damage
// -- (including loss of data, profits, goodwill, or any type of
// -- loss or damage suffered as a result of any action brought
// -- by a third party) even if such damage or loss was
// -- reasonably foreseeable or Xilinx had been advised of the
// -- possibility of the same.
// --
// -- CRITICAL APPLICATIONS
// -- Xilinx products are not designed or intended to be fail-
// -- safe, or for use in any application requiring fail-safe
// -- performance, such as life-support or safety devices or
// -- systems, Class III medical devices, nuclear facilities,
// -- applications related to the deployment of airbags, or any
// -- other applications that could lead to death, personal
// -- injury, or severe property or environmental damage
// -- (individually and collectively, "Critical
// -- Applications"). Customer assumes the sole risk and
// -- liability of any use of Xilinx products in Critical
// -- Applications, subject only to applicable laws and
// -- regulations governing limitations on product liability.
// --
// -- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// -- PART OF THIS FILE AT ALL TIMES.
//-----------------------------------------------------------------------------
//
// File name: axi_protocol_converter.v
//
// Description: 
//   This module is a bank of AXI4-Lite and AXI3 protocol converters for a vectored AXI interface.
//   The interface of this module consists of a vectored slave and master interface
//     which are each concatenations of upper-level AXI pathways,
//     plus various vectored parameters.
//   This module instantiates a set of individual protocol converter modules.
//
//-----------------------------------------------------------------------------
`timescale 1ps/1ps
`default_nettype none

module ict106_axi_protocol_converter #(
  parameter         C_FAMILY                    = "virtex6",
  parameter         C_AXI_PROTOCOL              = 0, 
  parameter integer C_IGNORE_RID               = 0,
                     // 1 = RID/BID are stored within SASD crossbar.
                     // 0 = RID/BID must be stored by axilite_conv.
  parameter integer C_AXI_ID_WIDTH              = 4,
  parameter integer C_AXI_ADDR_WIDTH            = 32,
  parameter integer C_AXI_DATA_WIDTH            = 32,
  parameter integer C_AXI_SUPPORTS_WRITE        = 1,
  parameter integer C_AXI_SUPPORTS_READ         = 1,
  parameter integer C_AXI_SUPPORTS_USER_SIGNALS = 0,
                     // 1 = Propagate all USER signals, 0 = Don’t propagate.
  parameter integer C_AXI_AWUSER_WIDTH          = 1,
  parameter integer C_AXI_ARUSER_WIDTH          = 1,
  parameter integer C_AXI_WUSER_WIDTH           = 1,
  parameter integer C_AXI_RUSER_WIDTH           = 1,
  parameter integer C_AXI_BUSER_WIDTH           = 1,
  parameter integer C_AXI3_BYPASS               = 0
) (
  // Global Signals
  input  wire                                 ACLK,
  input  wire                                 ARESETN,
  // Slave Interface Write Address Ports
  input  wire [C_AXI_ID_WIDTH-1:0]            S_AXI_AWID,
  input  wire [C_AXI_ADDR_WIDTH-1:0]          S_AXI_AWADDR,
  input  wire [8-1:0]                         S_AXI_AWLEN,
  input  wire [3-1:0]                         S_AXI_AWSIZE,
  input  wire [2-1:0]                         S_AXI_AWBURST,
  input  wire [2-1:0]                         S_AXI_AWLOCK,
  input  wire [4-1:0]                         S_AXI_AWCACHE,
  input  wire [3-1:0]                         S_AXI_AWPROT,
  input  wire [4-1:0]                         S_AXI_AWREGION,
  input  wire [4-1:0]                         S_AXI_AWQOS,
  input  wire [C_AXI_AWUSER_WIDTH-1:0]        S_AXI_AWUSER,
  input  wire                                 S_AXI_AWVALID,
  output wire                                 S_AXI_AWREADY,
  // Slave Interface Write Data Ports
  input  wire [C_AXI_ID_WIDTH-1:0]            S_AXI_WID,
  input  wire [C_AXI_DATA_WIDTH-1:0]          S_AXI_WDATA,
  input  wire [C_AXI_DATA_WIDTH/8-1:0]        S_AXI_WSTRB,
  input  wire                                 S_AXI_WLAST,
  input  wire [C_AXI_WUSER_WIDTH-1:0]         S_AXI_WUSER,
  input  wire                                 S_AXI_WVALID,
  output wire                                 S_AXI_WREADY,
  // Slave Interface Write Response Ports
  output wire [C_AXI_ID_WIDTH-1:0]            S_AXI_BID,
  output wire [2-1:0]                         S_AXI_BRESP,
  output wire [C_AXI_BUSER_WIDTH-1:0]         S_AXI_BUSER,
  output wire                                 S_AXI_BVALID,
  input  wire                                 S_AXI_BREADY,
  // Slave Interface Read Address Ports
  input  wire [C_AXI_ID_WIDTH-1:0]            S_AXI_ARID,
  input  wire [C_AXI_ADDR_WIDTH-1:0]          S_AXI_ARADDR,
  input  wire [8-1:0]                         S_AXI_ARLEN,
  input  wire [3-1:0]                         S_AXI_ARSIZE,
  input  wire [2-1:0]                         S_AXI_ARBURST,
  input  wire [2-1:0]                         S_AXI_ARLOCK,
  input  wire [4-1:0]                         S_AXI_ARCACHE,
  input  wire [3-1:0]                         S_AXI_ARPROT,
  input  wire [4-1:0]                         S_AXI_ARREGION,
  input  wire [4-1:0]                         S_AXI_ARQOS,
  input  wire [C_AXI_ARUSER_WIDTH-1:0]        S_AXI_ARUSER,
  input  wire                                 S_AXI_ARVALID,
  output wire                                 S_AXI_ARREADY,
  // Slave Interface Read Data Ports
  output wire [C_AXI_ID_WIDTH-1:0]            S_AXI_RID,
  output wire [C_AXI_DATA_WIDTH-1:0]          S_AXI_RDATA,
  output wire [2-1:0]                         S_AXI_RRESP,
  output wire                                 S_AXI_RLAST,
  output wire [C_AXI_RUSER_WIDTH-1:0]         S_AXI_RUSER,
  output wire                                 S_AXI_RVALID,
  input  wire                                 S_AXI_RREADY,
  // Master Interface Write Address Port
  output wire [C_AXI_ID_WIDTH-1:0]            M_AXI_AWID,
  output wire [C_AXI_ADDR_WIDTH-1:0]          M_AXI_AWADDR,
  output wire [8-1:0]                         M_AXI_AWLEN,
  output wire [3-1:0]                         M_AXI_AWSIZE,
  output wire [2-1:0]                         M_AXI_AWBURST,
  output wire [2-1:0]                         M_AXI_AWLOCK,
  output wire [4-1:0]                         M_AXI_AWCACHE,
  output wire [3-1:0]                         M_AXI_AWPROT,
  output wire [4-1:0]                         M_AXI_AWREGION,
  output wire [4-1:0]                         M_AXI_AWQOS,
  output wire [C_AXI_AWUSER_WIDTH-1:0]        M_AXI_AWUSER,
  output wire                                 M_AXI_AWVALID,
  input  wire                                 M_AXI_AWREADY,
  // Master Interface Write Data Ports
  output wire [C_AXI_ID_WIDTH-1:0]            M_AXI_WID,
  output wire [C_AXI_DATA_WIDTH-1:0]          M_AXI_WDATA,
  output wire [C_AXI_DATA_WIDTH/8-1:0]        M_AXI_WSTRB,
  output wire                                 M_AXI_WLAST,
  output wire [C_AXI_WUSER_WIDTH-1:0]         M_AXI_WUSER,
  output wire                                 M_AXI_WVALID,
  input  wire                                 M_AXI_WREADY,
  // Master Interface Write Response Ports
  input  wire [C_AXI_ID_WIDTH-1:0]            M_AXI_BID,
  input  wire [2-1:0]                         M_AXI_BRESP,
  input  wire [C_AXI_BUSER_WIDTH-1:0]         M_AXI_BUSER,
  input  wire                                 M_AXI_BVALID,
  output wire                                 M_AXI_BREADY,
  // Master Interface Read Address Port
  output wire [C_AXI_ID_WIDTH-1:0]            M_AXI_ARID,
  output wire [C_AXI_ADDR_WIDTH-1:0]          M_AXI_ARADDR,
  output wire [8-1:0]                         M_AXI_ARLEN,
  output wire [3-1:0]                         M_AXI_ARSIZE,
  output wire [2-1:0]                         M_AXI_ARBURST,
  output wire [2-1:0]                         M_AXI_ARLOCK,
  output wire [4-1:0]                         M_AXI_ARCACHE,
  output wire [3-1:0]                         M_AXI_ARPROT,
  output wire [4-1:0]                         M_AXI_ARREGION,
  output wire [4-1:0]                         M_AXI_ARQOS,
  output wire [C_AXI_ARUSER_WIDTH-1:0]        M_AXI_ARUSER,
  output wire                                 M_AXI_ARVALID,
  input  wire                                 M_AXI_ARREADY,
  // Master Interface Read Data Ports
  input  wire [C_AXI_ID_WIDTH-1:0]            M_AXI_RID,
  input  wire [C_AXI_DATA_WIDTH-1:0]          M_AXI_RDATA,
  input  wire [2-1:0]                         M_AXI_RRESP,
  input  wire                                 M_AXI_RLAST,
  input  wire [C_AXI_RUSER_WIDTH-1:0]         M_AXI_RUSER,
  input  wire                                 M_AXI_RVALID,
  output wire                                 M_AXI_RREADY
);

localparam P_AXI3 = 32'h1;
localparam P_AXILITE = 32'h2;
localparam P_AXILITE_SIZE = 3'b010;
localparam P_INCR = 2'b01;

generate
  if (C_AXI_PROTOCOL == P_AXILITE) begin : gen_axilite
    if (C_IGNORE_RID) begin : gen_axilite_passthru
      assign M_AXI_AWID         = 0;
      assign M_AXI_AWADDR       = S_AXI_AWADDR;
      assign M_AXI_AWLEN        = 0;
      assign M_AXI_AWSIZE       = P_AXILITE_SIZE;
      assign M_AXI_AWBURST      = P_INCR;
      assign M_AXI_AWLOCK       = 0;
      assign M_AXI_AWCACHE      = 0;
      assign M_AXI_AWPROT       = S_AXI_AWPROT;
      assign M_AXI_AWREGION     = 0;
      assign M_AXI_AWQOS        = 0;
      assign M_AXI_AWUSER       = 0;
      assign M_AXI_AWVALID      = S_AXI_AWVALID;
      assign S_AXI_AWREADY      = M_AXI_AWREADY;
      assign M_AXI_WID          = 0;
      assign M_AXI_WDATA        = S_AXI_WDATA;
      assign M_AXI_WSTRB        = S_AXI_WSTRB;
      assign M_AXI_WLAST        = 1'b1;
      assign M_AXI_WUSER        = 0;
      assign M_AXI_WVALID       = S_AXI_WVALID;
      assign S_AXI_WREADY       = M_AXI_WREADY;
      assign S_AXI_BID          = 0;
      assign S_AXI_BRESP        = M_AXI_BRESP;
      assign S_AXI_BUSER        = 0;
      assign S_AXI_BVALID       = M_AXI_BVALID;
      assign M_AXI_BREADY       = S_AXI_BREADY;
      assign M_AXI_ARID         = 0;
      assign M_AXI_ARADDR       = S_AXI_ARADDR;
      assign M_AXI_ARLEN        = 0;
      assign M_AXI_ARSIZE       = P_AXILITE_SIZE;
      assign M_AXI_ARBURST      = P_INCR;
      assign M_AXI_ARLOCK       = 0;
      assign M_AXI_ARCACHE      = 0;
      assign M_AXI_ARPROT       = S_AXI_ARPROT;
      assign M_AXI_ARREGION     = 0;
      assign M_AXI_ARQOS        = 0;
      assign M_AXI_ARUSER       = 0;
      assign M_AXI_ARVALID      = S_AXI_ARVALID;
      assign S_AXI_ARREADY      = M_AXI_ARREADY;
      assign S_AXI_RID          = 0;
      assign S_AXI_RDATA        = M_AXI_RDATA;
      assign S_AXI_RRESP        = M_AXI_RRESP;
      assign S_AXI_RLAST        = 1'b1;
      assign S_AXI_RUSER          = 0;
      assign S_AXI_RVALID       = M_AXI_RVALID;
      assign M_AXI_RREADY       = S_AXI_RREADY;
    end else begin : gen_axilite_conv
  ict106_axilite_conv #(
        .C_FAMILY                         (C_FAMILY),
        .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH),
        .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
        .C_AXI_DATA_WIDTH                 (C_AXI_DATA_WIDTH),
        .C_AXI_SUPPORTS_WRITE             (C_AXI_SUPPORTS_WRITE),
        .C_AXI_SUPPORTS_READ              (C_AXI_SUPPORTS_READ),
        .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
        .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
        .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
        .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
        .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH)
      ) axilite_conv_inst (
        .ARESETN                          (ARESETN),
        .ACLK                             (ACLK),
        .S_AXI_AWID                       (S_AXI_AWID),
        .S_AXI_AWADDR                     (S_AXI_AWADDR),
        .S_AXI_AWPROT                     (S_AXI_AWPROT),
        .S_AXI_AWVALID                    (S_AXI_AWVALID),
        .S_AXI_AWREADY                    (S_AXI_AWREADY),
        .S_AXI_WDATA                      (S_AXI_WDATA),
        .S_AXI_WSTRB                      (S_AXI_WSTRB),
        .S_AXI_WVALID                     (S_AXI_WVALID),
        .S_AXI_WREADY                     (S_AXI_WREADY),
        .S_AXI_BID                        (S_AXI_BID),
        .S_AXI_BRESP                      (S_AXI_BRESP),
        .S_AXI_BUSER                      (S_AXI_BUSER),
        .S_AXI_BVALID                     (S_AXI_BVALID),
        .S_AXI_BREADY                     (S_AXI_BREADY),
        .S_AXI_ARID                       (S_AXI_ARID),
        .S_AXI_ARADDR                     (S_AXI_ARADDR),
        .S_AXI_ARPROT                     (S_AXI_ARPROT),
        .S_AXI_ARVALID                    (S_AXI_ARVALID),
        .S_AXI_ARREADY                    (S_AXI_ARREADY),
        .S_AXI_RID                        (S_AXI_RID),
        .S_AXI_RDATA                      (S_AXI_RDATA),
        .S_AXI_RRESP                      (S_AXI_RRESP),
        .S_AXI_RLAST                      (S_AXI_RLAST),
        .S_AXI_RUSER                      (S_AXI_RUSER),
        .S_AXI_RVALID                     (S_AXI_RVALID),
        .S_AXI_RREADY                     (S_AXI_RREADY),
        .M_AXI_AWID                       (M_AXI_AWID),
        .M_AXI_AWADDR                     (M_AXI_AWADDR),
        .M_AXI_AWLEN                      (M_AXI_AWLEN),
        .M_AXI_AWSIZE                     (M_AXI_AWSIZE),
        .M_AXI_AWBURST                    (M_AXI_AWBURST),
        .M_AXI_AWLOCK                     (M_AXI_AWLOCK),
        .M_AXI_AWCACHE                    (M_AXI_AWCACHE),
        .M_AXI_AWPROT                     (M_AXI_AWPROT),
        .M_AXI_AWREGION                   (M_AXI_AWREGION),
        .M_AXI_AWQOS                      (M_AXI_AWQOS),
        .M_AXI_AWUSER                     (M_AXI_AWUSER),
        .M_AXI_AWVALID                    (M_AXI_AWVALID),
        .M_AXI_AWREADY                    (M_AXI_AWREADY),
        .M_AXI_WID                        (M_AXI_WID),
        .M_AXI_WDATA                      (M_AXI_WDATA),
        .M_AXI_WSTRB                      (M_AXI_WSTRB),
        .M_AXI_WLAST                      (M_AXI_WLAST),
        .M_AXI_WUSER                      (M_AXI_WUSER),
        .M_AXI_WVALID                     (M_AXI_WVALID),
        .M_AXI_WREADY                     (M_AXI_WREADY),
        .M_AXI_BRESP                      (M_AXI_BRESP),
        .M_AXI_BVALID                     (M_AXI_BVALID),
        .M_AXI_BREADY                     (M_AXI_BREADY),
        .M_AXI_ARID                       (M_AXI_ARID),
        .M_AXI_ARADDR                     (M_AXI_ARADDR),
        .M_AXI_ARLEN                      (M_AXI_ARLEN),
        .M_AXI_ARSIZE                     (M_AXI_ARSIZE),
        .M_AXI_ARBURST                    (M_AXI_ARBURST),
        .M_AXI_ARLOCK                     (M_AXI_ARLOCK),
        .M_AXI_ARCACHE                    (M_AXI_ARCACHE),
        .M_AXI_ARPROT                     (M_AXI_ARPROT),
        .M_AXI_ARREGION                   (M_AXI_ARREGION),
        .M_AXI_ARQOS                      (M_AXI_ARQOS),
        .M_AXI_ARUSER                     (M_AXI_ARUSER),
        .M_AXI_ARVALID                    (M_AXI_ARVALID),
        .M_AXI_ARREADY                    (M_AXI_ARREADY),
        .M_AXI_RDATA                      (M_AXI_RDATA),
        .M_AXI_RRESP                      (M_AXI_RRESP),
        .M_AXI_RVALID                     (M_AXI_RVALID),
        .M_AXI_RREADY                     (M_AXI_RREADY)
      );
    end
  end else if ((C_AXI_PROTOCOL == P_AXI3) && (C_AXI3_BYPASS == 0)) begin : gen_axi3
  ict106_axi3_conv #(
      .C_FAMILY                         (C_FAMILY),
      .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH),
      .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
      .C_AXI_DATA_WIDTH                 (C_AXI_DATA_WIDTH),
      .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
      .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
      .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
      .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
      .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
      .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
      .C_AXI_SUPPORTS_WRITE             (C_AXI_SUPPORTS_WRITE),
      .C_AXI_SUPPORTS_READ              (C_AXI_SUPPORTS_READ)
    ) axi3_conv_inst (
      .ARESETN                          (ARESETN),
      .ACLK                             (ACLK),
      .S_AXI_AWID                       (S_AXI_AWID),
      .S_AXI_AWADDR                     (S_AXI_AWADDR),
      .S_AXI_AWLEN                      (S_AXI_AWLEN),
      .S_AXI_AWSIZE                     (S_AXI_AWSIZE),
      .S_AXI_AWBURST                    (S_AXI_AWBURST),
      .S_AXI_AWLOCK                     (S_AXI_AWLOCK),
      .S_AXI_AWCACHE                    (S_AXI_AWCACHE),
      .S_AXI_AWPROT                     (S_AXI_AWPROT),
      .S_AXI_AWQOS                      (S_AXI_AWQOS),
      .S_AXI_AWUSER                     (S_AXI_AWUSER),
      .S_AXI_AWVALID                    (S_AXI_AWVALID),
      .S_AXI_AWREADY                    (S_AXI_AWREADY),
      .S_AXI_WDATA                      (S_AXI_WDATA),
      .S_AXI_WSTRB                      (S_AXI_WSTRB),
      .S_AXI_WLAST                      (S_AXI_WLAST),
      .S_AXI_WUSER                      (S_AXI_WUSER),
      .S_AXI_WVALID                     (S_AXI_WVALID),
      .S_AXI_WREADY                     (S_AXI_WREADY),
      .S_AXI_BID                        (S_AXI_BID),
      .S_AXI_BRESP                      (S_AXI_BRESP),
      .S_AXI_BUSER                      (S_AXI_BUSER),
      .S_AXI_BVALID                     (S_AXI_BVALID),
      .S_AXI_BREADY                     (S_AXI_BREADY),
      .S_AXI_ARID                       (S_AXI_ARID),
      .S_AXI_ARADDR                     (S_AXI_ARADDR),
      .S_AXI_ARLEN                      (S_AXI_ARLEN),
      .S_AXI_ARSIZE                     (S_AXI_ARSIZE),
      .S_AXI_ARBURST                    (S_AXI_ARBURST),
      .S_AXI_ARLOCK                     (S_AXI_ARLOCK),
      .S_AXI_ARCACHE                    (S_AXI_ARCACHE),
      .S_AXI_ARPROT                     (S_AXI_ARPROT),
      .S_AXI_ARQOS                      (S_AXI_ARQOS),
      .S_AXI_ARUSER                     (S_AXI_ARUSER),
      .S_AXI_ARVALID                    (S_AXI_ARVALID),
      .S_AXI_ARREADY                    (S_AXI_ARREADY),
      .S_AXI_RID                        (S_AXI_RID),
      .S_AXI_RDATA                      (S_AXI_RDATA),
      .S_AXI_RRESP                      (S_AXI_RRESP),
      .S_AXI_RLAST                      (S_AXI_RLAST),
      .S_AXI_RUSER                      (S_AXI_RUSER),
      .S_AXI_RVALID                     (S_AXI_RVALID),
      .S_AXI_RREADY                     (S_AXI_RREADY),
      .M_AXI_AWID                       (M_AXI_AWID),
      .M_AXI_AWADDR                     (M_AXI_AWADDR),
      .M_AXI_AWLEN                      (M_AXI_AWLEN),
      .M_AXI_AWSIZE                     (M_AXI_AWSIZE),
      .M_AXI_AWBURST                    (M_AXI_AWBURST),
      .M_AXI_AWLOCK                     (M_AXI_AWLOCK),
      .M_AXI_AWCACHE                    (M_AXI_AWCACHE),
      .M_AXI_AWPROT                     (M_AXI_AWPROT),
      .M_AXI_AWREGION                   (M_AXI_AWREGION),
      .M_AXI_AWQOS                      (M_AXI_AWQOS),
      .M_AXI_AWUSER                     (M_AXI_AWUSER),
      .M_AXI_AWVALID                    (M_AXI_AWVALID),
      .M_AXI_AWREADY                    (M_AXI_AWREADY),
      .M_AXI_WID                        (M_AXI_WID),
      .M_AXI_WDATA                      (M_AXI_WDATA),
      .M_AXI_WSTRB                      (M_AXI_WSTRB),
      .M_AXI_WLAST                      (M_AXI_WLAST),
      .M_AXI_WUSER                      (M_AXI_WUSER),
      .M_AXI_WVALID                     (M_AXI_WVALID),
      .M_AXI_WREADY                     (M_AXI_WREADY),
      .M_AXI_BID                        (M_AXI_BID),
      .M_AXI_BRESP                      (M_AXI_BRESP),
      .M_AXI_BUSER                      (M_AXI_BUSER),
      .M_AXI_BVALID                     (M_AXI_BVALID),
      .M_AXI_BREADY                     (M_AXI_BREADY),
      .M_AXI_ARID                       (M_AXI_ARID),
      .M_AXI_ARADDR                     (M_AXI_ARADDR),
      .M_AXI_ARLEN                      (M_AXI_ARLEN),
      .M_AXI_ARSIZE                     (M_AXI_ARSIZE),
      .M_AXI_ARBURST                    (M_AXI_ARBURST),
      .M_AXI_ARLOCK                     (M_AXI_ARLOCK),
      .M_AXI_ARCACHE                    (M_AXI_ARCACHE),
      .M_AXI_ARPROT                     (M_AXI_ARPROT),
      .M_AXI_ARREGION                   (M_AXI_ARREGION),
      .M_AXI_ARQOS                      (M_AXI_ARQOS),
      .M_AXI_ARUSER                     (M_AXI_ARUSER),
      .M_AXI_ARVALID                    (M_AXI_ARVALID),
      .M_AXI_ARREADY                    (M_AXI_ARREADY),
      .M_AXI_RID                        (M_AXI_RID),
      .M_AXI_RDATA                      (M_AXI_RDATA),
      .M_AXI_RRESP                      (M_AXI_RRESP),
      .M_AXI_RLAST                      (M_AXI_RLAST),
      .M_AXI_RUSER                      (M_AXI_RUSER),
      .M_AXI_RVALID                     (M_AXI_RVALID),
      .M_AXI_RREADY                     (M_AXI_RREADY)
    );
  end else begin :gen_no_conv
    assign M_AXI_AWID = S_AXI_AWID;
    assign M_AXI_AWADDR = S_AXI_AWADDR;
    assign M_AXI_AWLEN = S_AXI_AWLEN;
    assign M_AXI_AWSIZE = S_AXI_AWSIZE;
    assign M_AXI_AWBURST = S_AXI_AWBURST;
    assign M_AXI_AWLOCK = S_AXI_AWLOCK;
    assign M_AXI_AWCACHE = S_AXI_AWCACHE;
    assign M_AXI_AWPROT = S_AXI_AWPROT;
    assign M_AXI_AWREGION = S_AXI_AWREGION;
    assign M_AXI_AWQOS = S_AXI_AWQOS;
    assign M_AXI_AWUSER = S_AXI_AWUSER;
    assign M_AXI_AWVALID = S_AXI_AWVALID;
    assign S_AXI_AWREADY = M_AXI_AWREADY;
    assign M_AXI_WID = C_AXI3_BYPASS ? S_AXI_WID : {C_AXI_ID_WIDTH{1'b0}} ;
    assign M_AXI_WDATA = S_AXI_WDATA;
    assign M_AXI_WSTRB = S_AXI_WSTRB;
    assign M_AXI_WLAST = S_AXI_WLAST;
    assign M_AXI_WUSER = S_AXI_WUSER;
    assign M_AXI_WVALID = S_AXI_WVALID;
    assign S_AXI_WREADY = M_AXI_WREADY;
    assign S_AXI_BID = M_AXI_BID;
    assign S_AXI_BRESP = M_AXI_BRESP;
    assign S_AXI_BUSER = M_AXI_BUSER;
    assign S_AXI_BVALID = M_AXI_BVALID;
    assign M_AXI_BREADY = S_AXI_BREADY;
    assign M_AXI_ARID = S_AXI_ARID;
    assign M_AXI_ARADDR = S_AXI_ARADDR;
    assign M_AXI_ARLEN = S_AXI_ARLEN;
    assign M_AXI_ARSIZE = S_AXI_ARSIZE;
    assign M_AXI_ARBURST = S_AXI_ARBURST;
    assign M_AXI_ARLOCK = S_AXI_ARLOCK;
    assign M_AXI_ARCACHE = S_AXI_ARCACHE;
    assign M_AXI_ARPROT = S_AXI_ARPROT;
    assign M_AXI_ARREGION = S_AXI_ARREGION;
    assign M_AXI_ARQOS = S_AXI_ARQOS;
    assign M_AXI_ARUSER = S_AXI_ARUSER;
    assign M_AXI_ARVALID = S_AXI_ARVALID;
    assign S_AXI_ARREADY = M_AXI_ARREADY;
    assign S_AXI_RID = M_AXI_RID;
    assign S_AXI_RDATA = M_AXI_RDATA;
    assign S_AXI_RRESP = M_AXI_RRESP;
    assign S_AXI_RLAST = M_AXI_RLAST;
    assign S_AXI_RUSER = M_AXI_RUSER;
    assign S_AXI_RVALID = M_AXI_RVALID;
    assign M_AXI_RREADY = S_AXI_RREADY;
  end
endgenerate


endmodule

`default_nettype wire
