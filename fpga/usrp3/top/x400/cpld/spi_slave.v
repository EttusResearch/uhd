//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: spi_slave
//
// Description:
//
//   SPI slave for configuration CPOL = CPHA = 0.
//   Transfers 8 bit = 1 byte MSB first. Parallel data has to be
//   provided and consumed immediately when flags are asserted.
//
//   Limitation: clk frequency <= 2*sclk frequency
//
//   Data request from sclk domain is triggered towards the clk domain ahead of
//   time. This is due to the clock domain crossing using the synchronizer and
//   processing pipeline stages.
//
//   The worst case propagation delay of the used synchronizer is:
//
//     4 'clk' clock cycles:
//     1 clock cycle of signal propagation to synchronizer
//        (data_request_sclk assertion)
//     1 clock cycle to capture data with instability in first stage
//     1 clock cycle to stabilize first stage
//     1 clock cycle to capture data in second stage
//        (data_request_clk available in 'clk' domain)
//
//   Once synchronized in 'clk' domain, there is one additional clock cycle to
//   derive data_out_valid and data_in_required. To ensure that transmit data
//   is registered a 'clk' cycle ahead of the actual transmission we need 2
//   more 'clk' clock cycles. This ensures that transmit_word has changed and
//   is stable for at least one 'clk' cycle before 'sclk' asserts again. Any
//   additional time required externally to respond to the control port
//   requests should be considered in this crossing as well. This is a total of
//   7 clock cycles (+ctrlport response margin) @ clk domain. The minimum
//   required time in sclk domain to issue the request is calculated based on
//   the clock frequencies.
//
// Parameters:
//
//   CLK_FREQUENCY : Frequency of "clk"
//   SPI_FREQUENCY : Frequency of "sclk"
//

