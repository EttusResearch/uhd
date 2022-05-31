///////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0
//
// Module: e320_mgt_io_core
// Description:
//   Encapsulates the PSC/PMA, the MAC layer and the control interface
//   for 1GbE, 10GbE and Aurora
//
//////////////////////////////////////////////////////////////////////

module e320_mgt_io_core #(
  parameter        PROTOCOL     = "10GbE",    // Must be {10GbE, 1GbE, Aurora, Disabled}
  parameter [13:0] REG_BASE     = 14'h0,
  parameter        REG_DWIDTH   = 32,
  parameter        REG_AWIDTH   = 14,
  parameter [7:0]  PORTNUM      = 8'd0,
  parameter        MDIO_EN      = 0,
  parameter [4:0]  MDIO_PHYADDR = 5'd0
)(
  // Resets
  input                   areset,
  input                   bus_rst,
  // Clocks
  input                   gt_refclk,
  input                   gb_refclk,
  input                   misc_clk,
  input                   bus_clk,
  // SFP high-speed IO
  output                  txp,
  output                  txn,
  input                   rxp,
  input                   rxn,
  // SFP low-speed IO
  input                   sfpp_rxlos,
  input                   sfpp_tx_fault,
  output                  sfpp_tx_disable,
  // Data port: Ethernet TX
  input  [63:0]           s_axis_tdata,
  input  [3:0]            s_axis_tuser,
  input                   s_axis_tlast,
  input                   s_axis_tvalid,
  output                  s_axis_tready,
  // Data port: Ethernet RX
  output [63:0]           m_axis_tdata,
  output [3:0]            m_axis_tuser,
  output                  m_axis_tlast,
  output                  m_axis_tvalid,
  input                   m_axis_tready,
  // Register port
  input                   reg_wr_req,
  input  [REG_AWIDTH-1:0] reg_wr_addr,
  input  [REG_DWIDTH-1:0] reg_wr_data,
  input                   reg_rd_req,
  input  [REG_AWIDTH-1:0] reg_rd_addr,
  output                  reg_rd_resp,
  output [REG_DWIDTH-1:0] reg_rd_data,
  // Misc
  output [31:0]           port_info,
  output                  link_up,
  output reg              activity
);

  //-----------------------------------------------------------------
  // Registers
  //-----------------------------------------------------------------
  localparam [7:0] COMPAT_NUM         = 8'd2;

  // Common registers
  localparam REG_PORT_INFO            = REG_BASE + 'h0;
  localparam REG_MAC_CTRL_STATUS      = REG_BASE + 'h4;
  localparam REG_PHY_CTRL_STATUS      = REG_BASE + 'h8;
  localparam REG_MAC_LED_CTL          = REG_BASE + 'hC;

  // Ethernet specific
  localparam REG_ETH_MDIO_BASE        = REG_BASE + 'h10;
  // Aurora specific
  localparam REG_AURORA_OVERRUNS      = REG_BASE + 'h20;
  localparam REG_CHECKSUM_ERRORS      = REG_BASE + 'h24;
  localparam REG_BIST_CHECKER_SAMPS   = REG_BASE + 'h28;
  localparam REG_BIST_CHECKER_ERRORS  = REG_BASE + 'h2C;

  wire                  reg_rd_resp_mdio;
  reg                   reg_rd_resp_glob = 1'b0;
  wire [REG_DWIDTH-1:0] reg_rd_data_mdio;
  reg  [REG_DWIDTH-1:0] reg_rd_data_glob = 32'h0;

  // Protocol specific constants
  wire [7:0]  mgt_protocol;
  wire [31:0] mac_ctrl_rst_val, phy_ctrl_rst_val;
  wire [1:0]  mac_led_ctl_rst_val = 2'h0;

  // Flush logic: If the link is not up, we will flush all packets coming from
  // the device. This avoids the MAC backpressuring when the PHY is down.
  // The device will always send discovery packets to the transports during
  // initialization, and they have no way of knowing if it's safe to travel
  // down this route. c2mac == "CHDR to MAC"
  wire  [63:0]     c2mac_tdata;
  wire  [3:0]      c2mac_tuser;
  wire             c2mac_tlast;
  wire             c2mac_tvalid;
  wire             c2mac_tready;

  axis_packet_flush #(
    .WIDTH(64+3), // tdata + tuser
    .TIMEOUT_W(1), // Not using timeout
    .FLUSH_PARTIAL_PKTS(0),
    .PIPELINE("NONE")
  ) linkup_flush (
    .clk(bus_clk),
    .reset(bus_rst),
    .enable(~link_up), // enable flushing when link down
    .timeout(1'b0),
    .flushing(/* not required */),
    .done(/* not required */),
    // Input from device/crossbar
    .s_axis_tdata  ({s_axis_tuser, s_axis_tdata}),
    .s_axis_tlast  (s_axis_tlast),
    .s_axis_tvalid (s_axis_tvalid),
    .s_axis_tready (s_axis_tready),
    // Output to MAC
    .m_axis_tdata  ({c2mac_tuser, c2mac_tdata}),
    .m_axis_tlast  (c2mac_tlast),
    .m_axis_tvalid (c2mac_tvalid),
    .m_axis_tready (c2mac_tready)
  );

  generate
    if (PROTOCOL == "Aurora") begin
      assign mgt_protocol     = 8'd3;
      assign mac_ctrl_rst_val = 32'h0;
      assign phy_ctrl_rst_val = 32'h0;
    end else if (PROTOCOL == "10GbE") begin
      assign mgt_protocol     = 8'd2;
      assign mac_ctrl_rst_val = {31'h0, 1'b1}; // tx_enable on reset
      assign phy_ctrl_rst_val = 32'h0;
    end else if (PROTOCOL == "1GbE") begin
      assign mgt_protocol     = 8'd1;
      assign mac_ctrl_rst_val = {31'h0, 1'b1}; // tx_enable on reset
      assign phy_ctrl_rst_val = 32'h0;
    end else begin
      assign mgt_protocol     = 8'd0;
      assign mac_ctrl_rst_val = 32'h0;
      assign phy_ctrl_rst_val = 32'h0;
    end
  endgenerate

  // Writable registers
  reg [31:0] mac_ctrl_reg = 32'h0;
  reg [31:0] phy_ctrl_reg = 32'h0;
  reg [1:0]  mac_led_ctl  =  2'h0;

  always @(posedge bus_clk) begin
    if (bus_rst) begin
      mac_ctrl_reg <= mac_ctrl_rst_val;
      phy_ctrl_reg <= phy_ctrl_rst_val;
      mac_led_ctl  <= mac_led_ctl_rst_val;
    end else if (reg_wr_req) begin
      case(reg_wr_addr)
        REG_MAC_CTRL_STATUS:
          mac_ctrl_reg <= reg_wr_data;
        REG_PHY_CTRL_STATUS:
          phy_ctrl_reg <= reg_wr_data;
        REG_MAC_LED_CTL:
          mac_led_ctl <= reg_wr_data[1:0];
      endcase
    end
  end

  // Readable registers
  wire [31:0] overruns;
  wire [31:0] checksum_errors;
  wire [47:0] bist_checker_samps;
  wire [47:0] bist_checker_errors;
  wire [31:0] mac_status, phy_status;
  wire [31:0] mac_status_bclk, phy_status_bclk;

  assign port_info = {COMPAT_NUM, 6'h0, activity, link_up, mgt_protocol, PORTNUM};

  always @(posedge bus_clk) begin
    // No reset handling needed for readback
    if (reg_rd_req) begin
      reg_rd_resp_glob <= 1'b1;
      case(reg_rd_addr)
        REG_PORT_INFO:
          reg_rd_data_glob <= port_info;
        REG_MAC_CTRL_STATUS:
          reg_rd_data_glob <= mac_status_bclk;
        REG_PHY_CTRL_STATUS:
          reg_rd_data_glob <= phy_status_bclk;
        REG_MAC_LED_CTL:
          reg_rd_data_glob <= {30'd0, mac_led_ctl};
        REG_AURORA_OVERRUNS:
          reg_rd_data_glob <= overruns;
        REG_CHECKSUM_ERRORS:
          reg_rd_data_glob <= checksum_errors;
        REG_BIST_CHECKER_SAMPS:
          reg_rd_data_glob <= bist_checker_samps[47:16];  //Scale num samples by 2^16
        REG_BIST_CHECKER_ERRORS:
          reg_rd_data_glob <= bist_checker_errors[31:0];  //Don't scale errors
        default:
          reg_rd_resp_glob <= 1'b0;
      endcase
    end if (reg_rd_resp_glob) begin
      reg_rd_resp_glob <= 1'b0;
    end
  end

  synchronizer #( .STAGES(2), .WIDTH(32), .INITIAL_VAL(32'h0) ) mac_status_sync_i (
     .clk(bus_clk), .rst(1'b0), .in(mac_status), .out(mac_status_bclk)
  );

  synchronizer #( .STAGES(2), .WIDTH(32), .INITIAL_VAL(32'h0) ) phy_status_sync_i (
     .clk(bus_clk), .rst(1'b0), .in(phy_status), .out(phy_status_bclk)
  );

  // Regport Mux for response
  regport_resp_mux #(
    .WIDTH      (REG_DWIDTH),
    .NUM_SLAVES (2)
  ) reg_resp_mux_i (
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp({reg_rd_resp_mdio, reg_rd_resp_glob}),
    .sla_rd_data({reg_rd_data_mdio, reg_rd_data_glob}),
    .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
  );

  //-----------------------------------------------------------------
  // Ethernet Specific: MDIO
  //-----------------------------------------------------------------

  wire mdc, mdio_m2s, mdio_s2m, mdio_s2m_sync;
  generate
    if ((PROTOCOL == "10GbE" || PROTOCOL == "1GbE") && (MDIO_EN == 1)) begin
      mdio_master #(
        .REG_BASE     (REG_ETH_MDIO_BASE),
        .REG_AWIDTH   (REG_AWIDTH),
        .MDC_DIVIDER  (8'd200)
      ) mdio_master_i (
        .clk          (bus_clk),
        .rst          (bus_rst),
        .mdc          (mdc),
        .mdio_in      (mdio_s2m_sync),
        .mdio_out     (mdio_m2s),
        .mdio_tri     (),
        .reg_wr_req   (reg_wr_req),
        .reg_wr_addr  (reg_wr_addr),
        .reg_wr_data  (reg_wr_data),
        .reg_rd_req   (reg_rd_req),
        .reg_rd_addr  (reg_rd_addr),
        .reg_rd_data  (reg_rd_data_mdio),
        .reg_rd_resp  (reg_rd_resp_mdio)
      );

      // We can cross mdio_s2m into the bus_clk domain. A synchronizer is safe
      // here because the bit is inherently async
      synchronizer #(.INITIAL_VAL(1'b0)) mdio_s2m_sync_i (
        .clk(bus_clk), .rst(1'b0 /* no reset */), .in(mdio_s2m), .out(mdio_s2m_sync)
      );
    end else begin
      assign mdc              = 1'b0;
      assign mdio_m2s         = 1'b0;
      assign reg_rd_resp_mdio = 1'b0;
      assign reg_rd_data_mdio = 32'h0;
    end
  endgenerate

  generate
    if (PROTOCOL == "10GbE") begin
      //-----------------------------------------------------------------
      // 10 Gigabit Ethernet
      //-----------------------------------------------------------------
      wire [63:0] xgmii_txd;
      wire [7:0]  xgmii_txc;
      wire [63:0] xgmii_rxd;
      wire [7:0]  xgmii_rxc;
      wire        xge_phy_resetdone;

      ten_gige_phy ten_gige_phy_i
      (
        // Clocks and Reset
        .areset(areset | phy_ctrl_reg[0]), // Asynchronous reset for entire core.
        .refclk(gt_refclk),              // Transciever reference clock: 156.25MHz
        .clk156(gb_refclk),              // Globally buffered core clock: 156.25MHz
        .dclk(misc_clk),                 // Management/DRP clock: 78.125MHz
        .sim_speedup_control(1'b0),
        // GMII Interface (client MAC <=> PCS)
        .xgmii_txd(xgmii_txd),          // Transmit data from client MAC.
        .xgmii_txc(xgmii_txc),          // Transmit control signal from client MAC.
        .xgmii_rxd(xgmii_rxd),          // Received Data to client MAC.
        .xgmii_rxc(xgmii_rxc),          // Received control signal to client MAC.
        // Tranceiver Interface
        .txp(txp),                       // Differential +ve of serial transmission from PMA to PMD.
        .txn(txn),                       // Differential -ve of serial transmission from PMA to PMD.
        .rxp(rxp),                       // Differential +ve for serial reception from PMD to PMA.
        .rxn(rxn),                       // Differential -ve for serial reception from PMD to PMA.
        // Management: MDIO Interface
        .mdc(mdc),                       // Management Data Clock
        .mdio_in(mdio_m2s),              // Management Data In
        .mdio_out(mdio_s2m),             // Management Data Out
        .mdio_tri(),                     // Management Data Tristate
        .prtad(MDIO_PHYADDR),            // MDIO address
        // General IO's
        .core_status(phy_status[7:0]),      // Core status
        .resetdone(xge_phy_resetdone),
        .signal_detect(~sfpp_rxlos),     // Input from PMD to indicate presence of optical input.   (Undocumented, but it seems Xilinx expect this to be inverted.)
        .tx_fault(sfpp_tx_fault),
        .tx_disable(sfpp_tx_disable)
      );

      xge_mac_wrapper #(
        .PORTNUM(PORTNUM),
        .WISHBONE(0)
      ) xge_mac_wrapper_i (
        // XGMII
        .xgmii_clk(gb_refclk),
        .xgmii_txd(xgmii_txd),
        .xgmii_txc(xgmii_txc),
        .xgmii_rxd(xgmii_rxd),
        .xgmii_rxc(xgmii_rxc),
        // Client FIFO Interfaces
        .sys_clk(bus_clk),
        .sys_rst(bus_rst),
        .rx_tdata(m_axis_tdata),
        .rx_tuser(m_axis_tuser),
        .rx_tlast(m_axis_tlast),
        .rx_tvalid(m_axis_tvalid),
        .rx_tready(m_axis_tready),
        .tx_tdata(c2mac_tdata),
        .tx_tuser(c2mac_tuser),   // Bit[3] (error) is ignored for now.
        .tx_tlast(c2mac_tlast),
        .tx_tvalid(c2mac_tvalid),
        .tx_tready(c2mac_tready),
        // Other
        .phy_ready(xge_phy_resetdone),
        .ctrl_tx_enable(mac_ctrl_reg[0]),
        .status_crc_error(mac_status[0]),
        .status_fragment_error(mac_status[1]),
        .status_txdfifo_ovflow(mac_status[2]),
        .status_txdfifo_udflow(mac_status[3]),
        .status_rxdfifo_ovflow(mac_status[4]),
        .status_rxdfifo_udflow(mac_status[5]),
        .status_pause_frame_rx(mac_status[6]),
        .status_local_fault(mac_status[7]),
        .status_remote_fault(mac_status[8]),
        .wb_ack_o(),
        .wb_dat_o(),
        .wb_adr_i(1'b0),
        .wb_clk_i(1'b0),
        .wb_cyc_i(1'b0),
        .wb_dat_i(1'b0),
        .wb_rst_i(1'b0),
        .wb_stb_i(1'b0),
        .wb_we_i (1'b0),
        .wb_int_o()
      );

      assign phy_status[31:8] = 24'h0;
      assign mac_status[31:9] = 23'h0;
      assign link_up = phy_status_bclk[0];

    end else if (PROTOCOL == "1GbE") begin

      //-----------------------------------------------------------------
      // 1 Gigabit Ethernet
      //-----------------------------------------------------------------
      wire [7:0]  gmii_txd, gmii_rxd;
      wire        gmii_tx_en, gmii_tx_er, gmii_rx_dv, gmii_rx_er;
      wire        gmii_clk;
      wire        gt0_qplloutclk, gt0_qplloutrefclk; //unused in 7-series Zynq

      assign gt0_qplloutclk = 1'b0;
      assign gt0_qplloutrefclk = 1'b0;
      assign sfpp_tx_disable = 1'b0; // Always on.

      one_gige_phy one_gige_phy_i
      (
        .reset(areset | phy_ctrl_reg[0]),                  // Asynchronous reset for entire core.
        .independent_clock(bus_clk),
        // Tranceiver Interface
        .gtrefclk(gt_refclk),           // Reference clock for MGT: 125MHz, very high quality.
        .gtrefclk_bufg(gb_refclk),      // Reference clock routed through a BUFG
        .txp(txp),                      // Differential +ve of serial transmission from PMA to PMD.
        .txn(txn),                      // Differential -ve of serial transmission from PMA to PMD.
        .rxp(rxp),                      // Differential +ve for serial reception from PMD to PMA.
        .rxn(rxn),                      // Differential -ve for serial reception from PMD to PMA.
        // GMII Interface (client MAC <=> PCS)
        .gmii_clk(gmii_clk),            // Clock to client MAC.
        .gmii_txd(gmii_txd),            // Transmit data from client MAC.
        .gmii_tx_en(gmii_tx_en),        // Transmit control signal from client MAC.
        .gmii_tx_er(gmii_tx_er),        // Transmit control signal from client MAC.
        .gmii_rxd(gmii_rxd),            // Received Data to client MAC.
        .gmii_rx_dv(gmii_rx_dv),        // Received control signal to client MAC.
        .gmii_rx_er(gmii_rx_er),        // Received control signal to client MAC.
        // Management: MDIO Interface
        .mdc(mdc),                      // Management Data Clock
        .mdio_i(mdio_m2s),              // Management Data In
        .mdio_o(mdio_s2m),              // Management Data Out
        .mdio_t(),                      // Management Data Tristate
        .phyaddr(MDIO_PHYADDR),         // MDIO address
        .configuration_vector(5'd0),    // Alternative to MDIO interface.
        .configuration_valid(1'b1),     // Validation signal for Config vector (MUST be 1 for proper functionality...undocumented)
        // General IO's
        .status_vector(phy_status[15:0]),    // Core status.
        .signal_detect(1'b1 /*Optical module not supported*/) // Input from PMD to indicate presence of optical input.
      );

      simple_gemac_wrapper #(.RX_FLOW_CTRL(0), .PORTNUM(PORTNUM)) simple_gemac_wrapper_i
      (
        .clk125(gmii_clk),
        .reset(areset),

        .GMII_GTX_CLK(),
        .GMII_TX_EN(gmii_tx_en),
        .GMII_TX_ER(gmii_tx_er),
        .GMII_TXD(gmii_txd),
        .GMII_RX_CLK(gmii_clk),
        .GMII_RX_DV(gmii_rx_dv),
        .GMII_RX_ER(gmii_rx_er),
        .GMII_RXD(gmii_rxd),

        .sys_clk(bus_clk),
        .rx_tdata(m_axis_tdata),
        .rx_tuser(m_axis_tuser),
        .rx_tlast(m_axis_tlast),
        .rx_tvalid(m_axis_tvalid),
        .rx_tready(m_axis_tready),
        .tx_tdata(c2mac_tdata),
        .tx_tuser(c2mac_tuser),
        .tx_tlast(c2mac_tlast),
        .tx_tvalid(c2mac_tvalid),
        .tx_tready(c2mac_tready),

        .wb_clk_i(1'b0),
        .wb_rst_i(1'b0),
        .wb_adr_i(8'h0),
        .wb_dat_i(32'h0),
        .wb_we_i(1'b0),
        .wb_stb_i(1'b0),
        .wb_cyc_i(1'b0),
        .wb_dat_o(),
        .wb_ack_o(),
        .wb_int_o(),
        .mdc(),
        .mdio_out(1'b0),
        .mdio_tri(),
        .mdio_in(),
        .debug_tx(),
        .debug_rx()
      );

      assign phy_status[31:16] = 16'h0;
      assign mac_status[31:0]  = 32'h0;
      assign link_up = phy_status_bclk[0];

      assign gt_tx_out_clk_unbuf = 1'b0;

    end else if (PROTOCOL == "Aurora") begin

      //-----------------------------------------------------------------
      // Aurora
      //-----------------------------------------------------------------
      wire        user_clk, user_rst;
      wire [63:0] m2p_tdata, p2m_tdata;
      wire        m2p_tvalid, m2p_tready, p2m_tvalid;
      wire        channel_up, hard_err, soft_err, mac_crit_err;

      wire        bist_checker_en  = mac_ctrl_reg[0];
      wire        bist_gen_en      = mac_ctrl_reg[1];
      wire        bist_loopback_en = mac_ctrl_reg[2];
      wire [5:0]  bist_gen_rate    = mac_ctrl_reg[8:3];
      wire        phy_areset       = mac_ctrl_reg[9];
      wire        mac_clear        = mac_ctrl_reg[10];
      wire        bist_checker_locked;

      assign sfpp_tx_disable = 1'b0; // Always on.

      aurora_phy_x1 aurora_phy_i (
        // Resets
        .areset(areset | phy_areset),
        // Clocks
        .refclk(gt_refclk),
        .init_clk(misc_clk),
        .user_clk(user_clk),
        .user_rst(user_rst),
        // GTX Serial I/O
        .tx_p(txp),
        .tx_n(txn),
        .rx_p(rxp),
        .rx_n(rxn),
        // AXI4-Stream TX Interface
        .s_axis_tdata(m2p_tdata),
        .s_axis_tvalid(m2p_tvalid),
        .s_axis_tready(m2p_tready),
        // AXI4-Stream RX Interface
        .m_axis_tdata(p2m_tdata),
        .m_axis_tvalid(p2m_tvalid),
        // AXI4-Lite Config Interface (unused)
        .s_axi_awaddr(32'h0),
        .s_axi_araddr(32'h0),
        .s_axi_awvalid(1'b0),
        .s_axi_awready(),
        .s_axi_wdata(32'h0),
        .s_axi_wvalid(1'b0),
        .s_axi_wstrb(1'b0),
        .s_axi_wready(),
        .s_axi_bvalid(),
        .s_axi_bresp(),
        .s_axi_bready(1'b1),
        .s_axi_arready(),
        .s_axi_arvalid(1'b0),
        .s_axi_rdata(),
        .s_axi_rvalid(),
        .s_axi_rresp(),
        .s_axi_rready(1'b1),
        // Status and Error Reporting Interface
        .channel_up(channel_up),
        .hard_err(hard_err),
        .soft_err(soft_err)
      );

      aurora_axis_mac #(
        .PHY_ENDIANNESS ("LITTLE"),
        .PACKET_MODE    (1),
        .MAX_PACKET_SIZE(1024),
        .BIST_ENABLED   (1)
      ) aurora_mac_i (
        // Clocks and resets
        .phy_clk(user_clk), .phy_rst(user_rst),
        .sys_clk(bus_clk), .sys_rst(bus_rst),
        .clear(mac_clear),
        // PHY Interface (Synchronous to phy_clk)
        .phy_s_axis_tdata(p2m_tdata),
        .phy_s_axis_tvalid(p2m_tvalid),
        .phy_m_axis_tdata(m2p_tdata),
        .phy_m_axis_tvalid(m2p_tvalid),
        .phy_m_axis_tready(m2p_tready),
        // User Interface (Synchronous to sys_clk)
        .s_axis_tdata(c2mac_tdata),
        .s_axis_tlast(c2mac_tlast),
        .s_axis_tvalid(c2mac_tvalid),
        .s_axis_tready(c2mac_tready),
        .m_axis_tdata(m_axis_tdata),
        .m_axis_tlast(m_axis_tlast),
        .m_axis_tvalid(m_axis_tvalid),
        .m_axis_tready(m_axis_tready),
        // PHY Status Inputs (Synchronous to phy_clk)
        .channel_up(channel_up),
        .hard_err(hard_err),
        .soft_err(soft_err),
        // Status and Error Outputs (Synchronous to sys_clk)
        .overruns(overruns),
        .soft_errors(),
        .checksum_errors(checksum_errors),
        .critical_err(mac_crit_err),
        // BIST Interface (Synchronous to sys_clk)
        .bist_gen_en(bist_gen_en),
        .bist_gen_rate(bist_gen_rate),
        .bist_checker_en(bist_checker_en),
        .bist_loopback_en(bist_loopback_en),
        .bist_checker_locked(bist_checker_locked),
        .bist_checker_samps(bist_checker_samps),
        .bist_checker_errors(bist_checker_errors)
      );

      assign m_axis_tuser = 4'd0;

      wire channel_up_bclk, hard_err_bclk, soft_err_bclk, mac_crit_err_bclk;
      synchronizer #(.INITIAL_VAL(1'b0)) channel_up_sync (
        .clk(bus_clk), .rst(1'b0 /* no reset */), .in(channel_up), .out(channel_up_bclk));
      synchronizer #(.INITIAL_VAL(1'b0)) hard_err_sync (
        .clk(bus_clk), .rst(1'b0 /* no reset */), .in(hard_err), .out(hard_err_bclk));
      synchronizer #(.INITIAL_VAL(1'b0)) soft_err_sync (
        .clk(bus_clk), .rst(1'b0 /* no reset */), .in(soft_err), .out(soft_err_bclk));
      synchronizer #(.INITIAL_VAL(1'b0)) mac_crit_err_sync (
        .clk(bus_clk), .rst(1'b0 /* no reset */), .in(mac_crit_err), .out(mac_crit_err_bclk));

      reg [19:0]  bist_lock_latency;
      always @(posedge bus_clk) begin
        if (!bist_checker_en && !bist_checker_locked)
          bist_lock_latency <= 20'd0;
        else if (bist_checker_en && !bist_checker_locked)
          bist_lock_latency <= bist_lock_latency + 20'd1;
      end

      reg mac_crit_err_latch;
      always @(posedge bus_clk) begin
        if (bus_rst | mac_clear) begin
          mac_crit_err_latch <= 1'b0;
        end else begin
          if (mac_crit_err_bclk)
            mac_crit_err_latch <= 1'b1;
        end
      end

      assign phy_status = {30'd0, hard_err, channel_up};
      assign mac_status = {
        6'h0,                      //[31:26]
        mac_crit_err_latch,        //[25]
        1'b1,                      //[24]
        1'b0,                      //[23]
        1'b0,                      //[22]
        1'b0,                      //[21]
        1'b0,                      //[20]
        bist_lock_latency[19:4],   //[19:4]
        bist_checker_locked,       //[3]
        soft_err_bclk,             //[2]
        hard_err_bclk,             //[1]
        channel_up_bclk            //[0]
      };

      assign link_up = channel_up_bclk;

    end else begin

      //-----------------------------------------------------------------
      // Disabled
      //-----------------------------------------------------------------

      assign phy_status = 'h0;
      assign mac_status = 'h0;
      assign link_up = 1'b0;

      assign sfpp_tx_disable = 1'b0; // Always on.

      assign c2mac_tready = 1'b1;
      assign m_axis_tdata = 64'h0;
      assign m_axis_tuser = 4'h0;
      assign m_axis_tlast = 1'b0;
      assign m_axis_tvalid = 1'b0;

    end
  endgenerate

  wire identify_enable = mac_led_ctl[0];
  wire identify_value  = mac_led_ctl[1];

  //-----------------------------------------------------------------
  // Activity detector
  //-----------------------------------------------------------------
  wire activity_int;

  pulse_stretch act_pulse_str_i (
    .clk(bus_clk),
    .rst(bus_rst | ~link_up),
    .pulse((s_axis_tvalid & s_axis_tready) | (m_axis_tvalid & m_axis_tready)),
    .pulse_stretched(activity_int)
  );

  always @ (posedge bus_clk) activity <= identify_enable ? identify_value : activity_int;

endmodule
