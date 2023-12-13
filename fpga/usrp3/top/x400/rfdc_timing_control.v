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
  input wire [  1*NUM_DBOARDS-1:0] clk,
  input wire [  1*NUM_DBOARDS-1:0] rst,

  // CtrlPort Slave (from RFNoC Radio Block)
  input  wire [  1*NUM_DBOARDS-1:0] s_ctrlport_req_wr,
  input  wire [  1*NUM_DBOARDS-1:0] s_ctrlport_req_rd,
  input  wire [ 20*NUM_DBOARDS-1:0] s_ctrlport_req_addr,
  input  wire [ 32*NUM_DBOARDS-1:0] s_ctrlport_req_data,
  output wire [  1*NUM_DBOARDS-1:0] s_ctrlport_resp_ack,
  output wire [  2*NUM_DBOARDS-1:0] s_ctrlport_resp_status,
  output wire [ 32*NUM_DBOARDS-1:0] s_ctrlport_resp_data,

  // RF Reset Control
  output reg                        start_nco_reset,
  input  wire                       nco_reset_done,
  output reg  [  1*NUM_DBOARDS-1:0] adc_reset_pulse,
  output reg  [  1*NUM_DBOARDS-1:0] dac_reset_pulse
);

  `include "regmap/rfdc_timing_regmap_utils.vh"

  // Reset registers
  reg [NUM_DBOARDS-1:0] reg_nco_reset_start = {NUM_DBOARDS {1'b0}};

  genvar db;
  generate
    for (db = 0; db < NUM_DBOARDS; db = db+1) begin : gen_db_ctrlport

      //-----------------------------------------------------------------------
      // CtrlPort Breakdown
      //-----------------------------------------------------------------------

      wire         nco_ctrlport_req_wr;
      wire         nco_ctrlport_req_rd;
      wire [ 19:0] nco_ctrlport_req_addr;
      wire [ 31:0] nco_ctrlport_req_data;
      reg          nco_ctrlport_resp_ack;
      reg  [ 31:0] nco_ctrlport_resp_data;
      wire [  1:0] nco_ctrlport_resp_status = 2'b0;

      wire         nco_reset_done_clk;

      // When asserted, nco_reset_done will stay high until the next start_nco_reset
      // pulse, which by software design will not happen until this synchronizer
      // propagates and the converted nco_reset_done_clk is read.
      synchronizer #(
        .STAGES      (2),
        .WIDTH       (1),
        .INITIAL_VAL (1'h0)
      ) synchronizer_nco_reset_done (
        .clk (clk[db]),
        .rst (rst[db]),
        .in  (nco_reset_done),
        .out (nco_reset_done_clk)
      );

      assign nco_ctrlport_req_wr    = s_ctrlport_req_wr   [ 1*db+: 1];
      assign nco_ctrlport_req_rd    = s_ctrlport_req_rd   [ 1*db+: 1];
      assign nco_ctrlport_req_addr  = s_ctrlport_req_addr [20*db+:20];
      assign nco_ctrlport_req_data  = s_ctrlport_req_data [32*db+:32];

      assign s_ctrlport_resp_ack     [ 1*db+: 1] = nco_ctrlport_resp_ack;
      assign s_ctrlport_resp_status  [ 2*db+: 2] = nco_ctrlport_resp_data;
      assign s_ctrlport_resp_data    [32*db+:32] = nco_ctrlport_resp_status;

      //-----------------------------------------------------------------------
      // RF Reset Control
      //-----------------------------------------------------------------------

      always @(posedge clk[db]) begin
        if (rst[db]) begin
          nco_ctrlport_resp_ack   <= 1'b0;
          reg_nco_reset_start[db] <= 1'b0;
          nco_ctrlport_resp_data  <= 32'bX;
        end else begin
          // Default assignments
          reg_nco_reset_start[db] <= 1'b0;
          adc_reset_pulse[db]     <= 1'b0;
          dac_reset_pulse[db]     <= 1'b0;
          nco_ctrlport_resp_ack   <= 1'b0;
          nco_ctrlport_resp_data  <= 32'b0;

          // Handle register reads
          if (nco_ctrlport_req_rd) begin
            case (nco_ctrlport_req_addr)
              NCO_RESET_REG: begin
                nco_ctrlport_resp_ack <= 1'b1;
                nco_ctrlport_resp_data[NCO_RESET_DONE] <= nco_reset_done_clk;
              end
              GEARBOX_RESET_REG: begin
                nco_ctrlport_resp_ack <= 1'b1;
              end
            endcase
          end

          // Handle register writes
          if (nco_ctrlport_req_wr) begin
            case (nco_ctrlport_req_addr)
              NCO_RESET_REG: begin
                nco_ctrlport_resp_ack   <= 1'b1;
                reg_nco_reset_start[db] <= nco_ctrlport_req_data[NCO_RESET_START];
              end
              GEARBOX_RESET_REG: begin
                nco_ctrlport_resp_ack   <= 1'b1;
                adc_reset_pulse[db]     <= nco_ctrlport_req_data[ADC_RESET];
                dac_reset_pulse[db]     <= nco_ctrlport_req_data[DAC_RESET];
              end
            endcase
          end
        end
      end

    end
  endgenerate


  //---------------------------------------------------------------------------
  // Merge NCO Start
  //---------------------------------------------------------------------------
  //
  // There are multiple DBs but only one NCO start pulse, which is expected to
  // happen on the clk[0] domain. This pulse will be used to align NCO updates
  // to the next SYSREF pulse, which is related to all clk[db] domains.
  //
  //---------------------------------------------------------------------------

  wire [NUM_DBOARDS-1:0] reg_nco_reset_start_clk0;

  genvar db_i;
  generate
    for (db_i = 0; db_i < NUM_DBOARDS; db_i = db_i+1) begin : gen_nco_start_sync

      pulse_synchronizer #(
        .MODE("POSEDGE"), .STAGES(2)
      ) pulse_sync_nco_start (
         .clk_a(clk[db_i]), .rst_a(rst[db_i]), .pulse_a(reg_nco_reset_start[db_i]), .busy_a(),
         .clk_b(clk[0]), .pulse_b(reg_nco_reset_start_clk0[db_i])
      );

    end
  endgenerate

  always @(posedge clk[0]) begin
    if (rst[0]) begin
      start_nco_reset <= 1'b0;
    end else begin
      start_nco_reset <= |reg_nco_reset_start_clk0;
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
