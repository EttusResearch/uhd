//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_pkt_throttle
//
// Description:
//
//   This module takes in AXI-Stream and outputs the same stream, inserting
//   gaps between packets in order to maintain a specific data rate. The amount
//   of time between packets is controlled in such a way that the length of a
//   packet divided by the time between the start of that packet and the next
//   does not exceed some rate R.
//
//   This module does NOT insert stalls within a packet, only between packets,
//   so the peak data rate is not restricted and packet contiguity is not
//   affected. Also, the average data rate could be slightly higher than the
//   configured rate due to rounding error (within a single clock cycle per
//   packet).
//
//   The "throttle" input port controls the rate. In order to set the rate to
//   R, where R is a fraction in the range (0,1], set throttle (T) using the
//   formula T = (1/R)-1. In other words, R = 1/(T+1).
//
//   The "throttle" input is represented as an unsigned fixed-point value with
//   THROTTLE_W/2 whole bits and THROTTLE_W/2 fractional bits (UQn.n format).
//   Throttle is therefore in the range [0,1).
//
//   For example, if THROTTLE_W is 8 bits then throttle is in UQ4.4 format (4
//   whole bits and 4 fractional bits). In this case, a throttle value of 0
//   corresponds to a rate of 1/(0+1) = 1.0, which is 100% or full throttle. A
//   throttle value of 15.9375 (i.e., 0xFF, or the max value) corresponds to a
//   rate of 1/(15.9375+1) = 0.05904059, or 5.9% of the maximum rate.
//
//   The throttle input is sampled between packets. Changing the throttle
//   during a packet, or before its inserted stall time has elapsed, has no
//   effect until the next packet.
//
// Parameters:
//
//   THROTTLE_W : Width of the throttle input in bits.
//   DATA_W     : Width of data bus for AXI-Stream in bits.
//   MTU        : The maximum supported packet length is 2**MTU.
//

`default_nettype none


module axis_pkt_throttle #(
  parameter int THROTTLE_W = 8,
  parameter int DATA_W     = 64,
  parameter int MTU        = 10
) (
  input  wire clk,
  input  wire rst,

  input  wire [THROTTLE_W-1:0] throttle,

  // Input AXI-Stream
  input  wire [ DATA_W-1:0] i_tdata,
  input  wire               i_tlast,
  input  wire               i_tvalid,
  output wire               i_tready,

  // Output AXI-Stream
  output wire [ DATA_W-1:0] o_tdata,
  output wire               o_tlast,
  output wire               o_tvalid,
  input  wire               o_tready
);

  //---------------------------------------------------------------------------
  // Throttle Control Logic
  //---------------------------------------------------------------------------
  //
  // This logic monitors the data flow and determines when we should pass data
  // through and when we should stall in order to limit the data rate.
  //
  //---------------------------------------------------------------------------

  // Length of the fractional part of our fixed-point throttle and count.
  localparam int FRAC_W = THROTTLE_W/2;
  // Length of the whole-number part of our fixed-point throttle.
  localparam int WHOLE_W = THROTTLE_W - FRAC_W;
  // Width of an unsigned fixed-point value to track the amount of time to
  // ensure that we have between the starts of packets. This must be large
  // enough to store (2**MTU) * (2**THROTTLE_W-1).
  localparam int TIME_W = MTU + THROTTLE_W;
  // Width of an unsigned counter to track time between packets. Same as
  // TIME_W, but the whole-number part.
  localparam int COUNT_W = TIME_W - FRAC_W;
  // Compute the minimum count value we can have and still guarantee that the
  // next stall time adjustment won't cause underflow.
  localparam longint MIN_COUNT = -(2**TIME_W) + (2**THROTTLE_W-1);

  // Fixed-point accumulator that tracks amount of time to stall between
  // packets. We add an extra bit for the sign since this value can be negative.
  logic signed [TIME_W:0] stall_time;
  // Counter to track the whole number of clock cycles to stall.
  logic [COUNT_W-1:0] wait_count;
  // Flag to indicate if underflow occurred and our count can't be trusted.
  logic underflow;
  // Register to control the flow of packets through this module. When 1, data
  // flow is gated (stopped).
  logic gate = 1'b0;
  // Start of packet flag.
  logic sop = 1'b1;

  always_ff @(posedge clk) begin : throttle_control
    if (gate) begin
      wait_count <= wait_count-1;
      gate       <= (wait_count > 1);
      // Update stall_time for next packet, in case throttle changes.
      stall_time <= throttle;
    end else begin
      if (i_tvalid && o_tready) begin
        if (i_tlast) begin
          sop <= 1;
          // End of the packet. Start stalling, if needed, and reset for the
          // next packet.
          if (!underflow && !stall_time[TIME_W]) begin
            // No underflow and stall_time is non-negative, so start stalling
            // the accumulated amount.
            wait_count <=  stall_time[FRAC_W+:COUNT_W];
            gate       <= (stall_time[FRAC_W+:COUNT_W] != 0);
          end else begin
            // We underflowed or stall_time was negative, so don't stall.
            wait_count <= 0;
            gate       <= 0;
          end
          // Reset for next packet
          underflow  <= 0;
          stall_time <= throttle;
        end else begin
          // A transfer is happening this cycle. Update stall time. Note that
          // overflow is not possible as long as the MTU is honored.
          stall_time <= stall_time + throttle;
          sop <= 0;
        end
      end else begin
        if (sop) begin
          // We're in between packets. Update stall_time for next packet, in
          // case throttle changes.
          stall_time <= throttle;
        end else begin
          // An idle cycle (no transfer) is occurring this cycle so subtract
          // 1.0 from our stall time. We must check for underflow since there
          // is no limit to the number of idle cycles we might see.
          stall_time <= stall_time - (1 << FRAC_W);
          if (stall_time < MIN_COUNT) begin
            underflow <= 1;
          end
        end
      end
    end

    if (rst) begin
      sop        <= 1;
      stall_time <= 0;
      wait_count <= 'X;  // Don't care
      gate       <= 0;
      underflow  <= 0;
    end
  end : throttle_control

  //---------------------------------------------------------------------------
  // Data Pass-Through
  //---------------------------------------------------------------------------

  assign o_tdata  = i_tdata;
  assign o_tlast  = i_tlast;
  assign o_tvalid = i_tvalid & ~gate;
  assign i_tready = o_tready & ~gate;

endmodule : axis_pkt_throttle


`default_nettype wire
