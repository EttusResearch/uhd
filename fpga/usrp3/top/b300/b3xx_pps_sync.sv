//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: b3xx_pps_sync
//
// Description:
//
//   This module encapsulates the PPS handling and the related LMK SYNC signal.
//

`default_nettype none


module b3xx_pps_sync
(
  // clock and reset
  input  wire base_ref_clk, // BRC
  input  wire radio_clk_shifted,  // RC
  input  wire radio_clk,    // For LMK Sync output
  input  wire ctrl_clk,     // CC

  input  wire brc_rst,

  // PPS
  input  wire  pps_in_brc,
  output logic pps_in_rclk,

  // LMK control signal
  output logic  lmk_sync_reva = 1'b0,
  output logic  lmk_sync      = 1'b0,
  input  wire   lmk_sync_clk_sel,
  input  wire   lmk_clkin0_sync_sel,

  // Control signals (CC domain)
  // domain external to here?  Or bring that logic in.
  input   wire [26:0] lmk_sync_delay,
  input   wire        lmk_sync_trigger,
  output  logic       lmk_sync_done,
  input   wire  [9:0] pps_in_to_rclk_delay,
  output  logic [3:0] debug
);

  //---------------------------------------------------------------------------
  // LMK sync generation (BRC domain)
  //---------------------------------------------------------------------------

  // Detect rising edge of PPS
  // Ignore the first cycle after a new PPS source is selected as this might cause a change in the
  // PPS signal as the PPS sources are not aligned.
  // Skipping this switching cycle ensures the rising edge is detected from the selected PPS source.
  logic pps_brc_delayed, pps_rising_edge_brc;
  always_ff @(posedge base_ref_clk) begin
    pps_brc_delayed <= pps_in_brc;
  end
  assign pps_rising_edge_brc = pps_in_brc & ~pps_brc_delayed;

  // There is no data coherency guaranteed by this synchronizer, but this is
  // not required. The information is derived in the same clock domain as the
  // sync trigger. Both information in the worst case arrive in the same clock
  // cycle. In the state machine the trigger is changing the state to ARMED.
  // The delay value is required in the ARMED state. This way there is one more
  // clock cycle for this synchronizer to propagate the correct value of all
  // bits.
  logic [26:0] lmk_sync_delay_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1),
    .WIDTH            (27)
  ) synchronizer_sync_delay (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (lmk_sync_delay),
    .out (lmk_sync_delay_brc)
  );

  logic lmk_sync_trigger_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1),
    .WIDTH            (1)
  ) synchronizer_sync_trigger (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (lmk_sync_trigger),
    .out (lmk_sync_trigger_brc)
  );

  // Synchronization state machine
  typedef enum logic [1:0] { IDLE, ARMED, COUNT, DONE } state_t;
  state_t state;

  logic [26:0] delay_counter_brc = 27'd0;
  logic        lmk_sync_done_brc = 1'b0;
  logic        sync_brc = 1'b0;

  always_ff @(posedge base_ref_clk) begin
    if (brc_rst) begin
      sync_brc          <= 1'b0;
      lmk_sync_done_brc <= 1'b0;
      state             <= IDLE;
    end
    else begin
      case (state)
        IDLE: begin
          // Wait for trigger from control interface
          if (lmk_sync_trigger_brc) begin
            state <= ARMED;
          end
        end

        ARMED: begin
          // Wait for the rising edge of PPS and reset counter
          delay_counter_brc <= lmk_sync_delay_brc;
          if (pps_rising_edge_brc) begin
            state <= COUNT;
          end
        end

        // Delay assertion of sync signal by the given number of cycles
        COUNT: begin
          delay_counter_brc <= delay_counter_brc - 1;
          if (delay_counter_brc == 0) begin
            state     <= DONE;
            sync_brc  <= 1'b1;
          end
        end

        // Issue done signal until the trigger is released
        DONE: begin
          sync_brc          <= 1'b0;
          lmk_sync_done_brc <= 1'b1;
          if (lmk_sync_trigger_brc == 0) begin
            state             <= IDLE;
            lmk_sync_done_brc <= 1'b0;
          end
        end

        // In case we run into an undefined state
        default: begin
          state <= IDLE;
        end
      endcase
    end
  end

  // Transfer done signal back to ctrl_clk domain
  synchronizer #(
    .FALSE_PATH_TO_IN (1)
  ) synchronizer_pll_sync_done (
    .clk (ctrl_clk),
    .rst (1'b0),
    .in  (lmk_sync_done_brc),
    .out (lmk_sync_done)
  );

  logic lmk_clkin0_sync_sel_rc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1)
  ) synchronizer_sync_pin_ctrl (
    .clk (radio_clk),
    .rst (1'b0),
    .in  (lmk_clkin0_sync_sel),
    .out (lmk_clkin0_sync_sel_rc)
  );

  logic lmk_clkin0_sync_sel_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1)
  ) synchronizer_sync_pin_ctrl2 (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (lmk_clkin0_sync_sel),
    .out (lmk_clkin0_sync_sel_brc)
  );

  logic sync_rc, sync_rc_out, sync_rc_out_reva;
  always_ff @(posedge radio_clk_shifted) begin
    // Go into radio_clk_shifted domain as it is shifted to right to account
    // for higher clock insertion delay of ext_ref_clk
    sync_rc <= sync_brc;
  end

  always_ff @(posedge radio_clk) begin
    sync_rc_out_reva <= sync_rc && ~lmk_clkin0_sync_sel_rc ;
    sync_rc_out      <= sync_rc &&  lmk_clkin0_sync_sel_rc ;
  end

  logic sync_brc_out, sync_brc_out_reva;
  always_ff @(posedge base_ref_clk) begin
    sync_brc_out_reva <= sync_brc && ~lmk_clkin0_sync_sel_brc;
    sync_brc_out      <= sync_brc &&  lmk_clkin0_sync_sel_brc;
  end

  logic debug_3;
  always_comb begin : lmk_sync_select
    if (lmk_sync_clk_sel == 1'b0) begin
      lmk_sync_reva = sync_rc_out_reva;
      lmk_sync      = sync_rc_out;
      debug_3       = sync_rc_out;
    end else begin
      lmk_sync_reva = sync_brc_out_reva;
      lmk_sync      = sync_brc_out;
      debug_3       = sync_brc_out;
    end
  end

  assign debug[3] = debug_3;


  //---------------------------------------------------------------------------
  // PPS clock domain crossings
  //---------------------------------------------------------------------------
  // In the section below the PPS crosses multiple clock domains.
  // From the generation in BRC clock domain we transfer the signal over to
  // RC using the aligned edge of the external LMK IC.  x4xx also had
  // a radio clock, but for b3xx, our radio clock is the same frequency
  // as the RC (LMK dev_clock), thus our radio clock becomes the RC.

  // BRC       --\____/----\____/----\____/----\____/----\____/----\____/
  // RC        ___/---\___/---\___/---\___/---\___/---\___/---\___/---\__
  //                                      | aligned edge
  // PPS (BRC) __/--------------------------------------------------------
  // PPS (BRC delayed) ___________________/-------------------------------
  //   Has to shift PPS to start on aligned edge.
  //
  // PPS (RC) __________________________________________/----------------
  //                                      |------------->| 2 RC cycles
  //   2 stage synchronizer = 2 RC cycle delay on aligned edge
  //
  // PPS (RC delayed)  __________/----------------------------------------
  //                                                     |------------------
  //   ------------------------->| up to RC frequency cycles
  //   Shifts PPS pulse by up to 1 second (PPS period) to be present in the
  //   clock cycle before the aligned edge.
  //

  //---------------------------------------------------------------------------
  // PPS delay (BRC domain)
  //---------------------------------------------------------------------------
  // This shift register delays the PPS trigger until the appearance of
  // the aligned edge of BRC and RC.
  // This delay has to incorporate the delay of the state machine above from
  // pps to sync output, the delay of the LMK chip from sync edge to aligned
  // edge and delay setting applied to the sync signal. Be sure to reduce the
  // number by 1 at the end to account for the final register.

  logic [9:0] pps_in_to_rclk_delay_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1),
    .WIDTH            (10)
  ) synchronizer_pps_brc_delay (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (pps_in_to_rclk_delay),
    .out (pps_in_to_rclk_delay_brc)
  );

  logic [1023:0]                  pps_shift_reg_brc = 1024'b0;
  (* DONT_TOUCH = "TRUE" *) logic pps_delayed_brc   = 1'b0;
  (* DONT_TOUCH = "TRUE" *) logic pps_delayed_brc_debug   = 1'b0;
  always_ff @(posedge base_ref_clk) begin
    pps_shift_reg_brc     <= {pps_shift_reg_brc[1022:0], pps_in_brc};
    pps_delayed_brc       <= pps_shift_reg_brc[pps_in_to_rclk_delay_brc];
    pps_delayed_brc_debug <= pps_shift_reg_brc[pps_in_to_rclk_delay_brc];
  end

  //---------------------------------------------------------------------------
  // PPS clock domain crossing
  //---------------------------------------------------------------------------
  // On the aligned edge of BRC and RC this synchronizer is just a two stage
  // delay into the RC domain as the edges occur at the same time the tools
  // should make sure we close timing on this edge

  synchronizer #(
    .FALSE_PATH_TO_IN (0)
  ) synchronizer_pps_rclk (
    .clk (radio_clk_shifted),
    .rst (1'b0),
    .in  (pps_delayed_brc),
    .out (pps_in_rclk)
  );

  // Debug signals
  // Exported to verify clock alignment after a SYNC event.
  // This should happen synchronous to base_ref_clk and radio_clk_shifted.
  // It should rise just after the rising edge of both to be stable for the
  // next clock cycle
  assign debug[0] = pps_delayed_brc_debug;

  ODDR #(
    .DDR_CLK_EDGE("SAME_EDGE"), // Optional: "OPPOSITE_EDGE" or "SAME_EDGE"
    .INIT(1'b0),                // Initial state of the O pin (output)
    .SRTYPE("SYNC")             // Set/Reset type: "SYNC" or "ASYNC"
  ) ODDR_clk_debug_1 (
    .Q(debug[1]),               // Connect to the OBUF input
    .C(radio_clk_shifted),      // Clock input from the BUFG
    .CE(1'b1),                  // Clock Enable (always high for continuous clock)
    .R(1'b0),                   // Reset (always low)
    .S(1'b0),                   // Set (always low)
    .D1(1'b1),                  // Data input 1 (High)
    .D2(1'b0)                   // Data input 2 (Low)
  );

  ODDR #(
    .DDR_CLK_EDGE("SAME_EDGE"), // Optional: "OPPOSITE_EDGE" or "SAME_EDGE"
    .INIT(1'b0),                // Initial state of the O pin (output)
    .SRTYPE("SYNC")             // Set/Reset type: "SYNC" or "ASYNC"
  ) ODDR_clk_debug_2 (
    .Q(debug[2]),               // Connect to the OBUF input
    .C(base_ref_clk),           // Clock input from the BUFG
    .CE(1'b1),                  // Clock Enable (always high for continuous clock)
    .R(1'b0),                   // Reset (always low)
    .S(1'b0),                   // Set (always low)
    .D1(1'b1),                  // Data input 1 (High)
    .D2(1'b0)                   // Data input 2 (Low)
  );

endmodule : b3xx_pps_sync

`default_nettype wire
