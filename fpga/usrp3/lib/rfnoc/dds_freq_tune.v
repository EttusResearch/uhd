//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_freq_tune
//
// Description:
//
//   Performs a frequency shift on a signal by multiplying it with a complex
//   sinusoid synthesized from a DDS. This module expects samples data to be in
//   {Q,I} order.
//

module dds_freq_tune #(
  parameter WIDTH         = 24,
  parameter PHASE_WIDTH   = 24,
  parameter SIN_COS_WIDTH = 16,
  parameter OUTPUT_WIDTH  = 24
) (
  input clk,
  input reset,

  input eob,
  input rate_changed,

  input [15:0] dds_input_fifo_occupied,

  // IQ input
  input  [WIDTH*2-1:0] s_axis_din_tdata,
  input                s_axis_din_tlast,
  input                s_axis_din_tvalid,
  output               s_axis_din_tready,

  // Phase input from NCO
  input  [PHASE_WIDTH-1:0] s_axis_phase_tdata,
  input                    s_axis_phase_tlast,
  input                    s_axis_phase_tvalid,
  output                   s_axis_phase_tready,

  // IQ output
  output [OUTPUT_WIDTH*2-1:0] m_axis_dout_tdata,
  output                      m_axis_dout_tlast,
  output                      m_axis_dout_tvalid,
  input                       m_axis_dout_tready,

  // Debug signals
  output [                2:0] state_out,
  output                       phase_valid_hold_out,
  output [                7:0] phase_invalid_wait_count_out,
  output                       reset_dds_out,
  output                       m_axis_dds_tlast_out,
  output                       m_axis_dds_tvalid_out,
  output                       m_axis_dds_tready_out,
  output [SIN_COS_WIDTH*2-1:0] m_axis_dds_tdata_out
);

  // Wires for DDS output
  wire                       m_axis_dds_tlast;
  wire                       m_axis_dds_tvalid;
  wire                       m_axis_dds_tready;
  wire [SIN_COS_WIDTH*2-1:0] m_axis_dds_tdata;  // [31:16] = sin|q, [15:0]= cos|i

  reg        reset_reg;
  reg        phase_valid_hold;
  reg  [7:0] phase_invalid_wait_count;
  reg  [2:0] state;
  reg        phase_ready_wait;
  wire       s_axis_phase_tready_dds;

  // Initialize DDS resets to 1, since simulation model requires reset at time
  // 0 to avoid failure.
  reg reset_dds     = 1'b1;
  reg reset_dds_reg = 1'b1;

  // When we're holding valid, make ready low so no new data comes in.
  assign s_axis_phase_tready = s_axis_phase_tready_dds & ~phase_valid_hold;

  localparam INIT       = 3'b000;
  localparam VALID      = 3'b001;
  localparam WAIT       = 3'b010;
  localparam HOLD_VALID = 3'b011;

  // Reset needs to be 2 clk cycles minimum for Xilinx DDS IP
  always @(posedge clk) begin
    reset_reg     <= reset;
    reset_dds_reg <= reset_dds;
  end

  // This state machine resets the DDS when data stops coming and also holds
  // valid high until the last packet has been flushed through the DDS.
  always @(posedge clk) begin
    if(reset) begin
      state                    <= INIT;
      phase_valid_hold         <= 1'b0;
      phase_invalid_wait_count <= 16'h00;
      reset_dds                <= 1'b0;
    end else begin
      case(state)
        INIT : begin
          phase_valid_hold         <= 1'b0;
          phase_invalid_wait_count <= 16'h0000;
          reset_dds                <= 1'b0;
          if(s_axis_phase_tvalid) begin
            state <= VALID;
          end
        end
        VALID : begin
          if(~s_axis_phase_tvalid) begin
            state <= WAIT;
          end
        end
        WAIT : begin
          // Wait until we either get valid data or don't.
          if(m_axis_dds_tready) begin
            // Only increment when the downstream can accept data.
            phase_invalid_wait_count <= phase_invalid_wait_count + 4'b1;
          end
          if(s_axis_phase_tvalid) begin
            // If we get valid data shortly after, then don't push data through
            // and reset.
            state <= INIT;
          end else begin
            if(eob | (phase_invalid_wait_count >= 16'h40) | rate_changed) begin
              // If a valid never comes (EOB)
              state <= HOLD_VALID;
            end
          end
        end
        HOLD_VALID : begin
          // Hold valid to flush data through the DDS. The DDS IP won't empty
          // without additional transfers.
          phase_valid_hold <= 1'b1;
          // Wait for input FIFO to be empty
          if (~s_axis_din_tvalid) begin
            state     <= INIT;
            reset_dds <= 1'b1;
          end
        end
      endcase
    end
  end

  // DDS to generate sin/cos data from phase. It takes in a 24-bit phase value
  // and outputs two 16-bit values, with the sine value in the upper 16 bits
  // and the cosine value in the lower 16-bits.
  //
  // The phase input can be thought of as a 24-bit unsigned fixed-point value
  // with 24 fractional bits. In other words, the integer range of the input
  // maps to the the range [0, 2*pi) in radians.
  //
  // The output consists of two 16-bit signed fixed-point values with 14
  // fractional bits.
  //
  // This IP effectively computes Euler's formula, e^(j*2*pi*x) = cos(2*pi*x) +
  // j*sin(2*pi*x), where x is the phase value, and the output has the real
  // component in the lower bits and the imaginary component in the upper bits.
  dds_sin_cos_lut_only dds_sin_cos_lut_only_i (
    .aclk                (clk),
    .aresetn             (~(reset | reset_reg | reset_dds | reset_dds_reg)),
    .s_axis_phase_tvalid (s_axis_phase_tvalid | phase_valid_hold),
    .s_axis_phase_tready (s_axis_phase_tready_dds),
    .s_axis_phase_tlast  (s_axis_phase_tlast),
    .s_axis_phase_tuser  (1'b0),
    .s_axis_phase_tdata  (s_axis_phase_tdata),   // [23 : 0]
    .m_axis_data_tvalid  (m_axis_dds_tvalid),
    .m_axis_data_tready  (m_axis_dds_tready),
    .m_axis_data_tlast   (m_axis_dds_tlast),
    .m_axis_data_tdata   (m_axis_dds_tdata),     // [31 : 0]
    .m_axis_data_tuser   ()
  );

  wire [        WIDTH*2-1:0] mult_in_a_tdata;
  wire                       mult_in_a_tvalid;
  wire                       mult_in_a_tready;
  wire                       mult_in_a_tlast;
  wire [SIN_COS_WIDTH*2-1:0] mult_in_b_tdata;
  wire                       mult_in_b_tvalid;
  wire                       mult_in_b_tready;
  wire                       mult_in_b_tlast;
  wire [           2*32-1:0] mult_out_tdata;
  wire                       mult_out_tvalid;
  wire                       mult_out_tready;
  wire                       mult_out_tlast;

  axi_sync #(
    .SIZE      (2),
    .WIDTH_VEC ({SIN_COS_WIDTH*2, WIDTH*2}),
    .FIFO_SIZE (0)
  ) axi_sync_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (),
    .i_tdata  ({ m_axis_dds_tdata,  s_axis_din_tdata  }),
    .i_tlast  ({ m_axis_dds_tlast,  s_axis_din_tlast  }),
    .i_tvalid ({ m_axis_dds_tvalid, s_axis_din_tvalid }),
    .i_tready ({ m_axis_dds_tready, s_axis_din_tready }),
    .o_tdata  ({ mult_in_b_tdata,   mult_in_a_tdata   }),
    .o_tlast  ({ mult_in_b_tlast,   mult_in_a_tlast   }),
    .o_tvalid ({ mult_in_b_tvalid,  mult_in_a_tvalid  }),
    .o_tready ({ mult_in_b_tready,  mult_in_a_tready  })
  );

  // Use a complex multiplier to multiply the input sample (A) by the NCO
  // output (B). This multiplier has a 21-bit input A, 16-bit input B, and
  // 32-bit output. Due to AXI-Stream requirements, A is rounded up to 24-bit.
  //
  // Assuming default parameters and unchanged IP, The A input (sample) is
  // 21-bit with 15 fractional bits, and the B input (NCO) is 16-bit with 14
  // fractional bits. The full result would be 21+16+1 = 38 bits, but the
  // output is configured for 32, dropping the lower 6 bits. Therefore, the
  // result has 15+14-6 = 23 fractional bits.
  //
  // a = Input IQ data stream as 48-bit, lower bits i, upper bits q.
  // b = Output of DDS as 32 bit cos/sin, lower bits cos, upper bits sin.
  complex_multiplier_dds complex_multiplier_dds_i (
    .aclk               (clk),
    .aresetn            (~(reset | reset_reg)),
    .s_axis_a_tvalid    (mult_in_a_tvalid),
    .s_axis_a_tready    (mult_in_a_tready),
    .s_axis_a_tlast     (mult_in_a_tlast),
    .s_axis_a_tdata     ({mult_in_a_tdata}),    // [47 : 0]
    .s_axis_b_tvalid    (mult_in_b_tvalid),
    .s_axis_b_tready    (mult_in_b_tready),
    .s_axis_b_tlast     (mult_in_b_tlast),
    .s_axis_b_tdata     (mult_in_b_tdata),      // [31 : 0]
    .m_axis_dout_tvalid (mult_out_tvalid),
    .m_axis_dout_tready (mult_out_tready),
    .m_axis_dout_tlast  (mult_out_tlast),
    .m_axis_dout_tdata  (mult_out_tdata)        // [63 : 0]
  );

  // Round the 32-bit multiplier result down to 24 bits. This moves the binary
  // point so that we go from 23 fractional bits down to 15 fractional bits.
  axi_round_complex #(
    .WIDTH_IN  (32),
    .WIDTH_OUT (OUTPUT_WIDTH)
  ) axi_round_complex_i (
    .clk      (clk),
    .reset    (reset | reset_reg),
    .i_tdata  (mult_out_tdata),
    .i_tlast  (mult_out_tlast),
    .i_tvalid (mult_out_tvalid),
    .i_tready (mult_out_tready),
    .o_tdata  (m_axis_dout_tdata),
    .o_tlast  (m_axis_dout_tlast),
    .o_tvalid (m_axis_dout_tvalid),
    .o_tready (m_axis_dout_tready)
  );

  // Debug
  assign state_out                    = state;
  assign phase_valid_hold_out         = phase_valid_hold;
  assign phase_invalid_wait_count_out = phase_invalid_wait_count;
  assign reset_dds_out                = reset_dds;
  assign m_axis_dds_tlast_out         = m_axis_dds_tlast;
  assign m_axis_dds_tvalid_out        = m_axis_dds_tvalid;
  assign m_axis_dds_tready_out        = m_axis_dds_tready;
  assign m_axis_dds_tdata_out         = m_axis_dds_tdata;

endmodule
