//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: power_regs
//
// Description:
//   Registers to control the power supplies and the PLL ref clock buffer.
//

`default_nettype none

module power_regs #(
  parameter [19:0]  BASE_ADDRESS = 0,
  parameter [19:0]  SIZE_ADDRESS = 0
) (
  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  // Response
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  //reg clk domain
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  //Power Control
  output wire        enable_tx_7v0,
  output wire        enable_rx_7v0,
  output wire        enable_3v3,

  //Power Good Indicators
  input  wire        p7v_pg_b,
  input  wire        p7v_pg_a,

  //PRC buffer
  output reg         pll_ref_clk_enable = 1'b0
);

  `include "../regmap/power_regs_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //----------------------------------------------------------
  // Internal registers
  //----------------------------------------------------------
  reg    enable_3v3_reg = {ENABLE_3V3_SIZE    {1'b0}};
  reg enable_rx_7v0_reg = {ENABLE_RX_7V0_SIZE {1'b0}};
  reg enable_tx_7v0_reg = {ENABLE_TX_7V0_SIZE {1'b0}};

  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack     <= 1'b0;
      s_ctrlport_resp_data    <= {32{1'bx}};
      s_ctrlport_resp_status  <= CTRL_STS_OKAY;

      enable_3v3_reg          <= {ENABLE_3V3_SIZE    {1'b0}};
      enable_rx_7v0_reg       <= {ENABLE_RX_7V0_SIZE {1'b0}};
      enable_tx_7v0_reg       <= {ENABLE_TX_7V0_SIZE {1'b0}};

    end else begin

      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + RF_POWER_CONTROL: begin
            enable_3v3_reg    <= s_ctrlport_req_data[ENABLE_3V3];
            enable_rx_7v0_reg <= s_ctrlport_req_data[ENABLE_RX_7V0];
            enable_tx_7v0_reg <= s_ctrlport_req_data[ENABLE_TX_7V0];
          end

          BASE_ADDRESS + PRC_CONTROL: begin
            pll_ref_clk_enable <= s_ctrlport_req_data[PLL_REF_CLOCK_ENABLE];
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status  <= CTRL_STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport_resp_ack     <= 1'b0;
            end
          end
        endcase

      // read requests
      end else if (s_ctrlport_req_rd) begin
        // default assumption: valid request
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;
        s_ctrlport_resp_data    <= {32{1'b0}};

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + RF_POWER_CONTROL: begin
            s_ctrlport_resp_data[ENABLE_3V3]    <= enable_3v3_reg;
            s_ctrlport_resp_data[ENABLE_RX_7V0] <= enable_rx_7v0_reg;
            s_ctrlport_resp_data[ENABLE_TX_7V0] <= enable_tx_7v0_reg;
          end

          BASE_ADDRESS + RF_POWER_STATUS: begin
            s_ctrlport_resp_data[P7V_A_STATUS] <= p7v_pg_a;
            s_ctrlport_resp_data[P7V_B_STATUS] <= p7v_pg_b;
          end

          BASE_ADDRESS + PRC_CONTROL: begin
            s_ctrlport_resp_data[PLL_REF_CLOCK_ENABLE] <= pll_ref_clk_enable;
          end

          // error on undefined address
          default: begin
            s_ctrlport_resp_data <= {32{1'b0}};
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

  assign enable_tx_7v0 = enable_tx_7v0_reg;
  assign enable_rx_7v0 = enable_rx_7v0_reg;
  assign enable_3v3    = enable_3v3_reg;

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="POWER_REGS_REGMAP" generatevhdl="true" ettusguidelines="true">
// <group name="POWER_REGS_REGISTERS" size="0x010">
//   <info>
//     This regmap contains the registers to control the power supplies and the clock buffer for PLL reference clock.
//   </info>
//   <register name="RF_POWER_CONTROL" size="32" offset="0x00" attributes="Readable|Writable">
//     <info>
//       This register controls power supply enables to the Tx/Rx amps, switch control, and clk buffers. During normal
//       operations, all three power supplies should be enabled.
//     </info>
//     <bitfield name="ENABLE_3v3" range="2" initialvalue="0">
//       <info>
//         This power supply sources the switch control, and the clock buffers. By default this power supply is off.
//         The internal LOs will not work unless this bit is enabled.{BR/}
//       </info>
//     </bitfield>
//     <bitfield name="ENABLE_RX_7V0" range="1" initialvalue="0">
//       <info>
//         This power supply sources the Rx0 and Rx1 amps. By default this power supply is off.The Rx0/1 path will not
//         be active unless this power supply is enabled. Disabling this bit is similar to RX RF blanking{BR/}
//         {font color="red"}note to digital engineer, this is Pos7v0B{/font}
//       </info>
//     </bitfield>
//     <bitfield name="ENABLE_TX_7V0" range="0" initialvalue="0">
//       <info>
//         This power supply sources the Tx0 and Tx1 amps. By default this power supply is off. The Tx0/1 path will not
//         be active unless this power supply is enabled. Disabling this bit is similar to TX RF blanking{BR/}
//         {font color="red"}note to digital engineer, this is Pos7v0A{/font}
//       </info>
//     </bitfield>
//   </register>
//   <register name="RF_POWER_STATUS" size="32" offset="0x04" attributes="Readable">
//     <info>
//       Returns status of PowerGood indicators across the daughterboard.
//     </info>
//     <bitfield name="P7V_A_STATUS" range="0">
//       <info>
//         Returns status of 7V switching regulator A.{BR/}
//       </info>
//     </bitfield>
//     <bitfield name="P7V_B_STATUS" range="1">
//       <info>
//         Returns status of 7V switching regulator B.{BR/}
//       </info>
//     </bitfield>
//   </register>
//   <register name="PRC_CONTROL" size="32" offset="0x08" attributes="Readable|Writable">
//     <info>
//       Offers ability to enable or disable the PLL reference clock.
//     </info>
//      <bitfield name="PLL_REF_CLOCK_ENABLE" range="0" initialvalue="0">
//        <info>If set PLL reference clock is enabled.</info>
//      </bitfield>
//   </register>
//  </group>
//</regmap>
//XmlParse xml_off
