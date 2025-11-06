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
  input  wire [  1*NUM_DBOARDS-1:0]   s_ctrlport_req_wr,
  input  wire [  1*NUM_DBOARDS-1:0]   s_ctrlport_req_rd,
  input  wire [ 20*NUM_DBOARDS-1:0]   s_ctrlport_req_addr,
  input  wire [ 32*NUM_DBOARDS-1:0]   s_ctrlport_req_data,
  output wire [  1*NUM_DBOARDS-1:0]   s_ctrlport_resp_ack,
  output wire [  2*NUM_DBOARDS-1:0]   s_ctrlport_resp_status,
  output wire [ 32*NUM_DBOARDS-1:0]   s_ctrlport_resp_data,

  // RF Reset Control
  output wire                         start_nco_reset,
  input  wire                         nco_reset_done,
  input  wire                         noc_reset_sync_failed,
  output reg  [ 7:0]                  sysref_wait_cycles = 8'b0,

  output reg  [  1*NUM_DBOARDS-1:0]   adc_reset_pulse,
  output reg  [  1*NUM_DBOARDS-1:0]   dac_reset_pulse
);

  `include "regmap/rfdc_timing_regmap_utils.vh"

  // Reset registers
  reg [NUM_DBOARDS-1:0] reg_nco_reset_start = {NUM_DBOARDS {1'b0}};

  // Controlports in DB0 clk domain
  wire        dbx_ctrlport_req_wr [NUM_DBOARDS-1:0];
  wire        dbx_ctrlport_req_rd [NUM_DBOARDS-1:0];
  wire [19:0] dbx_ctrlport_req_addr [NUM_DBOARDS-1:0];
  wire [31:0] dbx_ctrlport_req_data [NUM_DBOARDS-1:0];
  reg         dbx_ctrlport_resp_ack [NUM_DBOARDS-1:0];
  reg  [1:0]  dbx_ctrlport_resp_status [NUM_DBOARDS-1:0];
  reg  [31:0] dbx_ctrlport_resp_data [NUM_DBOARDS-1:0];

  genvar db;
  generate
    for (db = 0; db < NUM_DBOARDS; db = db+1) begin : gen_db_ctrlport

    // Transfer all controlport signals to DB0 clk domain
    // Also do this for DB0 to ensure that the signals are time aligned
    if(db == 0) begin
      assign dbx_ctrlport_req_wr[0]   = s_ctrlport_req_wr   [ 1*db+: 1];
      assign dbx_ctrlport_req_rd[0]   = s_ctrlport_req_rd   [ 1*db+: 1];
      assign dbx_ctrlport_req_addr[0] = s_ctrlport_req_addr [20*db+:20];
      assign dbx_ctrlport_req_data[0] = s_ctrlport_req_data [32*db+:32];
      assign s_ctrlport_resp_ack [ 1*db+: 1]    = dbx_ctrlport_resp_ack[0];
      assign s_ctrlport_resp_status [ 2*db+: 2] = dbx_ctrlport_resp_status[0];
      assign s_ctrlport_resp_data [32*db+:32]   = dbx_ctrlport_resp_data[0];
    end

    else begin
      ctrlport_clk_cross ctrlport_clk_cross_i (
        .rst(rst[db]),
        // Inputs
        .s_ctrlport_clk(clk[db]),
        .s_ctrlport_req_wr  (s_ctrlport_req_wr   [ 1*db+: 1]),
        .s_ctrlport_req_rd  (s_ctrlport_req_rd   [ 1*db+: 1]),
        .s_ctrlport_req_addr(s_ctrlport_req_addr [20*db+:20]),
        .s_ctrlport_req_data(s_ctrlport_req_data [32*db+:32]),
        .s_ctrlport_resp_ack(s_ctrlport_resp_ack [ 1*db+: 1]),
        .s_ctrlport_resp_status(s_ctrlport_resp_status [ 2*db+: 2]),
        .s_ctrlport_resp_data(s_ctrlport_resp_data [32*db+:32]),
        // Outputs
        .m_ctrlport_clk(clk[0]),
        .m_ctrlport_req_wr  (dbx_ctrlport_req_wr[db]),
        .m_ctrlport_req_rd  (dbx_ctrlport_req_rd[db]),
        .m_ctrlport_req_addr(dbx_ctrlport_req_addr[db]),
        .m_ctrlport_req_data(dbx_ctrlport_req_data[db]),
        .m_ctrlport_resp_ack(dbx_ctrlport_resp_ack[db]),
        .m_ctrlport_resp_status(dbx_ctrlport_resp_status[db]),
        .m_ctrlport_resp_data(dbx_ctrlport_resp_data[db])
      );
    end

      //-----------------------------------------------------------------------
      // RF Reset Control
      //-----------------------------------------------------------------------

      always_ff @(posedge clk[0]) begin
        dbx_ctrlport_resp_status[db] <= 2'b00;
        if (rst[0]) begin
          dbx_ctrlport_resp_ack[db]  <= 1'b0;
          dbx_ctrlport_resp_data[db] <= 32'bX;
          reg_nco_reset_start[db]    <= 1'b0;
        end else begin
          // Default assignments
          reg_nco_reset_start[db]    <= 1'b0;
          adc_reset_pulse[db]        <= 1'b0;
          dac_reset_pulse[db]        <= 1'b0;
          dbx_ctrlport_resp_ack[db]  <= 1'b0;
          dbx_ctrlport_resp_data[db] <= 32'b0;

          // Handle register reads
          if (dbx_ctrlport_req_rd[db]) begin
            case (dbx_ctrlport_req_addr[db])
              NCO_RESET_REG: begin
                dbx_ctrlport_resp_ack[db] <= 1'b1;
                dbx_ctrlport_resp_data[db][NCO_SYNC_FAILED] <= noc_reset_sync_failed;
                dbx_ctrlport_resp_data[db][SYSREF_WAIT_MSB:SYSREF_WAIT] <= sysref_wait_cycles;
                dbx_ctrlport_resp_data[db][NCO_RESET_DONE] <= nco_reset_done;
              end
              GEARBOX_RESET_REG: begin
                dbx_ctrlport_resp_ack[db] <= 1'b1;
              end
            endcase
          end

          // Handle register writes
          if (dbx_ctrlport_req_wr[db]) begin
            case (dbx_ctrlport_req_addr[db])
              NCO_RESET_REG: begin
                dbx_ctrlport_resp_ack[db]   <= 1'b1;
                reg_nco_reset_start[db] <= dbx_ctrlport_req_data[db][NCO_RESET_START];
                // sysref_wait_cycles is handled seperately below
              end
              GEARBOX_RESET_REG: begin
                dbx_ctrlport_resp_ack[db]   <= 1'b1;
                adc_reset_pulse[db]     <= dbx_ctrlport_req_data[db][ADC_RESET];
                dac_reset_pulse[db]     <= dbx_ctrlport_req_data[db][DAC_RESET];
              end
            endcase
          end
        end
      end
    end

  endgenerate

  // Only assign the value requested by the lowest DB if multiple DBs are requesting
  // to write to sysref_wait_cycles simultanously
  always_ff @(posedge clk[0]) begin
    for(int i = 0; i<NUM_DBOARDS; i++) begin : gen_sysref_wait
      if(dbx_ctrlport_req_wr[i] && (dbx_ctrlport_req_addr[i] == NCO_RESET_REG)
         && dbx_ctrlport_req_data[i][WRITE_SYSREF_WAIT]) begin
        sysref_wait_cycles <= dbx_ctrlport_req_data[i][SYSREF_WAIT_MSB:SYSREF_WAIT];
        break;
      end
    end
  end

  //---------------------------------------------------------------------------
  // Merge NCO Start
  //---------------------------------------------------------------------------
  //
  // There are multiple DBs but only one NCO start pulse, which is expected to
  // happen on the clk[0] domain. This pulse will be used to align NCO updates
  // to the next SYSREF pulse, which is related to all clk[db] domains.
  //
  //---------------------------------------------------------------------------

  pulse_synchronizer #(
    .MODE("POSEDGE"), .STAGES(2)
  ) pulse_sync_nco_start (
      // reg_nco_reset_start is entirely in the clk[0] domain
      .clk_a(clk[0]), .rst_a(rst[0]), .pulse_a(|reg_nco_reset_start), .busy_a(),
      .clk_b(clk[0]), .pulse_b(start_nco_reset)
  );

endmodule


`default_nettype wire


