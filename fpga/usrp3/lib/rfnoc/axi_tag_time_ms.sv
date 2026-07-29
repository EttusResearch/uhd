//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_tag_time_ms
//
// Description:
//   Tags individual samples within a multi-sample-per-clock word based on
//   timed control port commands. For each timed write to REG_TAG_ADDR, the module
//   determines which sample in the output word corresponds to the command
//   timestamp and asserts the corresponding bit in m_axis_dout_ttags when the
//   current timestamp reaches the commanded time. Untimed commands are passed
//   through immediately on m_axis_cmd_tdata/tvalid.
// NOTE:
//   This module has a command stream latency of 4 cycles:
//   1 cycle - add command to FIFO,
//   1 cycle - tag generation,
//   1 cycle - timestamp check,
//   1 cycle - pop command FIFO and AXI-Stream output
//   Ensure there is a 4-cycle gap between two consecutive commands to ensure
//   command is applied to the correct sample in the output word.
//
// Parameters:
//   SPC           - Samples per clock word. Must be a power of 2
//   SAMP_W        - Width of each sample in bits
//   CMD_FIFO_SIZE - Log2 depth of the timed-command holding FIFOs
//   TICK_RATE_W   - Width of TICKS_PER_SAMPLE in bits
//   CMD_DATA_W    - Width of m_axis_cmd_tdata; Defaults to CTRLPORT_DATA_W
//   MSB_ALIGN     - Left align ctrlport data


