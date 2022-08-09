//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_timed
//
// Description:
//
//   DDS (direct digital synthesis) and frequency shift block that supports
//   timed commands via the settings bus.
//
//   This block takes in samples on i_t* and performs a complex multiplication
//   with a digitally synthesized oscillator to implement a digital RF mixer.
//   The output is then scaled (optionally), rounded, and clipped if necessary,
//   then output on o_t*.
//
//   Timed commands allow you to update the SR_FREQ register (the phase
//   increment) at the desired time.
//
//   The TUSER port contains the packet header information:
//
//     tuser[125]  : Has timestamp
//     tuser[124]  : End of burst (EOB)
//     tuser[63:0] : Timestamp
//
//   For the input, i_tuser should be valid for the duration of the packet. For
//   the output, o_tuser is only guaranteed to be valid for the last sample of
//   the packet.
//
// Registers:
//
//   SR_FREQ     : Frequency shift to apply to the input signal. This can be
//                 thought of as an unsigned PHASE_ACCUM_WIDTH-bit register
//                 with PHASE_ACCUM_WIDTH fractional bits. That is, the range
//                 of this register maps to the real values [0,1). This
//                 register controls the amount by which the phase accumulator
//                 for the DDS is incremented each clock cycle. It can
//                 therefore be thought of as a phase angle corresponding to
//                 the range [0,2π) radians.
//   SR_SCALE_IQ : Scaler by which to multiply the IQ outputs. This is a
//                 SCALING_WIDTH-bit signed fixed-point register with 15
//                 fractional bits. If SCALING_WIDTH is 18, then it has the
//                 range [-4,4).
//
// Parameters:
//
//   Note: Care must be used when overriding these parameters because there are
//         many dependencies on them. For example, the DDS_WIDTH and
//         PHASE_WIDTH depend on the configuration of the underlying DDS IP and
//         should only be modified to match that IP.
//
//   SR_FREQ_ADDR      : Register offset to assign to the SR_FREQ register,
//                       which contains the phase increment per sample needed
//                       to achieve the desired DDS frequency.
//   SR_SCALE_IQ_ADDR  : Register offset to assign to the SR_SCALE_IQ register.
//   CMD_FIFO_SIZE     : Log2 of the size of the timed command FIFO to use.
//   WIDTH             : Data width of the I/Q components of the input/output
//                       samples, typically 16.
//   DDS_WIDTH         : Bit width to use for the DDS and complex multiplier.
//   PHASE_WIDTH       : Bit width to use for the phase provided to the DDS IP.
//   PHASE_ACCUM_WIDTH : Bit width to use for the phase increment values.
//   SCALING_WIDTH     : Bit width to use for the IQ scale registers.
//   HEADER_WIDTH      : Width of the header info (tuser).
//   HEADER_FIFO_SIZE  : Log2 of the size of the header FIFO.
//   SR_AWIDTH         : Settings bus address width.
//   SR_DWIDTH         : Settings bus data width.
//   SR_TWIDTH         : Settings bus time width.
//

