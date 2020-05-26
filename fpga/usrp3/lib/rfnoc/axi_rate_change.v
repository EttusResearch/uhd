//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// - Implements rate change of N:M (a.k.a. M/N), handles headers automatically
// - Note: N should always be written before M in software to prevent false rate changes
//         while the block is active
// - User code is responsible for generating correct number of outputs per input
//   > Example: When set 1/N, after N input samples block should output 1 sample. If
//              user code's pipelining requires additional samples to "push" the 1
//              sample out, it is the user's responsibility to make the mechanism
//              (such as injecting extra samples) to do so.
// - Will always send an integer multiple of N samples to user logic. This ensures
//   the user will not need to manually clear a "partial output sample" stuck in their
//   pipeline due to an uneven (in respect to decimation rate) number of input samples.
// - Can optionally strobe clear_user after receiving packet with EOB
//   > enable_clear_user must be enabled via CONFIG settings register
//   > Warning: Input will be throttled until last packet has completely passed through
//     user code to prevent clearing valid data. In certain conditions, this throttling
//     can have a significant impact on throughput.
// - Output packet size will be identical to input packet size. The only exception is
//   the final output packet, which may be shorter due to a partial input packet.
// Limitations:
// - Rate changes are ignored while active. Block must be cleared or packet with EOB
//   (and enable_clear_user is set) will cause new rates to be loaded.
// - Can potentially use large amounts of block RAM when using large decimation rates
//   (greater than 2K). This occurs due to the feature that the block always sends a multiple
//   of N samples to the user. Implementing this feature requires N samples to be buffered.
// - User code with long pipelines may need to increase HEADER_FIFOSIZE. The debug signal
//   warning_header_fifo_full is useful in determining this case.
//
// Settings Registers:
//   sr_n:       Number of input samples per M output samples (Always write N before M)
//   sr_m:       Number of output samples per N input samples
//   sr_config:  0: Enable clear_user signal.

