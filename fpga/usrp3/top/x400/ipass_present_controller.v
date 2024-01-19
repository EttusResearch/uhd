//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ipass_present_controller
//
// Description:
//
//   Translate the iPass present signals on FPGA to control port requests in
//   order to transfer them to the MB CPLD, which needs it for PCIe reset
//   generation.
//

`default_nettype none


module ipass_present_controller (
  // Common ControlPort signals
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // ControlPort request
  output  reg        m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,
  output wire [ 3:0] m_ctrlport_req_byte_en,

  // ControlPort response
  input  wire        m_ctrlport_resp_ack,
  input  wire [ 1:0] m_ctrlport_resp_status,
  input  wire [31:0] m_ctrlport_resp_data,

  // Configuration
  input  wire        enable,

  // Asynchronous ipass present signals
  input  wire [ 1:0] ipass_present_n
);

`include "regmap/pl_cpld_regmap_utils.vh"
  `ifdef X440
    `include "cpld/regmap/x440/mb_cpld_pl_regmap_utils.vh"
  `else // Use X410 as the default variant for regmap.
    `include "cpld/regmap/x410/mb_cpld_pl_regmap_utils.vh"
  `endif
`include "cpld/regmap/pl_cpld_base_regmap_utils.vh"
`include "../../lib/rfnoc/core/ctrlport.vh"

//-----------------------------------------------------------------------------
// Transfer iPass signals to local clock domain
//-----------------------------------------------------------------------------

wire [1:0] ipass_present;
wire [1:0] ipass_present_lcl;

assign ipass_present = ~ipass_present_n;

synchronizer #(
  .WIDTH            (2),
  .STAGES           (2),
  .INITIAL_VAL      (0),
  .FALSE_PATH_TO_IN (1)
) synchronizer_i (
  .clk (ctrlport_clk),
  .rst (ctrlport_rst),
  .in  (ipass_present),
  .out (ipass_present_lcl)
); 

//-----------------------------------------------------------------------------
// Logic to wait for response after trigging request
//-----------------------------------------------------------------------------

reg transfer_in_progress  = 1'b0;
reg error_occurred        = 1'b0;
reg enable_delayed        = 1'b0;
reg [1:0] ipass_present_cached = 2'b0;

// Rising-edge detection on enable signal
wire activated;
assign activated = enable & ~enable_delayed;

always @(posedge ctrlport_clk) begin
  if (ctrlport_rst) begin
    m_ctrlport_req_wr    <= 1'b0;
    transfer_in_progress <= 1'b0;
    enable_delayed       <= 1'b0;
    error_occurred       <= 1'b0;
    ipass_present_cached <= 2'b0;
  end else begin
    // Default assignment
    m_ctrlport_req_wr <= 1'b0;
    enable_delayed    <= enable;

    // Issue new request on change if no request is pending
    if (((ipass_present_lcl != ipass_present_cached) || error_occurred || activated)
      && ~transfer_in_progress && enable) begin
      transfer_in_progress <= 1'b1;
      m_ctrlport_req_wr    <= 1'b1;
      ipass_present_cached <= ipass_present_lcl;
    end

    // Reset pending request
    if (m_ctrlport_resp_ack) begin
      transfer_in_progress <= 1'b0;
      error_occurred       <= m_ctrlport_resp_status != CTRL_STS_OKAY;
    end
  end
end

//-----------------------------------------------------------------------------
// Static ControlPort assignments
//-----------------------------------------------------------------------------

assign m_ctrlport_req_rd      = 1'b0;
assign m_ctrlport_req_byte_en = 4'b1111;
assign m_ctrlport_req_addr    = MB_CPLD + PL_REGISTERS + CABLE_PRESENT_REG;
assign m_ctrlport_req_data    = 32'b0 |
                                (ipass_present_cached[0] << IPASS0_CABLE_PRESENT) |
                                (ipass_present_cached[1] << IPASS1_CABLE_PRESENT);

endmodule


`default_nettype wire
