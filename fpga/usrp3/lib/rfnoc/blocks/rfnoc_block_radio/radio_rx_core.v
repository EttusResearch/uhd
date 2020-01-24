//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: radio_rx_core
//
// Description:
//
// This module contains the core Rx radio acquisition logic. It retrieves 
// sample data from the radio interface, as indicated by the radio's strobe 
// signal, and outputs the data via AXI-Stream.
//
// The receiver is operated by writing a time (optionally) to the 
// REG_RX_CMD_TIME_* registers and a number of words (optionally) to 
// REG_RX_CMD_NUM_WORDS_* registers followed by writing a command word to 
// REG_RX_CMD. The command word indicates whether it is a finite ("num samps 
// and done") or continuous acquisition and whether or not the acquisition 
// should start at the time indicated byREG_RX_CMD_TIME_*. A stop command will 
// stop any acquisition that's waiting to start or is in progress.
//
// The REG_RX_MAX_WORDS_PER_PKT and REG_RX_ERR_* registers should be 
// initialized prior to the first acquisition.
//
// Parameters:
//
//   SAMP_W    : Width of a radio sample
//   NSPC      : Number of radio samples per radio clock cycle
//
`default_nettype none


module radio_rx_core #(
  parameter SAMP_W    = 32,
  parameter NSPC      = 1
) (
  input wire radio_clk,
  input wire radio_rst,


  //---------------------------------------------------------------------------
  // Control Interface
  //---------------------------------------------------------------------------

  // Slave (Register Reads and Writes)
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack  = 1'b0,
  output reg  [31:0] s_ctrlport_resp_data,

  // Master (Error Reporting)
  output reg         m_ctrlport_req_wr = 1'b0,
  output reg  [19:0] m_ctrlport_req_addr,
  output reg  [31:0] m_ctrlport_req_data,
  output wire        m_ctrlport_req_has_time,
  output reg  [63:0] m_ctrlport_req_time,
  output wire [ 9:0] m_ctrlport_req_portid,
  output wire [15:0] m_ctrlport_req_rem_epid,
  output wire [ 9:0] m_ctrlport_req_rem_portid,
  input  wire        m_ctrlport_resp_ack,


  //---------------------------------------------------------------------------
  // Radio Interface
  //---------------------------------------------------------------------------

  input wire [63:0] radio_time,

  input  wire [SAMP_W*NSPC-1:0] radio_rx_data,
  input  wire                   radio_rx_stb,

  // Status indicator (true when receiving)
  output wire radio_rx_running,


  //---------------------------------------------------------------------------
  // AXI-Stream Data Output
  //---------------------------------------------------------------------------

  output wire [SAMP_W*NSPC-1:0] m_axis_tdata,
  output wire                   m_axis_tlast,
  output wire                   m_axis_tvalid,
  input  wire                   m_axis_tready,
  // Sideband info
  output wire [           63:0] m_axis_ttimestamp,
  output wire                   m_axis_thas_time,
  output wire                   m_axis_teob
);

  `include "rfnoc_block_radio_regs.vh"
  `include "../../core/rfnoc_chdr_utils.vh"

  localparam NUM_WORDS_LEN = RX_CMD_NUM_WORDS_LEN;


  //---------------------------------------------------------------------------
  // Register Read/Write Logic
  //---------------------------------------------------------------------------

  reg                      reg_cmd_valid        = 0;  // Indicates when the CMD_FIFO has been written
  reg  [   RX_CMD_LEN-1:0] reg_cmd_word         = 0;  // Command to execute
  reg  [NUM_WORDS_LEN-1:0] reg_cmd_num_words    = 0;  // Number of words for the command
  reg  [             63:0] reg_cmd_time         = 0;  // Time for the command
  reg                      reg_cmd_timed        = 0;  // Indicates if this is a timed command
  reg  [             31:0] reg_max_pkt_len      = 64; // Maximum words per packet
  reg  [              9:0] reg_error_portid     = 0;  // Port ID to use for error reporting
  reg  [             15:0] reg_error_rem_epid   = 0;  // Remote EPID to use for error reporting
  reg  [              9:0] reg_error_rem_portid = 0;  // Remote port ID to use for error reporting
  reg  [             19:0] reg_error_addr       = 0;  // Address to use for error reporting
  reg                      reg_has_time         = 1;  // Whether or not to use timestamps on data

  wire [15:0] cmd_fifo_space;   // Empty space in the command FIFO
  reg         cmd_stop     = 0; // Indicates a full stop request
  wire        cmd_stop_ack;     // Acknowledgment that a stop has completed
  reg         clear_fifo   = 0; // Signal to clear the command FIFO

  assign m_axis_thas_time = reg_has_time;

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      s_ctrlport_resp_ack  <= 0;
      reg_cmd_valid        <= 0;
      reg_cmd_word         <= 0;
      reg_cmd_num_words    <= 0;
      reg_cmd_time         <= 0;
      reg_cmd_timed        <= 0;
      reg_max_pkt_len      <= 64;
      reg_error_portid     <= 0;
      reg_error_rem_epid   <= 0;
      reg_error_rem_portid <= 0;
      reg_error_addr       <= 0;
      reg_has_time         <= 1;
      clear_fifo           <= 0;
      cmd_stop             <= 0;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack  <= 0;
      s_ctrlport_resp_data <= 0;
      reg_cmd_valid        <= 0;
      clear_fifo           <= 0;

      // Clear stop register when we enter the STOP state
      if (cmd_stop_ack) cmd_stop <= 1'b0;

      // Handle register writes
      if (s_ctrlport_req_wr) begin
        case (s_ctrlport_req_addr)
          REG_RX_CMD: begin
            // All commands go into the command FIFO except STOP
            reg_cmd_valid       <= (s_ctrlport_req_data[RX_CMD_LEN-1:0] != RX_CMD_STOP);
            reg_cmd_word        <=  s_ctrlport_req_data[RX_CMD_LEN-1:0];
            reg_cmd_timed       <=  s_ctrlport_req_data[RX_CMD_TIMED_POS];
            s_ctrlport_resp_ack <= 1;

            // cmd_stop must remain asserted until it has completed
            if (!cmd_stop || cmd_stop_ack) begin
              cmd_stop <= (s_ctrlport_req_data[RX_CMD_LEN-1:0] == RX_CMD_STOP);
            end
            clear_fifo <= (s_ctrlport_req_data[RX_CMD_LEN-1:0] == RX_CMD_STOP);
          end
          REG_RX_CMD_NUM_WORDS_LO: begin
            reg_cmd_num_words[31:0] <= s_ctrlport_req_data;
            s_ctrlport_resp_ack     <= 1;
          end
          REG_RX_CMD_NUM_WORDS_HI: begin
            reg_cmd_num_words[NUM_WORDS_LEN-1:32] <= s_ctrlport_req_data[NUM_WORDS_LEN-32-1:0];
            s_ctrlport_resp_ack                   <= 1;
          end
          REG_RX_CMD_TIME_LO: begin
            reg_cmd_time[31:0]  <= s_ctrlport_req_data;
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_CMD_TIME_HI: begin
            reg_cmd_time[63:32] <= s_ctrlport_req_data;
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_MAX_WORDS_PER_PKT: begin
            reg_max_pkt_len     <= s_ctrlport_req_data;
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_ERR_PORT: begin
            reg_error_portid    <= s_ctrlport_req_data[9:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_ERR_REM_PORT: begin
            reg_error_rem_portid <= s_ctrlport_req_data[9:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_RX_ERR_REM_EPID: begin
            reg_error_rem_epid  <= s_ctrlport_req_data[15:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_ERR_ADDR: begin
            reg_error_addr      <= s_ctrlport_req_data[19:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_HAS_TIME: begin
            reg_has_time        <= s_ctrlport_req_data[0:0];
            s_ctrlport_resp_ack <= 1;
          end
        endcase
      end

      // Handle register reads
      if (s_ctrlport_req_rd) begin
        case (s_ctrlport_req_addr)
          REG_RX_STATUS: begin
            s_ctrlport_resp_data[CMD_FIFO_SPACE_POS+:CMD_FIFO_SPACE_LEN] 
                                <= cmd_fifo_space[CMD_FIFO_SPACE_LEN-1:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_RX_CMD: begin
            s_ctrlport_resp_data[RX_CMD_LEN-1:0]   <= reg_cmd_word;
            s_ctrlport_resp_data[RX_CMD_TIMED_POS] <= reg_cmd_timed;
            s_ctrlport_resp_ack                    <= 1;
          end
          REG_RX_CMD_NUM_WORDS_LO: begin
            s_ctrlport_resp_data <= reg_cmd_num_words[31:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_RX_CMD_NUM_WORDS_HI: begin
            s_ctrlport_resp_data[NUM_WORDS_LEN-32-1:0] <= reg_cmd_num_words[NUM_WORDS_LEN-1:32];
            s_ctrlport_resp_ack                        <= 1;
          end
          REG_RX_CMD_TIME_LO: begin
            s_ctrlport_resp_data <= reg_cmd_time[31:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_RX_CMD_TIME_HI: begin
            s_ctrlport_resp_data <= reg_cmd_time[63:32];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_RX_MAX_WORDS_PER_PKT: begin
            s_ctrlport_resp_data <= reg_max_pkt_len;
            s_ctrlport_resp_ack  <= 1;
          end
          REG_RX_ERR_PORT: begin
            s_ctrlport_resp_data[9:0] <= reg_error_portid;
            s_ctrlport_resp_ack       <= 1;
          end
          REG_RX_ERR_REM_PORT: begin
            s_ctrlport_resp_data[9:0] <= reg_error_rem_portid;
            s_ctrlport_resp_ack       <= 1;
          end
          REG_RX_ERR_REM_EPID: begin
            s_ctrlport_resp_data[15:0] <= reg_error_rem_epid;
            s_ctrlport_resp_ack        <= 1;
          end
          REG_RX_ERR_ADDR: begin
            s_ctrlport_resp_data[19:0] <= reg_error_addr;
            s_ctrlport_resp_ack        <= 1;
          end
          REG_RX_DATA: begin
            s_ctrlport_resp_data <= radio_rx_data;
            s_ctrlport_resp_ack  <= 1;
          end
          REG_RX_HAS_TIME: begin
            s_ctrlport_resp_data[0] <= reg_has_time;
            s_ctrlport_resp_ack     <= 1;
          end
        endcase
      end

    end
  end


  //---------------------------------------------------------------------------
  // Command Queue
  //---------------------------------------------------------------------------

  wire [             63:0] cmd_time;        // Time for next start of command
  wire                     cmd_timed;       // Command is timed (use cmd_time)
  wire [NUM_WORDS_LEN-1:0] cmd_num_words;   // Number of words for next command
  wire                     cmd_continuous;  // Command is continuous (ignore cmd_num_words)
  wire                     cmd_valid;       // cmd_* is a valid command
  wire                     cmd_done;        // Command has completed and can be popped from FIFO

  axi_fifo #(
    .WIDTH (64 + 1 + NUM_WORDS_LEN + 1),
    .SIZE  (5)      // Ideally, this size will lead to an SRL-based FIFO
  ) cmd_fifo (
    .clk      (radio_clk),
    .reset    (radio_rst),
    .clear    (clear_fifo),
    .i_tdata  ({ reg_cmd_time, reg_cmd_timed, reg_cmd_num_words, (reg_cmd_word == RX_CMD_CONTINUOUS) }),
    .i_tvalid (reg_cmd_valid),
    .i_tready (),
    .o_tdata  ({ cmd_time, cmd_timed, cmd_num_words, cmd_continuous }),
    .o_tvalid (cmd_valid),
    .o_tready (cmd_done),
    .space    (cmd_fifo_space),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // Receiver State Machine
  //---------------------------------------------------------------------------

  // FSM state values
  localparam ST_IDLE               = 0;
  localparam ST_TIME_CHECK         = 1;
  localparam ST_RUNNING            = 2;
  localparam ST_STOP               = 3;
  localparam ST_REPORT_ERR         = 4;
  localparam ST_REPORT_ERR_WAIT    = 5;

  reg [              2:0] state   = ST_IDLE; // Current state
  reg [NUM_WORDS_LEN-1:0] words_left;        // Words left in current command
  reg [             31:0] words_left_pkt;    // Words left in current packet
  reg                     first_word = 1'b1; // Next word is first in packet
  reg [             15:0] seq_num = 0;       // Sequence number (packet count)
  reg [             63:0] error_time;        // Time at which overflow occurred
  reg [ERR_RX_CODE_W-1:0] error_code;        // Error code register

  // Output FIFO signals
  wire [           15:0] out_fifo_space;
  reg  [SAMP_W*NSPC-1:0] out_fifo_tdata;
  reg                    out_fifo_tlast;
  reg                    out_fifo_tvalid = 1'b0;
  reg  [           63:0] out_fifo_timestamp;
  reg                    out_fifo_teob;
  reg                    out_fifo_almost_full;

  reg [63:0] radio_time_low_samp, radio_time_hi_samp;
  reg        time_now, time_past;

  // All ctrlport requests have a time
  assign m_ctrlport_req_has_time = 1'b1;

  // Acknowledge STOP requests and pop the command FIFO in the STOP state
  assign cmd_stop_ack = (state == ST_STOP);
  assign cmd_done     = (state == ST_STOP);

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      state             <= ST_IDLE;
      out_fifo_tvalid   <= 1'b0;
      seq_num           <=  'd0;
      m_ctrlport_req_wr <= 1'b0;
      first_word        <= 1'b1;
    end else begin
      // Default assignments
      out_fifo_tvalid         <= 1'b0;
      out_fifo_tlast          <= 1'b0;
      out_fifo_teob           <= 1'b0;
      m_ctrlport_req_wr       <= 1'b0;

      if (radio_rx_stb) begin
        // Get the time for the low sample and the high sample of the radio
        // word (needed when NISPC > 1). Compensate for the delay required to
        // check the time by adding 3 clock cycles worth of samples.
        radio_time_low_samp <= (radio_time + 3*NSPC);
        radio_time_hi_samp  <= (radio_time + 3*NSPC + (NSPC-1));

        // Register the time comparisons so they don't become the critical path
        time_now  <= (cmd_time >= radio_time_low_samp &&
                      cmd_time <= radio_time_hi_samp);
        time_past <= (cmd_time <  radio_time_low_samp);
      end

      case (state)
        ST_IDLE : begin
          // Wait for a new command to arrive and allow a cycle for the time 
          // comparisons to update.
          if (cmd_valid && radio_rx_stb) begin
            state <= ST_TIME_CHECK;
          end else if (cmd_stop) begin
            state <= ST_STOP;
          end
          first_word <= 1'b1;
        end

        ST_TIME_CHECK : begin
          if (cmd_stop) begin
            // Nothing to do but stop (timed STOP commands are not supported)
            state <= ST_STOP;
          end else if (cmd_timed && time_past && radio_rx_stb) begin
            // Got this command later than its execution time
            //synthesis translate_off
            $display("WARNING: radio_rx_core: Late command error");
            //synthesis translate_on
            error_code <= ERR_RX_LATE_CMD;
            error_time <= radio_time;
            state      <= ST_REPORT_ERR;
          end else if (!cmd_timed || (time_now && radio_rx_stb)) begin
            // Either it's time to run this command or it should run
            // immediately.
            words_left     <= cmd_num_words;
            words_left_pkt <= reg_max_pkt_len;
            state          <= ST_RUNNING;
          end
        end

        ST_RUNNING : begin
          if (radio_rx_stb) begin
            // Output the next word
            out_fifo_tvalid    <= 1'b1;
            out_fifo_tdata     <= radio_rx_data;
            if (first_word) begin
              out_fifo_timestamp <= radio_time;
              first_word         <= 1'b0;
            end

            // Update word counters
            words_left     <= words_left - 1;
            words_left_pkt <= words_left_pkt - 1;

            if ((words_left == 1 && !cmd_continuous) || cmd_stop) begin
              // This command has finished, or we've been asked to stop.
              state          <= ST_STOP;
              out_fifo_tlast <= 1'b1;
              out_fifo_teob  <= 1'b1;
              first_word     <= 1'b1;
            end else if (words_left_pkt == 1) begin
              // We've finished building a packet
              seq_num        <= seq_num + 1;
              words_left_pkt <= reg_max_pkt_len;
              out_fifo_tlast <= 1'b1;
              first_word     <= 1'b1;
            end

            // Check for overflow. Note that we've left enough room in the
            // output FIFO so that we can end the packet cleanly.
            if (out_fifo_almost_full) begin
              // End the command and terminate packet early
              //synthesis translate_off
              $display("WARNING: radio_rx_core: Overrun error");
              //synthesis translate_on
              out_fifo_tlast <= 1'b1;
              out_fifo_teob  <= 1'b1;
              seq_num        <= seq_num + 1;
              error_time     <= radio_time;
              error_code     <= ERR_RX_OVERRUN;
              state          <= ST_REPORT_ERR;
            end

          end
        end

        ST_STOP : begin
          // This single-cycle state allows time for STOP to be acknowledged 
          // and for the command FIFO to be popped.
          state <= ST_IDLE;
        end

        ST_REPORT_ERR : begin
          // Setup write of error code
          m_ctrlport_req_wr                      <= 1'b1;
          m_ctrlport_req_data                    <= 0;
          m_ctrlport_req_data[ERR_RX_CODE_W-1:0] <= error_code;
          m_ctrlport_req_addr                    <= reg_error_addr;
          m_ctrlport_req_time                    <= error_time;
          state                                  <= ST_REPORT_ERR_WAIT;
        end

        ST_REPORT_ERR_WAIT : begin
          // Wait for write of error code and timestamp to complete
          if (m_ctrlport_resp_ack) begin
            state <= ST_STOP;
          end
        end

        default : state <= ST_IDLE;
      endcase
    end
  end


  assign radio_rx_running = (state == ST_RUNNING);  // We're actively acquiring

  // Directly connect the port ID, remote port ID, and remote EPID since they 
  // are only used for error reporting.
  assign m_ctrlport_req_portid     = reg_error_portid;
  assign m_ctrlport_req_rem_epid   = reg_error_rem_epid;
  assign m_ctrlport_req_rem_portid = reg_error_rem_portid;


  //---------------------------------------------------------------------------
  // Output FIFO
  //---------------------------------------------------------------------------
  //
  // Here we buffer output samples and monitor FIFO fullness to be able to
  // detect overflows.
  //
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH (1+64+1+SAMP_W*NSPC),
    .SIZE  (5)      // Ideally, this size will lead to an SRL-based FIFO
  ) output_fifo (
    .clk      (radio_clk),
    .reset    (radio_rst),
    .clear    (1'b0),
    .i_tdata  ({out_fifo_teob, out_fifo_timestamp, out_fifo_tlast, out_fifo_tdata}),
    .i_tvalid (out_fifo_tvalid),
    .i_tready (),
    .o_tdata  ({m_axis_teob, m_axis_ttimestamp, m_axis_tlast, m_axis_tdata}),
    .o_tvalid (m_axis_tvalid),
    .o_tready (m_axis_tready),
    .space    (out_fifo_space),
    .occupied ()
  );

  // Create a register to indicate if the output FIFO is about to overflow
  always @(posedge radio_clk) begin
    if (radio_rst) begin
      out_fifo_almost_full <= 1'b0;
    end else begin
      out_fifo_almost_full <= (out_fifo_space < 5);
    end
  end


endmodule


`default_nettype wire
