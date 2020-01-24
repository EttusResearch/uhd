//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_ram_fifo_bist
//
// Description:
//
//   Implements a built-in self test for the RAM FIFO. It can generate random
//   or sequential data that it outputs as quickly as possible. The output of
//   the RAM is verified to make sure that it matches what was input to the RAM.
//
// Parameters:
//
//   DATA_W   : The width of the data port to use for the AXI4-Stream interface
//
//   COUNT_W  : Width of internal counters. This must be wide enough so that
//              word, cycle, and and error counters don't overflow during a
//              test.
//
//   CLK_RATE : The frequency of clk in Hz
//
//   RAND     : Set to 1 for random data, 0 for sequential data.
//

module axi_ram_fifo_bist #(
  parameter DATA_W   = 64,
  parameter COUNT_W  = 48,
  parameter CLK_RATE = 200e6,
  parameter RAND     = 1
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
  output wire        s_ctrlport_resp_ack,
  output wire [31:0] s_ctrlport_resp_data,

  //--------------------------------------------------------------------------
  // AXI-Stream Interface
  //--------------------------------------------------------------------------

  // Output to RAM FIFO
  output wire [DATA_W-1:0] m_tdata,
  output reg               m_tvalid,
  input  wire              m_tready,

  // Input from RAM FIFO
  input  wire [DATA_W-1:0] s_tdata,
  input  wire              s_tvalid,
  output wire              s_tready,

  //---------------------------------------------------------------------------
  // Status
  //---------------------------------------------------------------------------

  output reg running

);

  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Internal word size to use for data generation. The output word will be a
  // multiple of this size.
  localparam WORD_W = 32;

  // Random number seed (must not be 0)
  localparam [WORD_W-1:0] SEED = 'h012345678;

  // Test data reset value
  localparam [WORD_W-1:0] INIT = RAND ? SEED : 0;


  //---------------------------------------------------------------------------
  // Assertions
  //---------------------------------------------------------------------------

  if (DATA_W % WORD_W != 0) begin
    DATA_W_must_be_a_multiple_of_WORD_W();
  end

  // LFSR only supports 8, 16, and 32 bits
  if (WORD_W != 32 && WORD_W != 16 && WORD_W != 8) begin
    WORD_W_not_supported();
  end

  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  // Linear-feedback Shift Register for random number generation.
  function [WORD_W-1:0] lfsr(input [WORD_W-1:0] din);
    reg new_bit;
    begin
      case (WORD_W)
        8  : new_bit = din[7]  ^ din[5]  ^ din[4]  ^ din[3];
        16 : new_bit = din[15] ^ din[14] ^ din[12] ^ din[3];
        32 : new_bit = din[31] ^ din[21] ^ din[1]  ^ din[0];
      endcase
      lfsr = { din[WORD_W-2:0], new_bit };
    end
  endfunction

  function [WORD_W-1:0] next(input [WORD_W-1:0] din);
    next = RAND ? lfsr(din) : din + 1;
  endfunction


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  reg [COUNT_W-1:0] tx_count;    // Number of words transmitted to FIFO
  reg [COUNT_W-1:0] rx_count;    // Number of words received back from FIFO
  reg [COUNT_W-1:0] error_count; // Number of words that show errors

  reg [WORD_W-1:0]  tx_data = next(INIT); // Transmitted data word
  reg [DATA_W-1:0]  rx_data = INIT;       // Received data words
  reg [WORD_W-1:0]  exp_data;             // Expected data word
  reg               rx_valid;             // Received word is value (strobe)

  wire [COUNT_W-1:0] num_words;   // Number of words to test
  reg  [COUNT_W-1:0] cycle_count; // Number of clock cycles test has been running for
  wire               start;       // Start test
  wire               stop;        // Stop test
  wire               clear;       // Clear the counters
  wire               continuous;  // Continuous test mode


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  axi_ram_fifo_bist_regs #(
    .DATA_W   (DATA_W),
    .COUNT_W  (COUNT_W),
    .CLK_RATE (CLK_RATE)
  ) axi_ram_fifo_bist_regs_i (
    .clk                  (clk),
    .rst                  (rst),
    .s_ctrlport_req_wr    (s_ctrlport_req_wr),
    .s_ctrlport_req_rd    (s_ctrlport_req_rd),
    .s_ctrlport_req_addr  (s_ctrlport_req_addr),
    .s_ctrlport_req_data  (s_ctrlport_req_data),
    .s_ctrlport_resp_ack  (s_ctrlport_resp_ack),
    .s_ctrlport_resp_data (s_ctrlport_resp_data),
    .tx_count             (tx_count),
    .rx_count             (rx_count),
    .error_count          (error_count),
    .cycle_count          (cycle_count),
    .num_words            (num_words),
    .start                (start),
    .stop                 (stop),
    .clear                (clear),
    .continuous           (continuous),
    .running              (running)
  );


  //---------------------------------------------------------------------------
  // State Machine
  //---------------------------------------------------------------------------

  localparam ST_IDLE      = 0;
  localparam ST_ACTIVE    = 1;
  localparam ST_WAIT_DONE = 2;

  reg [        1:0] state;
  reg [COUNT_W-1:0] num_words_m1;

  always @(posedge clk) begin
    if (rst) begin
      state    <= ST_IDLE;
      m_tvalid <= 0;
      running  <= 0;
    end else begin
      m_tvalid <= 0;

      case (state)
        ST_IDLE : begin
          num_words_m1 <= num_words-1;
          if (start) begin
            running <= 1;
            state   <= ST_ACTIVE;
          end
        end

        ST_ACTIVE : begin
          if (stop || (tx_count == num_words_m1 && m_tvalid && m_tready && !continuous)) begin
            m_tvalid <= 0;
            state    <= ST_WAIT_DONE;
          end else begin
            m_tvalid <= 1;
            running  <= 1;
          end
        end

        ST_WAIT_DONE : begin
          if (rx_count >= tx_count) begin
            running <= 0;
            state   <= ST_IDLE;
          end
        end
      endcase
    end
  end


  //---------------------------------------------------------------------------
  // Data Generator
  //---------------------------------------------------------------------------

  reg count_en;

  // Output data is the concatenation of our generated test word.
  assign m_tdata = {(DATA_W/WORD_W){ tx_data }};

  // We were born ready
  assign s_tready = 1;

  always @(posedge clk) begin
    if (rst) begin
      tx_data     <= next(INIT);
      exp_data    <= INIT;
      rx_valid    <= 0;
      tx_count    <= 0;
      rx_count    <= 0;
      error_count <= 0;
      cycle_count <= 0;
      count_en    <= 0;
    end else begin
      //
      // Output Data generation
      //
      if (m_tvalid && m_tready) begin
        tx_data  <= next(tx_data);
        tx_count <= tx_count + 1;
      end

      //
      // Expected Data Generation
      //
      if (s_tvalid & s_tready) begin
        rx_valid <= 1;
        exp_data <= next(exp_data);
        rx_count <= rx_count + 1;
        rx_data  <= s_tdata;
      end else begin
        rx_valid <= 0;
      end

      //
      // Data checker
      //
      if (rx_valid) begin
        if (rx_data !== {(DATA_W/WORD_W){exp_data}}) begin
          error_count <= error_count + 1;
        end
      end

      //
      // Cycle Counter
      //
      // Start counting after get the first word back so that we measure
      // throughput and not latency.
      if (state == ST_IDLE) count_en <= 0;
      else if (s_tvalid) count_en <= 1;

      if (count_en) cycle_count <= cycle_count + 1; 

      //
      // Clear counters upon request
      //
      if (clear) begin
        tx_count    <= 0;
        rx_count    <= 0;
        error_count <= 0;
        cycle_count <= 0;
      end
    end
  end

endmodule

