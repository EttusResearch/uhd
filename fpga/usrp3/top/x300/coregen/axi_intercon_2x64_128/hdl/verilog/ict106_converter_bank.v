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
// File name: converter_bank.v
//
// Description: 
//   This module is a bank of width and clock converters for a vectored AXI interface.
//   The interface of this module consists of a vectored slave and master interface
//     which are each concatenations of upper-level AXI pathways,
//     plus various vectored parameters.
//   This module instantiates a set of individual up-sizers, down-sizers and
//     clock converters.
//
//-----------------------------------------------------------------------------
//
// Structure:
//    converter_bank
//      axi_upsizer
//      clock_conv
//        fifo_gen
//        clock_sync_accel
//        clock_sync_decel
//      axi_downsizer
//      
//-----------------------------------------------------------------------------
`timescale 1ps/1ps
`default_nettype none

module ict106_converter_bank #
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
   parameter         C_S_AXI_DATA_WIDTH               = {16{32'h00000020}}, 
                       // SI-side data width of each slot.
                       // Format: C_NUM_SLOTS{Bit32}; 
                       // Range: 'h00000020, 'h00000040, 'h00000080, 'h00000100.
   parameter         C_M_AXI_DATA_WIDTH               = {16{32'h00000020}}, 
                       // MI-side data width of each slot.
                       // Format: C_NUM_SLOTS{Bit32}; 
                       // Range: 'h00000020, 'h00000040, 'h00000080, 'h00000100.
   parameter         C_AXI_PROTOCOL                 = {16{32'h00000000}}, 
                       // Full-AXI4 ('h00000000),
                       // AXI3 ('h00000001) or 
                       // Axi4Lite ('h00000002).
                       // Format: C_NUM_SLOTS{Bit32}.
   parameter         C_S_AXI_ACLK_RATIO               = {16{32'h00000001}}, 
                       // SI-side clock frequency ratio.
                       // Format: C_NUM_SLOTS{Bit32}; Range: >='h00000001.
   parameter         C_M_AXI_ACLK_RATIO               = {16{32'h00000001}}, 
                       // MI-side clock frequency ratio.
                       // Format: C_NUM_SLOTS{Bit32}; Range: >='h00000001.
   parameter         C_AXI_IS_ACLK_ASYNC            = 16'b00000000_00000000, 
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter         C_AXI_SUPPORTS_WRITE           = 16'b11111111_11111111, 
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter         C_AXI_SUPPORTS_READ            = 16'b11111111_11111111, 
                       // Format: C_NUM_SLOTS{Bit1}.
   parameter integer C_AXI_SUPPORTS_USER_SIGNALS      = 0,
                       // 1 = Propagate all USER signals, 0 = Dont propagate.
   parameter integer C_AXI_AWUSER_WIDTH               = 1,
   parameter integer C_AXI_ARUSER_WIDTH               = 1,
   parameter integer C_AXI_WUSER_WIDTH                = 1,
   parameter integer C_AXI_RUSER_WIDTH                = 1,
   parameter integer C_AXI_BUSER_WIDTH                = 1,
   parameter         C_HEMISPHERE                   = "si"
     // For SI hemisphere, ARESET is in clock domain of the M_AXI interface;
     // For MI hemisphere, ARESET is in clock domain of the S_AXI interface;
   )
  (
   // Global Signals
   input  wire                                              INTERCONNECT_ACLK,
   input  wire                                              INTERCONNECT_ARESETN,
   input  wire                                              LOCAL_ARESETN,
   output wire                                              INTERCONNECT_RESET_OUT_N,
   output wire [C_NUM_SLOTS-1:0]                            S_AXI_RESET_OUT_N,
   output wire [C_NUM_SLOTS-1:0]                            M_AXI_RESET_OUT_N,
   // Slave Interface Global Signals
   input  wire [C_NUM_SLOTS-1:0]                            S_AXI_ACLK,
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
   // Master Interface Global Signals
   input  wire [C_NUM_SLOTS-1:0]                            M_AXI_ACLK,
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
   
  localparam P_AXILITE = 32'h2;
  localparam integer P_CONDITIONAL_PACK = 1;
  localparam integer P_NEVER_PACK = 0;
  localparam P_LIGHTWT = 32'h7;
  localparam P_FULLY_REG = 32'h1;
  
  function integer f_cc_data_width (
  // Data width of selected slot at clock-converter.
      input integer slot_f
    );
    begin
      f_cc_data_width = (C_S_AXI_DATA_WIDTH[slot_f*32+:32] < C_M_AXI_DATA_WIDTH[slot_f*32+:32]) ? 
                                 C_M_AXI_DATA_WIDTH[slot_f*32+:32] : C_S_AXI_DATA_WIDTH[slot_f*32+:32];
    end
  endfunction

  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     si_us_rdata           ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]     si_us_bid           ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]     si_us_rid           ;
  
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           us_cc_awid            ;
  wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]         us_cc_awaddr          ;
  wire [C_NUM_SLOTS*8-1:0]                        us_cc_awlen           ;
  wire [C_NUM_SLOTS*3-1:0]                        us_cc_awsize          ;
  wire [C_NUM_SLOTS*2-1:0]                        us_cc_awburst         ;
  wire [C_NUM_SLOTS*2-1:0]                        us_cc_awlock          ;
  wire [C_NUM_SLOTS*4-1:0]                        us_cc_awcache         ;
  wire [C_NUM_SLOTS*3-1:0]                        us_cc_awprot          ;
  wire [C_NUM_SLOTS*4-1:0]                        us_cc_awregion        ;
  wire [C_NUM_SLOTS*4-1:0]                        us_cc_awqos           ;
  wire [C_NUM_SLOTS*C_AXI_AWUSER_WIDTH-1:0]       us_cc_awuser          ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_awvalid         ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_awready         ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     us_cc_wdata           ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]   us_cc_wstrb           ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_wlast           ;
  wire [C_NUM_SLOTS*C_AXI_WUSER_WIDTH-1:0]        us_cc_wuser           ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_wvalid          ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_wready          ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           us_cc_bid             ;
  wire [C_NUM_SLOTS*2-1:0]                        us_cc_bresp           ;
  wire [C_NUM_SLOTS*C_AXI_BUSER_WIDTH-1:0]        us_cc_buser           ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_bvalid          ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_bready          ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           us_cc_arid            ;
  wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]         us_cc_araddr          ;
  wire [C_NUM_SLOTS*8-1:0]                        us_cc_arlen           ;
  wire [C_NUM_SLOTS*3-1:0]                        us_cc_arsize          ;
  wire [C_NUM_SLOTS*2-1:0]                        us_cc_arburst         ;
  wire [C_NUM_SLOTS*2-1:0]                        us_cc_arlock          ;
  wire [C_NUM_SLOTS*4-1:0]                        us_cc_arcache         ;
  wire [C_NUM_SLOTS*3-1:0]                        us_cc_arprot          ;
  wire [C_NUM_SLOTS*4-1:0]                        us_cc_arregion        ;
  wire [C_NUM_SLOTS*4-1:0]                        us_cc_arqos           ;
  wire [C_NUM_SLOTS*C_AXI_ARUSER_WIDTH-1:0]       us_cc_aruser          ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_arvalid         ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_arready         ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           us_cc_rid             ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     us_cc_rdata           ;
  wire [C_NUM_SLOTS*2-1:0]                        us_cc_rresp           ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_rlast           ;
  wire [C_NUM_SLOTS*C_AXI_RUSER_WIDTH-1:0]        us_cc_ruser           ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_rvalid          ;
  wire [C_NUM_SLOTS-1:0]                          us_cc_rready          ;
                                                                    
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           cc_ds_awid            ;
  wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]         cc_ds_awaddr          ;
  wire [C_NUM_SLOTS*8-1:0]                        cc_ds_awlen           ;
  wire [C_NUM_SLOTS*3-1:0]                        cc_ds_awsize          ;
  wire [C_NUM_SLOTS*2-1:0]                        cc_ds_awburst         ;
  wire [C_NUM_SLOTS*2-1:0]                        cc_ds_awlock          ;
  wire [C_NUM_SLOTS*4-1:0]                        cc_ds_awcache         ;
  wire [C_NUM_SLOTS*3-1:0]                        cc_ds_awprot          ;
  wire [C_NUM_SLOTS*4-1:0]                        cc_ds_awregion        ;
  wire [C_NUM_SLOTS*4-1:0]                        cc_ds_awqos           ;
  wire [C_NUM_SLOTS*C_AXI_AWUSER_WIDTH-1:0]       cc_ds_awuser          ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_awvalid         ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_awready         ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     cc_ds_wdata           ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]   cc_ds_wstrb           ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_wlast           ;
  wire [C_NUM_SLOTS*C_AXI_WUSER_WIDTH-1:0]        cc_ds_wuser           ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_wvalid          ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_wready          ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           cc_ds_bid             ;
  wire [C_NUM_SLOTS*2-1:0]                        cc_ds_bresp           ;
  wire [C_NUM_SLOTS*C_AXI_BUSER_WIDTH-1:0]        cc_ds_buser           ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_bvalid          ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_bready          ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           cc_ds_arid            ;
  wire [C_NUM_SLOTS*C_AXI_ADDR_WIDTH-1:0]         cc_ds_araddr          ;
  wire [C_NUM_SLOTS*8-1:0]                        cc_ds_arlen           ;
  wire [C_NUM_SLOTS*3-1:0]                        cc_ds_arsize          ;
  wire [C_NUM_SLOTS*2-1:0]                        cc_ds_arburst         ;
  wire [C_NUM_SLOTS*2-1:0]                        cc_ds_arlock          ;
  wire [C_NUM_SLOTS*4-1:0]                        cc_ds_arcache         ;
  wire [C_NUM_SLOTS*3-1:0]                        cc_ds_arprot          ;
  wire [C_NUM_SLOTS*4-1:0]                        cc_ds_arregion        ;
  wire [C_NUM_SLOTS*4-1:0]                        cc_ds_arqos           ;
  wire [C_NUM_SLOTS*C_AXI_ARUSER_WIDTH-1:0]       cc_ds_aruser          ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_arvalid         ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_arready         ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]           cc_ds_rid             ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     cc_ds_rdata           ;
  wire [C_NUM_SLOTS*2-1:0]                        cc_ds_rresp           ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_rlast           ;
  wire [C_NUM_SLOTS*C_AXI_RUSER_WIDTH-1:0]        cc_ds_ruser           ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_rvalid          ;
  wire [C_NUM_SLOTS-1:0]                          cc_ds_rready          ;
  
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     ds_mi_wdata           ;
  wire [C_NUM_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]   ds_mi_wstrb           ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]       ds_mi_awid           ;
  wire [C_NUM_SLOTS*C_AXI_ID_MAX_WIDTH-1:0]       ds_mi_arid           ;
  
  wire [C_NUM_SLOTS-1:0]                          s_axi_reset_out_n_i;
  wire [C_NUM_SLOTS-1:0]                          m_axi_reset_out_n_i;
  wire [C_NUM_SLOTS-1:0]                          interconnect_reset_out_n_i;  // Only [0] is used
  
  genvar slot;
  
  assign INTERCONNECT_RESET_OUT_N = interconnect_reset_out_n_i[0];
  assign S_AXI_RESET_OUT_N        = s_axi_reset_out_n_i;
  assign M_AXI_RESET_OUT_N        = m_axi_reset_out_n_i;

generate
  for (slot=0;slot<C_NUM_SLOTS;slot=slot+1) begin : gen_conv_slot
    if (C_S_AXI_DATA_WIDTH[slot*32+:32] < C_M_AXI_DATA_WIDTH[slot*32+:32]) begin :  gen_upsizer
  ict106_axi_upsizer #
        (
          .C_FAMILY                         (C_FAMILY),
          .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH[slot*32+:32]),
          .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
          .C_S_AXI_DATA_WIDTH               (C_S_AXI_DATA_WIDTH[slot*32+:32]),
          .C_M_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH[slot*32+:32]),
          .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
          .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
          .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
          .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
          .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
          .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
          .C_AXI_SUPPORTS_WRITE             (C_AXI_SUPPORTS_WRITE[slot]),
          .C_AXI_SUPPORTS_READ              (C_AXI_SUPPORTS_READ[slot]),
          .C_M_AXI_AW_REGISTER              (1),
          .C_M_AXI_AR_REGISTER              (1),
          .C_M_AXI_W_REGISTER               (P_LIGHTWT),
          .C_M_AXI_R_REGISTER               ((C_AXI_PROTOCOL[slot*32+:32] != P_AXILITE)? P_FULLY_REG : P_LIGHTWT),
          .C_PACKING_LEVEL                  (P_CONDITIONAL_PACK),
          .C_SUPPORT_BURSTS                 (C_AXI_PROTOCOL[slot*32+:32] != P_AXILITE),
          .C_SINGLE_THREAD                  (1)
        )
        upsizer_inst 
        (
          .ARESETN                          ((C_HEMISPHERE=="mi")? LOCAL_ARESETN : s_axi_reset_out_n_i[slot]),
          .ACLK                             (S_AXI_ACLK[slot]),
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
          .S_AXI_WDATA                      (S_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]),
          .S_AXI_WSTRB                      (S_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_S_AXI_DATA_WIDTH[slot*32+:32]/8]),
          .S_AXI_WLAST                      (S_AXI_WLAST[slot*1+:1]),
          .S_AXI_WUSER                      (S_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .S_AXI_WVALID                     (S_AXI_WVALID[slot*1+:1]),
          .S_AXI_WREADY                     (S_AXI_WREADY[slot*1+:1]),
          .S_AXI_BID                        (si_us_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
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
          .S_AXI_RID                        (si_us_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_RDATA                      (si_us_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]),
          .S_AXI_RRESP                      (S_AXI_RRESP[slot*2+:2]),
          .S_AXI_RLAST                      (S_AXI_RLAST[slot*1+:1]),
          .S_AXI_RUSER                      (S_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .S_AXI_RVALID                     (S_AXI_RVALID[slot*1+:1]),
          .S_AXI_RREADY                     (S_AXI_RREADY[slot*1+:1]),
          .M_AXI_AWID                       (us_cc_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_AWADDR                     (us_cc_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .M_AXI_AWLEN                      (us_cc_awlen[slot*8+:8]),
          .M_AXI_AWSIZE                     (us_cc_awsize[slot*3+:3]),
          .M_AXI_AWBURST                    (us_cc_awburst[slot*2+:2]),
          .M_AXI_AWLOCK                     (us_cc_awlock[slot*2+:2]),
          .M_AXI_AWCACHE                    (us_cc_awcache[slot*4+:4]),
          .M_AXI_AWPROT                     (us_cc_awprot[slot*3+:3]),
          .M_AXI_AWREGION                   (us_cc_awregion[slot*4+:4]),
          .M_AXI_AWQOS                      (us_cc_awqos[slot*4+:4]),
          .M_AXI_AWUSER                     (us_cc_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]),
          .M_AXI_AWVALID                    (us_cc_awvalid[slot*1+:1]),
          .M_AXI_AWREADY                    (us_cc_awready[slot*1+:1]),
          .M_AXI_WDATA                      (us_cc_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]),
          .M_AXI_WSTRB                      (us_cc_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_M_AXI_DATA_WIDTH[slot*32+:32]/8]),
          .M_AXI_WLAST                      (us_cc_wlast[slot*1+:1]),
          .M_AXI_WUSER                      (us_cc_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .M_AXI_WVALID                     (us_cc_wvalid[slot*1+:1]),
          .M_AXI_WREADY                     (us_cc_wready[slot*1+:1]),
          .M_AXI_BID                        (us_cc_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_BRESP                      (us_cc_bresp[slot*2+:2]),
          .M_AXI_BUSER                      (us_cc_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .M_AXI_BVALID                     (us_cc_bvalid[slot*1+:1]),
          .M_AXI_BREADY                     (us_cc_bready[slot*1+:1]),
          .M_AXI_ARID                       (us_cc_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_ARADDR                     (us_cc_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .M_AXI_ARLEN                      (us_cc_arlen[slot*8+:8]),
          .M_AXI_ARSIZE                     (us_cc_arsize[slot*3+:3]),
          .M_AXI_ARBURST                    (us_cc_arburst[slot*2+:2]),
          .M_AXI_ARLOCK                     (us_cc_arlock[slot*2+:2]),
          .M_AXI_ARCACHE                    (us_cc_arcache[slot*4+:4]),
          .M_AXI_ARPROT                     (us_cc_arprot[slot*3+:3]),
          .M_AXI_ARREGION                   (us_cc_arregion[slot*4+:4]),
          .M_AXI_ARQOS                      (us_cc_arqos[slot*4+:4]),
          .M_AXI_ARUSER                     (us_cc_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]),
          .M_AXI_ARVALID                    (us_cc_arvalid[slot*1+:1]),
          .M_AXI_ARREADY                    (us_cc_arready[slot*1+:1]),
          .M_AXI_RID                        (us_cc_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_RDATA                      (us_cc_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]),
          .M_AXI_RRESP                      (us_cc_rresp[slot*2+:2]),
          .M_AXI_RLAST                      (us_cc_rlast[slot*1+:1]),
          .M_AXI_RUSER                      (us_cc_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .M_AXI_RVALID                     (us_cc_rvalid[slot*1+:1]),
          .M_AXI_RREADY                     (us_cc_rready[slot*1+:1])
        );
      assign S_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH] = si_us_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]];
      assign S_AXI_BID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = si_us_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
      assign S_AXI_RID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = si_us_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
    end else begin :gen_no_upsizer
      assign us_cc_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                             = S_AXI_AWID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                           ;
      assign us_cc_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = S_AXI_AWADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                     ;
      assign us_cc_awlen[slot*8+:8]                                                      = S_AXI_AWLEN[slot*8+:8]                                                    ;
      assign us_cc_awsize[slot*3+:3]                                                     = S_AXI_AWSIZE[slot*3+:3]                                                   ;
      assign us_cc_awburst[slot*2+:2]                                                    = S_AXI_AWBURST[slot*2+:2]                                                  ;
      assign us_cc_awlock[slot*2+:2]                                                     = S_AXI_AWLOCK[slot*2+:2]                                                   ;
      assign us_cc_awcache[slot*4+:4]                                                    = S_AXI_AWCACHE[slot*4+:4]                                                  ;
      assign us_cc_awprot[slot*3+:3]                                                     = S_AXI_AWPROT[slot*3+:3]                                                   ;
      assign us_cc_awregion[slot*4+:4]                                                   = S_AXI_AWREGION[slot*4+:4]                                                 ;
      assign us_cc_awqos[slot*4+:4]                                                      = S_AXI_AWQOS[slot*4+:4]                                                    ;
      assign us_cc_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                   = S_AXI_AWUSER[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                 ;
      assign us_cc_awvalid[slot*1+:1]                                                    = S_AXI_AWVALID[slot*1+:1]                                                  ;

      assign us_cc_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]     = S_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]     ;
      assign us_cc_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_S_AXI_DATA_WIDTH[slot*32+:32]/8] = S_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_S_AXI_DATA_WIDTH[slot*32+:32]/8] ;
      assign us_cc_wlast[slot*1+:1]                                                      = S_AXI_WLAST[slot*1+:1]                                                    ;
      assign us_cc_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                      = S_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                    ;
      assign us_cc_wvalid[slot*1+:1]                                                     = S_AXI_WVALID[slot*1+:1]                                                   ;
      assign us_cc_bready[slot*1+:1]                                                     = S_AXI_BREADY[slot*1+:1]                                                   ;
      assign us_cc_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                             = S_AXI_ARID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                           ;
      assign us_cc_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = S_AXI_ARADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                     ;
      assign us_cc_arlen[slot*8+:8]                                                      = S_AXI_ARLEN[slot*8+:8]                                                    ;
      assign us_cc_arsize[slot*3+:3]                                                     = S_AXI_ARSIZE[slot*3+:3]                                                   ;
      assign us_cc_arburst[slot*2+:2]                                                    = S_AXI_ARBURST[slot*2+:2]                                                  ;
      assign us_cc_arlock[slot*2+:2]                                                     = S_AXI_ARLOCK[slot*2+:2]                                                   ;
      assign us_cc_arcache[slot*4+:4]                                                    = S_AXI_ARCACHE[slot*4+:4]                                                  ;
      assign us_cc_arprot[slot*3+:3]                                                     = S_AXI_ARPROT[slot*3+:3]                                                   ;
      assign us_cc_arregion[slot*4+:4]                                                   = S_AXI_ARREGION[slot*4+:4]                                                 ;
      assign us_cc_arqos[slot*4+:4]                                                      = S_AXI_ARQOS[slot*4+:4]                                                    ;
      assign us_cc_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                   = S_AXI_ARUSER[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                 ;
      assign us_cc_arvalid[slot*1+:1]                                                    = S_AXI_ARVALID[slot*1+:1]                                                  ;
      assign us_cc_rready[slot*1+:1]                                                     = S_AXI_RREADY[slot*1+:1]                                                   ;
      
      assign S_AXI_AWREADY[slot*1+:1]                                                    = us_cc_awready[slot*1+:1]                                                  ;
      assign S_AXI_WREADY[slot*1+:1]                                                     = us_cc_wready[slot*1+:1]                                                   ;
      assign S_AXI_BID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                              = us_cc_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                            ;
      assign S_AXI_BRESP[slot*2+:2]                                                      = us_cc_bresp[slot*2+:2]                                                    ;
      assign S_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                      = us_cc_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                    ;
      assign S_AXI_BVALID[slot*1+:1]                                                     = us_cc_bvalid[slot*1+:1]                                                   ;
      assign S_AXI_ARREADY[slot*1+:1]                                                    = us_cc_arready[slot*1+:1]                                                  ;
      assign S_AXI_RID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                              = us_cc_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                            ;
      assign S_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH]     = us_cc_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]     ;
      assign S_AXI_RRESP[slot*2+:2]                                                      = us_cc_rresp[slot*2+:2]                                                    ;
      assign S_AXI_RLAST[slot*1+:1]                                                      = us_cc_rlast[slot*1+:1]                                                    ;
      assign S_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                      = us_cc_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                    ;
      assign S_AXI_RVALID[slot*1+:1]                                                     = us_cc_rvalid[slot*1+:1]                                                   ;
    end
    
  ict106_axi_clock_converter #
        (
          .C_FAMILY                         (C_FAMILY),
          .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH[slot*32+:32]),
          .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
          .C_AXI_DATA_WIDTH                 (f_cc_data_width(slot)),
          .C_AXI_IS_ACLK_ASYNC            (C_AXI_IS_ACLK_ASYNC[slot]),
          .C_S_AXI_ACLK_RATIO               (C_S_AXI_ACLK_RATIO[slot*32+:32]),
          .C_M_AXI_ACLK_RATIO               (C_M_AXI_ACLK_RATIO[slot*32+:32]),
          .C_AXI_PROTOCOL                   (C_AXI_PROTOCOL[slot*32+:32]),
          .C_AXI_SUPPORTS_WRITE           (C_AXI_SUPPORTS_WRITE[slot]),
          .C_AXI_SUPPORTS_READ            (C_AXI_SUPPORTS_READ[slot]),
          .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
          .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
          .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
          .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
          .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
          .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH)
        )
        clock_conv_inst 
        (
          .INTERCONNECT_ACLK                (INTERCONNECT_ACLK),
          .INTERCONNECT_ARESETN             (INTERCONNECT_ARESETN),
          .LOCAL_ARESETN                    (LOCAL_ARESETN),
          .INTERCONNECT_RESET_OUT_N         (interconnect_reset_out_n_i[slot]),
          .S_AXI_RESET_OUT_N                (s_axi_reset_out_n_i[slot]),
          .M_AXI_RESET_OUT_N                (m_axi_reset_out_n_i[slot]),
          .S_AXI_ACLK                       (S_AXI_ACLK[slot]),
          .S_AXI_AWID                       (us_cc_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_AWADDR                     (us_cc_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .S_AXI_AWLEN                      (us_cc_awlen[slot*8+:8]),
          .S_AXI_AWSIZE                     (us_cc_awsize[slot*3+:3]),
          .S_AXI_AWBURST                    (us_cc_awburst[slot*2+:2]),
          .S_AXI_AWLOCK                     (us_cc_awlock[slot*2+:2]),
          .S_AXI_AWCACHE                    (us_cc_awcache[slot*4+:4]),
          .S_AXI_AWPROT                     (us_cc_awprot[slot*3+:3]),
          .S_AXI_AWREGION                   (us_cc_awregion[slot*4+:4]),
          .S_AXI_AWQOS                      (us_cc_awqos[slot*4+:4]),
          .S_AXI_AWUSER                     (us_cc_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]),
          .S_AXI_AWVALID                    (us_cc_awvalid[slot*1+:1]),
          .S_AXI_AWREADY                    (us_cc_awready[slot*1+:1]),
          .S_AXI_WDATA                      (us_cc_wdata[slot*C_AXI_DATA_MAX_WIDTH+:f_cc_data_width(slot)]),
          .S_AXI_WSTRB                      (us_cc_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:f_cc_data_width(slot)/8]),
          .S_AXI_WLAST                      (us_cc_wlast[slot*1+:1]),
          .S_AXI_WUSER                      (us_cc_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .S_AXI_WVALID                     (us_cc_wvalid[slot*1+:1]),
          .S_AXI_WREADY                     (us_cc_wready[slot*1+:1]),
          .S_AXI_BID                        (us_cc_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_BRESP                      (us_cc_bresp[slot*2+:2]),
          .S_AXI_BUSER                      (us_cc_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .S_AXI_BVALID                     (us_cc_bvalid[slot*1+:1]),
          .S_AXI_BREADY                     (us_cc_bready[slot*1+:1]),
          .S_AXI_ARID                       (us_cc_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_ARADDR                     (us_cc_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .S_AXI_ARLEN                      (us_cc_arlen[slot*8+:8]),
          .S_AXI_ARSIZE                     (us_cc_arsize[slot*3+:3]),
          .S_AXI_ARBURST                    (us_cc_arburst[slot*2+:2]),
          .S_AXI_ARLOCK                     (us_cc_arlock[slot*2+:2]),
          .S_AXI_ARCACHE                    (us_cc_arcache[slot*4+:4]),
          .S_AXI_ARPROT                     (us_cc_arprot[slot*3+:3]),
          .S_AXI_ARREGION                   (us_cc_arregion[slot*4+:4]),
          .S_AXI_ARQOS                      (us_cc_arqos[slot*4+:4]),
          .S_AXI_ARUSER                     (us_cc_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]),
          .S_AXI_ARVALID                    (us_cc_arvalid[slot*1+:1]),
          .S_AXI_ARREADY                    (us_cc_arready[slot*1+:1]),
          .S_AXI_RID                        (us_cc_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_RDATA                      (us_cc_rdata[slot*C_AXI_DATA_MAX_WIDTH+:f_cc_data_width(slot)]),
          .S_AXI_RRESP                      (us_cc_rresp[slot*2+:2]),
          .S_AXI_RLAST                      (us_cc_rlast[slot*1+:1]),
          .S_AXI_RUSER                      (us_cc_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .S_AXI_RVALID                     (us_cc_rvalid[slot*1+:1]),
          .S_AXI_RREADY                     (us_cc_rready[slot*1+:1]),
          .M_AXI_ACLK                       (M_AXI_ACLK[slot]),
          .M_AXI_AWID                       (cc_ds_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_AWADDR                     (cc_ds_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .M_AXI_AWLEN                      (cc_ds_awlen[slot*8+:8]),
          .M_AXI_AWSIZE                     (cc_ds_awsize[slot*3+:3]),
          .M_AXI_AWBURST                    (cc_ds_awburst[slot*2+:2]),
          .M_AXI_AWLOCK                     (cc_ds_awlock[slot*2+:2]),
          .M_AXI_AWCACHE                    (cc_ds_awcache[slot*4+:4]),
          .M_AXI_AWPROT                     (cc_ds_awprot[slot*3+:3]),
          .M_AXI_AWREGION                   (cc_ds_awregion[slot*4+:4]),
          .M_AXI_AWQOS                      (cc_ds_awqos[slot*4+:4]),
          .M_AXI_AWUSER                     (cc_ds_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]),
          .M_AXI_AWVALID                    (cc_ds_awvalid[slot*1+:1]),
          .M_AXI_AWREADY                    (cc_ds_awready[slot*1+:1]),
          .M_AXI_WDATA                      (cc_ds_wdata[slot*C_AXI_DATA_MAX_WIDTH+:f_cc_data_width(slot)]),
          .M_AXI_WSTRB                      (cc_ds_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:f_cc_data_width(slot)/8]),
          .M_AXI_WLAST                      (cc_ds_wlast[slot*1+:1]),
          .M_AXI_WUSER                      (cc_ds_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .M_AXI_WVALID                     (cc_ds_wvalid[slot*1+:1]),
          .M_AXI_WREADY                     (cc_ds_wready[slot*1+:1]),
          .M_AXI_BID                        (cc_ds_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_BRESP                      (cc_ds_bresp[slot*2+:2]),
          .M_AXI_BUSER                      (cc_ds_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .M_AXI_BVALID                     (cc_ds_bvalid[slot*1+:1]),
          .M_AXI_BREADY                     (cc_ds_bready[slot*1+:1]),
          .M_AXI_ARID                       (cc_ds_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_ARADDR                     (cc_ds_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .M_AXI_ARLEN                      (cc_ds_arlen[slot*8+:8]),
          .M_AXI_ARSIZE                     (cc_ds_arsize[slot*3+:3]),
          .M_AXI_ARBURST                    (cc_ds_arburst[slot*2+:2]),
          .M_AXI_ARLOCK                     (cc_ds_arlock[slot*2+:2]),
          .M_AXI_ARCACHE                    (cc_ds_arcache[slot*4+:4]),
          .M_AXI_ARPROT                     (cc_ds_arprot[slot*3+:3]),
          .M_AXI_ARREGION                   (cc_ds_arregion[slot*4+:4]),
          .M_AXI_ARQOS                      (cc_ds_arqos[slot*4+:4]),
          .M_AXI_ARUSER                     (cc_ds_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]),
          .M_AXI_ARVALID                    (cc_ds_arvalid[slot*1+:1]),
          .M_AXI_ARREADY                    (cc_ds_arready[slot*1+:1]),
          .M_AXI_RID                        (cc_ds_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_RDATA                      (cc_ds_rdata[slot*C_AXI_DATA_MAX_WIDTH+:f_cc_data_width(slot)]),
          .M_AXI_RRESP                      (cc_ds_rresp[slot*2+:2]),
          .M_AXI_RLAST                      (cc_ds_rlast[slot*1+:1]),
          .M_AXI_RUSER                      (cc_ds_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .M_AXI_RVALID                     (cc_ds_rvalid[slot*1+:1]),
          .M_AXI_RREADY                     (cc_ds_rready[slot*1+:1])
        );
    
    if (C_S_AXI_DATA_WIDTH[slot*32+:32] > C_M_AXI_DATA_WIDTH[slot*32+:32]) begin :  gen_downsizer
  ict106_axi_downsizer #
        (
          .C_FAMILY                         (C_FAMILY),
          .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH[slot*32+:32]),
          .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
          .C_S_AXI_DATA_WIDTH               (C_S_AXI_DATA_WIDTH[slot*32+:32]),
          .C_M_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH[slot*32+:32]),
          .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
          .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
          .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
          .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
          .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
          .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
          .C_AXI_SUPPORTS_WRITE             (C_AXI_SUPPORTS_WRITE[slot]),
          .C_AXI_SUPPORTS_READ              (C_AXI_SUPPORTS_READ[slot]),
          .C_SUPPORT_SPLITTING              (C_AXI_PROTOCOL[slot*32+:32] != P_AXILITE),
          .C_SUPPORT_BURSTS                 (C_AXI_PROTOCOL[slot*32+:32] != P_AXILITE),
          .C_SINGLE_THREAD                  (1)
        )
        downsizer_inst 
        (
          .ARESETN                          ((C_HEMISPHERE=="si") ? LOCAL_ARESETN : m_axi_reset_out_n_i[slot]),
          .ACLK                             (M_AXI_ACLK[slot]),
          .S_AXI_AWID                       (cc_ds_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_AWADDR                     (cc_ds_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .S_AXI_AWLEN                      (cc_ds_awlen[slot*8+:8]),
          .S_AXI_AWSIZE                     (cc_ds_awsize[slot*3+:3]),
          .S_AXI_AWBURST                    (cc_ds_awburst[slot*2+:2]),
          .S_AXI_AWLOCK                     (cc_ds_awlock[slot*2+:2]),
          .S_AXI_AWCACHE                    (cc_ds_awcache[slot*4+:4]),
          .S_AXI_AWPROT                     (cc_ds_awprot[slot*3+:3]),
          .S_AXI_AWREGION                   (cc_ds_awregion[slot*4+:4]),
          .S_AXI_AWQOS                      (cc_ds_awqos[slot*4+:4]),
          .S_AXI_AWUSER                     (cc_ds_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]),
          .S_AXI_AWVALID                    (cc_ds_awvalid[slot*1+:1]),
          .S_AXI_AWREADY                    (cc_ds_awready[slot*1+:1]),
          .S_AXI_WDATA                      (cc_ds_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]),
          .S_AXI_WSTRB                      (cc_ds_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_S_AXI_DATA_WIDTH[slot*32+:32]/8]),
          .S_AXI_WLAST                      (cc_ds_wlast[slot*1+:1]),
          .S_AXI_WUSER                      (cc_ds_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .S_AXI_WVALID                     (cc_ds_wvalid[slot*1+:1]),
          .S_AXI_WREADY                     (cc_ds_wready[slot*1+:1]),
          .S_AXI_BID                        (cc_ds_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_BRESP                      (cc_ds_bresp[slot*2+:2]),
          .S_AXI_BUSER                      (cc_ds_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .S_AXI_BVALID                     (cc_ds_bvalid[slot*1+:1]),
          .S_AXI_BREADY                     (cc_ds_bready[slot*1+:1]),
          .S_AXI_ARID                       (cc_ds_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_ARADDR                     (cc_ds_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]),
          .S_AXI_ARLEN                      (cc_ds_arlen[slot*8+:8]),
          .S_AXI_ARSIZE                     (cc_ds_arsize[slot*3+:3]),
          .S_AXI_ARBURST                    (cc_ds_arburst[slot*2+:2]),
          .S_AXI_ARLOCK                     (cc_ds_arlock[slot*2+:2]),
          .S_AXI_ARCACHE                    (cc_ds_arcache[slot*4+:4]),
          .S_AXI_ARPROT                     (cc_ds_arprot[slot*3+:3]),
          .S_AXI_ARREGION                   (cc_ds_arregion[slot*4+:4]),
          .S_AXI_ARQOS                      (cc_ds_arqos[slot*4+:4]),
          .S_AXI_ARUSER                     (cc_ds_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]),
          .S_AXI_ARVALID                    (cc_ds_arvalid[slot*1+:1]),
          .S_AXI_ARREADY                    (cc_ds_arready[slot*1+:1]),
          .S_AXI_RID                        (cc_ds_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .S_AXI_RDATA                      (cc_ds_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]),
          .S_AXI_RRESP                      (cc_ds_rresp[slot*2+:2]),
          .S_AXI_RLAST                      (cc_ds_rlast[slot*1+:1]),
          .S_AXI_RUSER                      (cc_ds_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .S_AXI_RVALID                     (cc_ds_rvalid[slot*1+:1]),
          .S_AXI_RREADY                     (cc_ds_rready[slot*1+:1]),
          .M_AXI_AWID                       (ds_mi_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
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
          .M_AXI_WDATA                      (ds_mi_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]),
          .M_AXI_WSTRB                      (ds_mi_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_M_AXI_DATA_WIDTH[slot*32+:32]/8]),
          .M_AXI_WLAST                      (M_AXI_WLAST[slot*1+:1]),
          .M_AXI_WUSER                      (M_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]),
          .M_AXI_WVALID                     (M_AXI_WVALID[slot*1+:1]),
          .M_AXI_WREADY                     (M_AXI_WREADY[slot*1+:1]),
          .M_AXI_BID                        (M_AXI_BID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
          .M_AXI_BRESP                      (M_AXI_BRESP[slot*2+:2]),
          .M_AXI_BUSER                      (M_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]),
          .M_AXI_BVALID                     (M_AXI_BVALID[slot*1+:1]),
          .M_AXI_BREADY                     (M_AXI_BREADY[slot*1+:1]),
          .M_AXI_ARID                       (ds_mi_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]),
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
          .M_AXI_RDATA                      (M_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]),
          .M_AXI_RRESP                      (M_AXI_RRESP[slot*2+:2]),
          .M_AXI_RLAST                      (M_AXI_RLAST[slot*1+:1]),
          .M_AXI_RUSER                      (M_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]),
          .M_AXI_RVALID                     (M_AXI_RVALID[slot*1+:1]),
          .M_AXI_RREADY                     (M_AXI_RREADY[slot*1+:1])
        );
      assign M_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH] = ds_mi_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]];
      assign M_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_MAX_WIDTH/8] = ds_mi_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_M_AXI_DATA_WIDTH[slot*32+:32]/8];
      assign M_AXI_AWID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = ds_mi_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
      assign M_AXI_ARID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH] = ds_mi_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]];
    end else begin :gen_no_downsizer
      assign M_AXI_AWID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                             = cc_ds_awid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                           ;
      assign M_AXI_AWADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = cc_ds_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                     ;
      assign M_AXI_AWLEN[slot*8+:8]                                                      = cc_ds_awlen[slot*8+:8]                                                    ;
      assign M_AXI_AWSIZE[slot*3+:3]                                                     = cc_ds_awsize[slot*3+:3]                                                   ;
      assign M_AXI_AWBURST[slot*2+:2]                                                    = cc_ds_awburst[slot*2+:2]                                                  ;
      assign M_AXI_AWLOCK[slot*2+:2]                                                     = cc_ds_awlock[slot*2+:2]                                                   ;
      assign M_AXI_AWCACHE[slot*4+:4]                                                    = cc_ds_awcache[slot*4+:4]                                                  ;
      assign M_AXI_AWPROT[slot*3+:3]                                                     = cc_ds_awprot[slot*3+:3]                                                   ;
      assign M_AXI_AWREGION[slot*4+:4]                                                   = cc_ds_awregion[slot*4+:4]                                                 ;
      assign M_AXI_AWQOS[slot*4+:4]                                                      = cc_ds_awqos[slot*4+:4]                                                    ;
      assign M_AXI_AWUSER[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                   = cc_ds_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                 ;
      assign M_AXI_AWVALID[slot*1+:1]                                                    = cc_ds_awvalid[slot*1+:1]                                                  ;
      assign M_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH]                = cc_ds_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]     ;
      assign M_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_MAX_WIDTH/8]            = cc_ds_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_M_AXI_DATA_WIDTH[slot*32+:32]/8] ;
      assign M_AXI_WLAST[slot*1+:1]                                                      = cc_ds_wlast[slot*1+:1]                                                    ;
      assign M_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                      = cc_ds_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                    ;
      assign M_AXI_WVALID[slot*1+:1]                                                     = cc_ds_wvalid[slot*1+:1]                                                   ;
      assign M_AXI_BREADY[slot*1+:1]                                                     = cc_ds_bready[slot*1+:1]                                                   ;
      assign M_AXI_ARID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                             = cc_ds_arid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                           ;
      assign M_AXI_ARADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = cc_ds_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                     ;
      assign M_AXI_ARLEN[slot*8+:8]                                                      = cc_ds_arlen[slot*8+:8]                                                    ;
      assign M_AXI_ARSIZE[slot*3+:3]                                                     = cc_ds_arsize[slot*3+:3]                                                   ;
      assign M_AXI_ARBURST[slot*2+:2]                                                    = cc_ds_arburst[slot*2+:2]                                                  ;
      assign M_AXI_ARLOCK[slot*2+:2]                                                     = cc_ds_arlock[slot*2+:2]                                                   ;
      assign M_AXI_ARCACHE[slot*4+:4]                                                    = cc_ds_arcache[slot*4+:4]                                                  ;
      assign M_AXI_ARPROT[slot*3+:3]                                                     = cc_ds_arprot[slot*3+:3]                                                   ;
      assign M_AXI_ARREGION[slot*4+:4]                                                   = cc_ds_arregion[slot*4+:4]                                                 ;
      assign M_AXI_ARQOS[slot*4+:4]                                                      = cc_ds_arqos[slot*4+:4]                                                    ;
      assign M_AXI_ARUSER[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                   = cc_ds_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                 ;
      assign M_AXI_ARVALID[slot*1+:1]                                                    = cc_ds_arvalid[slot*1+:1]                                                  ;
      assign M_AXI_RREADY[slot*1+:1]                                                     = cc_ds_rready[slot*1+:1]                                                   ;
      
      assign cc_ds_awready[slot*1+:1]                                                    = M_AXI_AWREADY[slot*1+:1]                                                  ;
      assign cc_ds_wready[slot*1+:1]                                                     = M_AXI_WREADY[slot*1+:1]                                                   ;
      assign cc_ds_bid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                              = M_AXI_BID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                            ;
      assign cc_ds_bresp[slot*2+:2]                                                      = M_AXI_BRESP[slot*2+:2]                                                    ;
      assign cc_ds_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                      = M_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                    ;
      assign cc_ds_bvalid[slot*1+:1]                                                     = M_AXI_BVALID[slot*1+:1]                                                   ;
      assign cc_ds_arready[slot*1+:1]                                                    = M_AXI_ARREADY[slot*1+:1]                                                  ;
      assign cc_ds_rid[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_MAX_WIDTH]                              = M_AXI_RID[slot*C_AXI_ID_MAX_WIDTH+:C_AXI_ID_WIDTH[slot*32+:32]]                            ;
      assign cc_ds_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]     = M_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]     ;
      assign cc_ds_rresp[slot*2+:2]                                                      = M_AXI_RRESP[slot*2+:2]                                                    ;
      assign cc_ds_rlast[slot*1+:1]                                                      = M_AXI_RLAST[slot*1+:1]                                                    ;
      assign cc_ds_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                      = M_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                    ;
      assign cc_ds_rvalid[slot*1+:1]                                                     = M_AXI_RVALID[slot*1+:1]                                                   ;
    end
  end  // gen_conv_slot
endgenerate

endmodule

`default_nettype wire
