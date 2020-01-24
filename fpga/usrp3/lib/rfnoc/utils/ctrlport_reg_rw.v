//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  ctrlport_reg_rw
//
// Description:
//
//   Implements a read/write register on a CTRL Port bus. CTRL Port byte 
//   enables are supported on writes. All input addresses are assumed to be 
//   32-bit word aligned.
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
//   When COHERENCY is true and the WIDTH is larger than a single CTRL Port 
//   word (32 bits), writing the least-significant words of the register causes 
//   them to be saved in a cache register and does not immediately update those 
//   words in the register. Writing the most-significant word of the register 
//   causes all the words to be simultaneously written to the register. This 
//   allows writes of large, multi-word registers to be coherent. This is very 
//   important for registers in which there is a relationship between the upper 
//   and lower bits, such as in a counter value in which changing only part of 
//   the word at a time could be seen as a large change when in fact the final 
//   change is small. The most-significant word MUST always be written last 
//   when COHERENCY is true.
//
// Parameters:
//
//   ADDR      : Byte address to use for this register. This address must be 
//               aligned to the size of the register.
//   WIDTH     : Width of register to implement in bits. This determines the 
//               width of the "value_out" input and the amount of address space 
//               used by the register, which is always a power of 2.
//   COHERENT  : Setting to 1 implements additional logic so that register 
//               writes maintain coherency. Setting to 0 removes this logic, so 
//               that each 32-bit word of the register is treated independently.
//   RESET_VAL : Value to give the register at power-on and at reset.
//
// Ports:
//
//   *ctrlport* : CTRL Port interface.
//   value_out  : The current value of the register.
//   written    : A strobe (single-cycle pulse) that indicates when the 
//                register was written. The new value may or may not be the 
//                same as the old value.
//


module ctrlport_reg_rw #(
  parameter [     19:0] ADDR      = 0,
  parameter             WIDTH     = 32,
  parameter             COHERENT  = 0,
  parameter [WIDTH-1:0] RESET_VAL = 'h0
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire [ 3:0] s_ctrlport_req_byte_en,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  output wire [WIDTH-1:0] value_out,
  output reg              written
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
  localparam NUM_BYTES = max(4, 2**$clog2(WIDTH)/8);

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
  // Write Logic
  //---------------------------------------------------------------------------

  // Use full size to simplify indexing. Unused bits will be optimized away.
  reg [8*NUM_BYTES-1:0] reg_val = 0;

  reg [8*NUM_BYTES-1:0] write_cache_reg;
  reg [  NUM_BYTES-1:0] write_en_cache_reg;

  reg s_ctrlport_resp_ack_wr;

  integer b, w;

  //
  // Coherent implementation
  //
  if (WIDTH > 32 && COHERENT) begin : gen_coherent
    always @(posedge ctrlport_clk) begin
      if (ctrlport_rst) begin
        reg_val <= RESET_VAL;
        written <= 1'b0;
      end else begin
        // Check if any part of this register is being written to
        if (s_ctrlport_req_addr[19 : BYTE_ADDR_W] == ADDR[19 : BYTE_ADDR_W] && s_ctrlport_req_wr) begin
          s_ctrlport_resp_ack_wr <= 1'b1;

          // Check if we're writing the most-significant word
          if (s_ctrlport_req_addr[BYTE_ADDR_W-1 : 2] == {BYTE_ADDR_W-2{1'b1}}) begin
            written <= 1'b1;

            // Iterate over the 4 bytes, updating each based on byte_en
            for (b = 0; b < 4; b = b+1) begin
              // Update the most-significant word from the input
              if(s_ctrlport_req_byte_en[b]) begin
                reg_val[32*(NUM_BYTES/4-1)+b*8 +: 8] <= s_ctrlport_req_data[8*b +: 8];
              end

              // Update the least-significant words from the cache
              for (w = 0; w < NUM_BYTES/4; w = w+1) begin
                if (write_en_cache_reg[b]) begin
                  reg_val[32*w+b*8 +: 8] <= write_cache_reg[32*w+b*8 +: 8];
                end
              end
            end

          // We're writing one of the least-significant words, so just cache 
          // the values written.
          end else begin
            w = s_ctrlport_req_addr[2 +: WORD_ADDR_W];
            write_cache_reg[w*32 +: 32]  <= s_ctrlport_req_data;
            write_en_cache_reg[w*4 +: 4] <= s_ctrlport_req_byte_en;
          end

        end else begin
          s_ctrlport_resp_ack_wr <= 1'b0;
          written                <= 1'b0;
        end
      end
    end

  //
  // Non-coherent implementation
  //
  end else begin : gen_no_coherent
    always @(posedge ctrlport_clk) begin
      if (ctrlport_rst) begin
        reg_val <= RESET_VAL;
        written <= 1'b0;
      end else begin
        // Check if any part of the word is begin written to
        if (s_ctrlport_req_addr[19 : BYTE_ADDR_W] == ADDR[19 : BYTE_ADDR_W] && s_ctrlport_req_wr) begin
          for (b = 0; b < 4; b = b + 1) begin
            if (s_ctrlport_req_byte_en[b]) begin
              if (WORD_ADDR_W > 0) begin
                // Update only the word of the register being addressed. "max" 
                // is needed by Vivado here to elaborate when WORD_ADDR_W is 0.
                w = s_ctrlport_req_addr[2 +: max(1, WORD_ADDR_W)];
                reg_val[w*32+b*8 +: 8] <= s_ctrlport_req_data[8*b +: 8];
              end else begin
                reg_val[b*8 +: 8] <= s_ctrlport_req_data[8*b +: 8];
              end
            end
          end
          s_ctrlport_resp_ack_wr <= 1'b1;
          written                <= 1'b1;
        end else begin
          s_ctrlport_resp_ack_wr <= 1'b0;
          written                <= 1'b0;
        end
      end
    end

  end


  //---------------------------------------------------------------------------
  // Read Logic
  //---------------------------------------------------------------------------

  reg s_ctrlport_resp_ack_rd;

  assign s_ctrlport_resp_status = 0;  // Status is always "OK" (0)

  assign value_out = reg_val[WIDTH-1:0];

  // Because the register is only changed by software, read coherency is not 
  // required, so we just return the word that's being addressed.
  always @(posedge ctrlport_clk) begin
    // Check if any part of this register is being addressed
    if (s_ctrlport_req_addr[19 : BYTE_ADDR_W] == ADDR[19 : BYTE_ADDR_W] && s_ctrlport_req_rd) begin
      s_ctrlport_resp_ack_rd <= 1'b1;
      if (WORD_ADDR_W > 0) begin
        // Read back only the word of the register being addressed
        s_ctrlport_resp_data <= reg_val[s_ctrlport_req_addr[2 +: WORD_ADDR_W]*32 +: 32];
      end else begin
        s_ctrlport_resp_data <= reg_val[31:0];
      end
    end else begin
      s_ctrlport_resp_ack_rd <= 1'b0;
    end
  end

  // Combine read/write ack
  assign s_ctrlport_resp_ack = s_ctrlport_resp_ack_wr | s_ctrlport_resp_ack_rd;
   
endmodule
