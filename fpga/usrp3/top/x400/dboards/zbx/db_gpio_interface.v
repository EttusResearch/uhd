//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: db_gpio_interface
//
// Description:
//   Interface for GPIO interface towards daughterboards.
//
//   A ControlPort interface is serialized into bytes along with a valid signal.
//   The ControlPort supports write requests only. Byte enables are not supported.
//   There is support for timed commands.
//   Furthermore there are 4 state wires towards the DB. Ensure an appropriate
//   hold time on the states as the transmission happens in pll_ref_clk, which is
//   slower than radio_clk. Pulses of e.g. just a single clock cycle may not get
//   transferred to the DB.
//
//   The 20 available GPIO lines are assigned with
//   - 5x empty
//   - bytestream direction
//   - bytestream valid
//   - bytestream data (8 bits)
//   - 1x empty
//   - db_state (4 bits)
//

`default_nettype none

module db_gpio_interface (
  // Clocks and reset
  input  wire radio_clk,
  input  wire pll_ref_clk,

  // DB state lines (domain: radio_clk)
  input  wire [ 3:0] db_state,
  // Request (domain: radio_clk)
  input  wire        ctrlport_rst,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  // Response (domain: radio_clk)
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  // GPIO interface (domain: pll_ref_clk)
  input  wire [19:0] gpio_in,
  output wire [19:0] gpio_out,
  output wire [19:0] gpio_out_en,

  // Version (Constant)
  output wire [95:0] version_info
);

  `include "../../regmap/x410/versioning_regs_regmap_utils.vh"
  `include "../../regmap/versioning_utils.vh"

  //----------------------------------------------------------------------------
  // Clock domain crossing (radio_clk -> pll_ref_clk)
  //----------------------------------------------------------------------------
  // Radio_clk is derived from pll_ref_clk by an integer multiplier and
  // originate from the same PLL.
  // Therefore the clock crossing can be achieved by using simple registers.
  // Static timing analysis will be able to meet setup and hold requirements on
  // them.

  wire         ctrlport_rst_prc;

  wire         ctrlport_req_wr_prc;
  wire         ctrlport_req_rd_prc;
  wire  [19:0] ctrlport_req_addr_prc;
  wire  [31:0] ctrlport_req_data_prc;

  wire        ctrlport_resp_ack_prc;
  wire [ 1:0] ctrlport_resp_status_prc;
  wire [31:0] ctrlport_resp_data_prc;

  ctrlport_clk_crossing_derived ctrlport_clk_crossing_derived_i(
    .i_clk                          (radio_clk),
    .o_clk                          (pll_ref_clk),
    .i_ctrlport_rst                 (ctrlport_rst),
    .i_ctrlport_req_wr              (s_ctrlport_req_wr),
    .i_ctrlport_req_rd              (s_ctrlport_req_rd),
    .i_ctrlport_req_addr            (s_ctrlport_req_addr),
    .i_ctrlport_req_data            (s_ctrlport_req_data),
    .i_ctrlport_resp_ack            (s_ctrlport_resp_ack),
    .i_ctrlport_resp_status         (s_ctrlport_resp_status),
    .i_ctrlport_resp_data           (s_ctrlport_resp_data),
    .o_ctrlport_rst                 (ctrlport_rst_prc),
    .o_ctrlport_req_wr              (ctrlport_req_wr_prc),
    .o_ctrlport_req_rd              (ctrlport_req_rd_prc),
    .o_ctrlport_req_addr            (ctrlport_req_addr_prc),
    .o_ctrlport_req_data            (ctrlport_req_data_prc),
    .o_ctrlport_resp_ack            (ctrlport_resp_ack_prc),
    .o_ctrlport_resp_status         (ctrlport_resp_status_prc),
    .o_ctrlport_resp_data           (ctrlport_resp_data_prc)
  );

  // transfer state lines
  reg [3:0] db_state_prc    = 4'b0;
  reg [3:0] db_state_prc_fe = 4'b0;
  always @(posedge pll_ref_clk) begin
    db_state_prc    <= db_state;
  end
  always @(negedge pll_ref_clk) begin
    db_state_prc_fe <= db_state_prc;
  end

  //----------------------------------------------------------------------------
  // Ctrlport serializer
  //----------------------------------------------------------------------------
  wire [7:0] bytestream_data_in;
  wire [7:0] bytestream_data_out;
  wire       bytestream_direction;
  wire       bytestream_output_enable;
  wire       bytestream_valid_in;
  wire       bytestream_valid_out;

  ctrlport_byte_serializer serializer_i (
    .ctrlport_clk              (pll_ref_clk),
    .ctrlport_rst              (ctrlport_rst_prc),
    .s_ctrlport_req_wr         (ctrlport_req_wr_prc),
    .s_ctrlport_req_rd         (ctrlport_req_rd_prc),
    .s_ctrlport_req_addr       (ctrlport_req_addr_prc),
    .s_ctrlport_req_data       (ctrlport_req_data_prc),
    .s_ctrlport_resp_ack       (ctrlport_resp_ack_prc),
    .s_ctrlport_resp_status    (ctrlport_resp_status_prc),
    .s_ctrlport_resp_data      (ctrlport_resp_data_prc),
    .bytestream_data_in        (bytestream_data_in),
    .bytestream_valid_in       (bytestream_valid_in),
    .bytestream_data_out       (bytestream_data_out),
    .bytestream_valid_out      (bytestream_valid_out),
    .bytestream_direction      (bytestream_direction),
    .bytestream_output_enable  (bytestream_output_enable)
  );

  // IOB registers to drive data on the falling edge
  reg [7:0] bytestream_data_out_fe;
  reg       bytestream_direction_fe;
  reg       bytestream_output_enable_fe;
  reg       bytestream_valid_out_fe;

  // Signals are shifted into a falling edge domain to help meet
  // hold requirements at CPLD
  always @(negedge pll_ref_clk) begin
    if (ctrlport_rst_prc) begin
      bytestream_data_out_fe      <= 8'b0;
      bytestream_valid_out_fe     <= 1'b0;
      bytestream_direction_fe     <= 1'b0;
      bytestream_output_enable_fe <= 1'b1;
    end else begin
      bytestream_data_out_fe      <= bytestream_data_out;
      bytestream_valid_out_fe     <= bytestream_valid_out;
      bytestream_direction_fe     <= bytestream_direction;
      bytestream_output_enable_fe <= bytestream_output_enable;
    end
  end

  //----------------------------------------------------------------------------
  // wire assignment
  //----------------------------------------------------------------------------
  // 5 unused, 10 used, 1 unused and 4 used signals
  assign gpio_out    = {5'b0, bytestream_direction_fe, bytestream_valid_out_fe, bytestream_data_out_fe, 1'b0, db_state_prc_fe};
  assign gpio_out_en = {5'b0, 1'b1,                    {9 {bytestream_output_enable_fe}},               1'b0, {4 {1'b1}}  };

  assign bytestream_valid_in = gpio_in[13];
  assign bytestream_data_in  = gpio_in[12:5];

  //----------------------------------------------------------------------------
  // version_info
  //----------------------------------------------------------------------------

  // Version metadata, constants come from auto-generated versioning_regs_regmap_utils.vh
  assign version_info = build_component_versions(
    DB_GPIO_IFC_VERSION_LAST_MODIFIED_TIME,
    build_version(
      DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_MAJOR,
      DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_MINOR,
      DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_BUILD),
    build_version(
      DB_GPIO_IFC_CURRENT_VERSION_MAJOR,
      DB_GPIO_IFC_CURRENT_VERSION_MINOR,
      DB_GPIO_IFC_CURRENT_VERSION_BUILD));

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="VERSIONING_REGS_REGMAP">
//  <group name="VERSIONING_CONSTANTS">
//    <enumeratedtype name="DB_GPIO_IFC_VERSION" showhex="true">
//      <info>
//        Daughterboard GPIO interface.{BR/}
//        For guidance on when to update these revision numbers,
//        please refer to the register map documentation accordingly:
//        <li> Current version: @.VERSIONING_REGS_REGMAP..CURRENT_VERSION
//        <li> Oldest compatible version: @.VERSIONING_REGS_REGMAP..OLDEST_COMPATIBLE_VERSION
//        <li> Version last modified: @.VERSIONING_REGS_REGMAP..VERSION_LAST_MODIFIED
//      </info>
//      <value name="DB_GPIO_IFC_CURRENT_VERSION_MAJOR"           integer="1"/>
//      <value name="DB_GPIO_IFC_CURRENT_VERSION_MINOR"           integer="0"/>
//      <value name="DB_GPIO_IFC_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="1"/>
//      <value name="DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_MINOR" integer="0"/>
//      <value name="DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="DB_GPIO_IFC_VERSION_LAST_MODIFIED_TIME"      integer="0x20110616"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//XmlParse xml_off
