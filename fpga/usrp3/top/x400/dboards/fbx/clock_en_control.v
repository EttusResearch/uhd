//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: clock_en_control
//
// Description:
//  Implements control over clock enable. clk_in is always active.
//  The enable controls whether clk_out is active or not.
//
// Parameters:
//
//   REG_BASE : Base address to use for registers.
//   REG_SIZE : Register space size.
//

`default_nettype none

module clock_en_control #(
  parameter BASE_ADDRESS = 0,
  parameter REGMAP_SIZE = 8'h4
) (
  // Clock and reset
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  // Response
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  // Clock Enable (domain: ctrlport_clk)
  input wire         clk_in,
  output wire        clk_out
);

  `include "../../../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/clock_en_regmap_utils.vh"

  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg  [31:0] clk_ctrl_reg;

  //---------------------------------------------------------------
  // Handling of CtrlPort
  //---------------------------------------------------------------
  // Check of request address is targeted for this module.
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + REGMAP_SIZE);

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack <= 1'b0;
      s_ctrlport_resp_status <= 2'b0;
      s_ctrlport_resp_data <= 32'b0;
      clk_ctrl_reg <= 32'b0;

    end else begin
      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + CLK_EN_CONTROL: begin
            clk_ctrl_reg <= s_ctrlport_req_data;
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase

      //req_rd not delayed
      end else if (s_ctrlport_req_rd) begin
        // default assumption: valid request
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;
        s_ctrlport_resp_data    <= {32{1'b0}};

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + CLK_EN_CONTROL: begin
            s_ctrlport_resp_data <= clk_ctrl_reg & CLK_EN_CONTROL_MASK;
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase

      // no request
      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end

  //---------------------------------------------------------------
  // Clock Enable
  //---------------------------------------------------------------

  wire clk_en;

  synchronizer synchronizer_i (
    .clk(clk_in              ),
    .rst(1'b0                ),
    .in (clk_ctrl_reg[CLK_EN]),
    .out(clk_en              )
  );

  BUFGCE BUFGCE_i (
    .O (clk_out),
    .CE(clk_en ),
    .I (clk_in )
  );

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="CLK_EN_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="CLK_EN_REGISTERS">
//    <info>
//      Clock Enable Register
//    </info>
//    <register name="CLK_EN_CONTROL" size="32" offset="0x0" attributes="Readable|Writable">
//      <info>
//        This register configures Clock Enable.
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
