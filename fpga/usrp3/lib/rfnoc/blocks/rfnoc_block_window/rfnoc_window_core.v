//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_window_core
//
// Description:
//
//   This module contains the registers and window module for a single RFNoC
//   window module instance.
//
// Parameters:
//
//   MAX_WINDOW_SIZE : Maximum window size to support, in number of samples.
//   COEFF_WIDTH     : Width of the coefficients to use.
//

`default_nettype none


module rfnoc_window_core #(
  parameter MAX_WINDOW_SIZE = 4096,
  parameter COEFF_WIDTH     = 16
) (
  input wire clk,
  input wire rst,

  // CtrlPort Slave
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  // Input data stream
  input  wire [31:0] s_tdata,
  input  wire        s_tlast,
  input  wire        s_tvalid,
  output wire        s_tready,

  // Output data stream
  output wire [31:0] m_tdata,
  output wire        m_tlast,
  output wire        m_tvalid,
  input  wire        m_tready
);

  `include "rfnoc_block_window_regs.vh"

  // The maximum window size is 2**WINDOW_SIZE
  localparam WINDOW_SIZE = $clog2(MAX_WINDOW_SIZE);

  // WINDOW_SIZE_W is the number of bits needed to represent 2**WINDOW_SIZE.
  localparam WINDOW_SIZE_W = $clog2(MAX_WINDOW_SIZE+1);


  //-----------------------------------------------------------------------
  // Registers
  //-----------------------------------------------------------------------

  reg [WINDOW_SIZE_W-1:0] window_size;
  reg [  COEFF_WIDTH-1:0] m_axis_coeff_tdata;
  reg                     m_axis_coeff_tlast;
  reg                     m_axis_coeff_tvalid;

  always @(posedge clk) begin
    if (rst) begin
      window_size         <= 'bX;
      m_axis_coeff_tdata  <= 'bX;
      m_axis_coeff_tlast  <= 0;
      m_axis_coeff_tvalid <= 0;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack  <= 0;
      s_ctrlport_resp_data <= 0;
      m_axis_coeff_tlast   <= 0;
      m_axis_coeff_tvalid  <= 0;

      // Handle register writes
      if (s_ctrlport_req_wr) begin
        s_ctrlport_resp_ack <= 1;
        case (s_ctrlport_req_addr)
          REG_WINDOW_SIZE :
            window_size <= s_ctrlport_req_data[WINDOW_SIZE_W-1:0];
          REG_LOAD_COEFF : begin
            m_axis_coeff_tdata  <= s_ctrlport_req_data[COEFF_WIDTH-1:0];
            m_axis_coeff_tlast  <= 0;
            m_axis_coeff_tvalid <= 1;
          end
          REG_LOAD_COEFF_LAST : begin
            m_axis_coeff_tdata  <= s_ctrlport_req_data[COEFF_WIDTH-1:0];
            m_axis_coeff_tlast  <= 1;
            m_axis_coeff_tvalid <= 1;
          end
        endcase
      end

      // Handle register reads
      if (s_ctrlport_req_rd) begin
        s_ctrlport_resp_ack <= 1;
        case (s_ctrlport_req_addr)
          REG_WINDOW_SIZE :
            s_ctrlport_resp_data[WINDOW_SIZE_W-1:0] <= window_size;
          REG_WINDOW_MAX_SIZE :
            s_ctrlport_resp_data[WINDOW_SIZE_W-1:0] <= MAX_WINDOW_SIZE;
        endcase
      end
    end
  end

  //-----------------------------------------------------------------------
  // Window Instance
  //-----------------------------------------------------------------------

  window #(
    .WINDOW_SIZE (WINDOW_SIZE),
    .COEFF_WIDTH (COEFF_WIDTH)
  ) window_i (
    .clk                 (clk),
    .rst                 (rst),
    .window_size         (window_size),
    .m_axis_coeff_tdata  (m_axis_coeff_tdata),
    .m_axis_coeff_tlast  (m_axis_coeff_tlast),
    .m_axis_coeff_tvalid (m_axis_coeff_tvalid),
    .m_axis_coeff_tready (),         // Window block is always ready
    .i_tdata             (s_tdata),
    .i_tlast             (s_tlast),
    .i_tvalid            (s_tvalid),
    .i_tready            (s_tready),
    .o_tdata             (m_tdata),
    .o_tlast             (m_tlast),
    .o_tvalid            (m_tvalid),
    .o_tready            (m_tready)
  );

endmodule


`default_nettype wire
