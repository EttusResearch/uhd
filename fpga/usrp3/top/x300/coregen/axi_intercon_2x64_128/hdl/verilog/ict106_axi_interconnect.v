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
// File name: axi_interconnect.v
//
// Description: 
//   This is the top-level module of a NxM AXI Interconnect core.
//   The interface of this module consists of vectored slave and master interfaces,
//     each of which are the concatenation of all connected AXI master and slave signals,
//     plus various vectored parameters concatenated from parameters propagated
//     from connected master and slave IP.
//   The interface of this version also contains various vectored DEBUG ports.
//   This module instantiates the major vectored sub-modules that comprise the
//     AXI Interconnect core.
//   Supports AXI4, AXI3 and AXI4-Lite protocols.
//
//-----------------------------------------------------------------------------
//
// Structure:
//    axi_interconnect
//      register_slice_bank
//        axi_register_slice
//      protocol_conv_bank
//        axilite_conv
//        axi3_conv
//          a_axi3_conv
//            axic_fifo
//          w_axi3_conv
//          b_downsizer
//          r_axi3_conv
//      converter_bank
//        axi_upsizer
//        clock_conv
//          fifo_gen
//          clock_sync_accel
//          clock_sync_decel
//        axi_downsizer
//      data_fifo_bank
//        axi_data_fifo
//      crossbar
//        si_transactor
//          addr_decoder
//            comparator_static
//          mux_enc
//          axic_srl_fifo
//          arbiter_resp
//        splitter
//        wdata_router
//          axic_reg_srl_fifo
//        wdata_mux
//          axic_reg_srl_fifo
//          mux_enc
//        addr_decoder
//          comparator_static
//        axic_srl_fifo
//        axi_register_slice
//        addr_arbiter
//          mux_enc
//        decerr_slave
//      crossbar_sasd
//        addr_arbiter_sasd
//          mux_enc
//        addr_decoder
//          comparator_static
//        splitter
//        mux_enc
//        axic_register_slice
//        decerr_slave
//      
//-----------------------------------------------------------------------------
`timescale 1ps/1ps
`default_nettype none