`default_nettype none


module dds_timed #(
  parameter SR_FREQ_ADDR      = 0,
  parameter SR_SCALE_IQ_ADDR  = 1,
  parameter CMD_FIFO_SIZE     = 5,
  parameter WIDTH             = 16,
  parameter DDS_WIDTH         = 24,
  parameter PHASE_WIDTH       = 24,
  parameter PHASE_ACCUM_WIDTH = 32,
  parameter SCALING_WIDTH     = 18,
  parameter HEADER_WIDTH      = 128,
  parameter HEADER_FIFO_SIZE  = 5,
  parameter SR_AWIDTH         = 8,
  parameter SR_DWIDTH         = 32,
  parameter SR_TWIDTH         = 64
) (
  input wire clk,
  input wire reset,
  input wire clear,

  // Indicates if the timed command FIFO is full
  output wire timed_cmd_fifo_full,

  // Settings bus for register access
  input wire                 set_stb,
  input wire [SR_AWIDTH-1:0] set_addr,
  input wire [SR_DWIDTH-1:0] set_data,
  input wire [SR_TWIDTH-1:0] set_time,
  input wire                 set_has_time,

  // Input sample stream
  input  wire [     2*WIDTH-1:0] i_tdata,
  input  wire                    i_tlast,
  input  wire                    i_tvalid,
  output wire                    i_tready,
  input  wire [HEADER_WIDTH-1:0] i_tuser,

  // Output sample stream
  output wire [     2*WIDTH-1:0] o_tdata,
  output wire                    o_tlast,
  output wire                    o_tvalid,
  input  wire                    o_tready,
  output wire [HEADER_WIDTH-1:0] o_tuser
);

  //---------------------------------------------------------------------------
  // Time Tracking
  //---------------------------------------------------------------------------

  wire [     2*WIDTH-1:0] int_tdata;
  wire [HEADER_WIDTH-1:0] int_tuser;
  wire                    int_tlast;
  wire                    int_tvalid;
  wire                    int_tready;
  wire                    int_tag;
  wire [   SR_AWIDTH-1:0] out_set_addr;
  wire [   SR_AWIDTH-1:0] timed_set_addr;
  wire [   SR_DWIDTH-1:0] out_set_data;
  wire [   SR_DWIDTH-1:0] timed_set_data;
  wire                    out_set_stb;
  wire                    timed_set_stb;

  // This module checks for timed writes to SR_FREQ_ADDR and outputs the
  // register write on timed_set_* (if it was timed) or set_* (if it was not
  // timed). It then tags the sample for which the timed command to
  // SR_FREQ_ADDR should occur by asserting m_axis_data_tag when that sample is
  // output.
  axi_tag_time #(
    .WIDTH        (2*WIDTH),
    .NUM_TAGS     (1),
    .SR_TAG_ADDRS (SR_FREQ_ADDR)
  ) axi_tag_time (
    .clk                 (clk),
    .reset               (reset),
    .clear               (clear),
    .tick_rate           (16'd1),
    .timed_cmd_fifo_full (timed_cmd_fifo_full),
    .s_axis_data_tdata   (i_tdata),
    .s_axis_data_tlast   (i_tlast),
    .s_axis_data_tvalid  (i_tvalid),
    .s_axis_data_tready  (i_tready),
    .s_axis_data_tuser   (i_tuser),
    .m_axis_data_tdata   (int_tdata),
    .m_axis_data_tlast   (int_tlast),
    .m_axis_data_tvalid  (int_tvalid),
    .m_axis_data_tready  (int_tready),
    .m_axis_data_tuser   (int_tuser),
    .m_axis_data_tag     (int_tag),
    .in_set_stb          (set_stb),
    .in_set_addr         (set_addr),
    .in_set_data         (set_data),
    .in_set_time         (set_time),
    .in_set_has_time     (set_has_time),
    .out_set_stb         (out_set_stb),
    .out_set_addr        (out_set_addr),
    .out_set_data        (out_set_data),
    .timed_set_stb       (timed_set_stb),
    .timed_set_addr      (timed_set_addr),
    .timed_set_data      (timed_set_data)
  );

  wire [     2*WIDTH-1:0] dds_in_tdata;
  wire [     2*WIDTH-1:0] unused_tdata;
  wire [HEADER_WIDTH-1:0] header_in_tdata;
  wire [HEADER_WIDTH-1:0] header_out_tdata;
  wire [HEADER_WIDTH-1:0] dds_in_tuser;
  wire                    dds_in_tlast;
  wire                    dds_in_tvalid;
  wire                    dds_in_tready;
  wire                    dds_in_tag;
  wire                    header_in_tvalid;
  wire                    header_in_tready;
  wire                    header_in_tlast;
  wire                    unused_tag;
  wire                    header_out_tvalid;
  wire                    header_out_tready;


  //---------------------------------------------------------------------------
  // Split Stream
  //---------------------------------------------------------------------------
  //
  // Split the data stream into two streams, one with the data/tag (dds_in_t*)
  // and the other with the header (header_in_t*).
  //
  //---------------------------------------------------------------------------

  split_stream #(
    .WIDTH       (2*WIDTH+HEADER_WIDTH+1),
    .ACTIVE_MASK (4'b0011)
  ) split_head (
    .clk       (clk),
    .reset     (reset),
    .clear     (clear),
    .i_tdata   ({ int_tdata, int_tuser, int_tag }),
    .i_tlast   (int_tlast),
    .i_tvalid  (int_tvalid),
    .i_tready  (int_tready),
    .o0_tdata  ({ dds_in_tdata, dds_in_tuser, dds_in_tag }),
    .o0_tlast  (dds_in_tlast),
    .o0_tvalid (dds_in_tvalid),
    .o0_tready (dds_in_tready),
    .o1_tdata  ({ unused_tdata, header_in_tdata, unused_tag }),
    .o1_tlast  (header_in_tlast),
    .o1_tvalid (header_in_tvalid),
    .o1_tready (header_in_tready),
    .o2_tdata  (),
    .o2_tlast  (),
    .o2_tvalid (),
    .o2_tready (1'b0),
    .o3_tdata  (),
    .o3_tlast  (),
    .o3_tvalid (),
    .o3_tready (1'b0)
  );


  //---------------------------------------------------------------------------
  // Header FIFO
  //---------------------------------------------------------------------------
  //
  // Store each packet header in a FIFO to be read out when the packet is
  // output.
  //
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH (HEADER_WIDTH),
    .SIZE  (HEADER_FIFO_SIZE)
  ) axi_fifo_header (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear),
    .i_tdata  (header_in_tdata),
    .i_tvalid (header_in_tvalid & header_in_tlast),
    .i_tready (header_in_tready),
    .o_tdata  (header_out_tdata),
    .o_tvalid (header_out_tvalid),
    .o_tready (header_out_tready),    // Consume header on last output sample
    .space    (),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // Settings Bus Registers
  //---------------------------------------------------------------------------

  wire [PHASE_ACCUM_WIDTH-1:0] phase_inc_tdata;
  wire [PHASE_ACCUM_WIDTH-1:0] phase_inc_timed_tdata;
  wire                         phase_inc_tlast;
  wire                         phase_inc_tvalid;
  wire                         phase_inc_tready;
  wire                         phase_inc_timed_tlast;
  wire                         phase_inc_timed_tready;
  wire                         phase_inc_timed_tvalid;

  // Frequency register (phase increment) used for *un-timed* commands
  axi_setting_reg #(
    .ADDR        (SR_FREQ_ADDR),
    .AWIDTH      (SR_AWIDTH),
    .WIDTH       (PHASE_ACCUM_WIDTH),
    .STROBE_LAST (1)
  ) set_freq (
    .clk      (clk),
    .reset    (reset),
    .set_stb  (out_set_stb),
    .set_addr (out_set_addr),
    .set_data (out_set_data),
    .o_tdata  (phase_inc_tdata),
    .o_tlast  (phase_inc_tlast),
    .o_tvalid (phase_inc_tvalid),
    .o_tready (phase_inc_tready)
  );

  // Frequency register (phase increment) used for *timed* commands
  axi_setting_reg #(
    .ADDR        (SR_FREQ_ADDR),
    .USE_FIFO    (1),
    .FIFO_SIZE   (CMD_FIFO_SIZE),
    .AWIDTH      (SR_AWIDTH),
    .WIDTH       (PHASE_ACCUM_WIDTH),
    .STROBE_LAST (1)
  ) set_freq_timed (
    .clk      (clk),
    .reset    (reset),
    .set_stb  (timed_set_stb),
    .set_addr (timed_set_addr),
    .set_data (timed_set_data),
    .o_tdata  (phase_inc_timed_tdata),
    .o_tlast  (phase_inc_timed_tlast),
    .o_tvalid (phase_inc_timed_tvalid),
    .o_tready (phase_inc_timed_tready)
  );

  wire [SCALING_WIDTH-1:0] scaling_tdata;
  wire                     scaling_tready;

  // Scale value register
  axi_setting_reg #(
    .ADDR    (SR_SCALE_IQ_ADDR),
    .AWIDTH  (SR_AWIDTH),
    .WIDTH   (SCALING_WIDTH),
    .REPEATS (1)
  ) set_scale (
    .clk      (clk),
    .reset    (reset),
    .set_stb  (out_set_stb),
    .set_addr (out_set_addr),
    .set_data (out_set_data),
    .o_tdata  (scaling_tdata),
    .o_tlast  (),
    .o_tvalid (),
    .o_tready (scaling_tready)
  );


  //---------------------------------------------------------------------------
  // Phase Accumulator for DDS
  //---------------------------------------------------------------------------

  wire [PHASE_ACCUM_WIDTH-1:0] phase_inc_mux_tdata;
  reg  [PHASE_ACCUM_WIDTH-1:0] phase_inc;
  wire                         phase_inc_mux_tlast;
  wire                         phase_inc_mux_tvalid;
  wire                         phase_inc_mux_tready;
  reg  [PHASE_ACCUM_WIDTH-1:0] phase;

  wire [PHASE_WIDTH-1:0] phase_tdata  = phase[PHASE_ACCUM_WIDTH-1:PHASE_ACCUM_WIDTH-PHASE_WIDTH];
  wire                   phase_tvalid;
  wire                   phase_tready;
  wire                   phase_tlast;

  wire dds_in_teob = dds_in_tuser[124];

  // Multiplexer to select between the timed and un-timed phase registers.
  assign phase_inc_mux_tdata    = phase_inc_timed_tready ? phase_inc_timed_tdata  : phase_inc_tdata;
  assign phase_inc_mux_tlast    = phase_inc_timed_tready ? phase_inc_timed_tlast  : phase_inc_tlast;
  assign phase_inc_mux_tvalid   = phase_inc_timed_tready ? phase_inc_timed_tvalid : phase_inc_tvalid;
  assign phase_inc_tready       = phase_inc_mux_tready;
  assign phase_inc_timed_tready = phase_inc_mux_tready & dds_in_tag;
  assign phase_inc_mux_tready   = phase_tready;

  // Phase is only valid when input IQ data stream is valid
  assign phase_tvalid = dds_in_tvalid;
  assign phase_tlast  = dds_in_tlast;

  // Phase increment register, sourced from either the timed or un-timed
  // SR_FREQ register.
  always @(posedge clk) begin
    if (reset | clear) begin
      phase_inc <= 0;
    end else if (phase_inc_mux_tvalid & phase_inc_mux_tready) begin
      phase_inc <= phase_inc_mux_tdata;
    end
  end

  // Phase accumulator for DDS. This increments the "phase" input provided to
  // the DDS IP.
  always @(posedge clk) begin
    if (reset | clear | (phase_inc_mux_tvalid & phase_inc_mux_tready)) begin
      // Reset the phase on reset or clear, but also whenever the phase
      // increment is updated.
      phase <= 0;
    end else if (dds_in_tvalid & dds_in_tready) begin
      if (dds_in_tlast & dds_in_teob) begin
        // Reset the phase at the end of each burst so we get predictable
        // output.
        phase <= 0;
      end else begin
        // Increment the phase for each new sample.
        phase <= phase + phase_inc;
      end
    end
  end

  //---------------------------------------------------------------------------
  // AXI Sync
  //---------------------------------------------------------------------------
  //
  // Sync the IQ and phase paths' pipeline delay. This is needed to ensure that
  // applying the phase update happens on the correct sample regardless of
  // differences in path delays.
  //
  //---------------------------------------------------------------------------


  wire [PHASE_WIDTH-1:0] phase_sync_tdata;
  wire                   phase_sync_tvalid;
  wire                   phase_sync_tready;
  wire                   phase_sync_tlast;

  wire [    WIDTH*2-1:0] dds_in_sync_tdata;
  wire                   dds_in_sync_tvalid;
  wire                   dds_in_sync_tready;
  wire                   dds_in_sync_tlast;

  axi_sync #(
    .SIZE      (2),
    .WIDTH_VEC ({ PHASE_WIDTH, 2*WIDTH }),   // Vector of 32-bit width values
    .FIFO_SIZE (0)
  ) axi_sync_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear),
    .i_tdata  ({ phase_tdata,       dds_in_tdata       }),
    .i_tlast  ({ phase_tlast,       dds_in_tlast       }),
    .i_tvalid ({ phase_tvalid,      dds_in_tvalid      }),
    .i_tready ({ phase_tready,      dds_in_tready      }),
    .o_tdata  ({ phase_sync_tdata,  dds_in_sync_tdata  }),
    .o_tlast  ({ phase_sync_tlast,  dds_in_sync_tlast  }),
    .o_tvalid ({ phase_sync_tvalid, dds_in_sync_tvalid }),
    .o_tready ({ phase_sync_tready, dds_in_sync_tready })
  );


  //---------------------------------------------------------------------------
  // DDS and Complex Multiplier
  //---------------------------------------------------------------------------

  wire [DDS_WIDTH-1:0] dds_in_i_tdata;
  wire [DDS_WIDTH-1:0] dds_in_q_tdata;

  wire [DDS_WIDTH-1:0] dds_out_i_tdata;
  wire [DDS_WIDTH-1:0] dds_out_q_tdata;
  wire                 dds_out_tlast;
  wire                 dds_out_tvalid;
  wire                 dds_out_tready;

  // Sign extend I and Q to get up to 24 bits.
  sign_extend #(
    .bits_in  (WIDTH),
    .bits_out (DDS_WIDTH)
  ) sign_extend_i (
    .in  (dds_in_sync_tdata[2*WIDTH-1:WIDTH]),
    .out (dds_in_i_tdata)
  );
  sign_extend #(
    .bits_in  (WIDTH),
    .bits_out (DDS_WIDTH)
  ) sign_extend_q (
    .in  (dds_in_sync_tdata[WIDTH-1:0]),
    .out (dds_in_q_tdata)
  );

  // Wrapper for DDS + Complex Multiply. This block expects {q,i} instead of
  // {i,q} data ordering.
  dds_freq_tune_duc dds_freq_tune_duc_i (
    .clk                (clk),
    .reset              (reset | clear),
    // IQ input (signed 24-bit number with 15 fractional bits)
    .s_axis_din_tlast   (dds_in_sync_tlast),
    .s_axis_din_tvalid  (dds_in_sync_tvalid),
    .s_axis_din_tready  (dds_in_sync_tready),
    .s_axis_din_tdata   ({ dds_in_q_tdata, dds_in_i_tdata }),
    // Phase input from DDS (unsigned 24-bit number with 24 fractional bits)
    .s_axis_phase_tlast (phase_sync_tlast),
    .s_axis_phase_tvalid(phase_sync_tvalid),
    .s_axis_phase_tready(phase_sync_tready),
    .s_axis_phase_tdata (phase_sync_tdata), // 24-bit
    // IQ output (signed 24-bit number with 15 fractional bits)
    .m_axis_dout_tlast  (dds_out_tlast),
    .m_axis_dout_tvalid (dds_out_tvalid),
    .m_axis_dout_tready (dds_out_tready),
    .m_axis_dout_tdata  ({dds_out_q_tdata, dds_out_i_tdata})
  );


  //---------------------------------------------------------------------------
  // Scale the IQ Output
  //---------------------------------------------------------------------------

  wire [DDS_WIDTH+SCALING_WIDTH-1:0] scaled_i_tdata;
  wire [DDS_WIDTH+SCALING_WIDTH-1:0] scaled_q_tdata;
  wire                               scaled_tlast;
  wire                               scaled_tvalid;
  wire                               scaled_tready;

  mult #(
    .WIDTH_A     (DDS_WIDTH),
    .WIDTH_B     (SCALING_WIDTH),
    .WIDTH_P     (DDS_WIDTH+SCALING_WIDTH),
    .DROP_TOP_P  (4),
    .LATENCY     (3),
    .CASCADE_OUT (0)
  ) mult_i (
    .clk      (clk),
    .reset    (reset | clear),
    .a_tdata  (dds_out_i_tdata),
    .a_tlast  (dds_out_tlast),
    .a_tvalid (dds_out_tvalid),
    .a_tready (dds_out_tready),
    .b_tdata  (scaling_tdata),
    .b_tlast  (1'b0),
    .b_tvalid (dds_out_tvalid), // Align scaling_tdata with dds_out_tdata
    .b_tready (scaling_tready),
    .p_tdata  (scaled_i_tdata),
    .p_tlast  (scaled_tlast),
    .p_tvalid (scaled_tvalid),
    .p_tready (scaled_tready)
  );

  mult #(
    .WIDTH_A     (DDS_WIDTH),
    .WIDTH_B     (SCALING_WIDTH),
    .WIDTH_P     (DDS_WIDTH+SCALING_WIDTH),
    .DROP_TOP_P  (4),
    .LATENCY     (3),
    .CASCADE_OUT (0)
  ) mult_q (
    .clk      (clk),
    .reset    (reset | clear),
    .a_tdata  (dds_out_q_tdata),
    .a_tlast  (),
    .a_tvalid (dds_out_tvalid),
    .a_tready (),
    .b_tdata  (scaling_tdata),
    .b_tlast  (1'b0),
    .b_tvalid (dds_out_tvalid), // Align scaling_tdata with dds_out_tdata
    .b_tready (),
    .p_tdata  (scaled_q_tdata),
    .p_tlast  (),
    .p_tvalid (),
    .p_tready (scaled_tready)
  );


  //---------------------------------------------------------------------------
  // Round
  //---------------------------------------------------------------------------

  wire [2*WIDTH-1:0] sample_tdata;
  wire               sample_tlast;
  wire               sample_tvalid;
  wire               sample_tready;

  axi_round_and_clip_complex #(
    .WIDTH_IN  (DDS_WIDTH+SCALING_WIDTH),
    .WIDTH_OUT (WIDTH),
    .CLIP_BITS (12)
  ) axi_round_and_clip_complex_i (
    .clk      (clk),
    .reset    (reset | clear),
    .i_tdata  ({scaled_i_tdata, scaled_q_tdata}),
    .i_tlast  (scaled_tlast),
    .i_tvalid (scaled_tvalid),
    .i_tready (scaled_tready),
    .o_tdata  (sample_tdata),
    .o_tlast  (sample_tlast),
    .o_tvalid (sample_tvalid),
    .o_tready (sample_tready)
  );


  //---------------------------------------------------------------------------
  // Output Logic
  //---------------------------------------------------------------------------

  // Throttle output on last sample if header is not valid
  assign header_out_tready = sample_tlast & sample_tvalid & o_tready;
  assign sample_tready     = (sample_tvalid & sample_tlast) ?
                             (header_out_tvalid & o_tready) : o_tready;
  assign o_tvalid          = (sample_tvalid & sample_tlast) ?
                             header_out_tvalid : sample_tvalid;
  assign o_tlast           = sample_tlast;
  assign o_tdata           = sample_tdata;
  assign o_tuser           = header_out_tdata;

endmodule


`default_nettype wire
