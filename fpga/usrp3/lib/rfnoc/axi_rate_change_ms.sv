//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_rate_change_ms
//
// Description:
//  Generates new packets from user logic to NoC shell.
//
//  Monitors timestamps and packet sizes in the NoC shell to user logic side.
//  Generates timestamps for the first sample of each packet and generates
//  packets of maximum size depending on the input packet size for data to the
//  NoC shell.
//  Uses the tlast signal through the user logic to determine the end of the
//  burst. tlast packet boundaries are derived internally from the packet sizes
//  and end of bust. A timeout at the end of the burst guarantees that the
//  internal state machine will terminate each burst even if the tlast does not
//  make it through the user logic due to input data not matching the decimation
//  rate.
//  Generates timestamps on each packet of the burst or none depending on the
//  first received packet of the burst.
//
//   Parameters:
//
//     WIDTH        : Data bus width in bits (must match user logic).
//     SPC          : Samples per clock cycle (must match user logic).
//                    Width of sample stream tags.
//     SPC_MTU_LOG2 : Log2 of the maximum packet size in internal
//                    words(SPC*WIDTH wide). Internal counter widths
//                    are derived from this value.
//     MAX_USER_LOGIC_LATENCY : Maximum latency of the user logic in cycles for the transfer from
//                   m_axis_data_tlast to s_axis_data_tlast assuming that s_axis_data_tready is
//                   always high.
//

module axi_rate_change_ms
  import ctrlport_pkg::*;
  import rfnoc_chdr_utils_pkg::*;
