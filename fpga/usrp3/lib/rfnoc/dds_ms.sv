//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_ms
//
// Description: Apply a frequency shift by multiplying incoming IQ samples with
// DDS-generated sin/cos for multiple parallel samples-per-clock (SPC). Supports
// untimed and timed phase changes.
//
// ASCII Diagram (high-level)
//                                      ┌─────────────────────────────────────────────────────────┐
//  clk, rst   ─────────────────────────┤                         dds_ms                          │
//  s_axis_phase_tdata/tuser ───────────┤──────────────┐                                          │
//  s_axis_din_tuser[SPC:1](tag) ───────┤──────┐       │                                          │
//                                      │  ┌───▼────────────────┐   phase_inc   ┌─────────────┐   │
//                                      │  │ FSM + phase buffer │──────────────►│ phase_accum │   │
//                                      │  │ timed tag detect   │               └─────┬───────┘   │
//                                      │  └─────────┬──────┬───┘                     │           │
//                                      │            │      │       ┌─────────────────▼────────┐  │
//                                      │            │      │       │ gen_phase_offset         │  │
//                                      │            │      │       │ phase_offset[n] = n*inc  │  │
//                                      │            │      │       └─────────────────┬────────┘  │
//                                      │   data_lock│      │        ┌────────────────▼──────┐    │
//                                      │            │      │        │ dds_sin_cos_lut_only  │    │
//                                      │            │      │        │ per-lane NCOs (x SPC) │    │
//                                      │            │      │        └────────────────┬──────┘    │
//                                      │            │      └────── flush_enabled ───►│           │
//                                      │            │                                │           │
//  s_axis_din_tdata ────────────┐      │    ┌───────▼─────┐   din_tdata        ┌─────▼──────┐    │
//   (SPC * WIDTH)               ├──────│───►│ axi_fifo    │───────────────────►│complex mult │   │
//  tag, s_axis_din_tuser[0](EoB)┘      │    │ 2 elements  │                    │   (x SPC)   │   │
//                                      │    │data + tuser │                    └──────┬──────┘   │
//                                      │    └────┬────────┘                           │          │
//                                      │         │ din_tuser[0]                       │          │
//                                      │         └─────────────►┌──────────┐          │          │
//                                      │                        │tuser_fifo│ EoB tuser│          │
//                                      │                        │mult delay│─────────►│          │
//                                      │                        └──────────┘      ┌───▼──────┐   │
//                                      │                                          │reassemble│   │
//                                      │                                          │lanes / IQ│   │
//                                      │                                          └────┬─────┘   │
//                                      │                                  m_axis_dout_tdata      │
//                                      │                                  tvalid/tlast lane 0    │
//                                      └─────────────────────────────────────────────────────────┘
//
//  Multisample DDS: host writes phase increment via axi stream interface. Untimed writes reload
//  the phase accumulator on next transfer. Timed writes set s_axis_phase_tuser, while the
//  input data and tags pass through a FIFO until the tagged sample aligns with the phase
//  reload point. The module then marks the first post-tag LUT word, drains stale NCO outputs,
//  and releases the tagged IQ sample once the updated NCO reaches the multiplier. The end-of-
//  burst tuser bit passes through a separate FIFO to match the multiply pipeline latency; lane-0
//  controls the AXI stream ready/valid/last outputs.
//
// parameters:
// - SPC         : number of samples per clock cycle
// - SAMP_W      : width of each input sample (I and Q are packed in one sample,
//                 I and Q are each SAMP_W/2 bits)
// - SAMP_FRAC_W : number of fractional bits in the input IQ samples
// - PHASE_WIDTH : width of phase input to the DDS LUT
//
`default_nettype none

module dds_ms
  import ctrlport_pkg::*;
#(
  parameter SPC               = 1,
  parameter SAMP_W            = 32,
  parameter SAMP_FRAC_W       = 15,
  parameter PHASE_WIDTH       = 24
) (
  input wire logic clk,
  input wire logic rst,

  // IQ input
  input wire logic [SPC-1:0][SAMP_W-1:0]   s_axis_din_tdata,
  input wire logic                         s_axis_din_tlast,
  output     logic                         s_axis_din_tready,
  input wire logic                         s_axis_din_tvalid,
  // tuser[SPC:1] - time-tagged sample
  // tuser[0]     - end of burst
  input wire logic [SPC:0]                 s_axis_din_tuser,

  // Phase input
  input wire logic [PHASE_WIDTH-1:0]       s_axis_phase_tdata,
  output     logic                         s_axis_phase_tready,
  input wire logic                         s_axis_phase_tvalid,
  input wire logic                         s_axis_phase_tuser, // Indicates timed phase update

  // IQ output
  output     logic [SPC-1:0][SAMP_W-1:0]   m_axis_dout_tdata,
  output     logic                         m_axis_dout_tlast,
  input wire logic                         m_axis_dout_tready,
  output     logic                         m_axis_dout_tvalid,
  output     logic                         m_axis_dout_tuser  // Propagated tuser for end of burst
);
  //-----------------------------------------------------------------------------
  // Local parameters
  //-----------------------------------------------------------------------------
  localparam int SIN_COS_WIDTH   = 16;
  localparam int SPC_LOG2        = $clog2(SPC);
  // increase phase width by 3 because phase accumulator uses format #Q2.x,
  // pad phase with 3 zeros and then remove 3 MSBs to pass to lut
  localparam int PHASE_ACC_WIDTH = PHASE_WIDTH + 3;

  //----------------------------------------------------------------------------
  // Signal definition
  //----------------------------------------------------------------------------
  // Reset
  logic                        rst_stretch;

  // Timed phase change
  logic                        timed_update_pending;
  logic                        phase_update_pending;

  // State machine inputs
  logic [SPC:0]                din_tuser;
  logic                        din_tvalid;
  logic                        din_transfer_complete;

  // Phase increment
  logic [PHASE_WIDTH-1:0]      phase_inc_tdata = '0;
  logic                        phase_inc_tlast;
  logic                        phase_inc_tvalid;
  logic                        phase_inc_tready;

  // Flushing
  logic                        data_lock;
  logic                        flush_enabled;

  // Data FIFO output
  logic [SPC-1:0][SAMP_W-1:0]  din_tdata;
  logic                        din_tlast;
  logic [SPC-1:0]              din_tready;

  // Phase accumulator output
  logic [PHASE_ACC_WIDTH-1:0]  phase_accum_out_tdata;
  logic                        phase_accum_out_tlast;
  logic                        phase_accum_out_tvalid;
  logic [SPC-1:0]              phase_accum_out_tready;
  logic [PHASE_WIDTH-1:0]      base_phase;

  // Per-lane phase
  logic [PHASE_WIDTH-1:0]      phase_offset [SPC];
  logic [PHASE_WIDTH-1:0]      phase_tdata [SPC];
  logic                        phase_tlast [SPC];
  logic                        phase_tvalid [SPC];
  logic                        phase_tready [SPC];

  // Phasor
  logic [SIN_COS_WIDTH*2-1:0]  phasor_tdata [SPC];
  logic                        phasor_tlast [SPC];
  logic                        phasor_tvalid [SPC];
  logic                        phasor_tready [SPC];

  // Multiplier output
  logic [SPC-1:0]              mult_out_tlast;
  logic [SPC-1:0]              mult_out_tvalid;

  // tuser delay FIFO
  logic                        tuser_fifo_in_tdata;
  logic                        tuser_fifo_in_tvalid;
  logic                        tuser_fifo_in_tready;
  logic                        tuser_fifo_out_tdata;

  // Output
  logic                        last_transfer;

  //----------------------------------------------------------------------------
  // State Machine for phase updates and flushing control
  //----------------------------------------------------------------------------
  typedef enum {
    IDLE,
    WAIT_FOR_TRANSFER,
    WAIT_FOR_TAG,
    UPDATE_ACCUMULATOR,
    FLUSH
  } state_t;

  state_t current_state;

  always_ff @(posedge clk) begin
    if (rst) begin
      current_state <= IDLE;
      timed_update_pending <= '0;
      phase_update_pending <= '0;
    end else begin
      case (current_state)
        // wait for new phase increment to be requested
        IDLE: begin
          if (s_axis_phase_tvalid) begin
            phase_inc_tdata <= s_axis_phase_tdata;
            timed_update_pending <= s_axis_phase_tuser;
            // Check if transfer can be blocked in the next cycle
            if (!din_transfer_complete) begin
              current_state <= WAIT_FOR_TRANSFER;
            end else begin
              current_state <= WAIT_FOR_TAG;
              phase_update_pending <= '1;
            end
          end
        end
        // Wait for data transfer to start blocking
        WAIT_FOR_TRANSFER: begin
          if (din_transfer_complete) begin
            current_state <= WAIT_FOR_TAG;
            phase_update_pending <= '1;
          end
        end
        // wait for timed tag to arrive at the multiplier input (for timed command only)
        WAIT_FOR_TAG: begin
          if ((din_tuser[SPC:1] != '0 && din_tvalid) || !timed_update_pending) begin
            current_state <= UPDATE_ACCUMULATOR;
          end
        end
        // 1 cycle to start phase accumulator update
        UPDATE_ACCUMULATOR: begin
          if (phase_inc_tready) begin
            current_state <= FLUSH;
          end
        end
        // wait for the new phase to be propagated through LUT
        // just check data path of lane 0, since all lanes are aligned
        FLUSH: begin
          if (phasor_tvalid[0] && phasor_tlast[0] && phasor_tready[0]) begin
            current_state <= IDLE;
            timed_update_pending <= 1'b0;
            phase_update_pending <= 1'b0;
          end
        end
      endcase
    end
  end

  // output signals driven by state machine
  always_comb begin
    // default assignments
    s_axis_phase_tready = '0;
    phase_inc_tlast = '0;

    case (current_state)
      // state machine is ready to consume new phase increment
      IDLE : begin
        s_axis_phase_tready = '1;
      end
      // update the accumulator
      UPDATE_ACCUMULATOR : begin
        phase_inc_tlast = '1;
      end
      // nothing happens in other states
      default: begin
        ;
      end
    endcase
  end

  //----------------------------------------------------------------------------
  // Flushing mechanism
  //----------------------------------------------------------------------------
  // Input samples are first buffered in data_fifo and then presented at its
  // AXI-Stream output. Before changing phase, the FSM lets any current output
  // transfer complete. For an untimed update, the next sample presented at the
  // FIFO output is then stalled while the new phase propagates through the
  // accumulator and LUT. For a timed update, the sample is stalled when the
  // time-tagged word reaches the FIFO output. data_lock prevents the stalled
  // sample from entering the multiplier until the corresponding new phasor is
  // available.

  assign din_transfer_complete = (din_tvalid && din_tready[0]) || !din_tvalid;
  assign data_lock = (!timed_update_pending || (din_tvalid && din_tuser[SPC:1] != '0)) && phase_update_pending;
  assign flush_enabled = current_state == FLUSH || current_state == UPDATE_ACCUMULATOR;


  // --------------------------------------------------------------------------
  // Delay data (break combinatorial path from multiplier to input)
  // --------------------------------------------------------------------------
  axi_fifo #(
    .WIDTH (SPC*SAMP_W + SPC + 2),
    .SIZE  (1)
  ) data_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  ({s_axis_din_tdata, s_axis_din_tuser, s_axis_din_tlast}),
    .i_tvalid (s_axis_din_tvalid),
    .i_tready (s_axis_din_tready),
    .o_tdata  ({din_tdata, din_tuser, din_tlast}),
    .o_tvalid (din_tvalid),
    .o_tready (din_tready[0]),
    .space    (),
    .occupied ()
  );

  // --------------------------------------------------------------------------
  // Phase accumulator
  // --------------------------------------------------------------------------
  // Phase is always incremented and just taken as required through AXI
  // handshake. tlast is actively controlled to trigger phase updates.
  assign phase_inc_tvalid = '1;

  phase_accum #(
    .WIDTH_ACCUM (PHASE_ACC_WIDTH),
    .WIDTH_IN    (PHASE_ACC_WIDTH),
    .WIDTH_OUT   (PHASE_ACC_WIDTH)
  ) phase_accum (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    // Sign-extend phase_inc_tdata to PHASE_ACC_WIDTH bits and left-shift
    // by SPC_LOG2 so the per-clock phase advance equals SPC*freq_word.
    // (The host writes freq_word directly without considering SPC).
    .i_tdata  (PHASE_ACC_WIDTH'(signed'(phase_inc_tdata)) << SPC_LOG2),
    .i_tlast  (phase_inc_tlast),
    .i_tready (phase_inc_tready),
    .i_tvalid (phase_inc_tvalid),
    .o_tdata  (phase_accum_out_tdata),
    .o_tlast  (phase_accum_out_tlast),
    .o_tready (phase_accum_out_tready[0]),
    .o_tvalid (phase_accum_out_tvalid)
  );

  // remove the padding bits
  assign base_phase = phase_accum_out_tdata[PHASE_WIDTH-1:0];

  // -----------------------------------------------------------------------------
  // Per-lane phase generation
  // -----------------------------------------------------------------------------

  // Generate phase offsets for each LUT to process SPC samples in parallel.
  // Base phase increment is derived from the input frequency shift/phase
  // increment value. Each subsequent phase offset is computed as a multiple
  // of the base phase increment.
  for (genvar i=0; i < SPC; i++) begin : gen_phase_inc
    always_ff @(posedge clk) begin
      if (rst) begin
        phase_offset[i] <= '0;
      end else if (phase_inc_tvalid && phase_inc_tready && phase_inc_tlast) begin
        phase_offset[i] <= i * phase_inc_tdata;
      end
    end

    // Calculate the phase for each SPC lane
    logic [PHASE_WIDTH-1:0] next_phase;
    assign next_phase = base_phase + phase_offset[i];

    axi_fifo #(
      .WIDTH (PHASE_WIDTH+1),
      .SIZE  (0)
    ) phase_fifo (
      .clk      (clk),
      .reset    (rst),
      .clear    (1'b0),
      .i_tdata  ({next_phase, phase_accum_out_tlast}),
      .i_tvalid (phase_accum_out_tvalid),
      .i_tready (phase_accum_out_tready[i]),
      .o_tdata  ({phase_tdata[i], phase_tlast[i]}),
      .o_tvalid (phase_tvalid[i]),
      .o_tready (phase_tready[i]),
      .space    (),
      .occupied ()
    );
  end

  // -----------------------------------------------------------------------------
  // Generate the phasor for each SPC lane
  // -----------------------------------------------------------------------------

  // Reset logic: stretch rst for 2 cycles to ensure the DDS LUT is properly rst
  pulse_stretch_min #(2) rst_pulse_stretch (
    .clk(clk),
    .rst(),
    .pulse_in(rst),
    .pulse_out(rst_stretch)
  );

  for (genvar i=0; i < SPC; i++) begin : gen_phasor
    dds_sin_cos_lut_only dds_sin_cos_lut_only_i(
      .aclk                 (clk),
      .aresetn              (~rst_stretch),
      .s_axis_phase_tdata   (phase_tdata[i]),
      .s_axis_phase_tlast   (phase_tlast[i]),
      .s_axis_phase_tready  (phase_tready[i]),
      .s_axis_phase_tvalid  (phase_tvalid[i]),
      .s_axis_phase_tuser   (1'b0),
      .m_axis_data_tdata    (phasor_tdata[i]),
      .m_axis_data_tlast    (phasor_tlast[i]),
      .m_axis_data_tready   (phasor_tready[i]),
      .m_axis_data_tvalid   (phasor_tvalid[i]),
      .m_axis_data_tuser    ()
    );
  end

  // -----------------------------------------------------------------------------
  // IQ multiplication for each SPC lane
  // -----------------------------------------------------------------------------

  for (genvar i=0; i < SPC; i++) begin : gen_mult
    AxiStreamIf #(SAMP_W)          iq_in    (clk, rst);
    AxiStreamIf #(SIN_COS_WIDTH*2) nco_in   (clk, rst);
    AxiStreamIf #(SAMP_W)          mult_out (clk, rst);

    // Switch IQ order from {I, Q} (chdr) to {Q, I} to align with the complex_multiply_iq module
    assign iq_in.tdata      = { << (SAMP_W/2) {din_tdata[i]}};
    assign iq_in.tlast      = din_tlast;
    assign iq_in.tvalid     = din_tvalid && !data_lock; // discard valid during flushing
    assign din_tready[i]    = iq_in.tready && !data_lock;

    assign nco_in.tdata     = phasor_tdata[i];
    assign nco_in.tlast     = phasor_tlast[i];
    assign nco_in.tvalid    = phasor_tvalid[i] & !flush_enabled; // discard valid during flushing
    assign phasor_tready[i] = nco_in.tready || flush_enabled; // allow phasor to drain when flushing

    complex_multiply_iq #(
      .FRACTIONAL_BITS_A(SAMP_FRAC_W),
      .FRACTIONAL_BITS_B(SIN_COS_WIDTH-2),
      .FRACTIONAL_BITS_PRODUCT(SAMP_FRAC_W)
    ) complex_multiply_iq_i (
      .factor_a (iq_in),
      .factor_b (nco_in),
      .product  (mult_out)
    );

    // Switch IQ order from {I, Q} (chdr) to {Q, I} to align with the complex_multiply_iq module
    assign m_axis_dout_tdata[i] = { << (SAMP_W/2) {mult_out.tdata}};
    assign mult_out_tlast[i]    = mult_out.tlast;
    assign mult_out_tvalid[i]   = mult_out.tvalid;
    assign mult_out.tready      = m_axis_dout_tready;
  end

  // -----------------------------------------------------------------------------
  // Delay tuser through the complex multiply pipeline
  // -----------------------------------------------------------------------------
  // Two-stage FIFO to match the complex multiply pipeline latency: a 16-deep
  // shift-register FIFO followed by a 2-element output register. Assumes the
  // complex multiply completes in fewer than 16 (2^4) cycles.
  axi_fifo #(
    .WIDTH (1),
    .SIZE  (4)
  ) tuser_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  (din_tuser[0]),
    .i_tvalid (din_tvalid && din_tready[0]),
    .i_tready (),
    .o_tdata  (tuser_fifo_in_tdata),
    .o_tvalid (tuser_fifo_in_tvalid),
    .o_tready (tuser_fifo_in_tready),
    .space    (),
    .occupied ()
  );
  axi_fifo #(
    .WIDTH (1),
    .SIZE  (1)
  ) tuser_out_reg (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  (tuser_fifo_in_tdata),
    .i_tvalid (tuser_fifo_in_tvalid),
    .i_tready (tuser_fifo_in_tready),
    .o_tdata  (tuser_fifo_out_tdata),
    .o_tvalid (),
    .o_tready ((m_axis_dout_tvalid && m_axis_dout_tready)),
    .space    (),
    .occupied ()
  );


  // -----------------------------------------------------------------------------
  // Output assignments
  // -----------------------------------------------------------------------------
  assign m_axis_dout_tlast    = mult_out_tlast[0];
  assign m_axis_dout_tvalid   = mult_out_tvalid[0];
  // Propagate tuser only on the last word of a burst
  assign last_transfer        = m_axis_dout_tvalid && m_axis_dout_tready && m_axis_dout_tlast;
  assign m_axis_dout_tuser    = last_transfer ? tuser_fifo_out_tdata : 1'b0;

endmodule

`default_nettype wire
