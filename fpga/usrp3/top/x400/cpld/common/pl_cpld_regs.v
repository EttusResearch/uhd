//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: pl_cpld_regs
//
// Description:
//
//   Basic Registers to inform software about version and capabilities.
//
// Parameters:
//
//   BASE_ADDRESS : Base address for CtrlPort registers
//

`default_nettype none


module pl_cpld_regs #(
  parameter BASE_ADDRESS = 0
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  // Response
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  // QSFP LEDs
  // Port 0
  output wire [ 3:0] qsfp0_led_active,
  output wire [ 3:0] qsfp0_led_link,
  // Port 1
  output wire [ 3:0] qsfp1_led_active,
  output wire [ 3:0] qsfp1_led_link,

  // iPass status
  output wire [ 1:0] ipass_cable_present
);

  `ifdef X410
  `include "../regmap/x410/constants_regmap_utils.vh"
  `else
  `include "../regmap/x440/constants_regmap_utils.vh"
  `endif
  `include "../regmap/pl_cpld_base_regmap_utils.vh"
  `include "../../../../lib/rfnoc/core/ctrlport.vh"

  //---------------------------------------------------------------------------
  // Address Calculation
  //---------------------------------------------------------------------------

  localparam NUM_ADDRESSES = 64;
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) &&
                          (s_ctrlport_req_addr < BASE_ADDRESS + NUM_ADDRESSES);

  //---------------------------------------------------------------------------
  // Internal Registers
  //---------------------------------------------------------------------------

  reg [SCRATCH_REGISTER_SIZE-1:0]  scratch_reg;
  reg [LED_REGISTER_SIZE-1:0]      led_reg;
  reg [CABLE_PRESENT_REG_SIZE-1:0] ipass_reg;

  //---------------------------------------------------------------------------
  // Assign Outputs
  //---------------------------------------------------------------------------

  assign qsfp0_led_active = led_reg[QSFP0_LED_ACTIVE+:QSFP0_LED_ACTIVE_SIZE];
  assign qsfp0_led_link   = led_reg[QSFP0_LED_LINK+:QSFP0_LED_LINK_SIZE];

  assign qsfp1_led_active = led_reg[QSFP1_LED_ACTIVE+:QSFP1_LED_ACTIVE_SIZE];
  assign qsfp1_led_link   = led_reg[QSFP1_LED_LINK+:QSFP1_LED_LINK_SIZE];

  assign ipass_cable_present = ipass_reg;

  //---------------------------------------------------------------------------
  // Handling of ControlPort Requests
  //---------------------------------------------------------------------------

  always @(posedge ctrlport_clk) begin
    // Reset internal registers and responses
    if (ctrlport_rst) begin
      scratch_reg <= 0;
      led_reg <= 0;
      ipass_reg <= 0;
      s_ctrlport_resp_ack <= 1'b0;

    // Write requests
    end else if (s_ctrlport_req_wr) begin
      // Always issue an ack and no data
      s_ctrlport_resp_ack    <= 1'b1;
      s_ctrlport_resp_data   <= {CTRLPORT_DATA_W {1'bx}};
      s_ctrlport_resp_status <= CTRL_STS_OKAY;

      case (s_ctrlport_req_addr)
        BASE_ADDRESS + SCRATCH_REGISTER:
          scratch_reg <= s_ctrlport_req_data;

        BASE_ADDRESS + LED_REGISTER:
          led_reg <= s_ctrlport_req_data[LED_REGISTER_SIZE-1:0];

        BASE_ADDRESS + CABLE_PRESENT_REG: begin
          ipass_reg[0] <= s_ctrlport_req_data[IPASS0_CABLE_PRESENT];
          ipass_reg[1] <= s_ctrlport_req_data[IPASS1_CABLE_PRESENT];
        end

        // Error on undefined address
        default: begin
          if (address_in_range) begin
            s_ctrlport_resp_status <= CTRL_STS_CMDERR;

          // No response if out of range
          end else begin
            s_ctrlport_resp_ack <= 1'b0;
          end
        end
      endcase

    // Read request
    end else if (s_ctrlport_req_rd) begin
      // Default assumption: valid request
      s_ctrlport_resp_ack <= 1'b1;
      s_ctrlport_resp_status <= CTRL_STS_OKAY;

      case (s_ctrlport_req_addr)
        BASE_ADDRESS + SIGNATURE_REGISTER:
          s_ctrlport_resp_data <= PL_CPLD_SIGNATURE;

        BASE_ADDRESS + REVISION_REGISTER:
          s_ctrlport_resp_data <= CPLD_REVISION;

        BASE_ADDRESS + OLDEST_COMPATIBLE_REVISION_REGISTER:
          s_ctrlport_resp_data <= OLDEST_CPLD_REVISION;

        BASE_ADDRESS + SCRATCH_REGISTER:
          s_ctrlport_resp_data <= scratch_reg;

        BASE_ADDRESS + GIT_HASH_REGISTER:
            `ifdef GIT_HASH
              s_ctrlport_resp_data <= `GIT_HASH;
            `else
              s_ctrlport_resp_data <= 32'hDEADBEEF;
            `endif

        BASE_ADDRESS + LED_REGISTER:
          s_ctrlport_resp_data <= {{(CTRLPORT_DATA_W - LED_REGISTER_SIZE){1'b0}}, led_reg};

        BASE_ADDRESS + CABLE_PRESENT_REG: begin
          s_ctrlport_resp_data                       <= {CTRLPORT_DATA_W {1'b0}};
          s_ctrlport_resp_data[IPASS0_CABLE_PRESENT] <= ipass_reg[0];
          s_ctrlport_resp_data[IPASS1_CABLE_PRESENT] <= ipass_reg[1];
        end

        // Error on undefined address
        default: begin
          s_ctrlport_resp_data <= {CTRLPORT_DATA_W {1'bx}};
          if (address_in_range) begin
            s_ctrlport_resp_status <= CTRL_STS_CMDERR;

          // No response if out of range
          end else begin
            s_ctrlport_resp_ack <= 1'b0;
          end
        end
      endcase

    // No request
    end else begin
      s_ctrlport_resp_ack <= 1'b0;
    end
  end

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="PL_CPLD_BASE_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="PL_CPLD_BASE_REGS">
//    <info>
//      Basic registers containing version and capabilities information.
//    </info>
//
//    <register name="SIGNATURE_REGISTER" offset="0x00" writable="false" size="32">
//      <info>Contains the product's signature.</info>
//      <bitfield name="PRODUCT_SIGNATURE" range="31..0">
//        <info>Fixed value PL_CPLD_SIGNATURE of @.CONSTANTS_REGMAP</info>
//      </bitfield>
//    </register>
//
//    <register name="REVISION_REGISTER" offset="0x04" writable="false" size="32">
//      <info>Contains the CPLD revision (see CPLD_REVISION of @.CONSTANTS_REGMAP)</info>
//      <bitfield name="REVISION_HH" range="7..0">
//        <info>Contains revision hour code.</info>
//      </bitfield>
//      <bitfield name="REVISION_DD" range="15..8">
//        <info>Contains revision day code.</info>
//      </bitfield>
//      <bitfield name="REVISION_MM" range="23..16">
//        <info>Contains revision month code.</info>
//      </bitfield>
//      <bitfield name="REVISION_YY" range="31..24">
//        <info>Contains revision year code.</info>
//      </bitfield>
//    </register>
//
//    <register name="OLDEST_COMPATIBLE_REVISION_REGISTER" offset="0x08" writable="false" size="32">
//      <info>
//        This register returns (in YYMMDDHH format) the oldest revision
//        that is still compatible with this one.  Compatible means that
//        registers or register bits may have been added, but not
//        modified or deleted (see OLDEST_CPLD_REVISION of @.CONSTANTS_REGMAP).
//      </info>
//      <bitfield name="OLD_REVISION_HH" range="7..0">
//        <info>Contains revision hour code.</info>
//      </bitfield>
//      <bitfield name="OLD_REVISION_DD" range="15..8">
//        <info>Contains revision day code.</info>
//      </bitfield>
//      <bitfield name="OLD_REVISION_MM" range="23..16">
//        <info>Contains revision month code.</info>
//      </bitfield>
//      <bitfield name="OLD_REVISION_YY" range="31..24">
//        <info>Contains revision year code.</info>
//      </bitfield>
//    </register>
//
//    <register name="SCRATCH_REGISTER" offset="0x0C" size="32">
//      <info>Read/write register for general software use.</info>
//    </register>
//
//    <register name="GIT_HASH_REGISTER" offset="0x10" size="32" writable="false">
//      <info>
//        Git hash of commit used to build this image.{br}
//        Value equals 0xDEADBEEF if the git hash was not used during synthesis.
//      </info>
//      <bitfield name="GIT_CLEAN" range="31..28">
//        <info>
//          0x0 in case the git status was clean{br}
//          0xF in case there were uncommitted changes
//        </info>
//      </bitfield>
//      <bitfield name="GIT_HASH" range="27..0">
//        <info>7 hex digit hash code of the commit</info>
//      </bitfield>
//    </register>
//  </group>
//
//  <group name="MB_CPLD_LED_REGS">
//    <info>
//      Register Map to control QSFP LEDs.
//    </info>
//    <register name="LED_REGISTER" offset="0x20" size="16">
//      <info>
//        Provides to the LEDs of the QSFP ports.
//        Write access will directly change the LED status.
//        The LED lights up if the corresponding bit is set.
//      </info>
//      <bitfield name="QSFP0_LED_LINK" range="3..0">
//        <info>Link LEDs of QSFP port 0</info>
//      </bitfield>
//      <bitfield name="QSFP0_LED_ACTIVE" range="7..4">
//        <info>Active LEDs of QSFP port 0</info>
//      </bitfield>
//      <bitfield name="QSFP1_LED_LINK" range="11..8">
//        <info>Link LEDs of QSFP port 1</info>
//      </bitfield>
//      <bitfield name="QSFP1_LED_ACTIVE" range="15..12">
//        <info>Active LEDs of QSFP port 1</info>
//      </bitfield>
//    </register>
//  </group>
//
//  <group name="PL_CMI_REGS">
//    <info>
//      Cable present status register.
//    </info>
//    <register name="CABLE_PRESENT_REG" offset="0x30" size="2">
//      <info>
//        Information from FPGA about the cable present status.
//      </info>
//      <bitfield name="IPASS0_CABLE_PRESENT" range="0">
//        <info>Set to 1 if cable present in iPass 0 connector.</info>
//      </bitfield>
//      <bitfield name="IPASS1_CABLE_PRESENT" range="1">
//        <info>Set to 1 if cable present in iPass 1 connector.</info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off
