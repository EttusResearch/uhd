//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_byte_deserializer
//
// Description:
//   Slave interface of CtrlPort interface serialized as byte stream.
//   See description in ctrlport_byte_serializer module for more details.
//   Input and output data may be driven/registered using clocks with
//   different phases (but same frequency), this allows the capability
//   to facilitate meeting timing on this interface.
//

`default_nettype none

module ctrlport_byte_deserializer (
  // clock domain used to register input data
  input wire ctrlport_clk,
  // clock domain used to drive output data
  input wire ctrlport_clk_adjusted,
  input wire ctrlport_rst,

  // Request
  output wire        m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,

  // Response
  input  wire        m_ctrlport_resp_ack,
  input  wire [ 1:0] m_ctrlport_resp_status,
  input  wire [31:0] m_ctrlport_resp_data,

  // byte interface
  input  wire [ 7:0] bytestream_data_in,
  input  wire        bytestream_valid_in,
  input  wire        bytestream_direction,
  output reg  [ 7:0] bytestream_data_out = 8'b0,
  output reg         bytestream_valid_out = 1'b0,
  output reg         bytestream_output_enable = 1'b0
);

  `include "../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // transfer constants
  //---------------------------------------------------------------
  // derived from transaction specification
  localparam NUM_BYTES_RX_READ = 2;
  localparam NUM_BYTES_TX_READ = 5;
  localparam NUM_BYTES_RX_WRITE = 6;
  localparam NUM_BYTES_TX_WRITE = 1;

  localparam SPI_TRANSFER_ADDRESS_WIDTH = 15;

  //----------------------------------------------------------
  // handle transfer
  //----------------------------------------------------------
  localparam INIT_RX       = 2'd0;
  localparam RECEIVE       = 2'd1;
  localparam WAIT_RESPONSE = 2'd2;
  localparam SENDING       = 2'd3;

  // internal registers
  reg                     [ 1:0] state                  = INIT_RX;
  reg [NUM_BYTES_RX_WRITE*8-1:0] request_cache          = {NUM_BYTES_RX_WRITE*8 {1'b0}};
  reg [ NUM_BYTES_TX_READ*8-1:0] response_cache         = {NUM_BYTES_TX_READ*8 {1'b0}};
  reg                     [ 2:0] request_byte_counter   = 3'b0;
  reg                     [ 2:0] response_byte_counter  = 3'b0;
  reg                            transfer_complete      = 1'b0;
  reg                            write_transfer         = 1'b0;

  // input registers to relax input timing
  reg [7:0] bytestream_data_in_reg = 8'b0;
  reg       bytestream_valid_in_reg = 1'b0;
  reg       bytestream_direction_reg = 1'b0;

  always @ (posedge ctrlport_clk) begin
    bytestream_data_in_reg <= bytestream_data_in;
    bytestream_valid_in_reg <= bytestream_valid_in;
    bytestream_direction_reg <= bytestream_direction;
  end

  // state machine
  always @ (posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      state                    <= INIT_RX;
      request_byte_counter     <= 3'b0;
      transfer_complete        <= 1'b0;
      response_byte_counter    <= 3'b0;

    end else begin
      // default assignments
      transfer_complete <= 1'b0;

      case (state)
        // additional cycle for switching to make sure valid signal is driven
        // from master when being in RECEIVE state
        INIT_RX: begin
          request_byte_counter <= 3'b0;
          if (bytestream_direction_reg == 0) begin
            state <= RECEIVE;
          end
        end

        // wait for reception of request from master
        RECEIVE: begin
          if (bytestream_valid_in_reg) begin
            request_byte_counter  <= request_byte_counter + 1'b1;
            request_cache <= {request_cache[NUM_BYTES_RX_WRITE*8-9:0], bytestream_data_in_reg};

            // capture write or read
            if (request_byte_counter == 0) begin
              write_transfer <= bytestream_data_in_reg[7];
            end

            // wait until request completes
            if ((write_transfer && request_byte_counter == NUM_BYTES_RX_WRITE-1) ||
               (~write_transfer && request_byte_counter == NUM_BYTES_RX_READ-1)) begin
              transfer_complete <= 1'b1;
              state             <= WAIT_RESPONSE;
            end
          end

          // Workaround for missing pull down resistor:
          // Use pull up and schmitt trigger to detect FPGA reload by line going high unexpectedly
          if (bytestream_direction_reg == 1) begin
            state <= INIT_RX;
          end
        end

        WAIT_RESPONSE: begin
          request_byte_counter  <= 3'b0;
          response_byte_counter <= 3'b0;
          if (m_ctrlport_resp_ack) begin
            state <= SENDING;
          end

          //abort by host
          if (bytestream_direction_reg == 0) begin
            state <= INIT_RX;
          end
        end

        SENDING: begin
          response_byte_counter <= response_byte_counter + 1'b1;

          // wait until request completes
          if ((write_transfer && response_byte_counter == NUM_BYTES_TX_WRITE-1) ||
             (~write_transfer && response_byte_counter == NUM_BYTES_TX_READ-1)) begin
            state <= INIT_RX;
          end

          //abort by host
          if (bytestream_direction_reg == 0) begin
            state <= INIT_RX;
          end
        end

        default: begin
          state <= INIT_RX;
        end
      endcase
    end
  end

  // Output data control
  always @ (posedge ctrlport_clk_adjusted) begin
    if (ctrlport_rst) begin
      bytestream_valid_out     <= 1'b0;
      bytestream_output_enable <= 1'b0;
    end else begin

      bytestream_valid_out     <= 1'b0;
      // direction defined by master
      bytestream_output_enable <= bytestream_direction;

      case (state)
        WAIT_RESPONSE: begin

          if (write_transfer) begin
            response_cache <= {5'b0, 1'b1, m_ctrlport_resp_status, 32'b0};
          end else begin
            response_cache <= {m_ctrlport_resp_data, 5'b0, 1'b1, m_ctrlport_resp_status};
          end
        end

        SENDING: begin

          bytestream_valid_out <= 1'b1;
          bytestream_data_out  <= response_cache[NUM_BYTES_TX_READ*8-8+:8];
          response_cache       <= {response_cache[NUM_BYTES_TX_READ*8-9:0], 8'b0};

        end

        default: begin
          // NOP
        end
      endcase
    end
  end

  //----------------------------------------------------------
  // assign request to ctrlport
  //----------------------------------------------------------
  assign m_ctrlport_req_wr   = write_transfer & transfer_complete;
  assign m_ctrlport_req_rd   = ~write_transfer & transfer_complete;
  assign m_ctrlport_req_data = request_cache[0+:CTRLPORT_DATA_W];
  assign m_ctrlport_req_addr = (write_transfer) ?
    // Skipping data in LSBs to get to the address for writes.
    {5'b0, request_cache[CTRLPORT_DATA_W+:SPI_TRANSFER_ADDRESS_WIDTH]} :
    // Full request = address of 2 bytes in LSBs.
    {5'b0, request_cache[0+:SPI_TRANSFER_ADDRESS_WIDTH]};

endmodule

`default_nettype wire