//XmlParse xml_on
//
//<regmap name="RFDC_TIMING_REGMAP" readablestrobes="false" generatevhdl="true" generatesv="false" ettusguidelines="true">
//  <group name="RFDC_TIMING_REGS">
//    <register name="NCO_RESET_REG" offset="0x00" size="32" readable="true" writable="true">
//      <info>NCO reset control register.</info>
//      <bitfield name="NCO_RESET_START" range="0" readable="false" strobe="true">
//        <info>Write a 1 to this bit to start the RFDC's NCO reset state machine.</info>
//      </bitfield>
//      <bitfield name="NCO_RESET_DONE" range="1" writable="false">
//        <info>When 1, indicates that the NCO reset has completed.</info>
//      </bitfield>
//      <bitfield name="NCO_SYNC_FAILED" range="8" writable="false" readable="true">
//        <info>When 1, indicates that the NCO reset failed to complete in the expected time.</info>
//      </bitfield>
//      <bitfield name="SYSREF_WAIT" range="23..16" writable="true" readable="true">
//        <info>Sets the number of sysref cycles to wait before resuming NCO operation after reset.
//              DB0 will take priority if more than one db is setting this value.</info>
//      </bitfield>
//      <bitfield name="WRITE_SYSREF_WAIT" range="24" writable="true" readable="false">
//        <info>Enables writing to the SYSREF_WAIT bitfield</info>
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
