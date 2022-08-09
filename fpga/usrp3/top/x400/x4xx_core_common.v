//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_core_common
//
// Description:
//
//   This module contains the common core infrastructure for RFNoC, such as the
//   motherboard registers and timekeeper, as well as distribution of the
//   CtrlPort buses from each radio block.
//   This module contains blocks that respond to the AXI ctrlport interface,
//   the ctrlport interfaces in the radios, or a mix of both. A visual
//   representation of how the AXI Ctrlport interface and the
//   Ctrlport of each radio interact with the different blocks in this module
//   can be found in ./doc/x4xx_core_common_buses.svg
//
// Parameters:
//
//   CHDR_CLK_RATE  : Rate of rfnoc_chdr_clk in Hz
//   CHDR_W         : CHDR protocol width
//   RFNOC_PROTOVER : RFNoC protocol version (major in most-significant byte,
//                    Minor is least significant byte)
//   NUM_DBOARDS    : Number of daughter boards to support
//   PCIE_PRESENT   : Indicates if PCIe is present in this image
//

`default_nettype none


module x4xx_core_common #(
  parameter CHDR_CLK_RATE  = 200000000,
  parameter CHDR_W         = 64,
  parameter RFNOC_PROTOVER = {8'd1, 8'd0},
  parameter NUM_DBOARDS    = 2,
  parameter PCIE_PRESENT   = 0
) (
  // Clocks and resets
  input wire radio_clk,
  input wire radio_clk_2x,
  input wire radio_rst,

  input wire rfnoc_chdr_clk,
  input wire rfnoc_chdr_rst,

  input wire rfnoc_ctrl_clk,
  input wire rfnoc_ctrl_rst,

  // Ctrlport master interface from AXI
  input  wire        ctrlport_rst,
  input  wire        ctrlport_clk,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [ 9:0] s_ctrlport_req_portid,
  input  wire [15:0] s_ctrlport_req_rem_epid,
  input  wire [ 9:0] s_ctrlport_req_rem_portid,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire [ 3:0] s_ctrlport_req_byte_en,
  input  wire        s_ctrlport_req_has_time,
  input  wire [63:0] s_ctrlport_req_time,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  // PPS (top-level inputs)
  input wire pps_radioclk,

  // PPS and clock control (Domain: rfnoc_ctrl_clk)
  output wire [ 1:0] pps_select,
  output wire [ 1:0] trig_io_select,
  output wire        pll_sync_trigger,
  output wire [ 7:0] pll_sync_delay,
  input  wire        pll_sync_done,
  output wire [ 7:0] pps_brc_delay,
  output wire [25:0] pps_prc_delay,
  output wire [ 1:0] prc_rc_divider,
  output wire        pps_rc_enabled,

  // Timekeeper (Domain: radio_clk)
  input  wire [ 7:0] radio_spc,
  output wire [63:0] radio_time,
  input  wire        sample_rx_stb,
  input  wire [ 3:0] time_ignore_bits,

  // GPIO to DIO board (Domain: radio_clk)
  output wire [11:0] gpio_en_a,
  output wire [11:0] gpio_en_b,
  // GPIO to DIO board (async)
  input  wire [11:0] gpio_in_a,
  input  wire [11:0] gpio_in_b,
  output wire [11:0] gpio_out_a,
  output wire [11:0] gpio_out_b,

  // GPIO to application (Domain: async)
  output wire [11:0] gpio_in_fabric_a,
  output wire [11:0] gpio_in_fabric_b,
  input  wire [11:0] gpio_out_fabric_a,
  input  wire [11:0] gpio_out_fabric_b,

  // PS GPIO Control
  input  wire [11:0] ps_gpio_out_a,
  output wire [11:0] ps_gpio_in_a,
  input  wire [11:0] ps_gpio_ddr_a,
  input  wire [11:0] ps_gpio_out_b,
  output wire [11:0] ps_gpio_in_b,
  input  wire [11:0] ps_gpio_ddr_b,

  // CtrlPort Slave (from RFNoC Radio Blocks; Domain: radio_clk)
  input  wire [  1*NUM_DBOARDS-1:0] s_radio_ctrlport_req_wr,
  input  wire [  1*NUM_DBOARDS-1:0] s_radio_ctrlport_req_rd,
  input  wire [ 20*NUM_DBOARDS-1:0] s_radio_ctrlport_req_addr,
  input  wire [ 32*NUM_DBOARDS-1:0] s_radio_ctrlport_req_data,
  input  wire [  4*NUM_DBOARDS-1:0] s_radio_ctrlport_req_byte_en,
  input  wire [  1*NUM_DBOARDS-1:0] s_radio_ctrlport_req_has_time,
  input  wire [ 64*NUM_DBOARDS-1:0] s_radio_ctrlport_req_time,
  output wire [  1*NUM_DBOARDS-1:0] s_radio_ctrlport_resp_ack,
  output wire [  2*NUM_DBOARDS-1:0] s_radio_ctrlport_resp_status,
  output wire [ 32*NUM_DBOARDS-1:0] s_radio_ctrlport_resp_data,

  // CtrlPort Master (to Daughter Boards, Domain: radio_clk)
  output wire [  1*NUM_DBOARDS-1:0] m_radio_ctrlport_req_wr,
  output wire [  1*NUM_DBOARDS-1:0] m_radio_ctrlport_req_rd,
  output wire [ 20*NUM_DBOARDS-1:0] m_radio_ctrlport_req_addr,
  output wire [ 32*NUM_DBOARDS-1:0] m_radio_ctrlport_req_data,
  input  wire [  1*NUM_DBOARDS-1:0] m_radio_ctrlport_resp_ack,
  input  wire [  2*NUM_DBOARDS-1:0] m_radio_ctrlport_resp_status,
  input  wire [ 32*NUM_DBOARDS-1:0] m_radio_ctrlport_resp_data,

  // RF Reset Control
  output wire start_nco_reset,
  input  wire nco_reset_done,
  output wire adc_reset_pulse,
  output wire dac_reset_pulse,

  // Radio state for ATR control
  input wire [NUM_DBOARDS*2-1:0] tx_running,
  input wire [NUM_DBOARDS*2-1:0] rx_running,

  // Misc (Domain: rfnoc_ctrl_clk)
  input  wire [31:0] qsfp_port_0_0_info,
  input  wire [31:0] qsfp_port_0_1_info,
  input  wire [31:0] qsfp_port_0_2_info,
  input  wire [31:0] qsfp_port_0_3_info,
  input  wire [31:0] qsfp_port_1_0_info,
  input  wire [31:0] qsfp_port_1_1_info,
  input  wire [31:0] qsfp_port_1_2_info,
  input  wire [31:0] qsfp_port_1_3_info,
  output wire [15:0] device_id,
  output wire        mfg_test_en_fabric_clk,
  output wire        mfg_test_en_gty_rcv_clk,
  input  wire        fpga_aux_ref,

  // Version (Constant)
  // Each component consists of a 96-bit vector (refer to versioning_utils.vh)
  input wire [64*96-1:0] version_info

);

  `include "regmap/radio_ctrlport_regmap_utils.vh"
  `include "../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/core_regs_regmap_utils.vh"
  `include "regmap/radio_dio_regmap_utils.vh"

  //---------------------------------------------------------------------------
  // AXI4-Lite to ctrlport
  //---------------------------------------------------------------------------

  // Ctrlport master interface (domain: rfnoc_ctrl_clk)
  wire                       m_req_wr;
  wire                       m_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] m_req_addr;
  wire [CTRLPORT_DATA_W-1:0] m_req_data;
  wire                       m_resp_ack;
  wire [ CTRLPORT_STS_W-1:0] m_resp_status;
  wire [CTRLPORT_DATA_W-1:0] m_resp_data;

  // Split ctrlport for multiple endpoints (domain: rfnoc_ctrl_clk)
  wire                       timekeeper_req_wr,      versioning_req_wr,      global_regs_req_wr,      dio_req_wr;
  wire                       timekeeper_req_rd,      versioning_req_rd,      global_regs_req_rd,      dio_req_rd;
  wire [CTRLPORT_ADDR_W-1:0] timekeeper_req_addr,    versioning_req_addr,    global_regs_req_addr,    dio_req_addr;
  wire [CTRLPORT_DATA_W-1:0] timekeeper_req_data,    versioning_req_data,    global_regs_req_data,    dio_req_data;
  wire                       timekeeper_resp_ack,    versioning_resp_ack,    global_regs_resp_ack,    dio_resp_ack;
  wire [ CTRLPORT_STS_W-1:0] timekeeper_resp_status, versioning_resp_status, global_regs_resp_status, dio_resp_status;
  wire [CTRLPORT_DATA_W-1:0] timekeeper_resp_data,   versioning_resp_data,   global_regs_resp_data,   dio_resp_data;

  ctrlport_clk_cross ctrlport_clk_cross_i (
    .rst                       (ctrlport_rst),
    .s_ctrlport_clk            (ctrlport_clk),
    .s_ctrlport_req_wr         (s_ctrlport_req_wr),
    .s_ctrlport_req_rd         (s_ctrlport_req_rd),
    .s_ctrlport_req_addr       (s_ctrlport_req_addr),
    .s_ctrlport_req_portid     (s_ctrlport_req_portid),
    .s_ctrlport_req_rem_epid   (s_ctrlport_req_rem_epid),
    .s_ctrlport_req_rem_portid (s_ctrlport_req_rem_portid),
    .s_ctrlport_req_data       (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en    (s_ctrlport_req_byte_en),
    .s_ctrlport_req_has_time   (s_ctrlport_req_has_time),
    .s_ctrlport_req_time       (s_ctrlport_req_time),
    .s_ctrlport_resp_ack       (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (s_ctrlport_resp_status),
    .s_ctrlport_resp_data      (s_ctrlport_resp_data),
    .m_ctrlport_clk            (rfnoc_ctrl_clk),
    .m_ctrlport_req_wr         (m_req_wr),
    .m_ctrlport_req_rd         (m_req_rd),
    .m_ctrlport_req_addr       (m_req_addr),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (m_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (m_resp_ack),
    .m_ctrlport_resp_status    (m_resp_status),
    .m_ctrlport_resp_data      (m_resp_data)
  );

  ctrlport_splitter #(
    .NUM_SLAVES (4)
  ) ctrlport_splitter_i (
    .ctrlport_clk            (rfnoc_ctrl_clk),
    .ctrlport_rst            (rfnoc_ctrl_rst),
    .s_ctrlport_req_wr       (m_req_wr),
    .s_ctrlport_req_rd       (m_req_rd),
    .s_ctrlport_req_addr     (m_req_addr),
    .s_ctrlport_req_data     (m_req_data),
    .s_ctrlport_req_byte_en  (4'hF),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'h0),
    .s_ctrlport_resp_ack     (m_resp_ack),
    .s_ctrlport_resp_status  (m_resp_status),
    .s_ctrlport_resp_data    (m_resp_data),
    .m_ctrlport_req_wr       ({timekeeper_req_wr,      versioning_req_wr,      global_regs_req_wr,      dio_req_wr}),
    .m_ctrlport_req_rd       ({timekeeper_req_rd,      versioning_req_rd,      global_regs_req_rd,      dio_req_rd}),
    .m_ctrlport_req_addr     ({timekeeper_req_addr,    versioning_req_addr,    global_regs_req_addr,    dio_req_addr}),
    .m_ctrlport_req_data     ({timekeeper_req_data,    versioning_req_data,    global_regs_req_data,    dio_req_data}),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({timekeeper_resp_ack,    versioning_resp_ack,    global_regs_resp_ack,    dio_resp_ack}),
    .m_ctrlport_resp_status  ({timekeeper_resp_status, versioning_resp_status, global_regs_resp_status, dio_resp_status}),
    .m_ctrlport_resp_data    ({timekeeper_resp_data,   versioning_resp_data,   global_regs_resp_data,   dio_resp_data})
  );


  //--------------------------------------------------------------------
  // Global Registers
  // -------------------------------------------------------------------

  localparam NUM_TIMEKEEPERS = 1;

  x4xx_global_regs #(
    .REG_BASE        (GLOBAL_REGS),
    .REG_SIZE        (GLOBAL_REGS_SIZE),
    .CHDR_CLK_RATE   (CHDR_CLK_RATE),
    .CHDR_W          (CHDR_W),
    .RFNOC_PROTOVER  (RFNOC_PROTOVER),
    .NUM_TIMEKEEPERS (NUM_TIMEKEEPERS),
    .PCIE_PRESENT    (PCIE_PRESENT)
  ) x4xx_global_regs_i (
    .s_ctrlport_clk          (rfnoc_ctrl_clk),
    .s_ctrlport_rst          (rfnoc_ctrl_rst),
    .s_ctrlport_req_wr       (global_regs_req_wr),
    .s_ctrlport_req_rd       (global_regs_req_rd),
    .s_ctrlport_req_addr     (global_regs_req_addr),
    .s_ctrlport_req_data     (global_regs_req_data),
    .s_ctrlport_resp_ack     (global_regs_resp_ack),
    .s_ctrlport_resp_status  (global_regs_resp_status),
    .s_ctrlport_resp_data    (global_regs_resp_data),
    .rfnoc_chdr_clk          (rfnoc_chdr_clk),
    .rfnoc_chdr_rst          (rfnoc_chdr_rst),
    .pps_select              (pps_select),
    .trig_io_select          (trig_io_select),
    .pll_sync_trigger        (pll_sync_trigger),
    .pll_sync_delay          (pll_sync_delay),
    .pll_sync_done           (pll_sync_done),
    .pps_brc_delay           (pps_brc_delay),
    .pps_prc_delay           (pps_prc_delay),
    .prc_rc_divider          (prc_rc_divider),
    .pps_rc_enabled          (pps_rc_enabled),
    .qsfp_port_0_0_info      (qsfp_port_0_0_info),
    .qsfp_port_0_1_info      (qsfp_port_0_1_info),
    .qsfp_port_0_2_info      (qsfp_port_0_2_info),
    .qsfp_port_0_3_info      (qsfp_port_0_3_info),
    .qsfp_port_1_0_info      (qsfp_port_1_0_info),
    .qsfp_port_1_1_info      (qsfp_port_1_1_info),
    .qsfp_port_1_2_info      (qsfp_port_1_2_info),
    .qsfp_port_1_3_info      (qsfp_port_1_3_info),
    .mfg_test_en_fabric_clk  (mfg_test_en_fabric_clk),
    .mfg_test_en_gty_rcv_clk (mfg_test_en_gty_rcv_clk),
    .fpga_aux_ref            (fpga_aux_ref),
    .device_id               (device_id)
  );


  //--------------------------------------------------------------------
  // Version Registers
  // -------------------------------------------------------------------

  x4xx_versioning_regs #(
    .REG_BASE (VERSIONING_REGS)
  ) x4xx_versioning_regs_i (
    .s_ctrlport_clk         (rfnoc_ctrl_clk),
    .s_ctrlport_rst         (rfnoc_ctrl_rst),
    .s_ctrlport_req_wr      (versioning_req_wr),
    .s_ctrlport_req_rd      (versioning_req_rd),
    .s_ctrlport_req_addr    (versioning_req_addr),
    .s_ctrlport_req_data    (versioning_req_data),
    .s_ctrlport_resp_ack    (versioning_resp_ack),
    .s_ctrlport_resp_status (versioning_resp_status),
    .s_ctrlport_resp_data   (versioning_resp_data),
    .version_info           (version_info)
  );


  //---------------------------------------------------------------------------
  // Timekeeper
  //---------------------------------------------------------------------------

  assign timekeeper_resp_status = CTRL_STS_OKAY;

  timekeeper #(
    .BASE_ADDR      (TIMEKEEPER),
    .TIME_INCREMENT (0)
  ) timekeeper_i (
    .tb_clk                (radio_clk),
    .tb_rst                (radio_rst),
    .s_ctrlport_clk        (rfnoc_ctrl_clk),
    .s_ctrlport_req_wr     (timekeeper_req_wr),
    .s_ctrlport_req_rd     (timekeeper_req_rd),
    .s_ctrlport_req_addr   (timekeeper_req_addr),
    .s_ctrlport_req_data   (timekeeper_req_data),
    .s_ctrlport_resp_ack   (timekeeper_resp_ack),
    .s_ctrlport_resp_data  (timekeeper_resp_data),
    .time_increment        (radio_spc),
    .sample_rx_stb         (sample_rx_stb),
    .pps                   (pps_radioclk),
    .tb_timestamp          (radio_time),
    .tb_timestamp_last_pps (),
    .tb_period_ns_q32      ()
  );


  //-----------------------------------------------------------------------
  // Radio CtrlPort Splitter
  //-----------------------------------------------------------------------

  // Radio CtrlPort endpoints
  wire [  1*NUM_DBOARDS-1:0] rf_ctrlport_req_wr;
  wire [  1*NUM_DBOARDS-1:0] rf_ctrlport_req_rd;
  wire [ 20*NUM_DBOARDS-1:0] rf_ctrlport_req_addr;
  wire [ 32*NUM_DBOARDS-1:0] rf_ctrlport_req_data;
  wire [  1*NUM_DBOARDS-1:0] rf_ctrlport_resp_ack;
  wire [  2*NUM_DBOARDS-1:0] rf_ctrlport_resp_status;
  wire [ 32*NUM_DBOARDS-1:0] rf_ctrlport_resp_data;

  wire [  1*NUM_DBOARDS-1:0] radio_dio_req_wr;
  wire [  1*NUM_DBOARDS-1:0] radio_dio_req_rd;
  wire [ 20*NUM_DBOARDS-1:0] radio_dio_req_addr;
  wire [ 32*NUM_DBOARDS-1:0] radio_dio_req_data;
  wire [  1*NUM_DBOARDS-1:0] radio_dio_resp_ack;
  wire [  2*NUM_DBOARDS-1:0] radio_dio_resp_status;
  wire [ 32*NUM_DBOARDS-1:0] radio_dio_resp_data;

  wire [  1*NUM_DBOARDS-1:0] gpio_atr_ctrlport_req_wr;
  wire [  1*NUM_DBOARDS-1:0] gpio_atr_ctrlport_req_rd;
  wire [ 20*NUM_DBOARDS-1:0] gpio_atr_ctrlport_req_addr;
  wire [ 32*NUM_DBOARDS-1:0] gpio_atr_ctrlport_req_data;
  wire [  1*NUM_DBOARDS-1:0] gpio_atr_ctrlport_resp_ack;
  wire [  2*NUM_DBOARDS-1:0] gpio_atr_ctrlport_resp_status;
  wire [ 32*NUM_DBOARDS-1:0] gpio_atr_ctrlport_resp_data;

  wire [  1*NUM_DBOARDS-1:0] gpio_spi_ctrlport_req_wr;
  wire [  1*NUM_DBOARDS-1:0] gpio_spi_ctrlport_req_rd;
  wire [ 20*NUM_DBOARDS-1:0] gpio_spi_ctrlport_req_addr;
  wire [ 32*NUM_DBOARDS-1:0] gpio_spi_ctrlport_req_data;
  wire [  1*NUM_DBOARDS-1:0] gpio_spi_ctrlport_resp_ack;
  wire [  2*NUM_DBOARDS-1:0] gpio_spi_ctrlport_resp_status;
  wire [ 32*NUM_DBOARDS-1:0] gpio_spi_ctrlport_resp_data;

  // GPIO control signals from radio ctrlport endpoints to
  // x4xx_dio.
  wire [NUM_DBOARDS*32-1:0] atr_gpio_out;
  wire [NUM_DBOARDS*32-1:0] atr_gpio_ddr;

  wire [NUM_DBOARDS*32-1:0] spi_gpio_out;
  wire [NUM_DBOARDS*32-1:0] spi_gpio_ddr;

  genvar db;
  generate
    for (db = 0; db < NUM_DBOARDS; db = db+1) begin : gen_radio_ctrlport

      //----------------------------------------------------------------------------
      // Timed command processing
      //----------------------------------------------------------------------------

      wire [19:0] ctrlport_timed_req_addr;
      wire [31:0] ctrlport_timed_req_data;
      wire        ctrlport_timed_req_rd;
      wire        ctrlport_timed_req_wr;
      wire        ctrlport_timed_resp_ack;
      wire [31:0] ctrlport_timed_resp_data;
      wire [ 1:0] ctrlport_timed_resp_status;

      ctrlport_timer #(
        .EXEC_LATE_CMDS(1)
      ) ctrlport_timer_i (
        .clk                      (radio_clk),
        .rst                      (radio_rst),
        .time_now                 (radio_time),
        .time_now_stb             (sample_rx_stb),
        .time_ignore_bits         (time_ignore_bits),
        .s_ctrlport_req_wr        (s_radio_ctrlport_req_wr         [ 1*db+: 1]),
        .s_ctrlport_req_rd        (s_radio_ctrlport_req_rd         [ 1*db+: 1]),
        .s_ctrlport_req_addr      (s_radio_ctrlport_req_addr       [20*db+:20]),
        .s_ctrlport_req_data      (s_radio_ctrlport_req_data       [32*db+:32]),
        .s_ctrlport_req_byte_en   (s_radio_ctrlport_req_byte_en    [ 4*db+: 4]),
        .s_ctrlport_req_has_time  (s_radio_ctrlport_req_has_time   [ 1*db+: 1]),
        .s_ctrlport_req_time      (s_radio_ctrlport_req_time       [64*db+:64]),
        .s_ctrlport_resp_ack      (s_radio_ctrlport_resp_ack       [ 1*db+: 1]),
        .s_ctrlport_resp_status   (s_radio_ctrlport_resp_status    [ 2*db+: 2]),
        .s_ctrlport_resp_data     (s_radio_ctrlport_resp_data      [32*db+:32]),
        .m_ctrlport_req_wr        (ctrlport_timed_req_wr),
        .m_ctrlport_req_rd        (ctrlport_timed_req_rd),
        .m_ctrlport_req_addr      (ctrlport_timed_req_addr),
        .m_ctrlport_req_data      (ctrlport_timed_req_data),
        .m_ctrlport_req_byte_en   (),
        .m_ctrlport_resp_ack      (ctrlport_timed_resp_ack),
        .m_ctrlport_resp_status   (ctrlport_timed_resp_status),
        .m_ctrlport_resp_data     (ctrlport_timed_resp_data)
      );

      //-----------------------------------------------------------------------
      // Radio Block CtrlPort Splitter
      //-----------------------------------------------------------------------

      //   This section takes the CtrlPort master from each radio block and splits it
      //   into a CtrlPort bus for the associated daughter(m_radio_ctrlport_*), the
      //   RFDC timing control (rf_ctrlport_*), the ATR GPIO control for the DB state
      //   the current radio(db), the SPI controller of the radio, and DIO main
      //   control block(x4xx_dio).
      //   Refer to diagram in the RADIO_CTRLPORT_REGMAP Register map for a
      //   visual representation on how these interfaces are distributed.

      // Register space offset calculation
      localparam [19:0] DIO_SOURCE_CONTROL_OFFSET = DIO_WINDOW + DIO_SOURCE_CONTROL;
      localparam [19:0] RADIO_GPIO_ATR_OFFSET = DIO_WINDOW + RADIO_GPIO_ATR_REGS;
      localparam [19:0] DIGITAL_IFC_OFFSET = DIO_WINDOW + DIGITAL_IFC_REGS;

      // Register space size calculation
      localparam [31:0] RFDC_TIMING_WINDOW_SIZE_W = $clog2(RFDC_TIMING_WINDOW_SIZE);
      localparam [31:0] DB_WINDOW_SIZE_W          = $clog2(DB_WINDOW_SIZE);
      localparam [31:0] DIO_SOURCE_CONTROL_SIZE_W = $clog2(DIO_SOURCE_CONTROL_SIZE);
      localparam [31:0] RADIO_GPIO_ATR_SIZE_W     = $clog2(RADIO_GPIO_ATR_REGS_SIZE);
      localparam [31:0] DIGITAL_IFC_REGS_SIZE_W   = $clog2(DIGITAL_IFC_REGS_SIZE);

      ctrlport_decoder_param #(
        .NUM_SLAVES  (5),
        .PORT_BASE   ({ DIGITAL_IFC_OFFSET,
                        DIO_SOURCE_CONTROL_OFFSET,
                        RADIO_GPIO_ATR_OFFSET,
                        RFDC_TIMING_WINDOW[19:0],
                        DB_WINDOW[19:0]
                      }),
        .PORT_ADDR_W ({ DIGITAL_IFC_REGS_SIZE_W,
                        DIO_SOURCE_CONTROL_SIZE_W,
                        RADIO_GPIO_ATR_SIZE_W,
                        RFDC_TIMING_WINDOW_SIZE_W,
                        DB_WINDOW_SIZE_W
                      })
      ) ctrlport_decoder_param_i (
        .ctrlport_clk            (  radio_clk ),
        .ctrlport_rst            (  radio_rst ),
        .s_ctrlport_req_wr       (  ctrlport_timed_req_wr),
        .s_ctrlport_req_rd       (  ctrlport_timed_req_rd),
        .s_ctrlport_req_addr     (  ctrlport_timed_req_addr),
        .s_ctrlport_req_data     (  ctrlport_timed_req_data),
        .s_ctrlport_req_byte_en  (  4'hF),
        .s_ctrlport_req_has_time (  1'b0),
        .s_ctrlport_req_time     (  64'h0),
        .s_ctrlport_resp_ack     (  ctrlport_timed_resp_ack),
        .s_ctrlport_resp_status  (  ctrlport_timed_resp_status),
        .s_ctrlport_resp_data    (  ctrlport_timed_resp_data),
        .m_ctrlport_req_wr       ({ gpio_spi_ctrlport_req_wr        [ 1*db+: 1],
                                    radio_dio_req_wr                [ 1*db+: 1],
                                    gpio_atr_ctrlport_req_wr        [ 1*db+: 1],
                                    rf_ctrlport_req_wr              [ 1*db+: 1],
                                    m_radio_ctrlport_req_wr         [ 1*db+: 1] }),
        .m_ctrlport_req_rd       ({ gpio_spi_ctrlport_req_rd        [ 1*db+: 1],
                                    radio_dio_req_rd                [ 1*db+: 1],
                                    gpio_atr_ctrlport_req_rd        [ 1*db+: 1],
                                    rf_ctrlport_req_rd              [ 1*db+: 1],
                                    m_radio_ctrlport_req_rd         [ 1*db+: 1] }),
        .m_ctrlport_req_addr     ({ gpio_spi_ctrlport_req_addr      [20*db+:20],
                                    radio_dio_req_addr              [20*db+:20],
                                    gpio_atr_ctrlport_req_addr      [20*db+:20],
                                    rf_ctrlport_req_addr            [20*db+:20],
                                    m_radio_ctrlport_req_addr       [20*db+:20] }),
        .m_ctrlport_req_data     ({ gpio_spi_ctrlport_req_data      [32*db+:32],
                                    radio_dio_req_data              [32*db+:32],
                                    gpio_atr_ctrlport_req_data      [32*db+:32],
                                    rf_ctrlport_req_data            [32*db+:32],
                                    m_radio_ctrlport_req_data       [32*db+:32] }),
        .m_ctrlport_req_byte_en  (),
        .m_ctrlport_req_has_time (),
        .m_ctrlport_req_time     (),
        .m_ctrlport_resp_ack     ({ gpio_spi_ctrlport_resp_ack      [ 1*db+: 1],
                                    radio_dio_resp_ack              [ 1*db+: 1],
                                    gpio_atr_ctrlport_resp_ack      [ 1*db+: 1],
                                    rf_ctrlport_resp_ack            [ 1*db+: 1],
                                    m_radio_ctrlport_resp_ack       [ 1*db+: 1] }),
        .m_ctrlport_resp_status  ({ gpio_spi_ctrlport_resp_status   [ 2*db+: 2],
                                    radio_dio_resp_status           [ 2*db+: 2],
                                    gpio_atr_ctrlport_resp_status   [ 2*db+: 2],
                                    rf_ctrlport_resp_status         [ 2*db+: 2],
                                    m_radio_ctrlport_resp_status    [ 2*db+: 2] }),
        .m_ctrlport_resp_data    ({ gpio_spi_ctrlport_resp_data     [32*db+:32],
                                    radio_dio_resp_data             [32*db+:32],
                                    gpio_atr_ctrlport_resp_data     [32*db+:32],
                                    rf_ctrlport_resp_data           [32*db+:32],
                                    m_radio_ctrlport_resp_data      [32*db+:32] })
      );

      // Compute ATR state for this radio
      wire [ 3:0] db_state;

      assign db_state = { tx_running[2*db + 1], rx_running[2*db + 1],
                          tx_running[2*db + 0], rx_running[2*db + 0]};

      x4xx_gpio_atr #(
        .REG_SIZE (RADIO_GPIO_ATR_REGS_SIZE)
      ) x4xx_gpio_atr_i (
        .ctrlport_clk             (radio_clk),
        .ctrlport_rst             (radio_rst),
        .s_ctrlport_req_wr        (gpio_atr_ctrlport_req_wr       [ 1*db+: 1]),
        .s_ctrlport_req_rd        (gpio_atr_ctrlport_req_rd       [ 1*db+: 1]),
        .s_ctrlport_req_addr      (gpio_atr_ctrlport_req_addr     [20*db+:20]),
        .s_ctrlport_req_data      (gpio_atr_ctrlport_req_data     [32*db+:32]),
        .s_ctrlport_resp_ack      (gpio_atr_ctrlport_resp_ack     [ 1*db+: 1]),
        .s_ctrlport_resp_status   (gpio_atr_ctrlport_resp_status  [ 2*db+: 2]),
        .s_ctrlport_resp_data     (gpio_atr_ctrlport_resp_data    [32*db+:32]),
        .db_state                 (db_state),
        .gpio_in                  ({4'b0, gpio_in_b, 4'b0, gpio_in_a}),
        .gpio_out                 (atr_gpio_out[db*32+: 32]),
        .gpio_ddr                 (atr_gpio_ddr[db*32+: 32])
      );

      x4xx_gpio_spi #(
        .NUM_SLAVES (2)
      ) x4xx_gpio_spi_i(
        .ctrlport_clk             (radio_clk),
        .ctrlport_clk_2x          (radio_clk_2x),
        .ctrlport_rst             (radio_rst),
        .s_ctrlport_req_wr        (gpio_spi_ctrlport_req_wr       [ 1*db+: 1]),
        .s_ctrlport_req_rd        (gpio_spi_ctrlport_req_rd       [ 1*db+: 1]),
        .s_ctrlport_req_addr      (gpio_spi_ctrlport_req_addr     [20*db+:20]),
        .s_ctrlport_req_data      (gpio_spi_ctrlport_req_data     [32*db+:32]),
        .s_ctrlport_resp_ack      (gpio_spi_ctrlport_resp_ack     [ 1*db+: 1]),
        .s_ctrlport_resp_status   (gpio_spi_ctrlport_resp_status  [ 2*db+: 2]),
        .s_ctrlport_resp_data     (gpio_spi_ctrlport_resp_data    [32*db+:32]),
        .gpio_out                 (spi_gpio_out[db*32+: 32]),
        .gpio_ddr                 (spi_gpio_ddr[db*32+: 32]),
        .gpio_in                  ({4'b0, gpio_in_b, 4'b0, gpio_in_a})
      );

    end
  endgenerate


  //-------------------------------------------------------------------------
  // RF Timing Reset Control
  //-------------------------------------------------------------------------

  rfdc_timing_control #(
    .NUM_DBOARDS (NUM_DBOARDS)
  ) rfdc_timing_control_i (
    .clk                              (radio_clk),
    .rst                              (radio_rst),
    .s_ctrlport_req_wr                (rf_ctrlport_req_wr),
    .s_ctrlport_req_rd                (rf_ctrlport_req_rd),
    .s_ctrlport_req_addr              (rf_ctrlport_req_addr),
    .s_ctrlport_req_data              (rf_ctrlport_req_data),
    .s_ctrlport_resp_ack              (rf_ctrlport_resp_ack),
    .s_ctrlport_resp_status           (rf_ctrlport_resp_status),
    .s_ctrlport_resp_data             (rf_ctrlport_resp_data),
    .start_nco_reset                  (start_nco_reset),
    .nco_reset_done                   (nco_reset_done),
    .adc_reset_pulse                  (adc_reset_pulse),
    .dac_reset_pulse                  (dac_reset_pulse)
  );


  //---------------------------------------------------------------------------
  // DIO
  //---------------------------------------------------------------------------
  //
  // DIO lines may be controlled via 3 different ctrlport interfaces.
  // The diagram below shows how the different ctrlport interfaces are
  // conditioned and combined.
  //
  //                        ________________
  //  ps ctrlport          |                |
  // ----------------------|  CLK crossing  |-------+
  //                       |(rfnoc_ctrl_clk)|       |
  //                       |________________|       |
  //  radio 0 dio_ctrlport                          |     _____________
  // -------------------------------------------+   +----|             |
  //                                            |        |   Ctrlport  |
  //                                            +--------|   combiner  |---- x4xx_dio
  //  radio 1 dio_ctrlport                               |             |
  // ----------------------------------------------------|_____________|
  //     (radio_clk)
  //
  //

  // MPM ctrlport signals to convert to radio_clk domain
  wire        mpm_dio_req_wr;
  wire        mpm_dio_req_rd;
  wire [19:0] mpm_dio_req_addr;
  wire [31:0] mpm_dio_req_data;
  wire        mpm_dio_resp_ack;
  wire [ 1:0] mpm_dio_resp_status;
  wire [31:0] mpm_dio_resp_data;

  ctrlport_clk_cross ctrlport_clk_cross_dio (
    .rst                        (rfnoc_ctrl_rst),
    .s_ctrlport_clk             (rfnoc_ctrl_clk),
    .s_ctrlport_req_wr          (dio_req_wr),
    .s_ctrlport_req_rd          (dio_req_rd),
    .s_ctrlport_req_addr        (dio_req_addr),
    .s_ctrlport_req_portid      (10'b0),
    .s_ctrlport_req_rem_epid    (16'b0),
    .s_ctrlport_req_rem_portid  (10'b0),
    .s_ctrlport_req_data        (dio_req_data),
    .s_ctrlport_req_byte_en     (4'hF),
    .s_ctrlport_req_has_time    (1'b0),
    .s_ctrlport_req_time        (64'b0),
    .s_ctrlport_resp_ack        (dio_resp_ack),
    .s_ctrlport_resp_status     (dio_resp_status),
    .s_ctrlport_resp_data       (dio_resp_data),
    .m_ctrlport_clk             (radio_clk),
    .m_ctrlport_req_wr          (mpm_dio_req_wr),
    .m_ctrlport_req_rd          (mpm_dio_req_rd),
    .m_ctrlport_req_addr        (mpm_dio_req_addr),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (mpm_dio_req_data),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (mpm_dio_resp_ack),
    .m_ctrlport_resp_status     (mpm_dio_resp_status),
    .m_ctrlport_resp_data       (mpm_dio_resp_data)
  );

  // Since the PS ctrlport services other register endpoints
  // outside the DIO register space, none of which are addressed
  // through the ctrlport signals in the radio_clk domain.
  // For this reason, ctrlport transactions not intended for the
  // DIO register space will hang the bus if pushed into
  // the ctrlport_combiner. For this reason, transactions out
  // of said space will be filtered out from this clock crossed
  // instance of the PS ctrlport.

  // Windowed ctrlport signals
  wire        windowed_mpm_dio_req_wr;
  wire        windowed_mpm_dio_req_rd;
  wire [19:0] windowed_mpm_dio_req_addr;
  wire [31:0] windowed_mpm_dio_req_data;
  wire        windowed_mpm_dio_resp_ack;
  wire [ 1:0] windowed_mpm_dio_resp_status;
  wire [31:0] windowed_mpm_dio_resp_data;

  ctrlport_decoder_param #(
    .NUM_SLAVES   (1),
    .PORT_BASE    ({DIO[19:0]}),
    .PORT_ADDR_W  ({$clog2(DIO_SIZE)})
  ) ctrlport_decoder_dio_window (
    .ctrlport_clk               (radio_clk),
    .ctrlport_rst               (radio_rst),
    .s_ctrlport_req_wr          (mpm_dio_req_wr),
    .s_ctrlport_req_rd          (mpm_dio_req_rd),
    .s_ctrlport_req_addr        (mpm_dio_req_addr),
    .s_ctrlport_req_data        (mpm_dio_req_data),
    .s_ctrlport_req_byte_en     (4'hF),
    .s_ctrlport_req_has_time    (1'b0),
    .s_ctrlport_req_time        (64'b0),
    .s_ctrlport_resp_ack        (mpm_dio_resp_ack),
    .s_ctrlport_resp_status     (mpm_dio_resp_status),
    .s_ctrlport_resp_data       (mpm_dio_resp_data),
    .m_ctrlport_req_wr          ({windowed_mpm_dio_req_wr}),
    .m_ctrlport_req_rd          ({windowed_mpm_dio_req_rd}),
    .m_ctrlport_req_addr        ({windowed_mpm_dio_req_addr}),
    .m_ctrlport_req_data        ({windowed_mpm_dio_req_data}),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        ({windowed_mpm_dio_resp_ack}),
    .m_ctrlport_resp_status     ({windowed_mpm_dio_resp_status}),
    .m_ctrlport_resp_data       ({windowed_mpm_dio_resp_data})
  );


  // Combined dio ctrlport signals
  wire        dio_ctrlport_req_wr;
  wire        dio_ctrlport_req_rd;
  wire [19:0] dio_ctrlport_req_addr;
  wire [31:0] dio_ctrlport_req_data;
  wire        dio_ctrlport_resp_ack;
  wire [ 1:0] dio_ctrlport_resp_status;
  wire [31:0] dio_ctrlport_resp_data;

  // This combiner mixes the CtrlPort interfaces from each radio block with the
  // filtered PS CtrlPort bus to allow these interfaces to interact with the
  // DIO block.
  ctrlport_combiner #(
    .NUM_MASTERS  (NUM_DBOARDS + 1),
    .PRIORITY     (1)
  ) ctrlport_combiner_dio (
    .ctrlport_clk               (radio_clk),
    .ctrlport_rst               (radio_rst),
    .s_ctrlport_req_wr          ({radio_dio_req_wr,      windowed_mpm_dio_req_wr}),
    .s_ctrlport_req_rd          ({radio_dio_req_rd,      windowed_mpm_dio_req_rd}),
    .s_ctrlport_req_addr        ({radio_dio_req_addr,    windowed_mpm_dio_req_addr}),
    .s_ctrlport_req_portid      ({(NUM_DBOARDS+1){10'b0}}),
    .s_ctrlport_req_rem_epid    ({(NUM_DBOARDS+1){16'b0}}),
    .s_ctrlport_req_rem_portid  ({(NUM_DBOARDS+1){10'b0}}),
    .s_ctrlport_req_data        ({radio_dio_req_data,    windowed_mpm_dio_req_data}),
    .s_ctrlport_req_byte_en     ({(NUM_DBOARDS+1){4'hF}}),
    .s_ctrlport_req_has_time    ({(NUM_DBOARDS+1){1'b0}}),
    .s_ctrlport_req_time        ({(NUM_DBOARDS+1){64'b0}}),
    .s_ctrlport_resp_ack        ({radio_dio_resp_ack,    windowed_mpm_dio_resp_ack}),
    .s_ctrlport_resp_status     ({radio_dio_resp_status, windowed_mpm_dio_resp_status}),
    .s_ctrlport_resp_data       ({radio_dio_resp_data,   windowed_mpm_dio_resp_data}),
    .m_ctrlport_req_wr          (dio_ctrlport_req_wr),
    .m_ctrlport_req_rd          (dio_ctrlport_req_rd),
    .m_ctrlport_req_addr        (dio_ctrlport_req_addr),
    .m_ctrlport_req_portid      (),
    .m_ctrlport_req_rem_epid    (),
    .m_ctrlport_req_rem_portid  (),
    .m_ctrlport_req_data        (dio_ctrlport_req_data),
    .m_ctrlport_req_byte_en     (),
    .m_ctrlport_req_has_time    (),
    .m_ctrlport_req_time        (),
    .m_ctrlport_resp_ack        (dio_ctrlport_resp_ack),
    .m_ctrlport_resp_status     (dio_ctrlport_resp_status),
    .m_ctrlport_resp_data       (dio_ctrlport_resp_data)
  );

  x4xx_dio #(
    .REG_SIZE    (DIO_SIZE),
    .NUM_DBOARDS (NUM_DBOARDS)
  ) x4xx_dio_i (
    .ctrlport_clk                 (radio_clk),
    .ctrlport_rst                 (radio_rst),
    .s_ctrlport_req_wr            (dio_ctrlport_req_wr),
    .s_ctrlport_req_rd            (dio_ctrlport_req_rd),
    .s_ctrlport_req_addr          (dio_ctrlport_req_addr),
    .s_ctrlport_req_data          (dio_ctrlport_req_data),
    .s_ctrlport_resp_ack          (dio_ctrlport_resp_ack),
    .s_ctrlport_resp_status       (dio_ctrlport_resp_status),
    .s_ctrlport_resp_data         (dio_ctrlport_resp_data),
    .gpio_in_a                    (gpio_in_a),
    .gpio_in_b                    (gpio_in_b),
    .gpio_out_a                   (gpio_out_a),
    .gpio_out_b                   (gpio_out_b),
    .gpio_en_a                    (gpio_en_a),
    .gpio_en_b                    (gpio_en_b),
    .atr_gpio_out                 (atr_gpio_out),
    .atr_gpio_ddr                 (atr_gpio_ddr),
    .ps_gpio_out                  ({4'b0, ps_gpio_out_b, 4'b0, ps_gpio_out_a}),
    .ps_gpio_ddr                  ({4'b0, ps_gpio_ddr_b, 4'b0, ps_gpio_ddr_a}),
    .digital_ifc_gpio_out_radio0  (spi_gpio_out[31:0]),
    .digital_ifc_gpio_ddr_radio0  (spi_gpio_ddr[31:0]),
    .digital_ifc_gpio_out_radio1  (spi_gpio_out[63:32]),
    .digital_ifc_gpio_ddr_radio1  (spi_gpio_ddr[63:32]),
    .user_app_in_a                (gpio_in_fabric_a),
    .user_app_in_b                (gpio_in_fabric_b),
    .user_app_out_a               (gpio_out_fabric_a),
    .user_app_out_b               (gpio_out_fabric_b)
  );

  assign ps_gpio_in_a = gpio_in_fabric_a;
  assign ps_gpio_in_b = gpio_in_fabric_b;

endmodule


`default_nettype wire


//XmlParse xml_on
//
//<regmap name="RADIO_CTRLPORT_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="RADIO_CTRLPORT_WINDOWS">
//    <info>Each radio's CtrlPort peripheral interface is divided into the
//    following memory spaces. Note that the CtrlPort peripheral interface
//    starts at offset 0x80000 in the RFNoC Radio block's register space.
//    The following diagram displays the distribution of the CtrlPort
//    interface to the different modules it interacts with.
//    <img src = "x4xx_core_common_buses.svg"
// </info>
//    <window name="DB_WINDOW"        offset="0x00000" size="0x08000">
//      <info>Daughterboard GPIO interface. Register access within this space
//      is directed to the associated daughterboard CPLD.</info>
//    </window>
//    <window name="RFDC_TIMING_WINDOW" offset="0x08000" size="0x04000" targetregmap="RFDC_TIMING_REGMAP">
//      <info>RFDC timing control interface.</info>
//    </window>
//    <window name="DIO_WINDOW" offset="0x0C000" size="0x04000" targetregmap="RADIO_DIO_REGMAP">
//      <info>DIO control interface. Interacts with the DIO source selection
//            block, ATR-based DIO control and the DIO digital interface</info>
//    </window>
//  </group>
//</regmap>
//
//<regmap name="CORE_REGS_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This is the map for the registers that the CORE_REGS window has access to
//    from the ARM_AXI_HPM0_FPD port.
//
//    The registers contained here conform the mboard-regs node that MPM uses
//    to manage general FPGA control/status calls, such as versioning,
//    timekeeper, GPIO, etc.
//
//    The following diagram shows how the communication bus interacts with the
//    modules in CORE_REGS.
//    <img src = "x4xx_core_common_buses.svg"
//  </info>
//  <group name="CORE_REGS">
//    <window name="GLOBAL_REGS"     offset="0x0"   size="0xC00"  targetregmap="GLOBAL_REGS_REGMAP">
//      <info>Window to access global registers in the FPGA.</info>
//    </window>
//    <window name="VERSIONING_REGS" offset="0xC00" size="0x400"  targetregmap="VERSIONING_REGS_REGMAP">
//      <info>Window to access versioning registers in the FPGA.</info>
//    </window>
//    <window name="TIMEKEEPER"      offset="0x1000" size="0x20">
//      <info>Window to access the timekeeper register map.</info>
//    </window>
//    <window name="DIO"             offset="0x2000" size="0x40"  targetregmap="DIO_REGMAP">
//      <info>Window to access the DIO register map.</info>
//    </window>
//  </group>
//</regmap>

//<regmap name="RADIO_DIO_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This map contains register windows for controlling the different sources
//    that drive the state of DIO lines.
//  </info>
//  <group name="DIO_SOURCES">
//    <window name="RADIO_GPIO_ATR_REGS"     offset="0x0"    size="0x1000" targetregmap="GPIO_ATR_REGMAP">
//      <info>Contains controls for DIO behavior based on the ATR state of the accessed radio</info>
//    </window>
//    <window name="DIO_SOURCE_CONTROL"      offset="0x1000" size="0x1000" targetregmap="DIO_REGMAP">
//      <info>Window to access the DIO register map through the control port from the radio blocks.</info>
//    </window>
//    <window name="DIGITAL_IFC_REGS"      offset="0x2000" size="0x1000" targetregmap="DIG_IFC_REGMAP">
//      <info>Register space reserved for configuring a digital interface over the GPIO lines.
//            Currently, SPI is the only supported protocol.</info>
//    </window>
//  </group>
//</regmap>

//XmlParse xml_off
