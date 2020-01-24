//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  ctrlport_reg_ro
//
// Description:
//
//   Implements a read-only register on a CTRL Port bus. The actual register 
//   bits are driven from outside of this module and passed in through the 
//   "value_in" input port. All input addresses are assumed to be 32-bit word 
//   aligned.
//
//   The width of the register is configurable. The register will take up the 
//   full power-of-2 address region, with a minimum of a 4-byte region. For 
//   example:
//
//      WIDTH (Bits) │ Address Space (Bytes)
//     ──────────────┼───────────────────────
//        1  to 32   │   4
//        33 to 64   │   8
//        64 to 128  │   16
//        etc.       │   etc.
//
//   When COHERENT is true and the WIDTH is larger than a single CTRL Port word 
//   (32 bits), reading the least-significant word of the register causes the 
//   other words of the register to be read and saved in a cache register on 
//   the same clock cycle. Reading the upper words of the register will always  
//   read from the cached copy. This allows reads of large, multi-word 
//   registers to be coherent. This is very important for registers in which 
//   there is a relationship between the upper and lower bits, such as in a 
//   counter which could change or roll over between 32-bit reads. The 
//   least-significant word MUST always be read first when COHERENT is true.
//
// Parameters:
//
//   ADDR     : Byte address to use for this register. This address must be 
//              aligned to the size of the register.
//   WIDTH    : Width of register to implement in bits. This determines the 
//              width of the "value_in" input and the amount of address space 
//              used by the register, which is always a power of 2.
//   COHERENT : Setting to 1 implements additional logic so that register reads 
//              maintain coherency. Setting to 0 removes this logic, so that 
//              each 32-bit word of the register is treated independently.
//
// Ports:
//
//   *ctrlport* : CTRL Port interface.
//   value_in   : The current value of the register.
//


module ctrlport_reg_ro #(
  parameter [   19:0] ADDR     = 0,
  parameter           WIDTH    = 32,
  parameter           COHERENT = 0
) (
  input wire ctrlport_clk,

  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  output reg         s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  input wire [WIDTH-1:0] value_in
);

  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  function automatic integer max(input integer a, b);
    max = a > b ? a : b;
  endfunction


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Calculate the number of bytes of address space this register will take up. 
  // The minimum size is a 32-bit register (4 bytes).
  localparam NUM_BYTES = max(4, 2**$clog2(WIDTH) / 8);

  // Calculate the number of bits needed to index each byte of this register.
  localparam BYTE_ADDR_W = $clog2(NUM_BYTES);

  // Calculate the number of bits needed to index each 32-bit word of this  
  // register.
  localparam WORD_ADDR_W = BYTE_ADDR_W-2;


  //---------------------------------------------------------------------------
  // Parameter Checking
  //---------------------------------------------------------------------------

  // Make sure WIDTH is valid
  if (WIDTH < 1) begin
    WIDTH_must_be_at_least_1();
  end

  // Make sure the address is word-aligned to the size of the register
  if (ADDR[BYTE_ADDR_W-1:0] != 0) begin
    ADDR_must_be_aligned_to_the_size_of_the_register();
  end


  //---------------------------------------------------------------------------
  // Resize Input Value
  //---------------------------------------------------------------------------

  // Use full size to simplify indexing. Unused bits will be optimized away.
  reg [NUM_BYTES*8-1:0] reg_val = 0;

  always @(*) begin
    reg_val            <= 0;
    reg_val[WIDTH-1:0] <= value_in;
  end


  //---------------------------------------------------------------------------
  // Read Logic
  //---------------------------------------------------------------------------

  reg [WIDTH-1:0] cache_reg;

  assign s_ctrlport_resp_status = 0;  // Status is always "OK" (0)

  //
  // Coherent implementation
  //
  if (WIDTH > 32 && COHERENT) begin : gen_coherent
    // In this case we want the upper bits, when read separately, to be 
    // coherent with the lower bits. So we register the upper bits when the 
    // least-significant word is read.

    always @(posedge ctrlport_clk) begin
      // Check if any part of this register is being addressed
      if (s_ctrlport_req_addr[19 : BYTE_ADDR_W] == ADDR[19 : BYTE_ADDR_W] && s_ctrlport_req_rd) begin
        s_ctrlport_resp_ack  <= 1'b1;

        // Check if we're reading the least-significant word
        if (s_ctrlport_req_addr[BYTE_ADDR_W-1 : 2] == 0) begin
          s_ctrlport_resp_data <= reg_val[31:0];
          cache_reg            <= reg_val;   // Unused bits will be optimized away

        // Otherwise, grab the word that's being addressed from the cached value
        end else begin
          s_ctrlport_resp_data <= cache_reg[s_ctrlport_req_addr[2 +: WORD_ADDR_W]*32 +: 32];
        end
      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end

  //
  // Non-coherent implementation
  //
  end else begin : gen_no_coherent
    // In this case, coherency is not required, so we just return the word 
    // that's being addressed.

    always @(posedge ctrlport_clk) begin
      // Check if any part of this register is being addressed
      if (s_ctrlport_req_addr[19 : BYTE_ADDR_W] == ADDR[19 : BYTE_ADDR_W] && s_ctrlport_req_rd) begin
        s_ctrlport_resp_ack  <= 1'b1;
        if (WORD_ADDR_W > 0) begin
          // Read back only the word of the register being addressed
          s_ctrlport_resp_data <= reg_val[s_ctrlport_req_addr[2 +: WORD_ADDR_W]*32 +: 32];
        end else begin
          s_ctrlport_resp_data <= reg_val;
        end
      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end
   
endmodule
