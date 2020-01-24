//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axi_ram_fifo_regs
//
// Description:  
//
//   Implements the software-accessible registers for the axi_ram_fifo block.
//


module axi_ram_fifo_regs #(
  parameter                  MEM_ADDR_W         = 32,
  parameter                  MEM_DATA_W         = 64,
  parameter [MEM_ADDR_W-1:0] FIFO_ADDR_BASE     = 'h0,
  parameter [MEM_ADDR_W-1:0] FIFO_ADDR_MASK     = 'h0000FFFF,
  parameter [MEM_ADDR_W-1:0] FIFO_ADDR_MASK_MIN = 'h00000FFF,
  parameter                  BIST               = 1,
  parameter                  IN_FIFO_SIZE       = 10,
  parameter                  WORD_ADDR_W        = 29,
  parameter                  BURST_TIMEOUT      = 128,
  parameter                  TIMEOUT_W          = 12
) (

  input wire clk,
  input wire rst,

  //--------------------------------------------------------------------------
  // CTRL Port
  //--------------------------------------------------------------------------

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Register Inputs and Outputs
  //---------------------------------------------------------------------------

  // Read-back Registers
  input wire [         31:0] rb_out_pkt_count,
  input wire [WORD_ADDR_W:0] rb_occupied,

  // Settings Registers
  output reg [          15:0] set_suppress_threshold,
  output reg [ TIMEOUT_W-1:0] set_timeout,
  output reg [MEM_ADDR_W-1:0] set_fifo_addr_base = FIFO_ADDR_BASE,
  output reg [MEM_ADDR_W-1:0] set_fifo_addr_mask = FIFO_ADDR_MASK
);

  `include "axi_ram_fifo_regs.vh"

  function automatic integer min(input integer a, b);
    min = a < b ? a : b;
  endfunction

  function automatic integer max(input integer a, b);
    max = a > b ? a : b;
  endfunction

  wire [19:0] word_addr;
  wire [63:0] reg_fifo_fullness;
  reg  [31:0] reg_fifo_fullness_hi;

  // Only use the word address to simplify address decoding logic
  assign word_addr = {s_ctrlport_req_addr[19:2], 2'b00 };

  // Convert the "occupied" word count to a 64-bit byte value
  assign reg_fifo_fullness = {
    {64-MEM_ADDR_W{1'b0}},            // Set unused upper bits to 0
    rb_occupied,
    {(MEM_ADDR_W-WORD_ADDR_W){1'b0}}  // Set byte offset bits to 0
  };

  always @(posedge clk) begin
    if (rst) begin
      s_ctrlport_resp_ack    <= 0;
      set_suppress_threshold <= 0;
      set_timeout            <= BURST_TIMEOUT;
      set_fifo_addr_base     <= FIFO_ADDR_BASE;
      set_fifo_addr_mask     <= FIFO_ADDR_MASK;
    end else begin
      s_ctrlport_resp_ack <= 0;

      //-----------------------------------------------------------------------
      // Write Logic
      //-----------------------------------------------------------------------

      if (s_ctrlport_req_wr) begin
        case (word_addr)
          REG_FIFO_READ_SUPPRESS : begin
            set_suppress_threshold <= s_ctrlport_req_data[REG_FIFO_SUPPRESS_THRESH_POS +: REG_FIFO_SUPPRESS_THRESH_W];
            s_ctrlport_resp_ack    <= 1;
          end
          REG_FIFO_TIMEOUT : begin
            set_timeout[REG_TIMEOUT_W-1:0] <= s_ctrlport_req_data[REG_TIMEOUT_W-1:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_FIFO_ADDR_BASE_LO : begin
            set_fifo_addr_base[min(32, MEM_ADDR_W)-1:0] <= s_ctrlport_req_data[min(32, MEM_ADDR_W)-1:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_BASE_HI : begin
            if (MEM_ADDR_W > 32) begin
              set_fifo_addr_base[max(32, MEM_ADDR_W-1):32] <= s_ctrlport_req_data[max(0, MEM_ADDR_W-33):0];
            end
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_MASK_LO : begin
            // Coerce the lower bits so we are guaranteed to meet the minimum mask size requirement.
            set_fifo_addr_mask[min(32, MEM_ADDR_W)-1:0] <= 
              s_ctrlport_req_data[min(32, MEM_ADDR_W)-1:0] | FIFO_ADDR_MASK_MIN;
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_MASK_HI : begin
            if (MEM_ADDR_W > 32) begin
              set_fifo_addr_mask[max(32, MEM_ADDR_W-1):32] <= s_ctrlport_req_data[max(0, MEM_ADDR_W-33):0];
            end
            s_ctrlport_resp_ack  <= 1;
          end
        endcase
      end
      

      //-----------------------------------------------------------------------
      // Read Logic
      //-----------------------------------------------------------------------

      if (s_ctrlport_req_rd) begin
        case (word_addr)
          REG_FIFO_INFO : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data[REG_FIFO_MAGIC_POS +: REG_FIFO_MAGIC_W] <= 16'hF1F0;
            s_ctrlport_resp_data[REG_FIFO_BIST_PRSNT_POS] <= (BIST != 0);
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_READ_SUPPRESS : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data[REG_FIFO_IN_FIFO_SIZE_POS +: REG_FIFO_IN_FIFO_SIZE_W] 
              <= IN_FIFO_SIZE;
            s_ctrlport_resp_data[REG_FIFO_SUPPRESS_THRESH_POS +: REG_FIFO_SUPPRESS_THRESH_W]
              <= set_suppress_threshold;
            s_ctrlport_resp_ack <= 1;
          end
          REG_FIFO_MEM_SIZE : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data[REG_FIFO_DATA_SIZE_POS +: REG_FIFO_DATA_SIZE_W]
              <= MEM_DATA_W;
            s_ctrlport_resp_data[REG_FIFO_ADDR_SIZE_POS +: REG_FIFO_ADDR_SIZE_W]
              <= MEM_ADDR_W;
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_TIMEOUT : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data[REG_TIMEOUT_W-1:0] <= set_timeout[REG_TIMEOUT_W-1:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_FULLNESS_LO : begin
            s_ctrlport_resp_data <= reg_fifo_fullness[31:0];
            reg_fifo_fullness_hi <= reg_fifo_fullness[63:32];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_FULLNESS_HI : begin
            s_ctrlport_resp_data <= reg_fifo_fullness_hi;
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_BASE_LO : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data[min(32, MEM_ADDR_W)-1:0] <= set_fifo_addr_base[min(32, MEM_ADDR_W)-1:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_BASE_HI : begin
            s_ctrlport_resp_data <= 0;
            if (MEM_ADDR_W > 32) begin
              s_ctrlport_resp_data[max(0,MEM_ADDR_W-33):0] <= set_fifo_addr_base[max(32, MEM_ADDR_W-1):32];
            end
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_MASK_LO : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data[min(32, MEM_ADDR_W)-1:0] <= set_fifo_addr_mask[min(32, MEM_ADDR_W)-1:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_ADDR_MASK_HI : begin
            s_ctrlport_resp_data <= 0;
            if (MEM_ADDR_W > 32) begin
              s_ctrlport_resp_data[max(0, MEM_ADDR_W-33):0] <= set_fifo_addr_mask[max(32, MEM_ADDR_W-1):32];
            end
            s_ctrlport_resp_ack  <= 1;
          end
          REG_FIFO_PACKET_CNT : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data <= rb_out_pkt_count;
            s_ctrlport_resp_ack  <= 1;
          end
        endcase
      end

    end
  end
  
endmodule