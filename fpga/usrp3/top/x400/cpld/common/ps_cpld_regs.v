//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ps_cpld_regs
//
// Description:
//
//   Basic registers to inform software about version and capabilities.
//
// Parameters:
//
//   BASE_ADDRESS : Base address for CtrlPort registers
//

`default_nettype none


module ps_cpld_regs #(
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

  // Configuration outputs
  output reg  [ 1:0] db_clk_enable      = 2'b00,
  output reg  [ 1:0] db_reset           = 2'b11,
  output reg         pll_ref_clk_enable = 1'b0,

  output reg  [11:0] dio_direction_a    = 12'b0,
  output reg  [11:0] dio_direction_b    = 12'b0,

  output reg  [39:0] serial_num         = 40'b0,
  output reg         cmi_ready          = 1'b0,
  input  wire        cmi_other_side_detected
);
`ifdef X410
`include "../regmap/x410/constants_regmap_utils.vh"
`else
`include "../regmap/x440/constants_regmap_utils.vh"
`endif
`include "../regmap/ps_cpld_base_regmap_utils.vh"
`include "../../../../lib/rfnoc/core/ctrlport.vh"

//-----------------------------------------------------------------------------
// Address Calculation
//-----------------------------------------------------------------------------

localparam NUM_ADDRESSES = 64;
wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) &&
                        (s_ctrlport_req_addr < BASE_ADDRESS + NUM_ADDRESSES);

//-----------------------------------------------------------------------------
// Internal Registers
//-----------------------------------------------------------------------------

reg [SCRATCH_REGISTER_SIZE-1:0] scratch_reg;

//-----------------------------------------------------------------------------
// Handling of ControlPort Requests
//-----------------------------------------------------------------------------

