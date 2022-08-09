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
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  // GPIO interface (domain: pll_ref_clk)
  input  wire [19:0] gpio_in,
  output wire [19:0] gpio_out,
  output wire [19:0] gpio_out_en,

  // Version (Constant)
  output wire [95:0] version_info
);

  `include "../regmap/versioning_regs_regmap_utils.vh"
  `include "../regmap/versioning_utils.vh"

  //----------------------------------------------------------------------------
  // Clock domain crossing (radio_clk -> pll_ref_clk)
  //----------------------------------------------------------------------------
  // Radio_clk is derived from pll_ref_clk by an integer multiplier and
  // originate from the same PLL.
  // Therefore the clock crossing can be achieved by using simple registers.
  // Static timing analysis will be able to meet setup and hold requirements on
  // them.

  // holding read and write flags for multiple radio_clk cycles
  reg         ctrlport_req_wr_hold = 1'b0;
  reg         ctrlport_req_rd_hold = 1'b0;

  reg  [19:0] ctrlport_req_addr_prc = 20'b0;
  reg  [31:0] ctrlport_req_data_prc = 32'b0;
  reg         ctrlport_req_rd_prc   = 1'b0;
  reg         ctrlport_req_wr_prc   = 1'b0;

  wire        ctrlport_resp_ack_prc;
  wire [31:0] ctrlport_resp_data_prc;
  wire [ 1:0] ctrlport_resp_status_prc;

  reg         ctrlport_req_rd_fall      = 1'b0;
  reg         ctrlport_req_wr_fall      = 1'b0;
  reg  [31:0] ctrlport_resp_data_fall   = 32'b0;
  reg  [ 1:0] ctrlport_resp_status_fall = 2'b0;
  reg         ctrlport_resp_ack_fall    = 1'b0;

  // Retime signals to falling edge of radio_clk.
  // Because radio_clk is more heavily loaded than pll_ref_clk, it arrives at
  // the FF's later, which leads to hold time violations when moving signals
  // from pll_ref_clk to radio_clk. By sampling on the falling edge of
  // radio_clk, we provide (nominally) half a radio_clk period of hold, while
  // reducing setup time by half. The late arrival of radio_clk adds back some
  // of the lost setup margin.
  always @(negedge radio_clk) begin
    ctrlport_req_rd_fall      <= ctrlport_req_rd_prc;
    ctrlport_req_wr_fall      <= ctrlport_req_wr_prc;
    ctrlport_resp_ack_fall    <= ctrlport_resp_ack_prc;
    ctrlport_resp_status_fall <= ctrlport_resp_status_prc;
    ctrlport_resp_data_fall   <= ctrlport_resp_data_prc;
  end

  always @(posedge radio_clk) begin
    if (ctrlport_req_wr_fall) begin
      ctrlport_req_wr_hold <= 1'b0;
    end else if (s_ctrlport_req_wr) begin
      ctrlport_req_wr_hold <= 1'b1;
    end
    if (ctrlport_req_rd_fall) begin
      ctrlport_req_rd_hold <= 1'b0;
    end else if (s_ctrlport_req_rd) begin
      ctrlport_req_rd_hold <= 1'b1;
    end

    // capture request address and data
    if (s_ctrlport_req_wr || s_ctrlport_req_rd) begin
      ctrlport_req_addr_prc <= s_ctrlport_req_addr;
      ctrlport_req_data_prc <= s_ctrlport_req_data;
    end
  end

  // capture extended flags in pll_ref_clk domain
  always @(posedge pll_ref_clk) begin
    ctrlport_req_wr_prc <= ctrlport_req_wr_hold;
    ctrlport_req_rd_prc <= ctrlport_req_rd_hold;
  end

  // search for rising edge in response
  reg [1:0] ctrlport_resp_ack_reg = 2'b0;
  always @(posedge radio_clk) begin
    ctrlport_resp_ack_reg = {ctrlport_resp_ack_reg[0], ctrlport_resp_ack_fall};
  end
  assign s_ctrlport_resp_ack = ctrlport_resp_ack_reg[0] & ~ctrlport_resp_ack_reg[1];

  // capture response data
  always @(posedge radio_clk) begin
    if (ctrlport_resp_ack_fall) begin
      s_ctrlport_resp_status <= ctrlport_resp_status_fall;
      s_ctrlport_resp_data   <= ctrlport_resp_data_fall;
    end
  end

  // transfer state lines
  reg [3:0] db_state_prc    = 4'b0;
  reg [3:0] db_state_prc_fe = 4'b0;
  always @(posedge pll_ref_clk) begin
    db_state_prc    <= db_state;
  end
  always @(negedge pll_ref_clk) begin
    db_state_prc_fe <= db_state_prc;
  end

  // transfer reset
  reg ctrlport_rst_hold = 1'b0;
  reg ctrlport_rst_prc  = 1'b0;
  reg ctrlport_rst_fall = 1'b0;
  always @(posedge radio_clk) begin
    if (ctrlport_rst) begin
      ctrlport_rst_hold <= 1'b1;
    end else if (ctrlport_rst_fall) begin
      ctrlport_rst_hold <= 1'b0;
    end
  end
  always @(posedge pll_ref_clk) begin
    ctrlport_rst_prc <= ctrlport_rst_hold;
  end
  always @(negedge radio_clk) begin
    ctrlport_rst_fall <= ctrlport_rst_prc;
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
