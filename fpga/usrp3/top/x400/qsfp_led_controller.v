//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: qsfp_led_controller
//
// Description:
//
//   Translate the CLIP active and link LED signals on FPGA to control port
//   requests in order to transfer them to the CPLD, which drives the LEDs
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
  // Logic to wait for response after trigging request
  //----------------------------------------------------------

  reg transfer_in_progress;
  reg [15:0] led_combined_delayed;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      m_ctrlport_req_wr <= 1'b0;
      transfer_in_progress <= 1'b0;
      led_combined_delayed <= 16'b0;
    end else begin
      // Default assignment
      m_ctrlport_req_wr <= 1'b0;

      // Issue new request on change if no request is pending
      if (led_combined != led_combined_delayed && ~transfer_in_progress) begin
        transfer_in_progress <= 1'b1;
        m_ctrlport_req_wr <= 1'b1;
        led_combined_delayed <= led_combined;
      end

      // Reset pending request
      if (m_ctrlport_resp_ack) begin
        transfer_in_progress <= 1'b0;
      end
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
