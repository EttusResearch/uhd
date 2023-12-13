//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: led_control
//
// Description:
//  Implements control over LED state via CtrlPort. The default state
//  has the LEDs disabled.
//

`default_nettype none

module led_control #(
  parameter BASE_ADDRESS = 0,
  parameter REGMAP_SIZE = 8'h1
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

  // LED Control (domain: ctrlport_clk)
  output reg         ch0_rx2_led,
  output reg         ch0_tx_led,
  output reg         ch0_rx_led,
  output reg         ch1_rx2_led,
  output reg         ch1_tx_led,
  output reg         ch1_rx_led,
  output reg         ch2_rx2_led,
  output reg         ch2_tx_led,
  output reg         ch2_rx_led,
  output reg         ch3_rx2_led,
  output reg         ch3_tx_led,
  output reg         ch3_rx_led

);

  `include "../regmap/x440/led_setup_regmap_utils.vh"
  `include "../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------
  // ATR memory signals
  //---------------------------------------------------------------
  reg  [31:0] led_ctrl_reg = 32'h0;

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

    end else begin
      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + LED_CONTROL: begin
            led_ctrl_reg <= s_ctrlport_req_data;
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
          BASE_ADDRESS : begin
            s_ctrlport_resp_data <= led_ctrl_reg & LED_CONTROL_MASK;
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

  always @(posedge ctrlport_clk) begin
    //outputs
    ch0_rx2_led <= led_ctrl_reg[CH0_RX2_LED_EN];
    ch0_tx_led  <= led_ctrl_reg[CH0_TRX1_LED_RED_EN];
    ch0_rx_led  <= led_ctrl_reg[CH0_TRX1_LED_GR_EN];

    ch1_rx2_led <= led_ctrl_reg[CH1_RX2_LED_EN];
    ch1_tx_led  <= led_ctrl_reg[CH1_TRX1_LED_RED_EN];
    ch1_rx_led  <= led_ctrl_reg[CH1_TRX1_LED_GR_EN];

    ch2_rx2_led <= led_ctrl_reg[CH2_RX2_LED_EN];
    ch2_tx_led  <= led_ctrl_reg[CH2_TRX1_LED_RED_EN];
    ch2_rx_led  <= led_ctrl_reg[CH2_TRX1_LED_GR_EN];

    ch3_rx2_led <= led_ctrl_reg[CH3_RX2_LED_EN];
    ch3_tx_led  <= led_ctrl_reg[CH3_TRX1_LED_RED_EN];
    ch3_rx_led  <= led_ctrl_reg[CH3_TRX1_LED_GR_EN];
  end

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="LED_SETUP_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="LED_SETUP_REGISTERS">
//    <info>
//      Contains registers that control the LEDs.
//    </info>
//    <register name="LED_CONTROL" size="32" offset="0x0" attributes="Readable|Writable">
//      <info>
//        This register configures RF Frontend LEDs.
//      </info>
//      <bitfield name="CH0_RX2_LED_EN" range="0" initialvalue="0">
//        <info>
//          Enables the Ch0 Rx2 Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH0_TRX1_LED_RED_EN" range="1" initialvalue="0">
//        <info>
//          Enables the Ch0 TRX (TX) Red LED
//        </info>
//      </bitfield>
//      <bitfield name="CH0_TRX1_LED_GR_EN" range="2" initialvalue="0">
//        <info>
//          Enables the Ch0 TRX (RX) Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH1_RX2_LED_EN" range="8" initialvalue="0">
//        <info>
//          Enables the Ch1 Rx2 Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH1_TRX1_LED_RED_EN" range="9" initialvalue="0">
//        <info>
//          Enables the Ch1 TRX (TX) Red LED
//        </info>
//      </bitfield>
//      <bitfield name="CH1_TRX1_LED_GR_EN" range="10" initialvalue="0">
//        <info>
//          Enables the Ch1 TRX (RX) Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH2_RX2_LED_EN" range="16" initialvalue="0">
//        <info>
//          Enables the Ch2 Rx2 Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH2_TRX1_LED_RED_EN" range="17" initialvalue="0">
//        <info>
//          Enables the Ch2 TRX (TX) Red LED
//        </info>
//      </bitfield>
//      <bitfield name="CH2_TRX1_LED_GR_EN" range="18" initialvalue="0">
//        <info>
//          Enables the Ch2 TRX (RX) Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH3_RX2_LED_EN" range="24" initialvalue="0">
//        <info>
//          Enables the Ch3 Rx2 Green LED
//        </info>
//      </bitfield>
//      <bitfield name="CH3_TRX1_LED_RED_EN" range="25" initialvalue="0">
//        <info>
//          Enables the Ch3 TRX (TX) Red LED
//        </info>
//      </bitfield>
//      <bitfield name="CH3_TRX1_LED_GR_EN" range="26" initialvalue="0">
//        <info>
//          Enables the Ch3 TRX (RX) Green LED
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off
