//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: qsfp_led_controller
//
// Description:
//
//   Translate the active and link status LED signals on FPGA to control port
//   requests in order to transfer them to the CPLD, which drives the LEDs.
//
// Parameters:
//
//   LED_REGISTER_ADDRESS : Address of LED register within CPLD.
//

`default_nettype none


module qsfp_led_controller #(
  parameter LED_REGISTER_ADDRESS = 0
) (
  // Common ControlPort signals
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // ControlPort request
  output reg         m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,
  output wire [ 3:0] m_ctrlport_req_byte_en,

  // ControlPort response
  input wire        m_ctrlport_resp_ack,
  input wire [ 1:0] m_ctrlport_resp_status,
  input wire [31:0] m_ctrlport_resp_data,

  // QSFP port LED signals
  input wire [3:0] qsfp0_led_active,
  input wire [3:0] qsfp0_led_link,
  input wire [3:0] qsfp1_led_active,
  input wire [3:0] qsfp1_led_link
);

  `include "../../lib/rfnoc/core/ctrlport.vh"

  //----------------------------------------------------------
  // Transfer LED signals to local clock domain
  //----------------------------------------------------------

  wire [15:0] led_combined;

  synchronizer #(
    .WIDTH            (16),
    .STAGES           (2),
    .INITIAL_VAL      (0),
    .FALSE_PATH_TO_IN (1)
  ) synchronizer_i (
    .clk (ctrlport_clk),
    .rst (ctrlport_rst),
    .in  ({qsfp1_led_active, qsfp1_led_link, qsfp0_led_active, qsfp0_led_link}),
    .out (led_combined)
  );

  //----------------------------------------------------------
  // State machine to initialize and update LED status
  //----------------------------------------------------------

  localparam COUNT_W   = 8;
  localparam MAX_COUNT = 2**COUNT_W-1;

  // This counter ensures the we have a delay before we attempt to initialize
  // the LED state and limits the rate at which the LEDs update.
  reg [COUNT_W-1:0] startup_count = 0;

  localparam ST_INIT     = 0;
  localparam ST_WRITE    = 1;
  localparam ST_WAIT_ACK = 2;
  localparam ST_IDLE     = 3;

  reg [1:0] state = ST_INIT;

  reg [15:0] led_combined_delayed;

  always @(posedge ctrlport_clk) begin
    m_ctrlport_req_wr    <= 0;
    led_combined_delayed <= led_combined;

    case (state)
      ST_INIT : begin
        // Set LED state after a delay
        startup_count <= startup_count + 1;
        if (startup_count == MAX_COUNT) begin
          state <= ST_WRITE;
        end
      end

      ST_WRITE : begin
        // Start an LED update request
        m_ctrlport_req_wr <= 1;
        state             <= ST_WAIT_ACK;
      end

      ST_WAIT_ACK : begin
        // Wait for the acknowledgment from the bus
        if (m_ctrlport_resp_ack) begin
          if (m_ctrlport_resp_status != CTRL_STS_OKAY) begin
            // Status was bad, so try again after a delay
            state <= ST_INIT;
          end else begin
            state <= ST_IDLE;
          end
        end
      end

      ST_IDLE : begin
        // Wait here until the LED status changes
        if (led_combined != led_combined_delayed) begin
          state <= ST_INIT;
        end
      end
    endcase

    if (ctrlport_rst) begin
      state                <= ST_INIT;
      startup_count        <= 0;
      m_ctrlport_req_wr    <= 0;
      led_combined_delayed <= 0;
    end
  end

  //----------------------------------------------------------
  // Static ControlPort assignments
  //----------------------------------------------------------

  assign m_ctrlport_req_rd      = 0;
  assign m_ctrlport_req_byte_en = 4'b0011;
  assign m_ctrlport_req_addr    = LED_REGISTER_ADDRESS;
  assign m_ctrlport_req_data    = {16'b0, led_combined_delayed};

endmodule


`default_nettype wire
