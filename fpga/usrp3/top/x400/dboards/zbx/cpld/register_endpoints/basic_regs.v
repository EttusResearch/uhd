//
// Copyright 2021 Ettus Research, a National Instruments Brand
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
  input  wire        ctrlport_rst
);

  `include "../regmap/basic_regs_regmap_utils.vh"
  `include "../../../../../../lib/rfnoc/core/ctrlport.vh"

  //----------------------------------------------------------
  // Internal registers
  //----------------------------------------------------------
  reg [SCRATCH_REG_SIZE-1:0]  scratch_reg = {SCRATCH_REG_SIZE   {1'b0}};

`ifdef VARIANT_XO3
  localparam VARIANT_ID = VARIANT_ID_XO3;
`else
  localparam VARIANT_ID = VARIANT_ID_MAX10;
`endif

  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);

  always @(posedge ctrlport_clk) begin
    // reset internal registers and responses
    if (ctrlport_rst) begin
      scratch_reg             <= {SCRATCH_REG_SIZE {1'b0}};

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
          BASE_ADDRESS + SLAVE_SCRATCH: begin
            scratch_reg <= s_ctrlport_req_data[ SCRATCH_REG_MSB : SCRATCH_REG];
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
          BASE_ADDRESS + SLAVE_SIGNATURE: begin
            s_ctrlport_resp_data[BOARD_ID_MSB : BOARD_ID]
                                    <= BOARD_ID_VALUE[BOARD_ID_SIZE-1:0];
          end

          BASE_ADDRESS + SLAVE_REVISION: begin
            s_ctrlport_resp_data[REVISION_REG_MSB : REVISION_REG]
                                    <= CPLD_REVISION[REVISION_REG_SIZE-1:0];
          end

          BASE_ADDRESS + SLAVE_OLDEST_REVISION: begin
            s_ctrlport_resp_data[OLDEST_REVISION_REG_MSB : OLDEST_REVISION_REG]
                                    <= OLDEST_CPLD_REVISION[OLDEST_REVISION_REG_SIZE-1:0];
          end

          BASE_ADDRESS + SLAVE_SCRATCH: begin
            s_ctrlport_resp_data[SCRATCH_REG_MSB : SCRATCH_REG] <= scratch_reg;
          end

          BASE_ADDRESS + GIT_HASH_REGISTER: begin
            `ifdef GIT_HASH
              s_ctrlport_resp_data <= `GIT_HASH;
            `else
              s_ctrlport_resp_data <= 32'hDEADBEEF;
            `endif
          end

          BASE_ADDRESS + SLAVE_VARIANT: begin
            s_ctrlport_resp_data[VARIANT_REG_MSB : VARIANT_REG]
                                    <= VARIANT_ID[VARIANT_REG_SIZE-1:0];
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

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="BASIC_REGS_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
// <group name="BASIC_REGS_REGISTERS" size="0x010">
//   <info>
//     This regmap contains the revision registers, signature register, a scratch register, and a slave control reg.
//   </info>
//
//    <enumeratedtype name="BASIC_REGISTERS_VALUES" showhexvalue="true">
//      <info>
//        This enum is used to create the constants held in the basic registers in both verilog and vhdl.
//      </info>
//      <value name="BOARD_ID_VALUE"        integer="0x4002"/>
//      <value name="CPLD_REVISION"         integer="0x22082233"/>
//      <value name="OLDEST_CPLD_REVISION"  integer="0x20110611"/>
//      <value name="VARIANT_ID_XO3"        integer="0x584F33"/>
//      <value name="VARIANT_ID_MAX10"      integer="0x4D4158"/>
//    </enumeratedtype>
//
//   <register name="SLAVE_SIGNATURE" size="32" offset="0x00" attributes="Readable">
//     <info>
//       This register contains the unique signature of the DB. This signature is the same value as the one
//       stored on the board ID EEPROM
//     </info>
//     <bitfield name="BOARD_ID" range="15..0" type="integer">
//       <info>
//          Board ID corresponds to the las 16 digits of the daughterboard part number.
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="SLAVE_REVISION" size="32" offset="0x04" attributes="Readable">
//     <info>
//       This register contains the revision number of the current build
//     </info>
//     <bitfield name="REVISION_REG" range="31..0" type="integer">
//       <info>
//         Returns the revision in YYMMDDHH format
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="SLAVE_OLDEST_REVISION" size="32" offset="0x08" attributes="Readable">
//     <info>
//       This register contains the revision number of the oldest compatible revision
//     </info>
//     <bitfield name="OLDEST_REVISION_REG" range="31..0" type="integer">
//       <info>
//         Returns the oldest compatible revision in YYMMDDHH format
//       </info>
//     </bitfield>
//   </register>
//
//   <register name="SLAVE_SCRATCH" size="32" offset="0x0C" attributes="Readable|Writable">
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
//
//   <register name="SLAVE_VARIANT" size="32" offset="0x14" writable="false">
//     <info>
//       Contains information pertaining the variant of the programmable.
//     </info>
//     <bitfield name="VARIANT_REG" range="31..0" initialvalue="0">
//       <info>
//         Returns the variant of the programmable based on the part vendor.
//         MAX10 variants return 0x583033(ASCII for MAX), while the XO3 variant
//         returns 0x584F33 (ASCII for XO3)
//       </info>
//     </bitfield>
//   </register>
//  </group>
//</regmap>
//XmlParse xml_off
