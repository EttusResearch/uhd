//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_versioning_regs
//
// Description:
//
//   This module contains the motherboard registers for tracking
//   HDL components versions.
//
//   This versioning module is comprised of up to 256 read-only registers,
//   which provide versioning information for up to 64 components (256/4).
//   Each component has 3 x 32-bit registers for the following purpose:
//     - Current version
//     - Oldest compatible version
//     - Last modified time stamp
//
//   Note that in order to facilitate implementation, each component
//   allocates a 4th register address (Reserved) for future use.
//
//   Allocation of the 64 addressable components is shown below.
//   This allocation determines the definition of COMPONENTS_INDEXES
//   in the register map documentation below.
//
//   --- Common components ---
//   Reserved space for up to 24 components.
//     - FPGA
//     - MB CPLD interface
//     - RF core (db 0)
//     - RF core (db 1)
//     - GPIO interface (db 0)
//     - GPIO interface (db 1)
//
//   --- UHD-specific components ---
//   Reserved space for up to 20 components.
//     - QSFP wrapper (port 0)
//     - QSFP wrapper (port 1)
//
//   --- LV-specific components ---
//   Reserved space for up to 20 components.
//
// Parameters:
//
//   REG_BASE : Base address to use for registers.
//

`default_nettype none


module x4xx_versioning_regs #(
  parameter REG_BASE = 0
) (
  // Slave ctrlport interface
  input  wire        s_ctrlport_clk,
  input  wire        s_ctrlport_rst,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack    = 1'b0,
  output reg  [ 1:0] s_ctrlport_resp_status = 2'b00,
  output reg  [31:0] s_ctrlport_resp_data   = {32 {1'bX}},

  // Version (Constant)
  // Each component consists of a 96-bit vector (refer to versioning_utils.vh)
  input wire [64*96-1:0] version_info
);

  // Variant-dependent register map.
  `ifdef X440
    `include "regmap/x440/versioning_regs_regmap_utils.vh"
  `else // Use X410 as the default variant for regmap.
    `include "regmap/x410/versioning_regs_regmap_utils.vh"
  `endif

  `include "regmap/versioning_utils.vh"
  `include "../../lib/rfnoc/core/ctrlport.vh"

  // 64 components * 4 registers * 4 addresses p/ register
  localparam REG_SIZE = MAX_NUM_OF_COMPONENTS*4*4;

  //--------------------------------------------------------------------
  // Versioning Registers
  // -------------------------------------------------------------------

  // Check that address is within this module's range.
  wire address_in_range = (s_ctrlport_req_addr >= REG_BASE) && (s_ctrlport_req_addr < REG_BASE + REG_SIZE);
  // Mask out 6 bits (64 components) to be able to compare all components
  // against the same base register address.
  wire [31:0] register_base_address = {s_ctrlport_req_addr[19:10], 6'b0, s_ctrlport_req_addr[3:0]};
  // Extract masked out bits from the address, which represent the
  // component that is being addressed (0-63) = 6 bits.
  wire [ 5:0] component_index = s_ctrlport_req_addr[9:4];

  // Obtain the indexed component's versions
  wire [COMPONENT_VERSIONS_SIZE-1:0] component_versions = get_component_versions(version_info, component_index);

  // Registers implementation
  always @ (posedge s_ctrlport_clk) begin
    if (s_ctrlport_rst) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_status <= 2'b00;
      s_ctrlport_resp_data   <= {32 {1'bX}};

    end else begin
      // Write registers
      if (s_ctrlport_req_wr) begin
        // Do not acknowledge by default
        s_ctrlport_resp_ack  <= 1'b0;
        s_ctrlport_resp_data <= 32'h0;

        // No writable registers

        // Acknowledge and provide error status if address is in range
        if (address_in_range) begin
          s_ctrlport_resp_ack    <= 1'b1;
          s_ctrlport_resp_status <= CTRL_STS_CMDERR;
        end

      // Read registers
      end else if (s_ctrlport_req_rd) begin
        // Acknowledge by default
        s_ctrlport_resp_ack    <= 1'b1;
        s_ctrlport_resp_data   <= 32'h0;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (register_base_address)

          // Each concatenated 96-bit vector contains 3 x 32-bit values:
          //   [31: 0] -> Current version
          //   [63:32] -> Oldest compatible version
          //   [95:64] -> Last modified

          REG_BASE + CURRENT_VERSION(0): begin
            s_ctrlport_resp_data[VERSION_TYPE_SIZE-1:0]   <= current_version(component_versions);
          end
          REG_BASE + OLDEST_COMPATIBLE_VERSION(0): begin
            s_ctrlport_resp_data[VERSION_TYPE_SIZE-1:0]   <= oldest_compatible_version(component_versions);
          end
          REG_BASE + VERSION_LAST_MODIFIED(0): begin
            s_ctrlport_resp_data[TIMESTAMP_TYPE_SIZE-1:0] <= version_last_modified(component_versions);
          end

          // Do not acknowledge if address is not defined
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // No response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase

      end else begin
        s_ctrlport_resp_ack <= 1'b0;
      end
    end
  end

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="VERSIONING_REGS_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="VERSIONING_REGS">
//    <regtype name="VERSION_TYPE" size="32" attributes="Readable">
//      <bitfield name="MAJOR" range="31..23" initialvalue="0">
//        <info>
//          Major number (max = 511): an increase reflects a breaking change.{BR/}
//          <b>IMPORTANT!</b> @.MAJOR must always remain in sync between the component's
//          @.CURRENT_VERSION and @.OLDEST_COMPATIBLE_VERSION registers.{BR/}{BR/}
//          Update @.MAJOR when:
//          <li>the component has changed and requires a software changes as a result.
//          <li>the component's bitfields/registers have been modified or deleted.
//          <li>the component's bitfields/registers are initialized to different value (unexpected by software).
//          <li>new bitfields/registers are added that require software interaction for the component to operate.
//        </info>
//      </bitfield>
//      <bitfield name="MINOR" range="22..12" initialvalue="0">
//        <info>
//          Minor number (max = 2047): an increase reflects a non-breaking change that the driver should be aware of.{BR/}{BR/}
//          Update @.MINOR when:
//          <li>a new feature is added to the component, which does not conflict with the driver.
//          <li>minor implementation changes were made to the component which are worth tracking.
//          <li>the component has added new bitfields/registers that do not require software interaction
//              (i.e. the default value is 0 and writing 0 does not change behavior, assuming SW writes 0's to
//              previously undefined bits).
//          <li>@.MAJOR is updated (reset @.MINOR to 0).
//        </info>
//      </bitfield>
//      <bitfield name="BUILD" range="11..0" initialvalue="0">
//        <info>
//          Build number (max = 4095): an increase reflects a change in the source code that yields a new implementation,
//          but that should not impact the component's behavior {BR/}
//          Eventually, this number is intended to be automatically incremented for any new build.{BR/}{BR/}
//          Meanwhile, update @.BUILD when:
//          <li>the component's source code changes are not captured by @.MAJOR or @.MINOR.
//          <li>@.MINOR or @.MAJOR are updated (reset @.BUILD to 0).
//        </info>
//      </bitfield>
//    </regtype>
//    <regtype name="TIMESTAMP_TYPE" size="32" attributes="Readable">
//      <info>
//        Component's versions update time.{BR/}
//        This register provides the time stamp for the last modification to
//        the component's versions (current & oldest compatible).
//        The time stamp is provided in hexadecimal format: 0xYYMMDDHH.
//      </info>
//      <bitfield name="YY" range="31..24">
//        <info>This is the year number after 2000 (e.g. 2019 = 0x19).</info>
//      </bitfield>
//      <bitfield name="MM" range="23..16"/>
//      <bitfield name="DD" range="15..8"/>
//      <bitfield name="HH" range="7..0"/>
//    </regtype>
//    <regtype name="RESERVED_TYPE" size="32" attributes="Readable">
//      <info>
//        Reserved.
//      </info>
//    </regtype>
//
//    <enumeratedtype name="COMPONENTS_INDEXES">
//      <info>
//        This enum contains indexes for all the components in the X410
//        (both common and app-specific) which version information is
//        desired to be available for compatibility tracking purposes.{BR/}
//        {table border="1"}
//          {tr}{th}Description{/th}             {th}Index range{/th} {th}Max # of components{/th}{/tr}
//          {tr}{td}Common components{/td}       {td}0 to 23{/td}     {td}24{/td}{/tr}
//          {tr}{td}UHD-specific components{/td} {td}24 to 43{/td}    {td}20{/td}{/tr}
//          {tr}{td}LV-specific components{/td}  {td}44 to 63{/td}    {td}20{/td}{/tr}
//        {/table}
//      </info>
//      <value name="FPGA_VERSION_INDEX"    integer="0"/>
//      <value name="CPLD_IFC_INDEX"        integer="1"/>
//      <value name="DB0_RF_CORE_INDEX"     integer="2"/>
//      <value name="DB1_RF_CORE_INDEX"     integer="3"/>
//      <value name="DB0_GPIO_IFC_INDEX"    integer="4"/>
//      <value name="DB1_GPIO_IFC_INDEX"    integer="5"/>
//    </enumeratedtype>
//
//    <register name="CURRENT_VERSION"           offset="0x0" count="64" step="16" typename="VERSION_TYPE">
//      <info>
//        Component's current version.{BR/}
//        This register contains the current component's version implemented in HDL.
//        The current version shall be used to detect a component being too
//        old for the driver/software:{BR/}
//        <b>SW oldest compatible version > Component's current version --> Component is too old.</b>
//      </info>
//    </register>
//    <register name="OLDEST_COMPATIBLE_VERSION" offset="0x4" count="64" step="16" typename="VERSION_TYPE">
//      <info>
//        Component's oldest compatible version.{BR/}
//        This register contains the oldest compatible component's version, that is the oldest
//        component's implementation that is compatible with the current implementation.{BR/}
//        The oldest compatible version shall be used to detect a component being too
//        new for the driver/software:{BR/}
//        <b>SW current version < Component's oldest compatible version --> Component is too new.</b>
//      </info>
//    </register>
//    <register name="VERSION_LAST_MODIFIED"     offset="0x8" count="64" step="16" typename="TIMESTAMP_TYPE"/>
//    <register name="RESERVED"                  offset="0xC" count="64" step="16" typename="RESERVED_TYPE"/>
//
//  </group>
//</regmap>
//XmlParse xml_off
