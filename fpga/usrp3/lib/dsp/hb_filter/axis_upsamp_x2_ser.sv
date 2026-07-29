//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_upsamp_x2_ser
//
// Description: Serial x2 upsampler for AXI-Stream (SPC = 1).
//
//  Functional behavior:
//    - Accepts one input sample and expands it into two output samples.
//    - Exactly one sample contains the input sample; the other sample is zero.
//    - INTERPOLATION_PHASE controls the ordering:
//        * 0: {data, zero}
//        * 1: {zero, data}
//    - s_axis_tlast is propagated only on the second emitted sample of the pair,
//      matching end-of-frame semantics after interpolation-by-2 expansion.
//    - Samp 1 is driven combinatorially from s_axis in the same cycle the input
//      is accepted (pass-through), so throughput is 1 sample per 2 clock cycles.
//
//  The FSM drives an output register interface (oreg_*). When EN_FIFO_OUT_REG=1
//  an axi_fifo (SIZE=1, axi_fifo_flop2) register stage at the output breaks the
//  combinatorial path from s_axis to m_axis introduced by pass-through, at the
//  cost of one additional cycle of latency. When EN_FIFO_OUT_REG=0 the FIFO is
//  transparent (SIZE=-1, pure wires) so m_axis_* tracks oreg_* with zero latency
//  but the combinatorial path is preserved.
//
//  State machine (transitions on oreg_tready = output register stage input ready):
//    IDLE_ST      : s_axis_tready=1. Samp 1 is passed through combinatorially from s_axis.
//                   On s_axis_tvalid && oreg_tready  -> SEND_SAMP2_ST (fast path).
//                   On s_axis_tvalid && !oreg_tready -> WAIT_SAMP1_ST (stall path;
//                   input captured into data_buf/last_buf, s_axis_tready deasserts).
//    WAIT_SAMP1_ST: Samp 1 held in data_buf, oreg_tvalid=1. Waits for oreg_tready.
//                   On oreg_tready -> SEND_SAMP2_ST.
//    SEND_SAMP2_ST: Samp 2 driven (zero for phase 0, data_buf for phase 1), oreg_tvalid=1.
//                   On oreg_tready -> IDLE_ST.
//
//  Timing example (sample A with tlast=1, then sample B with tlast=0,
//                  m_axis_tready=1 throughout; output register stage adds 1 cycle latency):
//
//    Phase 0 (data first):
//      Cycle | s_xfer | m_xfer | state      | m_axis_tdata | m_axis_tlast | Notes
//      ------+--------+--------+------------+--------------+--------------+------------------------------------
//        C0  |   1    |   0    | IDLE       |      -       |      -       | Accept A; A pushed to output reg
//        C1  |   0    |   1    | SEND_SAMP2 |      A       |      0       | A exits output reg; zero pushed
//        C2  |   1    |   1    | IDLE       |      0       |      1       | Zero+tlast exits; accept+push B
//        C3  |   0    |   1    | SEND_SAMP2 |      B       |      0       | B exits output reg; zero pushed
//        C4  |   0    |   1    | IDLE       |      0       |      0       | Zero exits output reg
//
//    Phase 1 (zero first):
//      Cycle | s_xfer | m_xfer | state      | m_axis_tdata | m_axis_tlast | Notes
//      ------+--------+--------+------------+--------------+--------------+------------------------------------
//        C0  |   1    |   0    | IDLE       |      -       |      -       | Accept A; zero pushed to output reg
//        C1  |   0    |   1    | SEND_SAMP2 |      0       |      0       | Zero exits; A+tlast pushed
//        C2  |   1    |   1    | IDLE       |      A       |      1       | A+tlast exits; accept+push zero for B
//        C3  |   0    |   1    | SEND_SAMP2 |      0       |      0       | Zero exits; B+tlast pushed
//        C4  |   0    |   1    | IDLE       |      B       |      0       | B+tlast exits
//
//  Backpressure example (Phase 0, oreg_tready=0 during IDLE):
//      Cycle | s_xfer | oreg_xfer | state        | Notes
//      ------+--------+-----------+--------------+------------------------------------------
//        C0  |   1    |     0     | IDLE         | s_valid=1, oreg_ready=0: capture A -> WAIT_SAMP1
//        C1  |   0    |     0     | WAIT_SAMP1   | Hold A on oreg_*, oreg_ready still 0
//        C2  |   0    |     1     | WAIT_SAMP1   | oreg_ready asserts: samp 1 (A) accepted -> SEND_SAMP2
//        C3  |   0    |     1     | SEND_SAMP2   | Zero+tlast accepted -> IDLE
//
// Parameters:
//   SAMP_W: Sample width in bits.
//   INTERPOLATION_PHASE: 0 or 1 to select which interpolation phase to output
//                        for the inserted zero samples.
//                        0: data first, 1: zero first.
//   EN_FIFO_OUT_REG:     When 1 (default), inserts an axi_fifo_flop2 register
//                        stage at the output (axi_fifo SIZE=1) to break the
//                        combinatorial s_axis -> m_axis path at the cost of
//                        1 cycle of added latency. When 0, the output FIFO is
//                        transparent (axi_fifo SIZE=-1, pure wires) and the
//                        combinatorial path is preserved.
//
`default_nettype none

module axis_upsamp_x2_ser #(
  parameter int SAMP_W              = 32,
  parameter bit INTERPOLATION_PHASE = 1'b0,
  parameter bit EN_FIFO_OUT_REG     = 1'b0
) (
  input  wire               clk,
  input  wire               rst,
  input  wire               clr,
  input  wire [SAMP_W-1:0]  s_axis_tdata,
  input  wire               s_axis_tvalid,
  input  wire               s_axis_tlast,
  output logic              s_axis_tready,
  output logic [SAMP_W-1:0] m_axis_tdata,
  output logic              m_axis_tvalid,
  output logic              m_axis_tlast,
  input  wire               m_axis_tready
);
  // Output register interface: FSM outputs fed into the output register stage.
  logic [SAMP_W-1:0] oreg_tdata;
  logic              oreg_tvalid;
  logic              oreg_tlast;
  logic              oreg_tready;

  // State encoding for the pass-through upsampler.
  typedef enum logic [1:0] {
    IDLE_ST,       // Accept input; samp 1 passed through to oreg_* interface.
    WAIT_SAMP1_ST, // oreg_tready was low on arrival; hold buffered samp 1.
    SEND_SAMP2_ST  // Samp 1 accepted; driving samp 2.
  } state_t;

  state_t            state;
  logic [SAMP_W-1:0] data_buf;
  logic              last_buf;

  always_ff @(posedge clk) begin : ser_state_proc
    if (rst || clr) begin
      state    <= IDLE_ST;
      data_buf <= '0;
      last_buf <= 1'b0;
    end else begin
      unique case (state)
        IDLE_ST : begin
          if (s_axis_tvalid) begin
            // Capture input unconditionally; synthesis prunes unused paths.
            data_buf <= s_axis_tdata;
            last_buf <= s_axis_tlast;
            if (oreg_tready) begin
              // Fast path: samp 1 passed through this cycle.
              state <= SEND_SAMP2_ST;
            end else begin
              // Stall path: output register stage full; hold samp 1 in buffer.
              state <= WAIT_SAMP1_ST;
            end
          end
        end
        WAIT_SAMP1_ST : begin
          if (oreg_tready) begin
            state <= SEND_SAMP2_ST;
          end
        end
        SEND_SAMP2_ST : begin
          if (oreg_tready) begin
            state <= IDLE_ST;
          end
        end
        default : state <= IDLE_ST;
      endcase
    end
  end : ser_state_proc

  // Combinational output mux into the output register interface.
  // INTERPOLATION_PHASE is a static parameter so the synthesizer will select
  // exactly one branch per phase.
  always_comb begin : ser_output_proc
    // Safe defaults.
    s_axis_tready = 1'b0;
    oreg_tvalid   = 1'b0;
    oreg_tdata    = '0;
    oreg_tlast    = 1'b0;

    unique case (state)
      IDLE_ST : begin
        // Accept new input; samp 1 passes through combinatorially.
        s_axis_tready = 1'b1;
        oreg_tvalid   = s_axis_tvalid;
        oreg_tlast    = 1'b0;  // tlast always carried on samp 2
        if (INTERPOLATION_PHASE == 1'b0) begin
          oreg_tdata = s_axis_tdata;  // Phase 0: data on samp 1
        end else begin
          oreg_tdata = '0;            // Phase 1: zero on samp 1
        end
      end
      WAIT_SAMP1_ST : begin
        // Re-drive samp 1 from buffer until output register stage accepts it.
        s_axis_tready = 1'b0;
        oreg_tvalid   = 1'b1;
        oreg_tlast    = 1'b0;
        if (INTERPOLATION_PHASE == 1'b0) begin
          oreg_tdata = data_buf;  // Phase 0: buffered data on samp 1
        end else begin
          oreg_tdata = '0;        // Phase 1: zero on samp 1
        end
      end
      SEND_SAMP2_ST : begin
        // Drive samp 2; tlast propagated here for both phases.
        s_axis_tready = 1'b0;
        oreg_tvalid   = 1'b1;
        oreg_tlast    = last_buf;
        if (INTERPOLATION_PHASE == 1'b0) begin
          oreg_tdata = '0;        // Phase 0: zero on samp 2
        end else begin
          oreg_tdata = data_buf;  // Phase 1: data on samp 2
        end
      end
      default : begin
        s_axis_tready = 1'b0;
        oreg_tvalid   = 1'b0;
        oreg_tdata    = '0;
        oreg_tlast    = 1'b0;
      end
    endcase
  end : ser_output_proc

  // Output FIFO: SIZE=1 (axi_fifo_flop2) when EN_FIFO_OUT_REG=1, breaking
  // the combinatorial s_axis -> m_axis path at the cost of 1 cycle latency.
  // SIZE=-1 when EN_FIFO_OUT_REG=0, making the FIFO transparent (pure wires)
  // so m_axis_* equals oreg_* with zero latency.
  // tlast is packed into the data MSB and unpacked on output.
  localparam int FIFO_SIZE = EN_FIFO_OUT_REG ? 1 : -1;

  axi_fifo #(
    .WIDTH(SAMP_W + 1),
    .SIZE (FIFO_SIZE)
  ) out_reg_x (
    .clk     (clk),
    .reset   (rst),
    .clear   (clr),
    .i_tdata ({oreg_tlast, oreg_tdata}),
    .i_tvalid(oreg_tvalid),
    .i_tready(oreg_tready),
    .o_tdata ({m_axis_tlast, m_axis_tdata}),
    .o_tvalid(m_axis_tvalid),
    .o_tready(m_axis_tready),
    .space   (),
    .occupied()
  );

endmodule : axis_upsamp_x2_ser

`default_nettype wire