`default_nettype none


module spi_slave #(
  parameter CLK_FREQUENCY = 50000000,
  parameter SPI_FREQUENCY = 10000000
) (
  //---------------------------------------------------------------
  // SPI Interface
  //---------------------------------------------------------------

  input  wire sclk,
  input  wire cs_n,
  input  wire mosi,
  output wire miso,

  //---------------------------------------------------------------
  // Parallel Interface
  //---------------------------------------------------------------

  input  wire clk,
  input  wire rst,

  output reg  data_in_required,
  input  wire data_in_valid,
  input  wire [7:0] data_in,

  output reg  data_out_valid,
  output reg  [7:0] data_out,

  output wire active
);

  wire [0:0] data_request_clk;
  wire [0:0] reception_complete_clk;

  //---------------------------------------------------------------
  // SPI Receiver @ sclk
  //---------------------------------------------------------------

  reg [7:0] receiver_reg;
  reg [2:0] current_bit_index;
  reg       reception_complete_sclk = 1'b0;
  reg [7:0] received_word;

  always @(posedge sclk or posedge cs_n) begin
    // Reset logic on positive cs_n edge = slave idle
    if (cs_n) begin
      receiver_reg <= 8'b0;
    end
    // Rising edge of sclk
    else begin
      // Capture bits into shift register MSBs first
      receiver_reg <= {receiver_reg[6:0], mosi};
    end
  end

  // Reset with cs_n might occur too early during clk sync.
  // Reset half way through the reception.
  always @(posedge sclk) begin
    // Complete word was received
    if (current_bit_index == 7) begin
      reception_complete_sclk <= 1'b1;
      received_word <= {receiver_reg[6:0], mosi};

    // Reset after half transaction
    end else if (current_bit_index == 3) begin
      reception_complete_sclk <= 1'b0;
    end
  end

  //---------------------------------------------------------------
  // Handover of data sclk -> clk
  //---------------------------------------------------------------

  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (1)
  ) data_sync_inst (
    .clk (clk),
    .rst (1'b0),
    .in  (reception_complete_sclk),
    .out (reception_complete_clk)
  );

  //---------------------------------------------------------------
  // Parallel interface data output @ clk
  //---------------------------------------------------------------

  reg reception_complete_clk_delayed = 1'b0;

  // Propagate toggling signal without reset to ensure stability on reset
  always @(posedge clk) begin
    // Capture last state of reception
    reception_complete_clk_delayed <= reception_complete_clk;
  end

  // Derive data and control signal
  always @(posedge clk) begin
    if (rst) begin
      data_out_valid <= 1'b0;
      data_out <= 8'b0;
    end
    else begin
      // Default assignment
      data_out_valid <= 1'b0;

      // Provide data to output on rising_edge
      if (reception_complete_clk & ~reception_complete_clk_delayed) begin
        // Data can simply be captured as the reception complete signal
        // indicates stable values in received_word.
        data_out <= received_word;
        data_out_valid <= 1'b1;
      end
    end
  end

  //---------------------------------------------------------------
  // SPI Transmitter @ sclk
  //---------------------------------------------------------------

  // Data request calculation:
  // SCLK_CYCLES_DURING_DATA_REQ = 8 clk period / sclk period
  // Clock periods are expressed by reciprocal of frequencies.
  // Term "+CLK_FREQUENCY-1" is used to round up the result in integer logic.
  localparam SCLK_CYCLES_DURING_DATA_REQ  = (8*SPI_FREQUENCY + CLK_FREQUENCY-1)/CLK_FREQUENCY;
  // subtract from 8 bits per transfer to get target index
  localparam DATA_REQ_BIT_INDEX = 8 - SCLK_CYCLES_DURING_DATA_REQ;

  reg [7:0] transmit_bits;
  reg [7:0] transmit_word;
  reg       data_request_sclk = 1'b0;

  always @(negedge sclk or posedge cs_n) begin
    // Reset logic on positive cs_n edge = slave idle
    if (cs_n) begin
      current_bit_index <= 3'b0;
      data_request_sclk <= 1'b0;
      transmit_bits <= 8'b0;
    end
    // Falling edge of sclk
    else begin
      // Fill or move shift register for byte transmissions
      if (current_bit_index == 7) begin
        transmit_bits <= transmit_word;
      end else begin
        transmit_bits <= {transmit_bits[6:0], 1'b0};
      end

      // Update bit index
      current_bit_index <= current_bit_index + 1'b1;

      // Trigger request for new word at start of calculated index
      if (current_bit_index == DATA_REQ_BIT_INDEX-1) begin
        data_request_sclk <= 1'b1;
      // Reset after half the reception in case cs_n is not changed in between
      // two transactions.
      end else if (current_bit_index == (DATA_REQ_BIT_INDEX+4-1)%8) begin
        data_request_sclk <= 1'b0;
      end
    end
  end

  // Drive miso output with data when cs_n low
  assign miso = cs_n ? 1'bz : transmit_bits[7];

  //---------------------------------------------------------------
  // Handover of Data Request sclk -> clk
  //---------------------------------------------------------------

  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (1)
  ) request_sync_inst (
    .clk (clk),
    .rst (rst),
    .in  (data_request_sclk),
    .out (data_request_clk)
  );

  //---------------------------------------------------------------
  // Parallel Interface Data Input Control
  //---------------------------------------------------------------

  reg data_request_clk_delayed;

  always @(posedge clk) begin
    if (rst) begin
      data_request_clk_delayed <= 1'b0;
      data_in_required <= 1'b0;
      transmit_word <= 8'b0;
    end
    else begin
      // Default assignment
      data_in_required <= 1'b0;

      // Capture last state of data request
      data_request_clk_delayed <= data_request_clk;

      // Request data from input
      if (~data_request_clk_delayed & data_request_clk) begin
        data_in_required <= 1'b1;
      end

      // Capture new data if valid data available, 0 otherwise.
      if (data_in_required) begin
        if (data_in_valid) begin
          transmit_word <= data_in;
        end else begin
          transmit_word <= 8'b0;
        end
      end
    end
  end

  //---------------------------------------------------------------
  // Chip Select
  //---------------------------------------------------------------
  // Driven as active signal in parallel clock domain

  wire cs_n_clk;
  assign active = ~cs_n_clk;
  synchronizer #(
    .WIDTH            (1),
    .STAGES           (2),
    .INITIAL_VAL      (1'b1),
    .FALSE_PATH_TO_IN (1)
  ) active_sync_inst (
    .clk (clk),
    .rst (rst),
    .in  (cs_n),
    .out (cs_n_clk)
  );

endmodule


`default_nettype wire
