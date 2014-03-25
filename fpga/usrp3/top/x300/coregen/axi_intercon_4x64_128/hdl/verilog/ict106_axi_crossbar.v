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
// File name: axi_crossbar.v
//-----------------------------------------------------------------------------
`timescale 1ps/1ps
`default_nettype none

module ict106_axi_crossbar # (
   parameter integer C_MAX_S = 16,
   parameter integer C_MAX_M = 16,
   parameter integer C_NUM_ADDR_RANGES = 16,
   parameter         C_FAMILY                         = "rtl", 
                       // FPGA Base Family. Current version: virtex6 or spartan6.
   parameter integer C_NUM_SLAVE_SLOTS                = 1, 
                       // Number of Slave Interface (SI) slots for connecting 
                       // to master IP. Range: 1-C_MAX_S.
   parameter integer C_NUM_MASTER_SLOTS               = 1, 
                       // Number of Master Interface (MI) slots for connecting 
                       // to slave IP. Range: 1-C_MAX_M.
   parameter integer C_AXI_ID_WIDTH                   = 1, 
                       // Width of ID signals propagated by the Interconnect.
                       // Width of ID signals produced on all MI slots.
                       // Range: 1-16.
   parameter integer C_AXI_ADDR_WIDTH                 = 32, 
                       // Width of S_AXI_AWADDR, S_AXI_ARADDR, M_AXI_AWADDR and 
                       // M_AXI_ARADDR for all SI/MI slots.
                       // Range: 32.
   parameter integer C_INTERCONNECT_DATA_WIDTH        = 32, 
                       // Data width of the internal interconnect write and read 
                       // data paths.
                       // Range: 32, 64, 128, 256, 512, 1024.
   parameter integer C_AXI_DATA_MAX_WIDTH             = 32, 
                       // Largest value specified for any DATA_WIDTH (including C_INTERCONNECT_DATA_WIDTH).
                       // Determines the stride of all DATA signals.
                       // Range: 32, 64, 128, 256, 512, 1024.
   parameter [C_MAX_M*32-1:0] C_M_AXI_DATA_WIDTH               = {C_MAX_M{32'h00000020}}, 
                       // Width of M_AXI_WDATA and M_AXI_RDATA for each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 'h00000020, 'h00000040, 'h00000080, 'h00000100.
   parameter [C_MAX_S*32-1:0] C_S_AXI_PROTOCOL                 = {C_MAX_S{32'h00000000}}, 
                       // Indicates whether connected master is 
                       // Full-AXI4 ('h00000000),
                       // AXI3 ('h00000001) or 
                       // Axi4Lite ('h00000002), for each SI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter [C_MAX_M*32-1:0] C_M_AXI_PROTOCOL                 = {C_MAX_M{32'h00000000}}, 
                       // Indicates whether connected slave is
                       // Full-AXI4 ('h00000000),
                       // AXI3 ('h00000001) or 
                       // Axi4Lite ('h00000002), for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [C_MAX_M*C_NUM_ADDR_RANGES*64-1:0] C_M_AXI_BASE_ADDR                 = {C_MAX_M*C_NUM_ADDR_RANGES{64'hFFFFFFFF_FFFFFFFF}}, 
                       // Base address of each range of each MI slot. 
                       // For unused ranges, set base address to 'hFFFFFFFF.
                       // Format: C_NUM_MASTER_SLOTS{C_NUM_ADDR_RANGES{Bit64}}.
   parameter [C_MAX_M*C_NUM_ADDR_RANGES*64-1:0] C_M_AXI_HIGH_ADDR                 = {C_MAX_M*C_NUM_ADDR_RANGES{64'h00000000_00000000}}, 
                       // High address of each range of each MI slot. 
                       // For unused ranges, set high address to 'h00000000.
                       // Format: C_NUM_MASTER_SLOTS{C_NUM_ADDR_RANGES{Bit64}}.
   parameter [C_MAX_S*64-1:0] C_S_AXI_BASE_ID                  = {C_MAX_S{64'h00000000_00000000}},
                       // Base ID of each SI slot. 
                       // Format: C_NUM_SLAVE_SLOTS{Bit64};
                       // Range: 0 to 2**C_AXI_ID_WIDTH-1.
   parameter [C_MAX_S*64-1:0] C_S_AXI_HIGH_ID                  = {C_MAX_S{64'h00000000_00000000}},
                       // High ID of each SI slot. 
                       // Format: C_NUM_SLAVE_SLOTS{Bit64};
                       // Range: 0 to 2**C_AXI_ID_WIDTH-1.
   parameter [C_MAX_S*32-1:0] C_S_AXI_THREAD_ID_WIDTH                  = {C_MAX_S{32'h00000000}}, 
   parameter [C_MAX_S*1-1:0] C_S_AXI_IS_INTERCONNECT          = {C_MAX_S{1'b0}}, 
                       // Indicates whether connected master is an end-point
                       // master (0) or an interconnect (1), for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [C_MAX_S*1-1:0] C_S_AXI_SUPPORTS_WRITE           = {C_MAX_S{1'b1}}, 
                       // Indicates whether each SI supports write transactions.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [C_MAX_S*1-1:0] C_S_AXI_SUPPORTS_READ            = {C_MAX_S{1'b1}}, 
                       // Indicates whether each SI supports read transactions.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [C_MAX_M*1-1:0] C_M_AXI_SUPPORTS_WRITE           = {C_MAX_M{1'b1}}, 
                       // Indicates whether each MI supports write transactions.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [C_MAX_M*1-1:0] C_M_AXI_SUPPORTS_READ            = {C_MAX_M{1'b1}}, 
                       // Indicates whether each MI supports read transactions.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter integer C_AXI_SUPPORTS_USER_SIGNALS      = 0,
                       // 1 = Propagate all USER signals, 0 = Dont propagate.
   parameter integer C_AXI_AWUSER_WIDTH               = 1,
                       // Width of AWUSER signals for all SI slots and MI slots. 
                       // Range: >= 1.
   parameter integer C_AXI_ARUSER_WIDTH               = 1,
                       // Width of ARUSER signals for all SI slots and MI slots. 
                       // Range: >= 1.
   parameter integer C_AXI_WUSER_WIDTH                = 1,
                       // Width of WUSER signals for all SI slots and MI slots. 
                       // Range: >= 1.
   parameter integer C_AXI_RUSER_WIDTH                = 1,
                       // Width of RUSER signals for all SI slots and MI slots. 
                       // Range: >= 1.
   parameter integer C_AXI_BUSER_WIDTH                = 1,
                       // Width of BUSER signals for all SI slots and MI slots. 
                       // Range: >= 1.
   parameter [C_MAX_M*32-1:0] C_AXI_CONNECTIVITY               = {C_MAX_M{32'hFFFFFFFF}},
                       // Multi-pathway connectivity from each SI slot (N) to each 
                       // MI slot (M):
                       // 0 = no pathway required; 1 = pathway required.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}; 
                       // Range: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter integer C_INTERCONNECT_R_REGISTER               = 0,
                       // Insert register slice on R channel in the crossbar.
                       // Range: 0-8.
   parameter [C_MAX_S*1-1:0] C_S_AXI_SINGLE_THREAD                 = {C_MAX_S{1'b0}}, 
                       // 0 = Implement separate command queues per ID thread.
                       // 1 = Force corresponding SI slot to be single-threaded.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}; 
   parameter [C_MAX_M*1-1:0] C_M_AXI_SUPPORTS_REORDERING      = {C_MAX_M{1'b1}},
                       // Indicates whether the slave connected to each MI slot 
                       // supports response reordering.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}; 
   parameter [C_MAX_S*32-1:0] C_S_AXI_WRITE_ACCEPTANCE         = {C_MAX_S{32'h00000001}},
                       // Maximum number of active write transactions that each SI 
                       // slot can accept.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}; 
                       // Range: 2**0 - 2**5.
   parameter [C_MAX_S*32-1:0] C_S_AXI_READ_ACCEPTANCE          = {C_MAX_S{32'h00000001}},
                       // Maximum number of active read transactions that each SI 
                       // slot can accept.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 2**0 - 2**5.
   parameter [C_MAX_M*32-1:0] C_M_AXI_WRITE_ISSUING            = {C_MAX_M{32'h00000001}},
                       // Maximum number of data-active write transactions that 
                       // each MI slot can generate at any one time.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 2**0 - 2**5.
   parameter [C_MAX_M*32-1:0] C_M_AXI_READ_ISSUING            = {C_MAX_M{32'h00000001}},
                       // Maximum number of active read transactions that 
                       // each MI slot can generate at any one time.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 2**0 - 2**5.
   parameter [C_MAX_S*32-1:0] C_S_AXI_ARB_PRIORITY             = {C_MAX_S{32'h00000000}},
                       // Arbitration priority among each SI slot. 
                       // Higher values indicate higher priority.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 'h0-'hF.
   parameter [C_MAX_M*1-1:0] C_M_AXI_SECURE                   = {C_MAX_M{1'b0}},
                       // Indicates whether each MI slot connects to a secure slave 
                       // (allows only TrustZone secure access).
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter integer C_USE_CTRL_PORT                  = 0,
                       // Indicates whether diagnostic information is accessible 
                       // via the S_AXI_CTRL interface.
   parameter integer C_USE_INTERRUPT                  = 1,
                       // If CTRL interface enabled, indicates whether interrupts 
                       // are generated.
   parameter integer C_RANGE_CHECK                    = 2,
                       // 1 (non-zero) = Detect and issue DECERR on the following conditions:
                       //   a. address range mismatch (no valid MI slot)
                       //   b. Burst or >32-bit transfer to AxiLite slave
                       //   c. TrustZone access violation
                       //   d. R/W direction unsupported by target
                       // 0 = Pass all transactions (no DECERR):
                       //   a. Omit DECERR detection and response logic
                       //   b. Omit address decoder when only 1 MI slot (M_AXI_A*REGION = 0 always)
                       //   c. Unpredictable target MI-slot if address mismatch and >1 MI-slot
                       //   d. Transaction corruption if any burst or >32-bit transfer to AxiLite slave
                       // Illegal combination: C_RANGE_CHECK = 0 && C_M_AXI_SECURE != 0.
   parameter integer C_ADDR_DECODE                    = 0,
                       // 1 = Implement address decoder.
                       // 0 = Propagate address to single slave.
   parameter integer C_S_AXI_CTRL_ADDR_WIDTH          = 32,
                       // ADDR width of CTRL interface.
   parameter integer C_S_AXI_CTRL_DATA_WIDTH          = 32,
                       // DATA width of CTRL interface.
   parameter integer C_INTERCONNECT_CONNECTIVITY_MODE = 1,
                       // 0 = Shared-Address Shared-Data (SASD).
                       // 1 = Shared-Address Multi-Data (SAMD).
   parameter [(C_MAX_M+1)*32-1:0] C_W_ISSUE_WIDTH  = {C_MAX_M+1{32'h00000000}},
   parameter [(C_MAX_M+1)*32-1:0] C_R_ISSUE_WIDTH  = {C_MAX_M+1{32'h00000000}},
   parameter [C_MAX_S*32-1:0] C_W_ACCEPT_WIDTH = {C_MAX_S{32'h00000000}},
   parameter [C_MAX_S*32-1:0] C_R_ACCEPT_WIDTH = {C_MAX_S{32'h00000000}},
   parameter integer C_DEBUG              = 1,
   parameter integer C_MAX_DEBUG_THREADS  = 1
)
(
   // Global Signals
   input  wire                                                    INTERCONNECT_ACLK,
   input  wire                                                    ARESETN,
   output wire                                                    IRQ,
   // Slave Interface Write Address Ports
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]             S_AXI_AWID,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]           S_AXI_AWADDR,
   input  wire [C_NUM_SLAVE_SLOTS*8-1:0]                          S_AXI_AWLEN,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                          S_AXI_AWSIZE,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                          S_AXI_AWBURST,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                          S_AXI_AWLOCK,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                          S_AXI_AWCACHE,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                          S_AXI_AWPROT,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                          S_AXI_AWQOS,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_AWUSER_WIDTH-1:0]         S_AXI_AWUSER,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_AWVALID,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_AWREADY,
   // Slave Interface Write Data Ports
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]       S_AXI_WDATA,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]     S_AXI_WSTRB,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_WLAST,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_WUSER_WIDTH-1:0]          S_AXI_WUSER,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_WVALID,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_WREADY,
   // Slave Interface Write Response Ports
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]             S_AXI_BID,
   output wire [C_NUM_SLAVE_SLOTS*2-1:0]                          S_AXI_BRESP,
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_BUSER_WIDTH-1:0]          S_AXI_BUSER,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_BVALID,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_BREADY,
   // Slave Interface Read Address Ports
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]             S_AXI_ARID,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]           S_AXI_ARADDR,
   input  wire [C_NUM_SLAVE_SLOTS*8-1:0]                          S_AXI_ARLEN,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                          S_AXI_ARSIZE,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                          S_AXI_ARBURST,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                          S_AXI_ARLOCK,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                          S_AXI_ARCACHE,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                          S_AXI_ARPROT,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                          S_AXI_ARQOS,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ARUSER_WIDTH-1:0]         S_AXI_ARUSER,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_ARVALID,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_ARREADY,
   // Slave Interface Read Data Ports
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]             S_AXI_RID,
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]       S_AXI_RDATA,
   output wire [C_NUM_SLAVE_SLOTS*2-1:0]                          S_AXI_RRESP,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_RLAST,
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_RUSER_WIDTH-1:0]          S_AXI_RUSER,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_RVALID,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                            S_AXI_RREADY,
   // Master Interface Write Address Port
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]            M_AXI_AWID,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]          M_AXI_AWADDR,
   output wire [C_NUM_MASTER_SLOTS*8-1:0]                         M_AXI_AWLEN,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                         M_AXI_AWSIZE,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                         M_AXI_AWBURST,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                         M_AXI_AWLOCK,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                         M_AXI_AWCACHE,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                         M_AXI_AWPROT,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                         M_AXI_AWREGION,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                         M_AXI_AWQOS,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]        M_AXI_AWUSER,
   output wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_AWVALID,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_AWREADY,
   // Master Interface Write Data Ports
   output wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]      M_AXI_WDATA,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]    M_AXI_WSTRB,
   output wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_WLAST,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]         M_AXI_WUSER,
   output wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_WVALID,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_WREADY,
   // Master Interface Write Response Ports
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]            M_AXI_BID,
   input  wire [C_NUM_MASTER_SLOTS*2-1:0]                         M_AXI_BRESP,
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]         M_AXI_BUSER,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_BVALID,
   output wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_BREADY,
   // Master Interface Read Address Port
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]            M_AXI_ARID,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]          M_AXI_ARADDR,
   output wire [C_NUM_MASTER_SLOTS*8-1:0]                         M_AXI_ARLEN,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                         M_AXI_ARSIZE,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                         M_AXI_ARBURST,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                         M_AXI_ARLOCK,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                         M_AXI_ARCACHE,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                         M_AXI_ARPROT,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                         M_AXI_ARREGION,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                         M_AXI_ARQOS,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]        M_AXI_ARUSER,
   output wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_ARVALID,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_ARREADY,
   // Master Interface Read Data Ports
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]            M_AXI_RID,
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]      M_AXI_RDATA,
   input  wire [C_NUM_MASTER_SLOTS*2-1:0]                         M_AXI_RRESP,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_RLAST,
   input wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]          M_AXI_RUSER,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_RVALID,
   output wire [C_NUM_MASTER_SLOTS-1:0]                           M_AXI_RREADY,
   // Diagnostic AxiLite Slave Interface
   input wire [(C_S_AXI_CTRL_ADDR_WIDTH-1):0]                     S_AXI_CTRL_AWADDR,
   input wire                                                     S_AXI_CTRL_AWVALID,
   output wire                                                    S_AXI_CTRL_AWREADY,
   input wire [(C_S_AXI_CTRL_DATA_WIDTH-1):0]                     S_AXI_CTRL_WDATA,
   input wire                                                     S_AXI_CTRL_WVALID,
   output wire                                                    S_AXI_CTRL_WREADY,
   output wire [1:0]                                              S_AXI_CTRL_BRESP,
   output wire                                                    S_AXI_CTRL_BVALID,
   input wire                                                     S_AXI_CTRL_BREADY,
   input wire [(C_S_AXI_CTRL_ADDR_WIDTH-1):0]                     S_AXI_CTRL_ARADDR,
   input wire                                                     S_AXI_CTRL_ARVALID,
   output wire                                                    S_AXI_CTRL_ARREADY,
   output wire [(C_S_AXI_CTRL_DATA_WIDTH-1):0]                    S_AXI_CTRL_RDATA,
   output wire [1:0]                                              S_AXI_CTRL_RRESP,
   output wire                                                    S_AXI_CTRL_RVALID,
   input wire                                                     S_AXI_CTRL_RREADY,
   // Diagnostic Probe Ports
   output wire [8-1:0]                                            DEBUG_AW_TRANS_SEQ,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS-1:0]        DEBUG_AW_TRANS_QUAL,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_AW_ACCEPT_CNT,
   output wire [C_NUM_SLAVE_SLOTS*16-1:0]                         DEBUG_AW_ACTIVE_THREAD,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_AW_ACTIVE_TARGET,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_AW_ACTIVE_REGION,
   output wire [C_NUM_SLAVE_SLOTS*8-1:0]                          DEBUG_AW_ERROR,
   output wire [C_NUM_SLAVE_SLOTS*8-1:0]                          DEBUG_AW_TARGET,
   output wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     DEBUG_AW_ISSUING_CNT,
   output wire [8-1:0]                                            DEBUG_AW_ARB_GRANT,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_B_TRANS_SEQ,
   output wire [8-1:0]                                            DEBUG_AR_TRANS_SEQ,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS-1:0]        DEBUG_AR_TRANS_QUAL,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_AR_ACCEPT_CNT,
   output wire [C_NUM_SLAVE_SLOTS*16-1:0]                         DEBUG_AR_ACTIVE_THREAD,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_AR_ACTIVE_TARGET,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_AR_ACTIVE_REGION,
   output wire [C_NUM_SLAVE_SLOTS*8-1:0]                          DEBUG_AR_ERROR,
   output wire [C_NUM_SLAVE_SLOTS*8-1:0]                          DEBUG_AR_TARGET,
   output wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     DEBUG_AR_ISSUING_CNT,
   output wire [8-1:0]                                            DEBUG_AR_ARB_GRANT,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_R_BEAT_CNT,
   output wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      DEBUG_R_TRANS_SEQ,
   output wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     DEBUG_RID_TARGET,
   output wire [(C_NUM_MASTER_SLOTS+1)-1:0]                       DEBUG_RID_ERROR,
   output wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     DEBUG_BID_TARGET,
   output wire [(C_NUM_MASTER_SLOTS+1)-1:0]                       DEBUG_BID_ERROR,
   output wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     DEBUG_W_BEAT_CNT,
   output wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     DEBUG_W_TRANS_SEQ
);


generate
  if (C_INTERCONNECT_CONNECTIVITY_MODE==1) begin : gen_samd
  ict106_crossbar #
      (
        .C_MAX_S                          (C_MAX_S),
        .C_MAX_M                          (C_MAX_M),
        .C_NUM_ADDR_RANGES                (C_NUM_ADDR_RANGES),
        .C_FAMILY                         (C_FAMILY),
        .C_NUM_SLAVE_SLOTS                (C_NUM_SLAVE_SLOTS),
        .C_NUM_MASTER_SLOTS               (C_NUM_MASTER_SLOTS),
        .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH),
        .C_S_AXI_THREAD_ID_WIDTH          (C_S_AXI_THREAD_ID_WIDTH),
        .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
        .C_INTERCONNECT_DATA_WIDTH        (C_INTERCONNECT_DATA_WIDTH),
        .C_AXI_DATA_MAX_WIDTH             (C_AXI_DATA_MAX_WIDTH),
        .C_M_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH),  // Used to determine whether W-channel gets reg-slice
        .C_S_AXI_PROTOCOL                 (C_S_AXI_PROTOCOL),
        .C_M_AXI_PROTOCOL                 (C_M_AXI_PROTOCOL),
        .C_M_AXI_BASE_ADDR                (C_M_AXI_BASE_ADDR),
        .C_M_AXI_HIGH_ADDR                (C_M_AXI_HIGH_ADDR),
        .C_S_AXI_BASE_ID                  (C_S_AXI_BASE_ID),
        .C_S_AXI_HIGH_ID                  (C_S_AXI_HIGH_ID),
        .C_S_AXI_IS_INTERCONNECT          (C_S_AXI_IS_INTERCONNECT),
        .C_S_AXI_SUPPORTS_WRITE           (C_S_AXI_SUPPORTS_WRITE),
        .C_S_AXI_SUPPORTS_READ            (C_S_AXI_SUPPORTS_READ),
        .C_M_AXI_SUPPORTS_WRITE           (C_M_AXI_SUPPORTS_WRITE),
        .C_M_AXI_SUPPORTS_READ            (C_M_AXI_SUPPORTS_READ),
        .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
        .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
        .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
        .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
        .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
        .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
        .C_AXI_CONNECTIVITY               (C_AXI_CONNECTIVITY),
        .C_S_AXI_SINGLE_THREAD            (C_S_AXI_SINGLE_THREAD),
        .C_M_AXI_SUPPORTS_REORDERING      (C_M_AXI_SUPPORTS_REORDERING),
        .C_S_AXI_WRITE_ACCEPTANCE         (C_S_AXI_WRITE_ACCEPTANCE),
        .C_S_AXI_READ_ACCEPTANCE          (C_S_AXI_READ_ACCEPTANCE),
        .C_M_AXI_WRITE_ISSUING            (C_M_AXI_WRITE_ISSUING),
        .C_M_AXI_READ_ISSUING             (C_M_AXI_READ_ISSUING),
    //    .C_S_AXI_ARB_METHOD               (C_S_AXI_ARB_METHOD), // Reserved for future
        .C_S_AXI_ARB_PRIORITY             (C_S_AXI_ARB_PRIORITY),
    //    .C_S_AXI_ARB_TDM_SLOTS            (C_S_AXI_ARB_TDM_SLOTS), // Reserved for future
    //    .C_S_AXI_ARB_TDM_TOTAL            (C_S_AXI_ARB_TDM_TOTAL), // Reserved for future
        .C_M_AXI_SECURE                   (C_M_AXI_SECURE),
        .C_USE_CTRL_PORT                  (C_USE_CTRL_PORT),
        .C_USE_INTERRUPT                  (C_USE_INTERRUPT),
        .C_RANGE_CHECK                    (C_RANGE_CHECK),
        .C_ADDR_DECODE                    (C_ADDR_DECODE),
        .C_S_AXI_CTRL_ADDR_WIDTH          (C_S_AXI_CTRL_ADDR_WIDTH),
        .C_S_AXI_CTRL_DATA_WIDTH          (C_S_AXI_CTRL_DATA_WIDTH),
        .C_W_ISSUE_WIDTH                  (C_W_ISSUE_WIDTH ),
        .C_R_ISSUE_WIDTH                  (C_R_ISSUE_WIDTH ),
        .C_W_ACCEPT_WIDTH                 (C_W_ACCEPT_WIDTH),
        .C_R_ACCEPT_WIDTH                 (C_R_ACCEPT_WIDTH),
        .C_DEBUG                          (C_DEBUG),
        .C_MAX_DEBUG_THREADS              (C_MAX_DEBUG_THREADS)
      )
      crossbar_samd 
      (
        .INTERCONNECT_ACLK                (INTERCONNECT_ACLK),
        .ARESETN                          (ARESETN),
        .IRQ                              (IRQ),
        .S_AXI_AWID                       (S_AXI_AWID             ),
        .S_AXI_AWADDR                     (S_AXI_AWADDR           ),
        .S_AXI_AWLEN                      (S_AXI_AWLEN            ),
        .S_AXI_AWSIZE                     (S_AXI_AWSIZE           ),
        .S_AXI_AWBURST                    (S_AXI_AWBURST          ),
        .S_AXI_AWLOCK                     (S_AXI_AWLOCK           ),
        .S_AXI_AWCACHE                    (S_AXI_AWCACHE          ),
        .S_AXI_AWPROT                     (S_AXI_AWPROT           ),
        .S_AXI_AWQOS                      (S_AXI_AWQOS            ),
        .S_AXI_AWUSER                     (S_AXI_AWUSER           ),
        .S_AXI_AWVALID                    (S_AXI_AWVALID          ),
        .S_AXI_AWREADY                    (S_AXI_AWREADY          ),
        .S_AXI_WDATA                      (S_AXI_WDATA            ),
        .S_AXI_WSTRB                      (S_AXI_WSTRB            ),
        .S_AXI_WLAST                      (S_AXI_WLAST            ),
        .S_AXI_WUSER                      (S_AXI_WUSER            ),
        .S_AXI_WVALID                     (S_AXI_WVALID           ),
        .S_AXI_WREADY                     (S_AXI_WREADY           ),
        .S_AXI_BID                        (S_AXI_BID              ),
        .S_AXI_BRESP                      (S_AXI_BRESP            ),
        .S_AXI_BUSER                      (S_AXI_BUSER            ),
        .S_AXI_BVALID                     (S_AXI_BVALID           ),
        .S_AXI_BREADY                     (S_AXI_BREADY           ),
        .S_AXI_ARID                       (S_AXI_ARID             ),
        .S_AXI_ARADDR                     (S_AXI_ARADDR           ),
        .S_AXI_ARLEN                      (S_AXI_ARLEN            ),
        .S_AXI_ARSIZE                     (S_AXI_ARSIZE           ),
        .S_AXI_ARBURST                    (S_AXI_ARBURST          ),
        .S_AXI_ARLOCK                     (S_AXI_ARLOCK           ),
        .S_AXI_ARCACHE                    (S_AXI_ARCACHE          ),
        .S_AXI_ARPROT                     (S_AXI_ARPROT           ),
        .S_AXI_ARQOS                      (S_AXI_ARQOS            ),
        .S_AXI_ARUSER                     (S_AXI_ARUSER           ),
        .S_AXI_ARVALID                    (S_AXI_ARVALID          ),
        .S_AXI_ARREADY                    (S_AXI_ARREADY          ),
        .S_AXI_RID                        (S_AXI_RID              ),
        .S_AXI_RDATA                      (S_AXI_RDATA            ),
        .S_AXI_RRESP                      (S_AXI_RRESP            ),
        .S_AXI_RLAST                      (S_AXI_RLAST            ),
        .S_AXI_RUSER                      (S_AXI_RUSER            ),
        .S_AXI_RVALID                     (S_AXI_RVALID           ),
        .S_AXI_RREADY                     (S_AXI_RREADY           ),
        .M_AXI_AWID                       (M_AXI_AWID             ),
        .M_AXI_AWADDR                     (M_AXI_AWADDR           ),
        .M_AXI_AWLEN                      (M_AXI_AWLEN            ),
        .M_AXI_AWSIZE                     (M_AXI_AWSIZE           ),
        .M_AXI_AWBURST                    (M_AXI_AWBURST          ),
        .M_AXI_AWLOCK                     (M_AXI_AWLOCK           ),
        .M_AXI_AWCACHE                    (M_AXI_AWCACHE          ),
        .M_AXI_AWPROT                     (M_AXI_AWPROT           ),
        .M_AXI_AWREGION                   (M_AXI_AWREGION         ),
        .M_AXI_AWQOS                      (M_AXI_AWQOS            ),
        .M_AXI_AWUSER                     (M_AXI_AWUSER           ),
        .M_AXI_AWVALID                    (M_AXI_AWVALID          ),
        .M_AXI_AWREADY                    (M_AXI_AWREADY          ),
        .M_AXI_WDATA                      (M_AXI_WDATA            ),
        .M_AXI_WSTRB                      (M_AXI_WSTRB            ),
        .M_AXI_WLAST                      (M_AXI_WLAST            ),
        .M_AXI_WUSER                      (M_AXI_WUSER            ),
        .M_AXI_WVALID                     (M_AXI_WVALID           ),
        .M_AXI_WREADY                     (M_AXI_WREADY           ),
        .M_AXI_BID                        (M_AXI_BID              ),
        .M_AXI_BRESP                      (M_AXI_BRESP            ),
        .M_AXI_BUSER                      (M_AXI_BUSER            ),
        .M_AXI_BVALID                     (M_AXI_BVALID           ),
        .M_AXI_BREADY                     (M_AXI_BREADY           ),
        .M_AXI_ARID                       (M_AXI_ARID             ),
        .M_AXI_ARADDR                     (M_AXI_ARADDR           ),
        .M_AXI_ARLEN                      (M_AXI_ARLEN            ),
        .M_AXI_ARSIZE                     (M_AXI_ARSIZE           ),
        .M_AXI_ARBURST                    (M_AXI_ARBURST          ),
        .M_AXI_ARLOCK                     (M_AXI_ARLOCK           ),
        .M_AXI_ARCACHE                    (M_AXI_ARCACHE          ),
        .M_AXI_ARPROT                     (M_AXI_ARPROT           ),
        .M_AXI_ARREGION                   (M_AXI_ARREGION         ),
        .M_AXI_ARQOS                      (M_AXI_ARQOS            ),
        .M_AXI_ARUSER                     (M_AXI_ARUSER           ),
        .M_AXI_ARVALID                    (M_AXI_ARVALID          ),
        .M_AXI_ARREADY                    (M_AXI_ARREADY          ),
        .M_AXI_RID                        (M_AXI_RID              ),
        .M_AXI_RDATA                      (M_AXI_RDATA            ),
        .M_AXI_RRESP                      (M_AXI_RRESP            ),
        .M_AXI_RLAST                      (M_AXI_RLAST            ),
        .M_AXI_RUSER                      (M_AXI_RUSER            ),
        .M_AXI_RVALID                     (M_AXI_RVALID           ),
        .M_AXI_RREADY                     (M_AXI_RREADY           ),
        .S_AXI_CTRL_AWADDR                (S_AXI_CTRL_AWADDR      ),
        .S_AXI_CTRL_AWVALID               (S_AXI_CTRL_AWVALID     ),
        .S_AXI_CTRL_AWREADY               (S_AXI_CTRL_AWREADY     ),
        .S_AXI_CTRL_WDATA                 (S_AXI_CTRL_WDATA       ),
        .S_AXI_CTRL_WVALID                (S_AXI_CTRL_WVALID      ),
        .S_AXI_CTRL_WREADY                (S_AXI_CTRL_WREADY      ),
        .S_AXI_CTRL_BRESP                 (S_AXI_CTRL_BRESP       ),
        .S_AXI_CTRL_BVALID                (S_AXI_CTRL_BVALID      ),
        .S_AXI_CTRL_BREADY                (S_AXI_CTRL_BREADY      ),
        .S_AXI_CTRL_ARADDR                (S_AXI_CTRL_ARADDR      ),
        .S_AXI_CTRL_ARVALID               (S_AXI_CTRL_ARVALID     ),
        .S_AXI_CTRL_ARREADY               (S_AXI_CTRL_ARREADY     ),
        .S_AXI_CTRL_RDATA                 (S_AXI_CTRL_RDATA       ),
        .S_AXI_CTRL_RRESP                 (S_AXI_CTRL_RRESP       ),
        .S_AXI_CTRL_RVALID                (S_AXI_CTRL_RVALID      ),
        .S_AXI_CTRL_RREADY                (S_AXI_CTRL_RREADY      ),
        .DEBUG_AW_TRANS_SEQ               (DEBUG_AW_TRANS_SEQ     ),
        .DEBUG_AW_TRANS_QUAL              (DEBUG_AW_TRANS_QUAL    ),
        .DEBUG_AW_ACCEPT_CNT              (DEBUG_AW_ACCEPT_CNT    ),
        .DEBUG_AW_ACTIVE_THREAD           (DEBUG_AW_ACTIVE_THREAD ),
        .DEBUG_AW_ACTIVE_TARGET           (DEBUG_AW_ACTIVE_TARGET ),
        .DEBUG_AW_ACTIVE_REGION           (DEBUG_AW_ACTIVE_REGION ),
        .DEBUG_AW_ERROR                   (DEBUG_AW_ERROR         ),
        .DEBUG_AW_TARGET                  (DEBUG_AW_TARGET        ),
        .DEBUG_AW_ISSUING_CNT             (DEBUG_AW_ISSUING_CNT   ),
        .DEBUG_AW_ARB_GRANT               (DEBUG_AW_ARB_GRANT     ),
        .DEBUG_B_TRANS_SEQ                (DEBUG_B_TRANS_SEQ      ),
        .DEBUG_AR_TRANS_SEQ               (DEBUG_AR_TRANS_SEQ     ),
        .DEBUG_AR_TRANS_QUAL              (DEBUG_AR_TRANS_QUAL    ),
        .DEBUG_AR_ACCEPT_CNT              (DEBUG_AR_ACCEPT_CNT    ),
        .DEBUG_AR_ACTIVE_THREAD           (DEBUG_AR_ACTIVE_THREAD ),
        .DEBUG_AR_ACTIVE_TARGET           (DEBUG_AR_ACTIVE_TARGET ),
        .DEBUG_AR_ACTIVE_REGION           (DEBUG_AR_ACTIVE_REGION ),
        .DEBUG_AR_ERROR                   (DEBUG_AR_ERROR         ),
        .DEBUG_AR_TARGET                  (DEBUG_AR_TARGET        ),
        .DEBUG_AR_ISSUING_CNT             (DEBUG_AR_ISSUING_CNT   ),
        .DEBUG_AR_ARB_GRANT               (DEBUG_AR_ARB_GRANT     ),
        .DEBUG_R_BEAT_CNT                 (DEBUG_R_BEAT_CNT       ),
        .DEBUG_R_TRANS_SEQ                (DEBUG_R_TRANS_SEQ      ),
        .DEBUG_RID_TARGET                 (DEBUG_RID_TARGET       ),
        .DEBUG_RID_ERROR                  (DEBUG_RID_ERROR        ),
        .DEBUG_BID_TARGET                 (DEBUG_BID_TARGET       ),
        .DEBUG_BID_ERROR                  (DEBUG_BID_ERROR        ),
        .DEBUG_W_BEAT_CNT                 (DEBUG_W_BEAT_CNT       ),
        .DEBUG_W_TRANS_SEQ                (DEBUG_W_TRANS_SEQ      )
      );
  end else begin : gen_sasd
  ict106_crossbar_sasd #
      (
        .C_MAX_S                          (C_MAX_S),
        .C_MAX_M                          (C_MAX_M),
        .C_NUM_ADDR_RANGES                (C_NUM_ADDR_RANGES),
        .C_FAMILY                         (C_FAMILY),
        .C_NUM_SLAVE_SLOTS                (C_NUM_SLAVE_SLOTS),
        .C_NUM_MASTER_SLOTS               (C_NUM_MASTER_SLOTS),
        .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH),
        .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
        .C_INTERCONNECT_DATA_WIDTH        (C_INTERCONNECT_DATA_WIDTH),
        .C_AXI_DATA_MAX_WIDTH             (C_AXI_DATA_MAX_WIDTH),
        .C_S_AXI_PROTOCOL                 (C_S_AXI_PROTOCOL),
        .C_M_AXI_PROTOCOL                 (C_M_AXI_PROTOCOL),
        .C_M_AXI_BASE_ADDR                (C_M_AXI_BASE_ADDR),
        .C_M_AXI_HIGH_ADDR                (C_M_AXI_HIGH_ADDR),
        .C_S_AXI_BASE_ID                  (C_S_AXI_BASE_ID),
        .C_S_AXI_HIGH_ID                  (C_S_AXI_HIGH_ID),
        .C_S_AXI_SUPPORTS_WRITE           (C_S_AXI_SUPPORTS_WRITE),
        .C_S_AXI_SUPPORTS_READ            (C_S_AXI_SUPPORTS_READ),
        .C_M_AXI_SUPPORTS_WRITE           (C_M_AXI_SUPPORTS_WRITE),
        .C_M_AXI_SUPPORTS_READ            (C_M_AXI_SUPPORTS_READ),
        .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
        .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
        .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
        .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
        .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
        .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    //    .C_S_AXI_ARB_METHOD               (C_S_AXI_ARB_METHOD), // Reserved for future
        .C_S_AXI_ARB_PRIORITY             (C_S_AXI_ARB_PRIORITY),
    //    .C_S_AXI_ARB_TDM_SLOTS            (C_S_AXI_ARB_TDM_SLOTS), // Reserved for future
    //    .C_S_AXI_ARB_TDM_TOTAL            (C_S_AXI_ARB_TDM_TOTAL), // Reserved for future
        .C_M_AXI_SECURE                   (C_M_AXI_SECURE),
        .C_INTERCONNECT_R_REGISTER        (C_INTERCONNECT_R_REGISTER),
        .C_USE_CTRL_PORT                  (C_USE_CTRL_PORT),
        .C_USE_INTERRUPT                  (C_USE_INTERRUPT),
        .C_RANGE_CHECK                    (C_RANGE_CHECK),
        .C_ADDR_DECODE                    (C_ADDR_DECODE),
        .C_S_AXI_CTRL_ADDR_WIDTH          (C_S_AXI_CTRL_ADDR_WIDTH),
        .C_S_AXI_CTRL_DATA_WIDTH          (C_S_AXI_CTRL_DATA_WIDTH),
        .C_DEBUG                          (C_DEBUG),
        .C_MAX_DEBUG_THREADS              (C_MAX_DEBUG_THREADS)
      )
      crossbar_sasd_0
      (
        .INTERCONNECT_ACLK                (INTERCONNECT_ACLK),
        .ARESETN                          (ARESETN),
        .IRQ                              (IRQ),
        .S_AXI_AWID                       (S_AXI_AWID           ),
        .S_AXI_AWADDR                     (S_AXI_AWADDR         ),
        .S_AXI_AWLEN                      (S_AXI_AWLEN          ),
        .S_AXI_AWSIZE                     (S_AXI_AWSIZE         ),
        .S_AXI_AWBURST                    (S_AXI_AWBURST        ),
        .S_AXI_AWLOCK                     (S_AXI_AWLOCK         ),
        .S_AXI_AWCACHE                    (S_AXI_AWCACHE        ),
        .S_AXI_AWPROT                     (S_AXI_AWPROT         ),
        .S_AXI_AWQOS                      (S_AXI_AWQOS          ),
        .S_AXI_AWUSER                     (S_AXI_AWUSER         ),
        .S_AXI_AWVALID                    (S_AXI_AWVALID        ),
        .S_AXI_AWREADY                    (S_AXI_AWREADY        ),
        .S_AXI_WDATA                      (S_AXI_WDATA          ),
        .S_AXI_WSTRB                      (S_AXI_WSTRB          ),
        .S_AXI_WLAST                      (S_AXI_WLAST          ),
        .S_AXI_WUSER                      (S_AXI_WUSER          ),
        .S_AXI_WVALID                     (S_AXI_WVALID         ),
        .S_AXI_WREADY                     (S_AXI_WREADY         ),
        .S_AXI_BID                        (S_AXI_BID            ),
        .S_AXI_BRESP                      (S_AXI_BRESP          ),
        .S_AXI_BUSER                      (S_AXI_BUSER          ),
        .S_AXI_BVALID                     (S_AXI_BVALID         ),
        .S_AXI_BREADY                     (S_AXI_BREADY         ),
        .S_AXI_ARID                       (S_AXI_ARID           ),
        .S_AXI_ARADDR                     (S_AXI_ARADDR         ),
        .S_AXI_ARLEN                      (S_AXI_ARLEN          ),
        .S_AXI_ARSIZE                     (S_AXI_ARSIZE         ),
        .S_AXI_ARBURST                    (S_AXI_ARBURST        ),
        .S_AXI_ARLOCK                     (S_AXI_ARLOCK         ),
        .S_AXI_ARCACHE                    (S_AXI_ARCACHE        ),
        .S_AXI_ARPROT                     (S_AXI_ARPROT         ),
        .S_AXI_ARQOS                      (S_AXI_ARQOS          ),
        .S_AXI_ARUSER                     (S_AXI_ARUSER         ),
        .S_AXI_ARVALID                    (S_AXI_ARVALID        ),
        .S_AXI_ARREADY                    (S_AXI_ARREADY        ),
        .S_AXI_RID                        (S_AXI_RID            ),
        .S_AXI_RDATA                      (S_AXI_RDATA          ),
        .S_AXI_RRESP                      (S_AXI_RRESP          ),
        .S_AXI_RLAST                      (S_AXI_RLAST          ),
        .S_AXI_RUSER                      (S_AXI_RUSER          ),
        .S_AXI_RVALID                     (S_AXI_RVALID         ),
        .S_AXI_RREADY                     (S_AXI_RREADY         ),
        .M_AXI_AWID                       (M_AXI_AWID           ),
        .M_AXI_AWADDR                     (M_AXI_AWADDR         ),
        .M_AXI_AWLEN                      (M_AXI_AWLEN          ),
        .M_AXI_AWSIZE                     (M_AXI_AWSIZE         ),
        .M_AXI_AWBURST                    (M_AXI_AWBURST        ),
        .M_AXI_AWLOCK                     (M_AXI_AWLOCK         ),
        .M_AXI_AWCACHE                    (M_AXI_AWCACHE        ),
        .M_AXI_AWPROT                     (M_AXI_AWPROT         ),
        .M_AXI_AWREGION                   (M_AXI_AWREGION       ),
        .M_AXI_AWQOS                      (M_AXI_AWQOS          ),
        .M_AXI_AWUSER                     (M_AXI_AWUSER         ),
        .M_AXI_AWVALID                    (M_AXI_AWVALID        ),
        .M_AXI_AWREADY                    (M_AXI_AWREADY        ),
        .M_AXI_WDATA                      (M_AXI_WDATA          ),
        .M_AXI_WSTRB                      (M_AXI_WSTRB          ),
        .M_AXI_WLAST                      (M_AXI_WLAST          ),
        .M_AXI_WUSER                      (M_AXI_WUSER          ),
        .M_AXI_WVALID                     (M_AXI_WVALID         ),
        .M_AXI_WREADY                     (M_AXI_WREADY         ),
        .M_AXI_BID                        (M_AXI_BID            ),
        .M_AXI_BRESP                      (M_AXI_BRESP          ),
        .M_AXI_BUSER                      (M_AXI_BUSER          ),
        .M_AXI_BVALID                     (M_AXI_BVALID         ),
        .M_AXI_BREADY                     (M_AXI_BREADY         ),
        .M_AXI_ARID                       (M_AXI_ARID           ),
        .M_AXI_ARADDR                     (M_AXI_ARADDR         ),
        .M_AXI_ARLEN                      (M_AXI_ARLEN          ),
        .M_AXI_ARSIZE                     (M_AXI_ARSIZE         ),
        .M_AXI_ARBURST                    (M_AXI_ARBURST        ),
        .M_AXI_ARLOCK                     (M_AXI_ARLOCK         ),
        .M_AXI_ARCACHE                    (M_AXI_ARCACHE        ),
        .M_AXI_ARPROT                     (M_AXI_ARPROT         ),
        .M_AXI_ARREGION                   (M_AXI_ARREGION       ),
        .M_AXI_ARQOS                      (M_AXI_ARQOS          ),
        .M_AXI_ARUSER                     (M_AXI_ARUSER         ),
        .M_AXI_ARVALID                    (M_AXI_ARVALID        ),
        .M_AXI_ARREADY                    (M_AXI_ARREADY        ),
        .M_AXI_RID                        (M_AXI_RID            ),
        .M_AXI_RDATA                      (M_AXI_RDATA          ),
        .M_AXI_RRESP                      (M_AXI_RRESP          ),
        .M_AXI_RLAST                      (M_AXI_RLAST          ),
        .M_AXI_RUSER                      (M_AXI_RUSER          ),
        .M_AXI_RVALID                     (M_AXI_RVALID         ),
        .M_AXI_RREADY                     (M_AXI_RREADY         ),
        .S_AXI_CTRL_AWADDR                (S_AXI_CTRL_AWADDR    ),
        .S_AXI_CTRL_AWVALID               (S_AXI_CTRL_AWVALID   ),
        .S_AXI_CTRL_AWREADY               (S_AXI_CTRL_AWREADY   ),
        .S_AXI_CTRL_WDATA                 (S_AXI_CTRL_WDATA     ),
        .S_AXI_CTRL_WVALID                (S_AXI_CTRL_WVALID    ),
        .S_AXI_CTRL_WREADY                (S_AXI_CTRL_WREADY    ),
        .S_AXI_CTRL_BRESP                 (S_AXI_CTRL_BRESP     ),
        .S_AXI_CTRL_BVALID                (S_AXI_CTRL_BVALID    ),
        .S_AXI_CTRL_BREADY                (S_AXI_CTRL_BREADY    ),
        .S_AXI_CTRL_ARADDR                (S_AXI_CTRL_ARADDR    ),
        .S_AXI_CTRL_ARVALID               (S_AXI_CTRL_ARVALID   ),
        .S_AXI_CTRL_ARREADY               (S_AXI_CTRL_ARREADY   ),
        .S_AXI_CTRL_RDATA                 (S_AXI_CTRL_RDATA     ),
        .S_AXI_CTRL_RRESP                 (S_AXI_CTRL_RRESP     ),
        .S_AXI_CTRL_RVALID                (S_AXI_CTRL_RVALID    ),
        .S_AXI_CTRL_RREADY                (S_AXI_CTRL_RREADY    ),
        .DEBUG_AW_TRANS_SEQ               (DEBUG_AW_TRANS_SEQ   ),
        .DEBUG_AW_ERROR                   (DEBUG_AW_ERROR       ),
        .DEBUG_AW_TARGET                  (DEBUG_AW_TARGET      ),
        .DEBUG_AW_ARB_GRANT               (DEBUG_AW_ARB_GRANT   ),
        .DEBUG_AR_TRANS_SEQ               (DEBUG_AR_TRANS_SEQ   ),
        .DEBUG_AR_ERROR                   (DEBUG_AR_ERROR       ),
        .DEBUG_AR_TARGET                  (DEBUG_AR_TARGET      ),
        .DEBUG_AR_ARB_GRANT               (DEBUG_AR_ARB_GRANT   ),
        .DEBUG_R_BEAT_CNT                 (DEBUG_R_BEAT_CNT     ),
        .DEBUG_W_BEAT_CNT                 (DEBUG_W_BEAT_CNT     )
      );
    assign DEBUG_AW_TRANS_QUAL     = 0;
    assign DEBUG_AW_ACCEPT_CNT     = 0;
    assign DEBUG_AW_ACTIVE_THREAD  = 0;
    assign DEBUG_AW_ACTIVE_TARGET  = 0;
    assign DEBUG_AW_ACTIVE_REGION  = 0;
    assign DEBUG_AW_ISSUING_CNT    = 0;
    assign DEBUG_AR_TRANS_QUAL     = 0;
    assign DEBUG_AR_ACCEPT_CNT     = 0;
    assign DEBUG_AR_ACTIVE_THREAD  = 0;
    assign DEBUG_AR_ACTIVE_TARGET  = 0;
    assign DEBUG_AR_ACTIVE_REGION  = 0;
    assign DEBUG_AR_ISSUING_CNT    = 0;
    assign DEBUG_B_TRANS_SEQ       = 0;
    assign DEBUG_R_TRANS_SEQ       = 0;
    assign DEBUG_W_TRANS_SEQ       = 0;
    assign DEBUG_RID_TARGET        = 0;
    assign DEBUG_RID_ERROR         = 0;
    assign DEBUG_BID_TARGET        = 0;
    assign DEBUG_BID_ERROR         = 0;
  end
endgenerate

endmodule

`default_nettype wire

