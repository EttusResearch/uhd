//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: hbx_sync_injection
//
// Description:
//   Control interface to enable the clock for the HBX sync injection
//

module hbx_sync_injection (
  // control port interface
  ctrlport_if.slave s_ctrlport,

  // Related clocking
  input logic base_ref_clk,
  input logic pps_brc,
  output logic clk_out
);

  import ctrlport_pkg::*;
  import XmlSvPkgHBX_SYNC_INJECTION_REGMAP::*;

  //----------------------------------------------------------
  // Internal registers
  //----------------------------------------------------------
  // using register type definition from imported package
  CLK_EN_CONTROL clock_control_reg = '0;

  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  always_ff @(posedge s_ctrlport.clk) begin
    // reset internal registers and responses
    if (s_ctrlport.rst) begin
      clock_control_reg       <= '0;
      s_ctrlport.resp.ack     <= '0;
      s_ctrlport.resp.data    <= 'x;
      s_ctrlport.resp.status  <= STS_OKAY;

    end else begin

      // default to no ack and no data
      s_ctrlport.resp.ack     <= '0;
      s_ctrlport.resp.data    <= 'x;
      s_ctrlport.resp.status  <= STS_OKAY;

      // Write requests always get acknowledged with STS_OKAY for writable addresses.
      // All other addresses will return STS_CMDERR.
      // Overwrite the status at the respective register case if another status is desired.
      if (s_ctrlport.req.wr) begin
        s_ctrlport.resp.ack <= '1;

        case (s_ctrlport.req.addr)
          kCLK_EN_CONTROL: begin
            clock_control_reg <= s_ctrlport.req.data;
          end

          default: begin
            s_ctrlport.resp.status <= STS_CMDERR;
          end
        endcase

      // Read requests always get acknowledged with STS_OKAY for readable addresses.
      // All other addresses will return STS_CMDERR.
      // Overwrite the status at the respective register case if another status is desired.
      end else if (s_ctrlport.req.rd) begin
        s_ctrlport.resp.ack <= '1;

        case (s_ctrlport.req.addr)
          kCLK_EN_CONTROL: begin
            // mask the register to only return the used bits
            s_ctrlport.resp.data <= clock_control_reg & kCLK_EN_CONTROLMask;
          end

          default: begin
            s_ctrlport.resp.status <= STS_CMDERR;
          end
        endcase
      end
    end
  end

  //----------------------------------------------------------
  // Synchronize control signal
  //----------------------------------------------------------
  logic clock_control_reg_brc;
  synchronizer #(
    .WIDTH             (1),
    .STAGES            (2),
    .INITIAL_VAL       ('0),
    .FALSE_PATH_TO_IN  (1)
  ) clock_en_sync_i (
    .clk  (base_ref_clk),
    .rst  (1'b0),
    .in   (clock_control_reg.CLK_EN),
    .out  (clock_control_reg_brc)
  );

  //----------------------------------------------------------
  // Generate clock output
  //----------------------------------------------------------
  logic pps_brc_delayed = '0;
  logic clk_out_reg = '0;
  always_ff @(posedge base_ref_clk) begin
    // delayed pps_brc signal to detect rising edge
    pps_brc_delayed <= pps_brc;

    // clock is disabled
    if (!clock_control_reg_brc) begin
      clk_out_reg <= '0;
    // reset on rising edge of pps_brc
    end else if (pps_brc && !pps_brc_delayed) begin
      clk_out_reg <= '0;
    // toggle clock otherwise
    end else begin
      clk_out_reg <= ~clk_out_reg;
    end
  end

  //IOB reg
  logic clk_out_iob_reg = '0;
  always_ff @(posedge base_ref_clk) begin
    clk_out_iob_reg <= clk_out_reg;
  end

  assign clk_out = clk_out_iob_reg;

endmodule

//XmlParse xml_on
//<regmap name="HBX_SYNC_INJECTION_REGMAP" readablestrobes="false">
// <group name="HBX_SYNC_INJECTION_REGISTERS">
//   <info>
//     Control of the HBX sync injection clock.
//   </info>
//    <register name="CLK_EN_CONTROL" size="32" offset="0x0" attributes="Readable|Writable">
//      <info>
//        This register configures clock Enable.
//      </info>
//      <bitfield name="CLK_EN" range="0" initialvalue="0">
//        <info>
//          Enables the Clock.
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off
