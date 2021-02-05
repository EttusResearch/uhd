//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: atr_controller
//
// Description:
//   Controller of the ATR state configuration for the other register endpoints.
//

`default_nettype none

module atr_controller #(
  parameter [19:0] BASE_ADDRESS = 0,
  parameter [19:0] SIZE_ADDRESS = 0
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

  // ATR state of FPGA
  // Assumes the following assignment on the FPGA:
  // {tx_running[1], rx_running[1], tx_running[0], rx_running[0]}
  // where the array index indicates the RF chain.
  input  wire [ 3:0] atr_fpga_state,

  // derived configuration
  output reg  [ 7:0] atr_config_dsa_rf0 = 8'b0,
  output reg  [ 7:0] atr_config_dsa_rf1 = 8'b0,
  output reg  [ 7:0] atr_config_rf0     = 8'b0,
  output reg  [ 7:0] atr_config_rf1     = 8'b0
);

  `include "../regmap/atr_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //----------------------------------------------------------
  // Internal registers
  //----------------------------------------------------------
  reg [    RF0_OPTION_SIZE-1:0] option_rf0     = SW_DEFINED;
  reg [    RF1_OPTION_SIZE-1:0] option_rf1     = SW_DEFINED;
  reg [RF0_DSA_OPTION_SIZE-1:0] option_dsa_rf0 = SW_DEFINED;
  reg [RF1_DSA_OPTION_SIZE-1:0] option_dsa_rf1 = SW_DEFINED;

  reg [    SW_RF0_CONFIG_SIZE-1:0] sw_atr_config_rf0     = {SW_RF0_CONFIG_SIZE {1'b0}};
  reg [    SW_RF1_CONFIG_SIZE-1:0] sw_atr_config_rf1     = {SW_RF1_CONFIG_SIZE {1'b0}};
  reg [SW_RF0_DSA_CONFIG_SIZE-1:0] sw_atr_config_dsa_rf0 = {SW_RF0_DSA_CONFIG_SIZE {1'b0}};
  reg [SW_RF1_DSA_CONFIG_SIZE-1:0] sw_atr_config_dsa_rf1 = {SW_RF1_DSA_CONFIG_SIZE {1'b0}};

  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      option_rf0     <= SW_DEFINED;
      option_rf1     <= SW_DEFINED;
      option_dsa_rf0 <= SW_DEFINED;
      option_dsa_rf1 <= SW_DEFINED;

      sw_atr_config_rf0     <= {SW_RF0_CONFIG_SIZE {1'b0}};
      sw_atr_config_rf1     <= {SW_RF1_CONFIG_SIZE {1'b0}};
      sw_atr_config_dsa_rf0 <= {SW_RF0_DSA_CONFIG_SIZE {1'b0}};
      sw_atr_config_dsa_rf1 <= {SW_RF1_DSA_CONFIG_SIZE {1'b0}};

      s_ctrlport_resp_ack     <= 1'b0;
      s_ctrlport_resp_data    <= {32{1'bx}};
      s_ctrlport_resp_status  <= CTRL_STS_OKAY;

    end else begin

      // write requests
      if (s_ctrlport_req_wr) begin
        // always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + OPTION_REG: begin
            option_rf0     <= s_ctrlport_req_data[RF0_OPTION_MSB : RF0_OPTION];
            option_rf1     <= s_ctrlport_req_data[RF1_OPTION_MSB : RF1_OPTION];
            option_dsa_rf0 <= s_ctrlport_req_data[RF0_DSA_OPTION_MSB : RF0_DSA_OPTION];
            option_dsa_rf1 <= s_ctrlport_req_data[RF1_DSA_OPTION_MSB : RF1_DSA_OPTION];
          end

          BASE_ADDRESS + SW_CONFIG_REG: begin
            sw_atr_config_rf0     <= s_ctrlport_req_data[SW_RF0_CONFIG_MSB : SW_RF0_CONFIG];
            sw_atr_config_rf1     <= s_ctrlport_req_data[SW_RF1_CONFIG_MSB : SW_RF1_CONFIG];
            sw_atr_config_dsa_rf0 <= s_ctrlport_req_data[SW_RF0_DSA_CONFIG_MSB : SW_RF0_DSA_CONFIG];
            sw_atr_config_dsa_rf1 <= s_ctrlport_req_data[SW_RF1_DSA_CONFIG_MSB : SW_RF1_DSA_CONFIG];
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
          BASE_ADDRESS + CURRENT_CONFIG_REG: begin
            s_ctrlport_resp_data[CURRENT_RF0_CONFIG_MSB     : CURRENT_RF0_CONFIG]     <= atr_config_rf0;
            s_ctrlport_resp_data[CURRENT_RF1_CONFIG_MSB     : CURRENT_RF1_CONFIG]     <= atr_config_rf1;
            s_ctrlport_resp_data[CURRENT_RF0_DSA_CONFIG_MSB : CURRENT_RF0_DSA_CONFIG] <= atr_config_dsa_rf0;
            s_ctrlport_resp_data[CURRENT_RF1_DSA_CONFIG_MSB : CURRENT_RF1_DSA_CONFIG] <= atr_config_dsa_rf1;
          end

          BASE_ADDRESS + OPTION_REG: begin
            s_ctrlport_resp_data[RF0_OPTION_MSB     : RF0_OPTION]     <= option_rf0;
            s_ctrlport_resp_data[RF1_OPTION_MSB     : RF1_OPTION]     <= option_rf1;
            s_ctrlport_resp_data[RF0_DSA_OPTION_MSB : RF0_DSA_OPTION] <= option_dsa_rf0;
            s_ctrlport_resp_data[RF1_DSA_OPTION_MSB : RF1_DSA_OPTION] <= option_dsa_rf1;
          end

          BASE_ADDRESS + SW_CONFIG_REG: begin
            s_ctrlport_resp_data[SW_RF0_CONFIG_MSB     : SW_RF0_CONFIG]     <= sw_atr_config_rf0;
            s_ctrlport_resp_data[SW_RF1_CONFIG_MSB     : SW_RF1_CONFIG]     <= sw_atr_config_rf1;
            s_ctrlport_resp_data[SW_RF0_DSA_CONFIG_MSB : SW_RF0_DSA_CONFIG] <= sw_atr_config_dsa_rf0;
            s_ctrlport_resp_data[SW_RF1_DSA_CONFIG_MSB : SW_RF1_DSA_CONFIG] <= sw_atr_config_dsa_rf1;
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

  //----------------------------------------------------------
  // derive configuration
  //----------------------------------------------------------
  always @(posedge ctrlport_clk) begin
    case (option_rf0)
      SW_DEFINED: begin
        atr_config_rf0 <= sw_atr_config_rf0;
      end
      CLASSIC_ATR: begin
        atr_config_rf0 <= {6'b0, atr_fpga_state[1:0]};
      end
      FPGA_STATE: begin
        atr_config_rf0 <= {4'b0, atr_fpga_state};
      end
    endcase

    case (option_rf1)
      SW_DEFINED: begin
        atr_config_rf1 <= sw_atr_config_rf1;
      end
      CLASSIC_ATR: begin
        atr_config_rf1 <= {6'b0, atr_fpga_state[3:2]};
      end
      FPGA_STATE: begin
        atr_config_rf1 <= {4'b0, atr_fpga_state};
      end
    endcase

    case (option_dsa_rf0)
      SW_DEFINED: begin
        atr_config_dsa_rf0 <= sw_atr_config_dsa_rf0;
      end
      CLASSIC_ATR: begin
        atr_config_dsa_rf0 <= {6'b0, atr_fpga_state[1:0]};
      end
      FPGA_STATE: begin
        atr_config_dsa_rf0 <= {4'b0, atr_fpga_state};
      end
    endcase

    case (option_dsa_rf1)
      SW_DEFINED: begin
        atr_config_dsa_rf1 <= sw_atr_config_dsa_rf1;
      end
      CLASSIC_ATR: begin
        atr_config_dsa_rf1 <= {6'b0, atr_fpga_state[3:2]};
      end
      FPGA_STATE: begin
        atr_config_dsa_rf1 <= {4'b0, atr_fpga_state};
      end
    endcase
  end

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="ATR_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
// <group name="ATR_REGISTERS">
//   <info>
//     This regmap contains settings for the active configuration of RF 0 and 1.
//     There are two sets of configurations. One set comprises RF switches and
//     LEDs, the other set comprises the attenuators (DSA).
//   </info>
//
//    <enumeratedtype name="ATR_OPTIONS">
//      <info>
//        Contains the options available for RF 0 and RF 1. The chosen setting
//        affects how the active configuration of up to 8 bits is derived.
//      </info>
//      <value name="SW_DEFINED" integer="0">
//        <info>
//          Uses the respective value of @.SW_CONFIG_REG as configuration.
//        </info>
//      </value>
//      <value name="CLASSIC_ATR" integer="1">
//        <info>
//          This option assumes the FPGA state to be assigned with: Bit 0 = RF 0
//          RX running, Bit 1 = RF 0 TX running, Bit 2 = RF 1 RX running, Bit 3
//          = RF 1 TX running. The configuration for each RF chain is built
//          up of the 2 bits for the RF chain (4 possible states: IDLE, RX only,
//          TX only, TX/RX).
//        </info>0
//      </value>
//      <value name="FPGA_STATE" integer="2">
//        <info>
//          The 4 bit wide ATR FPGA state is used as configuration. This enables 16 states.
//        </info>
//      </value>
//    </enumeratedtype>
//
//   <register name="CURRENT_CONFIG_REG" size="32" offset="0x00" attributes="Readable">
//     <info>
//       Contains the current active configuration.
//     </info>
//     <bitfield name="CURRENT_RF0_CONFIG" range="7..0" type="integer">
//       <info>
//          Current active configuration for switches and LEDs of RF 0.
//       </info>
//     </bitfield>
//     <bitfield name="CURRENT_RF1_CONFIG" range="15..8" type="integer">
//       <info>
//          Current active configuration for switches and LEDs of RF 1.
//       </info>
//     </bitfield>
//     <bitfield name="CURRENT_RF0_DSA_CONFIG" range="23..16" type="integer">
//       <info>
//          Current active configuration for DSAs of RF 0.
//       </info>
//     </bitfield>
//     <bitfield name="CURRENT_RF1_DSA_CONFIG" range="31..24" type="integer">
//       <info>
//          Current active configuration for DSAs of RF 1.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="OPTION_REG" size="32" offset="0x04" attributes="Readable|Writable">
//     <info>
//       Set the option to be used for the RF chains.
//     </info>
//     <bitfield name="RF0_OPTION" range="1..0" type="ATR_OPTIONS" initialvalue="SW_DEFINED">
//       <info>
//         Option used for switches and LEDs of RF 0.
//       </info>
//     </bitfield>
//     <bitfield name="RF1_OPTION" range="9..8" type="ATR_OPTIONS" initialvalue="SW_DEFINED">
//       <info>
//         Option used for switches and LEDs of RF 1.
//       </info>
//     </bitfield>
//     <bitfield name="RF0_DSA_OPTION" range="17..16" type="ATR_OPTIONS" initialvalue="SW_DEFINED">
//       <info>
//         Option used for DSAs of RF 0.
//       </info>
//     </bitfield>
//     <bitfield name="RF1_DSA_OPTION" range="25..24" type="ATR_OPTIONS" initialvalue="SW_DEFINED">
//       <info>
//         Option used for DSAs of RF 1.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="SW_CONFIG_REG" size="32" offset="0x08" attributes="Readable|Writable">
//     <info>
//       Contains the configuration to be applied in case SW_DEFINED option is
//       chosen.
//     </info>
//     <bitfield name="SW_RF0_CONFIG" range="7..0" type="integer" initialvalue="0">
//       <info>
//          SW defined configuration for switches and LEDs of RF 0.
//       </info>
//     </bitfield>
//     <bitfield name="SW_RF1_CONFIG" range="15..8" type="integer" initialvalue="0">
//       <info>
//          SW defined configuration for switches and LEDs of RF 1.
//       </info>
//     </bitfield>
//     <bitfield name="SW_RF0_DSA_CONFIG" range="23..16" type="integer" initialvalue="0">
//       <info>
//          SW defined configuration for DSAs of RF 0.
//       </info>
//     </bitfield>
//     <bitfield name="SW_RF1_DSA_CONFIG" range="31..24" type="integer" initialvalue="0">
//       <info>
//          SW defined configuration for DSAs of RF 1.
//       </info>
//     </bitfield>
//   </register>
//  </group>
//</regmap>
//XmlParse xml_off
