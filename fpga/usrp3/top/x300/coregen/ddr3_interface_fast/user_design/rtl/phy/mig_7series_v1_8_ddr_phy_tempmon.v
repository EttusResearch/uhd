//*****************************************************************************
// (c) Copyright 2008 - 2012 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
//
//*****************************************************************************
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor                : Xilinx
// \   \   \/     Version               : %version
//  \   \         Application           : MIG
//  /   /         Filename              : mig_7series_v1_8_ddr_phy_tempmon.v
// /___/   /\     Date Last Modified    : $date$
// \   \  /  \    Date Created          : Jul 25 2012
//  \___\/\___\
//
//Device            : 7 Series
//Design Name       : DDR3 SDRAM
//Purpose           : Monitors chip temperature via the XADC and adjusts the
//                    stage 2 tap values as appropriate.
//Reference         :
//Revision History  :
//*****************************************************************************

`timescale 1 ps / 1 ps

module mig_7series_v1_8_ddr_phy_tempmon #
(
  parameter TCQ             = 100,      // Register delay (simulation only)
  // Temperature bands must be in order. To disable bands, set to extreme.
  parameter BAND1_TEMP_MIN  = 0,        // Degrees C. Min=-273. Max=231
  parameter BAND2_TEMP_MIN  = 12,       // Degrees C. Min=-273. Max=231
  parameter BAND3_TEMP_MIN  = 46,       // Degrees C. Min=-273. Max=231
  parameter BAND4_TEMP_MIN  = 82,       // Degrees C. Min=-273. Max=231
  parameter TEMP_HYST       = 5
)
(
  input           clk,                  // Fabric clock
  input           rst,                  // System reset
  input           calib_complete,       // Calibration complete
  input           tempmon_sample_en,    // Signal to enable sampling
  input   [11:0]  device_temp,          // Current device temperature
  output          tempmon_pi_f_inc,     // Increment PHASER_IN taps
  output          tempmon_pi_f_dec,     // Decrement PHASER_IN taps
  output          tempmon_sel_pi_incdec // Assume control of PHASER_IN taps
);

  // translate hysteresis into XADC units
  localparam HYST_OFFSET = (TEMP_HYST * 4096) / 504;

  // translate band boundaries into XADC units
  localparam BAND1_OFFSET = ((BAND1_TEMP_MIN + 273) * 4096) / 504;
  localparam BAND2_OFFSET = ((BAND2_TEMP_MIN + 273) * 4096) / 504;
  localparam BAND3_OFFSET = ((BAND3_TEMP_MIN + 273) * 4096) / 504;
  localparam BAND4_OFFSET = ((BAND4_TEMP_MIN + 273) * 4096) / 504;

  // incorporate hysteresis into band boundaries
  localparam BAND0_DEC_OFFSET =
    BAND1_OFFSET - HYST_OFFSET > 0    ? BAND1_OFFSET - HYST_OFFSET : 0    ;
  localparam BAND1_INC_OFFSET =
    BAND1_OFFSET + HYST_OFFSET < 4096 ? BAND1_OFFSET + HYST_OFFSET : 4096 ;
  localparam BAND1_DEC_OFFSET =
    BAND2_OFFSET - HYST_OFFSET > 0    ? BAND2_OFFSET - HYST_OFFSET : 0    ;
  localparam BAND2_INC_OFFSET =
    BAND2_OFFSET + HYST_OFFSET < 4096 ? BAND2_OFFSET + HYST_OFFSET : 4096 ;
  localparam BAND2_DEC_OFFSET =
    BAND3_OFFSET - HYST_OFFSET > 0    ? BAND3_OFFSET - HYST_OFFSET : 0    ;
  localparam BAND3_INC_OFFSET =
    BAND3_OFFSET + HYST_OFFSET < 4096 ? BAND3_OFFSET + HYST_OFFSET : 4096 ;
  localparam BAND3_DEC_OFFSET =
    BAND4_OFFSET - HYST_OFFSET > 0    ? BAND4_OFFSET - HYST_OFFSET : 0    ;
  localparam BAND4_INC_OFFSET =
    BAND4_OFFSET + HYST_OFFSET < 4096 ? BAND4_OFFSET + HYST_OFFSET : 4096 ;

  // Temperature sampler FSM encoding
  localparam INIT   = 2'b00;
  localparam IDLE   = 2'b01;
  localparam UPDATE = 2'b10;
  localparam WAIT   = 2'b11;

  // Temperature sampler state
  reg [2:0]                       tempmon_state       = INIT;
  reg [2:0]                       tempmon_next_state  = INIT;

  // Temperature storage
  reg   [11:0]                    previous_temp     = 12'b0;

  // Temperature bands
  reg [2:0]                       target_band       = 3'b000;
  reg [2:0]                       current_band      = 3'b000;

  // Tap count and control
  reg                             pi_f_inc          = 1'b0;
  reg                             pi_f_dec          = 1'b0;
  reg                             sel_pi_incdec     = 1'b0;

  // Temperature and band comparisons
  reg                             device_temp_lt_previous_temp = 1'b0;
  reg                             device_temp_gt_previous_temp = 1'b0;

  reg                             device_temp_lt_band1 = 1'b0;
  reg                             device_temp_lt_band2 = 1'b0;
  reg                             device_temp_lt_band3 = 1'b0;
  reg                             device_temp_lt_band4 = 1'b0;

  reg                             device_temp_lt_band0_dec = 1'b0;
  reg                             device_temp_lt_band1_dec = 1'b0;
  reg                             device_temp_lt_band2_dec = 1'b0;
  reg                             device_temp_lt_band3_dec = 1'b0;

  reg                             device_temp_gt_band1_inc = 1'b0;
  reg                             device_temp_gt_band2_inc = 1'b0;
  reg                             device_temp_gt_band3_inc = 1'b0;
  reg                             device_temp_gt_band4_inc = 1'b0;

  reg                             current_band_lt_target_band = 1'b0;
  reg                             current_band_gt_target_band = 1'b0;

  reg                             target_band_gt_1 = 1'b0;
  reg                             target_band_gt_2 = 1'b0;
  reg                             target_band_gt_3 = 1'b0;

  reg                             target_band_lt_1 = 1'b0;
  reg                             target_band_lt_2 = 1'b0;
  reg                             target_band_lt_3 = 1'b0;

  // Pass tap control signals back up to PHY
  assign tempmon_pi_f_inc = pi_f_inc;
  assign tempmon_pi_f_dec = pi_f_dec;
  assign tempmon_sel_pi_incdec = sel_pi_incdec;

  // XADC sampler state transition
  always @(posedge clk)
    if(rst)
      tempmon_state <= #TCQ INIT;
    else
      tempmon_state <= #TCQ tempmon_next_state;

  // XADC sampler next state transition
  always @(tempmon_state or calib_complete or tempmon_sample_en) begin

    tempmon_next_state = tempmon_state;

    case(tempmon_state)

      INIT:
        if(calib_complete)
          tempmon_next_state = IDLE;

      IDLE:
        if(tempmon_sample_en)
          tempmon_next_state = UPDATE;

      UPDATE:
        tempmon_next_state = WAIT;

      WAIT:
        if(~tempmon_sample_en)
          tempmon_next_state = IDLE;

      default:
        tempmon_next_state = INIT;

    endcase

  end

  // Record previous temperature during update cycle
  always @(posedge clk)
    if((tempmon_state == INIT) || (tempmon_state == UPDATE))
      previous_temp <= #TCQ device_temp;

  // Update target band
  always @(posedge clk) begin

    // register temperature comparisons
    device_temp_lt_previous_temp <= #TCQ (device_temp < previous_temp) ? 1'b1 : 1'b0;
    device_temp_gt_previous_temp <= #TCQ (device_temp > previous_temp) ? 1'b1 : 1'b0;     

    device_temp_lt_band1 <= #TCQ (device_temp < BAND1_OFFSET) ? 1'b1 : 1'b0;
    device_temp_lt_band2 <= #TCQ (device_temp < BAND2_OFFSET) ? 1'b1 : 1'b0;
    device_temp_lt_band3 <= #TCQ (device_temp < BAND3_OFFSET) ? 1'b1 : 1'b0;
    device_temp_lt_band4 <= #TCQ (device_temp < BAND4_OFFSET) ? 1'b1 : 1'b0;

    device_temp_lt_band0_dec <= #TCQ (device_temp < BAND0_DEC_OFFSET) ? 1'b1 : 1'b0;
    device_temp_lt_band1_dec <= #TCQ (device_temp < BAND1_DEC_OFFSET) ? 1'b1 : 1'b0;
    device_temp_lt_band2_dec <= #TCQ (device_temp < BAND2_DEC_OFFSET) ? 1'b1 : 1'b0;
    device_temp_lt_band3_dec <= #TCQ (device_temp < BAND3_DEC_OFFSET) ? 1'b1 : 1'b0;

    device_temp_gt_band1_inc <= #TCQ (device_temp > BAND1_INC_OFFSET) ? 1'b1 : 1'b0;
    device_temp_gt_band2_inc <= #TCQ (device_temp > BAND2_INC_OFFSET) ? 1'b1 : 1'b0;
    device_temp_gt_band3_inc <= #TCQ (device_temp > BAND3_INC_OFFSET) ? 1'b1 : 1'b0;
    device_temp_gt_band4_inc <= #TCQ (device_temp > BAND4_INC_OFFSET) ? 1'b1 : 1'b0;

    target_band_gt_1 <= #TCQ (target_band > 3'b001) ? 1'b1 : 1'b0;
    target_band_gt_2 <= #TCQ (target_band > 3'b010) ? 1'b1 : 1'b0;
    target_band_gt_3 <= #TCQ (target_band > 3'b011) ? 1'b1 : 1'b0;

    target_band_lt_1 <= #TCQ (target_band < 3'b001) ? 1'b1 : 1'b0;
    target_band_lt_2 <= #TCQ (target_band < 3'b010) ? 1'b1 : 1'b0;
    target_band_lt_3 <= #TCQ (target_band < 3'b011) ? 1'b1 : 1'b0;

    // Initialize band
    if(tempmon_state == INIT) begin

      if(device_temp_lt_band1)
        target_band <= #TCQ 3'b000;
      else if(device_temp_lt_band2)
        target_band <= #TCQ 3'b001;
      else if(device_temp_lt_band3)
        target_band <= #TCQ 3'b010;
      else if(device_temp_lt_band4)
        target_band <= #TCQ 3'b011;
      else
        target_band <= #TCQ 3'b100;

    end

    // Ready to update
    else if(tempmon_state == IDLE) begin

      // Temperature has increased, see if it is in a new band
      if(device_temp_gt_previous_temp) begin

        if(device_temp_gt_band4_inc)
          target_band <= #TCQ 3'b100;

        else if(device_temp_gt_band3_inc && target_band_lt_3)
          target_band <= #TCQ 3'b011;
 
        else if(device_temp_gt_band2_inc && target_band_lt_2)
          target_band <= #TCQ 3'b010;

        else if(device_temp_gt_band1_inc && target_band_lt_1)
          target_band <= #TCQ 3'b001;

      end

      // Temperature has decreased, see if it is in new band
      else if(device_temp_lt_previous_temp) begin

        if(device_temp_lt_band0_dec)
          target_band <= #TCQ 3'b000;

        else if(device_temp_lt_band1_dec && target_band_gt_1)
          target_band <= #TCQ 3'b001;

        else if(device_temp_lt_band2_dec && target_band_gt_2)
          target_band <= #TCQ 3'b010;

        else if(device_temp_lt_band3_dec && target_band_gt_3)
          target_band <= #TCQ 3'b011;

      end

    end

  end

  // Current band
  always @(posedge clk) begin

    current_band_lt_target_band = (current_band < target_band) ? 1'b1 : 1'b0;
    current_band_gt_target_band = (current_band > target_band) ? 1'b1 : 1'b0;     

    if(tempmon_state == INIT) begin

      if(device_temp_lt_band1)
        current_band <= #TCQ 3'b000;
      else if(device_temp_lt_band2)
        current_band <= #TCQ 3'b001;
      else if(device_temp_lt_band3)
        current_band <= #TCQ 3'b010;
      else if(device_temp_lt_band4)
        current_band <= #TCQ 3'b011;
      else
        current_band <= #TCQ 3'b100;

    end

    else if(tempmon_state == UPDATE) begin

      if(current_band_lt_target_band)
        current_band <= #TCQ current_band + 1;
      else if(current_band_gt_target_band)
        current_band <= #TCQ current_band - 1;

    end

  end

  // Tap control
  always @(posedge clk) begin

    if(rst) begin
      pi_f_inc <= #TCQ 1'b0;
      pi_f_dec <= #TCQ 1'b0;
      sel_pi_incdec <= #TCQ 1'b0;
    end

    else if(tempmon_state == UPDATE) begin

      if(current_band_lt_target_band) begin
        sel_pi_incdec <= #TCQ 1'b1;
        pi_f_dec <= #TCQ 1'b1;
      end

      else if(current_band_gt_target_band) begin
        sel_pi_incdec <= #TCQ 1'b1;
        pi_f_inc <= #TCQ 1'b1;
      end

    end

    else begin

      pi_f_inc <= #TCQ 1'b0;
      pi_f_dec <= #TCQ 1'b0;
      sel_pi_incdec <= #TCQ 1'b0;

    end

  end

endmodule
