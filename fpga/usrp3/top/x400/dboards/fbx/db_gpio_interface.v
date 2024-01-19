//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: db_gpio_interface
//
// Description:
//   Interface for GPIO interface towards FBX daughterboards.
//
//
//   The 20 available GPIO lines are assigned with
//   - 12x RF switches(3x per channel)
//   - 2x I2C to Sync switches IO Expander
//   - 6x empty
//

`default_nettype none

module db_gpio_interface #(
  parameter LED_REGISTER_ADDRESS = 0
) (
  // Clocks and reset
  input  wire radio_clk,
  input  wire pll_ref_clk,
  input  wire in_sync_clk,
  output wire out_sync_clk,

  // DB state lines (domain: radio_clk)
  input  wire [ 7:0] db_state,
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

  // ControlPort request to CPLD (domain: radio_clk)
  output wire        m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,
  output wire [ 3:0] m_ctrlport_req_byte_en,

  // ControlPort response from CPLD (domain: radio_clk)
  input wire        m_ctrlport_resp_ack,
  input wire [ 1:0] m_ctrlport_resp_status,
  input wire [31:0] m_ctrlport_resp_data,

  // GPIO interface (domain: pll_ref_clk)
  input  wire [19:0] gpio_in,
  output wire [19:0] gpio_out,
  output wire [19:0] gpio_out_en,

  // Version (Constant)
  output wire [95:0] version_info
);

  `include "../../regmap/x440/versioning_regs_regmap_utils.vh"
  `include "regmap/fbx_ctrl_regmap_utils.vh"
  `include "../../regmap/versioning_utils.vh"

  //----------------------------------------------------------------------------
  // Radio_clk -> pll_ref_clk clock crossing
  //----------------------------------------------------------------------------

  wire         ctrlport_rst_prc;

  wire         ctrlport_req_wr_prc;
  wire         ctrlport_req_rd_prc;
  wire  [19:0] ctrlport_req_addr_prc;
  wire  [31:0] ctrlport_req_data_prc;

  wire        ctrlport_resp_ack_prc;
  wire [ 1:0] ctrlport_resp_status_prc;
  wire [31:0] ctrlport_resp_data_prc;

  ctrlport_clk_cross core_clk_cross_slave (
    .rst                        (ctrlport_rst),
    .s_ctrlport_clk             (radio_clk),
    .s_ctrlport_req_wr          (s_ctrlport_req_wr),
    .s_ctrlport_req_rd          (s_ctrlport_req_rd),
    .s_ctrlport_req_addr        (s_ctrlport_req_addr),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en     (),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status     (s_ctrlport_resp_status),
    .s_ctrlport_resp_data       (s_ctrlport_resp_data),
    .m_ctrlport_clk             (pll_ref_clk),
    .m_ctrlport_req_wr          (ctrlport_req_wr_prc),
    .m_ctrlport_req_rd          (ctrlport_req_rd_prc),
    .m_ctrlport_req_addr        (ctrlport_req_addr_prc),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (ctrlport_req_data_prc),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (ctrlport_resp_ack_prc),
    .m_ctrlport_resp_status     (ctrlport_resp_status_prc),
    .m_ctrlport_resp_data       (ctrlport_resp_data_prc)
  );

  reset_sync reset_sync_prc (
    .clk       (pll_ref_clk),
    .reset_in  (ctrlport_rst),
    .reset_out (ctrlport_rst_prc)
  );

  wire [7:0] db_state_prc;

  handshake #(
    .WIDTH       (8)
  ) synchronizer_db_state (
    .clk_a   (radio_clk),
    .rst_a   (ctrlport_rst),
    .valid_a (1'b1),
    .data_a  (db_state),
    .busy_a  (),
    .clk_b   (pll_ref_clk),
    .valid_b (),
    .data_b  (db_state_prc)
  );

  //----------------------------------------------------------------------------
  // CtrlPort Splitter
  //----------------------------------------------------------------------------

  wire [19:0] rf_atr_ctrlport_req_addr;
  wire [31:0] rf_atr_ctrlport_req_data;
  wire        rf_atr_ctrlport_req_rd;
  wire        rf_atr_ctrlport_req_wr;
  wire        rf_atr_ctrlport_resp_ack;
  wire [31:0] rf_atr_ctrlport_resp_data;
  wire [ 1:0] rf_atr_ctrlport_resp_status;

  wire [19:0] sync_ctrlport_req_addr;
  wire [31:0] sync_ctrlport_req_data;
  wire        sync_ctrlport_req_rd;
  wire        sync_ctrlport_req_wr;
  wire        sync_ctrlport_resp_ack;
  wire [31:0] sync_ctrlport_resp_data;
  wire [ 1:0] sync_ctrlport_resp_status;

  wire [19:0] led_atr_ctrlport_req_addr;
  wire [31:0] led_atr_ctrlport_req_data;
  wire        led_atr_ctrlport_req_rd;
  wire        led_atr_ctrlport_req_wr;
  wire        led_atr_ctrlport_resp_ack;
  wire [31:0] led_atr_ctrlport_resp_data;
  wire [ 1:0] led_atr_ctrlport_resp_status;

  wire [19:0] clk_en_ctrlport_req_addr;
  wire [31:0] clk_en_ctrlport_req_data;
  wire        clk_en_ctrlport_req_rd;
  wire        clk_en_ctrlport_req_wr;
  wire        clk_en_ctrlport_resp_ack;
  wire [31:0] clk_en_ctrlport_resp_data;
  wire [ 1:0] clk_en_ctrlport_resp_status;

  ctrlport_splitter #(
    .NUM_SLAVES (4)
  ) fbx_ctrlport_splitter (
    .ctrlport_clk            (pll_ref_clk),
    .ctrlport_rst            (ctrlport_rst_prc),
    .s_ctrlport_req_wr       (ctrlport_req_wr_prc),
    .s_ctrlport_req_rd       (ctrlport_req_rd_prc),
    .s_ctrlport_req_addr     (ctrlport_req_addr_prc),
    .s_ctrlport_req_data     (ctrlport_req_data_prc),
    .s_ctrlport_req_byte_en  (4'hF),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'b0),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack_prc),
    .s_ctrlport_resp_status  (ctrlport_resp_status_prc),
    .s_ctrlport_resp_data    (ctrlport_resp_data_prc),
    .m_ctrlport_req_wr       ({ clk_en_ctrlport_req_wr,      led_atr_ctrlport_req_wr,      sync_ctrlport_req_wr,      rf_atr_ctrlport_req_wr      }),
    .m_ctrlport_req_rd       ({ clk_en_ctrlport_req_rd,      led_atr_ctrlport_req_rd,      sync_ctrlport_req_rd,      rf_atr_ctrlport_req_rd      }),
    .m_ctrlport_req_addr     ({ clk_en_ctrlport_req_addr,    led_atr_ctrlport_req_addr,    sync_ctrlport_req_addr,    rf_atr_ctrlport_req_addr    }),
    .m_ctrlport_req_data     ({ clk_en_ctrlport_req_data,    led_atr_ctrlport_req_data,    sync_ctrlport_req_data,    rf_atr_ctrlport_req_data    }),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({ clk_en_ctrlport_resp_ack,    led_atr_ctrlport_resp_ack,    sync_ctrlport_resp_ack,    rf_atr_ctrlport_resp_ack    }),
    .m_ctrlport_resp_status  ({ clk_en_ctrlport_resp_status, led_atr_ctrlport_resp_status, sync_ctrlport_resp_status, rf_atr_ctrlport_resp_status }),
    .m_ctrlport_resp_data    ({ clk_en_ctrlport_resp_data,   led_atr_ctrlport_resp_data,   sync_ctrlport_resp_data,   rf_atr_ctrlport_resp_data   })
  );

  //----------------------------------------------------------------------------
  // Switch Control
  //----------------------------------------------------------------------------

  wire rf0_tx_rx_rfs, rf0_rx_rfs, rf0_tdds;
  wire rf1_tx_rx_rfs, rf1_rx_rfs, rf1_tdds;
  wire rf2_tx_rx_rfs, rf2_rx_rfs, rf2_tdds;
  wire rf3_tx_rx_rfs, rf3_rx_rfs, rf3_tdds;

  rf_atr_control #(
    .REG_BASE (RF_ATR_REGS),
    .REG_SIZE (RF_ATR_REGS_SIZE)
  ) rf_atr_control_i (
    .ctrlport_clk              (pll_ref_clk),
    .ctrlport_rst              (ctrlport_rst_prc),
    .s_ctrlport_req_wr         (rf_atr_ctrlport_req_wr),
    .s_ctrlport_req_rd         (rf_atr_ctrlport_req_rd),
    .s_ctrlport_req_addr       (rf_atr_ctrlport_req_addr),
    .s_ctrlport_req_data       (rf_atr_ctrlport_req_data),
    .s_ctrlport_resp_ack       (rf_atr_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (rf_atr_ctrlport_resp_status),
    .s_ctrlport_resp_data      (rf_atr_ctrlport_resp_data),
    .db_state                  (db_state_prc),
    .rf0_tx_rx_rfs             (rf0_tx_rx_rfs),
    .rf0_rx_rfs                (rf0_rx_rfs),
    .rf0_tdds                  (rf0_tdds),
    .rf1_tx_rx_rfs             (rf1_tx_rx_rfs),
    .rf1_rx_rfs                (rf1_rx_rfs),
    .rf1_tdds                  (rf1_tdds),
    .rf2_tx_rx_rfs             (rf2_tx_rx_rfs),
    .rf2_rx_rfs                (rf2_rx_rfs),
    .rf2_tdds                  (rf2_tdds),
    .rf3_tx_rx_rfs             (rf3_tx_rx_rfs),
    .rf3_rx_rfs                (rf3_rx_rfs),
    .rf3_tdds                  (rf3_tdds)
  );

  //----------------------------------------------------------------------------
  // LED Control
  //----------------------------------------------------------------------------

  wire         cpld_ctrlport_req_wr;
  wire         cpld_ctrlport_req_rd;
  wire  [19:0] cpld_ctrlport_req_addr;
  wire  [31:0] cpld_ctrlport_req_data;
  wire  [ 3:0] cpld_ctrlport_req_byte_en;

  wire        cpld_ctrlport_resp_ack;
  wire [ 1:0] cpld_ctrlport_resp_status;
  wire [31:0] cpld_ctrlport_resp_data;


  led_atr_control #(
    .LED_REGISTER_ADDRESS (LED_REGISTER_ADDRESS),
    .REG_BASE             (LED_ATR_REGS),
    .REG_SIZE             (LED_ATR_REGS_SIZE)
  ) led_atr_control_i (
    .ctrlport_clk              (pll_ref_clk),
    .ctrlport_rst              (ctrlport_rst_prc),
    .s_ctrlport_req_wr         (led_atr_ctrlport_req_wr),
    .s_ctrlport_req_rd         (led_atr_ctrlport_req_rd),
    .s_ctrlport_req_addr       (led_atr_ctrlport_req_addr),
    .s_ctrlport_req_data       (led_atr_ctrlport_req_data),
    .s_ctrlport_resp_ack       (led_atr_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (led_atr_ctrlport_resp_status),
    .s_ctrlport_resp_data      (led_atr_ctrlport_resp_data),
    .db_state                  (db_state_prc),
    .m_ctrlport_req_wr         (cpld_ctrlport_req_wr),
    .m_ctrlport_req_rd         (cpld_ctrlport_req_rd),
    .m_ctrlport_req_addr       (cpld_ctrlport_req_addr),
    .m_ctrlport_req_data       (cpld_ctrlport_req_data),
    .m_ctrlport_req_byte_en    (cpld_ctrlport_req_byte_en),
    .m_ctrlport_resp_ack       (cpld_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (cpld_ctrlport_resp_status),
    .m_ctrlport_resp_data      (cpld_ctrlport_resp_data)
  );

  ctrlport_clk_cross core_clk_cross_cpld_master (
    .rst                        (ctrlport_rst_prc),
    .s_ctrlport_clk             (pll_ref_clk),
    .s_ctrlport_req_wr          (cpld_ctrlport_req_wr),
    .s_ctrlport_req_rd          (cpld_ctrlport_req_rd),
    .s_ctrlport_req_addr        (cpld_ctrlport_req_addr),
    .s_ctrlport_req_portid      (),
    .s_ctrlport_req_rem_epid    (),
    .s_ctrlport_req_rem_portid  (),
    .s_ctrlport_req_data        (cpld_ctrlport_req_data),
    .s_ctrlport_req_byte_en     (cpld_ctrlport_req_byte_en),
    .s_ctrlport_req_has_time    (),
    .s_ctrlport_req_time        (),
    .s_ctrlport_resp_ack        (cpld_ctrlport_resp_ack),
    .s_ctrlport_resp_status     (cpld_ctrlport_resp_status),
    .s_ctrlport_resp_data       (cpld_ctrlport_resp_data),
    .m_ctrlport_clk             (radio_clk),
    .m_ctrlport_req_wr          (m_ctrlport_req_wr),
    .m_ctrlport_req_rd          (m_ctrlport_req_rd),
    .m_ctrlport_req_addr        (m_ctrlport_req_addr),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en     (m_ctrlport_req_byte_en),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status     (m_ctrlport_resp_status),
    .m_ctrlport_resp_data       (m_ctrlport_resp_data)
  );

  //----------------------------------------------------------------------------
  // I2C Control
  //----------------------------------------------------------------------------

  wire i2c_scl_i, i2c_scl_o, i2c_scl_en_o;
  wire i2c_sda_i, i2c_sda_o, i2c_sda_en_o;

  ctrlport_to_i2c_sync_ctrl #(
    .BASE_ADDRESS(RF_SYNC_REGS),
    .PRESCALE(1000)
  ) ctrlport_to_i2c_inst (
    .ctrlport_clk              (pll_ref_clk),
    .ctrlport_rst              (ctrlport_rst_prc),
    .s_ctrlport_req_wr         (sync_ctrlport_req_wr),
    .s_ctrlport_req_rd         (sync_ctrlport_req_rd),
    .s_ctrlport_req_addr       (sync_ctrlport_req_addr),
    .s_ctrlport_req_data       (sync_ctrlport_req_data),
    .s_ctrlport_resp_ack       (sync_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (sync_ctrlport_resp_status),
    .s_ctrlport_resp_data      (sync_ctrlport_resp_data),
    .scl_pad_i                 (i2c_scl_i),
    .scl_pad_o                 (i2c_scl_o),
    .scl_pad_en_o              (i2c_scl_en_o),
    .sda_pad_i                 (i2c_sda_i),
    .sda_pad_o                 (i2c_sda_o),
    .sda_pad_en_o              (i2c_sda_en_o)
  );

  //----------------------------------------------------------------------------
  // Sync Clock Enable
  //----------------------------------------------------------------------------

  clock_en_control #(
    .BASE_ADDRESS(SYNC_CLK_EN_REGS)
  ) clock_en_control_inst (
    .ctrlport_clk              (pll_ref_clk),
    .ctrlport_rst              (ctrlport_rst_prc),
    .s_ctrlport_req_wr         (clk_en_ctrlport_req_wr),
    .s_ctrlport_req_rd         (clk_en_ctrlport_req_rd),
    .s_ctrlport_req_addr       (clk_en_ctrlport_req_addr),
    .s_ctrlport_req_data       (clk_en_ctrlport_req_data),
    .s_ctrlport_resp_ack       (clk_en_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (clk_en_ctrlport_resp_status),
    .s_ctrlport_resp_data      (clk_en_ctrlport_resp_data),
    .clk_in                    (in_sync_clk),
    .clk_out                   (out_sync_clk)
  );

  //----------------------------------------------------------------------------
  // wire assignment
  //----------------------------------------------------------------------------
  // RFS1, RFS2, RFS3, RFS4, RFS5, RFS6, RFS7, RFS8, TDDS1, TDDS2, TDDS3, TDDS4,
  // 2 unused, 2 for I2C, 4 unused.

  assign i2c_sda_i = gpio_in[4];
  assign i2c_scl_i = gpio_in[5];

  assign gpio_out    = {rf0_tx_rx_rfs, rf0_rx_rfs,
                        rf1_tx_rx_rfs, rf1_rx_rfs,
                        rf2_tx_rx_rfs, rf2_rx_rfs,
                        rf3_tx_rx_rfs, rf3_rx_rfs,
                        rf0_tdds, rf1_tdds,
                        rf2_tdds, rf3_tdds,
                        2'b0,   // Unused
                        i2c_scl_o, i2c_sda_o,   // I2C
                        4'b0    // Unused
  };

  assign gpio_out_en = {12'hFFF,  // Switches
                        2'b0,   // Unused
                        ~i2c_scl_en_o, ~i2c_sda_en_o, // I2C enables tri-state when HIGH
                        4'b0    // Unused
  };
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
//      <value name="DB_GPIO_IFC_VERSION_LAST_MODIFIED_TIME"      integer="0x22070116"/>
//    </enumeratedtype>
//  </group>
//</regmap>

//<regmap name="FBX_CTRL_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This map contains register windows for controlling the different sources
//    that drive the state of FBX control lines.
//  </info>
//  <group name="FBX_CONTROLS">
//    <window name="RF_ATR_REGS"     offset="0x0"    size="0x2000" targetregmap="RF_ATR_REGMAP">
//      <info>Control RF switches in FBX daughterboard based on the ATR state of the accessed radio</info>
//    </window>
//    <window name="RF_SYNC_REGS"     offset="0x2000"    size="0x2000" targetregmap="RF_SYNC_REGMAP">
//      <info>I2C interface to FBX daughterboard IO Expander that controls SYNC switches and rfs en</info>
//    </window>
//    <window name="LED_ATR_REGS"     offset="0x4000"    size="0x2000" targetregmap="LED_ATR_REGMAP">
//      <info>Control TX/RX LEDs based on the ATR state of the accessed radio</info>
//    </window>
//    <window name="SYNC_CLK_EN_REGS"  offset="0x6000"    size="0x2000" targetregmap="CLK_EN_REGMAP">
//      <info>Clock enable register for SYNC INJECT 5 MHz clock</info>
//    </window>
//  </group>
//</regmap>
//XmlParse xml_off
