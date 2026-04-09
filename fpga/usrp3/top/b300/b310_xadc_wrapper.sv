//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: b310_xadc_wrapper
//
// Description:
//
//  This module wraps the XADC IP core to provide periodic temperature monitoring of
//  the FPGA die and configuration of the over-temperature shutdown threshold.
//
//  This file heavily borrows the structure of the mig_7series_v4_2_tempmon example
//  design provided by Xilinx in their MIG 7 Series IP core.
//
// Parameters:
//
//  SHUTDOWN_TEMP_C: Over-temperature shutdown threshold (degrees C). When the FPGA
//                   temperature exceeds this value, the FPGA will initiate a
//                   shutdown sequence.
//

`default_nettype none

module b310_xadc_wrapper #(
  parameter int SHUTDOWN_TEMP_C    = 125    // OT shutdown temperature (degrees C)
) (
  input  wire         clk,   // Fabric/XADC clock (same domain)
  input  wire         rst,   // Synchronous reset
  output logic [11:0] device_temp
);

  localparam int XADC_WIDTH = 16;
  localparam int TEMP_WIDTH = 12;

  // INIT_53: Over-Temperature alarm trigger
  //   Xilinx transfer function: Temp(C) = (ADC_Code * 503.975 / 4096) - 273.15
  //   => ADC_Code = (SHUTDOWN_TEMP_C + 273.15) * 4096 / 503.975
  //   Register format: bits[15:4] = ADC_Code[11:0], bits[3:0] = 4'b0011 (per UG480)
  localparam logic [TEMP_WIDTH-1:0] OT_CODE      = (SHUTDOWN_TEMP_C + 273.15) * 4096.0 / 503.975;
  localparam logic [XADC_WIDTH-1:0] INIT_53_VAL  = {OT_CODE, 4'h3};

  // FSM state encoding
  typedef enum logic [3:0] {
    IDLE,
    REQUEST_TEMP_READ,
    WAIT_FOR_READ_COMPLETE,
    LATCH_TEMP
  } tempmon_state_t;

  tempmon_state_t tempmon_state;

  // XADC interface
  logic                   xadc_den;
  logic                   xadc_drdy;
  logic [XADC_WIDTH-1:0]  xadc_do;
  logic                   xadc_drdy_r;
  logic [XADC_WIDTH-1:0]  xadc_do_r;

  // Temperature storage
  logic [TEMP_WIDTH-1:0] temperature;

  // FSM
  always_ff @(posedge clk) begin
    if (rst) begin
      temperature         <= '0;
      xadc_den            <= 1'b0;
      tempmon_state       <= IDLE;
    end else begin

      case (tempmon_state)

        IDLE:
          tempmon_state <= REQUEST_TEMP_READ;

        REQUEST_TEMP_READ: begin
          xadc_den          <= 1'b1;
          tempmon_state     <= WAIT_FOR_READ_COMPLETE;
        end

        WAIT_FOR_READ_COMPLETE: begin
          // De-assert strobe signals.
          xadc_den          <= 1'b0;
          // Wait for DRDY to go high, indicating data is ready
          if (xadc_drdy_r) begin
            tempmon_state <= LATCH_TEMP;
          end
        end

        LATCH_TEMP: begin
          // Latch temperature and restart timer
          temperature       <= xadc_do_r[4 +: TEMP_WIDTH];
          tempmon_state     <= REQUEST_TEMP_READ;
        end

        default:
          tempmon_state <= IDLE;

      endcase
    end
  end

  // Register XADC outputs
  always_ff @(posedge clk)
    if (rst) begin
      xadc_drdy_r <= 1'b0;
      xadc_do_r   <= '0;
    end else begin
      xadc_drdy_r <= xadc_drdy;
      xadc_do_r   <= xadc_do;
    end

  assign device_temp = temperature;

  // XADC: Dual 12-Bit 1MSPS Analog-to-Digital Converter — 7 Series
  XADC #(
    // INIT_40 - INIT_42: XADC configuration registers
    .INIT_40(16'h1000),     // config reg 0
    .INIT_41(16'h21fe),     // config reg 1 - Enable OT alarm
    .INIT_42(16'h0800),     // config reg 2
    // INIT_48 - INIT_4F: Sequence registers
    .INIT_48(16'h0101),     // Sequencer channel selection
    .INIT_49(16'h0000),     // Sequencer channel selection
    .INIT_4A(16'h0000),     // Sequencer Average selection
    .INIT_4B(16'h0000),     // Sequencer Average selection
    .INIT_4C(16'h0000),     // Sequencer Bipolar selection
    .INIT_4D(16'h0000),     // Sequencer Bipolar selection
    .INIT_4E(16'h0000),     // Sequencer Acq time selection
    .INIT_4F(16'h0000),     // Sequencer Acq time selection
    // INIT_50 - INIT_58, INIT_5C: Alarm limit registers
    .INIT_50(16'hb5ed),     // Temp alarm trigger
    .INIT_51(16'h57e4),     // Vccint upper alarm limit
    .INIT_52(16'ha147),     // Vccaux upper alarm limit
    .INIT_53(INIT_53_VAL),  // Temp alarm OT upper (derived from SHUTDOWN_TEMP_C)
    .INIT_54(16'ha93a),     // Temp alarm reset
    .INIT_55(16'h52c6),     // Vccint lower alarm limit
    .INIT_56(16'h9555),     // Vccaux lower alarm limit
    .INIT_57(16'hb0de),     // Temp alarm OT reset - reactivate FPGA at 75C
    .INIT_58(16'h5999),     // VBRAM upper alarm limit
    .INIT_5C(16'h5111),     // VBRAM lower alarm limit
    .SIM_DEVICE("7SERIES")
  ) XADC_inst (
    .ALM(),
    .OT(),
    .DO(xadc_do),
    .DRDY(xadc_drdy),
    .BUSY(),
    .CHANNEL(),
    .EOC(),
    .EOS(),
    .JTAGBUSY(),
    .JTAGLOCKED(),
    .JTAGMODIFIED(),
    .MUXADDR(),
    .VAUXN(16'b0),
    .VAUXP(16'b0),
    .CONVST(1'b0),
    .CONVSTCLK(1'b0),
    .RESET(rst),
    .VN(1'b0),
    .VP(1'b0),
    .DADDR(7'h00),   // 0x00 = on-chip temperature sensor
    .DCLK(clk),
    .DEN(xadc_den),
    .DI({XADC_WIDTH{1'b0}}),
    .DWE(1'b0)
  );

endmodule: b310_xadc_wrapper

`default_nettype wire
