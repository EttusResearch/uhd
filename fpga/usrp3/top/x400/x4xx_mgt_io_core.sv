//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_mgt_io_core
//
// Description:
//
//   Encapsulates the PCS/PMA, the MAC layer and the control interface
//   for 10GbE, and 100Gbe.
//
// Parameters:
//
//   PROTOCOL   : Indicates the protocol to use for each of the 4 QSFP lanes.
//                See x4xx_mgt_types.vh for possible values.
//   REG_BASE   : Base address for internal registers
//   REG_DWIDTH : Register data width
//   REG_AWIDTH : Register address width
//   PORTNUM    : Port number, to distinguish between multiple QSFP ports
//   LANENUM    : Lane number
//

`include "./x4xx_mgt_types.vh"


module x4xx_mgt_io_core #(
  parameter        PROTOCOL   = `MGT_100GbE,
  parameter [13:0] REG_BASE   = 14'h0,
  parameter        REG_DWIDTH = 32,
  parameter        REG_AWIDTH = 14,
  parameter [ 7:0] PORTNUM    = 8'd0,
  parameter        LANENUM    = 0
) (
  // Resets
  input  logic areset,
  input  logic bus_rst,

  output logic mgt_rst,

  // Clocks
  input  logic clk100,
  input  logic bus_clk,
  input  logic refclk_p,
  input  logic refclk_n,

  output logic mgt_clk,

  // QSFP high-speed IO
  output logic [3:0] tx_p,
  output logic [3:0] tx_n,
  input  logic [3:0] rx_p,
  input  logic [3:0] rx_n,

  // Common signals for single lane 10 GbE
  output logic [0:0] qpll0_reset,
  input  logic [0:0] qpll0_lock,
  input  logic [0:0] qpll0_clk,
  input  logic [0:0] qpll0_refclk,
  output logic [0:0] qpll1_reset,
  input  logic [0:0] qpll1_lock,
  input  logic [0:0] qpll1_clk,
  input  logic [0:0] qpll1_refclk,

  // AXI-Lite
  AxiLiteIf.slave m_axi_mac,

  // Data port
  // Interface clocks on mgt_tx and mgt_rx are NOT used (logic uses mgt_clk).
  AxiStreamIf.slave  mgt_tx,
  AxiStreamIf.master mgt_rx,
  input logic        mgt_pause_req,

  // Register port
  input  logic                  reg_wr_req,
  input  logic [REG_AWIDTH-1:0] reg_wr_addr,
  input  logic [REG_DWIDTH-1:0] reg_wr_data,
  input  logic                  reg_rd_req,
  input  logic [REG_AWIDTH-1:0] reg_rd_addr,
  output logic                  reg_rd_resp,
  output logic [REG_DWIDTH-1:0] reg_rd_data,

  // Misc.
  output logic        rx_rec_clk_out,
  output logic [31:0] port_info,
  output logic        link_up,
  output logic        activity
);

  import PkgAxiLite::*;


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  localparam [7:0] COMPAT_NUM        = 8'd2;

  // Common registers
  localparam REG_PORT_INFO           = REG_BASE + 'h0;
  localparam REG_MAC_CTRL_STATUS     = REG_BASE + 'h4;
  localparam REG_PHY_CTRL_STATUS     = REG_BASE + 'h8;
  localparam REG_MAC_LED_CTL         = REG_BASE + 'hC;

  // Ethernet specific
  localparam REG_ETH_MDIO_BASE       = REG_BASE + 'h10;

  // Aurora specific
  localparam REG_AURORA_OVERRUNS     = REG_BASE + 'h20;
  localparam REG_CHECKSUM_ERRORS     = REG_BASE + 'h24;
  localparam REG_BIST_CHECKER_SAMPS  = REG_BASE + 'h28;
  localparam REG_BIST_CHECKER_ERRORS = REG_BASE + 'h2C;

  localparam [ 1:0] MAC_LED_CTRL_RST_VAL = 2'h0;
  localparam [ 7:0] MGT_PROTOCOL         = PROTOCOL;
  localparam [31:0] MAC_CTRL_RST_VAL     =
    PROTOCOL == `MGT_100GbE      ? {31'h0, 1'b1} : // Auto-connect enabled by default
    PROTOCOL == `MGT_WhiteRabbit ? 32'h0 :
    PROTOCOL == `MGT_Aurora      ? 32'h0 :
    PROTOCOL == `MGT_10GbE       ? {31'h0, 1'b1} : // Tx enabled by default
    PROTOCOL == `MGT_1GbE        ? {31'h0, 1'b1} : // Tx enabled by default
                                   32'h0;
  localparam [31:0] PHY_CTRL_RST_VAL =
    PROTOCOL == `MGT_100GbE      ? 32'h0 : // Unused
    PROTOCOL == `MGT_WhiteRabbit ? 32'h0 : // Unused
    PROTOCOL == `MGT_Aurora      ? 32'h0 :
    PROTOCOL == `MGT_10GbE       ? 32'h0 : // Unused
    PROTOCOL == `MGT_1GbE        ? 32'h0 :
                                   32'h0;
  // Writable registers
  logic [31:0] mac_ctrl    = MAC_CTRL_RST_VAL;
  logic [31:0] phy_ctrl    = PHY_CTRL_RST_VAL;
  logic [ 1:0] mac_led_ctl = MAC_LED_CTRL_RST_VAL;

  always @(posedge bus_clk) begin
    if (bus_rst) begin
      mac_ctrl    <= MAC_CTRL_RST_VAL;
      phy_ctrl    <= PHY_CTRL_RST_VAL;
      mac_led_ctl <= MAC_LED_CTRL_RST_VAL;
    end else if (reg_wr_req) begin
      case(reg_wr_addr)
        REG_MAC_CTRL_STATUS:
          mac_ctrl <= reg_wr_data;
        REG_PHY_CTRL_STATUS:
          phy_ctrl <= reg_wr_data;
        REG_MAC_LED_CTL:
          mac_led_ctl <= reg_wr_data[1:0];
      endcase
    end
  end

  // Readable registers
  logic [31:0] overruns;
  logic [31:0] checksum_errors;
  logic [47:0] bist_checker_samps;
  logic [47:0] bist_checker_errors;
  logic [31:0] mac_status, phy_status;
  logic [31:0] mac_status_bclk, phy_status_bclk;
  logic activity_bclk, link_up_bclk;

  assign port_info = {COMPAT_NUM, 6'h0, activity_bclk, link_up_bclk, MGT_PROTOCOL, PORTNUM};

  always @(posedge bus_clk) begin
    // No reset handling needed for readback
    if (reg_rd_req) begin
      reg_rd_resp <= 1'b1;
      case(reg_rd_addr)
        REG_PORT_INFO:
          reg_rd_data <= port_info;
        REG_MAC_CTRL_STATUS:
          reg_rd_data <= mac_status_bclk;
        REG_PHY_CTRL_STATUS:
          reg_rd_data <= phy_status_bclk;
        REG_MAC_LED_CTL:
          reg_rd_data <= {30'd0, mac_led_ctl};
        REG_AURORA_OVERRUNS:
          reg_rd_data <= overruns;
        REG_CHECKSUM_ERRORS:
          reg_rd_data <= checksum_errors;
        REG_BIST_CHECKER_SAMPS:
          reg_rd_data <= bist_checker_samps[47:16];  // Scale num samples by 2^16
        REG_BIST_CHECKER_ERRORS:
          reg_rd_data <= bist_checker_errors[31:0];  // Don't scale errors
        default:
          begin
            reg_rd_data <= 32'd0;
            reg_rd_resp <= 1'b0;
          end
      endcase
    end if (reg_rd_resp) begin
      reg_rd_resp <= 1'b0;
    end
  end

  synchronizer #(
    .STAGES(2), .WIDTH(32), .INITIAL_VAL(32'h0)
  ) synchronizer_mac_status (
     .clk(bus_clk), .rst(1'b0), .in(mac_status), .out(mac_status_bclk)
  );

  synchronizer #(
    .STAGES(2), .WIDTH(32), .INITIAL_VAL(32'h0)
  ) synchronizer_phy_status (
     .clk(bus_clk), .rst(1'b0), .in(phy_status), .out(phy_status_bclk)
  );

  logic link_up_mgtclk;
  logic wr_activity = 0;

  if (PROTOCOL == `MGT_10GbE) begin : core_10g

    //-------------------------------------------------------------------------
    // 10 GbE Interface
    //-------------------------------------------------------------------------

    eth_10g eth_10g_i (
      .areset         (areset),
      // Free-running 100 MHz clock used for InitClk and AxiLite to MAC
      .clk100         (clk100),
      // Quad Info
      .qpll0_refclk   (qpll0_refclk),
      .qpll0_clk      (qpll0_clk),
      .qpll0_lock     (qpll0_lock),
      .qpll0_reset    (qpll0_reset),
      .qpll1_refclk   (qpll1_refclk),
      .qpll1_clk      (qpll1_clk),
      .qpll1_lock     (qpll1_lock),
      .qpll1_reset    (qpll1_reset),
      // Recovered clock for export
      .rx_rec_clk_out (rx_rec_clk_out),
      // MGT TX/RX differential signals
      .tx_p           (tx_p[LANENUM]),
      .tx_n           (tx_n[LANENUM]),
      .rx_p           (rx_p[LANENUM]),
      .rx_n           (rx_n[LANENUM]),
      // 156.25 MHz clock
      .mgt_clk        (mgt_clk),
      .mgt_rst        (mgt_rst),
      // AXI Stream TX Interface
      .mgt_tx         (mgt_tx),
      // AXI Stream RX Interface
      // There is no RX TREADY signal support in the IP. Received data has to
      // be read immediately or it is lost. TUSER indicates an error on
      // received packet.
      .mgt_rx         (mgt_rx),
      // AXI-Lite bus for tie off
      .mgt_axil       (m_axi_mac),
      // LEDs of QSFP28 port
      .phy_status     (phy_status),
      .mac_ctrl       (mac_ctrl),
      .mac_status     (mac_status),
      .phy_reset      (),
      .link_up        (link_up_mgtclk)
    );

    always_comb begin : eth_10g_tieoff
      overruns            = 0;
      checksum_errors     = 0;
      bist_checker_samps  = 0;
      bist_checker_errors = 0;
    end : eth_10g_tieoff

  end else if (PROTOCOL == `MGT_100GbE) begin : core_100g

    //-------------------------------------------------------------------------
    // 100 GbE Interface
    //-------------------------------------------------------------------------

    eth_100g eth_100g_i (
      .areset         (areset),
      // Free-running 100 MHz clock used for InitClk and AxiLite to MAC
      .clk100         (clk100),
      // MGT Reference Clock 100/125/156.25/161.1328125 MHz
      .refclk_p       (refclk_p),
      .refclk_n       (refclk_n),
      // Recovered clock for export
      .rx_rec_clk_out (rx_rec_clk_out),
      // MGT TX/RX differential signals
      .tx_p           (tx_p),
      .tx_n           (tx_n),
      .rx_p           (rx_p),
      .rx_n           (rx_n),
      // 322.26666 MHz clock generated by 100G PHY from RefClock
      .mgt_clk        (mgt_clk),
      .mgt_rst        (mgt_rst),
      .mgt_pause_req  (mgt_pause_req),
      // AXI Stream TX Interface
      .mgt_tx         (mgt_tx),
      // AXI Stream RX Interface
      // There is no RX TREADY signal support in the IP. Received data has to
      // be read immediately or it is lost. TUSER indicates an error on
      // received packet.
      .mgt_rx         (mgt_rx),
      .mgt_axil       (m_axi_mac),
      // LEDs of QSFP28 port
      .phy_status     (phy_status),
      .mac_status     (mac_status),
      .mac_ctrl       (mac_ctrl),
      .phy_reset      (),
      .link_up        (link_up_mgtclk)
    );

    always_comb begin : eth_100g_tieoff
      overruns            = 0;
      checksum_errors     = 0;
      bist_checker_samps  = 0;
      bist_checker_errors = 0;

      qpll0_reset         = 0;
      qpll1_reset         = 0;
    end : eth_100g_tieoff

  end else if (PROTOCOL == `MGT_Aurora) begin : core_aurora

    Aurora_not_yet_supported();

    always_comb begin : aurrora_tieoff
      m_axi_mac.drive_read_resp(.resp(SLVERR),.data(0));
      m_axi_mac.drive_write_resp(.resp(SLVERR));
      m_axi_mac.arready = 1'b1;
      m_axi_mac.awready = 1'b1;
      m_axi_mac.wready  = 1'b1;

      phy_status     =  'h0;
      mac_status     =  'h0;
      link_up_mgtclk = 1'b0;

      overruns            = 0;
      checksum_errors     = 0;
      bist_checker_samps  = 0;
      bist_checker_errors = 0;
      rx_rec_clk_out      = 0;

      qpll0_reset = 0;
      qpll1_reset = 0;
    end : aurrora_tieoff

  end else begin : core_disabled

    //-------------------------------------------------------------------------
    // Port Disabled
    //-------------------------------------------------------------------------

    assign mgt_clk = bus_clk;
    assign mgt_rst = bus_rst;

    always_comb begin : disabled_tieoff
      m_axi_mac.drive_read_resp(.resp(SLVERR),.data(0));
      m_axi_mac.drive_write_resp(.resp(SLVERR));
      m_axi_mac.arready = 1'b1;
      m_axi_mac.awready = 1'b1;
      m_axi_mac.wready  = 1'b1;

      phy_status     =  'h0;
      mac_status     =  'h0;
      link_up_mgtclk = 1'b0;

      mgt_tx.tready =  1'b1;
      mgt_rx.tdata  = 64'h0;
      mgt_rx.tuser  =  4'h0;
      mgt_rx.tlast  =  1'b0;
      mgt_rx.tvalid =  1'b0;
      mgt_rx.tkeep  =   'b0;

      overruns            = 0;
      checksum_errors     = 0;
      bist_checker_samps  = 0;
      bist_checker_errors = 0;
      rx_rec_clk_out      = 0;

      tx_p = 0;
      tx_n = 0;

      qpll0_reset = 0;
      qpll1_reset = 0;
    end : disabled_tieoff

  end


  //---------------------------------------------------------------------------
  // Activity Detector
  //---------------------------------------------------------------------------

  logic identify_enable,identify_value;
  logic activity_mgtclk, activity_int;
  always_comb begin
    identify_enable = mac_led_ctl[0];
    identify_value  = mac_led_ctl[1];
  end

  pulse_stretch pulse_stretch_activity_i (
    .clk   (mgt_clk),
    .rst   (mgt_rst | ~link_up_mgtclk),
    .pulse ((mgt_tx.tvalid & mgt_tx.tready) | (mgt_rx.tvalid & mgt_rx.tready)),
    .pulse_stretched (activity_mgtclk)
  );

  synchronizer #(
    .WIDTH  (2),
    .STAGES (1)
  ) synchronizer_lnk_act_bclk (
    .clk (bus_clk),
    .rst (bus_rst),
    .in  ({link_up_mgtclk, activity_mgtclk}),
    .out ({link_up_bclk,   activity_int})
  );

  always @ (posedge bus_clk) begin
    activity_bclk <= identify_enable ? identify_value : activity_int;
  end

  synchronizer #(
    .WIDTH  (2),
    .STAGES (1)
  ) synchronizer_lnk_act_aclk (
    .clk (m_axi_mac.clk),
    .rst (m_axi_mac.rst),
    .in  ({link_up_bclk, activity_bclk}),
    .out ({link_up,      activity})
  );

endmodule
