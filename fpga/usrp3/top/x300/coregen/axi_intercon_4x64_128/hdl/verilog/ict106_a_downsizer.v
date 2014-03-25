// -- (c) Copyright 2010 - 2011 Xilinx, Inc. All rights reserved.
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
// Description: Address Down-Sizer
//
//
// Verilog-standard:  Verilog 2001
//--------------------------------------------------------------------------
//
// Structure:
//   a_downsizer
//     axic_fifo
//       fifo_gen
//         fifo_coregen
//
//--------------------------------------------------------------------------
`timescale 1ps/1ps

module ict106_a_downsizer #
  (
   parameter         C_FAMILY                         = "none", 
                       // FPGA Family. Current version: virtex6 or spartan6.
   parameter integer C_AXI_ID_WIDTH                   = 4, 
                       // Width of all ID signals on SI and MI side of converter.
                       // Range: >= 1.
   parameter integer C_AXI_ADDR_WIDTH                 = 32, 
                       // Width of all ADDR signals on SI and MI side of converter.
                       // Range: 32.
   parameter         C_S_AXI_DATA_WIDTH               = 32'h00000020,
                       // Width of S_AXI_WDATA and S_AXI_RDATA.
                       // Format: Bit32;
                       // Range: 'h00000040, 'h00000080, 'h00000100.
   parameter         C_M_AXI_DATA_WIDTH               = 32'h00000020, 
                       // Width of M_AXI_WDATA and M_AXI_RDATA. 
                       // Assume always smaller than C_S_AXI_DATA_WIDTH.
                       // Format: Bit32; 
                       // Range: 'h00000020, 'h00000040, 'h00000080.
   parameter integer C_AXI_SUPPORTS_USER_SIGNALS      = 0,
                       // 1 = Propagate all USER signals, 0 = Dont propagate.
   parameter integer C_AXI_AUSER_WIDTH                = 1,
                       // Width of AWUSER signals. 
                       // Range: >= 1.
   parameter integer C_AXI_CHANNEL                    = 0,
                       // 0 = AXI AW Channel.
                       // 1 = AXI AR Channel.
   parameter integer C_SUPPORT_SPLITTING              = 1,
                       // Implement transaction splitting logic.
   parameter integer C_SUPPORT_BURSTS                 = 1,
                       // Disabled when all connected slaves and masters are AxiLite,
                       //   allowing logic to be simplified.
   parameter integer C_SINGLE_THREAD                  = 1,
                       // Allow multiple outstanding transactions only if the IDs are the same.
   parameter integer C_S_AXI_BYTES_LOG                = 3,
                       // Log2 of number of 32bit word on SI-side.
   parameter integer C_M_AXI_BYTES_LOG                = 2,
                       // Log2 of number of 32bit word on MI-side.
   parameter integer C_RATIO_LOG                      = 1
                       // Log2 of Up-Sizing ratio for data.
   )
  (
   // Global Signals
   input  wire                                                    ARESET,
   input  wire                                                    ACLK,

   // Command Interface (W/R)
   output wire                              cmd_valid,
   output wire                              cmd_split,
   output wire                              cmd_mirror,
   output wire                              cmd_fix,
   output wire [C_S_AXI_BYTES_LOG-1:0]      cmd_first_word, 
   output wire [C_S_AXI_BYTES_LOG-1:0]      cmd_offset,
   output wire [C_S_AXI_BYTES_LOG-1:0]      cmd_mask,
   output wire [C_M_AXI_BYTES_LOG:0]        cmd_step,
   output wire [3-1:0]                      cmd_size,
   output wire [8-1:0]                      cmd_length,
   input  wire                              cmd_ready,
   
   // Command Interface (B)
   output wire                              cmd_b_valid,
   output wire                              cmd_b_split,
   output wire [4-1:0]                      cmd_b_repeat,
   input  wire                              cmd_b_ready,
   
   // Slave Interface Write Address Ports
   input  wire [C_AXI_ID_WIDTH-1:0]          S_AXI_AID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S_AXI_AADDR,
   input  wire [8-1:0]                         S_AXI_ALEN,
   input  wire [3-1:0]                         S_AXI_ASIZE,
   input  wire [2-1:0]                         S_AXI_ABURST,
   input  wire [2-1:0]                         S_AXI_ALOCK,
   input  wire [4-1:0]                         S_AXI_ACACHE,
   input  wire [3-1:0]                         S_AXI_APROT,
   input  wire [4-1:0]                         S_AXI_AREGION,
   input  wire [4-1:0]                         S_AXI_AQOS,
   input  wire [C_AXI_AUSER_WIDTH-1:0]         S_AXI_AUSER,
   input  wire                                                   S_AXI_AVALID,
   output wire                                                   S_AXI_AREADY,

   // Master Interface Write Address Port
   output wire [C_AXI_ID_WIDTH-1:0]            M_AXI_AID,
   output wire [C_AXI_ADDR_WIDTH-1:0]          M_AXI_AADDR,
   output wire [8-1:0]                         M_AXI_ALEN,
   output wire [3-1:0]                         M_AXI_ASIZE,
   output wire [2-1:0]                         M_AXI_ABURST,
   output wire [2-1:0]                         M_AXI_ALOCK,
   output wire [4-1:0]                         M_AXI_ACACHE,
   output wire [3-1:0]                         M_AXI_APROT,
   output wire [4-1:0]                         M_AXI_AREGION,
   output wire [4-1:0]                         M_AXI_AQOS,
   output wire [C_AXI_AUSER_WIDTH-1:0]         M_AXI_AUSER,
   output wire                                                   M_AXI_AVALID,
   input  wire                                                   M_AXI_AREADY
   );

   
  /////////////////////////////////////////////////////////////////////////////
  // Variables for generating parameter controlled instances.
  /////////////////////////////////////////////////////////////////////////////
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Local params
  /////////////////////////////////////////////////////////////////////////////
  
  // Decode the native transaction size on the SI-side interface.
  localparam [3-1:0] C_S_AXI_NATIVE_SIZE = (C_S_AXI_DATA_WIDTH == 1024) ? 3'b111 :
                                           (C_S_AXI_DATA_WIDTH ==  512) ? 3'b110 :
                                           (C_S_AXI_DATA_WIDTH ==  256) ? 3'b101 :
                                           (C_S_AXI_DATA_WIDTH ==  128) ? 3'b100 :
                                           (C_S_AXI_DATA_WIDTH ==   64) ? 3'b011 :
                                           (C_S_AXI_DATA_WIDTH ==   32) ? 3'b010 :
                                           (C_S_AXI_DATA_WIDTH ==   16) ? 3'b001 :
                                           3'b000;
  
  // Decode the native transaction size on the MI-side interface.
  localparam [3-1:0] C_M_AXI_NATIVE_SIZE = (C_M_AXI_DATA_WIDTH == 1024) ? 3'b111 :
                                           (C_M_AXI_DATA_WIDTH ==  512) ? 3'b110 :
                                           (C_M_AXI_DATA_WIDTH ==  256) ? 3'b101 :
                                           (C_M_AXI_DATA_WIDTH ==  128) ? 3'b100 :
                                           (C_M_AXI_DATA_WIDTH ==   64) ? 3'b011 :
                                           (C_M_AXI_DATA_WIDTH ==   32) ? 3'b010 :
                                           (C_M_AXI_DATA_WIDTH ==   16) ? 3'b001 :
                                           3'b000;
  
  // Help constant to generate mask signals.
  localparam [C_AXI_ADDR_WIDTH+8-1:0]      C_DOUBLE_LEN = {{C_AXI_ADDR_WIDTH{1'b0}}, 8'b1111_1111};
  
  // Constants for burst types.
  localparam [2-1:0] C_FIX_BURST         = 2'b00;
  localparam [2-1:0] C_INCR_BURST        = 2'b01;
  localparam [2-1:0] C_WRAP_BURST        = 2'b10;
  
  // Depth for command FIFO.
  localparam integer C_FIFO_DEPTH_LOG    = 5;
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Functions
  /////////////////////////////////////////////////////////////////////////////
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Internal signals
  /////////////////////////////////////////////////////////////////////////////
  
  // Access decoding related signals.
  wire                                access_is_fix;
  wire                                access_is_incr;
  wire                                access_is_wrap;
  wire [C_AXI_ADDR_WIDTH+16-1:0]      alen_help_vector;
  reg  [C_S_AXI_BYTES_LOG-1:0]        size_mask;
  reg  [C_AXI_ADDR_WIDTH-1:0]         split_addr_mask;
  reg  [C_S_AXI_BYTES_LOG+8-1:0]      full_downsized_len;
  wire [8-1:0]                        downsized_len;
  reg                                 legal_wrap_len;
  reg  [8-1:0]                        fix_len;
  reg  [8-1:0]                        unalignment_addr;
  reg  [C_AXI_ADDR_WIDTH-1:0]         burst_mask;
  wire [C_AXI_ADDR_WIDTH-1:0]         masked_addr;
  wire [C_AXI_ADDR_WIDTH-1:0]         burst_unalignment;
  wire [8-1:0]                        wrap_unaligned_len;
  reg  [8-1:0]                        wrap_rest_len;
  wire [C_S_AXI_BYTES_LOG-1:0]        num_transactions;
  wire                                access_fit_mi_side;
  wire                                si_full_size;
  wire                                fix_need_to_split;
  wire                                incr_need_to_split;
  wire                                wrap_need_to_split;
  wire [C_AXI_ADDR_WIDTH-1:0]         pre_mi_addr;
  reg  [C_AXI_ADDR_WIDTH-1:0]         next_mi_addr;
  reg                                 split_ongoing;
  reg  [4-1:0]                        pushed_commands;
  wire                                need_to_split;
  
  // Access decoding related signals for internal pipestage.
  reg                                 access_is_fix_q;
  reg                                 access_is_incr_q;
  reg                                 access_is_wrap_q;
  reg                                 access_fit_mi_side_q;
  reg                                 legal_wrap_len_q;
  reg                                 si_full_size_q;
  reg                                 fix_need_to_split_q;
  reg                                 incr_need_to_split_q;
  reg                                 wrap_need_to_split_q;
  wire                                need_to_split_q;
  reg  [C_AXI_ADDR_WIDTH-1:0]         split_addr_mask_q;
  reg  [C_S_AXI_BYTES_LOG-1:0]        num_transactions_q;
  reg  [8-1:0]                        wrap_unaligned_len_q;
  reg  [C_S_AXI_BYTES_LOG-1:0]        size_mask_q;
  reg  [8-1:0]                        downsized_len_q;
  reg  [8-1:0]                        fix_len_q;
  reg  [8-1:0]                        unalignment_addr_q;
  reg  [C_AXI_ADDR_WIDTH-1:0]         masked_addr_q;
  
  // Command buffer help signals.
  reg  [C_FIFO_DEPTH_LOG:0]           cmd_depth;
  reg                                 cmd_empty;
  reg  [C_AXI_ID_WIDTH-1:0]           queue_id;
  wire                                id_match;
  wire                                cmd_id_check;
  wire                                cmd_id_check_empty;
  wire                                s_ready;
  wire                                cmd_full;
  wire                                allow_new_cmd;
  wire                                cmd_push;
  reg                                 cmd_push_block;
  reg  [C_FIFO_DEPTH_LOG:0]           cmd_b_depth;
  reg                                 cmd_b_empty_i;
  wire                                cmd_b_empty;
  wire                                cmd_b_full;
  wire                                cmd_b_push;
  reg                                 cmd_b_push_block;
  wire                                pushed_new_cmd;
  wire                                last_fix_split;
  wire                                last_incr_split;
  wire                                last_wrap_split;
  wire                                last_split;
  
  // Internal Command Interface signals (W/R).
  wire                                cmd_valid_i;
  wire                                cmd_fix_i;
  wire                                cmd_split_i;
  wire                                cmd_mirror_i;
  reg  [C_S_AXI_BYTES_LOG-1:0]        cmd_first_word_ii;
  wire [C_S_AXI_BYTES_LOG-1:0]        cmd_first_word_i;
  wire [C_S_AXI_BYTES_LOG-1:0]        cmd_offset_i;
  reg  [C_S_AXI_BYTES_LOG-1:0]        cmd_mask_i;
  reg  [C_S_AXI_BYTES_LOG-1:0]        cmd_mask_q;
  reg  [3-1:0]                        cmd_size_i;
  wire [3-1:0]                        cmd_size_ii;
  reg  [7-1:0]                        cmd_step_i;
  wire [8-1:0]                        cmd_length_i;
  reg  [8-1:0]                        base_len;
  reg  [8-1:0]                        compensation_len;
  
  // Internal Command Interface signals (B).
  wire                                cmd_b_split_i;
  reg  [4-1:0]                        cmd_b_repeat_i;
  
  // Throttling help signals.
  wire                                mi_stalling;
  reg                                 command_ongoing;
   
  // Internal SI-side signals.
  reg  [C_AXI_ID_WIDTH-1:0]           S_AXI_AID_Q;
  reg  [C_AXI_ADDR_WIDTH-1:0]         S_AXI_AADDR_Q;
  reg  [8-1:0]                        S_AXI_ALEN_Q;
  reg  [3-1:0]                        S_AXI_ASIZE_Q;
  reg  [2-1:0]                        S_AXI_ABURST_Q;
  reg  [2-1:0]                        S_AXI_ALOCK_Q;
  reg  [4-1:0]                        S_AXI_ACACHE_Q;
  reg  [3-1:0]                        S_AXI_APROT_Q;
  reg  [4-1:0]                        S_AXI_AREGION_Q;
  reg  [4-1:0]                        S_AXI_AQOS_Q;
  reg  [C_AXI_AUSER_WIDTH-1:0]        S_AXI_AUSER_Q;
  reg                                 S_AXI_AREADY_I;
  
  // Internal MI-side signals.
  wire [C_AXI_ID_WIDTH-1:0]           M_AXI_AID_I;
  reg  [C_AXI_ADDR_WIDTH-1:0]         M_AXI_AADDR_I;
  wire [8-1:0]                        M_AXI_ALEN_I;
  reg  [3-1:0]                        M_AXI_ASIZE_I;
  reg  [2-1:0]                        M_AXI_ABURST_I;
  reg  [2-1:0]                        M_AXI_ALOCK_I;
  wire [4-1:0]                        M_AXI_ACACHE_I;
  wire [3-1:0]                        M_AXI_APROT_I;
  wire [4-1:0]                        M_AXI_AREGION_I;
  wire [4-1:0]                        M_AXI_AQOS_I;
  wire [C_AXI_AUSER_WIDTH-1:0]        M_AXI_AUSER_I;
  wire                                M_AXI_AVALID_I;
  wire                                M_AXI_AREADY_I;
  
  reg [1:0] areset_d; // Reset delay register
  always @(posedge ACLK) begin
    areset_d <= {areset_d[0], ARESET};
  end
  
  /////////////////////////////////////////////////////////////////////////////
  // Capture SI-Side signals.
  //
  /////////////////////////////////////////////////////////////////////////////
  
  // Register SI-Side signals.
  always @ (posedge ACLK) begin
    if ( S_AXI_AREADY_I ) begin
      S_AXI_AID_Q     <= S_AXI_AID;
      S_AXI_AADDR_Q   <= S_AXI_AADDR;
      S_AXI_ALEN_Q    <= S_AXI_ALEN;
      S_AXI_ASIZE_Q   <= S_AXI_ASIZE;
      S_AXI_ABURST_Q  <= S_AXI_ABURST;
      S_AXI_ALOCK_Q   <= S_AXI_ALOCK;
      S_AXI_ACACHE_Q  <= S_AXI_ACACHE;
      S_AXI_APROT_Q   <= S_AXI_APROT;
      S_AXI_AREGION_Q <= S_AXI_AREGION;
      S_AXI_AQOS_Q    <= S_AXI_AQOS;
      S_AXI_AUSER_Q   <= S_AXI_AUSER;
    end
  end
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Transfer SI-Side signals to internal Pipeline Stage.
  //
  /////////////////////////////////////////////////////////////////////////////
  always @ (posedge ACLK) begin
    if ( ARESET ) begin
      access_is_fix_q       <= 1'b0;
      access_is_incr_q      <= 1'b0;
      access_is_wrap_q      <= 1'b0;
      access_fit_mi_side_q  <= 1'b0;
      legal_wrap_len_q      <= 1'b0;
      si_full_size_q        <= 1'b0;
      fix_need_to_split_q   <= 1'b0;
      incr_need_to_split_q  <= 1'b0;
      wrap_need_to_split_q  <= 1'b0;
      split_addr_mask_q     <= {C_AXI_ADDR_WIDTH{1'b0}};
      num_transactions_q    <= {C_S_AXI_BYTES_LOG{1'b0}};
      wrap_unaligned_len_q  <= 8'b0;
      cmd_mask_q            <= {C_S_AXI_BYTES_LOG{1'b0}};
      size_mask_q           <= {C_S_AXI_BYTES_LOG{1'b0}};
      downsized_len_q       <= 8'b0;
      fix_len_q             <= 8'b0;
      unalignment_addr_q    <= 8'b0;
      masked_addr_q         <= {C_AXI_ADDR_WIDTH{1'b0}};
    end else begin
      if ( S_AXI_AREADY_I ) begin
        access_is_fix_q       <= access_is_fix;
        access_is_incr_q      <= access_is_incr;
        access_is_wrap_q      <= access_is_wrap;
        access_fit_mi_side_q  <= access_fit_mi_side;
        legal_wrap_len_q      <= legal_wrap_len;
        si_full_size_q        <= si_full_size;
        fix_need_to_split_q   <= fix_need_to_split;
        incr_need_to_split_q  <= incr_need_to_split;
        wrap_need_to_split_q  <= wrap_need_to_split;
        split_addr_mask_q     <= split_addr_mask;
        num_transactions_q    <= num_transactions;
        wrap_unaligned_len_q  <= wrap_unaligned_len;
        cmd_mask_q            <= cmd_mask_i;
        size_mask_q           <= size_mask;
        downsized_len_q       <= downsized_len;
        fix_len_q             <= fix_len;
        unalignment_addr_q    <= unalignment_addr;
        masked_addr_q         <= masked_addr;
      end
    end
  end
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Decode the Incoming Transaction.
  // 
  // Decode transaction type and various mask and length data.
  // 
  // Generate mask for address bits that are used unused on SI-side to remove
  // sub transaction size unalignment from data multiplex control signals.
  // 
  // Generate mask to keep addressbit between SI-side width and width of 
  // current transaction, i.e. when SI-side > AxSIZE > MI-side. Mask is used 
  // to make sure that split INCR transactions are continued at the correct 
  // offset after split.
  // 
  // Generate mask for subsize WRAP that fit in MI-Side to make sure offset and
  // sub-size wrap position is correct. For example 8-bit in a 32-bit or wider
  // MI-Side.
  // 
  // Calculate MI-Side downsize length. Downsized length is equal to SI-Side
  // length for sub-sized transactions, i.e. AxSIZE <= MI-Side width. The 8 
  // least significant bits are used to determine the length of a transaction 
  // that doesn't need to be split and the length of the last transaction if it 
  // has been split. 
  // The most significant bits are used to determine the number of transactions
  // that has to be generated on the MI-Side.
  // 
  // Evaluate if current SI-Side WRAP length is a legal length on the MI-Side.
  // 
  // Calculate the length of a FIX transaction that needs to be translated into
  // a INCR on the MI-Side. I.e. for transactions with AxSIZE > MI-Side width.
  // 
  // Determine unalignment offset depending on transaction size so that it can 
  // be discarded on the MI-side. I.e. Unused data is removed to save cycles.
  // 
  // Calculate the address bits on the MI-Side that is covered by this burst 
  // transaction. This mask is used to generate WRAP base address and the 
  // length of the second half of a WRAP transaction that has to be split into 
  // two MI-Side INCR transactions.
  // 
  // Decode if the transaction fits on the MI-Side without needing to translate 
  // it. Also decode if the tranaction is of maximum SI-Side width.
  // 
  /////////////////////////////////////////////////////////////////////////////
  
  // Transaction burst type.
  assign access_is_fix   = ( S_AXI_ABURST == C_FIX_BURST );
  assign access_is_incr  = ( S_AXI_ABURST == C_INCR_BURST );
  assign access_is_wrap  = ( S_AXI_ABURST == C_WRAP_BURST );
  
  // Generate address bits used for SI-side transaction size.
  always @ *
  begin
    case (S_AXI_ASIZE)
      3'b000: size_mask = ~C_DOUBLE_LEN[8 +: C_M_AXI_BYTES_LOG];
      3'b001: size_mask = ~C_DOUBLE_LEN[7 +: C_M_AXI_BYTES_LOG];
      3'b010: size_mask = ~C_DOUBLE_LEN[6 +: C_M_AXI_BYTES_LOG];
      3'b011: size_mask = ~C_DOUBLE_LEN[5 +: C_M_AXI_BYTES_LOG];
      3'b100: size_mask = ~C_DOUBLE_LEN[4 +: C_M_AXI_BYTES_LOG];
      3'b101: size_mask = ~C_DOUBLE_LEN[3 +: C_M_AXI_BYTES_LOG];
      3'b110: size_mask = ~C_DOUBLE_LEN[2 +: C_M_AXI_BYTES_LOG];
      3'b111: size_mask = ~C_DOUBLE_LEN[1 +: C_M_AXI_BYTES_LOG];
    endcase
  end
  
  // Generate address mask for split transactions.
  always @ *
  begin
    case (S_AXI_ASIZE)
      3'b000: split_addr_mask = ~C_DOUBLE_LEN[8 +: C_AXI_ADDR_WIDTH];
      3'b001: split_addr_mask = ~C_DOUBLE_LEN[7 +: C_AXI_ADDR_WIDTH];
      3'b010: split_addr_mask = ~C_DOUBLE_LEN[6 +: C_AXI_ADDR_WIDTH];
      3'b011: split_addr_mask = ~C_DOUBLE_LEN[5 +: C_AXI_ADDR_WIDTH];
      3'b100: split_addr_mask = ~C_DOUBLE_LEN[4 +: C_AXI_ADDR_WIDTH];
      3'b101: split_addr_mask = ~C_DOUBLE_LEN[3 +: C_AXI_ADDR_WIDTH];
      3'b110: split_addr_mask = ~C_DOUBLE_LEN[2 +: C_AXI_ADDR_WIDTH];
      3'b111: split_addr_mask = ~C_DOUBLE_LEN[1 +: C_AXI_ADDR_WIDTH];
    endcase
  end
  
  // Help vector to determine the affected addressbits in the SI-side domain.
  // Also help to determine the length of downzized thransaction on the MI-side.
  assign alen_help_vector = {{C_AXI_ADDR_WIDTH-8{1'b0}}, S_AXI_ALEN, 8'hFF};
  
  // Calculate the address bits that are affected when a complete wrap is detected.
  always @ *
  begin
    if ( access_is_wrap & ( C_SUPPORT_BURSTS == 1 ) ) begin
      case (S_AXI_ASIZE)
        3'b000: cmd_mask_i  = alen_help_vector[8-0 +: C_S_AXI_BYTES_LOG];
        3'b001: cmd_mask_i  = alen_help_vector[8-1 +: C_S_AXI_BYTES_LOG];
        3'b010: cmd_mask_i  = alen_help_vector[8-2 +: C_S_AXI_BYTES_LOG];
        3'b011: cmd_mask_i  = alen_help_vector[8-3 +: C_S_AXI_BYTES_LOG];
        3'b100: cmd_mask_i  = alen_help_vector[8-4 +: C_S_AXI_BYTES_LOG];
        3'b101: cmd_mask_i  = alen_help_vector[8-5 +: C_S_AXI_BYTES_LOG];
        3'b110: cmd_mask_i  = alen_help_vector[8-6 +: C_S_AXI_BYTES_LOG];
        3'b111: cmd_mask_i  = alen_help_vector[8-7 +: C_S_AXI_BYTES_LOG];
      endcase
    end else begin
      cmd_mask_i          = {C_S_AXI_BYTES_LOG{1'b1}};
    end
  end

  // Calculate the length of downzized thransaction on the MI-side.
  always @ *
  begin
    if ( access_fit_mi_side & ( C_SUPPORT_BURSTS == 1 ) ) begin
      full_downsized_len = alen_help_vector[8-0 +: C_S_AXI_BYTES_LOG + 8];
    end else begin
      case (S_AXI_ASIZE)
        3'b000: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-0 +: C_S_AXI_BYTES_LOG + 8];  // Illegal setting.
        3'b001: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-1 +: C_S_AXI_BYTES_LOG + 8];  // Illegal setting.
        3'b010: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-2 +: C_S_AXI_BYTES_LOG + 8];  // Illegal setting.
        3'b011: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-3 +: C_S_AXI_BYTES_LOG + 8];
        3'b100: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-4 +: C_S_AXI_BYTES_LOG + 8];
        3'b101: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-5 +: C_S_AXI_BYTES_LOG + 8];
        3'b110: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-6 +: C_S_AXI_BYTES_LOG + 8];
        3'b111: full_downsized_len = alen_help_vector[8+C_M_AXI_BYTES_LOG-7 +: C_S_AXI_BYTES_LOG + 8];
      endcase
    end
  end
  
  // Extract the least significant part (that fit MI-side LEN).
  assign downsized_len = full_downsized_len[8-1:0];
  
  // Calculate if the current setting would fit a WRAP downsizing.
  always @ *
  begin
    if ( access_fit_mi_side & ( C_SUPPORT_BURSTS == 1 ) ) begin
      legal_wrap_len = 1'b1;
    end else begin
      case (S_AXI_ASIZE)
        3'b000: legal_wrap_len = 1'b1;  // Illegal setting.
        3'b001: legal_wrap_len = 1'b1;  // Illegal setting.
        3'b010: legal_wrap_len = 1'b1;  // Illegal setting.
        3'b011: legal_wrap_len = S_AXI_ALEN < ( 16 * (2 ** C_M_AXI_NATIVE_SIZE) / (2 ** 3) );
        3'b100: legal_wrap_len = S_AXI_ALEN < ( 16 * (2 ** C_M_AXI_NATIVE_SIZE) / (2 ** 4) );
        3'b101: legal_wrap_len = S_AXI_ALEN < ( 16 * (2 ** C_M_AXI_NATIVE_SIZE) / (2 ** 5) );
        3'b110: legal_wrap_len = S_AXI_ALEN < ( 16 * (2 ** C_M_AXI_NATIVE_SIZE) / (2 ** 6) );
        3'b111: legal_wrap_len = S_AXI_ALEN < ( 16 * (2 ** C_M_AXI_NATIVE_SIZE) / (2 ** 7) );
      endcase
    end
  end
  
  // Length when converting a large FIX transaction into INCR.
  always @ *
  begin
    case (S_AXI_ASIZE)
      3'b000: fix_len = ( 8'h00 >> C_M_AXI_BYTES_LOG );
      3'b001: fix_len = ( 8'h01 >> C_M_AXI_BYTES_LOG );
      3'b010: fix_len = ( 8'h03 >> C_M_AXI_BYTES_LOG );
      3'b011: fix_len = ( 8'h07 >> C_M_AXI_BYTES_LOG );
      3'b100: fix_len = ( 8'h0F >> C_M_AXI_BYTES_LOG );
      3'b101: fix_len = ( 8'h1F >> C_M_AXI_BYTES_LOG );
      3'b110: fix_len = ( 8'h3F >> C_M_AXI_BYTES_LOG );
      3'b111: fix_len = ( 8'h7F >> C_M_AXI_BYTES_LOG );
    endcase
  end
  
  // Calculate unalignment address.
  always @ *
  begin
    case (S_AXI_ASIZE)
      3'b000: unalignment_addr  = 8'b0;
      3'b001: unalignment_addr  = {7'b0, ( S_AXI_AADDR[0 +: 1] >> C_M_AXI_BYTES_LOG )};
      3'b010: unalignment_addr  = {6'b0, ( S_AXI_AADDR[0 +: 2] >> C_M_AXI_BYTES_LOG )};
      3'b011: unalignment_addr  = {5'b0, ( S_AXI_AADDR[0 +: 3] >> C_M_AXI_BYTES_LOG )};
      3'b100: unalignment_addr  = {4'b0, ( S_AXI_AADDR[0 +: 4] >> C_M_AXI_BYTES_LOG )};
      3'b101: unalignment_addr  = {3'b0, ( S_AXI_AADDR[0 +: 5] >> C_M_AXI_BYTES_LOG )};
      3'b110: unalignment_addr  = {2'b0, ( S_AXI_AADDR[0 +: 6] >> C_M_AXI_BYTES_LOG )};
      3'b111: unalignment_addr  = {1'b0, ( S_AXI_AADDR[0 +: 7] >> C_M_AXI_BYTES_LOG )};
    endcase
  end
  
  // Mask for address bits that are inside burst address.
  always @ *
  begin
    case (S_AXI_ASIZE)
      3'b000: burst_mask  = alen_help_vector[8-0 +: C_AXI_ADDR_WIDTH];
      3'b001: burst_mask  = alen_help_vector[8-1 +: C_AXI_ADDR_WIDTH];
      3'b010: burst_mask  = alen_help_vector[8-2 +: C_AXI_ADDR_WIDTH];
      3'b011: burst_mask  = alen_help_vector[8-3 +: C_AXI_ADDR_WIDTH];
      3'b100: burst_mask  = alen_help_vector[8-4 +: C_AXI_ADDR_WIDTH];
      3'b101: burst_mask  = alen_help_vector[8-5 +: C_AXI_ADDR_WIDTH];
      3'b110: burst_mask  = alen_help_vector[8-6 +: C_AXI_ADDR_WIDTH];
      3'b111: burst_mask  = alen_help_vector[8-7 +: C_AXI_ADDR_WIDTH];
    endcase
  end
  
  // Mask address to get start WRAP boundary.
  assign masked_addr        = ( S_AXI_AADDR & ~burst_mask );
  
  // Calculate the burst WRAP LEN.
  assign burst_unalignment  = ( ( S_AXI_AADDR & burst_mask ) >> C_M_AXI_BYTES_LOG );
  assign wrap_unaligned_len = burst_unalignment[0 +: 8];
  
  // Get number of transactions for downsized data.
  assign num_transactions   = full_downsized_len[8 +: C_S_AXI_BYTES_LOG];

  // Detect if the transaction can fit on MI-side untouched.
  assign access_fit_mi_side = ( S_AXI_ASIZE <= C_M_AXI_NATIVE_SIZE );
  assign si_full_size       = ( S_AXI_ASIZE == C_S_AXI_NATIVE_SIZE );
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Generate Command Information.
  // 
  // From the decode SI-side information determine if the transaction need to 
  // be split:
  // * FIX is always split when the don't fit MI-Side (AxSIZE > MI-Side width).
  // * INCR is split when the calculated downsized length has any of the most
  //   significant bits set.
  // * WRAP is split when it cannot downsized to a legal length, i.e. the 
  //   downsized lenght requires more that 16 data beats. And it doesn't fit the 
  //   natuaral MI_Side width. And it is unaligned.
  //   (An aligneded WRAP can always be translated into a INCR without splitting,
  //    it is the wrap offset that forces the split).
  // 
  // Keep track of when the splitting of a transaction has starts and ends. This 
  // is done by detecting the need for a split and keeping track of the number 
  // of commands issued so far. The command count is reset when last command has 
  // been forwarded (and the SI-side transaction is acknowledged).
  // 
  // Calculate the length of the second part of a split WRAP transaction.
  // 
  // Determine when the last command for a split is about to be generated. This
  // information is used to ackowledge the SI-Side transaction and also prepare
  // for the next transaction. This means that the only command for a 
  // transaction that doesn't nedd to be split is also considered the last (and
  // only) command.
  // 
  // Set that Read data should mirrored when transaction is smaller or equal to
  // MI-Side width.
  // 
  // Extract unalignement information to be able to extract and merge data
  // correctly in the W and R channels. 
  // * First MI-Side data extraction isalways based on the SI-Side start address, 
  //   regardless of transaction type.
  // * WRAP and full size INCR transactions always start following split transactions
  //   at a SI-Side aligned boundary, i.e. 0.
  // * Split INCR that has AxSIZE less than SI-Side width has to adjust the data 
  //   extraction start with previously calculated address mask, i.e. to only use
  //   address bit defined by size difference between AxSIZE and SI-Side width.
  // 
  // Generate data extraction offset for small WRAP transactions.
  // 
  // Prepare address for next part of split transaction.
  // 
  /////////////////////////////////////////////////////////////////////////////
    
  // Detect when FIX must be split (and translate into INCR).
  assign fix_need_to_split  = access_is_fix & ~access_fit_mi_side &
                              ( C_SUPPORT_SPLITTING == 1 ) &
                              ( C_SUPPORT_BURSTS == 1 );
  
  // Detect when INCR must be split.
  assign incr_need_to_split = access_is_incr & ( num_transactions != 0 ) &
                              ( C_SUPPORT_SPLITTING == 1 ) &
                              ( C_SUPPORT_BURSTS == 1 );
  
  // Detect when WRAP must be split (and translate into INCR).
  assign wrap_need_to_split = access_is_wrap &
                              (~access_fit_mi_side & ~legal_wrap_len & ( wrap_unaligned_len != 0 )) &
                              ( C_SUPPORT_SPLITTING == 1 ) &
                              ( C_SUPPORT_BURSTS == 1 );
  
  // Detect when a command has to be split.
  assign need_to_split_q    = ( fix_need_to_split_q | incr_need_to_split_q | wrap_need_to_split_q );
  
  // Handle progress of split transactions.
  always @ (posedge ACLK) begin
    if ( ARESET ) begin
      split_ongoing     <= 1'b0;
    end else begin
      if ( pushed_new_cmd ) begin
        split_ongoing     <= need_to_split_q & ~last_split;
      end
    end
  end
  
  // Keep track of number of transactions generated.
  always @ (posedge ACLK) begin
    if ( ARESET ) begin
      pushed_commands <= 4'b0;
    end else begin
      if ( S_AXI_AREADY_I ) begin
        pushed_commands <= 4'b0;
      end else if ( pushed_new_cmd ) begin
        pushed_commands <= pushed_commands + 4'b1;
      end
    end
  end
  
  // Generate the remaining LEN for split WRAP transaction.
  always @ (posedge ACLK) begin
    if ( ARESET ) begin
      wrap_rest_len <= 8'b0;
    end else begin
      wrap_rest_len <= wrap_unaligned_len_q - 8'b1;
    end
  end
  
  // Detect last part of a command, split or not.
  assign last_fix_split     = access_is_fix_q & ( ~fix_need_to_split_q | 
                                                ( fix_need_to_split_q & ( S_AXI_ALEN_Q[0 +: 4] == pushed_commands ) ) );
  assign last_incr_split    = access_is_incr_q & ( num_transactions_q   == pushed_commands );
  assign last_wrap_split    = access_is_wrap_q & ( ~wrap_need_to_split_q |
                                                 ( wrap_need_to_split_q & split_ongoing) );
  assign last_split         = last_fix_split | last_incr_split | last_wrap_split |
                              ( C_SUPPORT_SPLITTING == 0 ) |
                              ( C_SUPPORT_BURSTS == 0 );
  
  // Only FIX that are small enough is concidered FIX.
  assign cmd_fix_i          = access_is_fix_q & access_fit_mi_side_q;
  
  // Assign Split signals.
  assign cmd_split_i        = need_to_split_q & ~last_split;
  assign cmd_b_split_i      = need_to_split_q & ~last_split;
  
  // Determine if data should be mirrored for Read.
  assign cmd_mirror_i       = ( access_fit_mi_side_q ) |
                              ( C_SUPPORT_BURSTS == 0 );
  
  // Get unalignment address bits (including aligning it inside covered area).
  always @ *
  begin
    if ( split_ongoing & access_is_incr_q & si_full_size_q | split_ongoing & access_is_wrap_q ) begin
      cmd_first_word_ii = {C_S_AXI_BYTES_LOG{1'b0}};
    end else if ( split_ongoing & access_is_incr_q ) begin
      cmd_first_word_ii = S_AXI_AADDR_Q[C_S_AXI_BYTES_LOG-1:0] & split_addr_mask_q[C_S_AXI_BYTES_LOG-1:0];
    end else begin
      cmd_first_word_ii = S_AXI_AADDR_Q[C_S_AXI_BYTES_LOG-1:0];
    end
  end
  assign cmd_first_word_i   = cmd_first_word_ii & cmd_mask_q & size_mask_q;
  
  // Offset is the bits that is outside of the Mask.
  assign cmd_offset_i       = cmd_first_word_ii & ~cmd_mask_q;
  
  // Calculate base for next address.
  assign pre_mi_addr        = ( M_AXI_AADDR_I & split_addr_mask_q & {{C_AXI_ADDR_WIDTH-C_M_AXI_BYTES_LOG{1'b1}}, {C_M_AXI_BYTES_LOG{1'b0}}} );
  always @ (posedge ACLK) begin
    if ( ARESET ) begin
      next_mi_addr  = {C_AXI_ADDR_WIDTH{1'b0}};
    end else if ( pushed_new_cmd ) begin
      next_mi_addr  = pre_mi_addr + ( 9'h100 << C_M_AXI_BYTES_LOG );
    end
  end
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Translating Transaction.
  // 
  // Setup the number of MI-Side parts for the current transaction:
  // * FIX transactions that needs to be spit will have # of parts set by the
  //   length of the SI-Side transaction. 
  // * FIX with no need to split has 1 part (1:1).
  // * WRAP will be 1 part unless the length and unalignment requires a split, in
  //   which case the # of parts will be 2.
  // * INCR transactions will have the # of parts defined by the most significant
  //   bits of the true downsized length calculation.
  // 
  // Addreess has to be calculated for each part of a transaction on MI-Side:
  // * AxADDR is always used for the first part (and all types of transactions).
  // * WRAP has aligned wrap boundary as start address for second part.
  // * Split INCR transaction will increase address with maximum sized that can
  //   be covered by a MI-Side burst, i.e. 256 * 2^miBytes.
  // * FIX always use AxADDR for all parts, if split.
  // 
  // The length of a transaction part is calculated by a base length that is
  // depending on the type of transaction. This is the adjusted by unalignment
  // that this or previous parts have had.
  // 
  // Transactions that fit tha native MI-side will pass without altering 
  // AxSIZE and AxBURST. A transaction that is translated will always have the 
  // full MI-Side data width, i.e. AxSIZE is adjusted.
  // FIX and WRAP transactions that cannot fit on MI side will change type to
  // INCR and be split accordingly.
  // 
  /////////////////////////////////////////////////////////////////////////////
  
  // Generate the number of splits, to be able to coalesce responses in B & R channels.
  always @ *
  begin
    if ( fix_need_to_split_q ) begin
      cmd_b_repeat_i = S_AXI_ALEN_Q[0 +: 4];
    end else if ( incr_need_to_split_q ) begin
      cmd_b_repeat_i = num_transactions_q;
    end else if ( wrap_need_to_split_q ) begin
      cmd_b_repeat_i = 4'b1;
    end else begin
      cmd_b_repeat_i = 4'b0;
    end
  end
  
  // Select new size or remaining size.
  always @ *
  begin
    if ( split_ongoing & access_is_incr_q ) begin
      M_AXI_AADDR_I = next_mi_addr;
    end else if ( split_ongoing & access_is_wrap_q ) begin
      M_AXI_AADDR_I = masked_addr_q;
    end else begin
      M_AXI_AADDR_I = S_AXI_AADDR_Q;
    end
  end
  
  // Generate the base length for each transaction.
  always @ *
  begin
    if ( access_fit_mi_side_q ) begin
      base_len = S_AXI_ALEN_Q;
      
    end else if ( ( access_is_wrap_q & legal_wrap_len_q ) | ( access_is_incr_q & ~incr_need_to_split_q ) |
                  ( access_is_wrap_q & ~split_ongoing ) | ( access_is_incr_q & incr_need_to_split_q & last_split ) ) begin
      base_len = downsized_len_q;
      
    end else if ( fix_need_to_split_q ) begin
      base_len = fix_len_q;
      
    end else if ( access_is_wrap_q & split_ongoing ) begin
      base_len = wrap_rest_len;
      
    end else begin
      base_len = 8'hFF; 
      
    end
  end
  
  // Generate the compensation value for the transaction.
  always @ *
  begin
    if ( wrap_need_to_split_q & ~split_ongoing ) begin
      compensation_len = wrap_unaligned_len_q;
      
    end else if ( ( incr_need_to_split_q & ~split_ongoing ) | 
                  ( access_is_incr_q & ~incr_need_to_split_q & ~access_fit_mi_side_q ) |
                  ( fix_need_to_split_q ) ) begin
      compensation_len = unalignment_addr_q;
      
    end else begin
      compensation_len = 8'b0;
      
    end
  end
  
  // Generate the actual length.
  assign cmd_length_i = base_len - compensation_len; 
  assign M_AXI_ALEN_I = cmd_length_i;
  
  // Select directly forwarded or modified transaction.
  always @ *
  begin
    if ( ~access_fit_mi_side_q ) begin
      // SI to MI-side transaction translation.
      M_AXI_ASIZE_I  = C_M_AXI_NATIVE_SIZE;
      if ( access_is_fix_q | access_is_wrap_q & ~legal_wrap_len_q ) begin
        M_AXI_ABURST_I = C_INCR_BURST;
      end else begin
        M_AXI_ABURST_I = S_AXI_ABURST_Q;
      end
      
      // Command settings.
      cmd_size_i     = C_M_AXI_NATIVE_SIZE;
    end else begin
      // SI to MI-side transaction forwarding.
      M_AXI_ASIZE_I  = S_AXI_ASIZE_Q;
      M_AXI_ABURST_I = S_AXI_ABURST_Q;
      
      // Command settings.
      cmd_size_i     = S_AXI_ASIZE_Q;
    end
  end
  
  // Kill Exclusive for Split transactions.
  always @ *
  begin
    if ( need_to_split_q ) begin
      M_AXI_ALOCK_I = {S_AXI_ALOCK_Q[1], 1'b0};
    end else begin
      M_AXI_ALOCK_I = S_AXI_ALOCK_Q;
    end
  end
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Forward the command to the MI-side interface.
  // 
  //
  /////////////////////////////////////////////////////////////////////////////
  
  // Generate ready signal.
  // Move SI-side transaction to internal pipe stage.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      command_ongoing <= 1'b0;
      S_AXI_AREADY_I <= 1'b0;
    end else begin
      if (areset_d == 2'b10) begin
        S_AXI_AREADY_I <= 1'b1;
      end else begin
        if ( S_AXI_AVALID & S_AXI_AREADY_I ) begin
          command_ongoing <= 1'b1;
          S_AXI_AREADY_I <= 1'b0;
        end else if ( pushed_new_cmd & last_split ) begin
          command_ongoing <= 1'b0;
          S_AXI_AREADY_I <= 1'b1;
        end 
      end
    end
  end
  
  assign S_AXI_AREADY   = S_AXI_AREADY_I;
  
  // Only allowed to forward translated command when command queue is ok with it.
  assign M_AXI_AVALID_I = allow_new_cmd & command_ongoing;
  
  // Detect when MI-side is stalling.
  assign mi_stalling    = M_AXI_AVALID_I & ~M_AXI_AREADY_I;
                          
  
  /////////////////////////////////////////////////////////////////////////////
  // Simple transfer of paramters that doesn't need to be adjusted.
  // 
  // ID     - Transaction still recognized with the same ID.
  // CACHE  - No need to change the chache features. Even if the modyfiable
  //          bit is overridden (forcefully) there is no need to let downstream
  //          component beleive it is ok to modify it further.
  // PROT   - Security level of access is not changed when upsizing.
  // REGION - Address region stays the same.
  // QOS    - Quality of Service remains the same.
  // USER   - User bits remains the same.
  // 
  /////////////////////////////////////////////////////////////////////////////
  
  // Assign MI-Side.
  assign M_AXI_AID_I      = S_AXI_AID_Q;
  assign M_AXI_ACACHE_I   = S_AXI_ACACHE_Q;
  assign M_AXI_APROT_I    = S_AXI_APROT_Q;
  assign M_AXI_AREGION_I  = S_AXI_AREGION_Q;
  assign M_AXI_AQOS_I     = S_AXI_AQOS_Q;
  assign M_AXI_AUSER_I    = ( C_AXI_SUPPORTS_USER_SIGNALS ) ? S_AXI_AUSER_Q : {C_AXI_AUSER_WIDTH{1'b0}};
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Control command queue to W/R channel.
  //
  // It is allowed to continue pushing new commands as long as
  // * There is room in the queue(s).
  // * The ID is the same as previously queued. Since data is not reordered
  //   for the same ID it is ok to let them proceed.
  //   (It is only required to control ID for the AW/AR channels)
  //
  /////////////////////////////////////////////////////////////////////////////
  
  // Keep track of current ID in queue.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      queue_id <= {C_AXI_ID_WIDTH{1'b0}};
    end else begin
      if ( cmd_push ) begin
        // Store ID (it will be matching ID or a "new beginning").
        queue_id <= S_AXI_AID_Q;
      end
    end
  end
  
  // Check ID to make sure this command is allowed.
  assign id_match       = ( C_SINGLE_THREAD == 0 ) | ( queue_id == S_AXI_AID_Q);
  assign cmd_id_check_empty = (C_AXI_CHANNEL == 0) ? cmd_b_empty : cmd_empty;
  assign cmd_id_check   = cmd_id_check_empty | ( id_match & ~cmd_id_check_empty );
  
  // Check if it is allowed to push more commands (ID is allowed and there is room in the queue).
  assign allow_new_cmd  = (~cmd_full & ~cmd_b_full & cmd_id_check) | cmd_push_block;
  
  // Push new command when allowed and MI-side is able to receive the command.
  assign cmd_push       = M_AXI_AVALID_I & ~cmd_push_block;
  assign cmd_b_push     = M_AXI_AVALID_I & ~cmd_b_push_block & (C_AXI_CHANNEL == 0);
  
  // Block furter push until command has been forwarded to MI-side.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      cmd_push_block <= 1'b0;
    end else begin
      if ( pushed_new_cmd ) begin
        cmd_push_block <= 1'b0;
      end else if ( cmd_push & mi_stalling ) begin
        cmd_push_block <= 1'b1;
      end 
    end
  end
  
  // Block furter push until command has been forwarded to MI-side.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      cmd_b_push_block <= 1'b0;
    end else begin
      if ( S_AXI_AREADY_I ) begin
        cmd_b_push_block <= 1'b0;
      end else if ( cmd_b_push ) begin
        cmd_b_push_block <= 1'b1;
      end 
    end
  end
  
  // Acknowledge command when we can push it into queue (and forward it).
  assign pushed_new_cmd = M_AXI_AVALID_I & M_AXI_AREADY_I;
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Command Queue (W/R):
  // 
  // Instantiate a FIFO as the queue and adjust the control signals.
  //
  // Decode size to step before passing it along.
  //
  // When there is no need for bursts the command FIFO can be greatly reduced 
  // becase the following is always true:
  // * split = 0 (only single words)
  // * mirror = 1 (always mirror read data)
  // * length = 0
  // * nothing can be packed (i.e. no WRAP at all)
  //   * never any sub-size wraping => static offset (0) and mask (1)
  // 
  /////////////////////////////////////////////////////////////////////////////
  
  // Instantiated queue.
  generate
    if (C_SUPPORT_BURSTS == 1) begin : USE_BURSTS
  ict106_axic_fifo #
      (
       .C_FAMILY(C_FAMILY),
       .C_FIFO_DEPTH_LOG(C_FIFO_DEPTH_LOG),
       .C_FIFO_WIDTH(1+1+1+C_S_AXI_BYTES_LOG+C_S_AXI_BYTES_LOG+C_S_AXI_BYTES_LOG+3+8+3),
       .C_FIFO_TYPE("lut")
       ) 
       cmd_queue
      (
       .ACLK(ACLK),
       .ARESET(ARESET),
       .S_MESG({cmd_fix_i, cmd_split_i, cmd_mirror_i, cmd_first_word_i, 
                cmd_offset_i, cmd_mask_q, cmd_size_i, cmd_length_i, S_AXI_ASIZE_Q}),
       .S_VALID(cmd_push),
       .S_READY(s_ready),
       .M_MESG({cmd_fix, cmd_split, cmd_mirror, cmd_first_word,  
                cmd_offset, cmd_mask, cmd_size_ii, cmd_length, cmd_size}),
       .M_VALID(cmd_valid_i),
       .M_READY(cmd_ready)
       );
    end else begin : NO_BURSTS
      
      wire [C_S_AXI_BYTES_LOG-1:0]        cmd_first_word_out;
  
  ict106_axic_fifo #
      (
       .C_FAMILY(C_FAMILY),
       .C_FIFO_DEPTH_LOG(C_FIFO_DEPTH_LOG),
       .C_FIFO_WIDTH(1+C_S_AXI_BYTES_LOG+3),
       .C_FIFO_TYPE("lut")
       ) 
       cmd_queue
      (
       .ACLK(ACLK),
       .ARESET(ARESET),
       .S_MESG({cmd_fix_i, cmd_first_word_i, cmd_size_i}),
       .S_VALID(cmd_push),
       .S_READY(s_ready),
       .M_MESG({cmd_fix, cmd_first_word_out, cmd_size_ii}),
       .M_VALID(cmd_valid_i),
       .M_READY(cmd_ready)
       );
       
       assign cmd_split         = 1'b0;
       assign cmd_mirror        = 1'b1;
       assign cmd_first_word    = cmd_first_word_out;
       assign cmd_offset        = {C_S_AXI_BYTES_LOG{1'b0}};
       assign cmd_mask          = {C_S_AXI_BYTES_LOG{1'b1}};
       assign cmd_size          = C_S_AXI_NATIVE_SIZE;        // Doen't matter, unused in target.
       assign cmd_length        = 8'b0;
    end
  endgenerate

  // Queue is concidered full when not ready.
  assign cmd_full   = ~s_ready;
  
  // Queue is empty when no data at output port.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      cmd_empty <= 1'b1;
      cmd_depth <= {C_FIFO_DEPTH_LOG+1{1'b0}};
    end else begin
      if ( cmd_push & ~cmd_ready ) begin
        // Push only => Increase depth.
        cmd_depth <= cmd_depth + 1'b1;
        cmd_empty <= 1'b0;
      end else if ( ~cmd_push & cmd_ready ) begin
        // Pop only => Decrease depth.
        cmd_depth <= cmd_depth - 1'b1;
        cmd_empty <= ( cmd_depth == 1 );
      end
    end
  end
  
  // Assign external signal.
  assign cmd_valid  = cmd_valid_i;
  
  // Translate SI-side size to step for upsizer function.
  always @ *
  begin
    case (cmd_size_ii)
      3'b000: cmd_step_i = 7'b0000001;
      3'b001: cmd_step_i = 7'b0000010;
      3'b010: cmd_step_i = 7'b0000100;
      3'b011: cmd_step_i = 7'b0001000;
      3'b100: cmd_step_i = 7'b0010000;
      3'b101: cmd_step_i = 7'b0100000;
      3'b110: cmd_step_i = 7'b1000000;
      3'b111: cmd_step_i = 7'b0000000; // Illegal setting.
    endcase
  end
  
  // Get only the applicable bits in step.
  assign cmd_step = cmd_step_i[C_M_AXI_BYTES_LOG:0];
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Command Queue (B):
  // 
  // Add command queue for B channel only when it is AW channel and both burst
  // and splitting is supported.
  //
  // When turned off the command appears always empty.
  // 
  /////////////////////////////////////////////////////////////////////////////
  
  // Instantiated queue.
  generate
    if ( C_AXI_CHANNEL == 0 && C_SUPPORT_SPLITTING == 1 && C_SUPPORT_BURSTS == 1 ) begin : USE_B_CHANNEL
      
      wire                                cmd_b_valid_i;
      wire                                s_b_ready;
      
  ict106_axic_fifo #
      (
       .C_FAMILY(C_FAMILY),
       .C_FIFO_DEPTH_LOG(C_FIFO_DEPTH_LOG),
       .C_FIFO_WIDTH(1+4),
       .C_FIFO_TYPE("lut")
       ) 
       cmd_b_queue
      (
       .ACLK(ACLK),
       .ARESET(ARESET),
       .S_MESG({cmd_b_split_i, cmd_b_repeat_i}),
       .S_VALID(cmd_b_push),
       .S_READY(s_b_ready),
       .M_MESG({cmd_b_split, cmd_b_repeat}),
       .M_VALID(cmd_b_valid_i),
       .M_READY(cmd_b_ready)
       );
    
      // Queue is concidered full when not ready.
      assign cmd_b_full   = ~s_b_ready;
      
      // Queue is empty when no data at output port.
      always @ (posedge ACLK) begin
        if (ARESET) begin
          cmd_b_empty_i <= 1'b1;
          cmd_b_depth <= {C_FIFO_DEPTH_LOG+1{1'b0}};
        end else begin
          if ( cmd_b_push & ~cmd_b_ready ) begin
            // Push only => Increase depth.
            cmd_b_depth <= cmd_b_depth + 1'b1;
            cmd_b_empty_i <= 1'b0;
          end else if ( ~cmd_b_push & cmd_b_ready ) begin
            // Pop only => Decrease depth.
            cmd_b_depth <= cmd_b_depth - 1'b1;
            cmd_b_empty_i <= ( cmd_b_depth == 1 );
          end
        end
      end
      
      // Assign external signal.
      assign cmd_b_valid  = cmd_b_valid_i;
      assign cmd_b_empty = cmd_b_empty_i;
      
    end else begin : NO_B_CHANNEL
      
      // Assign external command signals.
      assign cmd_b_valid    = 1'b0;
      assign cmd_b_split    = 1'b0;
      assign cmd_b_repeat   = 4'b0;
   
      // Assign internal command FIFO signals.
      assign cmd_b_full     = 1'b0;
      assign cmd_b_empty    = 1'b1;
      
    end
  endgenerate
  
  
  /////////////////////////////////////////////////////////////////////////////
  // MI-side output handling
  // 
  /////////////////////////////////////////////////////////////////////////////
// TODO: registered?  
  assign M_AXI_AID      = M_AXI_AID_I;
  assign M_AXI_AADDR    = M_AXI_AADDR_I;
  assign M_AXI_ALEN     = M_AXI_ALEN_I;
  assign M_AXI_ASIZE    = M_AXI_ASIZE_I;
  assign M_AXI_ABURST   = M_AXI_ABURST_I;
  assign M_AXI_ALOCK    = M_AXI_ALOCK_I;
  assign M_AXI_ACACHE   = M_AXI_ACACHE_I;
  assign M_AXI_APROT    = M_AXI_APROT_I;
  assign M_AXI_AREGION  = M_AXI_AREGION_I;
  assign M_AXI_AQOS     = M_AXI_AQOS_I;
  assign M_AXI_AUSER    = M_AXI_AUSER_I;
  assign M_AXI_AVALID   = M_AXI_AVALID_I;
  assign M_AXI_AREADY_I = M_AXI_AREADY;
  
  
endmodule

