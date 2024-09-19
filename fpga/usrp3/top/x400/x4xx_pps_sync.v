//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_pps_sync
//
// Description:
//
//   This module encapsulates the PPS handling and the related LMK SYNC signal.
//
// Parameters:
//
//   SIMULATION : When true, lowers 10 MHz PPS base reference clock to 10 kHz
//                to shorten simulation run time.
//

`default_nettype none


module x4xx_pps_sync #(
  parameter SIMULATION = 0
) (
  // clock and reset
  input  wire       base_ref_clk, // BRC
  input  wire       pll_ref_clk,  // PRC
  input  wire       ctrl_clk,     // CC
  input  wire [1:0] radio_clk,    // RC

  input  wire brc_rst,

  // PPS
  input  wire pps_in, // BRC domain
  output wire pps_out_brc,
  output reg  [1:0] pps_out_rc = 2'b00,

  // LMK control signal
  output reg  sync = 1'b0,

  // Control signals (CC domain)
  input  wire [1:0]  pps_select,
  input  wire        pll_sync_trigger,
  input  wire [7:0]  pll_sync_delay,
  output wire        pll_sync_done,
  input  wire [7:0]  pps_brc_delay,
  input  wire [25:0] pps_prc_delay,
  input  wire [9:0]  prc_rc_divider,
  input  wire        pps_rc_enabled,

  //signal for debugging
  output wire [1:0] debug
);

  `include "regmap/global_regs_regmap_utils.vh"

  //---------------------------------------------------------------------------
  // PPS Generation and Capturing (BRC domain)
  //---------------------------------------------------------------------------

  // Divide 10 MHz to 10 kHz in case test mode is activated
  localparam FREQUENCY_10M = SIMULATION ? 32'd10_000 : 32'd10_000_000;
  localparam FREQUENCY_25M = 32'd25_000_000;

  // Generate internal PPS signals, each with a 25% duty cycle, based on
  // the different Reference Clock rates. Only one will be used at a time.
  // Available base reference clock rates are: 10 MHz, 25 MHz
  wire pps_int_10mhz_brc;
  pps_generator #(
    .CLK_FREQ   (FREQUENCY_10M),
    .DUTY_CYCLE (25),
    .PIPELINE   ("OUT")
  ) pps_generator_10mhz (
    .clk   (base_ref_clk),
    .reset (1'b0),
    .pps   (pps_int_10mhz_brc)
  );
  wire pps_int_25mhz_brc;
  pps_generator #(
    .CLK_FREQ   (FREQUENCY_25M),
    .DUTY_CYCLE (25),
    .PIPELINE   ("OUT")
  ) pps_generator_25mhz (
    .clk   (base_ref_clk),
    .reset (1'b0),
    .pps   (pps_int_25mhz_brc)
  );

  // Capture the external PPSs with a FF before sending them to the mux. To be
  // safe, we double-synchronize the external signals. If we meet timing (which
  // we should) then this is a two-cycle delay. If we don't meet timing, then
  // it's 1-2 cycles and our system timing is thrown off--but at least our
  // downstream logic doesn't go metastable!
  wire pps_ext_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (0)
  ) synchronizer_pps_ext (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (pps_in),
    .out (pps_ext_brc)
  );

  // Synchronize the select bits over to the reference clock using a handshake
  // to guarantee transfer of the complete pps_select signal.
  // Further include the LMK sync trigger to make sure both signals are aligned.
  reg  [1:0] pps_select_brc;
  reg        pll_sync_trigger_brc;
  wire       pps_handshake_out_valid;
  wire [2:0] pps_handshake_out_data;
  handshake #(
    .WIDTH (3)
  ) handshake_pps (
    .clk_a   (ctrl_clk),
    .rst_a   (1'b0),
    .valid_a (1'b1),
    .data_a  ({pps_select, pll_sync_trigger}),
    .busy_a  (),
    .clk_b   (base_ref_clk),
    .valid_b (pps_handshake_out_valid),
    .data_b  (pps_handshake_out_data)
  );
  always @(posedge base_ref_clk) begin
    if (pps_handshake_out_valid) begin
      {pps_select_brc, pll_sync_trigger_brc} <= pps_handshake_out_data;
    end
  end

  // PPS MUX - selects internal or external PPS.
  // Generate the signal new_pps_selected to indicate a change in the PPS source.
  reg       pps_brc = 1'b0;
  reg [1:0] pps_select_delayed_brc = 2'b00;
  reg       new_pps_selected = 1'b0;
  always @(posedge base_ref_clk) begin
    // generate PPS signal based on selected source
    case (pps_select_brc)
      PPS_INT_10MHZ: begin
        pps_brc <= pps_int_10mhz_brc;
      end
      PPS_INT_25MHZ: begin
        pps_brc <= pps_int_25mhz_brc;
      end
      default: begin
        pps_brc <= pps_ext_brc;
      end
    endcase

    // Delay the select signal to compare with the current value
    pps_select_delayed_brc <= pps_select_brc;

    // Indicate the first cycle of a newly selected PPS source
    if (pps_select_brc != pps_select_delayed_brc) begin
      new_pps_selected <= 1'b1;
    end else begin
      new_pps_selected <= 1'b0;
    end
  end

  // forward BRC based PPS to output
  assign pps_out_brc = pps_brc;


  //---------------------------------------------------------------------------
  // LMK sync generation (BRC domain)
  //---------------------------------------------------------------------------

  // Detect rising edge of PPS
  // Ignore the first cycle after a new PPS source is selected as this might cause a change in the
  // PPS signal as the PPS sources are not aligned.
  // Skipping this switching cycle ensures the rising edge is detected from the selected PPS source.
  reg  pps_brc_delayed;
  wire pps_rising_edge_brc;
  always @(posedge base_ref_clk) begin
    pps_brc_delayed <= pps_brc;
  end
  assign pps_rising_edge_brc = pps_brc & ~pps_brc_delayed & ~new_pps_selected;

  // There is no data coherency guaranteed by this synchronizer, but this is
  // not required. The information is derived in the same clock domain as the
  // sync trigger. Both information in the worst case arrive in the same clock
  // cycle. In the state machine the trigger is changing the state to ARMED.
  // The delay value is required in the ARMED state. This way there is one more
  // clock cycle for this synchronizer to propagate the correct value of all
  // bits.
  wire [7:0] pll_sync_delay_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1),
    .WIDTH            (8)
  ) synchronizer_sync_delay (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (pll_sync_delay),
    .out (pll_sync_delay_brc)
  );

  // Synchronization state machine
  localparam IDLE  = 2'd0;
  localparam ARMED = 2'd1;
  localparam COUNT = 2'd2;
  localparam DONE  = 2'd3;

  reg [7:0] delay_counter_brc = 8'd0;
  reg [1:0] state = IDLE;
  reg       pll_sync_done_brc = 1'b0;
  reg       sync_int = 1'b0;

  always @(posedge base_ref_clk) begin
    if (brc_rst) begin
      sync_int <= 1'b0;
      pll_sync_done_brc <= 1'b0;
      state <= IDLE;
    end
    else begin
      case (state)
        IDLE: begin
          // Wait for trigger from control interface
          if (pll_sync_trigger_brc) begin
            state <= ARMED;
          end
        end

        ARMED: begin
          // Wait for the rising edge of PPS and reset counter
          delay_counter_brc <= pll_sync_delay_brc;
          if (pps_rising_edge_brc) begin
            state <= COUNT;
          end
        end

        // Delay assertion of sync signal by the given number of cycles
        COUNT: begin
          delay_counter_brc <= delay_counter_brc - 1;
          if (delay_counter_brc == 0) begin
            state <= DONE;
            sync_int <= 1'b1;
          end
        end

        // Issue done signal until the trigger is released
        DONE: begin
          sync_int <= 1'b0;
          pll_sync_done_brc <= 1'b1;
          if (pll_sync_trigger_brc == 0) begin
            state <= IDLE;
            pll_sync_done_brc <= 1'b0;
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
    .in  (pll_sync_done_brc),
    .out (pll_sync_done)
  );

  // Sync signal is captured at falling edge of clock to ensure hold time
  always @(negedge base_ref_clk) begin
    sync <= sync_int;
  end

  //---------------------------------------------------------------------------
  // PPS clock domain crossings
  //---------------------------------------------------------------------------
  // In the section below the PPS crosses multiple clock domains.
  // From the generation in BRC clock domain we transfer the signal over to
  // PRC using the aligned edge of the external LMK IC.
  // Afterwards we use the integer clock multiplier between PRC and RC to
  // get the PPS trigger to the radio clock domain.

  // BRC       --\____/----\____/----\____/----\____/----\____/----\____/
  // PRC       ___/---\___/---\___/---\___/---\___/---\___/---\___/---\__
  // RC        -\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\
  //                                      | aligned edge
  // PPS (BRC) __/--------------------------------------------------------
  // PPS (BRC delayed) ___________________/-------------------------------
  //   Has to shift PPS to start on aligned edge.
  //
  // PPS (PRC) __________________________________________/----------------
  //                                      |------------->| 2 PRC cycles
  //   2 stage synchronizer = 2 PRC cycle delay on aligned edge
  //
  // PPS (PRC delayed) __________/----------------------------------------
  //                                                     |------------------
  //   ------------------------->| up to PRC frequency cycles
  //   Shifts PPS pulse by up to 1 second (PPS period) to be present in the
  //   clock cycle before the aligned edge.
  //
  // PPS (RC)  ___________________________/-\_____________________________
  //                             |------->| RC clock multiplier based cycles
  //   Number of sync registers depends on clock multiplier between PRC and
  //   RC to align PPS signal with aligned edge. Additional logic to restore
  //   a one cycle long pulse from PPS signal with 25% duty cycle.

  //---------------------------------------------------------------------------
  // PPS delay (BRC domain)
  //---------------------------------------------------------------------------
  // This shift register delays the PPS trigger until the appearance of
  // the aligned edge of BRC and PRC.
  // This delay has to incorporate the delay of the state machine above from
  // pps to sync output, the delay of the LMK chip from sync edge to aligned
  // edge and delay setting applied to the sync signal. Be sure to reduce the
  // number by 1 at the end to account for the final register.

  wire [7:0] pps_brc_delay_brc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1),
    .WIDTH            (8)
  ) synchronizer_pps_brc_delay (
    .clk (base_ref_clk),
    .rst (1'b0),
    .in  (pps_brc_delay),
    .out (pps_brc_delay_brc)
  );

  reg [255:0] pps_shift_reg_brc = 256'b0;
  reg         pps_delayed_brc   = 1'b0;
  always @(posedge base_ref_clk) begin
    pps_shift_reg_brc <= {pps_shift_reg_brc[254:0], pps_brc};
    pps_delayed_brc <= pps_shift_reg_brc[pps_brc_delay_brc];
  end

  //---------------------------------------------------------------------------
  // PPS clock domain crossing
  //---------------------------------------------------------------------------
  // On the aligned edge of BRC and PRC this synchronizer is just a two stage
  // delay into the PRC domain as the edges occur at the same time the tools
  // should make sure we close timing on this edge

  wire pps_prc;
  synchronizer #(
    .FALSE_PATH_TO_IN (0)
  ) synchronizer_pps_prc (
    .clk (pll_ref_clk),
    .rst (1'b0),
    .in  (pps_delayed_brc),
    .out (pps_prc)
  );

  //---------------------------------------------------------------------------
  // PPS delay (PRC)
  //---------------------------------------------------------------------------
  // Delay the PPS signal in PRC domain by a specified amount to align with
  // other devices (max delay = 1 sec = next occurrence of pps rising edge).
  // Make sure that the initial count value accounts for the two stage
  // synchronizer from BRC to PRC, the final register upon counter reaches
  // its final value and it has to be one cycle earlier than the aligned edge
  // to get transferred to radio clock afterwards.

  wire [25:0] pps_prc_delay_prc;
  synchronizer #(
    .FALSE_PATH_TO_IN (1),
    .WIDTH            (26)
  ) synchronizer_pps_prc_delay (
    .clk (pll_ref_clk),
    .rst (1'b0),
    .in  (pps_prc_delay),
    .out (pps_prc_delay_prc)
  );

  reg [25:0] delay_counter_prc   = 26'b0;
  reg        pps_delayed_prc     = 1'b0;
  reg        pps_delayed_prc_out = 1'b0;
  reg        pps_prc_delayed     = 1'b0;
  always @(posedge pll_ref_clk) begin
    // Disable delayed rising edge by default
    pps_delayed_prc <= 1'b0;
    // pps_delayed_prc should assert one PRC clock cycle before the aligned edge,
    // so that it can be transferred to the radio clock domain when PRC and radio_clock
    // run at the same rate. pps_delayed_prc_out holds the PPS on PRC domain delayed to
    // the aligned edge.
    pps_delayed_prc_out <= pps_delayed_prc;
    pps_prc_delayed <= pps_prc;

    // Reset counter on rising edge
    if (pps_prc & ~pps_prc_delayed) begin
      delay_counter_prc <= pps_prc_delay_prc;
    end
    else begin
      if (delay_counter_prc != 0) begin
        delay_counter_prc <= delay_counter_prc - 1;
      end
      if (delay_counter_prc == 1) begin
        pps_delayed_prc <= 1'b1;
      end
    end
  end

  //---------------------------------------------------------------------------
  // PPS PRC to radio clock
  //---------------------------------------------------------------------------
  // Tiny shift register to account for the clock multiplier between prc and
  // rc. The divider has to account for the output register and the shift
  // register.

  genvar rc_sync_i;
  generate
    for (rc_sync_i = 0; rc_sync_i < 2; rc_sync_i = rc_sync_i+1) begin : gen_rc_sync

      wire [ 4:0] prc_rc_divider_rc;
      reg  [ 4:0] prc_rc_divider_reg_rc = 5'b0;
      wire        prc_rc_divider_valid;
      wire        pps_rc_enabled_rc;
      // Make signal one bit longer than maximum divider value to enable t-1 comparison.
      reg  [31:0] pps_shift_reg_rc = 32'b0;

      handshake #(
        .WIDTH            (5)
      ) synchronizer_prc_rc_divider (
        .clk_a    (ctrl_clk),
        .rst_a    (1'b0),
        .valid_a  (1'b1),
        .data_a   (prc_rc_divider[5*rc_sync_i+:5]),
        .busy_a   (),
        .clk_b    (radio_clk[rc_sync_i]),
        .valid_b  (prc_rc_divider_valid),
        .data_b   (prc_rc_divider_rc)
      );
      synchronizer #(
        .FALSE_PATH_TO_IN (1)
      ) synchronizer_pps_rc_enabled (
        .clk (radio_clk[rc_sync_i]),
        .rst (1'b0),
        .in  (pps_rc_enabled),
        .out (pps_rc_enabled_rc)
      );

      always @(posedge radio_clk[rc_sync_i]) begin
        if (prc_rc_divider_valid) begin
          prc_rc_divider_reg_rc <= prc_rc_divider_rc;
        end
        pps_shift_reg_rc <= {pps_shift_reg_rc[30:0],  pps_delayed_prc};
        // Restoring a one clock cycle pulse by feeding back to output value.
        pps_out_rc[rc_sync_i] <= pps_shift_reg_rc[prc_rc_divider_reg_rc] &
                                 ~pps_shift_reg_rc[prc_rc_divider_reg_rc+1] & pps_rc_enabled_rc;
      end

    end
  endgenerate

  //---------------------------------------------------------------------------
  // Debug assignment
  //---------------------------------------------------------------------------

  assign debug[0] = pps_delayed_brc;
  assign debug[1] = pps_delayed_prc_out;

endmodule


`default_nettype wire
