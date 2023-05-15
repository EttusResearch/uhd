//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: led_control_tb
//
// Description:
//  Simulates LED control module.
//
`timescale 1ns / 1ps

module led_control_tb();

reg clk50;
reg reset_clk50;
reg        s_ctrlport_req_wr;
reg        s_ctrlport_req_rd;
reg [19:0] s_ctrlport_req_addr;
reg [31:0] s_ctrlport_req_data;

// Response
wire         s_ctrlport_resp_ack;
wire  [ 1:0] s_ctrlport_resp_status;
wire  [31:0] s_ctrlport_resp_data;

// LED Control (domain: ctrlport_clk)
wire         ch0_rx2_led;
wire         ch0_tx_led;
wire         ch0_rx_led;
wire         ch1_rx2_led;
wire         ch1_tx_led;
wire         ch1_rx_led;
wire         ch2_rx2_led;
wire         ch2_tx_led;
wire         ch2_rx_led;
wire         ch3_rx2_led;
wire         ch3_tx_led;
wire         ch3_rx_led;

led_control #(
    .BASE_ADDRESS  (32'h10)
  ) led_control_db1 (
    .ctrlport_clk            (clk50),
    .ctrlport_rst            (reset_clk50),
    .s_ctrlport_req_wr       (s_ctrlport_req_wr),
    .s_ctrlport_req_rd       (s_ctrlport_req_rd),
    .s_ctrlport_req_addr     (s_ctrlport_req_addr),
    .s_ctrlport_req_data     (s_ctrlport_req_data),
    .s_ctrlport_resp_ack     (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (s_ctrlport_resp_status),
    .s_ctrlport_resp_data    (s_ctrlport_resp_data),
    .ch0_rx2_led             (ch0_rx2_led),
    .ch0_tx_led              (ch0_tx_led),
    .ch0_rx_led              (ch0_rx_led),
    .ch1_rx2_led             (ch1_rx2_led),
    .ch1_tx_led              (ch1_tx_led),
    .ch1_rx_led              (ch1_rx_led),
    .ch2_rx2_led             (ch2_rx2_led),
    .ch2_tx_led              (ch2_tx_led),
    .ch2_rx_led              (ch2_rx_led),
    .ch3_rx2_led             (ch3_rx2_led),
    .ch3_tx_led              (ch3_tx_led),
    .ch3_rx_led              (ch3_rx_led)
  );

always #20 clk50 = ~clk50;

initial begin
  clk50 = 0;
  reset_clk50 = 0;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 0;
  s_ctrlport_req_addr = 0;
  s_ctrlport_req_data = 0;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'hFFFFFFFF;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h00000000;
  #100;
  s_ctrlport_req_rd = 0;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h20;
  s_ctrlport_req_data = 32'h00000000;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h00000000;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h1;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h3;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h7;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h107;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h307;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h10707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h30707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h70707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h1070707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h3070707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
  #100;
  s_ctrlport_req_wr = 1;
  s_ctrlport_req_addr = 'h10;
  s_ctrlport_req_data = 32'h7070707;
  #100;
  s_ctrlport_req_wr = 0;
  s_ctrlport_req_rd = 1;
  s_ctrlport_req_addr = 'h10;
end

endmodule
