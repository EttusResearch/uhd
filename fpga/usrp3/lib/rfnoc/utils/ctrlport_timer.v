//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_timer
// Description:
//   The Control-Port timer module converts an asynchronous timed
//   transaction into a synchronous blocking transaction. This
//   module will use the input req_has_time and req_time fields and
//   produce an output transaction that will execute when the requested
//   time is current. The module does not pass the has_time and time
//   signals out because they are no longer relevant. The current time
//   is an input to this module, and must be a monotonic counter that
//   updates every time the time strobe is asserted.
//
// Parameters:
//   - PRECISION_BITS : The number of bits to ignore when performing a 
//                      time comparison to determine execution time.
//   - EXEC_LATE_CMDS : If a command is late, a TSERR response is sent.
//                      If EXEC_LATE_CMDS = 1, then the late command will
//                      be passed to the output regardless of the TSERR.
//
// Signals:
//   - time_now*      : The time_now signal is the current time and the stb
//                      signal indicates that the time_now is valid.
//   - s_ctrlport_*   : The slave Control-Port bus.
//                      This must have the has_time and time signals.
//   - m_ctrlport_*   : The master Control-Port bus.
//                      This will not have the has_time and time signals.

module ctrlport_timer #(
  parameter       PRECISION_BITS = 0,
  parameter [0:0] EXEC_LATE_CMDS = 0
)(
  // Clocks and Resets
  input  wire         clk,
  input  wire         rst,
  // Timestamp (synchronous to clk)
  input  wire [63:0]  time_now,
  input  wire         time_now_stb,
  // Control Port Master (Request)
  input  wire         s_ctrlport_req_wr,
  input  wire         s_ctrlport_req_rd,
  input  wire [19:0]  s_ctrlport_req_addr,
  input  wire [31:0]  s_ctrlport_req_data,
  input  wire [3:0]   s_ctrlport_req_byte_en,
  input  wire         s_ctrlport_req_has_time,
  input  wire [63:0]  s_ctrlport_req_time,
  // Control Port Slave (Response)
  output wire         s_ctrlport_resp_ack,
  output wire [1:0]   s_ctrlport_resp_status,
  output wire [31:0]  s_ctrlport_resp_data,
  // Control Port Master (Request)
  output wire         m_ctrlport_req_wr,
  output wire         m_ctrlport_req_rd,
  output wire [19:0]  m_ctrlport_req_addr,
  output wire [31:0]  m_ctrlport_req_data,
  output wire [3:0]   m_ctrlport_req_byte_en,
  // Control Port Master (Response)
  input  wire         m_ctrlport_resp_ack,
  input  wire [1:0]   m_ctrlport_resp_status,
  input  wire [31:0]  m_ctrlport_resp_data
);

  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_axis_ctrl_utils.vh"

  // Control triggers:
  // - pending: A command is waiting on the input port
  // - ontime: The timed command is due for execution (on time)
  // - late: The timed command is late
  // - exec: Execute the command (pass it to the output)
  // - consume: Consume the input command
  wire         pending, ontime, late, exec, consume;
  // Cached values for input command
  wire         cached_req_wr, cached_req_rd;
  wire [19:0]  cached_req_addr;
  wire [31:0]  cached_req_data;
  wire [3:0]   cached_req_byte_en;
  wire         cached_req_has_time;
  wire [63:0]  cached_req_time;

  axi_fifo_flop #(.WIDTH(1+1+20+32+4+1+64)) req_cache_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({s_ctrlport_req_wr, s_ctrlport_req_rd, s_ctrlport_req_addr, s_ctrlport_req_data,
              s_ctrlport_req_byte_en, s_ctrlport_req_has_time, s_ctrlport_req_time}),
    .i_tvalid(s_ctrlport_req_wr | s_ctrlport_req_rd), .i_tready(),
    .o_tdata({cached_req_wr, cached_req_rd, cached_req_addr, cached_req_data,
              cached_req_byte_en, cached_req_has_time, cached_req_time}),
    .o_tvalid(pending), .o_tready(consume),
    .occupied(), .space()
  );

  // Command is on time
  assign ontime = cached_req_has_time && pending && time_now_stb && 
    (cached_req_time[63:PRECISION_BITS] == time_now[63:PRECISION_BITS]);
  // Command is late
  assign late = cached_req_has_time && pending && time_now_stb && 
    (cached_req_time[63:PRECISION_BITS] < time_now[63:PRECISION_BITS]);
  // Logic to pass cmd forward
  assign exec = pending && (!cached_req_has_time || ontime || (EXEC_LATE_CMDS && late));
  assign consume = exec || late;

  assign m_ctrlport_req_wr      = cached_req_wr & exec;
  assign m_ctrlport_req_rd      = cached_req_rd & exec;
  assign m_ctrlport_req_addr    = cached_req_addr;
  assign m_ctrlport_req_data    = cached_req_data;
  assign m_ctrlport_req_byte_en = cached_req_byte_en;

  wire [1:0] resp_status = (late && !exec) ? AXIS_CTRL_STS_TSERR : m_ctrlport_resp_status;
  axi_fifo_flop #(.WIDTH(2+32)) resp_cache_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({resp_status, m_ctrlport_resp_data}),
    .i_tvalid(m_ctrlport_resp_ack || (late && !exec)), .i_tready(),
    .o_tdata({s_ctrlport_resp_status, s_ctrlport_resp_data}),
    .o_tvalid(s_ctrlport_resp_ack), .o_tready(s_ctrlport_resp_ack),
    .occupied(), .space()
  );

endmodule // ctrlport_timer

