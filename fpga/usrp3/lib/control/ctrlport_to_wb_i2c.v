//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_i2c
//
// Description:
//
//   This module wraps a I2C master and provides a ControlPort interface.
//
// Parameters:
//
//   BASE_ADDRESS : Base address for CtrlPort registers.
//

`default_nettype wire


module ctrlport_to_wb_i2c #(
  parameter BASE_ADDRESS = 0
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
  // I2C signals
  //---------------------------------------------------------------
  // i2c clock line
  input  wire scl_pad_i,       // SCL-line input
  output wire scl_pad_o,       // SCL-line output
  output wire scl_pad_en_o,    // SCL-line output enable (active low)

  // i2c data line
  input  wire sda_pad_i,       // SDA-line input
  output wire sda_pad_o,       // SDA-line output
  output wire sda_pad_en_o     // SDA-line output enable (active low)
);

  `include "../rfnoc/core/ctrlport.vh"
  `include "../wishbone/i2c_master.vh"

  //---------------------------------------------------------------
  // Translating CtrlPort <-> Wishbone I2C core
  //---------------------------------------------------------------

  reg         wb_cyc_i;         // Active bus cycle
  reg         wb_we_i  = 1'b0;  // Write access
  reg  [ 4:0] wb_adr_i = 5'b0;
  reg  [31:0] wb_dat_i = 32'b0;
  wire        wb_ack_o;
  wire [31:0] wb_dat_o;

  // Check for address to be in range [base_addr..base_addr+8)
  localparam NUM_ADDRESSES = 8;
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) &&
                          (s_ctrlport_req_addr < BASE_ADDRESS + NUM_ADDRESSES);

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
          //removed error condition since there is no error signal in i2c master
          s_ctrlport_resp_status <= CTRL_STS_OKAY;
        end

      // Write requests
      end else if (s_ctrlport_req_wr) begin
        // Assume there is a valid address
        //1-1 translation of wb to ctrl port interface
        if (s_ctrlport_req_addr < 8) begin
          wb_cyc_i <= 1'b1;
          wb_we_i <= 1'b1;
          wb_dat_i <= s_ctrlport_req_data;
        end
        case (s_ctrlport_req_addr)
          BASE_ADDRESS + WB_PRER_LO: begin
            wb_adr_i = WB_PRER_LO;
          end
          BASE_ADDRESS + WB_PRER_HI: begin
            wb_adr_i = WB_PRER_HI;
          end
          BASE_ADDRESS + WB_CTR: begin
            wb_adr_i = WB_CTR;
          end
          BASE_ADDRESS + WB_TXR: begin
            wb_adr_i = WB_TXR;
          end
          BASE_ADDRESS + WB_CR: begin
            wb_adr_i = WB_CR;
          end

        endcase

      // Read requests
      end else if (s_ctrlport_req_rd) begin
        // Assume there is a valid address
        wb_cyc_i <= 1'b1;
        wb_we_i <= 1'b0;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + WB_PRER_LO: begin
            wb_adr_i <= WB_PRER_LO;
          end
          BASE_ADDRESS + WB_PRER_HI: begin
            wb_adr_i <= WB_PRER_HI;
          end
          BASE_ADDRESS + WB_CTR: begin
            wb_adr_i <= WB_CTR;
          end
          BASE_ADDRESS + WB_RXR: begin
            wb_adr_i = WB_RXR;
          end
          BASE_ADDRESS + WB_SR: begin
            wb_adr_i = WB_SR;
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

  //wishbone-based i2c core
  i2c_master_top #(
    .ARST_LVL(1)
  ) i2c_master (
    .wb_clk_i(ctrlport_clk),
    .wb_rst_i(ctrlport_rst),
    .arst_i(1'b0),
    .wb_adr_i(wb_adr_i),
    .wb_dat_i(wb_dat_i),
    .wb_dat_o(wb_dat_o),
    .wb_we_i(wb_we_i),
    .wb_stb_i(wb_cyc_i),
    .wb_cyc_i(wb_cyc_i),
    .wb_ack_o(wb_ack_o),
    .wb_inta_o(),
    .scl_pad_i(scl_pad_i),
    .scl_pad_o(scl_pad_o),
    .scl_padoen_o(scl_pad_en_o),
    .sda_pad_i(sda_pad_i),
    .sda_pad_o(sda_pad_o),
    .sda_padoen_o(sda_pad_en_o)
  );

endmodule

