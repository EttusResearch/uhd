//
// Copyright 2026 Ettus Research, a National Instruments Brand
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
//   Furthermore there are 4 state logics towards the DB. Ensure an appropriate
//   hold time on the states as the transmission happens in pll_ref_clk, which is
//   slower than radio_clk. Pulses of e.g. just a single clock cycle may not get
//   transferred to the DB.
//
//   The 20 available GPIO lines are assigned with
//   - sync injection clock
//   - 4x empty
//   - bytestream direction
//   - bytestream valid
//   - bytestream data (8 bits)
//   - 3x empty
//   - db_state (2 bits)
//

module db_gpio_interface (
  // Clocks and reset
  input  logic radio_clk,
  input  logic pll_ref_clk,
  input  logic base_ref_clk,
  input  logic pps_refclk,

  // DB state lines (domain: radio_clk)
  input  logic [ 1:0] db_state,
  // Request (domain: radio_clk)
  input  logic        ctrlport_rst,
  input  logic        s_ctrlport_req_wr,
  input  logic        s_ctrlport_req_rd,
  input  logic [19:0] s_ctrlport_req_addr,
  input  logic [31:0] s_ctrlport_req_data,

  // Response (domain: radio_clk)
  output logic        s_ctrlport_resp_ack,
  output logic [ 1:0] s_ctrlport_resp_status,
  output logic [31:0] s_ctrlport_resp_data,

  // GPIO interface (domain: pll_ref_clk)
  input  logic [19:0] gpio_in,
  output logic [19:0] gpio_out,
  output logic [19:0] gpio_out_en,

  // Version (Constant)
  output logic [95:0] version_info
);

  `include "../../regmap/x420/versioning_regs_regmap_utils.vh"
  `include "../../regmap/versioning_utils.vh"

  import ctrlport_pkg::*;
  import XmlSvPkgDB_WINDOW_REGMAP::*;

  //----------------------------------------------------------------------------
  // Clock domain crossing (radio_clk -> pll_ref_clk)
  //----------------------------------------------------------------------------
  // Radio_clk is derived from pll_ref_clk by an integer multiplier and
  // originate from the same PLL.
  // Therefore the clock crossing can be achieved by using simple registers.
  // Static timing analysis will be able to meet setup and hold requirements on
  // them.

  logic         ctrlport_rst_prc;

  logic         ctrlport_req_wr_prc;
  logic         ctrlport_req_rd_prc;
  logic  [19:0] ctrlport_req_addr_prc;
  logic  [31:0] ctrlport_req_data_prc;

  logic        ctrlport_resp_ack_prc;
  logic [ 1:0] ctrlport_resp_status_prc;
  logic [31:0] ctrlport_resp_data_prc;

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
  reg [1:0] db_state_prc    = 2'b0;
  reg [1:0] db_state_prc_fe = 2'b0;
  always @(posedge pll_ref_clk) begin
    db_state_prc    <= db_state;
  end
  always @(negedge pll_ref_clk) begin
    db_state_prc_fe <= db_state_prc;
  end

  //----------------------------------------------------------------------------
  // Split address space
  //----------------------------------------------------------------------------
  ctrlport_if ctrlport_prc_if(.clk(pll_ref_clk), .rst(ctrlport_rst_prc));
  ctrlport_if ctrlport_db(.clk(pll_ref_clk), .rst(ctrlport_rst_prc));
  ctrlport_if ctrlport_fpga(.clk(pll_ref_clk), .rst(ctrlport_rst_prc));

  always_comb begin
    // Map the ControlPort interface to the internal signals
    ctrlport_prc_if.req      = '0;
    ctrlport_prc_if.req.wr   = ctrlport_req_wr_prc;
    ctrlport_prc_if.req.rd   = ctrlport_req_rd_prc;
    ctrlport_prc_if.req.addr = ctrlport_req_addr_prc;
    ctrlport_prc_if.req.data = ctrlport_req_data_prc;

    ctrlport_resp_ack_prc    = ctrlport_prc_if.resp.ack;
    ctrlport_resp_status_prc = ctrlport_prc_if.resp.status;
    ctrlport_resp_data_prc   = ctrlport_prc_if.resp.data;
  end

  ctrlport_if_decoder #(
    .NUM_SLAVES (2),
    .PORT_BASE  ('{kDB_GPIO_WINDOW,     kFPGA_GPIO_WINDOW}),
    .PORT_SIZE  ('{kDB_GPIO_WINDOWSize, kFPGA_GPIO_WINDOWSize})
  ) ctrlport_decoder_inst (
    .s_ctrlport (ctrlport_prc_if.slave),
    .m_ctrlport ('{ctrlport_db.master, ctrlport_fpga.master})
  );


  //----------------------------------------------------------------------------
  // Ctrlport serializer
  //----------------------------------------------------------------------------
  logic [7:0] bytestream_data_in;
  logic [7:0] bytestream_data_out;
  logic       bytestream_direction;
  logic       bytestream_output_enable;
  logic       bytestream_valid_in;
  logic       bytestream_valid_out;

  logic [1:0] ctrlport_db_resp_status;
  assign ctrlport_db.resp.status = ctrlport_status_t'(ctrlport_db_resp_status);

  ctrlport_byte_serializer serializer_i (
    .ctrlport_clk              (pll_ref_clk),
    .ctrlport_rst              (ctrlport_rst_prc),
    .s_ctrlport_req_wr         (ctrlport_db.req.wr),
    .s_ctrlport_req_rd         (ctrlport_db.req.rd),
    .s_ctrlport_req_addr       (ctrlport_db.req.addr),
    .s_ctrlport_req_data       (ctrlport_db.req.data),
    .s_ctrlport_resp_ack       (ctrlport_db.resp.ack),
    .s_ctrlport_resp_status    (ctrlport_db_resp_status),
    .s_ctrlport_resp_data      (ctrlport_db.resp.data),
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
  // sync injection
  //----------------------------------------------------------------------------
  logic half_base_ref_clk;
  hbx_sync_injection sync_injection_i (
    .s_ctrlport(ctrlport_fpga.slave),
    .base_ref_clk(base_ref_clk),
    .pps_brc(pps_refclk),
    .clk_out(half_base_ref_clk)
  );

  //----------------------------------------------------------------------------
  // logic assignment
  //----------------------------------------------------------------------------
  // sync clk, 4 unused, 10 used, 3 unused and 2 used signals
  assign gpio_out    = {half_base_ref_clk,
                        4'b0,
                        bytestream_direction_fe,
                        bytestream_valid_out_fe,
                        bytestream_data_out_fe,
                        3'b0,
                        db_state_prc_fe};
  assign gpio_out_en = {1'b1, // sync clk
                        4'b0, // unused
                        1'b1, // bytestream_direction
                        {9 {bytestream_output_enable_fe}}, // bytestream_valid + data
                        3'b0, // unused
                        {2 {1'b1}}}; //db_state

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
//      <value name="DB_GPIO_IFC_CURRENT_VERSION_MINOR"           integer="1"/>
//      <value name="DB_GPIO_IFC_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="1"/>
//      <value name="DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_MINOR" integer="1"/>
//      <value name="DB_GPIO_IFC_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="DB_GPIO_IFC_VERSION_LAST_MODIFIED_TIME"      integer="0x25060315"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//<regmap name="DB_WINDOW_REGMAP" generatevhdl="false" generateverilog="false">
//  <info>
//    This is a dummy regmap to have a common name for DB specific window to refer to.
//  </info>
//  <group name="DB_WINDOW">
//    <window name="DB_GPIO_WINDOW" offset="0x0" size="0x7000" targetregmap="GPIO_REGMAP"/>
//    <window name="FPGA_GPIO_WINDOW" offset="0x7000" size="0x1000" targetregmap="FPGA_GPIO_REGMAP"/>
//  </group>
//</regmap>
//<regmap name="FPGA_GPIO_REGMAP" generatevhdl="false" generateverilog="false">
//  <info>
//    Content in the GPIO address space which is implemented in the FPGA.
//  </info>
//  <group name="FPGA_GPIO_WINDOW">
//    <window name="SYNC_INJECTION_WINDOW" offset="0x0" size="0x1000" targetregmap="HBX_SYNC_INJECTION_REGMAP"/>
//  </group>
//</regmap>
//XmlParse xml_off
