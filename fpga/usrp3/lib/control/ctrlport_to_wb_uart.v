//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_wb_uart
//
// Description:
//
//   This module wraps a simple UART and provides a ControlPort interface.
//
// Parameters:
//
//   BASE_ADDRESS     : Base address for CtrlPort registers.
//   REG_STRIDE_SIZE  : Address separation between registers.
//

`default_nettype wire


module ctrlport_to_wb_uart #(
  parameter BASE_ADDRESS = 0,
  parameter REG_STRIDE_SIZE = 1,
  parameter CLKDIV_DEFAULT = 0,
  parameter RX_SIZE = 8
) (
  //---------------------------------------------------------------
  // ControlPort Slave
  //---------------------------------------------------------------

  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status  = 0,
  output reg  [31:0] s_ctrlport_resp_data    = 0,

  //---------------------------------------------------------------
  // UART signals
  //---------------------------------------------------------------

  input  wire uart_rx,
  output wire uart_tx
);

  `include "../rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // Translating CtrlPort <-> Wishbone UART core
  //---------------------------------------------------------------

  reg         wb_cyc_i;         // Active bus cycle
  reg         wb_we_i  = 1'b0;  // Write access
  reg  [ 4:0] wb_adr_i = 3'b0;
  reg  [31:0] wb_dat_i = 32'b0;
  wire        wb_ack_o;
  wire [31:0] wb_dat_o;

  localparam SUART_CLKDIV = 0;
  localparam SUART_TXLEVEL = 1;
  localparam SUART_RXLEVEL = 2;
  localparam SUART_TXCHAR = 3;
  localparam SUART_RXCHAR = 4;

  // Check for address to be in range [base_addr..base_addr+5*stride)
  localparam REG_WINDOW_SIZE = 5*REG_STRIDE_SIZE;

  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) &&
                          (s_ctrlport_req_addr < BASE_ADDRESS + REG_WINDOW_SIZE);

  // Following chapter 3.2.3 (classic standard SINGLE WRITE cycle) of
  // https://cdn.opencores.org/downloads/wbspec_b4.pdf
  always @(posedge ctrlport_clk) begin
    // Reset internal registers and responses
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack <= 0;
      s_ctrlport_resp_data <= 0;
      s_ctrlport_resp_status <= CTRL_STS_OKAY;
      wb_cyc_i <= 0;
      wb_we_i <= 0;
      wb_dat_i <= 0;
      wb_adr_i <= 0;
    end else begin
      // Request independent default assignments
      s_ctrlport_resp_ack <= 1'b0;

      // Wait for ack on active bus transactions
      if (wb_cyc_i) begin
        if (wb_ack_o) begin
          // End bus cycle and generate response
          wb_cyc_i <= 1'b0;
          s_ctrlport_resp_ack  <= 1'b1;
          s_ctrlport_resp_data <= wb_dat_o;
          //removed error condition since there is no error signal in UART
          //core
          s_ctrlport_resp_status <= CTRL_STS_OKAY;
        end

      // Write requests
      end else if (s_ctrlport_req_wr) begin
        // Assume there is a valid address
        //1-1 translation of wb to ctrl port interface
        if (address_in_range) begin
          wb_cyc_i <= 1'b1;
          wb_we_i <= 1'b1;
          wb_dat_i <= s_ctrlport_req_data;
        end
        case (s_ctrlport_req_addr)
          BASE_ADDRESS + SUART_CLKDIV * REG_STRIDE_SIZE: begin
            wb_adr_i = SUART_CLKDIV;
          end
          BASE_ADDRESS + SUART_TXLEVEL * REG_STRIDE_SIZE: begin
            wb_adr_i = SUART_TXLEVEL;
          end
          BASE_ADDRESS + SUART_RXLEVEL * REG_STRIDE_SIZE: begin
            wb_adr_i = SUART_RXLEVEL;
          end
          BASE_ADDRESS + SUART_TXCHAR * REG_STRIDE_SIZE: begin
            wb_adr_i = SUART_TXCHAR;
          end
          BASE_ADDRESS + SUART_RXCHAR * REG_STRIDE_SIZE: begin
            wb_adr_i = SUART_RXCHAR;
          end

        endcase

      // Read requests
      end else if (s_ctrlport_req_rd) begin

        if(address_in_range) begin
          // Assume there is a valid address
          wb_cyc_i <= 1'b1;
          wb_we_i <= 1'b0;

          case (s_ctrlport_req_addr)
            BASE_ADDRESS + SUART_CLKDIV * REG_STRIDE_SIZE: begin
              wb_adr_i <= SUART_CLKDIV;
            end
            BASE_ADDRESS + SUART_TXLEVEL * REG_STRIDE_SIZE: begin
              wb_adr_i <= SUART_TXLEVEL;
            end
            BASE_ADDRESS + SUART_RXLEVEL * REG_STRIDE_SIZE: begin
              wb_adr_i <= SUART_RXLEVEL;
            end
            BASE_ADDRESS + SUART_TXCHAR * REG_STRIDE_SIZE: begin
              wb_adr_i = SUART_TXCHAR;
            end
            BASE_ADDRESS + SUART_RXCHAR * REG_STRIDE_SIZE: begin
              wb_adr_i = SUART_RXCHAR;
            end
            // Respond with 0
            default: begin
              s_ctrlport_resp_ack <= 1'b1;
              s_ctrlport_resp_data <= 'b0;
            end

          endcase
        end
      end
    end
  end

  //wishbone-based UART core
  // Makin default baud
  simple_uart #(
    .CLKDIV_DEFAULT(CLKDIV_DEFAULT),
    .RX_SIZE(RX_SIZE)
  ) simple_uart_i (
    .clk_i(ctrlport_clk),
    .rst_i(ctrlport_rst),
    .adr_i(wb_adr_i),
    .dat_i(wb_dat_i),
    .dat_o(wb_dat_o),
    .we_i(wb_we_i),
    .stb_i(wb_cyc_i),
    .cyc_i(wb_cyc_i),
    .ack_o(wb_ack_o),
    .rx_int_o(),
    .tx_int_o(),
    .tx_o(uart_tx),
    .rx_i(uart_rx)
  );

endmodule

