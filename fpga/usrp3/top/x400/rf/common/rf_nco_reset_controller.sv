//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_nco_reset_controller
//
// This entity has the logic needed to synchronously reset the NCO inside the
// RF section.
//

module rf_nco_reset_controller(
  input wire config_clk,
  input wire pll_ref_clk,
  input wire data_clk,
  input wire p_sysref,
  input wire d_start_nco_reset,

  // DAC common NCO update controls and status.
  input wire [1:0] c_dac0_nco_update_busy,
  output wire c_dac0_nco_update_req,
  output wire c_dac0_sysref_int_gating,
  output wire c_dac0_sysref_int_reenable,

  // DAC Tile 229
  // DAC common NCO update controls and status.
  input wire c_dac1_nco_update_busy,
  // Update request for DAC 1 and all ADC tiles
  output wire c_all_other_nco_update_req,

  // ADC common NCO update controls and status.
  // ADC Tile 224
  input wire c_adc0_nco_update_busy,

  // ADC Tile 225
  input wire c_adc1_nco_update_busy,

  // ADC Tile 226
  input wire c_adc2_nco_update_busy,

  // ADC Tile 227
  input wire c_adc3_nco_update_busy,

  // NCO reset can be initiated only when c_nco_phase_rst is set to '1' and
  // c_nco_update_en = 0x20. The FSM in this entity will set these values when
  // an NCO reset is initiated during synchronization. These ports are common
  // for all the converters. So, we will fan these signals out to each
  // converter outside this entity.
  output reg c_nco_phase_rst,
  output reg [5:0] c_nco_update_en,

  // NCO reset status back to the user.
  output reg d_nco_reset_done = 1'b0,
  output reg d_nco_sync_failed = 1'b0,
  input wire [7:0] d_sysref_wait_cycles
);

  reg p_sysref_pulse = 1'b0;
  reg p_sysref_dlyd = 1'b0;
  reg p_sysref_int_gating = 1'b0;
  reg p_dac0_sysref_int_reenable = 1'b0;

  wire [1:0] p_dac0_nco_update_busy;
  // Register to ensure that start pulse from software is not missed
  reg d_start_nco_reset_reg  = 1'b0;
  // Update request for DAC 0 tile (must be asserted first)
  reg p_dac0_nco_update_req = 1'b0;
  // Update request for all remaining tiles
  reg p_all_other_nco_update_req = 1'b0;
  // Used to reset d_start_nco_reset_reg
  reg p_clear_nco_reset_reg = 1'b0;

  wire p_nco_reset_complete;
  reg c_nco_reset_complete = 1'b0;
  reg p_nco_sync_failed = 1'b0;

  reg [7:0] p_sysref_counter = 8'b0;

  typedef enum logic [2:0] {IDLE, REQ_GATING, CHECK_GATING, CHECK_UPDATE_DONE,
            CHECK_RESET_DONE} ResetState_t;

  ResetState_t p_reset_state = IDLE;
  assign c_nco_phase_rst = 1'b1;
  assign c_nco_update_en = 6'b100000;

  // Transfer reset complete signal to PRC domain
  synchronizer #(
    .WIDTH             (1),
    .STAGES            (2),
    .INITIAL_VAL       ('0),
    .FALSE_PATH_TO_IN  (1)
  ) nco_reset_complete_sync (
    .clk  (pll_ref_clk),
    .rst  (1'b0),
    .in   (c_nco_reset_complete),
    .out  (p_nco_reset_complete)
  );
  // Transfer c_dac0_nco_update_busy to PRC domain
  synchronizer #(
    .WIDTH             (2),
    .STAGES            (2),
    .INITIAL_VAL       ('0),
    .FALSE_PATH_TO_IN  (1)
  ) dac0_update_busy_sync (
    .clk  (pll_ref_clk),
    .rst  (1'b0),
    .in   (c_dac0_nco_update_busy),
    .out  (p_dac0_nco_update_busy)
  );

  // NCO start signal from the user is a one data_clk cycle strobe. In this
  // process, we register the NCO start request from the user. This NCO start
  // request register is cleared after the NCO reset sequence is initiated.
  always_ff @(posedge data_clk) begin
    if (p_clear_nco_reset_reg == 1'b1) begin
      d_start_nco_reset_reg <= 1'b0;
    end
    else if (d_start_nco_reset == 1'b1) begin
      d_start_nco_reset_reg <= 1'b1;
    end
  end

  // Check if all data converters have completed their NCO reset
  always_ff @(posedge config_clk) begin
    c_nco_reset_complete <= !c_dac1_nco_update_busy & !c_adc0_nco_update_busy & !c_adc1_nco_update_busy
    & !c_adc2_nco_update_busy & !c_adc3_nco_update_busy;
  end

  // Reset FSM
  always_ff @(posedge pll_ref_clk) begin
    p_sysref_dlyd <= p_sysref;
    p_sysref_pulse <= p_sysref & !p_sysref_dlyd;

    p_dac0_nco_update_req      <= 1'b0;
    p_all_other_nco_update_req <= 1'b0;
    p_dac0_sysref_int_reenable <= 1'b0;

    if (p_reset_state != IDLE && p_reset_state != CHECK_RESET_DONE) begin
      // Increment counter during the reset procedure
      if (p_sysref_pulse) begin
        p_sysref_counter <= p_sysref_counter + 1;
      end
      // Set NCO sync failed bit if update took too long
      if (p_sysref_counter > d_sysref_wait_cycles) begin
        p_nco_sync_failed <= 1'b1;
      end
    end

    case (p_reset_state)
      // Stay in this state until NCO reset sequence is initiated. NCO reset
      // is initiated only on the rising edge of SYSREF.
      // When NCO reset is initiated, gate the RFDC internal SYSREF. To gate
      // internal SYSREF set cSysrefIntGating to 1.
      IDLE : begin
        if (p_sysref_pulse & d_start_nco_reset_reg) begin
          p_reset_state <= REQ_GATING;
          p_nco_sync_failed <= 1'b0;
          p_sysref_counter <= 8'b0;
          p_sysref_int_gating <= 1'b1;
          d_nco_reset_done <= 1'b0;
        end
      end

      // To request NCO reset strobe c_dac0_nco_update_req for one config_clk period.
      // At this point, we can only request NCO reset for RF-DAC tile 228.
      REQ_GATING : begin
        p_reset_state <= CHECK_GATING;
        p_dac0_nco_update_req <= 1'b1;
        p_clear_nco_reset_reg <= 1'b1;
      end

      // Since we are gating c_dac0_nco_update_busyng SYSREF inside RFDC, we need to wait until SYSREF
      // is gated internally. RFDC sets c_dac0_nco_update_busy[0] to '1' when
      // SYSREF is gated. c_dac0_nco_update_busy[1] is also set to '1' to
      // indicate that NCO reset is still in progress. After the SYSREF is
      // gated request NCO reset on all other converter tiles.
      CHECK_GATING : begin
        if (p_dac0_nco_update_busy == 2'b11) begin
          p_reset_state <= CHECK_UPDATE_DONE;
          p_all_other_nco_update_req <= 1'b1;
        end
      end

      // In this state, we check if the RFDC block is ready for NCO reset.
      // This check is done using the *Busy signal from RFDC. Once RFDC is
      // ready for NCO reset and d_sysref_wait_cycles Sysref edges have arrived,
      // disable internal SYSREF gating.
      CHECK_UPDATE_DONE : begin
        // Wait until the minimum number of sysref cycles is reached
        if (p_dac0_nco_update_busy == 2'b10 & p_nco_reset_complete &
          p_sysref_counter >= d_sysref_wait_cycles) begin
          p_dac0_sysref_int_reenable <= 1'b1;
          p_reset_state <= CHECK_RESET_DONE;
        end
      end

      // Wait until c_dac0_nco_update_busy[1] is set to 0. This indicates that the
      // NCO has completed its reset. p_sysref_int_gating can be set to 0 here,
      // since it does not have an effect after p_dac0_sysref_int_reenable is set to 1.
      CHECK_RESET_DONE : begin
        p_sysref_counter <= 8'b0;

        if (p_dac0_nco_update_busy == 2'b00) begin
          p_reset_state <= IDLE;
          p_sysref_int_gating <= 1'b0;
          d_nco_reset_done <= 1'b1;
          p_clear_nco_reset_reg <= 1'b0;
        end
      end

    endcase

  end

  // Synchronize failed flag to data_clk domain
  always_ff @(posedge data_clk) begin
    d_nco_sync_failed <= p_nco_sync_failed;
  end

  // Transfer control signals for rfdc real time ports to config_clk domain
  synchronizer #(
    .WIDTH             (1),
    .STAGES            (2),
    .INITIAL_VAL       ('0),
    .FALSE_PATH_TO_IN  (1)
  ) sysref_gating_sync (
    .clk  (config_clk),
    .rst  (1'b0),
    .in   (p_sysref_int_gating),
    .out  (c_dac0_sysref_int_gating)
  );

  // Create Dac0 Update request pulse in config_clk domain
  pulse_synchronizer #(
    .MODE("PULSE"), .STAGES(2)
  ) nco_update_req_dac0_sync (
    .clk_a(pll_ref_clk),
    .rst_a(1'b0),
    .pulse_a(p_dac0_nco_update_req),
    .busy_a(),
    .clk_b(config_clk),
    .pulse_b(c_dac0_nco_update_req)
  );
  // Create Update request pulse in config_clk domain for the remaining converters
  pulse_synchronizer #(
    .MODE("PULSE"), .STAGES(2)
    ) nco_update_req_dac1_sync (
    .clk_a(pll_ref_clk),
    .rst_a(1'b0),
    .pulse_a(p_all_other_nco_update_req),
    .busy_a(),
    .clk_b(config_clk),
    .pulse_b(c_all_other_nco_update_req)
  );
  // Create Sysref reenable pulse in config_clk domain
  pulse_synchronizer #(
    .MODE("PULSE"), .STAGES(2)
  ) nco_update_reenable_sync (
    .clk_a(pll_ref_clk),
    .rst_a(1'b0),
    .pulse_a(p_dac0_sysref_int_reenable),
    .busy_a(),
    .clk_b(config_clk),
    .pulse_b(c_dac0_sysref_int_reenable)
  );

endmodule
