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
// Description: Write Data Down-Sizer
// Mirror data for simple accesses.
// Merge data for burst.
//
//
// Verilog-standard:  Verilog 2001
//--------------------------------------------------------------------------
//
// Structure:
//   w_downsizer
//
//--------------------------------------------------------------------------
`timescale 1ps/1ps

module ict106_w_downsizer #
  (
   parameter         C_FAMILY                         = "none", 
                       // FPGA Family. Current version: virtex6 or spartan6.
   parameter integer C_AXI_ID_WIDTH                   = 4, 
                       // Width of all ID signals on SI and MI side of converter.
                       // Range: >= 1.
   parameter         C_S_AXI_DATA_WIDTH               = 32'h00000040,
                       // Width of S_AXI_WDATA and S_AXI_RDATA.
                       // Format: Bit32;
                       // Range: 'h00000040, 'h00000080, 'h00000100.
   parameter         C_M_AXI_DATA_WIDTH               = 32'h00000020, 
                       // Width of M_AXI_WDATA and M_AXI_RDATA. 
                       // Assume always smaller than C_S_AXI_DATA_WIDTH.
                       // Format: Bit32; 
                       // Range: 'h00000020, 'h00000040, 'h00000080.
   parameter integer C_AXI_SUPPORTS_USER_SIGNALS      = 0,
                       // 1 = Propagate all USER signals, 0 = Don’t propagate.
   parameter integer C_AXI_WUSER_WIDTH                = 1,
                       // Width of WUSER signals. 
                       // Range: >= 1.
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

   // Command Interface
   input  wire                              cmd_valid,
   input  wire                              cmd_mirror,
   input  wire                              cmd_fix,
   input  wire [C_S_AXI_BYTES_LOG-1:0]      cmd_first_word, 
   input  wire [C_S_AXI_BYTES_LOG-1:0]      cmd_offset,
   input  wire [C_S_AXI_BYTES_LOG-1:0]      cmd_mask,
   input  wire [C_M_AXI_BYTES_LOG:0]        cmd_step,
   input  wire [3-1:0]                      cmd_size,
   input  wire [8-1:0]                      cmd_length,
   output wire                              cmd_ready,
   
   // Slave Interface Write Data Ports
   input  wire [C_S_AXI_DATA_WIDTH-1:0]     S_AXI_WDATA,
   input  wire [C_S_AXI_DATA_WIDTH/8-1:0]   S_AXI_WSTRB,
   input  wire                                                    S_AXI_WLAST,
   input  wire [C_AXI_WUSER_WIDTH-1:0]          S_AXI_WUSER,
   input  wire                                                    S_AXI_WVALID,
   output wire                                                    S_AXI_WREADY,

   // Master Interface Write Data Ports
   output wire [C_M_AXI_DATA_WIDTH-1:0]    M_AXI_WDATA,
   output wire [C_M_AXI_DATA_WIDTH/8-1:0]  M_AXI_WSTRB,
   output wire                                                   M_AXI_WLAST,
   output wire [C_AXI_WUSER_WIDTH-1:0]         M_AXI_WUSER,
   output wire                                                   M_AXI_WVALID,
   input  wire                                                   M_AXI_WREADY
   );

   
  /////////////////////////////////////////////////////////////////////////////
  // Variables for generating parameter controlled instances.
  /////////////////////////////////////////////////////////////////////////////
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Local params
  /////////////////////////////////////////////////////////////////////////////
  
  // .
  localparam [24-1:0] C_DOUBLE_LEN       = 24'b0000_0000_0000_0000_1111_1111;
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Functions
  /////////////////////////////////////////////////////////////////////////////
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Internal signals
  /////////////////////////////////////////////////////////////////////////////

  // Sub-word handling.
  reg                             first_word;
  reg  [C_S_AXI_BYTES_LOG-1:0]    current_word_1;
  reg  [C_S_AXI_BYTES_LOG-1:0]    current_word;
  wire [C_S_AXI_BYTES_LOG-1:0]    current_word_adjusted;
  wire [C_RATIO_LOG-1:0]          current_index;
  wire                            last_beat;
  wire                            last_word;
  reg  [C_S_AXI_BYTES_LOG-1:0]    size_mask;
  
  // Sub-word handling for the next cycle.
  wire [C_S_AXI_BYTES_LOG-1:0]    next_word;
  
  // Burst length handling.
  reg                             first_mi_word;
  reg  [8-1:0]                    length_counter_1;
  reg  [8-1:0]                    length_counter;
  wire [8-1:0]                    next_length_counter;
  
  // Throttling help signals.
  wire                            word_completed;
  wire                            cmd_ready_i;
  wire                            pop_mi_data;
  wire                            mi_stalling;
  
  // Internal SI side control signals.
  wire                            S_AXI_WREADY_I;
  
  // Internal signals for MI-side.
  wire [C_M_AXI_DATA_WIDTH-1:0]   M_AXI_WDATA_I;
  wire [C_M_AXI_DATA_WIDTH/8-1:0] M_AXI_WSTRB_I;
  wire                            M_AXI_WLAST_I;
  wire [C_AXI_WUSER_WIDTH-1:0]    M_AXI_WUSER_I;
  wire                            M_AXI_WVALID_I;
  wire                            M_AXI_WREADY_I;
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Handle interface handshaking:
  //
  // Determine if a SI-side word has been completely used. For FIX transactions
  // the SI-side word is used to extract a single data word. 
  // Otherwise is the SI-side word considered to be used when last MI-side beat
  // has been extracted or when the last (most significant) MI-side word has 
  // been extracted from the SI-side word.
  //
  // Data on the MI-side is available when data is being taken from SI-side.
  //
  // The command is popped from the command queue once the last beat on the 
  // MI-side has been ackowledged.
  // 
  /////////////////////////////////////////////////////////////////////////////
  
  // Generate address bits used for SI-side transaction size.
  always @ *
  begin
    case (cmd_size)
      3'b000: size_mask = C_DOUBLE_LEN[8 +: C_S_AXI_BYTES_LOG];
      3'b001: size_mask = C_DOUBLE_LEN[7 +: C_S_AXI_BYTES_LOG];
      3'b010: size_mask = C_DOUBLE_LEN[6 +: C_S_AXI_BYTES_LOG];
      3'b011: size_mask = C_DOUBLE_LEN[5 +: C_S_AXI_BYTES_LOG];
      3'b100: size_mask = C_DOUBLE_LEN[4 +: C_S_AXI_BYTES_LOG];
      3'b101: size_mask = C_DOUBLE_LEN[3 +: C_S_AXI_BYTES_LOG];
      3'b110: size_mask = C_DOUBLE_LEN[2 +: C_S_AXI_BYTES_LOG];
      3'b111: size_mask = C_DOUBLE_LEN[1 +: C_S_AXI_BYTES_LOG];  // Illegal setting.
    endcase
  end
  
  // Detect when SI-side word is completely used.
  assign word_completed = ( cmd_fix ) |
                          ( cmd_mirror ) |
                          ( ~cmd_fix & ( ( next_word & size_mask ) == {C_S_AXI_BYTES_LOG{1'b0}} ) ) | 
                          ( ~cmd_fix & last_word ) | 
                          ( C_SUPPORT_BURSTS == 0 );
  
  // Pop word from SI-side.
  assign S_AXI_WREADY_I = S_AXI_WVALID & cmd_valid & word_completed & ~mi_stalling;
  assign S_AXI_WREADY   = S_AXI_WREADY_I;
  
  // Indicate when there is data available @ MI-side.
  assign M_AXI_WVALID_I = S_AXI_WVALID & cmd_valid;
  
  // Get MI-side data.
  assign pop_mi_data    = M_AXI_WVALID_I & M_AXI_WREADY_I;
  
  // Signal that the command is done (so that it can be poped from command queue).
  assign cmd_ready_i    = cmd_valid & pop_mi_data & last_word;
  assign cmd_ready      = cmd_ready_i;
  
  // Detect when MI-side is stalling.
  assign mi_stalling    = M_AXI_WVALID_I & ~M_AXI_WREADY_I;
                          
  
  /////////////////////////////////////////////////////////////////////////////
  // Keep track of data extraction:
  // 
  // Current address is taken form the command buffer for the first data beat
  // to handle unaligned Read transactions. After this is the extraction 
  // address usually calculated from this point.
  // FIX transactions uses the same word address for all data beats. 
  // 
  // Next word address is generated as current word plus the current step 
  // size, with masking to facilitate sub-sized wraping. The Mask is all ones
  // for normal wraping, and less when sub-sized wraping is used.
  // 
  // The calculated word addresses (current and next) is offseted by the 
  // current Offset. For sub-sized transaction the Offset points to the least 
  // significant address of the included data beats. (The least significant 
  // word is not necessarily the first data to be extracted, consider WRAP).
  // Offset is only used for sub-sized WRAP transcation that are Complete.
  // 
  // First word is active during the first MI-side data beat.
  // 
  // First MI is set during the first MI-side data beat.
  //
  // The transaction length is taken from the command buffer combinatorialy
  // during the First MI cycle. For each generated MI word it is decreased 
  // until Last Beat is reached.
  // 
  // Last word is determined depending as the last MI-side word generated for 
  // the command (generated from the AW translation).
  // If burst aren't supported all MI-side words are concidered to be the last.
  //
  /////////////////////////////////////////////////////////////////////////////
  
  // Select if the offset comes from command queue directly or 
  // from a counter while when extracting multiple MI words per SI word
  always @ *
  begin
    if ( first_word | cmd_fix )
      current_word = cmd_first_word;
    else
      current_word = current_word_1;
  end
  
  // Calculate next word.
  assign next_word              = ( current_word + cmd_step ) & cmd_mask;
  
  // Calculate the word address with offset.
  assign current_word_adjusted  = current_word + cmd_offset;
  
  // Get the ratio bits (MI-side words vs SI-side words).
  assign current_index          = current_word_adjusted[C_S_AXI_BYTES_LOG-C_RATIO_LOG +: C_RATIO_LOG];
  
  // Prepare next word address.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      first_word      <= 1'b1;
      current_word_1  <= 'b0;
    end else begin
      if ( pop_mi_data ) begin
        if ( M_AXI_WLAST_I ) begin
          // Prepare for next access.
          first_word      <=  1'b1;
        end else begin
          first_word      <=  1'b0;
        end
      
        current_word_1  <= next_word;
      end
    end
  end
  
  // Select command length or counted length.
  always @ *
  begin
    if ( first_mi_word )
      length_counter = cmd_length;
    else
      length_counter = length_counter_1;
  end
  
  // Calculate next length counter value.
  assign next_length_counter = length_counter - 1'b1;
  
  // Keep track of burst length.
  always @ (posedge ACLK) begin
    if (ARESET) begin
      first_mi_word    <= 1'b1;
      length_counter_1 <= 8'b0;
    end else begin
      if ( pop_mi_data ) begin
        if ( M_AXI_WLAST_I ) begin
          first_mi_word    <= 1'b1;
        end else begin
          first_mi_word    <= 1'b0;
        end
      
        length_counter_1 <= next_length_counter;
      end
    end
  end
  
  // Detect last beat in a burst.
  assign last_beat = ( length_counter == 8'b0 );
  
  // Determine if this last word that shall be extracted from this SI-side word.
  assign last_word = ( last_beat ) |
                     ( C_SUPPORT_BURSTS == 0 );
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Select the SI-side word to write.
  // Data must be multiplexed but the other information can be reused 
  // directly (ID and USER).
  //
  // Last need special handling since it is the last MI-side word generated 
  // from the SI-side word that shall be marked.
  // Split transactions needs to insert new LAST transactions. So to simplify
  // is the LAST signal always generated.
  //
  /////////////////////////////////////////////////////////////////////////////
  
  // ID and USER is copied from the SI word to all MI word transactions.
  assign M_AXI_WUSER_I  = ( C_AXI_SUPPORTS_USER_SIGNALS ) ? S_AXI_WUSER : {C_AXI_WUSER_WIDTH{1'b0}};
  
  // Data has to be multiplexed.
  assign M_AXI_WDATA_I  = S_AXI_WDATA[current_index * C_M_AXI_DATA_WIDTH   +: C_M_AXI_DATA_WIDTH];
  assign M_AXI_WSTRB_I  = S_AXI_WSTRB[current_index * C_M_AXI_DATA_WIDTH/8 +: C_M_AXI_DATA_WIDTH/8];
  
  // Handle last flag, i.e. set for MI-side last word.
  assign M_AXI_WLAST_I  = last_word;
  
  
  /////////////////////////////////////////////////////////////////////////////
  // MI-side output handling
  /////////////////////////////////////////////////////////////////////////////
// TODO: registered?  
  assign M_AXI_WDATA    = M_AXI_WDATA_I;
  assign M_AXI_WSTRB    = M_AXI_WSTRB_I;
  assign M_AXI_WLAST    = M_AXI_WLAST_I;
  assign M_AXI_WUSER    = M_AXI_WUSER_I;
  assign M_AXI_WVALID   = M_AXI_WVALID_I;
  assign M_AXI_WREADY_I = M_AXI_WREADY;
  
  
endmodule
