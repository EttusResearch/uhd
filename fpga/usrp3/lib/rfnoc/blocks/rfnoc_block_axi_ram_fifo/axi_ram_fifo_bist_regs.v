//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_ram_fifo_bist_regs
//
// Description:
//
//   Implements the registers for the RAM FIFO BIST logic.
//
// Parameters:
//
//   DATA_W   : The width of the data port to use for the AXI4-Stream 
//              interface.
//
//   COUNT_W  : Width of internal counters. This must be wide enough so that 
//              word, cycle, and and error counters don't overflow during a 
//              test.
//
//   CLK_RATE : The frequency of clk in Hz
//

module axi_ram_fifo_bist_regs #(
  parameter DATA_W    = 64,
  parameter COUNT_W   = 48,
  parameter CLK_RATE  = 200e6
) (
  input clk,
  input rst,

  //--------------------------------------------------------------------------
  // CTRL Port
  //--------------------------------------------------------------------------

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  //--------------------------------------------------------------------------
  // Control and Status
  //--------------------------------------------------------------------------

  input wire [COUNT_W-1:0] tx_count,
  input wire [COUNT_W-1:0] rx_count,
  input wire [COUNT_W-1:0] error_count,
  input wire [COUNT_W-1:0] cycle_count,

  output wire [COUNT_W-1:0] num_words,
  
  output reg                start,
  output reg                stop,
  output reg                clear,
  output reg                continuous,
  input wire                running
);

  `include "axi_ram_fifo_regs.vh"

  localparam BYTES_PER_WORD = DATA_W/8;
  localparam WORD_SHIFT     = $clog2(BYTES_PER_WORD);

  // Make sure DATA_W is a power of 2, or else the word/byte count conversion 
  // logic won't be correct.
  if (2**$clog2(DATA_W) != DATA_W) begin
    DATA_W_must_be_a_power_of_2();
  end

  // The register logic currently assumes that COUNT_W is at least 33 bits.
  if (COUNT_W <= 32) begin
    COUNT_W_must_be_larger_than_32();
  end

  wire [19:0] word_addr;
  wire [63:0] tx_byte_count;
  wire [63:0] rx_byte_count;
  reg  [63:0] num_bytes = 0;

  reg [31:0] tx_byte_count_hi = 0;
  reg [31:0] rx_byte_count_hi = 0;
  reg [31:0] error_count_hi   = 0;
  reg [31:0] cycle_count_hi   = 0;

  // Only use the word address to simplify address decoding logic
  assign word_addr = {s_ctrlport_req_addr[19:2], 2'b00 };

  // Convert between words and bytes
  assign tx_byte_count = tx_count  << WORD_SHIFT;
  assign rx_byte_count = rx_count  << WORD_SHIFT;
  assign num_words     = num_bytes >> WORD_SHIFT;


  always @(posedge clk) begin
    if (rst) begin
      s_ctrlport_resp_ack <= 0;
      start               <= 0;
      stop                <= 0;
      continuous          <= 0;
      clear               <= 0;
      num_bytes           <= 0;
    end else begin
      // Default values
      s_ctrlport_resp_ack <= 0;
      start <= 0;
      stop  <= 0;
      clear <= 0;

      //-----------------------------------------------------------------------
      // Read Logic
      //-----------------------------------------------------------------------

      if (s_ctrlport_req_rd) begin
        case (word_addr)
          REG_BIST_CTRL : begin
            s_ctrlport_resp_data                       <= 0;
            s_ctrlport_resp_data[REG_BIST_RUNNING_POS] <= running;
            s_ctrlport_resp_data[REG_BIST_CONT_POS]    <= continuous;
            s_ctrlport_resp_ack                        <= 1;
          end
          REG_BIST_CLK_RATE : begin
            s_ctrlport_resp_data <= CLK_RATE;
            s_ctrlport_resp_ack  <= 1;
          end
          REG_BIST_NUM_BYTES_LO : begin
            s_ctrlport_resp_data <= num_bytes[31:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_BIST_NUM_BYTES_HI : begin
            s_ctrlport_resp_data <= num_bytes[63:32];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_BIST_TX_BYTE_COUNT_LO : begin
            s_ctrlport_resp_data <= tx_byte_count[31:0];
            tx_byte_count_hi     <= tx_byte_count[63:32];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_BIST_TX_BYTE_COUNT_HI : begin
            s_ctrlport_resp_data <= tx_byte_count_hi;
            s_ctrlport_resp_ack  <= 1;            
          end
          REG_BIST_RX_BYTE_COUNT_LO : begin
            s_ctrlport_resp_data           <= rx_byte_count[31:0];
            rx_byte_count_hi[COUNT_W-33:0] <= rx_byte_count[COUNT_W-1:32];
            s_ctrlport_resp_ack            <= 1;
          end
          REG_BIST_RX_BYTE_COUNT_HI : begin
            s_ctrlport_resp_data <= rx_byte_count_hi;
            s_ctrlport_resp_ack  <= 1;  
          end
          REG_BIST_ERROR_COUNT_LO : begin
            s_ctrlport_resp_data         <= error_count[31:0];
            error_count_hi[COUNT_W-33:0] <= error_count[COUNT_W-1:32];
            s_ctrlport_resp_ack          <= 1;  
          end
          REG_BIST_ERROR_COUNT_HI : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data <= error_count_hi;
            s_ctrlport_resp_ack  <= 1;  
          end
          REG_BIST_CYCLE_COUNT_LO : begin
            s_ctrlport_resp_data         <= cycle_count[31:0];
            cycle_count_hi[COUNT_W-33:0] <= cycle_count[COUNT_W-1:32];
            s_ctrlport_resp_ack          <= 1;  
          end
          REG_BIST_CYCLE_COUNT_HI : begin
            s_ctrlport_resp_data <= 0;
            s_ctrlport_resp_data <= cycle_count_hi;
            s_ctrlport_resp_ack  <= 1;  
          end
        endcase
      end


      //-----------------------------------------------------------------------
      // Write Logic
      //-----------------------------------------------------------------------

      if (s_ctrlport_req_wr) begin
        case (word_addr)
          REG_BIST_CTRL : begin
            start      <= s_ctrlport_req_data[REG_BIST_START_POS];
            stop       <= s_ctrlport_req_data[REG_BIST_STOP_POS];
            clear      <= s_ctrlport_req_data[REG_BIST_CLEAR_POS];
            continuous <= s_ctrlport_req_data[REG_BIST_CONT_POS];
            s_ctrlport_resp_ack <= 1;
          end
          REG_BIST_NUM_BYTES_LO : begin
            // Update only the word-count portion
            num_bytes[31:WORD_SHIFT] <= s_ctrlport_req_data[31:WORD_SHIFT];
            s_ctrlport_resp_ack <= 1;
          end
          REG_BIST_NUM_BYTES_HI : begin
            num_bytes[COUNT_W-1:32] <= s_ctrlport_req_data[COUNT_W-33:0];
            s_ctrlport_resp_ack <= 1;
          end
        endcase
      end

    end
  end

endmodule