`default_nettype none

module axi_tag_time_ms
  import ctrlport_pkg::*;
  import rfnoc_chdr_utils_pkg::*;
#(
  parameter int SPC           = 1,
  parameter int SAMP_W        = 32,
  parameter int CMD_FIFO_SIZE = 5,
  parameter int TICK_RATE_W   = 16,
  parameter int CMD_DATA_W    = CTRLPORT_DATA_W,
  parameter int MSB_ALIGN     = 0
) (
  input wire logic clk,
  input wire logic rst,

  output     logic                         cmd_fifo_full,

  // Input sample stream
  input wire logic [ SPC-1:0][SAMP_W-1:0]  s_axis_din_tdata,
  input wire logic                         s_axis_din_tlast,
  input wire logic                         s_axis_din_tvalid,
  input wire logic                         s_axis_din_thas_time,
  input wire logic [CHDR_TIMESTAMP_W-1:0]  s_axis_din_ttimestamp,
  input wire logic                         s_axis_din_teob,
  input wire logic [CHDR_LENGTH_W-1:0]     s_axis_din_tlength,
  output     logic                         s_axis_din_tready,

  // Output sample stream
  output     logic [ SPC-1:0][SAMP_W-1:0]  m_axis_dout_tdata,
  output     logic [             SPC-1:0]  m_axis_dout_ttags,
  output     logic                         m_axis_dout_tlast,
  output     logic                         m_axis_dout_tvalid,
  output     logic                         m_axis_dout_thas_time,
  output     logic [CHDR_TIMESTAMP_W-1:0]  m_axis_dout_ttimestamp,
  output     logic                         m_axis_dout_teob,
  output     logic [   CHDR_LENGTH_W-1:0]  m_axis_dout_tlength,
  input wire logic                         m_axis_dout_tready,

  // Control port interface to write register value
  input wire logic                          ctrlport_req_wr,
  input wire logic                          ctrlport_req_rd,
  input wire logic [   CTRLPORT_ADDR_W-1:0] ctrlport_req_addr,
  input wire logic [   CTRLPORT_DATA_W-1:0] ctrlport_req_data,
  input wire logic                          ctrlport_req_has_time,
  input wire logic [   CTRLPORT_TIME_W-1:0] ctrlport_req_time,
  output     logic                          ctrlport_resp_ack,
  output     logic [   CTRLPORT_DATA_W-1:0] ctrlport_resp_data,
  output     ctrlport_status_t              ctrlport_resp_status,

  // Stream out register write commands
  output     logic [        CMD_DATA_W-1:0] m_axis_cmd_tdata,
  output     logic                          m_axis_cmd_tvalid,
  // Propagate has_time on tuser
  output     logic                          m_axis_cmd_tuser,
  input wire logic                          m_axis_cmd_tready
);

  import axi_tag_time_ms_pkg::*;

  //------------------------------------------------------------------
  // Localparams
  //------------------------------------------------------------------
  // CMD FIFO = timestamp + data + has_time
  localparam int CMD_FIFO_WIDTH   = CTRLPORT_TIME_W + CMD_DATA_W + 1;
  // Data FIFO = data + timestamp + tlast, thas_time, teob + tlength
  localparam int DATA_FIFO_WIDTH  = SPC*SAMP_W + CHDR_TIMESTAMP_W + 3 + CHDR_LENGTH_W;
  localparam int SPC_LOG2         = $clog2(SPC);

  // Number of ticks per sample used to increment timestamp for each word
  // Current design works for only 1 tick per sample.
  // This makes timestamp increment always a power of 2 (depends only on SPC,
  // also currently only power of 2) and simplifes computing sample tag for
  // command timestamp.
  localparam logic [TICK_RATE_W-1:0] TICKS_PER_SAMPLE = 1;

  // Track FIFO full status
  typedef enum logic { ST_IDLE, ST_FULL } state_t;
  state_t current_state, next_state;

  //------------------------------------------------------------------
  // Time tracking
  //------------------------------------------------------------------
  logic                        start_of_burst = 1'b1;
  logic [CHDR_TIMESTAMP_W-1:0] current_timestamp;
  logic [CHDR_TIMESTAMP_W-1:0] timestamp_incr;
  logic [CHDR_TIMESTAMP_W-1:0] mod_time_incr;
  always_ff @(posedge clk) begin
    timestamp_incr <= TICKS_PER_SAMPLE * SPC;
    mod_time_incr  <= timestamp_incr - 1;
  end

  // Estimate timestamp of words in packet
  // Only first packet in burst is expected to have a timestamp, if timed.
  always_ff @(posedge clk) begin
    if (rst) begin
      start_of_burst    <= 1'b1;
      current_timestamp <= '0;
    end else if (s_axis_din_tvalid && s_axis_din_tready) begin
      start_of_burst    <= s_axis_din_tlast && s_axis_din_teob;
      if (start_of_burst) begin
        if (s_axis_din_thas_time) current_timestamp <= s_axis_din_ttimestamp;
        else                      current_timestamp <= '0;
      end else begin
        current_timestamp <= current_timestamp + timestamp_incr;
      end
    end
  end

  //------------------------------------------------------------------
  // Ctrlport request handling
  //------------------------------------------------------------------
  logic [CMD_DATA_W-1:0]      ctrlport_data_reg;
  logic [CTRLPORT_TIME_W-1:0] ctrlport_time;
  logic                       add_to_fifo, timed_req;
  logic                       send_ack;
  logic                       cmd_fifo_full_n;

  // State machine to handle ctrlport requests and add to FIFO
  always_ff @(posedge clk) begin
    if (rst) begin
      ctrlport_resp_ack    <= 1'b0;
      ctrlport_resp_data   <= '0;
      ctrlport_data_reg    <= '0;
      ctrlport_resp_status <= STS_OKAY;
      current_state        <= ST_IDLE;
    end else begin
      add_to_fifo          <= 1'b0;
      timed_req            <= 1'b0;
      ctrlport_time        <= '0;
      ctrlport_resp_data   <= '0;
      current_state        <= next_state;
      ctrlport_resp_ack    <= send_ack;
      if (ctrlport_req_wr) begin
        if (ctrlport_req_addr == REG_TAG_ADDR) begin
          if (MSB_ALIGN) ctrlport_data_reg <= ctrlport_req_data[CTRLPORT_DATA_W-1 -: CMD_DATA_W];
          else           ctrlport_data_reg <= ctrlport_req_data;
          add_to_fifo          <= 1'b1;
          timed_req            <= ctrlport_req_has_time;
          ctrlport_time        <= ctrlport_req_time;
          ctrlport_resp_status <= STS_OKAY;
        end else begin
          ctrlport_resp_status <= STS_CMDERR;
        end
      end
      // Register is write-only.
      // Returns STS_OKAY for reads to REG_TAG_ADDR but does not return any data.
      if (ctrlport_req_rd) begin
        if (ctrlport_req_addr == REG_TAG_ADDR) begin
          ctrlport_resp_status <= STS_OKAY;
        end else begin
          ctrlport_resp_status <= STS_CMDERR;
        end
      end
    end
  end

  // Send ctrlport_resp_ack when FIFO has space for the next command
  // If FIFO becomes full with current write, hold ack
  // until FIFO has space.
  always_comb begin
    next_state = current_state;
    send_ack   = 1'b0;
    case (current_state)
      ST_IDLE : begin
        if (ctrlport_req_rd ||
            (cmd_fifo_full_n && ctrlport_req_wr)) begin
          send_ack = 1'b1;
        end else if (ctrlport_req_wr) begin
          next_state = ST_FULL;
        end
      end
      ST_FULL : begin
        if (cmd_fifo_full_n) begin
          send_ack   = 1'b1;
          next_state = ST_IDLE;
        end
      end
      default: begin
        next_state = ST_IDLE;
      end
    endcase
  end

  // ------------------------------------------------------------------
  // Add ctrlport commands to a FIFO which is popped when the timestamp
  // reaches the command time or immediately when untimed.
  // ------------------------------------------------------------------

  // FIFO to queue incoming commands
  logic [CMD_DATA_W-1:0]      cmd_data, fifo_data;
  logic [CTRLPORT_TIME_W-1:0] cmd_time, fifo_time;
  logic                       cmd_has_time, fifo_has_time;
  logic                       cmd_valid, cmd_ready;
  logic                       fifo_tvalid, fifo_tready;
  logic                       cmd_write_en;

  axi_fifo #(
    .WIDTH (CMD_FIFO_WIDTH),
    .SIZE  (CMD_FIFO_SIZE)
  ) cmd_data_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  ({timed_req, ctrlport_time, ctrlport_data_reg}),
    .i_tvalid (add_to_fifo),
    .i_tready (cmd_fifo_full_n),
    .o_tdata  ({cmd_has_time, cmd_time, cmd_data}),
    .o_tvalid (cmd_valid),
    .o_tready (cmd_ready),
    .space    (),
    .occupied ()
  );

  // output register after FIFO to simplify timing closure
  axi_fifo #(
    .WIDTH (CMD_FIFO_WIDTH),
    .SIZE  (1)
  ) tag_out_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  ({cmd_has_time, cmd_time, cmd_data}),
    .i_tvalid (cmd_valid),
    .i_tready (cmd_ready),
    .o_tdata  ({fifo_has_time, fifo_time, fifo_data}),
    .o_tvalid (fifo_tvalid),
    .o_tready (fifo_tready),
    .space    (),
    .occupied ()
  );

  //------------------------------------------------------------------
  // Command pop logic
  //------------------------------------------------------------------

  // Pre-register the lookahead offset
  logic                       lookahead;
  logic [CTRLPORT_TIME_W-1:0] fifo_time_threshold;
  logic [SPC-1:0]             sample_alignment;
  logic [SPC-1:0]             sample_tag;

  logic pop_cmd;
  logic data_valid, fifo_has_cmd;
  assign data_valid   = m_axis_dout_tvalid && m_axis_dout_tready;
  assign fifo_has_cmd = fifo_tvalid && !fifo_tready;

  // Compute offset to start of word containing the sample corresponding to the command timestamp
  always_ff @(posedge clk) begin
    // sample alignment = (cmd time - current time) mod timestamp_incr
    // works only when timestamp_incr is a power of 2
    if (fifo_has_cmd && fifo_has_time)
      sample_alignment <= (fifo_time[SPC_LOG2:0] - current_timestamp[SPC_LOG2:0])
                          & mod_time_incr;
    else
      // If command is untimed, it is aligned to start of word.
      sample_alignment <= '0;
  end

  // Generate tags for the word corresponding to the command timestamp
  for (genvar spc_idx = 0; spc_idx < SPC; spc_idx++) begin : gen_sample_tags
    always_ff @(posedge clk) begin
      if (rst) sample_tag[spc_idx] <= '0;
      else
        sample_tag[spc_idx] <= (sample_alignment == spc_idx);
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      lookahead           <= '0;
      fifo_time_threshold <= 'x;
    end
    else begin
      if (data_valid) lookahead <= (fifo_time_threshold <= current_timestamp);
      if (fifo_has_cmd) begin
        if (fifo_has_time) begin
          fifo_time_threshold <= fifo_time
                                - (timestamp_incr << 1)
                                - sample_alignment;
        end else begin
          fifo_time_threshold <= '1;
        end
        if (lookahead && data_valid) begin
          // If we're already past the threshold, set it to 1 to pop immediately on next cycle
          fifo_time_threshold <= '1;
          lookahead           <= 1'b0;
        end
      end else fifo_time_threshold <= '1;
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      pop_cmd <= 1'b0;
    end else if (fifo_tready) begin
      // FIFO was popped (tag delivered), clear the latch
      pop_cmd <= 1'b0;
    end else if (data_valid && fifo_has_cmd && !pop_cmd) begin
      // Look ahead and set to pop on next cycle if command time is reached.
      // Latch once time is reached; hold until fifo_tready clears it
      // so a stall on data_valid doesn't drop the tag.
      pop_cmd <= lookahead;
    end
  end

  assign cmd_fifo_full = !cmd_fifo_full_n;
  assign fifo_tready   = (pop_cmd || !fifo_has_time) && m_axis_cmd_tready && data_valid;
  assign cmd_write_en  = fifo_tready && fifo_tvalid;

  assign m_axis_dout_ttags  = (cmd_write_en && fifo_has_time) ? sample_tag : '0;
  assign m_axis_cmd_tdata  = fifo_data;
  assign m_axis_cmd_tvalid = cmd_write_en;
  assign m_axis_cmd_tuser  = fifo_has_time;

  // Delay data by 1 cycle to allow current timestamp to update
  axi_fifo_flop #(
    .WIDTH (DATA_FIFO_WIDTH)
  ) data_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  ({ s_axis_din_tdata, s_axis_din_tlast, s_axis_din_teob,
              s_axis_din_ttimestamp, s_axis_din_thas_time, s_axis_din_tlength}),
    .i_tvalid (s_axis_din_tvalid),
    .i_tready (s_axis_din_tready),
    .o_tdata  ({ m_axis_dout_tdata, m_axis_dout_tlast, m_axis_dout_teob,
              m_axis_dout_ttimestamp, m_axis_dout_thas_time, m_axis_dout_tlength}),
    .o_tvalid (m_axis_dout_tvalid),
    .o_tready (m_axis_dout_tready),
    .space    (),
    .occupied ()
  );

endmodule
`default_nettype wire
