//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_timer
//
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
//   - EXEC_LATE_CMDS : If EXEC_LATE_CMDS = 0 and a command is late, then a
//                      TSERR response is returned. If EXEC_LATE_CMDS = 1, then
//                      a late command will be passed to the output as if it
//                      were on time.
//
// Signals:
//   - time_now*        : The time_now signal is the current time and the stb
//                        signal indicates that the time_now is valid.
//   - time_ignore_bits : Number of least-significant bits to ignore when doing
//                        time comparisons. This should be a constant.
//   - s_ctrlport_*     : The slave Control-Port bus.
//                        This must have the has_time and time signals.
//   - m_ctrlport_*     : The master Control-Port bus.
//                        This will not have the has_time and time signals.

module ctrlport_timer #(
  parameter [0:0] EXEC_LATE_CMDS = 1
)(
  // Clocks and Resets
  input  wire         clk,
  input  wire         rst,
  // Timestamp (synchronous to clk)
  input  wire [63:0]  time_now,
  input  wire         time_now_stb,
  input  wire [3:0]   time_ignore_bits,
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
  // - has_time: The command is timed
  // - on_time: The timed command is due for execution (on time)
  // - late: The timed command is late
  // - exec: Execute the command (pass it to the output)
  // - consume: Consume the input command
  reg          pending, has_time, on_time, late;
  wire         exec, consume, valid;
  // Cached values for input command
  wire         cached_req_wr, cached_req_rd;
  wire [19:0]  cached_req_addr;
  wire [31:0]  cached_req_data;
  wire [3:0]   cached_req_byte_en;
  wire         cached_req_has_time;
  wire [63:0]  cached_req_time;
  wire         cached_req_valid;

  axi_fifo_flop #(.WIDTH(1+1+20+32+4+1+64)) req_cache_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({s_ctrlport_req_wr, s_ctrlport_req_rd, s_ctrlport_req_addr, s_ctrlport_req_data,
              s_ctrlport_req_byte_en, s_ctrlport_req_has_time, s_ctrlport_req_time}),
    .i_tvalid(s_ctrlport_req_wr | s_ctrlport_req_rd), .i_tready(),
    .o_tdata({cached_req_wr, cached_req_rd, cached_req_addr, cached_req_data,
              cached_req_byte_en, cached_req_has_time, cached_req_time}),
    .o_tvalid(cached_req_valid), .o_tready(consume),
    .occupied(), .space()
  );

  // Create a mask so that we can ignore the lower time_ignore_bits of the time.
  wire [63:0] mask = (~64'd0 << time_ignore_bits);

  // Pipeline registers
  reg        has_time_0;
  reg        has_time_1;
  reg        valid_0;
  reg        valid_1;
  reg [63:0] time_0;
  reg        time_stb_1, time_stb_0;
  reg [63:0] req_time_0;
  reg        on_time_1;
  reg        late_1;

  // This always block pipelines the time comparisons and other condition
  // checks, so we don't have a long combinational path here. This adds some
  // delay to the execution of timed commands, but this block is not intended
  // for cycle-accurate register updates.
  always @(posedge clk) begin
    if (rst || consume) begin
      has_time_0 <= 'bX;
      time_stb_0 <= 'bX;
      time_0     <= 'bX;
      req_time_0 <= 'bX;
      has_time_1 <= 'bX;
      time_stb_1 <= 'bX;
      on_time_1  <= 'bX;
      late_1     <= 'bX;
      has_time   <= 'bX;
      on_time    <= 'bX;
      // These need to be zeroed, to clear the pipeline:
      valid_0    <= 1'b0;
      valid_1    <= 1'b0;
      pending    <= 1'b0;
      late       <= 1'b0;
    end else begin
      // Stage 0
      valid_0    <= cached_req_valid;
      has_time_0 <= cached_req_has_time;
      time_stb_0 <= time_now_stb;
      time_0     <= time_now & mask;
      req_time_0 <= cached_req_time & mask;

      // Stage 1
      valid_1    <= valid_0;
      has_time_1 <= has_time_0;
      time_stb_1 <= time_stb_0;
      on_time_1  <= req_time_0 == time_0;
      late_1     <= req_time_0 <  time_0;

      // Stage 3
      pending  <= valid_1;
      has_time <= has_time_1;
      on_time  <= valid_1 && has_time_1 && time_stb_1 && on_time_1;
      late     <= valid_1 && has_time_1 && time_stb_1 && late_1;
    end
  end

  // Logic to pass cmd forward
  assign exec    = pending && (!has_time || on_time || (EXEC_LATE_CMDS && late));
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

