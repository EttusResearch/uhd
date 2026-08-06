//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: basic_regs
//
// Description:
//   Basic Registers to inform software about version and capabilities.
//

`default_nettype none

module basic_regs #(
  parameter [19:0]  BASE_ADDRESS = 0,
  parameter [19:0]  SIZE_ADDRESS = 0
) (
  ctrlport_if.slave s_ctrlport
);

  import ctrlport_pkg::*;
  `include "../regmap/basic_regs_regmap_utils.vh"

  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  wire address_in_range = (s_ctrlport.req.addr >= BASE_ADDRESS) && (s_ctrlport.req.addr < BASE_ADDRESS + SIZE_ADDRESS);

  logic [SCRATCH_REG_SIZE-1:0] scratch_reg;

  always_ff @(posedge s_ctrlport.clk) begin
    // reset internal registers and responses
    if (s_ctrlport.rst) begin
      scratch_reg             <= '0;

      s_ctrlport.resp.ack     <= 1'b0;
      s_ctrlport.resp.data    <= 'x;
      s_ctrlport.resp.status  <= STS_OKAY;

    end else begin

      // write requests
      if (s_ctrlport.req.wr) begin
        // always issue an ack and no data
        s_ctrlport.resp.ack     <= 1'b1;
        s_ctrlport.resp.data    <= 'x;
        s_ctrlport.resp.status  <= STS_OKAY;

        case (s_ctrlport.req.addr)
          BASE_ADDRESS + CORE_SCRATCH: begin
            scratch_reg <= s_ctrlport.req.data[ SCRATCH_REG_MSB : SCRATCH_REG];
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport.resp.status  <= STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport.resp.ack     <= 1'b0;
            end
          end
        endcase

      // read requests
      end else if (s_ctrlport.req.rd) begin
        // default assumption: valid request
        s_ctrlport.resp.ack     <= 1'b1;
        s_ctrlport.resp.status  <= STS_OKAY;
        s_ctrlport.resp.data    <= '0;

        case (s_ctrlport.req.addr)
          BASE_ADDRESS + CORE_SIGNATURE: begin
            s_ctrlport.resp.data[BOARD_ID_MSB : BOARD_ID]
                                    <= BOARD_ID_VALUE[BOARD_ID_SIZE-1:0];
          end

          BASE_ADDRESS + CORE_REVISION: begin
            s_ctrlport.resp.data[CURRENT_REV_MAJOR_MSB : CURRENT_REV_MAJOR]
                <= FPGA_CURRENT_VERSION_MAJOR[CURRENT_REV_MAJOR_SIZE-1:0];
            s_ctrlport.resp.data[CURRENT_REV_MINOR_MSB : CURRENT_REV_MINOR]
                <= FPGA_CURRENT_VERSION_MINOR[CURRENT_REV_MINOR_SIZE-1:0];
            s_ctrlport.resp.data[CURRENT_REV_BUILD_MSB : CURRENT_REV_BUILD]
                <= FPGA_CURRENT_VERSION_BUILD[CURRENT_REV_BUILD_SIZE-1:0];
          end

          BASE_ADDRESS + CORE_OLDEST_REVISION: begin
            s_ctrlport.resp.data[OLDEST_REVISION_MAJOR_MSB : OLDEST_REVISION_MAJOR]
                <= FPGA_OLDEST_COMPATIBLE_VERSION_MAJOR[OLDEST_REVISION_MAJOR_SIZE-1:0];
            s_ctrlport.resp.data[OLDEST_REVISION_MINOR_MSB : OLDEST_REVISION_MINOR]
                <= FPGA_OLDEST_COMPATIBLE_VERSION_MINOR[OLDEST_REVISION_MINOR_SIZE-1:0];
            s_ctrlport.resp.data[OLDEST_REVISION_BUILD_MSB : OLDEST_REVISION_BUILD]
                <= FPGA_OLDEST_COMPATIBLE_VERSION_BUILD[OLDEST_REVISION_BUILD_SIZE-1:0];
          end

          BASE_ADDRESS + CORE_SCRATCH: begin
            s_ctrlport.resp.data[SCRATCH_REG_MSB : SCRATCH_REG] <= scratch_reg;
          end

          BASE_ADDRESS + GIT_HASH_REGISTER: begin
            `ifdef GIT_HASH
              s_ctrlport.resp.data <= `GIT_HASH;
            `else
              s_ctrlport.resp.data <= 32'hDEADBEEF;
            `endif
          end

          // error on undefined address
          default: begin
            s_ctrlport.resp.data <= '0;
            if (address_in_range) begin
              s_ctrlport.resp.status <= STS_CMDERR;

            // no response if out of range
            end else begin
              s_ctrlport.resp.ack <= 1'b0;
            end
          end
        endcase

      // no request
      end else begin
        s_ctrlport.resp.ack <= 1'b0;
      end
    end
  end

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="BASIC_REGS_REGMAP" readablestrobes="false" generateverilog="true" generatesv="false" ettusguidelines="true">
// <group name="BASIC_REGS_REGISTERS" size="0x020">
//   <info>
//     This regmap contains the revision registers, signature register, a scratch register, and a slave control reg.
//   </info>
//
//    <enumeratedtype name="FPGA_VERSION" showhex="true">
//      <info>
//        FPGA version.{BR/}
//        For guidance on when to update these revision numbers,
//        please refer to the register map documentation accordingly:
//        <li> Current version: @.BASIC_REGS_REGMAP..CORE_REVISION
//        <li> Oldest compatible version: @.BASIC_REGS_REGMAP..CORE_OLDEST_REVISION
//      </info>
//      <value name="BOARD_ID_VALUE"                       integer="0xB310"/>
//      <value name="FPGA_CURRENT_VERSION_MAJOR"           integer="2"/>
//      <value name="FPGA_CURRENT_VERSION_MINOR"           integer="1"/>
//      <value name="FPGA_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="FPGA_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="2"/>
//      <value name="FPGA_OLDEST_COMPATIBLE_VERSION_MINOR" integer="0"/>
//      <value name="FPGA_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//    </enumeratedtype>
//
//   <register name="CORE_SIGNATURE" size="32" offset="0x00" attributes="Readable">
//     <info>
//       This register contains the unique signature of the MB. This signature is the same value as the one
//       stored on the board ID EEPROM
//     </info>
//     <bitfield name="BOARD_ID" range="15..0" type="integer">
//       <info>
//          Board ID corresponds to the las 16 digits of the motherboard part number.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_REVISION" size="32" offset="0x04" attributes="Readable">
//     <info>
//       This register contains the revision number of the current build
//     </info>
//     <bitfield name="CURRENT_REV_MAJOR" range="31..24">
//       <info>
//         Major version number of the current build
//       </info>
//     </bitfield>
//     <bitfield name="CURRENT_REV_MINOR" range="23..16">
//       <info>
//         Minor version number of the current build
//       </info>
//     </bitfield>
//     <bitfield name="CURRENT_REV_BUILD" range="15..0">
//       <info>
//         Build number of the current build
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_OLDEST_REVISION" size="32" offset="0x08" attributes="Readable">
//     <info>
//       This register contains the revision number of the oldest compatible revision
//     </info>
//     <bitfield name="OLDEST_REVISION_MAJOR" range="31..24">
//       <info>
//         Major version number of the oldest compatible revision
//       </info>
//     </bitfield>
//     <bitfield name="OLDEST_REVISION_MINOR" range="23..16">
//       <info>
//         Minor version number of the oldest compatible revision
//       </info>
//     </bitfield>
//     <bitfield name="OLDEST_REVISION_BUILD" range="15..0">
//       <info>
//         Build number of the oldest compatible revision
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="CORE_SCRATCH" size="32" offset="0x10" attributes="Readable|Writable">
//     <info>
//       Read/write scratch register
//     </info>
//     <bitfield name="SCRATCH_REG" range="31..0" initialvalue="0">
//       <info>
//         Returns the value written here previously.
//       </info>
//     </bitfield>
//   </register>
//
//    <register name="GIT_HASH_REGISTER" offset="0x14" size="32" writable="false">
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
//
//  </group>
//</regmap>
//XmlParse xml_off