#(
  int WIDTH        = 32,
  int SPC          = 1,
  int SPC_MTU_LOG2 = 10,
  int MAX_USER_LOGIC_LATENCY = 85 + 5*SPC // current value for DDC Multisample user logic
)(
  input wire logic clk,
  input wire logic rst,
  input wire logic clear,

  // Strobed after end of burst (throttles input). Useful for resetting user
  // logic between bursts.
  output     logic clear_user,

  //---------------------------------------------------------------------------
  // CtrlPort slave – configuration registers
  //---------------------------------------------------------------------------
  input wire logic                       s_ctrlport_req_wr,
  input wire logic                       s_ctrlport_req_rd,
  input wire logic [CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input wire logic [CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  output     logic                       s_ctrlport_resp_ack,
  output     logic [ CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output     logic [CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Input AXI-Stream (from NoC shell, m_axis_data)
  // Sideband signals valid during complete packet transfer.
  //---------------------------------------------------------------------------
  input wire logic [           WIDTH-1:0] i_tdata,
  input wire logic                        i_tlast,
  input wire logic                        i_tvalid,
  output     logic                        i_tready,
  input wire logic [             SPC-1:0] i_ttags,
  input wire logic                        i_teob,
  input wire logic [CHDR_TIMESTAMP_W-1:0] i_ttimestamp,
  input wire logic                        i_thas_time,
  input wire logic [   CHDR_LENGTH_W-1:0] i_tlength,

  //---------------------------------------------------------------------------
  // Output AXI-Stream (to NoC shell, s_axis_data)
  // Sideband signals valid on tlast only.
  //---------------------------------------------------------------------------
  output     logic [           WIDTH-1:0] o_tdata,
  output     logic                        o_tlast,
  output     logic                        o_tvalid,
  input wire logic                        o_tready,
  output     logic                        o_teob,
  output     logic [CHDR_TIMESTAMP_W-1:0] o_ttimestamp,
  output     logic                        o_thas_time,

  //---------------------------------------------------------------------------
  // AXI-Stream to user logic (e.g. ddc_multisample / axi_decim)
  // Guaranteed to be an integer multiple of N words per burst.
  //---------------------------------------------------------------------------
  output     logic [WIDTH-1:0] m_axis_data_tdata,
  output     logic             m_axis_data_tlast,
  output     logic             m_axis_data_tvalid,
  input wire logic             m_axis_data_tready,
  output     logic             m_axis_data_teob,
  output     logic [SPC-1:0]   m_axis_data_ttags,

  //---------------------------------------------------------------------------
  // AXI-Stream from user logic
  //---------------------------------------------------------------------------
  input wire logic [WIDTH-1:0] s_axis_data_tdata,
  input wire logic             s_axis_data_tlast,
  input wire logic             s_axis_data_tvalid,
  output     logic             s_axis_data_tready
);

  import axi_rate_change_ms_pkg::*;

  // ========================================================================
  // Local parameters
  // ========================================================================
  localparam int WORD_BYTES    = WIDTH / 8;
  localparam int PAYLOAD_CTR_W = SPC_MTU_LOG2 + $clog2(WORD_BYTES);
  localparam int TIME_INCR_W   = CTRLPORT_DATA_W;

  // =========================================================================
  // CtrlPort register decode
  // =========================================================================
  logic [TIME_INCR_W-1:0] r_time_incr = 0;

  always_ff @(posedge clk) begin
    s_ctrlport_resp_ack    <= '0;
    s_ctrlport_resp_status <= STS_OKAY;
    s_ctrlport_resp_data   <= 'X;
    if (s_ctrlport_req_wr) begin
      case (s_ctrlport_req_addr)
        REG_AXI_RATE_SR_TIME_INCR_ADDR: begin
          r_time_incr <= s_ctrlport_req_data[TIME_INCR_W-1:0];
          s_ctrlport_resp_ack <= '1;
        end
        default: begin
          s_ctrlport_resp_status <= STS_CMDERR; // unsupported address
          s_ctrlport_resp_ack  <= '1;
        end
      endcase
    end
    if (s_ctrlport_req_rd) begin
      case (s_ctrlport_req_addr)
        REG_AXI_RATE_SR_TIME_INCR_ADDR: begin
          s_ctrlport_resp_data <= r_time_incr;
          s_ctrlport_resp_ack  <= '1;
        end
        default: begin
          s_ctrlport_resp_status <= STS_CMDERR; // unsupported address
          s_ctrlport_resp_ack  <= '1;
        end
      endcase
    end
    if (rst) begin
      s_ctrlport_resp_ack <= '0;
      r_time_incr         <= '0;
    end
  end

  // =========================================================================
  // Input state machine
  //
  // Functionality: Stall data at the end of the burst until output SM has
  // finished draining the burst.
  // =========================================================================
  // make it 2x to ensure that the user logic has enough time to drain the burst
  localparam int MAX_TIMEOUT = 2*MAX_USER_LOGIC_LATENCY;
  localparam int TIMEOUT_WIDTH = $clog2(MAX_TIMEOUT);
  logic [TIMEOUT_WIDTH-1:0] latency_counter = '0;

  typedef enum {
    PASSTHROUGH, WAIT_FOR_BURST_END, TERMINATE_BURST, CLEAR_DATA_PATH
  } input_state_t;
  input_state_t input_state;

  always_ff @(posedge clk) begin
    if (rst || clear) begin
      input_state <= PASSTHROUGH;
    end else begin
      case (input_state)
        // Transfers data into the user logic until the end of the burst.
        PASSTHROUGH : begin
          if (i_tvalid && i_tready && i_teob && i_tlast) begin
            input_state <= WAIT_FOR_BURST_END;
          end
          latency_counter <= '0;
        end
        // Wait for the end of the burst from the user logic. If the user logic
        // does not propagate the end of the burst within the timeout period,
        // terminate the burst.
        WAIT_FOR_BURST_END : begin
          if (o_tvalid && o_tready && o_teob && o_tlast) begin
            input_state <= CLEAR_DATA_PATH;
          end
          // count each clock cycles which the user logic can propagate data to
          // the output
          // regardless of the ready the data is available when valid is set
          if (!o_tvalid) begin
            latency_counter <= latency_counter + 1;
          end
          // in case of timeout go to termination state
          if (latency_counter == (MAX_TIMEOUT-1)) begin
            input_state <= TERMINATE_BURST;
          end
        end
        // Send a single transaction to the NoC shell to terminate the burst.
        TERMINATE_BURST : begin
          if (o_tready) begin
            input_state <= CLEAR_DATA_PATH;
          end
        end
        // Clear the user logic to reset the data path for the next burst.
        CLEAR_DATA_PATH : begin
          input_state <= PASSTHROUGH;
        end
        default : begin
          input_state <= PASSTHROUGH;
        end
      endcase
    end
  end

  // =========================================================================
  // Input data path forwarding
  // - stall input until burst has finished
  // =========================================================================
  logic stall_input;
  assign stall_input = (input_state != PASSTHROUGH);

  assign m_axis_data_tdata  = i_tdata;
  assign m_axis_data_tlast  = i_tlast && i_teob; // reuse tlast as the end of burst signal
  assign m_axis_data_teob   = i_teob;
  assign m_axis_data_ttags  = i_ttags;
  assign m_axis_data_tvalid = i_tvalid && !stall_input;
  assign i_tready           = m_axis_data_tready && !stall_input;

  assign clear_user = (input_state == CLEAR_DATA_PATH);

  // =========================================================================
  // Timestamp generation
  // Captures the CHDR timestamp from the first input packet, then
  // increments by time_incr for each subsequent output word/packet.
  // =========================================================================
  // extract timestamp from first packet in burst and notify output state
  // machine
  logic [CHDR_TIMESTAMP_W-1:0] first_pkt_timestamp;
  // valid flag to minimize impact on the output timestamp
  logic                        first_pkt_timestamp_valid;
  logic                        burst_has_time = '0;
  logic                        first_burst_transfer = '1;

  // determine first packet in burst
  always_ff @(posedge clk) begin
    if (rst || clear || clear_user) begin
      first_burst_transfer <= 1'b1;
    end else begin
      if (i_tvalid && i_tready && i_teob && i_tlast) begin
        first_burst_transfer <= 1'b1;
      end else if (i_tvalid && i_tready) begin
        first_burst_transfer <= 1'b0;
      end
    end
  end

  // Working copy of time_incr (latched at start of the burst)
  logic [TIME_INCR_W-1:0] time_incr;

  always_ff @(posedge clk) begin
    if (first_burst_transfer) begin
      time_incr <= r_time_incr;
    end
  end

  // Capture timestamp metadata as soon as valid sideband is available.
  // as well as the information if the burst has some data
  always_ff @(posedge clk) begin
    // single pulse signal
    first_pkt_timestamp_valid <= 1'b0;

    if (first_burst_transfer && i_thas_time && i_tvalid && i_tready) begin
      first_pkt_timestamp       <= i_ttimestamp;
      first_pkt_timestamp_valid <= 1'b1;
      burst_has_time            <= 1'b1;
    end

    // reset
    if (clear_user) begin
      burst_has_time <= 1'b0;
    end
  end

  // update outgoing timestamp
  // always updated at the beginning of the burst through input state machine
  logic [CHDR_TIMESTAMP_W-1:0] timestamp_out = '0;
  logic [CHDR_TIMESTAMP_W-1:0] timestamp_accu = '0;
  always_ff @(posedge clk) begin
    if (first_pkt_timestamp_valid) begin
      timestamp_out <= first_pkt_timestamp;
      timestamp_accu <= first_pkt_timestamp;
    end else if (o_tvalid && o_tready) begin
      timestamp_accu <= timestamp_accu + time_incr;
      if (o_tlast) begin
        timestamp_out <= timestamp_accu + time_incr;
      end
    end
  end

  // =========================================================================
  // Packet boundary detection
  // =========================================================================
  logic [PAYLOAD_CTR_W-1:0] payload_length_out = '0;
  logic [PAYLOAD_CTR_W-1:0] payload_length_out_m1 = '0;
  logic [PAYLOAD_CTR_W-1:0] out_payload_cnt = WORD_BYTES;
  logic end_of_packet = '0;

  always_ff @(posedge clk) begin
    if (clear_user) begin
      payload_length_out <= '0;
    end else if (i_tvalid && i_tready && (payload_length_out < i_tlength)) begin
      payload_length_out <= i_tlength;
    end

    // decrement each cycle to have an updated value
    payload_length_out_m1 <= payload_length_out - WORD_BYTES;

    // actually count the output payload
    if (clear_user) begin
      out_payload_cnt <= WORD_BYTES;
    end else if (o_tvalid && o_tready) begin
      if (end_of_packet) begin
        out_payload_cnt <= WORD_BYTES;
      end else begin
        out_payload_cnt <= out_payload_cnt + WORD_BYTES;
      end
    end

    // prepare the end of the packet
    if (clear_user) begin
      end_of_packet <= 1'b0;
    // capture worst case of just 1 word per packet
    end else if (payload_length_out == WORD_BYTES) begin
      end_of_packet <= 1'b1;
    end else if (o_tvalid && o_tready) begin
      end_of_packet <= (out_payload_cnt == payload_length_out_m1) && !end_of_packet;
    end
  end


  // =========================================================================
  // Output data path forwarding
  // - use t_eob from user logic to generate t_last for NoC shell
  // =========================================================================
  logic terminate_burst;
  assign terminate_burst = (input_state == TERMINATE_BURST);

  assign o_tdata  = s_axis_data_tdata;
  assign o_teob   = s_axis_data_tlast | terminate_burst;
  assign o_tvalid = s_axis_data_tvalid | terminate_burst;
  assign s_axis_data_tready = o_tready;

  // insert generated signals
  assign o_tlast = end_of_packet | o_teob | terminate_burst;
  assign o_ttimestamp = timestamp_out;
  assign o_thas_time = o_tlast && burst_has_time; //only valid on tlast


endmodule
