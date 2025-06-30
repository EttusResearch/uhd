//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-late
//
// Module: obx_register_endpoints
//
// Description:
//
//   Implements simple register endpoints for SPI transactions
//   targeted at the OBX CPLD. This module is used to, especially,
//   allows for configuration of the front-end controls.
//   There are only two registers currently defined:
//     - TX Control Register
//     - RX Control Register
//


`default_nettype none

module obx_register_endpoints(
  input  wire         reset_n,
  // The CS line is used asynchronously to update internal registers,
  // as well as clearing readback control signals.
  input  wire         spi_cs,
  input  wire         spi_sdi, spi_clk,
  output wire         spi_sdo,
  input  wire         internal_spi_access,
  output reg  [23:0]  tx_fe_ctrl,
  output reg  [23:0]  rx_fe_ctrl
);

  `include "regmap/obx_cpld_regs_utils.vh"

  // SPI buffer consists of:
  // 1-bit read/write indicator
  // 7-bit address
  // 24-bit data
  logic  [31:0]  spi_buf = 'b0;

  // SPI input buffer.
  always @(posedge spi_clk or negedge reset_n) begin
    if(~reset_n) begin
      spi_buf <= 'b0;
    end
    else begin
      spi_buf <= {spi_buf[30:0], spi_sdi};
    end
  end

  // SPI transaction decoding
  logic                          cpld_spi_rw;
  logic  [ 6:0]                  cpld_spi_address;
  logic  [23:0]                  cpld_spi_data_in;
  logic  [23:0]                  cpld_spi_data_out;
  logic  [SCRATCH_DATA_SIZE-1:0] scratch_data;

  always_comb begin
    cpld_spi_rw      = spi_buf[31];
    cpld_spi_address = spi_buf[30:24];
    cpld_spi_data_in = spi_buf[23:0];
  end

  // The TX and RX control registers are updated asynchronously
  // when the SPI CS line is asserted.
  always @(posedge spi_cs or negedge reset_n) begin
    if(~reset_n) begin
      tx_fe_ctrl <= 'b0;
      rx_fe_ctrl <= 'b0;
    end
    else if(internal_spi_access) begin // In-CPLD SPI access
      if (~cpld_spi_rw) begin
        case (cpld_spi_address)
          TX_CONTROL: begin
            tx_fe_ctrl <= cpld_spi_data_in;
          end
          RX_CONTROL: begin
            rx_fe_ctrl <= cpld_spi_data_in;
          end
          SCRATCH_REGISTER: begin
            // Handle scratch register
            scratch_data <= cpld_spi_data_in[15:0];
          end
          default: begin
            // Do nothing
            ;
          end
        endcase
      end
    end
  end

  // Address completion shift register. It is 9 bits wide
  // to account for a 1-bit read/write indicator, 7-bit address, and
  // the last bit is used to determine that this address was just
  // received and the output buffer can be updated
  logic  [8:0] address_complete_sr = 'b0;
  wire         clear_address_sr    = ~reset_n | spi_cs;

  always @(posedge spi_clk or posedge clear_address_sr) begin
    if(clear_address_sr) begin
      address_complete_sr <= 'b0;
    end
    // This shift register is only used on CPLD reads, so we can
    // ignore it when in SPI passthrough mode.
    else if(internal_spi_access) begin
      address_complete_sr <= {address_complete_sr[7:0], 1'b1};
    end
  end

  // Register readback.
  always @(posedge spi_cs or posedge spi_clk) begin
    // Readback data shift register is cleared asynchronously at the end
    // of a transaction.
    if(spi_cs) begin
      cpld_spi_data_out <= 'b0;
    end
    else if(internal_spi_access) begin
      // When address has been fully received, update output
      // buffer based on the address.
      if (!address_complete_sr[8] && address_complete_sr[7]) begin
        cpld_spi_data_out <= 'b0;
        case (spi_buf[6:0])
          TX_CONTROL: begin
            // Handle TX control register read
            cpld_spi_data_out <= tx_fe_ctrl;
          end
          RX_CONTROL: begin
            // Handle RX control register read
            cpld_spi_data_out <= rx_fe_ctrl;
          end
          REVISION_REGISTER: begin
            // Handle revision register read
            cpld_spi_data_out[REVISION_MAJOR_MSB:REVISION_MAJOR] <=
              REVISION_MAJOR_VAL[REVISION_MAJOR_SIZE-1:0];
            cpld_spi_data_out[REVISION_MINOR_MSB:REVISION_MINOR] <=
              REVISION_MINOR_VAL[REVISION_MINOR_SIZE-1:0];
          end
          SCRATCH_REGISTER: begin
            // Handle scratch register read
            cpld_spi_data_out[SCRATCH_DATA_MSB:SCRATCH_DATA] <= scratch_data;
          end
          default: begin
            // Do nothing
            ;
          end
        endcase
      end
      else begin
        // keep shifting data out buffer
        cpld_spi_data_out <= {cpld_spi_data_out[22:0], 1'b0};
      end
    end
  end

  // SPI output
  assign spi_sdo = (internal_spi_access) ? cpld_spi_data_out[23] : 1'bz;

endmodule

`default_nettype wire
