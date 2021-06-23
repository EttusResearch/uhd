//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfdc_timing_control
//
// Description:
//
//   This module handles timed register writes for the RFDC, such as NCO reset
//   control. Timed commands are handled by the ctrlport_timer module.
//
// Parameters:
//
//   NUM_DBOARDS : Number of daughter boards to support
//

`default_nettype none


module rfdc_timing_control #(
  parameter NUM_DBOARDS = 2
) (
  // Clocks and resets
  input wire clk,
  input wire rst,

  // Time
  input wire [63:0] time_now,
  input wire        time_now_stb,
  input wire [ 3:0] time_ignore_bits,

  // CtrlPort Slave (from RFNoC Radio Block)
  input  wire [  1*NUM_DBOARDS-1:0] s_ctrlport_req_wr,
  input  wire [  1*NUM_DBOARDS-1:0] s_ctrlport_req_rd,
  input  wire [ 20*NUM_DBOARDS-1:0] s_ctrlport_req_addr,
  input  wire [ 32*NUM_DBOARDS-1:0] s_ctrlport_req_data,
  input  wire [  4*NUM_DBOARDS-1:0] s_ctrlport_req_byte_en,
  input  wire [  1*NUM_DBOARDS-1:0] s_ctrlport_req_has_time,
  input  wire [ 64*NUM_DBOARDS-1:0] s_ctrlport_req_time,
  output wire [  1*NUM_DBOARDS-1:0] s_ctrlport_resp_ack,
  output wire [  2*NUM_DBOARDS-1:0] s_ctrlport_resp_status,
  output wire [ 32*NUM_DBOARDS-1:0] s_ctrlport_resp_data,

  // RF Reset Control
  output reg  start_nco_reset,
  input  wire nco_reset_done,
  output reg  adc_reset_pulse,
  output reg  dac_reset_pulse
);

  `include "regmap/rfdc_timing_regmap_utils.vh"

  // Reset registers
  reg [NUM_DBOARDS-1:0] reg_nco_reset_start = 0;
  reg [NUM_DBOARDS-1:0] reg_adc_reset_pulse = 0;
  reg [NUM_DBOARDS-1:0] reg_dac_reset_pulse = 0;

  genvar db;
  generate
    for (db = 0; db < NUM_DBOARDS; db = db+1) begin : gen_db_ctrlport

      //-----------------------------------------------------------------------
      // RF Reset Control
      //-----------------------------------------------------------------------

      wire [  1-1:0] nco_ctrlport_req_wr;
      wire [  1-1:0] nco_ctrlport_req_rd;
      wire [ 20-1:0] nco_ctrlport_req_addr;
      wire [ 32-1:0] nco_ctrlport_req_data;
      reg  [  1-1:0] nco_ctrlport_resp_ack;
      reg  [ 32-1:0] nco_ctrlport_resp_data;

      ctrlport_timer #(
        .EXEC_LATE_CMDS (1)
      ) ctrlport_timer_nco (
        .clk                     (clk),
        .rst                     (rst),
        .time_now                (time_now),
        .time_now_stb            (time_now_stb),
        .time_ignore_bits        (time_ignore_bits),
        .s_ctrlport_req_wr       (s_ctrlport_req_wr       [ 1*db+: 1]),
        .s_ctrlport_req_rd       (s_ctrlport_req_rd       [ 1*db+: 1]),
        .s_ctrlport_req_addr     (s_ctrlport_req_addr     [20*db+:20]),
        .s_ctrlport_req_data     (s_ctrlport_req_data     [32*db+:32]),
        .s_ctrlport_req_byte_en  (s_ctrlport_req_byte_en  [ 4*db+: 4]),
        .s_ctrlport_req_has_time (s_ctrlport_req_has_time [ 1*db+: 1]),
        .s_ctrlport_req_time     (s_ctrlport_req_time     [64*db+:64]),
        .s_ctrlport_resp_ack     (s_ctrlport_resp_ack     [ 1*db+: 1]),
        .s_ctrlport_resp_status  (s_ctrlport_resp_status  [ 2*db+: 2]),
        .s_ctrlport_resp_data    (s_ctrlport_resp_data    [32*db+:32]),
        .m_ctrlport_req_wr       (nco_ctrlport_req_wr),
        .m_ctrlport_req_rd       (nco_ctrlport_req_rd),
        .m_ctrlport_req_addr     (nco_ctrlport_req_addr),
        .m_ctrlport_req_data     (nco_ctrlport_req_data),
        .m_ctrlport_req_byte_en  (),
        .m_ctrlport_resp_ack     (nco_ctrlport_resp_ack),
        .m_ctrlport_resp_status  (2'b0),
        .m_ctrlport_resp_data    (nco_ctrlport_resp_data)
      );

      always @(posedge clk) begin
        if (rst) begin
          nco_ctrlport_resp_ack   <= 0;
          reg_nco_reset_start[db] <= 0;
          nco_ctrlport_resp_data  <= 'bX;
        end else begin
          // Default assignments
          reg_nco_reset_start[db] <= 0;
          reg_adc_reset_pulse[db] <= 0;
          reg_dac_reset_pulse[db] <= 0;
          nco_ctrlport_resp_ack   <= 0;
          nco_ctrlport_resp_data  <= 0;

          // Handle register reads
          if (nco_ctrlport_req_rd) begin
            case (nco_ctrlport_req_addr)
              NCO_RESET_REG: begin
                nco_ctrlport_resp_ack <= 1;
                nco_ctrlport_resp_data[NCO_RESET_DONE] <= nco_reset_done;
              end
              GEARBOX_RESET_REG: begin
                nco_ctrlport_resp_ack <= 1;
              end
            endcase
          end

          // Handle register writes
          if (nco_ctrlport_req_wr) begin
            case (nco_ctrlport_req_addr)
              NCO_RESET_REG: begin
                nco_ctrlport_resp_ack   <= 1;
                reg_nco_reset_start[db] <= nco_ctrlport_req_data[NCO_RESET_START];
              end
              GEARBOX_RESET_REG: begin
                nco_ctrlport_resp_ack   <= 1;
                reg_adc_reset_pulse[db] <= nco_ctrlport_req_data[ADC_RESET];
                reg_dac_reset_pulse[db] <= nco_ctrlport_req_data[DAC_RESET];
              end
            endcase
          end
        end
      end

    end
  endgenerate


  //---------------------------------------------------------------------------
  // Merge Resets
  //---------------------------------------------------------------------------
  //
  // There are multiple DBs but only one reset signal for each RF component.
  // Since the reset is simply a single cycle pulse, we OR the reset registers
  // for each daughter board together.
  //
  //---------------------------------------------------------------------------

  always @(posedge clk) begin
    if (rst) begin
      start_nco_reset <= 0;
      adc_reset_pulse <= 0;
      dac_reset_pulse <= 0;
    end else begin
      start_nco_reset <= |reg_nco_reset_start;
      adc_reset_pulse <= |reg_adc_reset_pulse;
      dac_reset_pulse <= |reg_dac_reset_pulse;
    end
  end

endmodule


`default_nettype wire


//XmlParse xml_on
//
//<regmap name="RFDC_TIMING_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="RFDC_TIMING_REGS">
//    <register name="NCO_RESET_REG" offset="0x00" size="32" readable="true" writable="true">
//      <info>NCO reset control register.</info>
//      <bitfield name="NCO_RESET_START" range="0" readable="false" strobe="true">
//        <info>Write a 1 to this bit to start a reset the RFDC's NCO.</info>
//      </bitfield>
//      <bitfield name="NCO_RESET_DONE" range="1" writable="false">
//        <info>When 1, indicates that the NCO reset has completed.</info>
//      </bitfield>
//    </register>
//    <register name="GEARBOX_RESET_REG" offset="0x04" size="32" readable="true" writable="true">
//      <info>Gearbox reset control register.</info>
//      <bitfield name="ADC_RESET" range="0" readable="false" strobe="true">
//        <info>
//          This reset is for the gearbox on the ADC data path that is used to
//          move data from one clock domain to another outside the RFDC. Write
//          a 1 to this bit to send a reset pulse to the ADC gearbox.
//        </info>
//      </bitfield>
//      <bitfield name="DAC_RESET" range="1" readable="false" strobe="true">
//        <info>
//          This reset is for the gearbox on the DAC data path that is used to
//          move data from one clock domain to another outside the RFDC. Write
//          a 1 to this bit to send a reset pulse to the DAC gearbox.
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//
//XmlParse xml_off
