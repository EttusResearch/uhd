//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_byte_serializer
//
// Description:
//   Serializes CtrlPort requests into a byte stream.
//
//   The serialized data is similar to an AXI4-Streaming interface with one byte
//   per clock cycle and a valid signal. Direction controls the current transmission
//   direction. 0 = Master to Slave, 1 = Slave to Master, where this module is the
//   master. Direction is always present in direction master to slave where the
//   other signals valid and data can be shared on a tri-state bus.
//
//   The transmission is defined as described below. The bytes are transmitted MSB
//   first.
//   Write request:
//   1'b1 = write, 15 bit address, 32 bit data (MOSI) = 6 bytes request
//   5 bit padding, 1 bit ack, 2 bit status = 1 byte response
//   Read request:
//   1'b0 = read, 15 bit address = 2 bytes request
//   32 bit data, 5 bit padding, 1 bit ack, 2 bit status = 5 bytes response
//
//   When sharing valid and data signal lines between master and slave the
//   timing is defined as:
//
//   clk   __/--\__/--\__/--\__/--\__/--\__/--\__/--\__/--\__/--\__/--\__/--\__
//   direction            _______________________/-----------------\____________
//   master output enable _____/-----------------\______________________________
//   slave output enable  _____________________________/-----------------\______
//   valid & data         zzzzz| Master driven   | zzz | Slave driven    | zzzzz
//   transaction                <--- Request ---><--- Response --->
//
//   The slave should use the direction signal to derive it's own output enable
//   leaving the master the option to terminate the transaction.
//   On switch from slave to master the direction has to be changed at least
//   one clock cycle before enabling the master output enable to avoid driving the
//   bus from two sources.
//