`define P_MAX_S 16
`define P_MAX_M 16
`define P_NUM_ADDR_RANGES 16

module ict106_axi_interconnect #
  (
   parameter         C_BASEFAMILY                         = "rtl", 
                       // FPGA Base Family. Current version: virtex6 or spartan6.
   parameter integer C_NUM_SLAVE_SLOTS                = 1, 
                       // Number of Slave Interface (SI) slots for connecting 
                       // to master IP. Range: 1-`P_MAX_S.
   parameter integer C_NUM_MASTER_SLOTS               = 1, 
                       // Number of Master Interface (MI) slots for connecting 
                       // to slave IP. Range: 1-`P_MAX_M.
   parameter integer C_AXI_ID_WIDTH                   = 1, 
                       // Width of ID signals propagated by the Interconnect.
                       // Width of ID signals produced on all MI slots.
                       // Range: 1-16.
   parameter integer C_AXI_ADDR_WIDTH                 = 32, 
                       // Width of S_AXI_AWADDR, S_AXI_ARADDR, M_AXI_AWADDR and 
                       // M_AXI_ARADDR for all SI/MI slots.
                       // Range: 32.
   parameter integer C_AXI_DATA_MAX_WIDTH             = 32, 
                       // Largest value specified for any DATA_WIDTH (including C_INTERCONNECT_DATA_WIDTH).
                       // Determines the stride of all DATA signals.
                       // Range: 32, 64, 128, 256, 512, 1024.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_DATA_WIDTH               = {`P_MAX_S{32'h00000020}}, 
                       // Width of S_AXI_WDATA and S_AXI_RDATA for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}; 
                       // Range: 'h00000020, 'h00000040, 'h00000080, 'h00000100.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_DATA_WIDTH               = {`P_MAX_M{32'h00000020}}, 
                       // Width of M_AXI_WDATA and M_AXI_RDATA for each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 'h00000020, 'h00000040, 'h00000080, 'h00000100.
   parameter integer C_INTERCONNECT_DATA_WIDTH        = 32, 
                       // Data width of the internal interconnect write and read 
                       // data paths.
                       // Range: 32, 64, 128, 256, 512, 1024.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_PROTOCOL                 = {`P_MAX_S{32'h00000000}}, 
                       // Indicates whether connected master is 
                       // Full-AXI4 ('h00000000),
                       // AXI3 ('h00000001) or 
                       // Axi4Lite ('h00000002), for each SI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_PROTOCOL                 = {`P_MAX_M{32'h00000000}}, 
                       // Indicates whether connected slave is
                       // Full-AXI4 ('h00000000),
                       // AXI3 ('h00000001) or 
                       // Axi4Lite ('h00000002), for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [`P_MAX_M*`P_NUM_ADDR_RANGES*64-1:0] C_M_AXI_BASE_ADDR                 = {`P_MAX_M*`P_NUM_ADDR_RANGES{64'hFFFFFFFF_FFFFFFFF}}, 
                       // Base address of each range of each MI slot. 
                       // For unused ranges, set base address to 'hFFFFFFFF.
                       // Format: C_NUM_MASTER_SLOTS{`P_NUM_ADDR_RANGES{Bit64}}.
   parameter [`P_MAX_M*`P_NUM_ADDR_RANGES*64-1:0] C_M_AXI_HIGH_ADDR                 = {`P_MAX_M*`P_NUM_ADDR_RANGES{64'h00000000_00000000}}, 
                       // High address of each range of each MI slot. 
                       // For unused ranges, set high address to 'h00000000.
                       // Format: C_NUM_MASTER_SLOTS{`P_NUM_ADDR_RANGES{Bit64}}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_BASE_ID                  = {`P_MAX_S{32'h00000000}},
                       // Base ID of each SI slot. 
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 0 to 2**C_AXI_ID_WIDTH-1.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_THREAD_ID_WIDTH                  = {`P_MAX_S{32'h00000000}},
                       // Width of variable ID signals received from master connected 
                       //   to each SI slot. 
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 0 to C_AXI_ID_WIDTH.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_IS_INTERCONNECT          = {`P_MAX_S{1'b0}}, 
                       // Indicates whether connected master is an end-point
                       // master (0) or an interconnect (1), for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_ACLK_RATIO               = {`P_MAX_S{32'h00000001}}, 
                       // Clock frequency ratio of each SI slot w.r.t. internal 
                       // interconnect. (Slowest clock input should have ratio=1.)
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}; Range: >='h00000001.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_IS_ACLK_ASYNC            = {`P_MAX_S{1'b0}}, 
                       // Indicates whether the clock for each SI slot is asynchronous
                       // to the interconnect native clock.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_ACLK_RATIO               = {`P_MAX_M{32'h00000001}}, 
                       // Clock frequency ratio of each MI slot w.r.t. internal 
                       // interconnect. (Slowest clock input should have ratio=1.)
                       // Format: C_NUM_MASTER_SLOTS{Bit32}; Range: >='h00000001.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_IS_ACLK_ASYNC            = {`P_MAX_M{1'b0}}, 
                       // Indicates whether the clock for each MI slot is asynchronous
                       // to the interconnect native clock.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter integer C_INTERCONNECT_ACLK_RATIO        = 1,
                       // Clock frequency ratio of the internal interconnect w.r.t. 
                       // all SI and MI slots. (Slowest clock input should have 
                       // ratio=1.) Range: >= 1.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_SUPPORTS_WRITE           = {`P_MAX_S{1'b1}}, 
                       // Indicates whether each SI supports write transactions.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_SUPPORTS_READ            = {`P_MAX_S{1'b1}}, 
                       // Indicates whether each SI supports read transactions.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_SUPPORTS_WRITE           = {`P_MAX_M{1'b1}}, 
                       // Indicates whether each MI supports write transactions.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_SUPPORTS_READ            = {`P_MAX_M{1'b1}}, 
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
   parameter [`P_MAX_M*32-1:0] C_AXI_CONNECTIVITY               = {`P_MAX_M{32'hFFFFFFFF}},
                       // Multi-pathway connectivity from each SI slot (N) to each 
                       // MI slot (M):
                       // 0 = no pathway required; 1 = pathway required.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}; 
                       // Range: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_SINGLE_THREAD                 = {`P_MAX_S{1'b0}}, 
                       // 0 = Implement separate command queues per ID thread.
                       // 1 = Force corresponding SI slot to be single-threaded.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}; 
   parameter [`P_MAX_M*1-1:0] C_M_AXI_SUPPORTS_REORDERING      = {`P_MAX_M{1'b1}},
                       // Indicates whether the slave connected to each MI slot 
                       // supports response reordering.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}; 
   parameter [`P_MAX_S*1-1:0] C_S_AXI_SUPPORTS_NARROW_BURST       = {`P_MAX_S{1'b1}},
                       // This parameter is ignored by the hdl.
                       // Indicates whether the master connected to each SI slot 
                       // is assumed to generate narrow bursts.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}; 
   parameter [`P_MAX_M*1-1:0] C_M_AXI_SUPPORTS_NARROW_BURST       = {`P_MAX_M{1'b1}},
                       // This parameter is ignored by the hdl.
                       // Indicates whether the slave connected to each MI slot 
                       // can handle narrow-burst transactions.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}; 
   parameter [`P_MAX_S*32-1:0] C_S_AXI_WRITE_ACCEPTANCE         = {`P_MAX_S{32'h00000001}},
                       // Maximum number of active write transactions that each SI 
                       // slot can accept.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}; 
                       // Range: 2**0 - 2**5.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_READ_ACCEPTANCE          = {`P_MAX_S{32'h00000001}},
                       // Maximum number of active read transactions that each SI 
                       // slot can accept.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 2**0 - 2**5.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_WRITE_ISSUING            = {`P_MAX_M{32'h00000001}},
                       // Maximum number of data-active write transactions that 
                       // each MI slot can generate at any one time.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 2**0 - 2**5.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_READ_ISSUING            = {`P_MAX_M{32'h00000001}},
                       // Maximum number of active read transactions that 
                       // each MI slot can generate at any one time.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 2**0 - 2**5.
//   parameter         C_AXI_ARB_METHOD = "priority", // Reserved for future
//                       // Arbitration method.
//                       // Format: String; 
//                       // Range: "priority" ("tdm" not yet implemented).
   parameter [`P_MAX_S*32-1:0] C_S_AXI_ARB_PRIORITY             = {`P_MAX_S{32'h00000000}},
                       // Arbitration priority among each SI slot. 
                       // Higher values indicate higher priority.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 'h0-'hF.
//   parameter [`P_MAX_S*32-1:0] C_S_AXI_ARB_TDM_SLOTS            = {`P_MAX_S{32'h00000000}}, // Reserved for future
//                       // Maximum number of consecutive TDM arbitration slots 
//                       // allocated among each SI slot.
//                       // Format: C_NUM_SLAVE_SLOTS{Bit32});
//   parameter integer C_AXI_ARB_TDM_TOTAL            = 0, // Reserved for future
//                       // Total number of TDM arbitration slots during which all 
//                       // TDM masters must be serviced.
//                       // (Must be >= sum of all C_S_AXI_ARB_TDM_SLOTS.)
   parameter [`P_MAX_M*1-1:0] C_M_AXI_SECURE                   = {`P_MAX_M{1'b0}},
                       // Indicates whether each MI slot connects to a secure slave 
                       // (allows only TrustZone secure access).
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_WRITE_FIFO_DEPTH         = {`P_MAX_S{32'h00000000}},
                       // Depth of SI-side write data FIFO for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}; 
                       // Range: 'h00000000, 'h00000020, 'h00000200.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_WRITE_FIFO_TYPE          = {`P_MAX_S{1'b1}},
                       // Type of SI-side write data FIFO for each SI slot.
                       // 0 = LUT flop/RAM only, 1 = BRAM allowed.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_WRITE_FIFO_DELAY         = {`P_MAX_S{1'b0}},
                       // Indicates whether AWVALID assertion is delayed until the 
                       // last burst data beat is received by the SI-side 
                       // write data FIFO, for each SI slot. 
                       // 0 means VALID is asserted whenever FIFO not empty.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_READ_FIFO_DEPTH          = {`P_MAX_S{32'h00000000}},
                       // Depth of SI-side read data FIFO for each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32};
                       // Range: 'h00000000, 'h00000020, 'h00000200.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_READ_FIFO_TYPE           = {`P_MAX_S{1'b1}},
                       // Type of SI-side read data FIFO for each SI slot.
                       // 0 = LUT flop/RAM only, 1 = BRAM allowed.
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_S*1-1:0] C_S_AXI_READ_FIFO_DELAY          = {`P_MAX_S{1'b0}},
                       // Indicates whether ARVALID assertion is delayed until the SI-side 
                       // read data FIFO has vacancy for whole burst, for each SI slot. 
                       // Format: C_NUM_SLAVE_SLOTS{Bit1}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_WRITE_FIFO_DEPTH         = {`P_MAX_M{32'h00000000}},
                       // Depth of MI-side write data FIFO for each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 'h00000000, 'h00000020, 'h00000200.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_WRITE_FIFO_TYPE          = {`P_MAX_M{1'b1}},
                       // Type of MI-side write data FIFO for each MI slot.
                       // 0 = LUT flop/RAM only, 1 = BRAM allowed.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_WRITE_FIFO_DELAY         = {`P_MAX_M{1'b0}},
                       // Indicates whether AWVALID assertion is delayed until the 
                       // last burst data beat is received by the MI-side 
                       // write data FIFO, for each MI slot. 
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_READ_FIFO_DEPTH          = {`P_MAX_M{32'h00000000}},
                       // Depth of MI-side read data FIFO for each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32};
                       // Range: 'h00000000, 'h00000020, 'h00000200.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_READ_FIFO_TYPE           = {`P_MAX_M{1'b1}},
                       // Type of MI-side read data FIFO for each MI slot.
                       //      = LUT flop/RAM only, 1 = BRAM allowed.
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [`P_MAX_M*1-1:0] C_M_AXI_READ_FIFO_DELAY          = {`P_MAX_M{1'b0}},
                       // Indicates whether ARVALID assertion is delayed until the MI-side 
                       // read data FIFO has vacancy for whole burst, for each MI slot. 
                       // Format: C_NUM_MASTER_SLOTS{Bit1}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_AW_REGISTER              = {`P_MAX_S{32'h00000000}},
                       // Insert register slice on AW channel at each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_AR_REGISTER              = {`P_MAX_S{32'h00000000}},
                       // Insert register slice on AR channel at each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_W_REGISTER               = {`P_MAX_S{32'h00000000}},
                       // Insert register slice on W channel at each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_R_REGISTER               = {`P_MAX_S{32'h00000000}},
                       // Insert register slice on R channel at each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [`P_MAX_S*32-1:0] C_S_AXI_B_REGISTER               = {`P_MAX_S{32'h00000000}},
                       // Insert register slice on B channel at each SI slot.
                       // Format: C_NUM_SLAVE_SLOTS{Bit32}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_AW_REGISTER              = {`P_MAX_M{32'h00000000}},
                       // Insert register slice on AW channel at each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_AR_REGISTER              = {`P_MAX_M{32'h00000000}},
                       // Insert register slice on AR channel at each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_W_REGISTER               = {`P_MAX_M{32'h00000000}},
                       // Insert register slice on W channel at each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_R_REGISTER               = {`P_MAX_M{32'h00000000}},
                       // Insert register slice on R channel at each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter [`P_MAX_M*32-1:0] C_M_AXI_B_REGISTER               = {`P_MAX_M{32'h00000000}},
                       // Insert register slice on B channel at each MI slot.
                       // Format: C_NUM_MASTER_SLOTS{Bit32}.
   parameter integer C_INTERCONNECT_R_REGISTER               = 0,
                       // Insert register slice on R channel in the crossbar.
                       // Range: 0-8.
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
   parameter integer C_S_AXI_CTRL_ADDR_WIDTH          = 32,
                       // ADDR width of CTRL interface.
   parameter integer C_S_AXI_CTRL_DATA_WIDTH          = 32,
                       // DATA width of CTRL interface.
   parameter integer C_INTERCONNECT_CONNECTIVITY_MODE = 1,
                       // 0 = Shared-Address Shared-Data (SASD).
                       // 1 = Shared-Address Multi-Data (SAMD).
   parameter integer C_DEBUG          = 1,
                       // Generate internal debug transaction sequence counters and data beat counters
                       // Default = 1 for testbench simulation
                       // Default = 0 for MPD
   parameter integer C_S_AXI_DEBUG_SLOT   = 0,
                       // SI slot slice of all SI-related DEBUG signals to connect to DEBUG output ports.
                       // Range: [0 : (C_NUM_SLAVE_SLOTS-1)] 
   parameter integer C_M_AXI_DEBUG_SLOT   = 0,
                       // MI slot slice of all MI-related DEBUG signals to connect to DEBUG output ports.
                       // Range: [0 : (C_NUM_MASTER_SLOTS-1)] 
   parameter integer C_MAX_DEBUG_THREADS  = 1
                       // Number of sets of ID-thread-specific DEBUG signals to monitor from each SI-slot (SAMD crossbar only)
                       // Range: [1 : (2**max(C_S_AXI_THREAD_ID_WIDTH))] 
   )
  (
   // Global Signals
   input  wire                                                  INTERCONNECT_ACLK,
   (* KEEP = "TRUE" *) input  wire                              INTERCONNECT_ARESETN /* synthesis syn_keep = 1 */,
   output wire                                                  IRQ,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_ARESET_OUT_N,  // Non-AXI resynchronized reset output
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_ARESET_OUT_N,  // Non-AXI resynchronized reset output
   output wire                                                  INTERCONNECT_ARESET_OUT_N,  // Non-AXI resynchronized reset for DEBUG
   // Slave Interface System Signals
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_ACLK,
   // Slave Interface Write Address Ports
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]           S_AXI_AWID,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]         S_AXI_AWADDR,
   input  wire [C_NUM_SLAVE_SLOTS*8-1:0]                        S_AXI_AWLEN,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                        S_AXI_AWSIZE,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                        S_AXI_AWBURST,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                        S_AXI_AWLOCK,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                        S_AXI_AWCACHE,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                        S_AXI_AWPROT,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                        S_AXI_AWQOS,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_AWUSER_WIDTH-1:0]       S_AXI_AWUSER,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_AWVALID,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_AWREADY,
   // Slave Interface Write Data Ports
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]           S_AXI_WID,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     S_AXI_WDATA,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]   S_AXI_WSTRB,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_WLAST,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_WUSER_WIDTH-1:0]        S_AXI_WUSER,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_WVALID,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_WREADY,
   // Slave Interface Write Response Ports
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]           S_AXI_BID,
   output wire [C_NUM_SLAVE_SLOTS*2-1:0]                        S_AXI_BRESP,
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_BUSER_WIDTH-1:0]        S_AXI_BUSER,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_BVALID,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_BREADY,
   // Slave Interface Read Address Ports
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]           S_AXI_ARID,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]         S_AXI_ARADDR,
   input  wire [C_NUM_SLAVE_SLOTS*8-1:0]                        S_AXI_ARLEN,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                        S_AXI_ARSIZE,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                        S_AXI_ARBURST,
   input  wire [C_NUM_SLAVE_SLOTS*2-1:0]                        S_AXI_ARLOCK,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                        S_AXI_ARCACHE,
   input  wire [C_NUM_SLAVE_SLOTS*3-1:0]                        S_AXI_ARPROT,
   input  wire [C_NUM_SLAVE_SLOTS*4-1:0]                        S_AXI_ARQOS,
   input  wire [C_NUM_SLAVE_SLOTS*C_AXI_ARUSER_WIDTH-1:0]       S_AXI_ARUSER,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_ARVALID,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_ARREADY,
   // Slave Interface Read Data Ports
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]           S_AXI_RID,
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]     S_AXI_RDATA,
   output wire [C_NUM_SLAVE_SLOTS*2-1:0]                        S_AXI_RRESP,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_RLAST,
   output wire [C_NUM_SLAVE_SLOTS*C_AXI_RUSER_WIDTH-1:0]        S_AXI_RUSER,
   output wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_RVALID,
   input  wire [C_NUM_SLAVE_SLOTS-1:0]                          S_AXI_RREADY,
   // Master Interface System Signals
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_ACLK,
   // Master Interface Write Address Port
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]          M_AXI_AWID,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]        M_AXI_AWADDR,
   output wire [C_NUM_MASTER_SLOTS*8-1:0]                       M_AXI_AWLEN,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                       M_AXI_AWSIZE,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                       M_AXI_AWBURST,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                       M_AXI_AWLOCK,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                       M_AXI_AWCACHE,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                       M_AXI_AWPROT,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                       M_AXI_AWREGION,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                       M_AXI_AWQOS,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]      M_AXI_AWUSER,
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_AWVALID,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_AWREADY,
   // Master Interface Write Data Ports
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]          M_AXI_WID,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    M_AXI_WDATA,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]  M_AXI_WSTRB,
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_WLAST,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]       M_AXI_WUSER,
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_WVALID,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_WREADY,
   // Master Interface Write Response Ports
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]          M_AXI_BID,
   input  wire [C_NUM_MASTER_SLOTS*2-1:0]                       M_AXI_BRESP,
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]       M_AXI_BUSER,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_BVALID,
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_BREADY,
   // Master Interface Read Address Port
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]          M_AXI_ARID,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]        M_AXI_ARADDR,
   output wire [C_NUM_MASTER_SLOTS*8-1:0]                       M_AXI_ARLEN,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                       M_AXI_ARSIZE,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                       M_AXI_ARBURST,
   output wire [C_NUM_MASTER_SLOTS*2-1:0]                       M_AXI_ARLOCK,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                       M_AXI_ARCACHE,
   output wire [C_NUM_MASTER_SLOTS*3-1:0]                       M_AXI_ARPROT,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                       M_AXI_ARREGION,
   output wire [C_NUM_MASTER_SLOTS*4-1:0]                       M_AXI_ARQOS,
   output wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]      M_AXI_ARUSER,
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_ARVALID,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_ARREADY,
   // Master Interface Read Data Ports
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]          M_AXI_RID,
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    M_AXI_RDATA,
   input  wire [C_NUM_MASTER_SLOTS*2-1:0]                       M_AXI_RRESP,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_RLAST,
   input  wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]       M_AXI_RUSER,
   input  wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_RVALID,
   output wire [C_NUM_MASTER_SLOTS-1:0]                         M_AXI_RREADY,
   // Diagnostic AxiLite Slave Interface
   input wire [(C_S_AXI_CTRL_ADDR_WIDTH-1):0]                   S_AXI_CTRL_AWADDR,
   input wire                                                   S_AXI_CTRL_AWVALID,
   output wire                                                  S_AXI_CTRL_AWREADY,
   input wire [(C_S_AXI_CTRL_DATA_WIDTH-1):0]                   S_AXI_CTRL_WDATA,
   input wire                                                   S_AXI_CTRL_WVALID,
   output wire                                                  S_AXI_CTRL_WREADY,
   output wire [1:0]                                            S_AXI_CTRL_BRESP,
   output wire                                                  S_AXI_CTRL_BVALID,
   input wire                                                   S_AXI_CTRL_BREADY,
   input wire [(C_S_AXI_CTRL_ADDR_WIDTH-1):0]                   S_AXI_CTRL_ARADDR,
   input wire                                                   S_AXI_CTRL_ARVALID,
   output wire                                                  S_AXI_CTRL_ARREADY,
   output wire [(C_S_AXI_CTRL_DATA_WIDTH-1):0]                  S_AXI_CTRL_RDATA,
   output wire [1:0]                                            S_AXI_CTRL_RRESP,
   output wire                                                  S_AXI_CTRL_RVALID,
   input wire                                                   S_AXI_CTRL_RREADY,
   
   // Debug ports monitoring Crossbar module internal diagnostic nets:
   // DEBUG_A*_ERROR codes:                    
                               // Bit 0 = Invalid target address        
                               // Bit 1 = TrustZone violation           
                               // Bit 2 = AxiLite access violation      
                               // Bit 3 = R/W direction unsupported by target (SASD only)
                               // Bit 4-7 = Reserved
   // Global debug ports (independent of selected SI/MI slot):
   output wire [8-1:0]                       DEBUG_AW_TRANS_SEQ,      // Write transaction sequence #, increments each M_AW transfer (modulo 256)      
   output wire [8-1:0]                       DEBUG_AW_ARB_GRANT,      // SI currently granted for write (Bit8)                                               
   output wire [8-1:0]                       DEBUG_AR_TRANS_SEQ,      // Read transaction sequence #, increments each M_AR transfer (modulo 256) 
   output wire [8-1:0]                       DEBUG_AR_ARB_GRANT,      // SI currently granted for read  (Bit8)                                          

   // Debug ports selected by C_S_AXI_DEBUG_SLOT:
   output wire [C_MAX_DEBUG_THREADS-1:0]     DEBUG_AW_TRANS_QUAL,     // ID thread is qualified for write arbitration (1-hot)                          
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_AW_ACCEPT_CNT,     // Write accetance counter, per ID thread (Bit8)                                 
   output wire [16-1:0]                      DEBUG_AW_ACTIVE_THREAD,  // Last issued AWID value, when SINGLE_THREADed (Bit16)                           
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_AW_ACTIVE_TARGET,  // Last MI to be issued a write, per ID thread (Bit8)                                  
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_AW_ACTIVE_REGION,  // Last M_REGION issued for write, per ID thread (Bit8)                                    
   output wire [8-1:0]                       DEBUG_AW_ERROR,          // Write transaction error code (Bit8)                                   
   output wire [8-1:0]                       DEBUG_AW_TARGET,         // MI currently targeted for write (Bit8)                               
   output wire [C_MAX_DEBUG_THREADS-1:0]     DEBUG_AR_TRANS_QUAL,     // ID thread is qualified for read arbitration (1-hot)                     
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_AR_ACCEPT_CNT,     // Read accetance counter, per ID thread (Bit8)                            
   output wire [16-1:0]                      DEBUG_AR_ACTIVE_THREAD,  // Last issued ARID value, when SINGLE_THREADed (Bit16)                      
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_AR_ACTIVE_TARGET,  // Last MI to be issued a read, per ID thread (Bit8)                             
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_AR_ACTIVE_REGION,  // Last M_REGION issued for read, per ID thread (Bit8)                               
   output wire [8-1:0]                       DEBUG_AR_ERROR,          // Read transaction error code (Bit8)                              
   output wire [8-1:0]                       DEBUG_AR_TARGET,         // MI currently targeted for read (Bit8)                          
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_B_TRANS_SEQ,       // Write transaction sequence # of last B-channel transfer, per ID thread (Bit8)
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_R_BEAT_CNT,        // Beat within burst of last R-channel transfer, per ID thread (Bit8)              
   output wire [C_MAX_DEBUG_THREADS*8-1:0]   DEBUG_R_TRANS_SEQ,       // Read transaction sequence # of last R-channel transfer, per ID thread (Bit8)    

   // Debug ports selected by C_M_AXI_DEBUG_SLOT:
   output wire [8-1:0]                       DEBUG_AW_ISSUING_CNT,    // Write issuing counter (Bit8)                                          
   output wire [8-1:0]                       DEBUG_AR_ISSUING_CNT,    // Read issuing counter (Bit8)                                     
   output wire [8-1:0]                       DEBUG_W_BEAT_CNT,        // Beat within burst of last W-channel transfer (Bit8)                      
   output wire [8-1:0]                       DEBUG_W_TRANS_SEQ,       // Write transaction sequence # of last W-channel transfer (Bit8)          
   output wire [8-1:0]                       DEBUG_BID_TARGET,        // SI targeted by current BID (Bit8)                                                                                              
   output wire                               DEBUG_BID_ERROR,         // Invalid SI targeted by current BID                                                                                     
   output wire [8-1:0]                       DEBUG_RID_TARGET,        // SI targeted by current RID (Bit8)                                        
   output wire                               DEBUG_RID_ERROR,         // Invalid SI targeted by current RID                               
   
  // DEBUG ports monitoring AXI signals connecting major functional blocks
  //   Format: DEBUG_<si_side_module>_<mi_side_module>_<debug_port_name>
  //   Module mnemonics:
  //   "SR" = SI-hemisphere Register-slice bank
  //   "SC" = SI-hemisphere Converter bank (upsizer -> clock_conv -> downsizer)
  //   "SF" = SI-hemisphere data-path FIFO bank
  //   "CB" = Crossbar module (crossbar or crossbar_sasd)
  //   "MF" = MI-hemisphere data-path FIFO bank
  //   "MC" = MI-hemisphere Converter bank (upsizer -> clock_conv -> downsizer)
  //   "MP" = MI-hemisphere Protocol converter bank (axi4lite conv or axi3 conv)
  //   "MR" = MI-hemisphere Register-slice bank
  
  //   Debug port contents:
  //
  //   ARADDRCONTROL[0:0]                         = arvalid
  //   ARADDRCONTROL[1:1]                         = arready
  //   ARADDRCONTROL[9:2]                         = arlen[7:0]
  //   ARADDRCONTROL[12:10]                       = arsize[2:0]
  //   ARADDRCONTROL[14:13]                       = arburst[1:0]
  //   ARADDRCONTROL[15:15]                       = arlock 
  //   ARADDRCONTROL[19:16]                       = arcache[3:0]
  //   ARADDRCONTROL[22:20]                       = arprot[2:0]
  //   ARADDRCONTROL[22+C_AXI_ID_WIDTH:23]        = arid[C_AXI_ID_WIDTH-1:0]
  //                                              
  //   AWADDRCONTROL[0:0]                         = awvalid
  //   AWADDRCONTROL[1:1]                         = awready
  //   AWADDRCONTROL[9:2]                         = awlen[7:0]
  //   AWADDRCONTROL[12:10]                       = awsize[2:0]
  //   AWADDRCONTROL[14:13]                       = awburst[1:0]
  //   AWADDRCONTROL[15:15]                       = awlock 
  //   AWADDRCONTROL[19:16]                       = awcache[3:0]
  //   AWADDRCONTROL[22:20]                       = awprot[2:0]
  //   AWADDRCONTROL[22+C_AXI_ID_WIDTH:23]        = awid[C_AXI_ID_WIDTH-1:0]
  //          
  //   BRESP[0:0]                                 = bvalid 
  //   BRESP[1:1]                                 = bready 
  //   BRESP[3:2]                                 = bresp[1:0]  
  //   BRESP[3+C_AXI_ID_WIDTH:4]                  = bid[C_AXI_ID_WIDTH-1:0]    
  //          
  //   RDATACONTROL[0:0]                          = rvalid 
  //   RDATACONTROL[1:1]                          = rready 
  //   RDATACONTROL[2:2]                          = rlast  
  //   RDATACONTROL[4:3]                          = rresp[1:0]  
  //   RDATACONTROL[4+C_AXI_ID_WIDTH:5]           = rid[C_AXI_ID_WIDTH-1:0]    
  //          
  //   WDATACONTROL[0:0]                          = wvalid 
  //   WDATACONTROL[1:1]                          = wready 
  //   WDATACONTROL[2:2]                          = wlast  
  //   WDATACONTROL[2+(C_AXI_DATA_MAX_WIDTH/8):3] = wstrb[(C_AXI_DATA_MAX_WIDTH/8)-1:0]  
  
  // SI-side DEBUG ports selected by C_S_AXI_DEBUG_SLOT:
       
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_SR_SC_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_SR_SC_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_SR_SC_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_SR_SC_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_SR_SC_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_SR_SC_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_SR_SC_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_SR_SC_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_SR_SC_WDATACONTROL  ,
       
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_SC_SF_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_SC_SF_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_SC_SF_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_SC_SF_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_SC_SF_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_SC_SF_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_SC_SF_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_SC_SF_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_SC_SF_WDATACONTROL  ,
       
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_SF_CB_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_SF_CB_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_SF_CB_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_SF_CB_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_SF_CB_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_SF_CB_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_SF_CB_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_SF_CB_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_SF_CB_WDATACONTROL  ,
   
  // MI-side DEBUG ports selected by C_M_AXI_DEBUG_SLOT:
  
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_CB_MF_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_CB_MF_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_CB_MF_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_CB_MF_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_CB_MF_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_CB_MF_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_CB_MF_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_CB_MF_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_CB_MF_WDATACONTROL  ,
  
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_MF_MC_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_MF_MC_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_MF_MC_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_MF_MC_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_MF_MC_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_MF_MC_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_MF_MC_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_MF_MC_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_MF_MC_WDATACONTROL  ,
  
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_MC_MP_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_MC_MP_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_MC_MP_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_MC_MP_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_MC_MP_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_MC_MP_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_MC_MP_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_MC_MP_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_MC_MP_WDATACONTROL  ,
  
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_MP_MR_ARADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_MP_MR_ARADDRCONTROL ,
   output wire [C_AXI_ADDR_WIDTH-1:0]                             DEBUG_MP_MR_AWADDR        ,
   output wire [(1+1+8+3+2+1+4+3+C_AXI_ID_WIDTH)-1:0]             DEBUG_MP_MR_AWADDRCONTROL ,
   output wire [(1+1+2+C_AXI_ID_WIDTH)-1:0]                       DEBUG_MP_MR_BRESP         ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_MP_MR_RDATA         ,
   output wire [(1+1+1+2+C_AXI_ID_WIDTH)-1:0]                     DEBUG_MP_MR_RDATACONTROL  ,
   output wire [C_AXI_DATA_MAX_WIDTH-1:0]                         DEBUG_MP_MR_WDATA         ,
   output wire [(1+1+1+(C_AXI_DATA_MAX_WIDTH/8))-1:0]             DEBUG_MP_MR_WDATACONTROL  
   );

  // Nets connecting major functional blocks
  //   Format: <si_side_module>_<mi_side_module>_<axi_net_name>
  //   Module mnemonics:
  //   "si" = Slave Interface of axi_interconnect
  //   "sr" = SI-hemisphere Register-slice bank
  //   "sc" = SI-hemisphere Converter bank (upsizer -> clock_conv -> downsizer)
  //   "sf" = SI-hemisphere data-path FIFO bank
  //   "cb" = Crossbar module (crossbar or crossbar_sasd)
  //   "mf" = MI-hemisphere data-path FIFO bank
  //   "mc" = MI-hemisphere Converter bank (upsizer -> clock_conv -> downsizer)
  //   "mp" = MI-hemisphere Protocol converter bank (axi4lite conv or axi3 conv)
  //   "mr" = MI-hemisphere Register-slice bank
  //   "mi" = Master Interface of axi_interconnect
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          si_sr_awid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        si_sr_awaddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       si_sr_awlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       si_sr_awsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       si_sr_awburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       si_sr_awlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       si_sr_awcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       si_sr_awprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       si_sr_awqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_AWUSER_WIDTH-1:0]      si_sr_awuser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_awvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_awready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    si_sr_wdata           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]  si_sr_wstrb           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_wlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_WUSER_WIDTH-1:0]       si_sr_wuser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_wvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_wready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          si_sr_bid             ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       si_sr_bresp           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_BUSER_WIDTH-1:0]       si_sr_buser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_bvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_bready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          si_sr_arid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        si_sr_araddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       si_sr_arlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       si_sr_arsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       si_sr_arburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       si_sr_arlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       si_sr_arcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       si_sr_arprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       si_sr_arqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ARUSER_WIDTH-1:0]      si_sr_aruser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_arvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_arready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          si_sr_rid             ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    si_sr_rdata           ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       si_sr_rresp           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_rlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_RUSER_WIDTH-1:0]       si_sr_ruser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_rvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         si_sr_rready          ;
                                             
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sr_sc_awid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        sr_sc_awaddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       sr_sc_awlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sr_sc_awsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sr_sc_awburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sr_sc_awlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sr_sc_awcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sr_sc_awprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sr_sc_awqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_AWUSER_WIDTH-1:0]      sr_sc_awuser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_awvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_awready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    sr_sc_wdata           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]  sr_sc_wstrb           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_wlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_WUSER_WIDTH-1:0]       sr_sc_wuser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_wvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_wready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sr_sc_bid             ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sr_sc_bresp           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_BUSER_WIDTH-1:0]       sr_sc_buser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_bvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_bready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sr_sc_arid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        sr_sc_araddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       sr_sc_arlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sr_sc_arsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sr_sc_arburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sr_sc_arlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sr_sc_arcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sr_sc_arprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sr_sc_arqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ARUSER_WIDTH-1:0]      sr_sc_aruser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_arvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_arready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sr_sc_rid             ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    sr_sc_rdata           ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sr_sc_rresp           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_rlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_RUSER_WIDTH-1:0]       sr_sc_ruser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_rvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sr_sc_rready          ;
                                             
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sc_sf_awid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        sc_sf_awaddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       sc_sf_awlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sc_sf_awsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sc_sf_awburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sc_sf_awlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sc_sf_awcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sc_sf_awprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sc_sf_awqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_AWUSER_WIDTH-1:0]      sc_sf_awuser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_awvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_awready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    sc_sf_wdata           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]  sc_sf_wstrb           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_wlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_WUSER_WIDTH-1:0]       sc_sf_wuser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_wvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_wready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sc_sf_bid             ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sc_sf_bresp           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_BUSER_WIDTH-1:0]       sc_sf_buser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_bvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_bready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sc_sf_arid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        sc_sf_araddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       sc_sf_arlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sc_sf_arsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sc_sf_arburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sc_sf_arlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sc_sf_arcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sc_sf_arprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sc_sf_arqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ARUSER_WIDTH-1:0]      sc_sf_aruser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_arvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_arready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sc_sf_rid             ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    sc_sf_rdata           ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sc_sf_rresp           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_rlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_RUSER_WIDTH-1:0]       sc_sf_ruser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_rvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sc_sf_rready          ;
                                             
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sf_cb_awid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        sf_cb_awaddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       sf_cb_awlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sf_cb_awsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sf_cb_awburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sf_cb_awlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sf_cb_awcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sf_cb_awprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sf_cb_awqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_AWUSER_WIDTH-1:0]      sf_cb_awuser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_awvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_awready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    sf_cb_wdata           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0]  sf_cb_wstrb           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_wlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_WUSER_WIDTH-1:0]       sf_cb_wuser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_wvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_wready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sf_cb_bid             ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sf_cb_bresp           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_BUSER_WIDTH-1:0]       sf_cb_buser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_bvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_bready          ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sf_cb_arid            ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ADDR_WIDTH-1:0]        sf_cb_araddr          ;
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                       sf_cb_arlen           ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sf_cb_arsize          ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sf_cb_arburst         ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sf_cb_arlock          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sf_cb_arcache         ;
  wire [C_NUM_SLAVE_SLOTS*3-1:0]                       sf_cb_arprot          ;
  wire [C_NUM_SLAVE_SLOTS*4-1:0]                       sf_cb_arqos           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ARUSER_WIDTH-1:0]      sf_cb_aruser          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_arvalid         ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_arready         ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH-1:0]          sf_cb_rid             ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]    sf_cb_rdata           ;
  wire [C_NUM_SLAVE_SLOTS*2-1:0]                       sf_cb_rresp           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_rlast           ;
  wire [C_NUM_SLAVE_SLOTS*C_AXI_RUSER_WIDTH-1:0]       sf_cb_ruser           ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_rvalid          ;
  wire [C_NUM_SLAVE_SLOTS-1:0]                         sf_cb_rready          ;
                                             
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         cb_mf_awid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       cb_mf_awaddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      cb_mf_awlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      cb_mf_awsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      cb_mf_awburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      cb_mf_awlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      cb_mf_awcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      cb_mf_awprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      cb_mf_awregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      cb_mf_awqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]     cb_mf_awuser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_awvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_awready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   cb_mf_wdata           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0] cb_mf_wstrb           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_wlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]      cb_mf_wuser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_wvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_wready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         cb_mf_bid             ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      cb_mf_bresp           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]      cb_mf_buser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_bvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_bready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         cb_mf_arid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       cb_mf_araddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      cb_mf_arlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      cb_mf_arsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      cb_mf_arburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      cb_mf_arlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      cb_mf_arcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      cb_mf_arprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      cb_mf_arregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      cb_mf_arqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]     cb_mf_aruser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_arvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_arready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         cb_mf_rid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   cb_mf_rdata           ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      cb_mf_rresp           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_rlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]      cb_mf_ruser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_rvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        cb_mf_rready          ;
                                             
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mf_mc_awid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mf_mc_awaddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mf_mc_awlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mf_mc_awsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mf_mc_awburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mf_mc_awlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mf_mc_awcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mf_mc_awprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mf_mc_awregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mf_mc_awqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]     mf_mc_awuser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_awvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_awready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mf_mc_wdata           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0] mf_mc_wstrb           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_wlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]      mf_mc_wuser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_wvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_wready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mf_mc_bid             ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mf_mc_bresp           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]      mf_mc_buser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_bvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_bready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mf_mc_arid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mf_mc_araddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mf_mc_arlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mf_mc_arsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mf_mc_arburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mf_mc_arlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mf_mc_arcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mf_mc_arprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mf_mc_arregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mf_mc_arqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]     mf_mc_aruser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_arvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_arready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mf_mc_rid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mf_mc_rdata           ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mf_mc_rresp           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_rlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]      mf_mc_ruser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_rvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mf_mc_rready          ;
                                             
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mc_mp_awid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mc_mp_awaddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mc_mp_awlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mc_mp_awsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mc_mp_awburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mc_mp_awlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mc_mp_awcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mc_mp_awprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mc_mp_awregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mc_mp_awqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]     mc_mp_awuser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_awvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_awready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mc_mp_wdata           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0] mc_mp_wstrb           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_wlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]      mc_mp_wuser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_wvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_wready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mc_mp_bid             ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mc_mp_bresp           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]      mc_mp_buser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_bvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_bready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mc_mp_arid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mc_mp_araddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mc_mp_arlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mc_mp_arsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mc_mp_arburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mc_mp_arlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mc_mp_arcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mc_mp_arprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mc_mp_arregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mc_mp_arqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]     mc_mp_aruser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_arvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_arready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mc_mp_rid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mc_mp_rdata           ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mc_mp_rresp           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_rlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]      mc_mp_ruser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_rvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mc_mp_rready          ;
                                                
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mp_mr_awid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mp_mr_awaddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mp_mr_awlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mp_mr_awsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mp_mr_awburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mp_mr_awlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mp_mr_awcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mp_mr_awprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mp_mr_awregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mp_mr_awqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]     mp_mr_awuser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_awvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_awready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mp_mr_wid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mp_mr_wdata           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0] mp_mr_wstrb           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_wlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]      mp_mr_wuser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_wvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_wready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mp_mr_bid             ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mp_mr_bresp           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]      mp_mr_buser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_bvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_bready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mp_mr_arid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mp_mr_araddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mp_mr_arlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mp_mr_arsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mp_mr_arburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mp_mr_arlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mp_mr_arcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mp_mr_arprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mp_mr_arregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mp_mr_arqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]     mp_mr_aruser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_arvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_arready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mp_mr_rid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mp_mr_rdata           ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mp_mr_rresp           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_rlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]      mp_mr_ruser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_rvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mp_mr_rready          ;
                                                
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mr_mi_awid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mr_mi_awaddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mr_mi_awlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mr_mi_awsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mr_mi_awburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mr_mi_awlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mr_mi_awcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mr_mi_awprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mr_mi_awregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mr_mi_awqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_AWUSER_WIDTH-1:0]     mr_mi_awuser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_awvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_awready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mr_mi_wid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mr_mi_wdata           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH/8-1:0] mr_mi_wstrb           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_wlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_WUSER_WIDTH-1:0]      mr_mi_wuser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_wvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_wready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mr_mi_bid             ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mr_mi_bresp           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_BUSER_WIDTH-1:0]      mr_mi_buser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_bvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_bready          ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mr_mi_arid            ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ADDR_WIDTH-1:0]       mr_mi_araddr          ;
  wire [C_NUM_MASTER_SLOTS*8-1:0]                      mr_mi_arlen           ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mr_mi_arsize          ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mr_mi_arburst         ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mr_mi_arlock          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mr_mi_arcache         ;
  wire [C_NUM_MASTER_SLOTS*3-1:0]                      mr_mi_arprot          ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mr_mi_arregion        ;
  wire [C_NUM_MASTER_SLOTS*4-1:0]                      mr_mi_arqos           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ARUSER_WIDTH-1:0]     mr_mi_aruser          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_arvalid         ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_arready         ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH-1:0]         mr_mi_rid             ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_DATA_MAX_WIDTH-1:0]   mr_mi_rdata           ;
  wire [C_NUM_MASTER_SLOTS*2-1:0]                      mr_mi_rresp           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_rlast           ;
  wire [C_NUM_MASTER_SLOTS*C_AXI_RUSER_WIDTH-1:0]      mr_mi_ruser           ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_rvalid          ;
  wire [C_NUM_MASTER_SLOTS-1:0]                        mr_mi_rready          ;
  
  wire                                                  interconnect_aresetn_i;
  wire [C_NUM_SLAVE_SLOTS-1:0]                          s_axi_reset_out_n_i;
  wire [C_NUM_MASTER_SLOTS-1:0]                         m_axi_reset_out_n_i;
                                                                    
  wire [C_S_AXI_CTRL_ADDR_WIDTH-1:0]    cb_ctrl_awaddr            ;
  wire                                  cb_ctrl_awvalid           ;
  wire                                  cb_ctrl_awready           ;
  wire [C_S_AXI_CTRL_DATA_WIDTH-1:0]    cb_ctrl_wdata             ;
  wire                                  cb_ctrl_wvalid            ;
  wire                                  cb_ctrl_wready            ;
  wire [2-1:0]                          cb_ctrl_bresp             ;
  wire                                  cb_ctrl_bvalid            ;
  wire                                  cb_ctrl_bready            ;
  wire [C_S_AXI_CTRL_ADDR_WIDTH-1:0]    cb_ctrl_araddr            ;
  wire                                  cb_ctrl_arvalid           ;
  wire                                  cb_ctrl_arready           ;
  wire [C_S_AXI_CTRL_DATA_WIDTH-1:0]    cb_ctrl_rdata             ;
  wire [2-1:0]                          cb_ctrl_rresp             ;
  wire                                  cb_ctrl_rvalid            ;
  wire                                  cb_ctrl_rready            ;

   // Diagnostic Probe Nets from Crossbar module
   // DEBUG_A*_ERROR codes:                    
                               // Bit 0 = Invalid target address        
                               // Bit 1 = TrustZone violation           
                               // Bit 2 = AxiLite access violation      
                               // Bit 3 = R/W direction unsupported by target (SASD only)
                               // Bit 4-7 = Reserved

  wire [8-1:0]                                            aw_trans_seq;      // Write transaction sequence #, increments each M_AW transfer (modulo 256)      
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS-1:0]        aw_trans_qual;     // ID thread is qualified for write arbitration (1-hot)                          
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      aw_accept_cnt;     // Write accetance counter, per ID thread (Bit8)                                 
  wire [C_NUM_SLAVE_SLOTS*16-1:0]                         aw_active_thread;  // Last issued AWID value, when SINGLE_THREADed (Bit16)                           
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      aw_active_target;  // Last MI to be issued a write, per ID thread (Bit8)                                  
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      aw_active_region;  // Last M_REGION issued for write, per ID thread (Bit8)                                    
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                          aw_error;          // Write transaction error code, per SI (Bit8)                                   
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                          aw_target;         // MI currently targeted for write, per SI (Bit8)                               
  wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     aw_issuing_cnt;    // Write issuing counter, per MI (Bit8)                                          
  wire [8-1:0]                                            aw_arb_grant;      // SI currently granted for write (Bit8)                                               
  wire [8-1:0]                                            ar_trans_seq;      // Read transaction sequence #, increments each M_AR transfer (modulo 256) 
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS-1:0]        ar_trans_qual;     // ID thread is qualified for read arbitration (1-hot)                     
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      ar_accept_cnt;     // Read accetance counter, per ID thread (Bit8)                            
  wire [C_NUM_SLAVE_SLOTS*16-1:0]                         ar_active_thread;  // Last issued ARID value, when SINGLE_THREADed (Bit16)                      
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      ar_active_target;  // Last MI to be issued a read, per ID thread (Bit8)                             
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      ar_active_region;  // Last M_REGION issued for read, per ID thread (Bit8)                               
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                          ar_error;          // Read transaction error code, per SI (Bit8)                              
  wire [C_NUM_SLAVE_SLOTS*8-1:0]                          ar_target;         // MI currently targeted for read, per SI (Bit8)                          
  wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     ar_issuing_cnt;    // Read issuing counter, per MI (Bit8)                                     
  wire [8-1:0]                                            ar_arb_grant;      // SI currently granted for read  (Bit8)                                          
  wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     w_beat_cnt;        // Beat within burst of last W-channel transfer, per MI (Bit8)                      
  wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     w_trans_seq;       // Write transaction sequence # of last W-channel transfer, per MI (Bit8)          
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      b_trans_seq;       // Write transaction sequence # of last B-channel transfer, per ID thread (Bit8)
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      r_beat_cnt;        // Beat within burst of last R-channel transfer, per ID thread (Bit8)              
  wire [C_NUM_SLAVE_SLOTS*C_MAX_DEBUG_THREADS*8-1:0]      r_trans_seq;       // Read transaction sequence # of last R-channel transfer, per ID thread (Bit8)    
  wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     bid_target;        // SI targeted by current BID, per MI (Bit8)                                                                                              
  wire [(C_NUM_MASTER_SLOTS+1)-1:0]                       bid_error;         // Invalid SI targeted by current BID, per MI (1-hot)                                                                                     
  wire [(C_NUM_MASTER_SLOTS+1)*8-1:0]                     rid_target;        // SI targeted by current RID, per MI (Bit8)                                        
  wire [(C_NUM_MASTER_SLOTS+1)-1:0]                       rid_error;         // Invalid SI targeted by current RID, per MI (1-hot)                               

  assign DEBUG_AW_TRANS_SEQ     = aw_trans_seq;
  assign DEBUG_AW_TRANS_QUAL    = aw_trans_qual[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS+:C_MAX_DEBUG_THREADS];
  assign DEBUG_AW_ACCEPT_CNT    = aw_accept_cnt[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_AW_ACTIVE_THREAD = aw_active_thread[C_S_AXI_DEBUG_SLOT*16+:16];
  assign DEBUG_AW_ACTIVE_TARGET = aw_active_target[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_AW_ACTIVE_REGION = aw_active_region[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_AW_ERROR         = aw_error[C_S_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_AW_TARGET        = aw_target[C_S_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_AW_ISSUING_CNT   = aw_issuing_cnt[C_M_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_AW_ARB_GRANT     = aw_arb_grant;
  assign DEBUG_AR_TRANS_SEQ     = ar_trans_seq;
  assign DEBUG_AR_TRANS_QUAL    = ar_trans_qual[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS+:C_MAX_DEBUG_THREADS];
  assign DEBUG_AR_ACCEPT_CNT    = ar_accept_cnt[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_AR_ACTIVE_THREAD = ar_active_thread[C_S_AXI_DEBUG_SLOT*16+:16];
  assign DEBUG_AR_ACTIVE_TARGET = ar_active_target[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_AR_ACTIVE_REGION = ar_active_region[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_AR_ERROR         = ar_error[C_S_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_AR_TARGET        = ar_target[C_S_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_AR_ISSUING_CNT   = ar_issuing_cnt[C_M_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_AR_ARB_GRANT     = ar_arb_grant;
  assign DEBUG_W_BEAT_CNT       = w_beat_cnt[C_M_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_W_TRANS_SEQ      = w_trans_seq[C_M_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_B_TRANS_SEQ      = b_trans_seq[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_R_BEAT_CNT       = r_beat_cnt[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_R_TRANS_SEQ      = r_trans_seq[C_S_AXI_DEBUG_SLOT*C_MAX_DEBUG_THREADS*8+:C_MAX_DEBUG_THREADS*8];
  assign DEBUG_BID_TARGET       = bid_target[C_M_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_BID_ERROR        = bid_error[C_M_AXI_DEBUG_SLOT];
  assign DEBUG_RID_TARGET       = rid_target[C_M_AXI_DEBUG_SLOT*8+:8];
  assign DEBUG_RID_ERROR        = rid_error[C_M_AXI_DEBUG_SLOT];

  assign DEBUG_SR_SC_ARADDR        =  sr_sc_araddr[(C_S_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_SR_SC_ARADDRCONTROL = {
                                      sr_sc_arid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sr_sc_arprot[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sr_sc_arcache[(C_S_AXI_DEBUG_SLOT*4)+:4],
                                      sr_sc_arlock[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_arburst[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sr_sc_arsize[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sr_sc_arlen[(C_S_AXI_DEBUG_SLOT*8)+:8],
                                      sr_sc_arready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_arvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SR_SC_AWADDR        =  sr_sc_awaddr[(C_S_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_SR_SC_AWADDRCONTROL = {
                                      sr_sc_awid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sr_sc_awprot[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sr_sc_awcache[(C_S_AXI_DEBUG_SLOT*4)+:4],
                                      sr_sc_awlock[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_awburst[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sr_sc_awsize[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sr_sc_awlen[(C_S_AXI_DEBUG_SLOT*8)+:8],
                                      sr_sc_awready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_awvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SR_SC_BRESP         = {
                                      sr_sc_bid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sr_sc_bresp[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sr_sc_bready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_bvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SR_SC_RDATA         =  sr_sc_rdata[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_SR_SC_RDATACONTROL  = {
                                      sr_sc_rid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sr_sc_rresp[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sr_sc_rlast[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_rready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_rvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SR_SC_WDATA         =  sr_sc_wdata[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_SR_SC_WDATACONTROL  = {
                                      sr_sc_wstrb[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      sr_sc_wlast[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_wready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sr_sc_wvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign DEBUG_SC_SF_ARADDR        =  sc_sf_araddr[(C_S_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_SC_SF_ARADDRCONTROL = {
                                      sc_sf_arid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sc_sf_arprot[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sc_sf_arcache[(C_S_AXI_DEBUG_SLOT*4)+:4],
                                      sc_sf_arlock[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_arburst[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sc_sf_arsize[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sc_sf_arlen[(C_S_AXI_DEBUG_SLOT*8)+:8],
                                      sc_sf_arready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_arvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SC_SF_AWADDR        =  sc_sf_awaddr[(C_S_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_SC_SF_AWADDRCONTROL = {
                                      sc_sf_awid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sc_sf_awprot[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sc_sf_awcache[(C_S_AXI_DEBUG_SLOT*4)+:4],
                                      sc_sf_awlock[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_awburst[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sc_sf_awsize[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sc_sf_awlen[(C_S_AXI_DEBUG_SLOT*8)+:8],
                                      sc_sf_awready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_awvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SC_SF_BRESP         = {
                                      sc_sf_bid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sc_sf_bresp[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sc_sf_bready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_bvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SC_SF_RDATA         =  sc_sf_rdata[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_SC_SF_RDATACONTROL  = {
                                      sc_sf_rid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sc_sf_rresp[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sc_sf_rlast[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_rready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_rvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SC_SF_WDATA         =  sc_sf_wdata[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_SC_SF_WDATACONTROL  = {
                                      sc_sf_wstrb[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      sc_sf_wlast[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_wready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sc_sf_wvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign DEBUG_SF_CB_ARADDR        =  sf_cb_araddr[(C_S_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_SF_CB_ARADDRCONTROL = {
                                      sf_cb_arid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sf_cb_arprot[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sf_cb_arcache[(C_S_AXI_DEBUG_SLOT*4)+:4],
                                      sf_cb_arlock[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_arburst[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sf_cb_arsize[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sf_cb_arlen[(C_S_AXI_DEBUG_SLOT*8)+:8],
                                      sf_cb_arready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_arvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SF_CB_AWADDR        =  sf_cb_awaddr[(C_S_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_SF_CB_AWADDRCONTROL = {
                                      sf_cb_awid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sf_cb_awprot[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sf_cb_awcache[(C_S_AXI_DEBUG_SLOT*4)+:4],
                                      sf_cb_awlock[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_awburst[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sf_cb_awsize[(C_S_AXI_DEBUG_SLOT*3)+:3],
                                      sf_cb_awlen[(C_S_AXI_DEBUG_SLOT*8)+:8],
                                      sf_cb_awready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_awvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SF_CB_BRESP         = {
                                      sf_cb_bid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sf_cb_bresp[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sf_cb_bready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_bvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SF_CB_RDATA         =  sf_cb_rdata[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_SF_CB_RDATACONTROL  = {
                                      sf_cb_rid[(C_S_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      sf_cb_rresp[(C_S_AXI_DEBUG_SLOT*2)+:2],
                                      sf_cb_rlast[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_rready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_rvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_SF_CB_WDATA         =  sf_cb_wdata[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_SF_CB_WDATACONTROL  = {
                                      sf_cb_wstrb[(C_S_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      sf_cb_wlast[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_wready[(C_S_AXI_DEBUG_SLOT*1)+:1],
                                      sf_cb_wvalid[(C_S_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign DEBUG_CB_MF_ARADDR        =  cb_mf_araddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_CB_MF_ARADDRCONTROL = {
                                      cb_mf_arid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      cb_mf_arprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      cb_mf_arcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      cb_mf_arlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_arburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      cb_mf_arsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      cb_mf_arlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      cb_mf_arready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_arvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_CB_MF_AWADDR        =  cb_mf_awaddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_CB_MF_AWADDRCONTROL = {
                                      cb_mf_awid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      cb_mf_awprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      cb_mf_awcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      cb_mf_awlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_awburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      cb_mf_awsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      cb_mf_awlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      cb_mf_awready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_awvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_CB_MF_BRESP         = {
                                      cb_mf_bid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      cb_mf_bresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      cb_mf_bready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_bvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_CB_MF_RDATA         =  cb_mf_rdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_CB_MF_RDATACONTROL  = {
                                      cb_mf_rid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      cb_mf_rresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      cb_mf_rlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_rready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_rvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_CB_MF_WDATA         =  cb_mf_wdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_CB_MF_WDATACONTROL  = {
                                      cb_mf_wstrb[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      cb_mf_wlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_wready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      cb_mf_wvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign DEBUG_MF_MC_ARADDR        =  mf_mc_araddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_MF_MC_ARADDRCONTROL = {
                                      mf_mc_arid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mf_mc_arprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mf_mc_arcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      mf_mc_arlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_arburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mf_mc_arsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mf_mc_arlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      mf_mc_arready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_arvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MF_MC_AWADDR        =  mf_mc_awaddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_MF_MC_AWADDRCONTROL = {
                                      mf_mc_awid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mf_mc_awprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mf_mc_awcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      mf_mc_awlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_awburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mf_mc_awsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mf_mc_awlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      mf_mc_awready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_awvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MF_MC_BRESP         = {
                                      mf_mc_bid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mf_mc_bresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mf_mc_bready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_bvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MF_MC_RDATA         =  mf_mc_rdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_MF_MC_RDATACONTROL  = {
                                      mf_mc_rid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mf_mc_rresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mf_mc_rlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_rready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_rvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MF_MC_WDATA         =  mf_mc_wdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_MF_MC_WDATACONTROL  = {
                                      mf_mc_wstrb[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      mf_mc_wlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_wready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mf_mc_wvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign DEBUG_MC_MP_ARADDR        =  mc_mp_araddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_MC_MP_ARADDRCONTROL = {
                                      mc_mp_arid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mc_mp_arprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mc_mp_arcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      mc_mp_arlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_arburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mc_mp_arsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mc_mp_arlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      mc_mp_arready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_arvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MC_MP_AWADDR        =  mc_mp_awaddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_MC_MP_AWADDRCONTROL = {
                                      mc_mp_awid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mc_mp_awprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mc_mp_awcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      mc_mp_awlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_awburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mc_mp_awsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mc_mp_awlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      mc_mp_awready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_awvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MC_MP_BRESP         = {
                                      mc_mp_bid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mc_mp_bresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mc_mp_bready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_bvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MC_MP_RDATA         =  mc_mp_rdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_MC_MP_RDATACONTROL  = {
                                      mc_mp_rid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mc_mp_rresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mc_mp_rlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_rready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_rvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MC_MP_WDATA         =  mc_mp_wdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_MC_MP_WDATACONTROL  = {
                                      mc_mp_wstrb[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      mc_mp_wlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_wready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mc_mp_wvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign DEBUG_MP_MR_ARADDR        =  mp_mr_araddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_MP_MR_ARADDRCONTROL = {
                                      mp_mr_arid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mp_mr_arprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mp_mr_arcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      mp_mr_arlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_arburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mp_mr_arsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mp_mr_arlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      mp_mr_arready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_arvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MP_MR_AWADDR        =  mp_mr_awaddr[(C_M_AXI_DEBUG_SLOT*C_AXI_ADDR_WIDTH)+:C_AXI_ADDR_WIDTH];
  assign DEBUG_MP_MR_AWADDRCONTROL = {
                                      mp_mr_awid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mp_mr_awprot[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mp_mr_awcache[(C_M_AXI_DEBUG_SLOT*4)+:4],
                                      mp_mr_awlock[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_awburst[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mp_mr_awsize[(C_M_AXI_DEBUG_SLOT*3)+:3],
                                      mp_mr_awlen[(C_M_AXI_DEBUG_SLOT*8)+:8],
                                      mp_mr_awready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_awvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MP_MR_BRESP         = {
                                      mp_mr_bid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mp_mr_bresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mp_mr_bready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_bvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MP_MR_RDATA         =  mp_mr_rdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_MP_MR_RDATACONTROL  = {
                                      mp_mr_rid[(C_M_AXI_DEBUG_SLOT*C_AXI_ID_WIDTH)+:C_AXI_ID_WIDTH],
                                      mp_mr_rresp[(C_M_AXI_DEBUG_SLOT*2)+:2],
                                      mp_mr_rlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_rready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_rvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };
  assign DEBUG_MP_MR_WDATA         =  mp_mr_wdata[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH)+:C_AXI_DATA_MAX_WIDTH];
  assign DEBUG_MP_MR_WDATACONTROL  = {
                                      mp_mr_wstrb[(C_M_AXI_DEBUG_SLOT*C_AXI_DATA_MAX_WIDTH/8)+:C_AXI_DATA_MAX_WIDTH/8],
                                      mp_mr_wlast[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_wready[(C_M_AXI_DEBUG_SLOT*1)+:1],
                                      mp_mr_wvalid[(C_M_AXI_DEBUG_SLOT*1)+:1]
                                     };

  assign INTERCONNECT_ARESET_OUT_N = interconnect_aresetn_i;

  localparam [`P_MAX_M*32-1:0] P_M_AXI_ID_WIDTH = {`P_MAX_M{f_bit32_qual(C_AXI_ID_WIDTH, 1, 0)}};
  localparam [`P_MAX_S*32-1:0] P_S_AXI_ID_WIDTH_MAX1 = f_thread_id_width(0);
  localparam [`P_MAX_S*64-1:0] P_S_AXI_BASE_ID = f_base_id(0);
  localparam [`P_MAX_S*64-1:0] P_S_AXI_HIGH_ID = f_high_id(0);
  localparam [31:0]  P_AXI4 = 32'h0;
  localparam [31:0]  P_AXI3 = 32'h1;
  localparam [31:0]  P_AXILITE = 32'h2;
  localparam [2:0]   P_AXILITE_SIZE = 3'b010;
  localparam [1:0]   P_INCR = 2'b01;
  localparam integer P_RANGE_CHECK = (C_RANGE_CHECK == 0) ? 0 : 1;  // Re-code 2 -> 1
  localparam integer P_ADDR_DECODE = (P_RANGE_CHECK || (C_NUM_MASTER_SLOTS>1) || f_multi_addr_ranges(0)) ? 1 : 0;
  localparam integer P_IGNORE_RID = ((C_INTERCONNECT_CONNECTIVITY_MODE==0) && ((C_NUM_MASTER_SLOTS>1) || (C_NUM_SLAVE_SLOTS>1) || (P_ADDR_DECODE!=0))) ? 1 : 0;
  localparam integer P_AXI3_BYPASS = (C_NUM_MASTER_SLOTS == 1) && (C_NUM_SLAVE_SLOTS == 1) && (P_ADDR_DECODE == 0) && 
             (C_S_AXI_PROTOCOL[31:0] == P_AXI3) && (C_M_AXI_PROTOCOL[31:0] == P_AXI3) &&
             (C_S_AXI_DATA_WIDTH[31:0] == C_M_AXI_DATA_WIDTH[31:0]) && (C_S_AXI_DATA_WIDTH[31:0] == C_INTERCONNECT_DATA_WIDTH) &&
             (C_S_AXI_ACLK_RATIO[31:0] == C_M_AXI_ACLK_RATIO[31:0]) && (C_S_AXI_ACLK_RATIO[31:0] == C_INTERCONNECT_ACLK_RATIO) &&
             (C_S_AXI_IS_ACLK_ASYNC[0] == 1'b0) && (C_M_AXI_IS_ACLK_ASYNC[0] == 1'b0) &&
             (C_S_AXI_WRITE_FIFO_DEPTH[31:0] == 0) && (C_S_AXI_READ_FIFO_DEPTH[31:0] == 0) && (C_M_AXI_WRITE_FIFO_DEPTH[31:0] == 0) && (C_M_AXI_READ_FIFO_DEPTH[31:0] == 0) && 
             (C_S_AXI_AW_REGISTER[31:0] == 0) && (C_S_AXI_W_REGISTER[31:0] == 0) && (C_S_AXI_B_REGISTER[31:0] == 0) && (C_S_AXI_AR_REGISTER[31:0] == 0) && (C_S_AXI_R_REGISTER[31:0] == 0) && 
             (C_M_AXI_AW_REGISTER[31:0] == 0) && (C_M_AXI_W_REGISTER[31:0] == 0) && (C_M_AXI_B_REGISTER[31:0] == 0) && (C_M_AXI_AR_REGISTER[31:0] == 0) && (C_M_AXI_R_REGISTER[31:0] == 0);
  
  function integer f_ceil_log2
    (
     input integer x
     );
    integer acc;
    begin
      acc=0;
      while ((2**acc) < x)
        acc = acc + 1;
      f_ceil_log2 = acc;
    end
  endfunction

  function [31:0] f_bit32_qual(
  // Cast as bit32. Replace with null_value if not qualified.
      input [31:0] arg,
      input        qual, // boolean
      input [31:0] null_val
    );
    begin
      f_bit32_qual = qual ? arg : null_val;
    end
  endfunction

  // Convert C_S_AXI_BASE_ID vector from Bit32 to Bit64 format
  function [`P_MAX_S*64-1:0] f_base_id
    (input null_arg);
    integer si;
    reg [`P_MAX_S*64-1:0] result;
    begin
      result = 0;
      for (si=0; si<C_NUM_SLAVE_SLOTS; si=si+1) begin
        result[si*64+:32] = C_S_AXI_BASE_ID[si*32+:32];
      end
      f_base_id = result;
    end
  endfunction

  // Construct P_S_HIGH_ID vector
  function [`P_MAX_S*64-1:0] f_high_id
    (input null_arg);
    integer si;
    reg [`P_MAX_S*64-1:0] result;
    begin
      result = 0;
      for (si=0; si<C_NUM_SLAVE_SLOTS; si=si+1) begin
        // Guard against any C_S_AXI_THREAD_ID_WIDTH > C_AXI_ID_WIDTH to prevent logic blow-up.
        // If violated, crossbar behaves as if all ID inputs sampled by corresponding SI-slot are all-zeros.
        result[si*64+:32] = 
          (C_S_AXI_THREAD_ID_WIDTH[si*32+16+:16] != 16'b0) ? C_S_AXI_BASE_ID[si*32+:32] :
          (C_S_AXI_THREAD_ID_WIDTH[si*32+:32] > C_AXI_ID_WIDTH) ? C_S_AXI_BASE_ID[si*32+:32] :
          (C_S_AXI_BASE_ID[si*32+:32] + 2**C_S_AXI_THREAD_ID_WIDTH[si*32+:32] - 1);
      end
      f_high_id = result;
    end
  endfunction

  // For a given slot (32-bit value) in C_S_AXI_THREAD_ID_WIDTH, generate a mask of valid ID bits.
  function [C_AXI_ID_WIDTH-1:0] f_thread_id_mask
    (input integer slot);
    integer i;
    begin
      f_thread_id_mask = 
        (C_S_AXI_THREAD_ID_WIDTH[slot*32+:32] == 32'b0) ? {C_AXI_ID_WIDTH{1'b0}} : 
        (C_S_AXI_THREAD_ID_WIDTH[slot*32+16+:16] != 16'b0) ? {C_AXI_ID_WIDTH{1'b0}} :
        (C_S_AXI_THREAD_ID_WIDTH[slot*32+:32] > C_AXI_ID_WIDTH) ? {C_AXI_ID_WIDTH{1'b0}} :
        ((2**C_S_AXI_THREAD_ID_WIDTH[slot*32+:32])-1);
    end
  endfunction

  function [`P_MAX_S*32-1:0] f_thread_id_width
  // For each 32-bit value in C_S_AXI_THREAD_ID_WIDTH, force within range 1-C_AXI_ID_WIDTH.
    (input null_arg);
    integer i;
    reg [`P_MAX_S*32-1:0] result;
    begin
      for (i=0; i<`P_MAX_S; i=i+1) begin
        result[i*32+:32] = 
          (C_S_AXI_THREAD_ID_WIDTH[i*32+:32] == 32'b0) ? 32'h00000001 : 
          (C_S_AXI_THREAD_ID_WIDTH[i*32+16+:16] != 16'b0) ? 32'h00000001 :
          (C_S_AXI_THREAD_ID_WIDTH[i*32+:32] > C_AXI_ID_WIDTH) ? 32'h00000001 :
          C_S_AXI_THREAD_ID_WIDTH[i*32+:32];
      end
      f_thread_id_width = result;
    end
  endfunction

  // Widths of all write issuance counters implemented in crossbar (before counter carry-out bit)
  function [(`P_MAX_M+1)*32-1:0] f_write_issue_width_vec
    (input null_arg);
    integer mi;
    reg [(`P_MAX_M+1)*32-1:0] result;
    begin
      result = 0;
      for (mi=0; mi<C_NUM_MASTER_SLOTS; mi=mi+1) begin
        result[mi*32+:32] = (C_M_AXI_PROTOCOL[mi*32+:32] == P_AXILITE) ? 32'h0 : f_ceil_log2(C_M_AXI_WRITE_ISSUING[mi*32+:32]);
      end
      result[C_NUM_MASTER_SLOTS*32+:32] = 32'h0;
      f_write_issue_width_vec = result;
    end
  endfunction

  // Widths of all read issuance counters implemented in crossbar (before counter carry-out bit)
  function [(`P_MAX_M+1)*32-1:0] f_read_issue_width_vec
    (input null_arg);
    integer mi;
    reg [(`P_MAX_M+1)*32-1:0] result;
    begin
      result = 0;
      for (mi=0; mi<C_NUM_MASTER_SLOTS; mi=mi+1) begin
        result[mi*32+:32] = (C_M_AXI_PROTOCOL[mi*32+:32] == P_AXILITE) ? 32'h0 : f_ceil_log2(C_M_AXI_READ_ISSUING[mi*32+:32]);
      end
      result[C_NUM_MASTER_SLOTS*32+:32] = 32'h0;
      f_read_issue_width_vec = result;
    end
  endfunction

  // Widths of all write acceptance counters implemented in crossbar (before counter carry-out bit)
  function [`P_MAX_S*32-1:0] f_write_accept_width_vec
    (input null_arg);
    integer si;
    reg [`P_MAX_S*32-1:0] result;
    begin
      result = 0;
      for (si=0; si<C_NUM_SLAVE_SLOTS; si=si+1) begin
        result[si*32+:32] = (C_S_AXI_PROTOCOL[si*32+:32] == P_AXILITE) ? 32'h0 : f_ceil_log2(C_S_AXI_WRITE_ACCEPTANCE[si*32+:32]);
      end
      f_write_accept_width_vec = result;
    end
  endfunction

  // Widths of all read acceptance counters implemented in crossbar (before counter carry-out bit)
  function [`P_MAX_S*32-1:0] f_read_accept_width_vec
    (input null_arg);
    integer si;
    reg [`P_MAX_S*32-1:0] result;
    begin
      result = 0;
      for (si=0; si<C_NUM_SLAVE_SLOTS; si=si+1) begin
        result[si*32+:32] = (C_S_AXI_PROTOCOL[si*32+:32] == P_AXILITE) ? 32'h0 : f_ceil_log2(C_S_AXI_READ_ACCEPTANCE[si*32+:32]);
      end
      f_read_accept_width_vec = result;
    end
  endfunction

  // Count number of valid address ranges for MI-slot #0
  function f_multi_addr_ranges
    (
      input integer null_arg
    );
    reg multi;
    integer rng;
    begin
      multi = 1'b0;
      for (rng=1; rng<`P_NUM_ADDR_RANGES; rng=rng+1) begin
        multi = multi | (~C_M_AXI_BASE_ADDR[rng*64+C_AXI_ADDR_WIDTH-1]) | C_M_AXI_HIGH_ADDR[rng*64+C_AXI_ADDR_WIDTH-1];
      end
      f_multi_addr_ranges = multi;
    end
  endfunction
  
  assign S_AXI_ARESET_OUT_N = s_axi_reset_out_n_i;
  assign M_AXI_ARESET_OUT_N = m_axi_reset_out_n_i;
  
genvar slot;

generate
  for (slot=0;slot<C_NUM_SLAVE_SLOTS;slot=slot+1) begin : gen_si_tieoff
    assign si_sr_awid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                             = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? (S_AXI_AWID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH] & f_thread_id_mask(slot))              : 0 ;
    assign si_sr_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_AWADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       : 0 ;
    assign si_sr_awlen[slot*8+:8]                                                      = (~C_S_AXI_SUPPORTS_WRITE[slot]) ? 0 : (C_S_AXI_PROTOCOL[slot*32+:32]==P_AXI4                             ) ? S_AXI_AWLEN[slot*8+:8] : (C_S_AXI_PROTOCOL[slot*32+:32]==P_AXI3) ? S_AXI_AWLEN[slot*8+:4] : 0 ;
    assign si_sr_awsize[slot*3+:3]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_AWSIZE[slot*3+:3]                                                     : P_AXILITE_SIZE ;
    assign si_sr_awburst[slot*2+:2]                                                    = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_AWBURST[slot*2+:2]                                                    : P_INCR ;
    assign si_sr_awlock[slot*2+:2]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? {1'b0, S_AXI_AWLOCK[slot*2+:1]}                                             : 0 ;
    assign si_sr_awcache[slot*4+:4]                                                    = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_AWCACHE[slot*4+:4]                                                    : 0 ;
    assign si_sr_awprot[slot*3+:3]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_AWPROT[slot*3+:3]                                                     : 0 ;
    assign si_sr_awqos[slot*4+:4]                                                      = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_AWQOS[slot*4+:4]                                                      : 0 ;
    assign si_sr_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                   = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? S_AXI_AWUSER[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                   : 0 ;
    assign si_sr_awvalid[slot*1+:1]                                                    = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_AWVALID[slot*1+:1]                                                    : 0 ;
    assign si_sr_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH]                = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]     : 0 ;
    assign si_sr_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_MAX_WIDTH/8]            = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_S_AXI_DATA_WIDTH[slot*32+:32]/8] : 0 ;
    assign si_sr_wlast[slot*1+:1]                                                      = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_WLAST[slot*1+:1]                                                      : 1'b1 ;
    assign si_sr_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                      = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? S_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                      : 0 ;
    assign si_sr_wvalid[slot*1+:1]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_WVALID[slot*1+:1]                                                     : 0 ;
    assign si_sr_bready[slot*1+:1]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? S_AXI_BREADY[slot*1+:1]                                                     : 0 ;
    assign si_sr_arid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                             = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? (S_AXI_ARID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH] & f_thread_id_mask(slot))              : 0 ;
    assign si_sr_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? S_AXI_ARADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       : 0 ;
    assign si_sr_arlen[slot*8+:8]                                                      = (~C_S_AXI_SUPPORTS_READ[slot]) ? 0 : (C_S_AXI_PROTOCOL[slot*32+:32]==P_AXI4                              ) ? S_AXI_ARLEN[slot*8+:8] : (C_S_AXI_PROTOCOL[slot*32+:32]==P_AXI3) ? S_AXI_ARLEN[slot*8+:4] : 0 ;
    assign si_sr_arsize[slot*3+:3]                                                     = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_ARSIZE[slot*3+:3]                                                     : P_AXILITE_SIZE ;
    assign si_sr_arburst[slot*2+:2]                                                    = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_ARBURST[slot*2+:2]                                                    : P_INCR ;
    assign si_sr_arlock[slot*2+:2]                                                     = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? {1'b0, S_AXI_ARLOCK[slot*2+:1]}                                             : 0 ;
    assign si_sr_arcache[slot*4+:4]                                                    = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_ARCACHE[slot*4+:4]                                                    : 0 ;
    assign si_sr_arprot[slot*3+:3]                                                     = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? S_AXI_ARPROT[slot*3+:3]                                                     : 0 ;
    assign si_sr_arqos[slot*4+:4]                                                      = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? S_AXI_ARQOS[slot*4+:4]                                                      : 0 ;
    assign si_sr_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                   = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? S_AXI_ARUSER[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                   : 0 ;
    assign si_sr_arvalid[slot*1+:1]                                                    = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? S_AXI_ARVALID[slot*1+:1]                                                    : 0 ;
    assign si_sr_rready[slot*1+:1]                                                     = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? S_AXI_RREADY[slot*1+:1]                                                     : 0 ;                                       
                                                                                                                                                                                                                                                                                      
    assign S_AXI_AWREADY[slot*1+:1]                                                    = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? si_sr_awready[slot*1+:1]                                                    : 0 ;
    assign S_AXI_WREADY[slot*1+:1]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? si_sr_wready[slot*1+:1]                                                     : 0 ;
    assign S_AXI_BID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? (si_sr_bid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH] & f_thread_id_mask(slot))               : 0 ;
    assign S_AXI_BRESP[slot*2+:2]                                                      = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? si_sr_bresp[slot*2+:2]                                                      : 0 ;
    assign S_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                      = (C_S_AXI_SUPPORTS_WRITE[slot] && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? si_sr_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                      : 0 ;
    assign S_AXI_BVALID[slot*1+:1]                                                     = (C_S_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? si_sr_bvalid[slot*1+:1]                                                     : 0 ;
    assign S_AXI_ARREADY[slot*1+:1]                                                    = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? si_sr_arready[slot*1+:1]                                                    : 0 ;
    assign S_AXI_RID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? (si_sr_rid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH] & f_thread_id_mask(slot))               : 0 ;
    assign S_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH]                = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? si_sr_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_S_AXI_DATA_WIDTH[slot*32+:32]]     : 0 ;
    assign S_AXI_RRESP[slot*2+:2]                                                      = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? si_sr_rresp[slot*2+:2]                                                      : 0 ;
    assign S_AXI_RLAST[slot*1+:1]                                                      = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? si_sr_rlast[slot*1+:1]                                                      : 0 ;
    assign S_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                      = (C_S_AXI_SUPPORTS_READ[slot]  && C_S_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? si_sr_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                      : 0 ;
    assign S_AXI_RVALID[slot*1+:1]                                                     = (C_S_AXI_SUPPORTS_READ[slot]                                                                             ) ? si_sr_rvalid[slot*1+:1]                                                     : 0 ;
  end
endgenerate

generate
  for (slot=0;slot<C_NUM_MASTER_SLOTS;slot=slot+1) begin : gen_mi_tieoff
    assign M_AXI_AWID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                             = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                             : 0 ;
    assign M_AXI_AWADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awaddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       : 0 ;
    assign M_AXI_AWLEN[slot*8+:8]                                                      = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awlen[slot*8+:8]                                                      : 0 ;
    assign M_AXI_AWSIZE[slot*3+:3]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awsize[slot*3+:3]                                                     : 0 ;
    assign M_AXI_AWBURST[slot*2+:2]                                                    = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awburst[slot*2+:2]                                                    : 0 ;
    assign M_AXI_AWLOCK[slot*2+:2]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awlock[slot*2+:2]                                                     : 0 ;
    assign M_AXI_AWCACHE[slot*4+:4]                                                    = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awcache[slot*4+:4]                                                    : 0 ;
    assign M_AXI_AWPROT[slot*3+:3]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awprot[slot*3+:3]                                                     : 0 ;
    assign M_AXI_AWREGION[slot*4+:4]                                                   = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awregion[slot*4+:4]                                                   : 0 ;
    assign M_AXI_AWQOS[slot*4+:4]                                                      = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awqos[slot*4+:4]                                                      : 0 ;
    assign M_AXI_AWUSER[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                   = (C_M_AXI_SUPPORTS_WRITE[slot] && C_AXI_SUPPORTS_USER_SIGNALS                                             ) ? mr_mi_awuser[slot*C_AXI_AWUSER_WIDTH+:C_AXI_AWUSER_WIDTH]                   : 0 ;
    assign M_AXI_AWVALID[slot*1+:1]                                                    = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_awvalid[slot*1+:1]                                                    : 0 ;
    assign M_AXI_WID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_wid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              : 0 ;
    assign M_AXI_WDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH]                = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_wdata[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]     : 0 ;
    assign M_AXI_WSTRB[slot*C_AXI_DATA_MAX_WIDTH/8+:C_AXI_DATA_MAX_WIDTH/8]            = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_wstrb[slot*C_AXI_DATA_MAX_WIDTH/8+:C_M_AXI_DATA_WIDTH[slot*32+:32]/8] : 0 ;
    assign M_AXI_WLAST[slot*1+:1]                                                      = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_wlast[slot*1+:1]                                                      : 0 ;
    assign M_AXI_WUSER[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                      = (C_M_AXI_SUPPORTS_WRITE[slot] && C_AXI_SUPPORTS_USER_SIGNALS                                             ) ? mr_mi_wuser[slot*C_AXI_WUSER_WIDTH+:C_AXI_WUSER_WIDTH]                      : 0 ;
    assign M_AXI_WVALID[slot*1+:1]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_wvalid[slot*1+:1]                                                     : 0 ;
    assign M_AXI_BREADY[slot*1+:1]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? mr_mi_bready[slot*1+:1]                                                     : 0 ;
    assign M_AXI_ARID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                             = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                            : 0 ;
    assign M_AXI_ARADDR[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_araddr[slot*C_AXI_ADDR_WIDTH+:C_AXI_ADDR_WIDTH]                       : 0 ;
    assign M_AXI_ARLEN[slot*8+:8]                                                      = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arlen[slot*8+:8]                                                      : 0 ;
    assign M_AXI_ARSIZE[slot*3+:3]                                                     = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arsize[slot*3+:3]                                                     : 0 ;
    assign M_AXI_ARBURST[slot*2+:2]                                                    = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arburst[slot*2+:2]                                                    : 0 ;
    assign M_AXI_ARLOCK[slot*2+:2]                                                     = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arlock[slot*2+:2]                                                     : 0 ;
    assign M_AXI_ARCACHE[slot*4+:4]                                                    = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arcache[slot*4+:4]                                                    : 0 ;
    assign M_AXI_ARPROT[slot*3+:3]                                                     = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arprot[slot*3+:3]                                                     : 0 ;
    assign M_AXI_ARREGION[slot*4+:4]                                                   = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arregion[slot*4+:4]                                                   : 0 ;
    assign M_AXI_ARQOS[slot*4+:4]                                                      = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arqos[slot*4+:4]                                                      : 0 ;
    assign M_AXI_ARUSER[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                   = (C_M_AXI_SUPPORTS_READ[slot]  && C_AXI_SUPPORTS_USER_SIGNALS                                             ) ? mr_mi_aruser[slot*C_AXI_ARUSER_WIDTH+:C_AXI_ARUSER_WIDTH]                   : 0 ;
    assign M_AXI_ARVALID[slot*1+:1]                                                    = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_arvalid[slot*1+:1]                                                    : 0 ;
    assign M_AXI_RREADY[slot*1+:1]                                                     = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? mr_mi_rready[slot*1+:1]                                                     : 0 ;
                                                                                                                                                                                                                                                                                      
    assign mr_mi_awready[slot*1+:1]                                                    = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? M_AXI_AWREADY[slot*1+:1]                                                    : 0 ;
    assign mr_mi_wready[slot*1+:1]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? M_AXI_WREADY[slot*1+:1]                                                     : 0 ;
    assign mr_mi_bid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              = (C_M_AXI_SUPPORTS_WRITE[slot] && C_M_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? M_AXI_BID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              : 0 ;
    assign mr_mi_bresp[slot*2+:2]                                                      = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? M_AXI_BRESP[slot*2+:2]                                                      : 0 ;
    assign mr_mi_buser[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                      = (C_M_AXI_SUPPORTS_WRITE[slot] && C_M_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? M_AXI_BUSER[slot*C_AXI_BUSER_WIDTH+:C_AXI_BUSER_WIDTH]                      : 0 ;
    assign mr_mi_bvalid[slot*1+:1]                                                     = (C_M_AXI_SUPPORTS_WRITE[slot]                                                                            ) ? M_AXI_BVALID[slot*1+:1]                                                     : 0 ;
    assign mr_mi_arready[slot*1+:1]                                                    = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? M_AXI_ARREADY[slot*1+:1]                                                    : 0 ;
    assign mr_mi_rid[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              = (C_M_AXI_SUPPORTS_READ[slot]  && C_M_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? M_AXI_RID[slot*C_AXI_ID_WIDTH+:C_AXI_ID_WIDTH]                              : 0 ;
    assign mr_mi_rdata[slot*C_AXI_DATA_MAX_WIDTH+:C_AXI_DATA_MAX_WIDTH]                = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? M_AXI_RDATA[slot*C_AXI_DATA_MAX_WIDTH+:C_M_AXI_DATA_WIDTH[slot*32+:32]]     : 0 ;
    assign mr_mi_rresp[slot*2+:2]                                                      = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? M_AXI_RRESP[slot*2+:2]                                                      : 0 ;
    assign mr_mi_rlast[slot*1+:1]                                                      = (C_M_AXI_SUPPORTS_READ[slot]  && C_M_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE                                ) ? M_AXI_RLAST[slot*1+:1]                                                      : 1'b1 ;
    assign mr_mi_ruser[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                      = (C_M_AXI_SUPPORTS_READ[slot]  && C_M_AXI_PROTOCOL[slot*32+:32]!=P_AXILITE && C_AXI_SUPPORTS_USER_SIGNALS ) ? M_AXI_RUSER[slot*C_AXI_RUSER_WIDTH+:C_AXI_RUSER_WIDTH]                      : 0 ;
    assign mr_mi_rvalid[slot*1+:1]                                                     = (C_M_AXI_SUPPORTS_READ[slot]                                                                             ) ? M_AXI_RVALID[slot*1+:1]                                                     : 0 ;
  end
endgenerate

  ict106_register_slice_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                (C_NUM_SLAVE_SLOTS),
    .C_AXI_ID_WIDTH                 (P_S_AXI_ID_WIDTH_MAX1),
//    .C_AXI_ID_MASK                 (P_S_AXI_ID_MASK),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_WIDTH               (C_S_AXI_DATA_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_AXI_PROTOCOL                 (C_S_AXI_PROTOCOL),
    .C_AXI_SUPPORTS_WRITE           (C_S_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_S_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_AXI_AW_REGISTER              (C_S_AXI_AW_REGISTER),
    .C_AXI_AR_REGISTER              (C_S_AXI_AR_REGISTER),
    .C_AXI_W_REGISTER               (C_S_AXI_W_REGISTER),
    .C_AXI_R_REGISTER               (C_S_AXI_R_REGISTER),
    .C_AXI_B_REGISTER               (C_S_AXI_B_REGISTER)
  )
  si_register_slice_bank 
  (
    .ARESETN                          (s_axi_reset_out_n_i),
    .ACLK                             (S_AXI_ACLK),
    .S_AXI_AWID                       (si_sr_awid        ),
    .S_AXI_AWADDR                     (si_sr_awaddr      ),
    .S_AXI_AWLEN                      (si_sr_awlen       ),
    .S_AXI_AWSIZE                     (si_sr_awsize      ),
    .S_AXI_AWBURST                    (si_sr_awburst     ),
    .S_AXI_AWLOCK                     (si_sr_awlock      ),
    .S_AXI_AWCACHE                    (si_sr_awcache     ),
    .S_AXI_AWPROT                     (si_sr_awprot      ),
    .S_AXI_AWQOS                      (si_sr_awqos       ),
    .S_AXI_AWUSER                     (si_sr_awuser      ),
    .S_AXI_AWVALID                    (si_sr_awvalid     ),
    .S_AXI_AWREADY                    (si_sr_awready     ),
    .S_AXI_WDATA                      (si_sr_wdata       ),
    .S_AXI_WSTRB                      (si_sr_wstrb       ),
    .S_AXI_WLAST                      (si_sr_wlast       ),
    .S_AXI_WUSER                      (si_sr_wuser       ),
    .S_AXI_WVALID                     (si_sr_wvalid      ),
    .S_AXI_WREADY                     (si_sr_wready      ),
    .S_AXI_BID                        (si_sr_bid         ),
    .S_AXI_BRESP                      (si_sr_bresp       ),
    .S_AXI_BUSER                      (si_sr_buser       ),
    .S_AXI_BVALID                     (si_sr_bvalid      ),
    .S_AXI_BREADY                     (si_sr_bready      ),
    .S_AXI_ARID                       (si_sr_arid        ),
    .S_AXI_ARADDR                     (si_sr_araddr      ),
    .S_AXI_ARLEN                      (si_sr_arlen       ),
    .S_AXI_ARSIZE                     (si_sr_arsize      ),
    .S_AXI_ARBURST                    (si_sr_arburst     ),
    .S_AXI_ARLOCK                     (si_sr_arlock      ),
    .S_AXI_ARCACHE                    (si_sr_arcache     ),
    .S_AXI_ARPROT                     (si_sr_arprot      ),
    .S_AXI_ARQOS                      (si_sr_arqos       ),
    .S_AXI_ARUSER                     (si_sr_aruser      ),
    .S_AXI_ARVALID                    (si_sr_arvalid     ),
    .S_AXI_ARREADY                    (si_sr_arready     ),
    .S_AXI_RID                        (si_sr_rid         ),
    .S_AXI_RDATA                      (si_sr_rdata       ),
    .S_AXI_RRESP                      (si_sr_rresp       ),
    .S_AXI_RLAST                      (si_sr_rlast       ),
    .S_AXI_RUSER                      (si_sr_ruser       ),
    .S_AXI_RVALID                     (si_sr_rvalid      ),
    .S_AXI_RREADY                     (si_sr_rready      ),
    .S_AXI_WID                        ({C_NUM_SLAVE_SLOTS*C_AXI_ID_WIDTH{1'b0}}),  // SI has no WID ports
    .S_AXI_AWREGION                   ({C_NUM_SLAVE_SLOTS{4'b0}}),  // SI has no REGION ports
    .S_AXI_ARREGION                   ({C_NUM_SLAVE_SLOTS{4'b0}}),  // SI has no REGION ports
    .M_AXI_AWID                       (sr_sc_awid        ),
    .M_AXI_AWADDR                     (sr_sc_awaddr      ),
    .M_AXI_AWLEN                      (sr_sc_awlen       ),
    .M_AXI_AWSIZE                     (sr_sc_awsize      ),
    .M_AXI_AWBURST                    (sr_sc_awburst     ),
    .M_AXI_AWLOCK                     (sr_sc_awlock      ),
    .M_AXI_AWCACHE                    (sr_sc_awcache     ),
    .M_AXI_AWPROT                     (sr_sc_awprot      ),
    .M_AXI_AWREGION                   (                  ),  // SI has no REGION ports
    .M_AXI_AWQOS                      (sr_sc_awqos       ),
    .M_AXI_AWUSER                     (sr_sc_awuser      ),
    .M_AXI_AWVALID                    (sr_sc_awvalid     ),
    .M_AXI_AWREADY                    (sr_sc_awready     ),
    .M_AXI_WID                        (                  ),  // SI has no WID ports
    .M_AXI_WDATA                      (sr_sc_wdata       ),
    .M_AXI_WSTRB                      (sr_sc_wstrb       ),
    .M_AXI_WLAST                      (sr_sc_wlast       ),
    .M_AXI_WUSER                      (sr_sc_wuser       ),
    .M_AXI_WVALID                     (sr_sc_wvalid      ),
    .M_AXI_WREADY                     (sr_sc_wready      ),
    .M_AXI_BID                        (sr_sc_bid         ),
    .M_AXI_BRESP                      (sr_sc_bresp       ),
    .M_AXI_BUSER                      (sr_sc_buser       ),
    .M_AXI_BVALID                     (sr_sc_bvalid      ),
    .M_AXI_BREADY                     (sr_sc_bready      ),
    .M_AXI_ARID                       (sr_sc_arid        ),
    .M_AXI_ARADDR                     (sr_sc_araddr      ),
    .M_AXI_ARLEN                      (sr_sc_arlen       ),
    .M_AXI_ARSIZE                     (sr_sc_arsize      ),
    .M_AXI_ARBURST                    (sr_sc_arburst     ),
    .M_AXI_ARLOCK                     (sr_sc_arlock      ),
    .M_AXI_ARCACHE                    (sr_sc_arcache     ),
    .M_AXI_ARPROT                     (sr_sc_arprot      ),
    .M_AXI_ARREGION                   (                  ),  // SI has no REGION ports
    .M_AXI_ARQOS                      (sr_sc_arqos       ),
    .M_AXI_ARUSER                     (sr_sc_aruser      ),
    .M_AXI_ARVALID                    (sr_sc_arvalid     ),
    .M_AXI_ARREADY                    (sr_sc_arready     ),
    .M_AXI_RID                        (sr_sc_rid         ),
    .M_AXI_RDATA                      (sr_sc_rdata       ),
    .M_AXI_RRESP                      (sr_sc_rresp       ),
    .M_AXI_RLAST                      (sr_sc_rlast       ),
    .M_AXI_RUSER                      (sr_sc_ruser       ),
    .M_AXI_RVALID                     (sr_sc_rvalid      ),
    .M_AXI_RREADY                     (sr_sc_rready      )
  );

  ict106_register_slice_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                  (C_NUM_MASTER_SLOTS),
    .C_AXI_ID_WIDTH                 (P_M_AXI_ID_WIDTH),
//    .C_AXI_ID_MASK                 ({C_NUM_MASTER_SLOTS*C_AXI_ID_WIDTH{1'b1}}),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_AXI_PROTOCOL                 (C_M_AXI_PROTOCOL),
    .C_AXI_SUPPORTS_WRITE           (C_M_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_M_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_AXI_AW_REGISTER              (C_M_AXI_AW_REGISTER),
    .C_AXI_AR_REGISTER              (C_M_AXI_AR_REGISTER),
    .C_AXI_W_REGISTER               (C_M_AXI_W_REGISTER),
    .C_AXI_R_REGISTER               (C_M_AXI_R_REGISTER),
    .C_AXI_B_REGISTER               (C_M_AXI_B_REGISTER)
  )
  mi_register_slice_bank 
  (
    .ARESETN                          (m_axi_reset_out_n_i),
    .ACLK                             (M_AXI_ACLK),
    .S_AXI_AWID                       (mp_mr_awid         ),
    .S_AXI_AWADDR                     (mp_mr_awaddr       ),
    .S_AXI_AWLEN                      (mp_mr_awlen        ),
    .S_AXI_AWSIZE                     (mp_mr_awsize       ),
    .S_AXI_AWBURST                    (mp_mr_awburst      ),
    .S_AXI_AWLOCK                     (mp_mr_awlock       ),
    .S_AXI_AWCACHE                    (mp_mr_awcache      ),
    .S_AXI_AWPROT                     (mp_mr_awprot       ),
    .S_AXI_AWREGION                   (mp_mr_awregion     ),
    .S_AXI_AWQOS                      (mp_mr_awqos        ),
    .S_AXI_AWUSER                     (mp_mr_awuser       ),
    .S_AXI_AWVALID                    (mp_mr_awvalid      ),
    .S_AXI_AWREADY                    (mp_mr_awready      ),
    .S_AXI_WID                        (mp_mr_wid          ),
    .S_AXI_WDATA                      (mp_mr_wdata        ),
    .S_AXI_WSTRB                      (mp_mr_wstrb        ),
    .S_AXI_WLAST                      (mp_mr_wlast        ),
    .S_AXI_WUSER                      (mp_mr_wuser        ),
    .S_AXI_WVALID                     (mp_mr_wvalid       ),
    .S_AXI_WREADY                     (mp_mr_wready       ),
    .S_AXI_BID                        (mp_mr_bid          ),
    .S_AXI_BRESP                      (mp_mr_bresp        ),
    .S_AXI_BUSER                      (mp_mr_buser        ),
    .S_AXI_BVALID                     (mp_mr_bvalid       ),
    .S_AXI_BREADY                     (mp_mr_bready       ),
    .S_AXI_ARID                       (mp_mr_arid         ),
    .S_AXI_ARADDR                     (mp_mr_araddr       ),
    .S_AXI_ARLEN                      (mp_mr_arlen        ),
    .S_AXI_ARSIZE                     (mp_mr_arsize       ),
    .S_AXI_ARBURST                    (mp_mr_arburst      ),
    .S_AXI_ARLOCK                     (mp_mr_arlock       ),
    .S_AXI_ARCACHE                    (mp_mr_arcache      ),
    .S_AXI_ARPROT                     (mp_mr_arprot       ),
    .S_AXI_ARREGION                   (mp_mr_arregion     ),
    .S_AXI_ARQOS                      (mp_mr_arqos        ),
    .S_AXI_ARUSER                     (mp_mr_aruser       ),
    .S_AXI_ARVALID                    (mp_mr_arvalid      ),
    .S_AXI_ARREADY                    (mp_mr_arready      ),
    .S_AXI_RID                        (mp_mr_rid          ),
    .S_AXI_RDATA                      (mp_mr_rdata        ),
    .S_AXI_RRESP                      (mp_mr_rresp        ),
    .S_AXI_RLAST                      (mp_mr_rlast        ),
    .S_AXI_RUSER                      (mp_mr_ruser        ),
    .S_AXI_RVALID                     (mp_mr_rvalid       ),
    .S_AXI_RREADY                     (mp_mr_rready       ),
    .M_AXI_AWID                       (mr_mi_awid         ),
    .M_AXI_AWADDR                     (mr_mi_awaddr       ),
    .M_AXI_AWLEN                      (mr_mi_awlen        ),
    .M_AXI_AWSIZE                     (mr_mi_awsize       ),
    .M_AXI_AWBURST                    (mr_mi_awburst      ),
    .M_AXI_AWLOCK                     (mr_mi_awlock       ),
    .M_AXI_AWCACHE                    (mr_mi_awcache      ),
    .M_AXI_AWPROT                     (mr_mi_awprot       ),
    .M_AXI_AWREGION                   (mr_mi_awregion     ),
    .M_AXI_AWQOS                      (mr_mi_awqos        ),
    .M_AXI_AWUSER                     (mr_mi_awuser       ),
    .M_AXI_AWVALID                    (mr_mi_awvalid      ),
    .M_AXI_AWREADY                    (mr_mi_awready      ),
    .M_AXI_WID                        (mr_mi_wid          ),
    .M_AXI_WDATA                      (mr_mi_wdata        ),
    .M_AXI_WSTRB                      (mr_mi_wstrb        ),
    .M_AXI_WLAST                      (mr_mi_wlast        ),
    .M_AXI_WUSER                      (mr_mi_wuser        ),
    .M_AXI_WVALID                     (mr_mi_wvalid       ),
    .M_AXI_WREADY                     (mr_mi_wready       ),
    .M_AXI_BID                        (mr_mi_bid          ),
    .M_AXI_BRESP                      (mr_mi_bresp        ),
    .M_AXI_BUSER                      (mr_mi_buser        ),
    .M_AXI_BVALID                     (mr_mi_bvalid       ),
    .M_AXI_BREADY                     (mr_mi_bready       ),
    .M_AXI_ARID                       (mr_mi_arid         ),
    .M_AXI_ARADDR                     (mr_mi_araddr       ),
    .M_AXI_ARLEN                      (mr_mi_arlen        ),
    .M_AXI_ARSIZE                     (mr_mi_arsize       ),
    .M_AXI_ARBURST                    (mr_mi_arburst      ),
    .M_AXI_ARLOCK                     (mr_mi_arlock       ),
    .M_AXI_ARCACHE                    (mr_mi_arcache      ),
    .M_AXI_ARPROT                     (mr_mi_arprot       ),
    .M_AXI_ARREGION                   (mr_mi_arregion     ),
    .M_AXI_ARQOS                      (mr_mi_arqos        ),
    .M_AXI_ARUSER                     (mr_mi_aruser       ),
    .M_AXI_ARVALID                    (mr_mi_arvalid      ),
    .M_AXI_ARREADY                    (mr_mi_arready      ),
    .M_AXI_RID                        (mr_mi_rid          ),
    .M_AXI_RDATA                      (mr_mi_rdata        ),
    .M_AXI_RRESP                      (mr_mi_rresp        ),
    .M_AXI_RLAST                      (mr_mi_rlast        ),
    .M_AXI_RUSER                      (mr_mi_ruser        ),
    .M_AXI_RVALID                     (mr_mi_rvalid       ),
    .M_AXI_RREADY                     (mr_mi_rready       )
  );

  ict106_protocol_conv_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                  (C_NUM_MASTER_SLOTS),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_IGNORE_RID                      (P_IGNORE_RID),
    .C_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_AXI_PROTOCOL               (C_M_AXI_PROTOCOL),
    .C_AXI_SUPPORTS_WRITE           (C_M_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_M_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_AXI3_BYPASS                    (P_AXI3_BYPASS)
  )
  mi_protocol_conv_bank 
  (
    .ARESETN                          (m_axi_reset_out_n_i),
    .ACLK                             (M_AXI_ACLK),
    .S_AXI_AWID                       (mc_mp_awid         ),
    .S_AXI_AWADDR                     (mc_mp_awaddr       ),
    .S_AXI_AWLEN                      (mc_mp_awlen        ),
    .S_AXI_AWSIZE                     (mc_mp_awsize       ),
    .S_AXI_AWBURST                    (mc_mp_awburst      ),
    .S_AXI_AWLOCK                     (mc_mp_awlock       ),
    .S_AXI_AWCACHE                    (mc_mp_awcache      ),
    .S_AXI_AWPROT                     (mc_mp_awprot       ),
    .S_AXI_AWREGION                   (mc_mp_awregion     ),
    .S_AXI_AWQOS                      (mc_mp_awqos        ),
    .S_AXI_AWUSER                     (mc_mp_awuser       ),
    .S_AXI_AWVALID                    (mc_mp_awvalid      ),
    .S_AXI_AWREADY                    (mc_mp_awready      ),
    .S_AXI_WID                        (S_AXI_WID[C_AXI_ID_WIDTH-1:0]),
    .S_AXI_WDATA                      (mc_mp_wdata        ),
    .S_AXI_WSTRB                      (mc_mp_wstrb        ),
    .S_AXI_WLAST                      (mc_mp_wlast        ),
    .S_AXI_WUSER                      (mc_mp_wuser        ),
    .S_AXI_WVALID                     (mc_mp_wvalid       ),
    .S_AXI_WREADY                     (mc_mp_wready       ),
    .S_AXI_BID                        (mc_mp_bid          ),
    .S_AXI_BRESP                      (mc_mp_bresp        ),
    .S_AXI_BUSER                      (mc_mp_buser        ),
    .S_AXI_BVALID                     (mc_mp_bvalid       ),
    .S_AXI_BREADY                     (mc_mp_bready       ),
    .S_AXI_ARID                       (mc_mp_arid         ),
    .S_AXI_ARADDR                     (mc_mp_araddr       ),
    .S_AXI_ARLEN                      (mc_mp_arlen        ),
    .S_AXI_ARSIZE                     (mc_mp_arsize       ),
    .S_AXI_ARBURST                    (mc_mp_arburst      ),
    .S_AXI_ARLOCK                     (mc_mp_arlock       ),
    .S_AXI_ARCACHE                    (mc_mp_arcache      ),
    .S_AXI_ARPROT                     (mc_mp_arprot       ),
    .S_AXI_ARREGION                   (mc_mp_arregion     ),
    .S_AXI_ARQOS                      (mc_mp_arqos        ),
    .S_AXI_ARUSER                     (mc_mp_aruser       ),
    .S_AXI_ARVALID                    (mc_mp_arvalid      ),
    .S_AXI_ARREADY                    (mc_mp_arready      ),
    .S_AXI_RID                        (mc_mp_rid          ),
    .S_AXI_RDATA                      (mc_mp_rdata        ),
    .S_AXI_RRESP                      (mc_mp_rresp        ),
    .S_AXI_RLAST                      (mc_mp_rlast        ),
    .S_AXI_RUSER                      (mc_mp_ruser        ),
    .S_AXI_RVALID                     (mc_mp_rvalid       ),
    .S_AXI_RREADY                     (mc_mp_rready       ),
    .M_AXI_AWID                       (mp_mr_awid         ),
    .M_AXI_AWADDR                     (mp_mr_awaddr       ),
    .M_AXI_AWLEN                      (mp_mr_awlen        ),
    .M_AXI_AWSIZE                     (mp_mr_awsize       ),
    .M_AXI_AWBURST                    (mp_mr_awburst      ),
    .M_AXI_AWLOCK                     (mp_mr_awlock       ),
    .M_AXI_AWCACHE                    (mp_mr_awcache      ),
    .M_AXI_AWPROT                     (mp_mr_awprot       ),
    .M_AXI_AWREGION                   (mp_mr_awregion     ),
    .M_AXI_AWQOS                      (mp_mr_awqos        ),
    .M_AXI_AWUSER                     (mp_mr_awuser       ),
    .M_AXI_AWVALID                    (mp_mr_awvalid      ),
    .M_AXI_AWREADY                    (mp_mr_awready      ),
    .M_AXI_WID                        (mp_mr_wid          ),
    .M_AXI_WDATA                      (mp_mr_wdata        ),
    .M_AXI_WSTRB                      (mp_mr_wstrb        ),
    .M_AXI_WLAST                      (mp_mr_wlast        ),
    .M_AXI_WUSER                      (mp_mr_wuser        ),
    .M_AXI_WVALID                     (mp_mr_wvalid       ),
    .M_AXI_WREADY                     (mp_mr_wready       ),
    .M_AXI_BID                        (mp_mr_bid          ),
    .M_AXI_BRESP                      (mp_mr_bresp        ),
    .M_AXI_BUSER                      (mp_mr_buser        ),
    .M_AXI_BVALID                     (mp_mr_bvalid       ),
    .M_AXI_BREADY                     (mp_mr_bready       ),
    .M_AXI_ARID                       (mp_mr_arid         ),
    .M_AXI_ARADDR                     (mp_mr_araddr       ),
    .M_AXI_ARLEN                      (mp_mr_arlen        ),
    .M_AXI_ARSIZE                     (mp_mr_arsize       ),
    .M_AXI_ARBURST                    (mp_mr_arburst      ),
    .M_AXI_ARLOCK                     (mp_mr_arlock       ),
    .M_AXI_ARCACHE                    (mp_mr_arcache      ),
    .M_AXI_ARPROT                     (mp_mr_arprot       ),
    .M_AXI_ARREGION                   (mp_mr_arregion     ),
    .M_AXI_ARQOS                      (mp_mr_arqos        ),
    .M_AXI_ARUSER                     (mp_mr_aruser       ),
    .M_AXI_ARVALID                    (mp_mr_arvalid      ),
    .M_AXI_ARREADY                    (mp_mr_arready      ),
    .M_AXI_RID                        (mp_mr_rid          ),
    .M_AXI_RDATA                      (mp_mr_rdata        ),
    .M_AXI_RRESP                      (mp_mr_rresp        ),
    .M_AXI_RLAST                      (mp_mr_rlast        ),
    .M_AXI_RUSER                      (mp_mr_ruser        ),
    .M_AXI_RVALID                     (mp_mr_rvalid       ),
    .M_AXI_RREADY                     (mp_mr_rready       )
  );

  ict106_converter_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                (C_NUM_SLAVE_SLOTS),
    .C_AXI_ID_WIDTH                 (P_S_AXI_ID_WIDTH_MAX1),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_S_AXI_DATA_WIDTH               (C_S_AXI_DATA_WIDTH),
    .C_M_AXI_DATA_WIDTH               ({C_NUM_SLAVE_SLOTS{f_bit32_qual(C_INTERCONNECT_DATA_WIDTH,1,0)}}),
    .C_AXI_PROTOCOL                 (C_S_AXI_PROTOCOL),
    .C_AXI_IS_ACLK_ASYNC            (C_S_AXI_IS_ACLK_ASYNC),
    .C_S_AXI_ACLK_RATIO               (C_S_AXI_ACLK_RATIO),
    .C_M_AXI_ACLK_RATIO               ({C_NUM_SLAVE_SLOTS{f_bit32_qual(C_INTERCONNECT_ACLK_RATIO,1,0)}}),
    .C_AXI_SUPPORTS_WRITE           (C_S_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_S_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_HEMISPHERE                   ("si")
  )
  si_converter_bank 
  (
    .INTERCONNECT_ACLK                (INTERCONNECT_ACLK),
    .INTERCONNECT_ARESETN             (INTERCONNECT_ARESETN),
    .LOCAL_ARESETN                    (interconnect_aresetn_i),
    .INTERCONNECT_RESET_OUT_N         (interconnect_aresetn_i),
    .S_AXI_RESET_OUT_N                (s_axi_reset_out_n_i),
    .M_AXI_RESET_OUT_N                (),
    .S_AXI_ACLK                       (S_AXI_ACLK           ),
    .S_AXI_AWID                       (sr_sc_awid           ),
    .S_AXI_AWADDR                     (sr_sc_awaddr         ),
    .S_AXI_AWLEN                      (sr_sc_awlen          ),
    .S_AXI_AWSIZE                     (sr_sc_awsize         ),
    .S_AXI_AWBURST                    (sr_sc_awburst        ),
    .S_AXI_AWLOCK                     (sr_sc_awlock         ),
    .S_AXI_AWCACHE                    (sr_sc_awcache        ),
    .S_AXI_AWPROT                     (sr_sc_awprot         ),
    .S_AXI_AWQOS                      (sr_sc_awqos          ),
    .S_AXI_AWUSER                     (sr_sc_awuser         ),
    .S_AXI_AWVALID                    (sr_sc_awvalid        ),
    .S_AXI_AWREADY                    (sr_sc_awready        ),
    .S_AXI_WDATA                      (sr_sc_wdata          ),
    .S_AXI_WSTRB                      (sr_sc_wstrb          ),
    .S_AXI_WLAST                      (sr_sc_wlast          ),
    .S_AXI_WUSER                      (sr_sc_wuser          ),
    .S_AXI_WVALID                     (sr_sc_wvalid         ),
    .S_AXI_WREADY                     (sr_sc_wready         ),
    .S_AXI_BID                        (sr_sc_bid            ),
    .S_AXI_BRESP                      (sr_sc_bresp          ),
    .S_AXI_BUSER                      (sr_sc_buser          ),
    .S_AXI_BVALID                     (sr_sc_bvalid         ),
    .S_AXI_BREADY                     (sr_sc_bready         ),
    .S_AXI_ARID                       (sr_sc_arid           ),
    .S_AXI_ARADDR                     (sr_sc_araddr         ),
    .S_AXI_ARLEN                      (sr_sc_arlen          ),
    .S_AXI_ARSIZE                     (sr_sc_arsize         ),
    .S_AXI_ARBURST                    (sr_sc_arburst        ),
    .S_AXI_ARLOCK                     (sr_sc_arlock         ),
    .S_AXI_ARCACHE                    (sr_sc_arcache        ),
    .S_AXI_ARPROT                     (sr_sc_arprot         ),
    .S_AXI_ARQOS                      (sr_sc_arqos          ),
    .S_AXI_ARUSER                     (sr_sc_aruser         ),
    .S_AXI_ARVALID                    (sr_sc_arvalid        ),
    .S_AXI_ARREADY                    (sr_sc_arready        ),
    .S_AXI_RID                        (sr_sc_rid            ),
    .S_AXI_RDATA                      (sr_sc_rdata          ),
    .S_AXI_RRESP                      (sr_sc_rresp          ),
    .S_AXI_RLAST                      (sr_sc_rlast          ),
    .S_AXI_RUSER                      (sr_sc_ruser          ),
    .S_AXI_RVALID                     (sr_sc_rvalid         ),
    .S_AXI_RREADY                     (sr_sc_rready         ),
    .S_AXI_AWREGION                   ({C_NUM_SLAVE_SLOTS{4'b0}}),  // SI has no REGION ports
    .S_AXI_ARREGION                   ({C_NUM_SLAVE_SLOTS{4'b0}}),  // SI has no REGION ports
    .M_AXI_ACLK                       ({C_NUM_SLAVE_SLOTS{INTERCONNECT_ACLK}}),
    .M_AXI_AWID                       (sc_sf_awid           ),
    .M_AXI_AWADDR                     (sc_sf_awaddr         ),
    .M_AXI_AWLEN                      (sc_sf_awlen          ),
    .M_AXI_AWSIZE                     (sc_sf_awsize         ),
    .M_AXI_AWBURST                    (sc_sf_awburst        ),
    .M_AXI_AWLOCK                     (sc_sf_awlock         ),
    .M_AXI_AWCACHE                    (sc_sf_awcache        ),
    .M_AXI_AWPROT                     (sc_sf_awprot         ),
    .M_AXI_AWREGION                   (                     ),  // SI has no REGION ports
    .M_AXI_AWQOS                      (sc_sf_awqos          ),
    .M_AXI_AWUSER                     (sc_sf_awuser         ),
    .M_AXI_AWVALID                    (sc_sf_awvalid        ),
    .M_AXI_AWREADY                    (sc_sf_awready        ),
    .M_AXI_WDATA                      (sc_sf_wdata          ),
    .M_AXI_WSTRB                      (sc_sf_wstrb          ),
    .M_AXI_WLAST                      (sc_sf_wlast          ),
    .M_AXI_WUSER                      (sc_sf_wuser          ),
    .M_AXI_WVALID                     (sc_sf_wvalid         ),
    .M_AXI_WREADY                     (sc_sf_wready         ),
    .M_AXI_BID                        (sc_sf_bid            ),
    .M_AXI_BRESP                      (sc_sf_bresp          ),
    .M_AXI_BUSER                      (sc_sf_buser          ),
    .M_AXI_BVALID                     (sc_sf_bvalid         ),
    .M_AXI_BREADY                     (sc_sf_bready         ),
    .M_AXI_ARID                       (sc_sf_arid           ),
    .M_AXI_ARADDR                     (sc_sf_araddr         ),
    .M_AXI_ARLEN                      (sc_sf_arlen          ),
    .M_AXI_ARSIZE                     (sc_sf_arsize         ),
    .M_AXI_ARBURST                    (sc_sf_arburst        ),
    .M_AXI_ARLOCK                     (sc_sf_arlock         ),
    .M_AXI_ARCACHE                    (sc_sf_arcache        ),
    .M_AXI_ARPROT                     (sc_sf_arprot         ),
    .M_AXI_ARREGION                   (                     ),  // SI has no REGION ports
    .M_AXI_ARQOS                      (sc_sf_arqos          ),
    .M_AXI_ARUSER                     (sc_sf_aruser         ),
    .M_AXI_ARVALID                    (sc_sf_arvalid        ),
    .M_AXI_ARREADY                    (sc_sf_arready        ),
    .M_AXI_RID                        (sc_sf_rid            ),
    .M_AXI_RDATA                      (sc_sf_rdata          ),
    .M_AXI_RRESP                      (sc_sf_rresp          ),
    .M_AXI_RLAST                      (sc_sf_rlast          ),
    .M_AXI_RUSER                      (sc_sf_ruser          ),
    .M_AXI_RVALID                     (sc_sf_rvalid         ),
    .M_AXI_RREADY                     (sc_sf_rready         )
  );

  ict106_converter_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                (C_NUM_MASTER_SLOTS),
    .C_AXI_ID_WIDTH                 (P_M_AXI_ID_WIDTH),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_S_AXI_DATA_WIDTH               ({C_NUM_MASTER_SLOTS{f_bit32_qual(C_INTERCONNECT_DATA_WIDTH,1,0)}}),
    .C_M_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH),
    .C_AXI_PROTOCOL                 (C_M_AXI_PROTOCOL),
    .C_AXI_IS_ACLK_ASYNC            (C_M_AXI_IS_ACLK_ASYNC),
    .C_S_AXI_ACLK_RATIO               ({C_NUM_MASTER_SLOTS{f_bit32_qual(C_INTERCONNECT_ACLK_RATIO,1,0)}}),
    .C_M_AXI_ACLK_RATIO               (C_M_AXI_ACLK_RATIO),
    .C_AXI_SUPPORTS_WRITE           (C_M_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_M_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_HEMISPHERE                   ("mi")
  )
  mi_converter_bank 
  (
    .INTERCONNECT_ACLK                (INTERCONNECT_ACLK),
    .INTERCONNECT_ARESETN             (INTERCONNECT_ARESETN),
    .LOCAL_ARESETN                    (interconnect_aresetn_i),
    .INTERCONNECT_RESET_OUT_N         (),
    .S_AXI_RESET_OUT_N                (),
    .M_AXI_RESET_OUT_N                (m_axi_reset_out_n_i),
    .S_AXI_ACLK                       ({C_NUM_MASTER_SLOTS{INTERCONNECT_ACLK}}),
    .S_AXI_AWID                       (mf_mc_awid           ),
    .S_AXI_AWADDR                     (mf_mc_awaddr         ),
    .S_AXI_AWLEN                      (mf_mc_awlen          ),
    .S_AXI_AWSIZE                     (mf_mc_awsize         ),
    .S_AXI_AWBURST                    (mf_mc_awburst        ),
    .S_AXI_AWLOCK                     (mf_mc_awlock         ),
    .S_AXI_AWCACHE                    (mf_mc_awcache        ),
    .S_AXI_AWPROT                     (mf_mc_awprot         ),
    .S_AXI_AWREGION                   (mf_mc_awregion       ),
    .S_AXI_AWQOS                      (mf_mc_awqos          ),
    .S_AXI_AWUSER                     (mf_mc_awuser         ),
    .S_AXI_AWVALID                    (mf_mc_awvalid        ),
    .S_AXI_AWREADY                    (mf_mc_awready        ),
    .S_AXI_WDATA                      (mf_mc_wdata          ),
    .S_AXI_WSTRB                      (mf_mc_wstrb          ),
    .S_AXI_WLAST                      (mf_mc_wlast          ),
    .S_AXI_WUSER                      (mf_mc_wuser          ),
    .S_AXI_WVALID                     (mf_mc_wvalid         ),
    .S_AXI_WREADY                     (mf_mc_wready         ),
    .S_AXI_BID                        (mf_mc_bid            ),
    .S_AXI_BRESP                      (mf_mc_bresp          ),
    .S_AXI_BUSER                      (mf_mc_buser          ),
    .S_AXI_BVALID                     (mf_mc_bvalid         ),
    .S_AXI_BREADY                     (mf_mc_bready         ),
    .S_AXI_ARID                       (mf_mc_arid           ),
    .S_AXI_ARADDR                     (mf_mc_araddr         ),
    .S_AXI_ARLEN                      (mf_mc_arlen          ),
    .S_AXI_ARSIZE                     (mf_mc_arsize         ),
    .S_AXI_ARBURST                    (mf_mc_arburst        ),
    .S_AXI_ARLOCK                     (mf_mc_arlock         ),
    .S_AXI_ARCACHE                    (mf_mc_arcache        ),
    .S_AXI_ARPROT                     (mf_mc_arprot         ),
    .S_AXI_ARREGION                   (mf_mc_arregion       ),
    .S_AXI_ARQOS                      (mf_mc_arqos          ),
    .S_AXI_ARUSER                     (mf_mc_aruser         ),
    .S_AXI_ARVALID                    (mf_mc_arvalid        ),
    .S_AXI_ARREADY                    (mf_mc_arready        ),
    .S_AXI_RID                        (mf_mc_rid            ),
    .S_AXI_RDATA                      (mf_mc_rdata          ),
    .S_AXI_RRESP                      (mf_mc_rresp          ),
    .S_AXI_RLAST                      (mf_mc_rlast          ),
    .S_AXI_RUSER                      (mf_mc_ruser          ),
    .S_AXI_RVALID                     (mf_mc_rvalid         ),
    .S_AXI_RREADY                     (mf_mc_rready         ),
    .M_AXI_ACLK                       (M_AXI_ACLK           ),
    .M_AXI_AWID                       (mc_mp_awid           ),
    .M_AXI_AWADDR                     (mc_mp_awaddr         ),
    .M_AXI_AWLEN                      (mc_mp_awlen          ),
    .M_AXI_AWSIZE                     (mc_mp_awsize         ),
    .M_AXI_AWBURST                    (mc_mp_awburst        ),
    .M_AXI_AWLOCK                     (mc_mp_awlock         ),
    .M_AXI_AWCACHE                    (mc_mp_awcache        ),
    .M_AXI_AWPROT                     (mc_mp_awprot         ),
    .M_AXI_AWREGION                   (mc_mp_awregion       ),
    .M_AXI_AWQOS                      (mc_mp_awqos          ),
    .M_AXI_AWUSER                     (mc_mp_awuser         ),
    .M_AXI_AWVALID                    (mc_mp_awvalid        ),
    .M_AXI_AWREADY                    (mc_mp_awready        ),
    .M_AXI_WDATA                      (mc_mp_wdata          ),
    .M_AXI_WSTRB                      (mc_mp_wstrb          ),
    .M_AXI_WLAST                      (mc_mp_wlast          ),
    .M_AXI_WUSER                      (mc_mp_wuser          ),
    .M_AXI_WVALID                     (mc_mp_wvalid         ),
    .M_AXI_WREADY                     (mc_mp_wready         ),
    .M_AXI_BID                        (mc_mp_bid            ),
    .M_AXI_BRESP                      (mc_mp_bresp          ),
    .M_AXI_BUSER                      (mc_mp_buser          ),
    .M_AXI_BVALID                     (mc_mp_bvalid         ),
    .M_AXI_BREADY                     (mc_mp_bready         ),
    .M_AXI_ARID                       (mc_mp_arid           ),
    .M_AXI_ARADDR                     (mc_mp_araddr         ),
    .M_AXI_ARLEN                      (mc_mp_arlen          ),
    .M_AXI_ARSIZE                     (mc_mp_arsize         ),
    .M_AXI_ARBURST                    (mc_mp_arburst        ),
    .M_AXI_ARLOCK                     (mc_mp_arlock         ),
    .M_AXI_ARCACHE                    (mc_mp_arcache        ),
    .M_AXI_ARPROT                     (mc_mp_arprot         ),
    .M_AXI_ARREGION                   (mc_mp_arregion       ),
    .M_AXI_ARQOS                      (mc_mp_arqos          ),
    .M_AXI_ARUSER                     (mc_mp_aruser         ),
    .M_AXI_ARVALID                    (mc_mp_arvalid        ),
    .M_AXI_ARREADY                    (mc_mp_arready        ),
    .M_AXI_RID                        (mc_mp_rid            ),
    .M_AXI_RDATA                      (mc_mp_rdata          ),
    .M_AXI_RRESP                      (mc_mp_rresp          ),
    .M_AXI_RLAST                      (mc_mp_rlast          ),
    .M_AXI_RUSER                      (mc_mp_ruser          ),
    .M_AXI_RVALID                     (mc_mp_rvalid         ),
    .M_AXI_RREADY                     (mc_mp_rready         )
  );

  ict106_data_fifo_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                (C_NUM_SLAVE_SLOTS),
    .C_AXI_ID_WIDTH                 (P_S_AXI_ID_WIDTH_MAX1),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_AXI_DATA_WIDTH               ({C_NUM_SLAVE_SLOTS{f_bit32_qual(C_INTERCONNECT_DATA_WIDTH,1,0)}}),
    .C_AXI_SUPPORTS_WRITE           (C_S_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_S_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_AXI_WRITE_FIFO_DEPTH         (C_S_AXI_WRITE_FIFO_DEPTH),
    .C_AXI_WRITE_FIFO_TYPE          (C_S_AXI_WRITE_FIFO_TYPE),
    .C_AXI_WRITE_FIFO_DELAY         (C_S_AXI_WRITE_FIFO_DELAY),
    .C_AXI_READ_FIFO_DEPTH          (C_S_AXI_READ_FIFO_DEPTH),
    .C_AXI_READ_FIFO_TYPE           (C_S_AXI_READ_FIFO_TYPE),
    .C_AXI_READ_FIFO_DELAY          (C_S_AXI_READ_FIFO_DELAY)
  )
  si_data_fifo_bank 
  (
    .ARESETN                          (interconnect_aresetn_i),
    .ACLK                             (INTERCONNECT_ACLK),
    .S_AXI_AWID                       (sc_sf_awid           ),
    .S_AXI_AWADDR                     (sc_sf_awaddr         ),
    .S_AXI_AWLEN                      (sc_sf_awlen          ),
    .S_AXI_AWSIZE                     (sc_sf_awsize         ),
    .S_AXI_AWBURST                    (sc_sf_awburst        ),
    .S_AXI_AWLOCK                     (sc_sf_awlock         ),
    .S_AXI_AWCACHE                    (sc_sf_awcache        ),
    .S_AXI_AWPROT                     (sc_sf_awprot         ),
    .S_AXI_AWQOS                      (sc_sf_awqos          ),
    .S_AXI_AWUSER                     (sc_sf_awuser         ),
    .S_AXI_AWVALID                    (sc_sf_awvalid        ),
    .S_AXI_AWREADY                    (sc_sf_awready        ),
    .S_AXI_WDATA                      (sc_sf_wdata          ),
    .S_AXI_WSTRB                      (sc_sf_wstrb          ),
    .S_AXI_WLAST                      (sc_sf_wlast          ),
    .S_AXI_WUSER                      (sc_sf_wuser          ),
    .S_AXI_WVALID                     (sc_sf_wvalid         ),
    .S_AXI_WREADY                     (sc_sf_wready         ),
    .S_AXI_BID                        (sc_sf_bid            ),
    .S_AXI_BRESP                      (sc_sf_bresp          ),
    .S_AXI_BUSER                      (sc_sf_buser          ),
    .S_AXI_BVALID                     (sc_sf_bvalid         ),
    .S_AXI_BREADY                     (sc_sf_bready         ),
    .S_AXI_ARID                       (sc_sf_arid           ),
    .S_AXI_ARADDR                     (sc_sf_araddr         ),
    .S_AXI_ARLEN                      (sc_sf_arlen          ),
    .S_AXI_ARSIZE                     (sc_sf_arsize         ),
    .S_AXI_ARBURST                    (sc_sf_arburst        ),
    .S_AXI_ARLOCK                     (sc_sf_arlock         ),
    .S_AXI_ARCACHE                    (sc_sf_arcache        ),
    .S_AXI_ARPROT                     (sc_sf_arprot         ),
    .S_AXI_ARQOS                      (sc_sf_arqos          ),
    .S_AXI_ARUSER                     (sc_sf_aruser         ),
    .S_AXI_ARVALID                    (sc_sf_arvalid        ),
    .S_AXI_ARREADY                    (sc_sf_arready        ),
    .S_AXI_RID                        (sc_sf_rid            ),
    .S_AXI_RDATA                      (sc_sf_rdata          ),
    .S_AXI_RRESP                      (sc_sf_rresp          ),
    .S_AXI_RLAST                      (sc_sf_rlast          ),
    .S_AXI_RUSER                      (sc_sf_ruser          ),
    .S_AXI_RVALID                     (sc_sf_rvalid         ),
    .S_AXI_RREADY                     (sc_sf_rready         ),
    .S_AXI_AWREGION                   ({C_NUM_SLAVE_SLOTS{4'b0}}),  // SI has no REGION ports
    .S_AXI_ARREGION                   ({C_NUM_SLAVE_SLOTS{4'b0}}),  // SI has no REGION ports
    .M_AXI_AWID                       (sf_cb_awid           ),
    .M_AXI_AWADDR                     (sf_cb_awaddr         ),
    .M_AXI_AWLEN                      (sf_cb_awlen          ),
    .M_AXI_AWSIZE                     (sf_cb_awsize         ),
    .M_AXI_AWBURST                    (sf_cb_awburst        ),
    .M_AXI_AWLOCK                     (sf_cb_awlock         ),
    .M_AXI_AWCACHE                    (sf_cb_awcache        ),
    .M_AXI_AWPROT                     (sf_cb_awprot         ),
    .M_AXI_AWREGION                   (                     ),  // SI has no REGION ports
    .M_AXI_AWQOS                      (sf_cb_awqos          ),
    .M_AXI_AWUSER                     (sf_cb_awuser         ),
    .M_AXI_AWVALID                    (sf_cb_awvalid        ),
    .M_AXI_AWREADY                    (sf_cb_awready        ),
    .M_AXI_WDATA                      (sf_cb_wdata          ),
    .M_AXI_WSTRB                      (sf_cb_wstrb          ),
    .M_AXI_WLAST                      (sf_cb_wlast          ),
    .M_AXI_WUSER                      (sf_cb_wuser          ),
    .M_AXI_WVALID                     (sf_cb_wvalid         ),
    .M_AXI_WREADY                     (sf_cb_wready         ),
    .M_AXI_BID                        (sf_cb_bid            ),
    .M_AXI_BRESP                      (sf_cb_bresp          ),
    .M_AXI_BUSER                      (sf_cb_buser          ),
    .M_AXI_BVALID                     (sf_cb_bvalid         ),
    .M_AXI_BREADY                     (sf_cb_bready         ),
    .M_AXI_ARID                       (sf_cb_arid           ),
    .M_AXI_ARADDR                     (sf_cb_araddr         ),
    .M_AXI_ARLEN                      (sf_cb_arlen          ),
    .M_AXI_ARSIZE                     (sf_cb_arsize         ),
    .M_AXI_ARBURST                    (sf_cb_arburst        ),
    .M_AXI_ARLOCK                     (sf_cb_arlock         ),
    .M_AXI_ARCACHE                    (sf_cb_arcache        ),
    .M_AXI_ARPROT                     (sf_cb_arprot         ),
    .M_AXI_ARREGION                   (                     ),  // SI has no REGION ports
    .M_AXI_ARQOS                      (sf_cb_arqos          ),
    .M_AXI_ARUSER                     (sf_cb_aruser         ),
    .M_AXI_ARVALID                    (sf_cb_arvalid        ),
    .M_AXI_ARREADY                    (sf_cb_arready        ),
    .M_AXI_RID                        (sf_cb_rid            ),
    .M_AXI_RDATA                      (sf_cb_rdata          ),
    .M_AXI_RRESP                      (sf_cb_rresp          ),
    .M_AXI_RLAST                      (sf_cb_rlast          ),
    .M_AXI_RUSER                      (sf_cb_ruser          ),
    .M_AXI_RVALID                     (sf_cb_rvalid         ),
    .M_AXI_RREADY                     (sf_cb_rready         )
  );

  ict106_data_fifo_bank #
  (
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLOTS                (C_NUM_MASTER_SLOTS),
    .C_AXI_ID_WIDTH                 (P_M_AXI_ID_WIDTH),
    .C_AXI_ID_MAX_WIDTH                 (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_MAX_WIDTH           (C_AXI_DATA_MAX_WIDTH),
    .C_AXI_DATA_WIDTH               ({C_NUM_MASTER_SLOTS{f_bit32_qual(C_INTERCONNECT_DATA_WIDTH,1,0)}}),
    .C_AXI_SUPPORTS_WRITE           (C_M_AXI_SUPPORTS_WRITE),
    .C_AXI_SUPPORTS_READ            (C_M_AXI_SUPPORTS_READ),
    .C_AXI_SUPPORTS_USER_SIGNALS      (C_AXI_SUPPORTS_USER_SIGNALS),
    .C_AXI_AWUSER_WIDTH               (C_AXI_AWUSER_WIDTH),
    .C_AXI_ARUSER_WIDTH               (C_AXI_ARUSER_WIDTH),
    .C_AXI_WUSER_WIDTH                (C_AXI_WUSER_WIDTH),
    .C_AXI_RUSER_WIDTH                (C_AXI_RUSER_WIDTH),
    .C_AXI_BUSER_WIDTH                (C_AXI_BUSER_WIDTH),
    .C_AXI_WRITE_FIFO_DEPTH         (C_M_AXI_WRITE_FIFO_DEPTH),
    .C_AXI_WRITE_FIFO_TYPE          (C_M_AXI_WRITE_FIFO_TYPE),
    .C_AXI_WRITE_FIFO_DELAY         (C_M_AXI_WRITE_FIFO_DELAY),
    .C_AXI_READ_FIFO_DEPTH          (C_M_AXI_READ_FIFO_DEPTH),
    .C_AXI_READ_FIFO_TYPE           (C_M_AXI_READ_FIFO_TYPE),
    .C_AXI_READ_FIFO_DELAY          (C_M_AXI_READ_FIFO_DELAY)
  )
  mi_data_fifo_bank 
  (
    .ARESETN                          (interconnect_aresetn_i),
    .ACLK                             (INTERCONNECT_ACLK),
    .S_AXI_AWID                       (cb_mf_awid           ),
    .S_AXI_AWADDR                     (cb_mf_awaddr         ),
    .S_AXI_AWLEN                      (cb_mf_awlen          ),
    .S_AXI_AWSIZE                     (cb_mf_awsize         ),
    .S_AXI_AWBURST                    (cb_mf_awburst        ),
    .S_AXI_AWLOCK                     (cb_mf_awlock         ),
    .S_AXI_AWCACHE                    (cb_mf_awcache        ),
    .S_AXI_AWPROT                     (cb_mf_awprot         ),
    .S_AXI_AWREGION                   (cb_mf_awregion       ),
    .S_AXI_AWQOS                      (cb_mf_awqos          ),
    .S_AXI_AWUSER                     (cb_mf_awuser         ),
    .S_AXI_AWVALID                    (cb_mf_awvalid        ),
    .S_AXI_AWREADY                    (cb_mf_awready        ),
    .S_AXI_WDATA                      (cb_mf_wdata          ),
    .S_AXI_WSTRB                      (cb_mf_wstrb          ),
    .S_AXI_WLAST                      (cb_mf_wlast          ),
    .S_AXI_WUSER                      (cb_mf_wuser          ),
    .S_AXI_WVALID                     (cb_mf_wvalid         ),
    .S_AXI_WREADY                     (cb_mf_wready         ),
    .S_AXI_BID                        (cb_mf_bid            ),
    .S_AXI_BRESP                      (cb_mf_bresp          ),
    .S_AXI_BUSER                      (cb_mf_buser          ),
    .S_AXI_BVALID                     (cb_mf_bvalid         ),
    .S_AXI_BREADY                     (cb_mf_bready         ),
    .S_AXI_ARID                       (cb_mf_arid           ),
    .S_AXI_ARADDR                     (cb_mf_araddr         ),
    .S_AXI_ARLEN                      (cb_mf_arlen          ),
    .S_AXI_ARSIZE                     (cb_mf_arsize         ),
    .S_AXI_ARBURST                    (cb_mf_arburst        ),
    .S_AXI_ARLOCK                     (cb_mf_arlock         ),
    .S_AXI_ARCACHE                    (cb_mf_arcache        ),
    .S_AXI_ARPROT                     (cb_mf_arprot         ),
    .S_AXI_ARREGION                   (cb_mf_arregion       ),
    .S_AXI_ARQOS                      (cb_mf_arqos          ),
    .S_AXI_ARUSER                     (cb_mf_aruser         ),
    .S_AXI_ARVALID                    (cb_mf_arvalid        ),
    .S_AXI_ARREADY                    (cb_mf_arready        ),
    .S_AXI_RID                        (cb_mf_rid            ),
    .S_AXI_RDATA                      (cb_mf_rdata          ),
    .S_AXI_RRESP                      (cb_mf_rresp          ),
    .S_AXI_RLAST                      (cb_mf_rlast          ),
    .S_AXI_RUSER                      (cb_mf_ruser          ),
    .S_AXI_RVALID                     (cb_mf_rvalid         ),
    .S_AXI_RREADY                     (cb_mf_rready         ),
    .M_AXI_AWID                       (mf_mc_awid           ),
    .M_AXI_AWADDR                     (mf_mc_awaddr         ),
    .M_AXI_AWLEN                      (mf_mc_awlen          ),
    .M_AXI_AWSIZE                     (mf_mc_awsize         ),
    .M_AXI_AWBURST                    (mf_mc_awburst        ),
    .M_AXI_AWLOCK                     (mf_mc_awlock         ),
    .M_AXI_AWCACHE                    (mf_mc_awcache        ),
    .M_AXI_AWPROT                     (mf_mc_awprot         ),
    .M_AXI_AWREGION                   (mf_mc_awregion       ),
    .M_AXI_AWQOS                      (mf_mc_awqos          ),
    .M_AXI_AWUSER                     (mf_mc_awuser         ),
    .M_AXI_AWVALID                    (mf_mc_awvalid        ),
    .M_AXI_AWREADY                    (mf_mc_awready        ),
    .M_AXI_WDATA                      (mf_mc_wdata          ),
    .M_AXI_WSTRB                      (mf_mc_wstrb          ),
    .M_AXI_WLAST                      (mf_mc_wlast          ),
    .M_AXI_WUSER                      (mf_mc_wuser          ),
    .M_AXI_WVALID                     (mf_mc_wvalid         ),
    .M_AXI_WREADY                     (mf_mc_wready         ),
    .M_AXI_BID                        (mf_mc_bid            ),
    .M_AXI_BRESP                      (mf_mc_bresp          ),
    .M_AXI_BUSER                      (mf_mc_buser          ),
    .M_AXI_BVALID                     (mf_mc_bvalid         ),
    .M_AXI_BREADY                     (mf_mc_bready         ),
    .M_AXI_ARID                       (mf_mc_arid           ),
    .M_AXI_ARADDR                     (mf_mc_araddr         ),
    .M_AXI_ARLEN                      (mf_mc_arlen          ),
    .M_AXI_ARSIZE                     (mf_mc_arsize         ),
    .M_AXI_ARBURST                    (mf_mc_arburst        ),
    .M_AXI_ARLOCK                     (mf_mc_arlock         ),
    .M_AXI_ARCACHE                    (mf_mc_arcache        ),
    .M_AXI_ARPROT                     (mf_mc_arprot         ),
    .M_AXI_ARREGION                   (mf_mc_arregion       ),
    .M_AXI_ARQOS                      (mf_mc_arqos          ),
    .M_AXI_ARUSER                     (mf_mc_aruser         ),
    .M_AXI_ARVALID                    (mf_mc_arvalid        ),
    .M_AXI_ARREADY                    (mf_mc_arready        ),
    .M_AXI_RID                        (mf_mc_rid            ),
    .M_AXI_RDATA                      (mf_mc_rdata          ),
    .M_AXI_RRESP                      (mf_mc_rresp          ),
    .M_AXI_RLAST                      (mf_mc_rlast          ),
    .M_AXI_RUSER                      (mf_mc_ruser          ),
    .M_AXI_RVALID                     (mf_mc_rvalid         ),
    .M_AXI_RREADY                     (mf_mc_rready         )
  );



  ict106_axi_crossbar #(
    .C_MAX_S                          (`P_MAX_S),
    .C_MAX_M                          (`P_MAX_M),
    .C_NUM_ADDR_RANGES                (`P_NUM_ADDR_RANGES),
    .C_INTERCONNECT_CONNECTIVITY_MODE (C_INTERCONNECT_CONNECTIVITY_MODE),
    .C_FAMILY                         (C_BASEFAMILY),
    .C_NUM_SLAVE_SLOTS                (C_NUM_SLAVE_SLOTS),
    .C_NUM_MASTER_SLOTS               (C_NUM_MASTER_SLOTS),
    .C_AXI_ID_WIDTH                   (C_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_INTERCONNECT_DATA_WIDTH        (C_INTERCONNECT_DATA_WIDTH),
    .C_AXI_DATA_MAX_WIDTH             (C_AXI_DATA_MAX_WIDTH),
    .C_M_AXI_DATA_WIDTH               (C_M_AXI_DATA_WIDTH),  // Used to determine whether W-channel gets reg-slice
    .C_S_AXI_PROTOCOL                 (C_S_AXI_PROTOCOL),
    .C_M_AXI_PROTOCOL                 (C_M_AXI_PROTOCOL),
    .C_M_AXI_BASE_ADDR                (C_M_AXI_BASE_ADDR),
    .C_M_AXI_HIGH_ADDR                (C_M_AXI_HIGH_ADDR),
    .C_S_AXI_BASE_ID                  (P_S_AXI_BASE_ID),
    .C_S_AXI_HIGH_ID                  (P_S_AXI_HIGH_ID),
    .C_S_AXI_THREAD_ID_WIDTH          (C_S_AXI_THREAD_ID_WIDTH),
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
    .C_RANGE_CHECK                    (P_RANGE_CHECK),
    .C_ADDR_DECODE                    (P_ADDR_DECODE),
    .C_INTERCONNECT_R_REGISTER        (C_INTERCONNECT_R_REGISTER),
    .C_S_AXI_CTRL_ADDR_WIDTH          (C_S_AXI_CTRL_ADDR_WIDTH),
    .C_S_AXI_CTRL_DATA_WIDTH          (C_S_AXI_CTRL_DATA_WIDTH),
    .C_W_ISSUE_WIDTH                  (f_write_issue_width_vec(0)),
    .C_R_ISSUE_WIDTH                  (f_read_issue_width_vec(0)),
    .C_W_ACCEPT_WIDTH                 (f_write_accept_width_vec(0)),
    .C_R_ACCEPT_WIDTH                 (f_read_accept_width_vec(0)),
    .C_DEBUG                          (C_DEBUG),
    .C_MAX_DEBUG_THREADS              (C_MAX_DEBUG_THREADS)
  ) crossbar_samd (
    .INTERCONNECT_ACLK                (INTERCONNECT_ACLK),
    .ARESETN                          (interconnect_aresetn_i),
    .IRQ                              (IRQ),
    .S_AXI_AWID                       (sf_cb_awid           ),
    .S_AXI_AWADDR                     (sf_cb_awaddr         ),
    .S_AXI_AWLEN                      (sf_cb_awlen          ),
    .S_AXI_AWSIZE                     (sf_cb_awsize         ),
    .S_AXI_AWBURST                    (sf_cb_awburst        ),
    .S_AXI_AWLOCK                     (sf_cb_awlock         ),
    .S_AXI_AWCACHE                    (sf_cb_awcache        ),
    .S_AXI_AWPROT                     (sf_cb_awprot         ),
    .S_AXI_AWQOS                      (sf_cb_awqos          ),
    .S_AXI_AWUSER                     (sf_cb_awuser         ),
    .S_AXI_AWVALID                    (sf_cb_awvalid        ),
    .S_AXI_AWREADY                    (sf_cb_awready        ),
    .S_AXI_WDATA                      (sf_cb_wdata          ),
    .S_AXI_WSTRB                      (sf_cb_wstrb          ),
    .S_AXI_WLAST                      (sf_cb_wlast          ),
    .S_AXI_WUSER                      (sf_cb_wuser          ),
    .S_AXI_WVALID                     (sf_cb_wvalid         ),
    .S_AXI_WREADY                     (sf_cb_wready         ),
    .S_AXI_BID                        (sf_cb_bid            ),
    .S_AXI_BRESP                      (sf_cb_bresp          ),
    .S_AXI_BUSER                      (sf_cb_buser          ),
    .S_AXI_BVALID                     (sf_cb_bvalid         ),
    .S_AXI_BREADY                     (sf_cb_bready         ),
    .S_AXI_ARID                       (sf_cb_arid           ),
    .S_AXI_ARADDR                     (sf_cb_araddr         ),
    .S_AXI_ARLEN                      (sf_cb_arlen          ),
    .S_AXI_ARSIZE                     (sf_cb_arsize         ),
    .S_AXI_ARBURST                    (sf_cb_arburst        ),
    .S_AXI_ARLOCK                     (sf_cb_arlock         ),
    .S_AXI_ARCACHE                    (sf_cb_arcache        ),
    .S_AXI_ARPROT                     (sf_cb_arprot         ),
    .S_AXI_ARQOS                      (sf_cb_arqos          ),
    .S_AXI_ARUSER                     (sf_cb_aruser         ),
    .S_AXI_ARVALID                    (sf_cb_arvalid        ),
    .S_AXI_ARREADY                    (sf_cb_arready        ),
    .S_AXI_RID                        (sf_cb_rid            ),
    .S_AXI_RDATA                      (sf_cb_rdata          ),
    .S_AXI_RRESP                      (sf_cb_rresp          ),
    .S_AXI_RLAST                      (sf_cb_rlast          ),
    .S_AXI_RUSER                      (sf_cb_ruser          ),
    .S_AXI_RVALID                     (sf_cb_rvalid         ),
    .S_AXI_RREADY                     (sf_cb_rready         ),
    .M_AXI_AWID                       (cb_mf_awid           ),
    .M_AXI_AWADDR                     (cb_mf_awaddr         ),
    .M_AXI_AWLEN                      (cb_mf_awlen          ),
    .M_AXI_AWSIZE                     (cb_mf_awsize         ),
    .M_AXI_AWBURST                    (cb_mf_awburst        ),
    .M_AXI_AWLOCK                     (cb_mf_awlock         ),
    .M_AXI_AWCACHE                    (cb_mf_awcache        ),
    .M_AXI_AWPROT                     (cb_mf_awprot         ),
    .M_AXI_AWREGION                   (cb_mf_awregion       ),
    .M_AXI_AWQOS                      (cb_mf_awqos          ),
    .M_AXI_AWUSER                     (cb_mf_awuser         ),
    .M_AXI_AWVALID                    (cb_mf_awvalid        ),
    .M_AXI_AWREADY                    (cb_mf_awready        ),
    .M_AXI_WDATA                      (cb_mf_wdata          ),
    .M_AXI_WSTRB                      (cb_mf_wstrb          ),
    .M_AXI_WLAST                      (cb_mf_wlast          ),
    .M_AXI_WUSER                      (cb_mf_wuser          ),
    .M_AXI_WVALID                     (cb_mf_wvalid         ),
    .M_AXI_WREADY                     (cb_mf_wready         ),
    .M_AXI_BID                        (cb_mf_bid            ),
    .M_AXI_BRESP                      (cb_mf_bresp          ),
    .M_AXI_BUSER                      (cb_mf_buser          ),
    .M_AXI_BVALID                     (cb_mf_bvalid         ),
    .M_AXI_BREADY                     (cb_mf_bready         ),
    .M_AXI_ARID                       (cb_mf_arid           ),
    .M_AXI_ARADDR                     (cb_mf_araddr         ),
    .M_AXI_ARLEN                      (cb_mf_arlen          ),
    .M_AXI_ARSIZE                     (cb_mf_arsize         ),
    .M_AXI_ARBURST                    (cb_mf_arburst        ),
    .M_AXI_ARLOCK                     (cb_mf_arlock         ),
    .M_AXI_ARCACHE                    (cb_mf_arcache        ),
    .M_AXI_ARPROT                     (cb_mf_arprot         ),
    .M_AXI_ARREGION                   (cb_mf_arregion       ),
    .M_AXI_ARQOS                      (cb_mf_arqos          ),
    .M_AXI_ARUSER                     (cb_mf_aruser         ),
    .M_AXI_ARVALID                    (cb_mf_arvalid        ),
    .M_AXI_ARREADY                    (cb_mf_arready        ),
    .M_AXI_RID                        (cb_mf_rid            ),
    .M_AXI_RDATA                      (cb_mf_rdata          ),
    .M_AXI_RRESP                      (cb_mf_rresp          ),
    .M_AXI_RLAST                      (cb_mf_rlast          ),
    .M_AXI_RUSER                      (cb_mf_ruser          ),
    .M_AXI_RVALID                     (cb_mf_rvalid         ),
    .M_AXI_RREADY                     (cb_mf_rready         ),
    .S_AXI_CTRL_AWADDR                (cb_ctrl_awaddr       ),
    .S_AXI_CTRL_AWVALID               (cb_ctrl_awvalid      ),
    .S_AXI_CTRL_AWREADY               (cb_ctrl_awready      ),
    .S_AXI_CTRL_WDATA                 (cb_ctrl_wdata        ),
    .S_AXI_CTRL_WVALID                (cb_ctrl_wvalid       ),
    .S_AXI_CTRL_WREADY                (cb_ctrl_wready       ),
    .S_AXI_CTRL_BRESP                 (cb_ctrl_bresp        ),
    .S_AXI_CTRL_BVALID                (cb_ctrl_bvalid       ),
    .S_AXI_CTRL_BREADY                (cb_ctrl_bready       ),
    .S_AXI_CTRL_ARADDR                (cb_ctrl_araddr       ),
    .S_AXI_CTRL_ARVALID               (cb_ctrl_arvalid      ),
    .S_AXI_CTRL_ARREADY               (cb_ctrl_arready      ),
    .S_AXI_CTRL_RDATA                 (cb_ctrl_rdata        ),
    .S_AXI_CTRL_RRESP                 (cb_ctrl_rresp        ),
    .S_AXI_CTRL_RVALID                (cb_ctrl_rvalid       ),
    .S_AXI_CTRL_RREADY                (cb_ctrl_rready       ),
    .DEBUG_AW_TRANS_SEQ               (aw_trans_seq         ),
    .DEBUG_AW_TRANS_QUAL              (aw_trans_qual        ),
    .DEBUG_AW_ACCEPT_CNT              (aw_accept_cnt        ),
    .DEBUG_AW_ACTIVE_THREAD           (aw_active_thread     ),
    .DEBUG_AW_ACTIVE_TARGET           (aw_active_target     ),
    .DEBUG_AW_ACTIVE_REGION           (aw_active_region     ),
    .DEBUG_AW_ERROR                   (aw_error             ),
    .DEBUG_AW_TARGET                  (aw_target            ),
    .DEBUG_AW_ISSUING_CNT             (aw_issuing_cnt       ),
    .DEBUG_AW_ARB_GRANT               (aw_arb_grant         ),
    .DEBUG_B_TRANS_SEQ                (b_trans_seq          ),
    .DEBUG_AR_TRANS_SEQ               (ar_trans_seq         ),
    .DEBUG_AR_TRANS_QUAL              (ar_trans_qual        ),
    .DEBUG_AR_ACCEPT_CNT              (ar_accept_cnt        ),
    .DEBUG_AR_ACTIVE_THREAD           (ar_active_thread     ),
    .DEBUG_AR_ACTIVE_TARGET           (ar_active_target     ),
    .DEBUG_AR_ACTIVE_REGION           (ar_active_region     ),
    .DEBUG_AR_ERROR                   (ar_error             ),
    .DEBUG_AR_TARGET                  (ar_target            ),
    .DEBUG_AR_ISSUING_CNT             (ar_issuing_cnt       ),
    .DEBUG_AR_ARB_GRANT               (ar_arb_grant         ),
    .DEBUG_R_BEAT_CNT                 (r_beat_cnt           ),
    .DEBUG_R_TRANS_SEQ                (r_trans_seq          ),
    .DEBUG_RID_TARGET                 (rid_target           ),
    .DEBUG_RID_ERROR                  (rid_error            ),
    .DEBUG_BID_TARGET                 (bid_target           ),
    .DEBUG_BID_ERROR                  (bid_error            ),
    .DEBUG_W_BEAT_CNT                 (w_beat_cnt           ),
    .DEBUG_W_TRANS_SEQ                (w_trans_seq          )
  );
    
  assign cb_ctrl_awaddr      = S_AXI_CTRL_AWADDR     ;
  assign cb_ctrl_awvalid     = S_AXI_CTRL_AWVALID    ;
  assign cb_ctrl_wdata       = S_AXI_CTRL_WDATA      ;
  assign cb_ctrl_wvalid      = S_AXI_CTRL_WVALID     ;
  assign cb_ctrl_bready      = S_AXI_CTRL_BREADY     ;
  assign cb_ctrl_araddr      = S_AXI_CTRL_ARADDR     ;
  assign cb_ctrl_arvalid     = S_AXI_CTRL_ARVALID    ;
  assign cb_ctrl_rready      = S_AXI_CTRL_RREADY     ;
  assign S_AXI_CTRL_AWREADY  = cb_ctrl_awready       ;
  assign S_AXI_CTRL_WREADY   = cb_ctrl_wready        ;
  assign S_AXI_CTRL_BRESP    = cb_ctrl_bresp         ;
  assign S_AXI_CTRL_BVALID   = cb_ctrl_bvalid        ;
  assign S_AXI_CTRL_ARREADY  = cb_ctrl_arready       ;
  assign S_AXI_CTRL_RDATA    = cb_ctrl_rdata         ;
  assign S_AXI_CTRL_RRESP    = cb_ctrl_rresp         ;
  assign S_AXI_CTRL_RVALID   = cb_ctrl_rvalid        ;

endmodule

`default_nettype wire
