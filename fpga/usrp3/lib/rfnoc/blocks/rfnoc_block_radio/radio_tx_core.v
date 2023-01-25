//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: radio_tx_core
//
// Description:
//
// This module contains the core Tx radio data-path logic. It receives samples
// over AXI-Stream that it then sends to the radio interface coincident with a
// strobe signal that must be provided by the radio interface.
//
// There are no registers for starting or stopping the transmitter. It is
// operated simply by providing data packets via its AXI-Stream data interface.
// The end-of-burst (EOB) signal is used to indicate when the transmitter is
// allowed to stop transmitting. Packet timestamps can be used to indicate when
// transmission should start.
//
// Care must be taken to provide data to the transmitter at a rate that is
// faster than the radio needs it so that underflows do not occur. Similarly,
// timed packets must be delivered before the timestamp expires. If a packet
// arrives late, then it will be dropped and the error will be reported via the
// CTRL port interface.
//
// Parameters:
//
//   SAMP_W : Width of a radio sample
//   NSPC   : Number of radio samples per radio clock cycle
//


module radio_tx_core #(
  parameter SAMP_W = 32,
  parameter NSPC   = 1
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

  output wire [SAMP_W*NSPC-1:0] radio_tx_data,
  input  wire                   radio_tx_stb,

  // Status indicator (true when transmitting)
  output wire radio_tx_running,


  //---------------------------------------------------------------------------
  // AXI-Stream Data Input
  //---------------------------------------------------------------------------

  input  wire [SAMP_W*NSPC-1:0] s_axis_tdata,
  input  wire                   s_axis_tlast,
  input  wire                   s_axis_tvalid,
  output wire                   s_axis_tready,
  // Sideband info
  input  wire [           63:0] s_axis_ttimestamp,
  input  wire                   s_axis_thas_time,
  input  wire                   s_axis_teob
);

  `include "rfnoc_block_radio_regs.vh"
  `include "../../core/rfnoc_chdr_utils.vh"


  //---------------------------------------------------------------------------
  // Register Read/Write Logic
  //---------------------------------------------------------------------------

  reg [SAMP_W-1:0] reg_idle_value       = 0; // Value to output when transmitter is idle
  reg [       9:0] reg_error_portid     = 0; // Port ID to use for error reporting
  reg [      15:0] reg_error_rem_epid   = 0; // Remote EPID to use for error reporting
  reg [       9:0] reg_error_rem_portid = 0; // Remote port ID to use for error reporting
  reg [      19:0] reg_error_addr       = 0; // Address to use for error reporting

  reg [TX_ERR_POLICY_LEN-1:0] reg_policy = TX_ERR_POLICY_PACKET;

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      s_ctrlport_resp_ack  <= 0;
      reg_idle_value       <= 0;
      reg_error_portid     <= 0;
      reg_error_rem_epid   <= 0;
      reg_error_rem_portid <= 0;
      reg_error_addr       <= 0;
      reg_policy           <= TX_ERR_POLICY_PACKET;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack  <= 0;
      s_ctrlport_resp_data <= 0;

      // Handle register writes
      if (s_ctrlport_req_wr) begin
        case (s_ctrlport_req_addr)
          REG_TX_IDLE_VALUE: begin
            reg_idle_value      <= s_ctrlport_req_data[SAMP_W-1:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_TX_ERROR_POLICY: begin
            // Only allow valid configurations
            case (s_ctrlport_req_data[TX_ERR_POLICY_LEN-1:0])
              TX_ERR_POLICY_PACKET : reg_policy <= TX_ERR_POLICY_PACKET;
              TX_ERR_POLICY_BURST  : reg_policy <= TX_ERR_POLICY_BURST;
              default              : reg_policy <= TX_ERR_POLICY_PACKET;
            endcase
            s_ctrlport_resp_ack <= 1;
          end
          REG_TX_ERR_PORT: begin
            reg_error_portid    <= s_ctrlport_req_data[9:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_TX_ERR_REM_PORT: begin
            reg_error_rem_portid <= s_ctrlport_req_data[9:0];
            s_ctrlport_resp_ack  <= 1;
          end
          REG_TX_ERR_REM_EPID: begin
            reg_error_rem_epid  <= s_ctrlport_req_data[15:0];
            s_ctrlport_resp_ack <= 1;
          end
          REG_TX_ERR_ADDR: begin
            reg_error_addr      <= s_ctrlport_req_data[19:0];
            s_ctrlport_resp_ack <= 1;
          end
        endcase
      end

      // Handle register reads
      if (s_ctrlport_req_rd) begin
        case (s_ctrlport_req_addr)
          REG_TX_IDLE_VALUE: begin
            s_ctrlport_resp_data[SAMP_W-1:0] <= reg_idle_value;
            s_ctrlport_resp_ack              <= 1;
          end
          REG_TX_ERROR_POLICY: begin
            s_ctrlport_resp_data[TX_ERR_POLICY_LEN-1:0] <= reg_policy;
            s_ctrlport_resp_ack                         <= 1;
          end
          REG_TX_ERR_PORT: begin
            s_ctrlport_resp_data[9:0] <= reg_error_portid;
            s_ctrlport_resp_ack       <= 1;
          end
          REG_TX_ERR_REM_PORT: begin
            s_ctrlport_resp_data[9:0] <= reg_error_rem_portid;
            s_ctrlport_resp_ack       <= 1;
          end
          REG_TX_ERR_REM_EPID: begin
            s_ctrlport_resp_data[15:0] <= reg_error_rem_epid;
            s_ctrlport_resp_ack        <= 1;
          end
          REG_TX_ERR_ADDR: begin
            s_ctrlport_resp_data[19:0] <= reg_error_addr;
            s_ctrlport_resp_ack        <= 1;
          end
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Sample Alignment
  //---------------------------------------------------------------------------
  //
  // Shift the outgoing data to align the first sample with sample position
  // corresponding to the requested timestamp.
  //
  //---------------------------------------------------------------------------

  localparam SHIFT_W = $clog2(NSPC);

  reg  [SHIFT_W-1:0]     time_shift   = 0;
  reg                    align_cfg_en = 0;
  wire [SAMP_W*NSPC-1:0] unaligned_data;

  if (NSPC > 1) begin : gen_time_alignment
    align_samples #(
      .SAMP_W  (SAMP_W),
      .SPC     (NSPC  ),
      .USER_W  (1     ),
      .PIPE_IN (1     ),
      .PIPE_OUT(1     )
    ) align_samples_i (
      .clk     (radio_clk     ),
      .i_data  (unaligned_data),
      .i_push  (radio_tx_stb  ),
      .i_user  (1'b0          ),
      .i_dir   (1'b0          ),
      .i_shift (time_shift    ),
      .i_cfg_en(align_cfg_en  ),
      .o_data  (radio_tx_data ),
      .o_user  (              )
    );
  end else begin : gen_no_time_alignment
    assign radio_tx_data = unaligned_data;
  end


  //---------------------------------------------------------------------------
  // Transmitter State Machine
  //---------------------------------------------------------------------------

  // FSM state values
  localparam ST_IDLE        = 0;
  localparam ST_TIME_CHECK  = 1;
  localparam ST_TRANSMIT    = 2;
  localparam ST_WAIT_ALIGN0 = 3;
  localparam ST_WAIT_ALIGN1 = 4;
  localparam ST_POLICY_WAIT = 5;

  reg [2:0] state = ST_IDLE;

  reg sop = 1'b1;  // Start of packet

  reg [ERR_TX_CODE_W-1:0] new_error_code;
  reg [             63:0] new_error_time;
  reg                     new_error_valid = 1'b0;

  reg time_now;    // Indicates when we've reached the requested timestamp
  reg time_now_m1; // Indicates we've reached the requested timestamp minus 1
  reg time_past;   // Indicates when we've passed the requested timestamp

  reg [SHIFT_W-1:0] radio_offset = 0;
  reg               send_early;

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      state           <= ST_IDLE;
      sop             <= 1'b1;
      new_error_valid <= 1'b0;
      time_shift      <= 0;
      align_cfg_en    <= 1'b0;

      // Registers for which we don't care if they have a reset or not because
      // they're set during state machine execution.
      radio_offset    <= 'bX;
      send_early      <= 'bX;
      new_error_code  <= 'bX;
      new_error_time  <= 'bX;
      new_error_valid <= 'bX;
      time_now        <= 'bX;
      time_now_m1     <= 'bX;
      time_past       <= 'bX;
    end else begin
      // Default assignments
      new_error_valid <= 1'b0;
      align_cfg_en    <= 1'b0;

      if (radio_tx_stb) begin
        // Register time comparisons so they don't become the critical path
        time_now_m1 <= (radio_time[63:SHIFT_W]+1 == s_axis_ttimestamp[63:SHIFT_W]);
        time_now    <= time_now_m1;
        time_past   <= (radio_time[63:SHIFT_W]    > s_axis_ttimestamp[63:SHIFT_W]);
      end

      if (NSPC > 1) begin
        if (radio_tx_stb) begin
          radio_offset <= radio_time[0+:SHIFT_W];
        end
      end else begin
        radio_offset <= 0;
      end

      // Track if the next word will be the start of a packet (sop)
      if (s_axis_tvalid && s_axis_tready) begin
        sop <= s_axis_tlast;
      end

      case (state)
        ST_IDLE : begin
          // Wait for a new packet to arrive and a radio strobe to update the
          // time comparisons.
          if (s_axis_tvalid && radio_tx_stb) begin
            align_cfg_en <= 1'b1;
            state        <= ST_TIME_CHECK;
          end

          // Calculate the time shift, in samples, needed to left-shift the
          // first sample to be transmitted into the time slot indicated by the
          // requested timestamp. "send_early" means that the requested
          // timestamp is actually one word earlier than the word with the
          // matching timestamp because of the way the radio_time is aligned.
          if (NSPC > 1) begin
            if (s_axis_thas_time) begin
              if (radio_offset > s_axis_ttimestamp[0+:SHIFT_W]) begin
                time_shift <= NSPC - (radio_offset - s_axis_ttimestamp[0+:SHIFT_W]);
                send_early <= 1'b1;
              end else begin
                time_shift <= s_axis_ttimestamp[0+:SHIFT_W] - radio_offset;
                send_early <= 1'b0;
              end
            end else begin
              time_shift <= 0;
              send_early <= 1'b0;
            end
          end
        end

        ST_TIME_CHECK : begin
          if (!s_axis_thas_time ||
            (radio_tx_stb && time_now_m1 && ( send_early && NSPC  > 1)) ||
            (radio_tx_stb && time_now    && (!send_early || NSPC == 1))
          ) begin
            // We have a new packet without a timestamp, or a new packet
            // whose time has arrived.
            state <= ST_TRANSMIT;
          end else if (time_past) begin
            // We have a new packet with a timestamp, but the time has passed.
            //synthesis translate off
            $display("WARNING: radio_tx_core: Late data error");
            //synthesis translate_on
            new_error_code  <= ERR_TX_LATE_DATA;
            new_error_time  <= radio_time;
            new_error_valid <= 1'b1;
            state           <= ST_POLICY_WAIT;
          end
        end

        ST_TRANSMIT : begin
          if (radio_tx_stb) begin
            if (!s_axis_tvalid) begin
              // The radio strobed for new data but we don't have any to give
              //synthesis translate off
              $display("WARNING: radio_tx_core: Underrun error");
              //synthesis translate_on
              new_error_code  <= ERR_TX_UNDERRUN;
              new_error_time  <= radio_time;
              new_error_valid <= 1'b1;
              state           <= ST_POLICY_WAIT;
            end else if (s_axis_tlast && s_axis_teob) begin
              // We're done with this burst of packets, so acknowledge EOB and
              // go back to idle.
              new_error_code  <= ERR_TX_EOB_ACK;
              new_error_time  <= radio_time;
              new_error_valid <= 1'b1;
              if (NSPC > 1) begin
                state <= ST_WAIT_ALIGN0;
              end else begin
                state <= ST_IDLE;
              end
            end
          end
        end

        ST_WAIT_ALIGN0 : begin
          // Add extra radio word delays to ensure we don't update the time
          // alignment until the last word is strobed out.
          if (radio_tx_stb) begin
            state <= ST_WAIT_ALIGN1;
          end
        end

        ST_WAIT_ALIGN1 : begin
          if (radio_tx_stb) begin
            state <= ST_IDLE;
          end
        end

        ST_POLICY_WAIT : begin
          // If we came here from ST_TIME_CHECK or ST_TRANSMIT and we're in the
          // middle of a packet then we just wait until we reach the end of the
          // packet.
          if (s_axis_tvalid && s_axis_tlast) begin
            // We're either at the end of a packet or between packets
            if (reg_policy == TX_ERR_POLICY_PACKET ||
               (reg_policy == TX_ERR_POLICY_BURST  && s_axis_teob)) begin
              state <= ST_IDLE;
            end

          // If we came from ST_TRANSMIT and we happen to already be between
          // packets (i.e., we underflowed while waiting for the next packet).
          end else if (!s_axis_tvalid && sop) begin
            if (reg_policy == TX_ERR_POLICY_PACKET) state <= ST_IDLE;
          end
        end

        default : state <= ST_IDLE;
      endcase
    end
  end


  // Output the current sample whenever we're transmitting and the sample is
  // valid. Otherwise, output the idle value.
  assign unaligned_data = (s_axis_tvalid && state == ST_TRANSMIT) ?
                          s_axis_tdata :
                          {NSPC{reg_idle_value[SAMP_W-1:0]}};

  // Read packet in the transmit state or dump it in the error state
  assign s_axis_tready = (radio_tx_stb && (state == ST_TRANSMIT)) ||
                         (state == ST_POLICY_WAIT);

  // Indicate whether Tx interface is actively transmitting
  assign radio_tx_running = (state == ST_TRANSMIT);


  //---------------------------------------------------------------------------
  // Error FIFO
  //---------------------------------------------------------------------------
  //
  // This FIFO queues up errors in case we get multiple errors in a row faster
  // than they can be reported. If the FIFO fills then new errors will be
  // ignored.
  //
  //---------------------------------------------------------------------------

  // Error information
  wire [ERR_TX_CODE_W-1:0] next_error_code;
  wire [             63:0] next_error_time;
  wire                     next_error_valid;
  reg                      next_error_ready = 1'b0;

  wire new_error_ready;

  axi_fifo_short #(
    .WIDTH (64 + ERR_TX_CODE_W)
  ) error_fifo (
    .clk      (radio_clk),
    .reset    (radio_rst),
    .clear    (1'b0),
    .i_tdata  ({new_error_time, new_error_code}),
    .i_tvalid (new_error_valid & new_error_ready),   // Mask with ready to prevent FIFO corruption
    .i_tready (new_error_ready),
    .o_tdata  ({next_error_time, next_error_code}),
    .o_tvalid (next_error_valid),
    .o_tready (next_error_ready),
    .space    (),
    .occupied ()
  );

  //synthesis translate_off
  // Output a message if the error FIFO overflows
  always @(posedge radio_clk) begin
    if (new_error_valid && !new_error_ready) begin
      $display("WARNING: Tx error report dropped!");
    end
  end
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // Error Reporting State Machine
  //---------------------------------------------------------------------------
  //
  // This state machine reports errors that have been queued up in the error
  // FIFO.
  //
  //---------------------------------------------------------------------------

  localparam ST_ERR_IDLE = 0;
  localparam ST_ERR_CODE = 1;

  reg [0:0] err_state = ST_ERR_IDLE;

  // All ctrlport requests have a time
  assign m_ctrlport_req_has_time = 1'b1;

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      m_ctrlport_req_wr <= 1'b0;
      err_state         <= ST_ERR_IDLE;
      next_error_ready  <= 1'b0;
    end else begin
      m_ctrlport_req_wr <= 1'b0;
      next_error_ready  <= 1'b0;

      case (err_state)
        ST_ERR_IDLE : begin
          if (next_error_valid) begin
            // Setup write of error code
            m_ctrlport_req_wr   <= 1'b1;
            m_ctrlport_req_addr <= reg_error_addr;
            m_ctrlport_req_data <= {{(32-ERR_TX_CODE_W){1'b0}}, next_error_code};
            m_ctrlport_req_time <= next_error_time;
            next_error_ready    <= 1'b1;
            err_state           <= ST_ERR_CODE;
          end
        end

        ST_ERR_CODE : begin
          // Wait for write of error code and timestamp
          if (m_ctrlport_resp_ack) begin
            err_state <= ST_ERR_IDLE;
          end
        end

        default : err_state <= ST_ERR_IDLE;
      endcase
    end
  end


  // Directly connect the port ID, remote port ID, remote EPID since they are
  // only used for error reporting.
  assign m_ctrlport_req_portid     = reg_error_portid;
  assign m_ctrlport_req_rem_epid   = reg_error_rem_epid;
  assign m_ctrlport_req_rem_portid = reg_error_rem_portid;


endmodule
