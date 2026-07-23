//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_timer
//
// Description:
//   SystemVerilog wrapper for ctrlport_timer that provides translation
//   between ctrlport interface signals and the individual port signals
//   used by the Verilog ctrlport_timer module.
//
//   This wrapper takes ctrlport interfaces and breaks them down into
//   individual signals for the underlying Verilog module, then
//   reassembles the response back into the interface format.
//
// Parameters:
//   - EXEC_LATE_CMDS : If EXEC_LATE_CMDS = 0 and a command is late, then a
//                      TSERR response is returned. If EXEC_LATE_CMDS = 1, then
//                      a late command will be passed to the output as if it
//                      were on time.

module ctrlport_if_timer #(
  parameter bit EXEC_LATE_CMDS = 1
)(
  // Clocks and Resets
  input  logic        clk,
  input  logic        rst,

  // Timestamp (synchronous to clk)
  input  logic [63:0] time_now,
  input  logic        time_now_stb,
  input  logic [3:0]  time_ignore_bits,

  // Control Port Slave Interface (Input - with timing)
  ctrlport_if.slave   s_ctrlport,

  // Control Port Master Interface (Output - without timing)
  ctrlport_if.master  m_ctrlport
);

  // Import the ctrlport package for type definitions
  import ctrlport_pkg::*;

  // Internal signals for connecting to the Verilog module
  logic         s_ctrlport_req_wr;
  logic         s_ctrlport_req_rd;
  logic [19:0]  s_ctrlport_req_addr;
  logic [31:0]  s_ctrlport_req_data;
  logic [3:0]   s_ctrlport_req_byte_en;
  logic         s_ctrlport_req_has_time;
  logic [63:0]  s_ctrlport_req_time;

  logic         s_ctrlport_resp_ack;
  logic [1:0]   s_ctrlport_resp_status;
  logic [31:0]  s_ctrlport_resp_data;

  logic         m_ctrlport_req_wr;
  logic         m_ctrlport_req_rd;
  logic [19:0]  m_ctrlport_req_addr;
  logic [31:0]  m_ctrlport_req_data;
  logic [3:0]   m_ctrlport_req_byte_en;

  logic         m_ctrlport_resp_ack;
  logic [1:0]   m_ctrlport_resp_status;
  logic [31:0]  m_ctrlport_resp_data;

  // =========================================================================
  // Input Interface Signal Translation (ctrlport_if to individual signals)
  // =========================================================================

  // Extract request signals from slave interface
  assign s_ctrlport_req_wr       = s_ctrlport.req.wr;
  assign s_ctrlport_req_rd       = s_ctrlport.req.rd;
  assign s_ctrlport_req_addr     = s_ctrlport.req.addr;
  assign s_ctrlport_req_data     = s_ctrlport.req.data;
  assign s_ctrlport_req_byte_en  = s_ctrlport.req.byte_en;
  assign s_ctrlport_req_has_time = s_ctrlport.req.has_time;
  assign s_ctrlport_req_time     = s_ctrlport.req.timestamp;

  // Assemble response signals to slave interface
  assign s_ctrlport.resp.ack     = s_ctrlport_resp_ack;
  assign s_ctrlport.resp.status  = ctrlport_status_t'(s_ctrlport_resp_status);
  assign s_ctrlport.resp.data    = s_ctrlport_resp_data;

  // =========================================================================
  // Output Interface Signal Translation (individual signals to ctrlport_if)
  // =========================================================================

  // Assemble request signals to master interface
  assign m_ctrlport.req.wr           = m_ctrlport_req_wr;
  assign m_ctrlport.req.rd           = m_ctrlport_req_rd;
  assign m_ctrlport.req.addr         = m_ctrlport_req_addr;
  assign m_ctrlport.req.data         = m_ctrlport_req_data;
  assign m_ctrlport.req.byte_en      = m_ctrlport_req_byte_en;
  // Timer output doesn't have timing signals
  assign m_ctrlport.req.has_time     = 1'b0;
  assign m_ctrlport.req.timestamp    = 64'h0;
  // Pass through other fields from input (these are not modified by timer)
  assign m_ctrlport.req.port_id      = s_ctrlport.req.port_id;
  assign m_ctrlport.req.remote_epid  = s_ctrlport.req.remote_epid;
  assign m_ctrlport.req.remote_portid = s_ctrlport.req.remote_portid;

  // Extract response signals from master interface
  assign m_ctrlport_resp_ack    = m_ctrlport.resp.ack;
  assign m_ctrlport_resp_status = m_ctrlport.resp.status;
  assign m_ctrlport_resp_data   = m_ctrlport.resp.data;

  // =========================================================================
  // Instantiate the Verilog ctrlport_timer module
  // =========================================================================

  ctrlport_timer #(
    .EXEC_LATE_CMDS(EXEC_LATE_CMDS)
  ) ctrlport_timer_inst (
    // Clocks and Resets
    .clk                     (clk),
    .rst                     (rst),

    // Timestamp
    .time_now                (time_now),
    .time_now_stb            (time_now_stb),
    .time_ignore_bits        (time_ignore_bits),

    // Control Port Slave (Request)
    .s_ctrlport_req_wr       (s_ctrlport_req_wr),
    .s_ctrlport_req_rd       (s_ctrlport_req_rd),
    .s_ctrlport_req_addr     (s_ctrlport_req_addr),
    .s_ctrlport_req_data     (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (s_ctrlport_req_byte_en),
    .s_ctrlport_req_has_time (s_ctrlport_req_has_time),
    .s_ctrlport_req_time     (s_ctrlport_req_time),

    // Control Port Slave (Response)
    .s_ctrlport_resp_ack     (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (s_ctrlport_resp_status),
    .s_ctrlport_resp_data    (s_ctrlport_resp_data),

    // Control Port Master (Request)
    .m_ctrlport_req_wr       (m_ctrlport_req_wr),
    .m_ctrlport_req_rd       (m_ctrlport_req_rd),
    .m_ctrlport_req_addr     (m_ctrlport_req_addr),
    .m_ctrlport_req_data     (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en  (m_ctrlport_req_byte_en),

    // Control Port Master (Response)
    .m_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status  (m_ctrlport_resp_status),
    .m_ctrlport_resp_data    (m_ctrlport_resp_data)
  );

endmodule : ctrlport_if_timer
