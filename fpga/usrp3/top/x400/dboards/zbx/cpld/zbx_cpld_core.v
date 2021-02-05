//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: zbx_cpld_core
//
// Description:
//   Wrapper containing multiple register blocks, each in charge of controlling
//   different features/signals across the daughterboard. Currently, the following
//   blocks supported are:
//     - Scratch/Signature/Revision(Basic Regs) register block
//     - Led Control block
//     - TX/RX path switch control block
//     - TX/RX DSA control block
//     - Local Oscillator SPI control block
//

`default_nettype none

module zbx_cpld_core #(
  parameter [19:0] BASE_ADDRESS = 0
) (
  /////////////////////////////////////////////////////////////////////////////
  // CtrlPort access
  /////////////////////////////////////////////////////////////////////////////
  // Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  // Response
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  //reg clk domain
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,

  /////////////////////////////////////////////////////////////////////////////
  // ATR controls
  /////////////////////////////////////////////////////////////////////////////
  input wire [3:0] atr_fpga_state,

  /////////////////////////////////////////////////////////////////////////////
  //// LO SPI signals
  /////////////////////////////////////////////////////////////////////////////
  // LO SPI for LMX2572
  input  wire [7:0] lo_miso,
  output wire [7:0] lo_csb,

  output wire       lo_sclk,
  output wire       lo_mosi,

  /////////////////////////////////////////////////////////////////////////////
  //// LO SYNC signals
  /////////////////////////////////////////////////////////////////////////////
  // Incoming SYNC
  input wire mb_synth_sync,

  // SYNC for LMX2572
  output wire tx0_lo1_sync,
  output wire tx0_lo2_sync,
  output wire tx1_lo1_sync,
  output wire tx1_lo2_sync,
  output wire rx0_lo1_sync,
  output wire rx0_lo2_sync,
  output wire rx1_lo1_sync,
  output wire rx1_lo2_sync,

  /////////////////////////////////////////////////////////////////////////////
  //// TX0 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Tx0 Switch control
  output wire tx0_sw1_sw2_ctrl,
  output wire tx0_sw3_a,
  output wire tx0_sw3_b,
  output wire tx0_sw4_a,
  output wire tx0_sw4_b,
  output wire tx0_sw5_a,
  output wire tx0_sw5_b,
  output wire tx0_sw6_a,
  output wire tx0_sw6_b,
  output wire tx0_sw7_a,
  output wire tx0_sw7_b,
  output wire tx0_sw8_v1,
  output wire tx0_sw8_v2,
  output wire tx0_sw8_v3,
  output wire tx0_sw9_a,
  output wire tx0_sw9_b,
  output wire tx0_sw10_a,
  output wire tx0_sw10_b,
  output wire tx0_sw11_a,
  output wire tx0_sw11_b,
  output wire tx0_sw13_v1,
  output wire tx0_sw14_v1,

  //Tx0 DSA control
  output wire [6:2] tx0_dsa1,
  output wire [6:2] tx0_dsa2,

  /////////////////////////////////////////////////////////////////////////////
  //// TX1 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Tx1 Switch control
  output wire tx1_sw1_sw2_ctrl,
  output wire tx1_sw3_a,
  output wire tx1_sw3_b,
  output wire tx1_sw4_a,
  output wire tx1_sw4_b,
  output wire tx1_sw5_a,
  output wire tx1_sw5_b,
  output wire tx1_sw6_a,
  output wire tx1_sw6_b,
  output wire tx1_sw7_a,
  output wire tx1_sw7_b,
  output wire tx1_sw8_v1,
  output wire tx1_sw8_v2,
  output wire tx1_sw8_v3,
  output wire tx1_sw9_a,
  output wire tx1_sw9_b,
  output wire tx1_sw10_a,
  output wire tx1_sw10_b,
  output wire tx1_sw11_a,
  output wire tx1_sw11_b,
  output wire tx1_sw13_v1,
  output wire tx1_sw14_v1,

  //Tx1 DSA control
  output wire [6:2] tx1_dsa1,
  output wire [6:2] tx1_dsa2,

  /////////////////////////////////////////////////////////////////////////////
  //// RX0 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Rx0 Switch control
  output wire rx0_sw1_a,
  output wire rx0_sw1_b,
  output wire rx0_sw2_a,
  output wire rx0_sw3_v1,
  output wire rx0_sw3_v2,
  output wire rx0_sw3_v3,
  output wire rx0_sw4_a,
  output wire rx0_sw5_a,
  output wire rx0_sw5_b,
  output wire rx0_sw6_a,
  output wire rx0_sw6_b,
  output wire rx0_sw7_sw8_ctrl,
  output wire rx0_sw9_v1,
  output wire rx0_sw10_v1,
  output wire rx0_sw11_v3,
  output wire rx0_sw11_v2,
  output wire rx0_sw11_v1,

  //Rx0 DSA control
  output wire [1:4] rx0_dsa1_n,
  output wire [1:4] rx0_dsa2_n,
  output wire [1:4] rx0_dsa3_a_n,
  output wire [1:4] rx0_dsa3_b_n,

  /////////////////////////////////////////////////////////////////////////////
  //// RX1 Controls
  /////////////////////////////////////////////////////////////////////////////

  //Rx1 Switch Control
  output wire rx1_sw1_a,
  output wire rx1_sw1_b,
  output wire rx1_sw2_a,
  output wire rx1_sw3_v1,
  output wire rx1_sw3_v2,
  output wire rx1_sw3_v3,
  output wire rx1_sw4_a,
  output wire rx1_sw5_a,
  output wire rx1_sw5_b,
  output wire rx1_sw6_a,
  output wire rx1_sw6_b,
  output wire rx1_sw7_sw8_ctrl,
  output wire rx1_sw9_v1,
  output wire rx1_sw10_v1,
  output wire rx1_sw11_v3,
  output wire rx1_sw11_v2,
  output wire rx1_sw11_v1,

  //Rx1 DSA control
  output wire [1:4] rx1_dsa1_n,
  output wire [1:4] rx1_dsa2_n,
  output wire [1:4] rx1_dsa3_a_n,
  output wire [1:4] rx1_dsa3_b_n,

  /////////////////////////////////////////////////////////////////////////////
  // LED Control
  /////////////////////////////////////////////////////////////////////////////

  output wire ch0_rx2_led,
  output wire ch0_tx_led,
  output wire ch0_rx_led,
  output wire ch1_rx2_led,
  output wire ch1_tx_led,
  output wire ch1_rx_led

);

  `include "regmap/db_control_regmap_utils.vh"
  `include "../../../../../lib/rfnoc/core/ctrlport.vh"

  // internal ATR configuration
  wire [7:0] atr_config_rf0;
  wire [7:0] atr_config_rf1;
  wire [7:0] atr_config_dsa_rf0;
  wire [7:0] atr_config_dsa_rf1;

  // Master Interfaces
  wire [                REGISTER_BLOCKS_SIZE-1:0] ctrlport_req_wr;
  wire [                REGISTER_BLOCKS_SIZE-1:0] ctrlport_req_rd;
  wire [CTRLPORT_ADDR_W*REGISTER_BLOCKS_SIZE-1:0] ctrlport_req_addr;
  wire [CTRLPORT_DATA_W*REGISTER_BLOCKS_SIZE-1:0] ctrlport_req_data;
  wire [                REGISTER_BLOCKS_SIZE-1:0] ctrlport_resp_ack;
  wire [ CTRLPORT_STS_W*REGISTER_BLOCKS_SIZE-1:0] ctrlport_resp_status;
  wire [CTRLPORT_DATA_W*REGISTER_BLOCKS_SIZE-1:0] ctrlport_resp_data;

  ctrlport_splitter #(
    .NUM_SLAVES(REGISTER_BLOCKS_SIZE)
  ) ctrlport_splitter_i (
    .ctrlport_clk             (ctrlport_clk),
    .ctrlport_rst             (ctrlport_rst),
    .s_ctrlport_req_wr        (s_ctrlport_req_wr),
    .s_ctrlport_req_rd        (s_ctrlport_req_rd),
    .s_ctrlport_req_addr      (s_ctrlport_req_addr),
    .s_ctrlport_req_data      (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en   (),
    .s_ctrlport_req_has_time  (),
    .s_ctrlport_req_time      (),
    .s_ctrlport_resp_ack      (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status   (s_ctrlport_resp_status),
    .s_ctrlport_resp_data     (s_ctrlport_resp_data),
    .m_ctrlport_req_wr        (ctrlport_req_wr),
    .m_ctrlport_req_rd        (ctrlport_req_rd),
    .m_ctrlport_req_addr      (ctrlport_req_addr),
    .m_ctrlport_req_data      (ctrlport_req_data),
    .m_ctrlport_req_byte_en   (),
    .m_ctrlport_req_has_time  (),
    .m_ctrlport_req_time      (),
    .m_ctrlport_resp_ack      (ctrlport_resp_ack),
    .m_ctrlport_resp_status   (ctrlport_resp_status),
    .m_ctrlport_resp_data     (ctrlport_resp_data)
  );

  atr_controller #(
    .BASE_ADDRESS  (ATR_CONTROLLER_REGS + BASE_ADDRESS),
    .SIZE_ADDRESS  (ATR_CONTROLLER_REGS_SIZE)
  ) atr_controller_i (
    .ctrlport_clk            (ctrlport_clk),
    .ctrlport_rst            (ctrlport_rst),
    .s_ctrlport_req_wr       (ctrlport_req_wr[ATR_REGISTERS]),
    .s_ctrlport_req_rd       (ctrlport_req_rd[ATR_REGISTERS]),
    .s_ctrlport_req_addr     (ctrlport_req_addr[CTRLPORT_ADDR_W*(ATR_REGISTERS) +: CTRLPORT_ADDR_W]),
    .s_ctrlport_req_data     (ctrlport_req_data[CTRLPORT_DATA_W*(ATR_REGISTERS) +: CTRLPORT_DATA_W]),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack[ATR_REGISTERS]),
    .s_ctrlport_resp_status  (ctrlport_resp_status[CTRLPORT_STS_W*(ATR_REGISTERS) +: CTRLPORT_STS_W]),
    .s_ctrlport_resp_data    (ctrlport_resp_data[CTRLPORT_DATA_W*(ATR_REGISTERS) +: CTRLPORT_DATA_W]),
    .atr_fpga_state          (atr_fpga_state),
    .atr_config_dsa_rf0      (atr_config_dsa_rf0),
    .atr_config_dsa_rf1      (atr_config_dsa_rf1),
    .atr_config_rf0          (atr_config_rf0),
    .atr_config_rf1          (atr_config_rf1)
  );

  switch_control #(
    .BASE_ADDRESS  (SWITCH_SETUP_REGS + BASE_ADDRESS),
    .SIZE_ADDRESS  (SWITCH_SETUP_REGS_SIZE)
  ) switch_control_i (
    .ctrlport_clk            (ctrlport_clk),
    .ctrlport_rst            (ctrlport_rst),
    .s_ctrlport_req_wr       (ctrlport_req_wr[SW_CONTROL]),
    .s_ctrlport_req_rd       (ctrlport_req_rd[SW_CONTROL]),
    .s_ctrlport_req_addr     (ctrlport_req_addr[CTRLPORT_ADDR_W*(SW_CONTROL) +: CTRLPORT_ADDR_W]),
    .s_ctrlport_req_data     (ctrlport_req_data[CTRLPORT_DATA_W*(SW_CONTROL) +: CTRLPORT_DATA_W]),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack[SW_CONTROL]),
    .s_ctrlport_resp_status  (ctrlport_resp_status[CTRLPORT_STS_W*(SW_CONTROL) +: CTRLPORT_STS_W]),
    .s_ctrlport_resp_data    (ctrlport_resp_data[CTRLPORT_DATA_W*(SW_CONTROL) +: CTRLPORT_DATA_W]),
    .atr_config_rf0          (atr_config_rf0),
    .atr_config_rf1          (atr_config_rf1),
    .tx0_sw1_sw2_ctrl        (tx0_sw1_sw2_ctrl),
    .tx0_sw3_a               (tx0_sw3_a),
    .tx0_sw3_b               (tx0_sw3_b),
    .tx0_sw4_a               (tx0_sw4_a),
    .tx0_sw4_b               (tx0_sw4_b),
    .tx0_sw5_a               (tx0_sw5_a),
    .tx0_sw5_b               (tx0_sw5_b),
    .tx0_sw6_a               (tx0_sw6_a),
    .tx0_sw6_b               (tx0_sw6_b),
    .tx0_sw7_a               (tx0_sw7_a),
    .tx0_sw7_b               (tx0_sw7_b),
    .tx0_sw8_v1              (tx0_sw8_v1),
    .tx0_sw8_v2              (tx0_sw8_v2),
    .tx0_sw8_v3              (tx0_sw8_v3),
    .tx0_sw9_a               (tx0_sw9_a),
    .tx0_sw9_b               (tx0_sw9_b),
    .tx0_sw10_a              (tx0_sw10_a),
    .tx0_sw10_b              (tx0_sw10_b),
    .tx0_sw11_a              (tx0_sw11_a),
    .tx0_sw11_b              (tx0_sw11_b),
    .tx0_sw13_v1             (tx0_sw13_v1),
    .tx0_sw14_v1             (tx0_sw14_v1),
    .tx1_sw1_sw2_ctrl        (tx1_sw1_sw2_ctrl),
    .tx1_sw3_a               (tx1_sw3_a),
    .tx1_sw3_b               (tx1_sw3_b),
    .tx1_sw4_a               (tx1_sw4_a),
    .tx1_sw4_b               (tx1_sw4_b),
    .tx1_sw5_a               (tx1_sw5_a),
    .tx1_sw5_b               (tx1_sw5_b),
    .tx1_sw6_a               (tx1_sw6_a),
    .tx1_sw6_b               (tx1_sw6_b),
    .tx1_sw7_a               (tx1_sw7_a),
    .tx1_sw7_b               (tx1_sw7_b),
    .tx1_sw8_v1              (tx1_sw8_v1),
    .tx1_sw8_v2              (tx1_sw8_v2),
    .tx1_sw8_v3              (tx1_sw8_v3),
    .tx1_sw9_a               (tx1_sw9_a),
    .tx1_sw9_b               (tx1_sw9_b),
    .tx1_sw10_a              (tx1_sw10_a),
    .tx1_sw10_b              (tx1_sw10_b),
    .tx1_sw11_a              (tx1_sw11_a),
    .tx1_sw11_b              (tx1_sw11_b),
    .tx1_sw13_v1             (tx1_sw13_v1),
    .tx1_sw14_v1             (tx1_sw14_v1),
    .rx0_sw1_a               (rx0_sw1_a),
    .rx0_sw1_b               (rx0_sw1_b),
    .rx0_sw2_a               (rx0_sw2_a),
    .rx0_sw3_v1              (rx0_sw3_v1),
    .rx0_sw3_v2              (rx0_sw3_v2),
    .rx0_sw3_v3              (rx0_sw3_v3),
    .rx0_sw4_a               (rx0_sw4_a),
    .rx0_sw5_a               (rx0_sw5_a),
    .rx0_sw5_b               (rx0_sw5_b),
    .rx0_sw6_a               (rx0_sw6_a),
    .rx0_sw6_b               (rx0_sw6_b),
    .rx0_sw7_sw8_ctrl        (rx0_sw7_sw8_ctrl),
    .rx0_sw9_v1              (rx0_sw9_v1),
    .rx0_sw10_v1             (rx0_sw10_v1),
    .rx0_sw11_v3             (rx0_sw11_v3),
    .rx0_sw11_v2             (rx0_sw11_v2),
    .rx0_sw11_v1             (rx0_sw11_v1),
    .rx1_sw1_a               (rx1_sw1_a),
    .rx1_sw1_b               (rx1_sw1_b),
    .rx1_sw2_a               (rx1_sw2_a),
    .rx1_sw3_v1              (rx1_sw3_v1),
    .rx1_sw3_v2              (rx1_sw3_v2),
    .rx1_sw3_v3              (rx1_sw3_v3),
    .rx1_sw4_a               (rx1_sw4_a),
    .rx1_sw5_a               (rx1_sw5_a),
    .rx1_sw5_b               (rx1_sw5_b),
    .rx1_sw6_a               (rx1_sw6_a),
    .rx1_sw6_b               (rx1_sw6_b),
    .rx1_sw7_sw8_ctrl        (rx1_sw7_sw8_ctrl),
    .rx1_sw9_v1              (rx1_sw9_v1),
    .rx1_sw10_v1             (rx1_sw10_v1),
    .rx1_sw11_v3             (rx1_sw11_v3),
    .rx1_sw11_v2             (rx1_sw11_v2),
    .rx1_sw11_v1             (rx1_sw11_v1)
  );

  dsa_control #(
    .BASE_ADDRESS  (DSA_SETUP_REGS + BASE_ADDRESS),
    .SIZE_ADDRESS  (DSA_SETUP_REGS_SIZE)
  ) dsa_control_i (
    .ctrlport_clk            (ctrlport_clk),
    .ctrlport_rst            (ctrlport_rst),
    .s_ctrlport_req_wr       (ctrlport_req_wr[DSA_CONTROL]),
    .s_ctrlport_req_rd       (ctrlport_req_rd[DSA_CONTROL]),
    .s_ctrlport_req_addr     (ctrlport_req_addr[CTRLPORT_ADDR_W*(DSA_CONTROL) +:CTRLPORT_ADDR_W]),
    .s_ctrlport_req_data     (ctrlport_req_data[CTRLPORT_DATA_W*(DSA_CONTROL) +:CTRLPORT_DATA_W]),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack[DSA_CONTROL]),
    .s_ctrlport_resp_status  (ctrlport_resp_status[CTRLPORT_STS_W*(DSA_CONTROL) +:CTRLPORT_STS_W]),
    .s_ctrlport_resp_data    (ctrlport_resp_data[CTRLPORT_DATA_W*(DSA_CONTROL) +:CTRLPORT_DATA_W]),
    .atr_config_rf0          (atr_config_dsa_rf0),
    .atr_config_rf1          (atr_config_dsa_rf1),
    .tx0_dsa1                (tx0_dsa1),
    .tx0_dsa2                (tx0_dsa2),
    .tx1_dsa1                (tx1_dsa1),
    .tx1_dsa2                (tx1_dsa2),
    .rx0_dsa1_n              (rx0_dsa1_n),
    .rx0_dsa2_n              (rx0_dsa2_n),
    .rx0_dsa3_a_n            (rx0_dsa3_a_n),
    .rx0_dsa3_b_n            (rx0_dsa3_b_n),
    .rx1_dsa1_n              (rx1_dsa1_n),
    .rx1_dsa2_n              (rx1_dsa2_n),
    .rx1_dsa3_a_n            (rx1_dsa3_a_n),
    .rx1_dsa3_b_n            (rx1_dsa3_b_n)
  );

  led_control #(
    .BASE_ADDRESS  (LED_SETUP_REGS + BASE_ADDRESS),
    .SIZE_ADDRESS  (LED_SETUP_REGS_SIZE)
  ) led_control_i (
    .ctrlport_clk            (ctrlport_clk),
    .ctrlport_rst            (ctrlport_rst),
    .s_ctrlport_req_wr       (ctrlport_req_wr[LED_REGISTERS]),
    .s_ctrlport_req_rd       (ctrlport_req_rd[LED_REGISTERS]),
    .s_ctrlport_req_addr     (ctrlport_req_addr[CTRLPORT_ADDR_W*(LED_REGISTERS) +: CTRLPORT_ADDR_W]),
    .s_ctrlport_req_data     (ctrlport_req_data[CTRLPORT_DATA_W*(LED_REGISTERS) +: CTRLPORT_DATA_W]),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack[LED_REGISTERS]),
    .s_ctrlport_resp_status  (ctrlport_resp_status[CTRLPORT_STS_W*(LED_REGISTERS) +: CTRLPORT_STS_W]),
    .s_ctrlport_resp_data    (ctrlport_resp_data[CTRLPORT_DATA_W*(LED_REGISTERS) +: CTRLPORT_DATA_W]),
    .ch0_rx2_led             (ch0_rx2_led),
    .ch0_tx_led              (ch0_tx_led),
    .ch0_rx_led              (ch0_rx_led),
    .ch1_rx2_led             (ch1_rx2_led),
    .ch1_tx_led              (ch1_tx_led),
    .ch1_rx_led              (ch1_rx_led),
    .atr_config_rf0          (atr_config_rf0),
    .atr_config_rf1          (atr_config_rf1)
  );

  lo_control #(
    .BASE_ADDRESS  (LO_CONTROL_REGS + BASE_ADDRESS),
    .SIZE_ADDRESS  (LO_CONTROL_REGS_SIZE)
  ) lo_control_i (
    .s_ctrlport_req_wr       (ctrlport_req_wr[LO_SPI]),
    .s_ctrlport_req_rd       (ctrlport_req_rd[LO_SPI]),
    .s_ctrlport_req_addr     (ctrlport_req_addr[CTRLPORT_ADDR_W*(LO_SPI) +: CTRLPORT_ADDR_W]),
    .s_ctrlport_req_data     (ctrlport_req_data[CTRLPORT_DATA_W*(LO_SPI) +: CTRLPORT_DATA_W]),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack[LO_SPI]),
    .s_ctrlport_resp_status  (ctrlport_resp_status[CTRLPORT_STS_W*(LO_SPI) +: CTRLPORT_STS_W]),
    .s_ctrlport_resp_data    (ctrlport_resp_data[CTRLPORT_DATA_W*(LO_SPI) +: CTRLPORT_DATA_W]),
    .ctrlport_clk            (ctrlport_clk),
    .ctrlport_rst            (ctrlport_rst),
    .miso                    (lo_miso),
    .ss                      (lo_csb),
    .sclk                    (lo_sclk),
    .mosi                    (lo_mosi),
    .mb_synth_sync           (mb_synth_sync),
    .tx0_lo1_sync            (tx0_lo1_sync),
    .tx0_lo2_sync            (tx0_lo2_sync),
    .tx1_lo1_sync            (tx1_lo1_sync),
    .tx1_lo2_sync            (tx1_lo2_sync),
    .rx0_lo1_sync            (rx0_lo1_sync),
    .rx0_lo2_sync            (rx0_lo2_sync),
    .rx1_lo1_sync            (rx1_lo1_sync),
    .rx1_lo2_sync            (rx1_lo2_sync));

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="DB_CONTROL_REGMAP" readablestrobes="false" generatevhdl="true">
//  <group name="REGISTER_ENDPOINTS" offset="0x000" size="0x000">
//    <enumeratedtype name="REGISTER_BLOCKS">
//      <value name="ATR_REGISTERS"   integer="0"/>
//      <value name="LED_REGISTERS"   integer="1"/>
//      <value name="LO_SPI"          integer="2"/>
//      <value name="SW_CONTROL"      integer="3"/>
//      <value name="DSA_CONTROL"     integer="4"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//XmlParse xml_off
