// -- (c) Copyright 2009 - 2011 Xilinx, Inc. All rights reserved.
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
// File name: data_fifo_bank.v
//
// Description: 
//   This module is a bank of data-path FIFOs for a vectored AXI interface.
//   The interface of this module consists of a vectored slave and master interface
//     which are each concatenations of upper-level AXI pathways,
//     plus various vectored parameters.
//   This module instantiates a set of individual AXI FIFO buffers.
//
//--------------------------------------------------------------------------
//
// Structure:
//    data_fifo_bank
//      axi_data_fifo
//
//-----------------------------------------------------------------------------
`timescale 1ps/1ps
`default_nettype none

module ict106_data_fifo_bank #
  (
   parameter         C_FAMILY                         = "none", 
   parameter integer C_NUM_SLOTS                = 1, 
   parameter         C_AXI_ID_WIDTH                 = 1, 
                       // Effective width of ID ports for each SI and MI slot.
                       // Format: C_NUM_SLOTS{Bit32}; 
                       // Range: 'h00000001 - C_AXI_ID_MAX_WIDTH.
   parameter integer C_AXI_ID_MAX_WIDTH                 = 1, 
                       // Width of ID signals propagated by the Interconnect.
                       // Stride of ID fields within each ID port.
                       // Range: >= 1.
   parameter integer C_AXI_ADDR_WIDTH                 = 32, 
   parameter integer C_AXI_DATA_MAX_WIDTH           = 256, 
                       // Largest value supported for any DATA_WIDTH.
                       // Stride of data fields within each DATA port.
   parameter         C_AXI_DATA_WIDTH               = {16{32'h00000020}}, 
                       // Format: C_NUM_SLOTS{Bit32}; 
                       // Range: 'h00000020, 'h00000040, 'h00000080, 'h00000100.
   parameter         C_AXI_SUPPORTS_WRITE           = 16'b11111111_11111111, 
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter         C_AXI_SUPPORTS_READ            = 16'b11111111_11111111, 
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter integer C_AXI_SUPPORTS_USER_SIGNALS      = 0,
                       // 1 = Propagate all USER signals, 0 = Don’t propagate.
   parameter integer C_AXI_AWUSER_WIDTH               = 1,
   parameter integer C_AXI_ARUSER_WIDTH               = 1,
   parameter integer C_AXI_WUSER_WIDTH                = 1,
   parameter integer C_AXI_RUSER_WIDTH                = 1,
   parameter integer C_AXI_BUSER_WIDTH                = 1,
   parameter         C_AXI_WRITE_FIFO_DEPTH         = {16{32'h00000000}},
                       // Format: C_NUM_SLOTS{Bit32}; 
                       // Range: 'h00000000, 'h00000020, 'h00000200.
   parameter         C_AXI_WRITE_FIFO_TYPE          = 16'b11111111_11111111,
                       // 0 = LUT flop/RAM only, 1 = BRAM allowed.
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter         C_AXI_WRITE_FIFO_DELAY         = 16'b00000000_00000000,  // 0 = No, 1 = Yes (FUTURE FEATURE)
                       // Indicates whether AWVALID and WVALID assertion is delayed until:
                       //   a. the corresponding WLAST is stored in the FIFO, or
                       //   b. no WLAST is stored and the FIFO is full.
                       // 0 means AW channel is pass-through and 
                       //   WVALID is asserted whenever FIFO is not empty.
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter         C_AXI_READ_FIFO_DEPTH          = {16{32'h00000000}},
                       // Format: C_NUM_SLOTS{Bit32}; 
   parameter         C_AXI_READ_FIFO_TYPE           = 16'b11111111_11111111,
                       // 0 = LUT flop/RAM only, 1 = BRAM allowed.
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter         C_AXI_READ_FIFO_DELAY          = 16'b00000000_00000000  // 0 = No, 1 = Yes (FUTURE FEATURE)
                       // Indicates whether ARVALID assertion is delayed until the 
                       //   the remaining vacancy of the FIFO is at least the burst length
                       //   as indicated by ARLEN.
                       // 0 means AR channel is pass-through.
                       // Format: C_NUM_SLOTS{Bit1}.
   )
  (
   // Global Signals
   input  wire                                              ACLK,
   input  wire                                              ARESETN,
   // Slave Interface Write Address Ports
   input  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           S_AXI_AWID,
   input  wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]           S_AXI_AWADDR,
   input  wire [C_NUM_SLOTS*8-1:0]                          S_AXI_AWLEN,
   input  wire [C_NUM_SLOTS*3-1:0]                          S_AXI_AWSIZE,
   input  wire [C_NUM_SLOTS*2-1:0]                          S_AXI_AWBURST,
   input  wire [C_NUM_SLOTS*2-1:0]                          S_AXI_AWLOCK,
   input  wire [C_NUM_SLOTS*4-1:0]                          S_AXI_AWCACHE,
   input  wire [C_NUM_SLOTS*3-1:0]                          S_AXI_AWPROT,
   input  wire [C_NUM_SLOTS*4-1:0]                          S_AXI_AWREGION,
   input  wire [C_NUM_SLOTS*4-1:0]                          S_AXI_AWQOS,
   input  wire [C_NUM_SLOTS*C_AXI_AWUSER_WIDTH-1:0]         S_AXI_AWUSER,
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_AWVALID,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_AWREADY,
   // Slave Interface Write Data Ports
   input  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     S_AXI_WDATA,
   input  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]   S_AXI_WSTRB,
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_WLAST,
   input  wire [C_NUM_SLOTS*C_AXI_WUSER_WIDTH-1:0]          S_AXI_WUSER,
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_WVALID,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_WREADY,
   // Slave Interface Write Response Ports
   output wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           S_AXI_BID,
   output wire [C_NUM_SLOTS*2-1:0]                          S_AXI_BRESP,
   output wire [C_NUM_SLOTS*C_AXI_BUSER_WIDTH-1:0]          S_AXI_BUSER,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_BVALID,
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_BREADY,
   // Slave Interface Read Address Ports
   input  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           S_AXI_ARID,
   input  wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]           S_AXI_ARADDR,
   input  wire [C_NUM_SLOTS*8-1:0]                          S_AXI_ARLEN,
   input  wire [C_NUM_SLOTS*3-1:0]                          S_AXI_ARSIZE,
   input  wire [C_NUM_SLOTS*2-1:0]                          S_AXI_ARBURST,
   input  wire [C_NUM_SLOTS*2-1:0]                          S_AXI_ARLOCK,
   input  wire [C_NUM_SLOTS*4-1:0]                          S_AXI_ARCACHE,
   input  wire [C_NUM_SLOTS*3-1:0]                          S_AXI_ARPROT,
   input  wire [C_NUM_SLOTS*4-1:0]                          S_AXI_ARREGION,
   input  wire [C_NUM_SLOTS*4-1:0]                          S_AXI_ARQOS,
   input  wire [C_NUM_SLOTS*C_AXI_ARUSER_WIDTH-1:0]         S_AXI_ARUSER,
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_ARVALID,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_ARREADY,
   // Slave Interface Read Data Ports
   output wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           S_AXI_RID,
   output wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     S_AXI_RDATA,
   output wire [C_NUM_SLOTS*2-1:0]                          S_AXI_RRESP,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_RLAST,
   output wire [C_NUM_SLOTS*C_AXI_RUSER_WIDTH-1:0]          S_AXI_RUSER,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_RVALID,
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_RREADY,
   // Master Interface Write Address Port
   output wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]          M_AXI_AWID,
   output wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]          M_AXI_AWADDR,
   output wire [C_NUM_SLOTS*8-1:0]                         M_AXI_AWLEN,
   output wire [C_NUM_SLOTS*3-1:0]                         M_AXI_AWSIZE,
   output wire [C_NUM_SLOTS*2-1:0]                         M_AXI_AWBURST,
   output wire [C_NUM_SLOTS*2-1:0]                         M_AXI_AWLOCK,
   output wire [C_NUM_SLOTS*4-1:0]                         M_AXI_AWCACHE,
   output wire [C_NUM_SLOTS*3-1:0]                         M_AXI_AWPROT,
   output wire [C_NUM_SLOTS*4-1:0]                         M_AXI_AWREGION,
   output wire [C_NUM_SLOTS*4-1:0]                         M_AXI_AWQOS,
   output wire [C_NUM_SLOTS*C_AXI_AWUSER_WIDTH-1:0]        M_AXI_AWUSER,
   output wire [C_NUM_SLOTS-1:0]                           M_AXI_AWVALID,
   input  wire [C_NUM_SLOTS-1:0]                           M_AXI_AWREADY,
   // Master Interface Write Data Ports
   output wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    M_AXI_WDATA,
   output wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]  M_AXI_WSTRB,
   output wire [C_NUM_SLOTS-1:0]                           M_AXI_WLAST,
   output wire [C_NUM_SLOTS*C_AXI_WUSER_WIDTH-1:0]         M_AXI_WUSER,
   output wire [C_NUM_SLOTS-1:0]                           M_AXI_WVALID,
   input  wire [C_NUM_SLOTS-1:0]                           M_AXI_WREADY,
   // Master Interface Write Response Ports
   input  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]          M_AXI_BID,
   input  wire [C_NUM_SLOTS*2-1:0]                         M_AXI_BRESP,
   input  wire [C_NUM_SLOTS*C_AXI_BUSER_WIDTH-1:0]         M_AXI_BUSER,
   input  wire [C_NUM_SLOTS-1:0]                           M_AXI_BVALID,
   output wire [C_NUM_SLOTS-1:0]                           M_AXI_BREADY,
   // Master Interface Read Address Port
   output wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]          M_AXI_ARID,
   output wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]          M_AXI_ARADDR,
   output wire [C_NUM_SLOTS*8-1:0]                         M_AXI_ARLEN,
   output wire [C_NUM_SLOTS*3-1:0]                         M_AXI_ARSIZE,
   output wire [C_NUM_SLOTS*2-1:0]                         M_AXI_ARBURST,
   output wire [C_NUM_SLOTS*2-1:0]                         M_AXI_ARLOCK,
   output wire [C_NUM_SLOTS*4-1:0]                         M_AXI_ARCACHE,
   output wire [C_NUM_SLOTS*3-1:0]                         M_AXI_ARPROT,
   output wire [C_NUM_SLOTS*4-1:0]                         M_AXI_ARREGION,
   output wire [C_NUM_SLOTS*4-1:0]                         M_AXI_ARQOS,
   output wire [C_NUM_SLOTS*C_AXI_ARUSER_WIDTH-1:0]        M_AXI_ARUSER,
   output wire [C_NUM_SLOTS-1:0]                           M_AXI_ARVALID,
   input  wire [C_NUM_SLOTS-1:0]                           M_AXI_ARREADY,
   // Master Interface Read Data Ports
   input  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]          M_AXI_RID,
   input  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    M_AXI_RDATA,
   input  wire [C_NUM_SLOTS*2-1:0]                         M_AXI_RRESP,
   input  wire [C_NUM_SLOTS-1:0]                           M_AXI_RLAST,
   input  wire [C_NUM_SLOTS*C_AXI_RUSER_WIDTH-1:0]         M_AXI_RUSER,
   input  wire [C_NUM_SLOTS-1:0]                           M_AXI_RVALID,
   output wire [C_NUM_SLOTS-1:0]                           M_AXI_RREADY
   );

  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     si_df_rdata           ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     df_mi_wdata           ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]   df_mi_wstrb           ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]       si_df_bid             ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]       si_df_rid             ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]       df_mi_awid             ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]       df_mi_arid             ;
  
  genvar slot;
  
  generate
    for (slot=0;slot<C_NUM_SLOTS;slot=slot+1) begin : gen_fifo_slot
      
  ict106_axi_data_fifo #
        (
          .C_FAMILY                         (C_FAMILY),
          .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH[slot*32+:32]),
          .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
          .C_AXI_DATA_WIDTH                 (C_AXI_DATA_WIDTH[slot*32+:32]),
          .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
          .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
          .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
          .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
          .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
          .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
          .C_AXI_WRITE_FIFO_DEPTH           (
              C_AXI_SUPPORTS_WRITE[slot] ? C_AXI_WRITE_FIFO_DEPTH[slot*32+:32] : 0),
          .C_AXI_WRITE_FIFO_TYPE            (
              ~C_AXI_WRITE_FIFO_TYPE[slot] ? "lut" :
              C_AXI_WRITE_FIFO_DEPTH[slot*32+:32]>32 ? "bram" : "lut"
              ),
          .C_AXI_WRITE_FIFO_DELAY           (C_AXI_WRITE_FIFO_DELAY[slot]),
          .C_AXI_READ_FIFO_DEPTH           (
              C_AXI_SUPPORTS_READ[slot] ? C_AXI_READ_FIFO_DEPTH[slot*32+:32] : 0),
          .C_AXI_READ_FIFO_TYPE            (
              ~C_AXI_READ_FIFO_TYPE[slot] ? "lut" :
              C_AXI_READ_FIFO_DEPTH[slot*32+:32]>32 ? "bram" : "lut"
              ),
          .C_AXI_READ_FIFO_DELAY           (C_AXI_READ_FIFO_DELAY[slot])
        )
        data_fifo_inst 
        (
          .ACLK                       (ACLK),
          .ARESETN                    (ARESETN),
          .S_AXI_AWID                       (S_AXI_AWID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_AWADDR                     (S_AXI_AWADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .S_AXI_AWLEN                      (S_AXI_AWLEN[slot*8+:8]),
          .S_AXI_AWSIZE                     (S_AXI_AWSIZE[slot*3+:3]),
          .S_AXI_AWBURST                    (S_AXI_AWBURST[slot*2+:2]),
          .S_AXI_AWLOCK                     (S_AXI_AWLOCK[slot*2+:2]),
          .S_AXI_AWCACHE                    (S_AXI_AWCACHE[slot*4+:4]),
          .S_AXI_AWPROT                     (S_AXI_AWPROT[slot*3+:3]),
          .S_AXI_AWREGION                   (S_AXI_AWREGION[slot*4+:4]),
          .S_AXI_AWQOS                      (S_AXI_AWQOS[slot*4+:4]),
          .S_AXI_AWUSER                     (S_AXI_AWUSER[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]),
          .S_AXI_AWVALID                    (S_AXI_AWVALID[slot*1+:1]),
          .S_AXI_AWREADY                    (S_AXI_AWREADY[slot*1+:1]),
          .S_AXI_WDATA                      (S_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_WIDTH[slot*32+:32]]),
          .S_AXI_WSTRB                      (S_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_WIDTH[slot*32+:32]/8]),
          .S_AXI_WLAST                      (S_AXI_WLAST[slot*1+:1]),
          .S_AXI_WUSER                      (S_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .S_AXI_WVALID                     (S_AXI_WVALID[slot*1+:1]),
          .S_AXI_WREADY                     (S_AXI_WREADY[slot*1+:1]),
          .S_AXI_BID                        (si_df_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_BRESP                      (S_AXI_BRESP[slot*2+:2]),
          .S_AXI_BUSER                      (S_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .S_AXI_BVALID                     (S_AXI_BVALID[slot*1+:1]),
          .S_AXI_BREADY                     (S_AXI_BREADY[slot*1+:1]),
          .S_AXI_ARID                       (S_AXI_ARID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_ARADDR                     (S_AXI_ARADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .S_AXI_ARLEN                      (S_AXI_ARLEN[slot*8+:8]),
          .S_AXI_ARSIZE                     (S_AXI_ARSIZE[slot*3+:3]),
          .S_AXI_ARBURST                    (S_AXI_ARBURST[slot*2+:2]),
          .S_AXI_ARLOCK                     (S_AXI_ARLOCK[slot*2+:2]),
          .S_AXI_ARCACHE                    (S_AXI_ARCACHE[slot*4+:4]),
          .S_AXI_ARPROT                     (S_AXI_ARPROT[slot*3+:3]),
          .S_AXI_ARREGION                   (S_AXI_ARREGION[slot*4+:4]),
          .S_AXI_ARQOS                      (S_AXI_ARQOS[slot*4+:4]),
          .S_AXI_ARUSER                     (S_AXI_ARUSER[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]),
          .S_AXI_ARVALID                    (S_AXI_ARVALID[slot*1+:1]),
          .S_AXI_ARREADY                    (S_AXI_ARREADY[slot*1+:1]),
          .S_AXI_RID                        (si_df_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_RDATA                      (si_df_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_WIDTH[slot*32+:32]]),
          .S_AXI_RRESP                      (S_AXI_RRESP[slot*2+:2]),
          .S_AXI_RLAST                      (S_AXI_RLAST[slot*1+:1]),
          .S_AXI_RUSER                      (S_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .S_AXI_RVALID                     (S_AXI_RVALID[slot*1+:1]),
          .S_AXI_RREADY                     (S_AXI_RREADY[slot*1+:1]),
          .M_AXI_AWID                       (df_mi_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_AWADDR                     (M_AXI_AWADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .M_AXI_AWLEN                      (M_AXI_AWLEN[slot*8+:8]),
          .M_AXI_AWSIZE                     (M_AXI_AWSIZE[slot*3+:3]),
          .M_AXI_AWBURST                    (M_AXI_AWBURST[slot*2+:2]),
          .M_AXI_AWLOCK                     (M_AXI_AWLOCK[slot*2+:2]),
          .M_AXI_AWCACHE                    (M_AXI_AWCACHE[slot*4+:4]),
          .M_AXI_AWPROT                     (M_AXI_AWPROT[slot*3+:3]),
          .M_AXI_AWREGION                   (M_AXI_AWREGION[slot*4+:4]),
          .M_AXI_AWQOS                      (M_AXI_AWQOS[slot*4+:4]),
          .M_AXI_AWUSER                     (M_AXI_AWUSER[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]),
          .M_AXI_AWVALID                    (M_AXI_AWVALID[slot*1+:1]),
          .M_AXI_AWREADY                    (M_AXI_AWREADY[slot*1+:1]),
          .M_AXI_WDATA                      (df_mi_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_WIDTH[slot*32+:32]]),
          .M_AXI_WSTRB                      (df_mi_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_WIDTH[slot*32+:32]/8]),
          .M_AXI_WLAST                      (M_AXI_WLAST[slot*1+:1]),
          .M_AXI_WUSER                      (M_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .M_AXI_WVALID                     (M_AXI_WVALID[slot*1+:1]),
          .M_AXI_WREADY                     (M_AXI_WREADY[slot*1+:1]),
          .M_AXI_BID                        (M_AXI_BID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_BRESP                      (M_AXI_BRESP[slot*2+:2]),
          .M_AXI_BUSER                      (M_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .M_AXI_BVALID                     (M_AXI_BVALID[slot*1+:1]),
          .M_AXI_BREADY                     (M_AXI_BREADY[slot*1+:1]),
          .M_AXI_ARID                       (df_mi_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_ARADDR                     (M_AXI_ARADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .M_AXI_ARLEN                      (M_AXI_ARLEN[slot*8+:8]),
          .M_AXI_ARSIZE                     (M_AXI_ARSIZE[slot*3+:3]),
          .M_AXI_ARBURST                    (M_AXI_ARBURST[slot*2+:2]),
          .M_AXI_ARLOCK                     (M_AXI_ARLOCK[slot*2+:2]),
          .M_AXI_ARCACHE                    (M_AXI_ARCACHE[slot*4+:4]),
          .M_AXI_ARPROT                     (M_AXI_ARPROT[slot*3+:3]),
          .M_AXI_ARREGION                   (M_AXI_ARREGION[slot*4+:4]),
          .M_AXI_ARQOS                      (M_AXI_ARQOS[slot*4+:4]),
          .M_AXI_ARUSER                     (M_AXI_ARUSER[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]),
          .M_AXI_ARVALID                    (M_AXI_ARVALID[slot*1+:1]),
          .M_AXI_ARREADY                    (M_AXI_ARREADY[slot*1+:1]),
          .M_AXI_RID                        (M_AXI_RID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_RDATA                      (M_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_WIDTH[slot*32+:32]]),
          .M_AXI_RRESP                      (M_AXI_RRESP[slot*2+:2]),
          .M_AXI_RLAST                      (M_AXI_RLAST[slot*1+:1]),
          .M_AXI_RUSER                      (M_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .M_AXI_RVALID                     (M_AXI_RVALID[slot*1+:1]),
          .M_AXI_RREADY                     (M_AXI_RREADY[slot*1+:1])
        );
      assign S_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH] = si_df_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_WIDTH[slot*32+:32]] ;
      assign M_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH] = df_mi_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_WIDTH[slot*32+:32]] ;
      assign M_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_MAX_WIDTH/8] = df_mi_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_WIDTH[slot*32+:32]/8] ;
      assign S_AXI_BID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = si_df_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
      assign S_AXI_RID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = si_df_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
      assign M_AXI_AWID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = df_mi_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
      assign M_AXI_ARID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = df_mi_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
      
    end
  endgenerate

endmodule

`default_nettype wire