always @(posedge ctrlport_clk) begin
  // Reset internal registers and responses
  if (ctrlport_rst) begin
    scratch_reg             <= 0;
    db_clk_enable           <= 2'b00;
    db_reset                <= 2'b11;
    pll_ref_clk_enable      <= 1'b0;
    dio_direction_a         <= {DIO_DIRECTION_A_SIZE{1'b0}};
    dio_direction_b         <= {DIO_DIRECTION_B_SIZE{1'b0}};
    s_ctrlport_resp_ack     <= 1'b0;
    s_ctrlport_resp_data    <= {CTRLPORT_ADDR_W {1'bx}};
    s_ctrlport_resp_status  <= CTRL_STS_OKAY;

  // Write requests
  end else begin
    if (s_ctrlport_req_wr) begin
      // Always issue an ack and no data
      s_ctrlport_resp_ack    <= 1'b1;
      s_ctrlport_resp_data   <= {CTRLPORT_ADDR_W {1'bx}};
      s_ctrlport_resp_status <= CTRL_STS_OKAY;

      case (s_ctrlport_req_addr)
        BASE_ADDRESS + SCRATCH_REGISTER:
          scratch_reg <= s_ctrlport_req_data;

        BASE_ADDRESS + PL_DB_REGISTER: begin
          if (s_ctrlport_req_data[DISABLE_CLOCK_DB0]) begin
            db_clk_enable[0] <= 1'b0;
          end else if (s_ctrlport_req_data[ENABLE_CLOCK_DB0]) begin
            db_clk_enable[0] <= 1'b1;
          end
          if (s_ctrlport_req_data[DISABLE_CLOCK_DB1]) begin
            db_clk_enable[1] <= 1'b0;
          end else if (s_ctrlport_req_data[ENABLE_CLOCK_DB1]) begin
            db_clk_enable[1] <= 1'b1;
          end
          if (s_ctrlport_req_data[DISABLE_PLL_REF_CLOCK]) begin
            pll_ref_clk_enable <= 1'b0;
          end else if (s_ctrlport_req_data[ENABLE_PLL_REF_CLOCK]) begin
            pll_ref_clk_enable <= 1'b1;
          end
          if (s_ctrlport_req_data[ASSERT_RESET_DB0]) begin
            db_reset[0] <= 1'b1;
          end else if (s_ctrlport_req_data[RELEASE_RESET_DB0]) begin
            db_reset[0] <= 1'b0;
          end
          if (s_ctrlport_req_data[ASSERT_RESET_DB1]) begin
            db_reset[1] <= 1'b1;
          end else if (s_ctrlport_req_data[RELEASE_RESET_DB1]) begin
            db_reset[1] <= 1'b0;
          end
        end

        BASE_ADDRESS + DIO_DIRECTION_REGISTER: begin
          dio_direction_a <= s_ctrlport_req_data[DIO_DIRECTION_A_MSB:DIO_DIRECTION_A];
          dio_direction_b <= s_ctrlport_req_data[DIO_DIRECTION_B_MSB:DIO_DIRECTION_B];
        end

        BASE_ADDRESS + SERIAL_NUM_LOW_REG: begin
          serial_num[31:0] <= s_ctrlport_req_data;
        end

        BASE_ADDRESS + SERIAL_NUM_HIGH_REG: begin
          serial_num[39:32] <= s_ctrlport_req_data[SERIAL_NUM_HIGH_REG_SIZE-1:0];
        end

        BASE_ADDRESS + CMI_CONTROL_STATUS: begin
          cmi_ready <= s_ctrlport_req_data[CMI_READY];
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
      s_ctrlport_resp_ack     <= 1'b1;
      s_ctrlport_resp_status  <= CTRL_STS_OKAY;
      s_ctrlport_resp_data    <= {CTRLPORT_DATA_W {1'b0}};

      case (s_ctrlport_req_addr)
        BASE_ADDRESS + SIGNATURE_REGISTER:
          s_ctrlport_resp_data <= PS_CPLD_SIGNATURE;

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

        BASE_ADDRESS + PL_DB_REGISTER: begin
          s_ctrlport_resp_data[DB0_CLOCK_ENABLED]     <= db_clk_enable[0];
          s_ctrlport_resp_data[DB1_CLOCK_ENABLED]     <= db_clk_enable[1];
          s_ctrlport_resp_data[PLL_REF_CLOCK_ENABLED] <= pll_ref_clk_enable;
          s_ctrlport_resp_data[DB0_RESET_ASSERTED]    <= db_reset[0];
          s_ctrlport_resp_data[DB1_RESET_ASSERTED]    <= db_reset[1];
        end

        BASE_ADDRESS + DIO_DIRECTION_REGISTER: begin
          s_ctrlport_resp_data[DIO_DIRECTION_A_MSB:DIO_DIRECTION_A] <= dio_direction_a;
          s_ctrlport_resp_data[DIO_DIRECTION_B_MSB:DIO_DIRECTION_B] <= dio_direction_b;
        end

        BASE_ADDRESS + SERIAL_NUM_LOW_REG: begin
          s_ctrlport_resp_data <= serial_num[31:0];
        end

        BASE_ADDRESS + SERIAL_NUM_HIGH_REG: begin
          s_ctrlport_resp_data[SERIAL_NUM_HIGH_REG_SIZE-1:0] <= serial_num[39:32];
        end

        BASE_ADDRESS + CMI_CONTROL_STATUS: begin
          s_ctrlport_resp_data[CMI_READY] <= cmi_ready;
          s_ctrlport_resp_data[OTHER_SIDE_DETECTED] <= cmi_other_side_detected;
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
end

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="PS_CPLD_BASE_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="PS_CPLD_BASE_REGS">
//    <info>
//      Basic registers containing version and capabilites information.
//    </info>
//
//    <register name="SIGNATURE_REGISTER" offset="0x00" writable="false" size="32">
//      <info>Contains the product's signature.</info>
//      <bitfield name="PRODUCT_SIGNATURE" range="31..0">
//        <info>Fixed value PS_CPLD_SIGNATURE of @.CONSTANTS_REGMAP</info>
//      </bitfield>
//    </register>
//
//    <register name="REVISION_REGISTER" offset="0x04" writable="false" size="32">
//      <info>Contains the CPLD revision (see CPLD_REVISION of @.CONSTANTS_REGMAP).</info>
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
//        that is still compatible with this one. Compatible means that
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
//  <group name="PS_CONTROL_REGS">
//    <info>
//      Register Map to control MB CPLD functions.
//    </info>
//    <register name="PL_DB_REGISTER" offset="0x20" size="32">
//      <info>
//        Register to control the PL part DB SPI connection and reset generation.
//        The DB connection is clocked with PLL reference clock. Ensure this clock is stable
//        and enabled before starting any SPI request.
//        The PLL reference clock can be disabled if both DB connections are disabled or inactive.
//        To enable the DB connection, enable clock with one write access and release
//        reset with the next write access.
//        To disable the DB connection, assert reset with one write access and
//        disable clocks with the next write access.
//      </info>
//      <bitfield name="DB0_CLOCK_ENABLED" range="0" writable="false">
//        <info>Indicates if a clock is forwarded to DB 0.</info>
//      </bitfield>
//      <bitfield name="DB1_CLOCK_ENABLED" range="1" writable="false">
//        <info>Indicates if a clock is forwarded to DB 1.</info>
//      </bitfield>
//      <bitfield name="PLL_REF_CLOCK_ENABLED" range="2" writable="false">
//        <info>Indicates if the PLL reference clock for the PL interface is enabled.</info>
//      </bitfield>
//      <bitfield name="DB0_RESET_ASSERTED" range="4" writable="false">
//        <info>Indicates that reset is asserted for DB 0.</info>
//      </bitfield>
//      <bitfield name="DB1_RESET_ASSERTED" range="5" writable="false">
//        <info>Indicates that reset is asserted for DB 1.</info>
//      </bitfield>
//      <bitfield name="ENABLE_CLOCK_DB0" range="8" readable="false">
//        <info>Writing with this flag set enables DB 0 clock forwarding. (may be overwritten by @.DISABLE_CLOCK_DB0)</info>
//      </bitfield>
//      <bitfield name="ENABLE_CLOCK_DB1" range="9" readable="false">
//        <info>Writing with this flag set enables DB 1 clock forwarding. (may be overwritten by @.DISABLE_CLOCK_DB1)</info>
//      </bitfield>
//      <bitfield name="ENABLE_PLL_REF_CLOCK" range="10" readable="false">
//        <info>Writing with this flag set enables the PLL reference clock. Assert this flag after PLL reference clock is stable. (may be overwritten by @.DISABLE_PLL_REF_CLOCK)</info>
//      </bitfield>
//      <bitfield name="DISABLE_CLOCK_DB0" range="12" readable="false">
//        <info>Writing with this flag set disables DB 0 clock forwarding (overrides @.ENABLE_CLOCK_DB0)</info>
//      </bitfield>
//      <bitfield name="DISABLE_CLOCK_DB1" range="13" readable="false">
//        <info>Writing with this flag set disables DB 1 clock forwarding (overrides @.ENABLE_CLOCK_DB1)</info>
//      </bitfield>
//      <bitfield name="DISABLE_PLL_REF_CLOCK" range="14" readable="false">
//        <info>Writing with this flag set disables the PLL reference clock (overrides @.ENABLE_PLL_REF_CLOCK). Assert this flag to reconfigure the clock.</info>
//      </bitfield>
//      <bitfield name="RELEASE_RESET_DB0" range="16" readable="false">
//        <info>Writing with this flag set releases DB 0 reset. (may be overwritten by @.ASSERT_RESET_DB0)</info>
//      </bitfield>
//      <bitfield name="RELEASE_RESET_DB1" range="17" readable="false">
//        <info>Writing with this flag set releases DB 1 reset. (may be overwritten by @.ASSERT_RESET_DB1)</info>
//      </bitfield>
//      <bitfield name="ASSERT_RESET_DB0" range="20" readable="false">
//        <info>Writing with this flag set asserts reset for DB 0 (overrides @.RELEASE_RESET_DB0)</info>
//      </bitfield>
//      <bitfield name="ASSERT_RESET_DB1" range="21" readable="false">
//        <info>Writing with this flag set asserts reset for DB 1 (overrides @.RELEASE_RESET_DB1)</info>
//      </bitfield>
//    </register>
//  </group>
//
//  <group name="DIO_REGS">
//    <info>
//      Registers to control the GPIO buffer direction on the DIO board connected to the FPGA.
//      Make sure the GPIO lines between FPGA and GPIO board are not driven by two drivers.
//      Set the direction in the FPGA's DIO register appropriately.
//    </info>
//    <register name="DIO_DIRECTION_REGISTER" offset="0x30" size="32">
//      <info>
//        Set the direction of FPGA buffer connected to DIO ports on the DIO board.{br/}
//        Each bit represents one signal line. 0 = line is an input to the FPGA, 1 = line is an output driven by the FPGA.
//      </info>
//      <bitfield name="DIO_DIRECTION_A" range="11..0" initialvalue="0"/>
//      <bitfield name="DIO_DIRECTION_B" range="27..16" initialvalue="0"/>
//    </register>
//  </group>
//
//  <group name="PS_CMI_REGS">
//    <info>
//      Cable present status register.
//    </info>
//    <register name="SERIAL_NUM_LOW_REG" offset="0x34" size="32">
//      <info>Least significant bytes of 5 byte serial number.</info>
//    </register>
//    <register name="SERIAL_NUM_HIGH_REG" offset="0x38" size="8">
//      <info>Most significant byte of 5 byte serial number.</info>
//    </register>
//    <register name="CMI_CONTROL_STATUS" offset="0x3C" size="32">
//      <info>Control CMI communication and delivers information on the CMI link status.</info>
//      <bitfield name="CMI_READY" range="0">
//        <info>Set if the device is ready to establish a PCI-Express link (affects CMI_CLP_READY bit).</info>
//      </bitfield>
//      <bitfield name="OTHER_SIDE_DETECTED" range="31" writable="false">
//        <info>1 if an upstream CMI device has been detected.</info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off