`default_nettype none

module ctrlport_byte_serializer (
  // Clock and Reset
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  // Response
  output reg         s_ctrlport_resp_ack = 1'b0,
  output reg  [ 1:0] s_ctrlport_resp_status = 2'b0,
  output reg  [31:0] s_ctrlport_resp_data = 32'b0,

  // GPIO interface
  input  wire [ 7:0] bytestream_data_in,
  input  wire        bytestream_valid_in,
  output reg  [ 7:0] bytestream_data_out = 8'b0,
  output reg         bytestream_valid_out = 1'b0,
  output reg         bytestream_direction = 1'b0,
  output reg         bytestream_output_enable = 1'b1
);

  `include "../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // transfer constants
  //---------------------------------------------------------------
  // derived from the transaction format (see description above)
  localparam NUM_BYTES_TX_READ = 2;
  localparam NUM_BYTES_RX_READ = 5;
  localparam NUM_BYTES_TX_WRITE = 6;
  localparam NUM_BYTES_RX_WRITE = 1;

  localparam TIMEOUT_COUNTER_WIDTH = 6;

  //----------------------------------------------------------
  // FSM to handle transfers
  //----------------------------------------------------------
  localparam IDLE           = 3'd0;
  localparam SENDING        = 3'd1;
  localparam INIT_RX        = 3'd2;
  localparam DIR_SWITCH     = 3'd3;
  localparam DIR_SWITCH_DLY = 3'd4;
  localparam RECEIVING      = 3'd5;
  localparam ACK            = 3'd6;
  localparam TIMEOUT        = 3'd7;

  // input registers to relax input timing
  reg [7:0] bytestream_data_in_reg = 8'b0;
  reg       bytestream_valid_in_reg = 1'b0;
  always @ (posedge ctrlport_clk) begin
    bytestream_data_in_reg <= bytestream_data_in;
    bytestream_valid_in_reg <= bytestream_valid_in;
  end

  // internal registers
  reg                      [ 2:0] state           = IDLE;
  reg  [NUM_BYTES_TX_WRITE*8-1:0] request_cache   = {NUM_BYTES_TX_WRITE*8{1'b0}};
  reg  [ NUM_BYTES_RX_READ*8-1:0] response_cache  = {NUM_BYTES_RX_READ*8{1'b0}};
  reg                      [ 2:0] byte_counter    = 3'b0;
  reg                             write_transfer  = 1'b0;
  reg [TIMEOUT_COUNTER_WIDTH-1:0] timeout_counter = {TIMEOUT_COUNTER_WIDTH {1'b0}};

  always @ (posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      state <= IDLE;

      bytestream_valid_out     <= 1'b0;
      byte_counter             <= 3'b0;
      bytestream_direction     <= 1'b0;
      bytestream_output_enable <= 1'b1;

      s_ctrlport_resp_ack      <= 1'b0;
    end else begin
      case (state)
        IDLE: begin
          // reset values from previous states
          s_ctrlport_resp_ack      <= 1'b0;
          bytestream_valid_out     <= 1'b0;
          bytestream_output_enable <= 1'b1;
          byte_counter             <= 3'b0;
          timeout_counter          <= {TIMEOUT_COUNTER_WIDTH {1'b0}};

          // start transmission on read or write
          if (s_ctrlport_req_rd || s_ctrlport_req_wr) begin
            state <= SENDING;
            request_cache  <= {s_ctrlport_req_wr, s_ctrlport_req_addr[14:0], s_ctrlport_req_data};
            write_transfer <= s_ctrlport_req_wr;
          end
        end

        // send as many bytes as required for read / write
        SENDING: begin
          bytestream_data_out  <= request_cache[NUM_BYTES_TX_WRITE*8-8+:8];
          request_cache        <= {request_cache[NUM_BYTES_TX_WRITE*8-9:0], 8'b0};
          bytestream_valid_out <= 1'b1;
          byte_counter         <= byte_counter + 1'b1;

          if ((write_transfer && byte_counter == NUM_BYTES_TX_WRITE-1) ||
               (~write_transfer && byte_counter == NUM_BYTES_TX_READ-1)) begin
            state <= INIT_RX;
          end
        end

        // first cycle for switching to make sure valid signal is driven
        // from slave when being in RECEIVING state
        INIT_RX: begin
          state <= DIR_SWITCH;

          bytestream_direction     <= 1'b1;
          bytestream_output_enable <= 1'b0;
          bytestream_valid_out     <= 1'b0;

          byte_counter <= 3'b0;
        end

        // second switching cycle to let CPLD load the lines based on direction
        DIR_SWITCH: begin
          state <= DIR_SWITCH_DLY;
        end

        // third switching cycle to compensate data input register
        DIR_SWITCH_DLY: begin
          state <= RECEIVING;
        end

        // wait for response to be received
        // immediately change direction after successful reception to have one
        // clock cycle of pause to avoid double driving the bus
        RECEIVING: begin
          timeout_counter <= timeout_counter + 1;

          if (bytestream_valid_in_reg) begin
            byte_counter <= byte_counter + 1'b1;
            response_cache = {response_cache[NUM_BYTES_RX_READ*8-9:0], bytestream_data_in_reg};

            if ((write_transfer && byte_counter == NUM_BYTES_RX_WRITE-1) ||
               (~write_transfer && byte_counter == NUM_BYTES_RX_READ-1)) begin
              state                <= ACK;
              bytestream_direction <= 1'b0;
            end
          end

          if (timeout_counter == {TIMEOUT_COUNTER_WIDTH {1'b1}}) begin
            state                <= TIMEOUT;
            bytestream_direction <= 1'b0;
          end
        end

        // issue ctrlport response
        ACK: begin
          state <= IDLE;

          s_ctrlport_resp_ack    <= 1'b1;
          // status based on received ack
          s_ctrlport_resp_status <= response_cache[2] ? response_cache[1:0] : CTRL_STS_CMDERR;
          if (write_transfer) begin
            s_ctrlport_resp_data <= 32'b0;
          end else begin
            s_ctrlport_resp_data <= response_cache[39:8];
          end
        end

        TIMEOUT: begin
          state <= IDLE;

          s_ctrlport_resp_ack    <= 1'b1;
          s_ctrlport_resp_status <= CTRL_STS_CMDERR;
          s_ctrlport_resp_data   <= 32'b0;
        end

        default: begin
          state <= IDLE;
        end
      endcase
    end
  end

endmodule

`default_nettype wire