module axi_rate_change #(
  parameter WIDTH                   = 32, // Input bit width, must be a power of 2 and greater than or equal to 8.
  parameter MAX_N                   = 2**16,
  parameter MAX_M                   = 2**16,
  parameter MAXIMIZE_OUTPUT_PKT_LEN = 1,
  // Settings registers
  parameter SR_N_ADDR               = 0,
  parameter SR_M_ADDR               = 1,
  parameter SR_CONFIG_ADDR          = 2,
  parameter SR_TIME_INCR_ADDR       = 3,
  parameter DEFAULT_N               = 1,
  parameter DEFAULT_M               = 1
)(
  input clk, input reset, input clear,
  output clear_user,  // Strobed after end of burst. Throttles input. Useful for resetting user code between bursts.
  input [15:0] src_sid, input [15:0] dst_sid,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready, input [127:0] i_tuser,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready, output [127:0] o_tuser,
  output [WIDTH-1:0] m_axis_data_tdata, output m_axis_data_tlast, output m_axis_data_tvalid, input m_axis_data_tready,
  input [WIDTH-1:0] s_axis_data_tdata, input s_axis_data_tlast, input s_axis_data_tvalid, output s_axis_data_tready,
  // Debugging signals:
  // - Warnings indicate there may be an issue with user code.
  // - Errors mean the user code has violated a rule.
  // - Signals latch once set and block must be reset to clear.
  output reg warning_long_throttle,     // In the throttle state for a "long" time.
  output reg error_extra_outputs,       // User code generated extra outputs, i.e. received more than the expected M outputs.
  output reg error_drop_pkt_lockup      // Drop partial packet module is not accepting data even though user code is ready.
);

  reg [$clog2(MAX_N+1)-1:0] n = DEFAULT_N;
  reg [$clog2(MAX_M+1)-1:0] m = DEFAULT_M;

  wire [WIDTH-1:0] i_reg_tdata;
  wire i_reg_tvalid, i_reg_tready, i_reg_tlast;
  wire i_reg_tvalid_int, i_reg_tready_int, i_reg_tlast_int;

  reg throttle = 1'b1, first_header, partial_first_word;
  reg [15:0] word_cnt_div_n;
  reg [$clog2(MAX_N+1)-1:0] word_cnt_div_n_frac = 1;
  reg [$clog2(MAX_N+1)-1:0] in_pkt_cnt = 1;

  reg send_done;
  reg rate_changed;

  /********************************************************
  ** Settings Registers
  ********************************************************/
  wire [$clog2(MAX_N+1)-1:0] sr_n;
  wire n_changed;
  setting_reg #(.my_addr(SR_N_ADDR), .width($clog2(MAX_N+1)), .at_reset(DEFAULT_N)) set_n (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(sr_n), .changed(n_changed));

  wire [$clog2(MAX_M+1)-1:0] sr_m;
  wire m_changed;
  setting_reg #(.my_addr(SR_M_ADDR), .width($clog2(MAX_M+1)), .at_reset(DEFAULT_M)) set_m (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(sr_m), .changed(m_changed));

  wire sr_config;
  wire enable_clear_user; // Enable strobing clear_user between bursts.
  setting_reg #(.my_addr(SR_CONFIG_ADDR), .width(1), .at_reset(1'b1)) set_config (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(sr_config), .changed());
  assign enable_clear_user = sr_config;

  // Time increment in ticks per M samples
  wire [$clog2(MAX_N+1)-1:0] sr_time_incr;
  reg  [$clog2(MAX_N+1)-1:0] time_incr;
  setting_reg #(
    .my_addr(SR_TIME_INCR_ADDR), .width($clog2(MAX_N+1)), .at_reset(0)
  ) set_time_incr (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(sr_time_incr), .changed());

  /********************************************************
  ** Header, word count FIFOs
  ** - Header provides VITA Time and payload length for
  **   output packets
  ** - Word count provides a normalized count for the
  **   output state machine to know when it has consumed
  **   the final input sample in a burst.
  ********************************************************/
  // Decode input header
  wire [127:0] i_reg_tuser;
  wire has_time_in, eob_in, eob_in_header;
  wire [15:0] payload_length_in;
  reg [15:0] payload_length_out;
  wire [63:0] vita_time_in;
  cvita_hdr_decoder cvita_hdr_decoder_in_header (
    .header(i_reg_tuser), .pkt_type(), .eob(eob_in_header),
    .has_time(has_time_in), .seqnum(), .length(), .payload_length(payload_length_in),
    .src_sid(), .dst_sid(), .vita_time(vita_time_in));

  assign eob_in = eob_in_header | rate_changed;

  reg [15:0] word_cnt_div_n_tdata;
  wire [15:0] word_cnt_div_n_fifo_tdata;
  reg word_cnt_div_n_tvalid;
  wire word_cnt_div_n_tready, word_cnt_div_n_fifo_tvalid, word_cnt_div_n_fifo_tready;
  axi_fifo #(.WIDTH(16), .SIZE(0)) axi_fifo_word_cnt (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(word_cnt_div_n_tdata), .i_tvalid(word_cnt_div_n_tvalid), .i_tready(word_cnt_div_n_tready),
    .o_tdata(word_cnt_div_n_fifo_tdata), .o_tvalid(word_cnt_div_n_fifo_tvalid), .o_tready(word_cnt_div_n_fifo_tready),
    .space(), .occupied());

  /********************************************************
  ** Register input stream
  ** - Upsteam will be throttled when clearing user logic
  ********************************************************/
  // Input register with header
  axi_fifo_flop #(.WIDTH(WIDTH+1+128)) axi_fifo_flop_input (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({i_tlast,i_tdata,i_tuser}), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata({i_reg_tlast,i_reg_tdata,i_reg_tuser}), .o_tvalid(i_reg_tvalid_int), .o_tready(i_reg_tready),
    .space(), .occupied());

  assign i_reg_tready     = i_reg_tready_int & word_cnt_div_n_tready & ~throttle;
  assign i_reg_tvalid     = i_reg_tvalid_int & word_cnt_div_n_tready & ~throttle;
  // Assert AXI Drop Partial Packet's i_tlast every N samples, which is used to detect and drop
  // partial output samples.
  assign i_reg_tlast_int  = (word_cnt_div_n_frac == n) | (eob_in & i_reg_tlast);

  /********************************************************
  ** Input state machine
  ********************************************************/
  reg [1:0] input_state;
  localparam RECV_INIT                = 0;
  localparam RECV                     = 1;
  localparam RECV_WAIT_FOR_SEND_DONE  = 2;

  always @(posedge clk) begin
    if (reset | clear) begin
      n                     <= DEFAULT_N;
      m                     <= DEFAULT_M;
      rate_changed          <= 1'b0;
      first_header          <= 1'b1;
      partial_first_word    <= 1'b1;
      payload_length_out    <= 'd0;
      word_cnt_div_n        <= 0;
      word_cnt_div_n_frac   <= 1;
      throttle              <= 1'b1;
      word_cnt_div_n_tvalid <= 1'b0;
      word_cnt_div_n_tdata  <= 'd0;
      input_state           <= RECV_INIT;
    end else begin
      if (word_cnt_div_n_tvalid & word_cnt_div_n_tready) begin
        word_cnt_div_n_tvalid <= 1'b0;
      end
      // Input state machine
      case (input_state)
        RECV_INIT : begin
          n                      <= sr_n;
          m                      <= sr_m;
          // Default time increment to sr_n value to preserve default behavior
          time_incr              <= sr_time_incr == 0 ? sr_n : sr_time_incr;
          rate_changed           <= 1'b0;
          first_header           <= 1'b1;
          partial_first_word     <= 1'b1;
          payload_length_out     <= 'd0;
          word_cnt_div_n         <= 0;
          word_cnt_div_n_frac    <= 1;
          if (i_reg_tvalid_int & word_cnt_div_n_tready) begin
            throttle             <= 1'b0;
            input_state          <= RECV;
          end
        end
        // Logic used by the RECV state to track several variables:
        // word_cnt_div_n:      Number of words received divided by n.
        //                      Needed for tracking final sample in a burst.
        // word_cnt_div_n_frac: Used to increment word_cnt_div_n. Can be
        //                      thought of as the fractional part of
        //                      word_cnt_div_n.
        // in_pkt_cnt:          Similar to in_word_cnt, but for packets. Used
        //                      to determine when a group of N packets has been
        //                      received to store the next header.
        // first_header:        We only use the header from the first packet in
        //                      a group of N packets (this greatly reduces
        //                      the header FIFO size).
        RECV : begin
          // If rate changed, force a EOB.
          if (m_changed) begin
            rate_changed           <= 1'b1;
          end
          if (i_reg_tvalid & i_reg_tready) begin
            // Track the number of words sent to the user divided by N.
            // At the end of a burst, this value is forwarded to the output
            // state machine and used to determine when the final sample has
            // arrived from the user code.
            if (word_cnt_div_n_frac == n) begin
              word_cnt_div_n       <= word_cnt_div_n + 1;
              word_cnt_div_n_frac  <= 1;
            end else begin
              word_cnt_div_n_frac  <= word_cnt_div_n_frac + 1;
            end
            // Use payload length from first packet
            first_header           <= 1'b0;
            if (first_header) begin
              payload_length_out   <= payload_length_in;
            end else if (MAXIMIZE_OUTPUT_PKT_LEN) begin
              if (payload_length_out < payload_length_in) begin
                payload_length_out <= payload_length_in;
              end
            end
            // Track when at least N input samples have been received in this burst
            if (partial_first_word & (word_cnt_div_n_frac == n)) begin
              partial_first_word   <= 1'b0;
            end
            // Burst ended before we received enough samples to form
            // at least one full output sample.
            // Note: axi_drop_partial_packet automatically handles
            //       dropping the partial sample.
            if (i_reg_tlast & eob_in & partial_first_word) begin
              input_state          <= RECV_INIT;
            end else begin
              if (i_reg_tlast) begin
                // At the end of a burst, forward the number of words divided by N to
                // the output state machine via a FIFO. This allows the output state
                // machine to know when it has received the final output word.
                // We use a FIFO in case the bursts are very small and we
                // need to store several of these values.
                if (eob_in) begin
                  word_cnt_div_n_tdata  <= word_cnt_div_n + (word_cnt_div_n_frac == n);
                  word_cnt_div_n_tvalid <= 1'b1;
                  throttle              <= 1'b1;
                  if (enable_clear_user) begin
                    input_state         <= RECV_WAIT_FOR_SEND_DONE;
                  end else begin
                    input_state         <= RECV_INIT;
                  end
                end
              end
            end
          end
        end
        // Wait until last sample has been output and user logic is cleared
        // WARNING: This can be a huge bubble state! However, since it only happens with
        //          EOBs, it should be infrequent.
        RECV_WAIT_FOR_SEND_DONE : begin
          if (send_done) begin
            input_state <= RECV_INIT;
          end
        end
        default : begin
          input_state   <= RECV_INIT;
        end
      endcase
    end
  end

  assign clear_user = send_done & enable_clear_user;

  /********************************************************
  ** AXI Drop Partial Packet (to user)
  ** - Enforces sending integer multiple of N samples
  **   to user
  ********************************************************/
  axi_drop_partial_packet #(
    .WIDTH(WIDTH+1),
    .HOLD_LAST_WORD(1),
    .MAX_PKT_SIZE(MAX_N),
    .SR_PKT_SIZE_ADDR(SR_N_ADDR),
    .DEFAULT_PKT_SIZE(DEFAULT_N))
  axi_drop_partial_packet (
    .clk(clk), .reset(reset), .clear(clear | send_done),
    .flush(word_cnt_div_n_tvalid & word_cnt_div_n_tready),  // Flush on EOB
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .i_tdata({i_reg_tlast,i_reg_tdata}), .i_tvalid(i_reg_tvalid), .i_tlast(i_reg_tlast_int), .i_tready(i_reg_tready_int),
    .o_tdata({m_axis_data_tlast,m_axis_data_tdata}), .o_tvalid(m_axis_data_tvalid), .o_tlast(/* Unused */), .o_tready(m_axis_data_tready));

  /********************************************************
  ** Output state machine
  ********************************************************/
  reg [1:0] output_state;
  localparam SEND_INIT = 0;
  localparam SEND      = 1;

  wire [WIDTH-1:0] o_reg_tdata;
  wire [127:0] o_reg_tuser;
  wire o_reg_tvalid, o_reg_tready, o_reg_tlast, o_reg_tlast_int;

  reg [15:0] out_payload_cnt = (WIDTH/8);
  reg [15:0] word_cnt_div_m;
  reg [$clog2(MAX_M+1)-1:0] word_cnt_div_m_frac = 1;
  reg [$clog2(MAX_M+1)-1:0] out_pkt_cnt = 1;

  // End of burst tracking. Compare the number of words sent to the user divided by N
  // to the number of words received from the user divided by M. When they equal each other
  // then we have received the last word from the user in this burst.
  // Note: Using word_cnt_div_n_fifo_tdata to make sure the last word is identified before
  //       it has been consumed.
  wire last_word_in_burst = word_cnt_div_n_fifo_tvalid &
                            (word_cnt_div_m == word_cnt_div_n_fifo_tdata) &
                            (word_cnt_div_m_frac == m);

  always @(posedge clk) begin
    if (reset | clear) begin
      word_cnt_div_m      <= 1;
      word_cnt_div_m_frac <= 1;
      out_payload_cnt     <= (WIDTH/8);
      send_done           <= 1'b0;
      output_state        <= SEND_INIT;
    end else begin
      // Track
      case (output_state)
        SEND_INIT : begin
          word_cnt_div_m      <= 1;
          word_cnt_div_m_frac <= 1;
          out_payload_cnt     <= (WIDTH/8);
          send_done           <= 1'b0;
          output_state        <= SEND;
        end
        SEND : begin
          if (o_reg_tvalid & o_reg_tready) begin
            if (o_reg_tlast) begin
              // Track number of samples from user to set tlast
              out_payload_cnt <= (WIDTH/8);
            end else begin
              out_payload_cnt <= out_payload_cnt + (WIDTH/8);
            end
            // Track number of words consumed divided by M. This is used
            // in conjunction with word_cnt_div_n to determine when we have received
            // the last word in a burst from the user.
            if (word_cnt_div_m_frac == m) begin
              word_cnt_div_m      <= word_cnt_div_m + 1;
              word_cnt_div_m_frac <= 1;
            end else begin
              word_cnt_div_m_frac <= word_cnt_div_m_frac + 1;
            end
            if (last_word_in_burst) begin
              send_done    <= 1'b1;
              output_state <= SEND_INIT;
            end
          end
        end
        default : begin
          output_state <= SEND_INIT;
        end
      endcase
    end
  end

  // Only pop this FIFO at EOB.
  assign word_cnt_div_n_fifo_tready = o_reg_tvalid & o_reg_tready & last_word_in_burst;

  /********************************************************
  ** Adjust VITA time
  ********************************************************/
  localparam VT_INIT       = 0;
  localparam VT_INCREMENT  = 1;
  reg vt_state;

  reg has_time_out, has_time_clear;
  reg [63:0] vita_time_out, vita_time_accum;

  always @(posedge clk) begin
    if (reset | clear) begin
      vt_state               <= VT_INIT;
    end else begin
      case (vt_state)
        VT_INIT : begin
          has_time_clear       <= 1'b0;
          if (i_reg_tvalid & i_reg_tready & first_header) begin
            vita_time_out      <= vita_time_in;
            vita_time_accum    <= vita_time_in;
            has_time_out       <= has_time_in;
            vt_state           <= VT_INCREMENT;
          end
        end
        VT_INCREMENT : begin
          // Stop sending vita time if user does not send vita time
          if (i_reg_tvalid & ~has_time_in) begin
            has_time_clear     <= 1'b1;
          end
          if (o_reg_tvalid & o_reg_tready) begin
            if (o_reg_tlast) begin
              if (has_time_clear) begin
                has_time_out   <= 1'b0;
              end
              vita_time_out    <= vita_time_accum + time_incr;
            end
            vita_time_accum    <= vita_time_accum + time_incr;
            if (last_word_in_burst) begin
              vt_state         <= VT_INIT;
            end
          end
        end
        default : begin
          vt_state             <= VT_INIT;
        end
      endcase
    end
  end

  // Create output header
  cvita_hdr_encoder cvita_hdr_encoder (
    .pkt_type(2'd0), .eob(last_word_in_burst), .has_time(has_time_out),
    .seqnum(12'd0), .payload_length(16'd0), // Not needed, handled by AXI Wrapper
    .src_sid(src_sid), .dst_sid(dst_sid),
    .vita_time(vita_time_out),
    .header(o_reg_tuser));

  /********************************************************
  ** Register input stream from user and output stream
  ********************************************************/
  assign o_reg_tlast      = o_reg_tlast_int |
                            // End of packet
                            (out_payload_cnt == payload_length_out) |
                            // EOB, could be a partial packet
                            last_word_in_burst;

  axi_fifo_flop #(.WIDTH(WIDTH+1)) axi_fifo_flop_from_user_0 (
    .clk(clk), .reset(reset), .clear(clear),
    // FIXME: If user asserts tlast at the wrong time, it likely causes a deadlock. For now ignore tlast.
    //.i_tdata({s_axis_data_tlast,s_axis_data_tdata}), .i_tvalid(s_axis_data_tvalid), .i_tready(s_axis_data_tready),
    .i_tdata({1'b0,s_axis_data_tdata}), .i_tvalid(s_axis_data_tvalid), .i_tready(s_axis_data_tready),
    .o_tdata({o_reg_tlast_int,o_reg_tdata}), .o_tvalid(o_reg_tvalid), .o_tready(o_reg_tready),
    .space(), .occupied());

  // Output register with header
  axi_fifo_flop #(.WIDTH(WIDTH+1+128)) axi_fifo_flop_output (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({o_reg_tlast,o_reg_tdata,o_reg_tuser}), .i_tvalid(o_reg_tvalid), .i_tready(o_reg_tready),
    .o_tdata({o_tlast,o_tdata,o_tuser}), .o_tvalid(o_tvalid), .o_tready(o_tready),
    .space(), .occupied());

  /********************************************************
  ** Error / warning signals
  ********************************************************/
  reg [23:0] counter_header_fifo_full, counter_throttle, counter_drop_pkt_lockup;
  reg [2:0] counter_header_fifo_empty;
  always @(posedge clk) begin
    if (reset) begin
      warning_long_throttle       <= 1'b0;
      error_extra_outputs         <= 1'b0;
      error_drop_pkt_lockup       <= 1'b0;
      counter_throttle            <= 0;
      counter_header_fifo_full    <= 0;
      counter_drop_pkt_lockup     <= 0;
      counter_header_fifo_empty   <= 0;
    end else begin
      // In throttle state for a "long" time
      if (throttle) begin
        counter_throttle          <= counter_throttle + 1;
        if (counter_throttle == 2**24-1) begin
          warning_long_throttle   <= 1'b1;
        end
      end else begin
        counter_throttle          <= 0;
      end
      // More than M outputs per N inputs
      if (word_cnt_div_n_fifo_tvalid & (word_cnt_div_m > word_cnt_div_n_fifo_tdata)) begin
        error_extra_outputs         <= 1'b1;
      end
      // Bad internal state. AXI drop partial packet is in a lockup condition.
      if (~i_reg_tready_int & m_axis_data_tready) begin
        counter_drop_pkt_lockup     <= counter_drop_pkt_lockup + 1;
        if (counter_drop_pkt_lockup == 2**24-1) begin
          error_drop_pkt_lockup     <= 1'b1;
        end
      end else begin
        counter_drop_pkt_lockup     <= 0;
      end
    end
  end

endmodule
