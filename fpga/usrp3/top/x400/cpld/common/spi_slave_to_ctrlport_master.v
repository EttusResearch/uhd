//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: spi_slave_to_ctrlport_master
//
// Description:
//
//   SPI slave to ContolPort master conversion in order to tunnel control port
//   request through an SPI bus.
//
//   The request format on SPI is defined as:
//
//     Write request:
//     1'b1 = write, 15 bit address, 32 bit data (MOSI), 8 bit processing gap,
//     5 bit padding, 1 bit ack, 2 bit status
//
//     Read request:
//     1'b0 = read, 15 bit address, 8 bit processing gap, 32 bit data (MISO),
//     5 bit padding, 1 bit ack, 2 bit status
//
// Parameters:
//
//   CLK_FREQUENCY : Frequency of "clk"
//   SPI_FREQUENCY : Frequency of "sclk"
//

`default_nettype none


module spi_slave_to_ctrlport_master #(
  parameter CLK_FREQUENCY = 50000000,
  parameter SPI_FREQUENCY = 10000000
) (
  //---------------------------------------------------------------
  // ControlPort Master
  //---------------------------------------------------------------

  input wire ctrlport_clk,
  input wire ctrlport_rst,

  output wire        m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,

  input wire        m_ctrlport_resp_ack,
  input wire [ 1:0] m_ctrlport_resp_status,
  input wire [31:0] m_ctrlport_resp_data,

  //---------------------------------------------------------------
  // SPI Slave
  //---------------------------------------------------------------

  input  wire sclk,
  input  wire cs_n,
  input  wire mosi,
  output wire miso
);

  `include "../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // SPI Slave
  //---------------------------------------------------------------

  wire [7:0] data_in;
  wire [7:0] data_out;
  wire       data_in_valid;
  wire       data_out_valid;
  wire       data_in_required;
  wire       spi_slave_active;

  spi_slave #(
    .CLK_FREQUENCY (CLK_FREQUENCY),
    .SPI_FREQUENCY (SPI_FREQUENCY)
  ) spi_slave_async (
    .sclk             (sclk),
    .cs_n             (cs_n),
    .mosi             (mosi),
    .miso             (miso),
    .clk              (ctrlport_clk),
    .rst              (ctrlport_rst),
    .data_in_required (data_in_required),
    .data_in_valid    (data_in_valid),
    .data_in          (data_in),
    .data_out_valid   (data_out_valid),
    .data_out         (data_out),
    .active           (spi_slave_active)
  );

  //---------------------------------------------------------------
  // Reset Generation from SPI Slave
  //---------------------------------------------------------------

  reg spi_slave_active_delayed = 1'b0;
  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      spi_slave_active_delayed <= 1'b0;
    end
    else begin
      spi_slave_active_delayed <= spi_slave_active;
    end
  end

  // Trigger reset on falling edge of active signal (rising edge of cs_n)
  wire spi_slave_reset;
  assign spi_slave_reset = spi_slave_active_delayed & (~spi_slave_active);

  //---------------------------------------------------------------
  // Transfer Constants
  //---------------------------------------------------------------

  localparam NUM_BYTES_TRANSACTION           = 8;
  localparam NUM_BYTES_WRITE_REQUEST_PAYLOAD = 6;
  localparam NUM_BYTES_READ_REQUEST_PAYLOAD  = 2;
  localparam MAX_BYTES_RESPONSE_PAYLOAD      = 5;

  //---------------------------------------------------------------
  // Data Receiver
  //---------------------------------------------------------------

  reg [3:0] num_bytes_received;
  reg       request_received;
  reg       write_request;
  reg       provide_response;
  reg [NUM_BYTES_WRITE_REQUEST_PAYLOAD*8-1:0] request_reg = {NUM_BYTES_WRITE_REQUEST_PAYLOAD*8 {1'b0}};

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst || spi_slave_reset) begin
      num_bytes_received <= 4'b0;
      request_received <= 1'b0;
      write_request <= 1'b0;
      provide_response <= 1'b0;
    end
    else begin
      // Counter number of received bytes
      if (data_out_valid) begin
        // Increment counter
        num_bytes_received <= num_bytes_received + 1'b1;

        if (num_bytes_received == NUM_BYTES_TRANSACTION-1) begin
          num_bytes_received <= 4'b0;
        end
      end

      // Check for read / write on first received byte's MSB
      if (data_out_valid && (num_bytes_received == 0)) begin
        write_request <= data_out[7];
      end

      // Detect complete request
      request_received <= 1'b0;
      if (data_out_valid) begin
        if (write_request && (num_bytes_received == NUM_BYTES_WRITE_REQUEST_PAYLOAD-1)) begin
          request_received <= 1'b1;
          provide_response <= 1'b1;
        end else if (~write_request && (num_bytes_received == NUM_BYTES_READ_REQUEST_PAYLOAD-1)) begin
          request_received <= 1'b1;
          provide_response <= 1'b1;
        end
      end

      // Detect end of response on last received byte
      if (num_bytes_received == NUM_BYTES_TRANSACTION-1) begin
        provide_response <= 1'b0;
      end

      // Capture data into shift register
      if (data_out_valid) begin
        request_reg <= {request_reg[NUM_BYTES_WRITE_REQUEST_PAYLOAD*8-8-1:0], data_out};
      end
    end
  end

  // Drive ControlPort
  localparam SPI_TRANSFER_ADDRESS_WIDTH = 15;
  assign m_ctrlport_req_wr   = request_received && write_request;
  assign m_ctrlport_req_rd   = request_received && ~write_request;
  assign m_ctrlport_req_data = request_reg[CTRLPORT_DATA_W-1:0];
  assign m_ctrlport_req_addr = (write_request) ?
                               {5'b0, request_reg[CTRLPORT_DATA_W+:SPI_TRANSFER_ADDRESS_WIDTH]} :
                               {5'b0, request_reg[0+:SPI_TRANSFER_ADDRESS_WIDTH]};

  //---------------------------------------------------------------
  // Response Handling
  //---------------------------------------------------------------

  reg  [MAX_BYTES_RESPONSE_PAYLOAD*8-1:0] response_reg;
  reg  ready_for_response; // active during processing gap
  wire write_response_byte;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst || spi_slave_reset) begin
      response_reg <= {8*MAX_BYTES_RESPONSE_PAYLOAD {1'b0}};
      ready_for_response <= 1'b0;
    end
    else begin
      // Reset response on new request
      if (request_received) begin
        ready_for_response <= 1'b1;
        if (write_request) begin
          // Just last byte -> padding, ack flag, CMDERR, padding (data length)
          response_reg <= {5'b0, 1'b1, CTRL_STS_CMDERR, {CTRLPORT_DATA_W{1'b0}}};
        end else begin
          // Last 5 bytes -> data = 0, Padding, ack flag, CMDERR
          response_reg <= {{CTRLPORT_DATA_W{1'b0}}, 5'b0, 1'b1, CTRL_STS_CMDERR};
        end

      // Capture response within processing gap, leave default response from above otherwise
      end else if (m_ctrlport_resp_ack && ready_for_response) begin
        if (write_request) begin
          response_reg <= {5'b0, m_ctrlport_resp_ack, m_ctrlport_resp_status, {CTRLPORT_DATA_W{1'b0}}};
        end else begin
          response_reg <= {m_ctrlport_resp_data, 5'b0, m_ctrlport_resp_ack, m_ctrlport_resp_status};
        end
      end

      // Shift data after writing to slave
      if (write_response_byte) begin
        response_reg <= {response_reg[0+:(MAX_BYTES_RESPONSE_PAYLOAD-1)*8], 8'b0};
        ready_for_response <= 1'b0;
      end
    end
  end

  // Response is written after request part has been transferred
  assign write_response_byte = data_in_required && provide_response;

  // Assign SPI slave inputs
  assign data_in       = response_reg[(MAX_BYTES_RESPONSE_PAYLOAD-1)*8+:8];
  assign data_in_valid = write_response_byte;

endmodule


`default_nettype wire
