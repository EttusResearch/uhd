//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rfnoc_keep_one_in_n
//
// Keep one sample and drop N-1 samples. Also, handles timestamp and setting EOB / EOV.
//
// n:    Drop n-1 samples or packets
// mode: 0 = operate on samples, 1 = operate on packets that are delineated by tlast
//
`default_nettype none

module rfnoc_keep_one_in_n #(
  parameter WIDTH      = 32,
  parameter WIDTH_N    = 16
)(
  input  wire                       clk,
  input  wire                       reset,
  input  wire                       mode,
  input  wire [WIDTH_N-1:0]         n,
  input  wire [WIDTH-1:0]           s_axis_tdata,
  input  wire                       s_axis_tlast,
  input  wire                       s_axis_tvalid,
  output wire                       s_axis_tready,
  input  wire [63:0]                s_axis_ttimestamp,
  input  wire                       s_axis_thas_time,
  input  wire [15:0]                s_axis_tlength,
  input  wire                       s_axis_teov,
  input  wire                       s_axis_teob,
  output wire [WIDTH-1:0]           m_axis_tdata,
  output wire                       m_axis_tlast,
  output wire                       m_axis_tvalid,
  input  wire                       m_axis_tready,
  output wire [63:0]                m_axis_ttimestamp,
  output wire                       m_axis_thas_time,
  output wire                       m_axis_teov,
  output wire                       m_axis_teob
);

  reg first_samp = 1'b1;
  always @(posedge clk) begin
    if (reset) begin
      first_samp <= 1'b1;
    end else begin
      if (s_axis_tvalid & s_axis_tready) begin
        first_samp <= s_axis_tlast;
      end
    end
  end

  // Packet mode state machine
  // - Keeps first packet which is delinated by tlast
  //   and drops the following n-1 packets.
  // - EOB signifies the end of a stream and causes the
  //   state machine to stay in or immediately return to the
  //   "keep" state.
  reg state_pkt;
  localparam S_PKT_KEEP = 1'd0;
  localparam S_PKT_DROP = 1'd1;

  reg [WIDTH_N-1:0] cnt_pkt_n = 2;

  always @(posedge clk) begin
    if (reset) begin
      cnt_pkt_n <= 2;
      state_pkt <= S_PKT_KEEP;
    end else begin
      case (state_pkt)
        S_PKT_KEEP : begin
          cnt_pkt_n <= 2;
          if (s_axis_tvalid & s_axis_tready & s_axis_tlast) begin
            // If EOB or n == 1, stay in this state
            if (~s_axis_teob & n != 1) begin
              state_pkt <= S_PKT_DROP;
            end
          end
        end
        S_PKT_DROP : begin
          if (s_axis_tvalid & s_axis_tready & s_axis_tlast) begin
            if (s_axis_teob) begin
              state_pkt <= S_PKT_KEEP;
            end else begin
              cnt_pkt_n <= cnt_pkt_n + 1;
              if (cnt_pkt_n >= n) begin
                cnt_pkt_n <= 2;
                state_pkt <= S_PKT_KEEP;
              end
            end
          end
        end
        default : state_pkt <= S_PKT_KEEP;
      endcase
    end
  end

  // Sample mode state machine
  // - Keeps first sample and drops n-1 samples.
  // - EOB also causes this state machine stay in or return
  //   to the "keep" state.
  reg [WIDTH_N-1:0] cnt_samp_n;

  reg state_samp;
  localparam S_SAMP_KEEP = 1'd0;
  localparam S_SAMP_DROP = 1'd1;

  always @(posedge clk) begin
    if (reset) begin
      cnt_samp_n <= 2;
      state_samp <= S_SAMP_KEEP;
    end else begin
      case (state_samp)
        S_SAMP_KEEP : begin
          cnt_samp_n <= 2;
          if (s_axis_tvalid & s_axis_tready) begin
            // If EOB or n == 1, stay in this state
            if (~(s_axis_tlast & s_axis_teob) & n != 1) begin
              state_samp <= S_SAMP_DROP;
            end
          end
        end
        S_SAMP_DROP : begin
          if (s_axis_tvalid & s_axis_tready) begin
            if (s_axis_tlast & s_axis_teob) begin
              state_samp <= S_SAMP_KEEP;
            end else begin
              cnt_samp_n <= cnt_samp_n + 1;
              if (cnt_samp_n >= n) begin
                cnt_samp_n <= 2;
                state_samp <= S_SAMP_KEEP;
              end
            end
          end
        end
        default : state_samp <= S_SAMP_KEEP;
      endcase
    end
  end

  wire keep_sample = mode ? (state_pkt == S_PKT_KEEP) : (state_samp == S_SAMP_KEEP);

  // Output state machine
  reg [1:0] state_o;
  localparam S_O_FIRST_SAMP    = 2'd0;
  localparam S_O_OUTPUT        = 2'd1;
  localparam S_O_LAST_SAMP     = 2'd2;
  localparam S_O_LAST_SAMP_EOB = 2'd3;

  reg [WIDTH-1:0]      sample_reg;
  reg [63:0]           timestamp_reg;
  reg                  has_time_reg;
  reg                  eov_reg;
  reg [15-WIDTH/8:0]   length_reg;
  reg [15-WIDTH/8:0]   length_cnt;

  always @(posedge clk) begin
    if (reset) begin
      length_cnt    <= 2;
      sample_reg    <=  'd0;
      timestamp_reg <=  'd0;
      has_time_reg  <= 1'b0;
      length_reg    <=  'd0;
      eov_reg       <= 1'b0;
      state_o       <= S_O_FIRST_SAMP;
    end else begin
      case (state_o)
        // Preload the output register sample_reg. This is necessary
        // so the state machine can have a sample to output if
        // an EOB arrives while dropping samples / packets. If the
        // state machine did not have that sample to output when
        // an EOB arrives, then the EOB would be dropped because
        // you cannot send packets without a payload with RFNoC.
        S_O_FIRST_SAMP : begin
          length_cnt <= 2;
          if (keep_sample & s_axis_tvalid & s_axis_tready) begin
            sample_reg    <= s_axis_tdata;
            timestamp_reg <= s_axis_ttimestamp;
            // If this sample isn't the first sample in the input packet,
            // then the vita time does not correspond to this sample
            // and should be ignored. This situation can happen if the
            // packet length is not consistent.
            has_time_reg  <= s_axis_thas_time & first_samp;
            length_reg    <= s_axis_tlength[15:$clog2(WIDTH/8)];
            eov_reg       <= s_axis_teov;
            // First sample is also EOB, so it will be immediately released
            if (s_axis_tlast & s_axis_teob) begin
              state_o <= S_O_LAST_SAMP_EOB;
            // Packet size is 1 sample
            end else if (s_axis_tlength[15:$clog2(WIDTH/8)] == 1) begin
              state_o <= S_O_LAST_SAMP;
            end else begin
              state_o <= S_O_OUTPUT;
            end
          end
        end
        // Output samples until either we need to either
        // set tlast or encounter an EOB
        S_O_OUTPUT : begin
          if (s_axis_tvalid & s_axis_tready) begin
            // Make EOV bit sticky
            eov_reg <= eov_reg | s_axis_teov;
            if (keep_sample) begin
              sample_reg <= s_axis_tdata;
              length_cnt <= length_cnt + 1;
            end
            if (s_axis_tlast & s_axis_teob) begin
              state_o <= S_O_LAST_SAMP_EOB;
            end else if (keep_sample) begin
              // Use length from input packet to set tlast
              if (length_cnt >= length_reg) begin
                state_o  <= S_O_LAST_SAMP;
              end
            end
          end
        end
        S_O_LAST_SAMP : begin
          length_cnt <= 2;
          if (s_axis_tvalid & s_axis_tready) begin
            if (keep_sample) begin
              sample_reg    <= s_axis_tdata;
              timestamp_reg <= s_axis_ttimestamp;
              has_time_reg  <= s_axis_thas_time & first_samp;
              length_reg    <= s_axis_tlength[15:$clog2(WIDTH/8)];
              eov_reg       <= s_axis_teov;
            end else begin
              eov_reg       <= eov_reg | s_axis_teov;
            end
            if (s_axis_tlast & s_axis_teob) begin
              state_o <= S_O_LAST_SAMP_EOB;
            end else if (keep_sample) begin
              if (s_axis_tlength[15:$clog2(WIDTH/8)] > 1) begin
                state_o <= S_O_OUTPUT;
              end
            end
          end
        end
        S_O_LAST_SAMP_EOB : begin
          if (s_axis_tready) begin
            state_o <= S_O_FIRST_SAMP;
          end
        end
        default : state_o <= S_O_FIRST_SAMP;
      endcase
    end
  end

  assign m_axis_tdata      = sample_reg;
  assign m_axis_tlast      = (state_o == S_O_LAST_SAMP) || (state_o == S_O_LAST_SAMP_EOB);
  assign m_axis_ttimestamp = timestamp_reg;
  assign m_axis_thas_time  = has_time_reg;
  assign m_axis_teov       = eov_reg;
  assign m_axis_teob       = (state_o == S_O_LAST_SAMP_EOB);

  assign m_axis_tvalid = (state_o == S_O_FIRST_SAMP)      ? 1'b0 :
                         (state_o == S_O_OUTPUT)          ? keep_sample & s_axis_tvalid :
                         (state_o == S_O_LAST_SAMP)       ? keep_sample & s_axis_tvalid :
                         (state_o == S_O_LAST_SAMP_EOB)   ? 1'b1 :
                         1'b0;

  assign s_axis_tready = (state_o == S_O_FIRST_SAMP)      ? 1'b1 :
                         (state_o == S_O_OUTPUT)          ? m_axis_tready :
                         (state_o == S_O_LAST_SAMP)       ? m_axis_tready :
                         (state_o == S_O_LAST_SAMP_EOB)   ? m_axis_tready :
                         1'b0;

endmodule

`default_nettype wire
