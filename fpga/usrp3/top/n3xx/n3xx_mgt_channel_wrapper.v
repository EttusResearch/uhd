///////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: n3xx_mgt_channel_wrapper
// Description:
//   Aurora/10 GbE wrapper for up to 4 QSFP lanes  -or-
//   Aurora/1 GbE/10 GbE/White Rabbit wrapper for 1 SFP+ lane
//
//////////////////////////////////////////////////////////////////////

`default_nettype none
module n3xx_mgt_channel_wrapper #(
  parameter PROTOCOL     = "10GbE",// Must be {10GbE, Aurora, Disabled}
  parameter LANES        = 2,      // Number of lanes of to instantiate (Supported = {1,2,3,4})
  parameter REG_BASE     = 32'h0,  // Base register address
  parameter PORTNUM_BASE = 4,      // Base port number for discovery
  parameter MDIO_EN      = 1,      // Enable MDIO port
  parameter [4:0] MDIO_PHYADDR = 5'd0,   // Enable MDIO port
  parameter REG_DWIDTH   = 32,     // Width of regport address bus
  parameter REG_AWIDTH   = 14,     // Width of regport data bus
  parameter GT_COMMON    = 1
)(
  // Resets
  input  wire                    areset,
  input  wire                    bus_rst,

  // Clocks
  input  wire                    gt_refclk,
  input  wire                    gb_refclk,
  input  wire                    misc_clk,
  input  wire                    bus_clk,
  input  wire                    user_clk,
  input  wire                    sync_clk,

  //Axi-lite
  input  wire                    s_axi_aclk,
  input  wire                    s_axi_aresetn,
  input  wire [REG_AWIDTH-1:0]   s_axi_awaddr,
  input  wire                    s_axi_awvalid,
  output wire                    s_axi_awready,

  input  wire [REG_DWIDTH-1:0]   s_axi_wdata,
  input  wire [REG_DWIDTH/8-1:0] s_axi_wstrb,
  input  wire                    s_axi_wvalid,
  output wire                    s_axi_wready,

  output wire [1:0]              s_axi_bresp,
  output wire                    s_axi_bvalid,
  input  wire                    s_axi_bready,

  input  wire [REG_AWIDTH-1:0]   s_axi_araddr,
  input  wire                    s_axi_arvalid,
  output wire                    s_axi_arready,

  output wire [REG_DWIDTH-1:0]   s_axi_rdata,
  output wire [1:0]              s_axi_rresp,
  output wire                    s_axi_rvalid,
  input  wire                    s_axi_rready,

  // Serial lanes (high-speed IO)
  output wire [LANES-1:0]        txp,
  output wire [LANES-1:0]        txn,
  input  wire [LANES-1:0]        rxp,
  input  wire [LANES-1:0]        rxn,

  // Low-speed IO (QSFP+ and SFP+ module signals)
  input  wire                    mod_present_n,
  input  wire                    mod_rxlos,
  input  wire                    mod_tx_fault,
  output wire                    mod_tx_disable,
  input  wire                    mod_reset_n,
  input  wire                    mod_int_n,
  output wire                    mod_lpmode,
  output wire                    mod_sel_n,

  // Timebase Outputs
  output wire                    mod_pps,
  output wire                    mod_refclk,

  // GT Common
  output wire                    qpllreset,
  input  wire                    qplllock,
  input  wire                    qplloutclk,
  input  wire                    qplloutrefclk,
  input  wire                    qpllrefclklost,

  // Aurora MMCM
  input  wire                    mmcm_locked,
  output wire                    gt_pll_lock,
  output wire                    gt_tx_out_clk_unbuf,

  // AXIS output interface
  output wire [(LANES*64)-1:0]   e2v_tdata,
  output wire [LANES-1:0]        e2v_tlast,
  output wire [LANES-1:0]        e2v_tvalid,
  input  wire [LANES-1:0]        e2v_tready,
  // AXIS input interface
  input  wire [(LANES*64)-1:0]   v2e_tdata,
  input  wire [LANES-1:0]        v2e_tlast,
  input  wire [LANES-1:0]        v2e_tvalid,
  output wire [LANES-1:0]        v2e_tready,

  // CPU
  output wire [LANES*64-1:0]     e2c_tdata,
  output wire [LANES*8-1:0]      e2c_tkeep,
  output wire [LANES-1:0]        e2c_tlast,
  output wire [LANES-1:0]        e2c_tvalid,
  input  wire [LANES-1:0]        e2c_tready,

  input  wire [LANES*64-1:0]     c2e_tdata,
  input  wire [LANES*8-1:0]      c2e_tkeep,
  input  wire [LANES-1:0]        c2e_tlast,
  input  wire [LANES-1:0]        c2e_tvalid,
  output wire [LANES-1:0]        c2e_tready,

  // MISC
  output wire [LANES*32-1:0]     port_info,
  input wire [15:0]              device_id,

  // Sideband White Rabbit Control
  input  wire                    wr_reset_n,
  input  wire                    wr_refclk,

  output wire                    wr_dac_sclk,
  output wire                    wr_dac_din,
  output wire                    wr_dac_clr_n,
  output wire                    wr_dac_cs_n,
  output wire                    wr_dac_ldac_n,

  output wire                    wr_eeprom_scl_o,
  input  wire                    wr_eeprom_scl_i,
  output wire                    wr_eeprom_sda_o,
  input  wire                    wr_eeprom_sda_i,

  input  wire                    wr_uart_rx,
  output wire                    wr_uart_tx,

  // WR AXI Control
  output wire                    wr_axi_aclk,
  input  wire                    wr_axi_aresetn,
  input  wire [31:0]             wr_axi_awaddr,
  input  wire                    wr_axi_awvalid,
  output wire                    wr_axi_awready,
  input  wire [REG_DWIDTH-1:0]   wr_axi_wdata,
  input  wire [REG_DWIDTH/8-1:0] wr_axi_wstrb,
  input  wire                    wr_axi_wvalid,
  output wire                    wr_axi_wready,
  output wire [1:0]              wr_axi_bresp,
  output wire                    wr_axi_bvalid,
  input  wire                    wr_axi_bready,
  input  wire [31:0]             wr_axi_araddr,
  input  wire                    wr_axi_arvalid,
  output wire                    wr_axi_arready,
  output wire [REG_DWIDTH-1:0]   wr_axi_rdata,
  output wire [1:0]              wr_axi_rresp,
  output wire                    wr_axi_rvalid,
  input  wire                    wr_axi_rready,
  output wire                    wr_axi_rlast,

  output wire [LANES-1:0]        link_up,
  output wire [LANES-1:0]        activity
);

  //--------------------------------------------------------------
  // QSFP module I/O
  //--------------------------------------------------------------
  assign mod_reset_n  = 1'b1;
  assign mod_sel_n = 1'b0;
  assign mod_lpmode   = 1'b0;

  //--------------------------------------------------------------
  // Common clocking
  //--------------------------------------------------------------

  wire [LANES-1:0]  qpllreset_ln;
  wire [LANES-1:0]  gt_tx_out_clk;
  wire [LANES-1:0]  gt_pll_lock_ln;

  assign qpllreset = |qpllreset_ln;
  assign gt_tx_out_clk_unbuf = gt_tx_out_clk[0];
  assign gt_pll_lock = gt_pll_lock_ln[0];

  //--------------------------------------------------------------
  // Register bus
  //--------------------------------------------------------------
  localparam REG_BLOCK_SIZE = 20'h4000;

  // AXI4-Lite to RegPort (PS to PL Register Access)
  wire                    reg_wr_req;
  wire  [REG_AWIDTH-1:0]  reg_wr_addr;
  wire  [REG_DWIDTH-1:0]  reg_wr_data;
  wire                    reg_rd_req;
  wire  [REG_AWIDTH-1:0]  reg_rd_addr;
  wire                    reg_rd_resp;
  wire  [REG_DWIDTH-1:0]  reg_rd_data;

  axil_regport_master #(
    .DWIDTH         (REG_DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
    .AWIDTH         (REG_AWIDTH),   // Width of the address bus
    .WRBASE         (0),        // Write address base
    .RDBASE         (0),        // Read address base
    .TIMEOUT        (10)        // log2(timeout). Read will timeout after (2^TIMEOUT - 1) cycles
  ) mgt_reg_mst_i (
    // Clock and reset
    .s_axi_aclk     (s_axi_aclk),
    .s_axi_aresetn  (s_axi_aresetn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr   (s_axi_awaddr),
    .s_axi_awvalid  (s_axi_awvalid),
    .s_axi_awready  (s_axi_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata    (s_axi_wdata),
    .s_axi_wstrb    (s_axi_wstrb),
    .s_axi_wvalid   (s_axi_wvalid),
    .s_axi_wready   (s_axi_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp    (s_axi_bresp),
    .s_axi_bvalid   (s_axi_bvalid),
    .s_axi_bready   (s_axi_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr   (s_axi_araddr),
    .s_axi_arvalid  (s_axi_arvalid),
    .s_axi_arready  (s_axi_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata    (s_axi_rdata),
    .s_axi_rresp    (s_axi_rresp),
    .s_axi_rvalid   (s_axi_rvalid),
    .s_axi_rready   (s_axi_rready),
    // Register port: Write port (domain: reg_clk)
    .reg_clk        (bus_clk),
    .reg_wr_req     (reg_wr_req),
    .reg_wr_addr    (reg_wr_addr),
    .reg_wr_data    (reg_wr_data),
    .reg_wr_keep    (/*unused*/),
    // Register port: Read port (domain: reg_clk)
    .reg_rd_req     (reg_rd_req),
    .reg_rd_addr    (reg_rd_addr),
    .reg_rd_resp    (reg_rd_resp),
    .reg_rd_data    (reg_rd_data)
  );

  wire [LANES-1:0]      reg_rd_resp_flat;
  wire [(LANES*REG_DWIDTH)-1:0] reg_rd_data_flat;

  regport_resp_mux #(
    .WIDTH      (REG_DWIDTH),
    .NUM_SLAVES (LANES)
  ) reg_resp_mux_i(
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp(reg_rd_resp_flat), .sla_rd_data(reg_rd_data_flat),
    .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
  );

  //--------------------------------------------------------------
  // Lanes
  //--------------------------------------------------------------

  genvar l;
  generate
    for (l = 0; l < LANES; l = l + 1) begin: lanes
      n3xx_mgt_wrapper #(
        .PROTOCOL       (PROTOCOL),
        .REG_BASE       (REG_BASE + (REG_BLOCK_SIZE * l)),
        .REG_DWIDTH     (REG_DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
        .REG_AWIDTH     (REG_AWIDTH),   // Width of the address bus
        .GT_COMMON      (GT_COMMON),
        .MDIO_EN        (MDIO_EN),
        .MDIO_PHYADDR   (MDIO_PHYADDR),
        .PORTNUM        (PORTNUM_BASE + l)
      ) lane_i (
        //must reset all channels on quad when sfp1 gtx core is reset
        .areset         (areset),
        .gt_refclk      (gt_refclk),
        .gb_refclk      (gb_refclk),
        .misc_clk       (misc_clk),
        .user_clk       (user_clk),
        .sync_clk       (sync_clk),

        .bus_rst        (bus_rst),
        .bus_clk        (bus_clk),

        //RegPort
        .reg_wr_req     (reg_wr_req),
        .reg_wr_addr    (reg_wr_addr),
        .reg_wr_data    (reg_wr_data),
        .reg_rd_req     (reg_rd_req),
        .reg_rd_addr    (reg_rd_addr),
        .reg_rd_resp    (reg_rd_resp_flat[l]),
        .reg_rd_data    (reg_rd_data_flat[l*REG_DWIDTH +: REG_DWIDTH]),

        .txp            (txp[l]),
        .txn            (txn[l]),
        .rxp            (rxp[l]),
        .rxn            (rxn[l]),

        .mod_present_n  (mod_present_n),
        .mod_rxlos      (mod_rxlos),
        .mod_tx_fault   (mod_tx_fault),
        .mod_tx_disable (mod_tx_disable),

        .qpllrefclklost (qpllrefclklost),
        .qplllock       (qplllock),
        .qplloutclk     (qplloutclk),
        .qplloutrefclk  (qplloutrefclk),
        .qpllreset      (qpllreset_ln[l]),

        .mmcm_locked    (mmcm_locked),
        .gt_pll_lock    (gt_pll_lock_ln[l]),
        .gt_tx_out_clk_unbuf(gt_tx_out_clk[l]),

        // Vita router interface (Synchronous to bus_clk)
        .e2v_tdata      (e2v_tdata[l*64 +: 64]),
        .e2v_tlast      (e2v_tlast[l]),
        .e2v_tvalid     (e2v_tvalid[l]),
        .e2v_tready     (e2v_tready[l]),

        .v2e_tdata      (v2e_tdata[l*64 +: 64]),
        .v2e_tlast      (v2e_tlast[l]),
        .v2e_tvalid     (v2e_tvalid[l]),
        .v2e_tready     (v2e_tready[l]),

        // CPU
        .e2c_tdata      (e2c_tdata[l*64 +: 64]),
        .e2c_tkeep      (e2c_tkeep[l*8 +: 8]),
        .e2c_tlast      (e2c_tlast[l]),
        .e2c_tvalid     (e2c_tvalid[l]),
        .e2c_tready     (e2c_tready[l]),

        .c2e_tdata      (c2e_tdata[l*64 +: 64]),
        .c2e_tkeep      (c2e_tkeep[l*8 +: 8]),
        .c2e_tlast      (c2e_tlast[l]),
        .c2e_tvalid     (c2e_tvalid[l]),
        .c2e_tready     (c2e_tready[l]),

        .port_info      (port_info[l*32 +: 32]),
        .device_id      (device_id),

        // Sideband White Rabbit Control
        .wr_reset_n     (wr_reset_n),
        .wr_refclk      (wr_refclk),

        .wr_dac_sclk    (wr_dac_sclk),
        .wr_dac_din     (wr_dac_din),
        .wr_dac_clr_n   (wr_dac_clr_n),
        .wr_dac_cs_n    (wr_dac_cs_n),
        .wr_dac_ldac_n  (wr_dac_ldac_n),

        .wr_eeprom_scl_o(wr_eeprom_scl_o),
        .wr_eeprom_scl_i(wr_eeprom_scl_i),
        .wr_eeprom_sda_o(wr_eeprom_sda_o),
        .wr_eeprom_sda_i(wr_eeprom_sda_o),

        .wr_uart_rx     (wr_uart_rx),
        .wr_uart_tx     (wr_uart_tx),

        // WR AXI Control
        .wr_axi_aclk    (wr_axi_aclk),
        .wr_axi_aresetn (wr_axi_aresetn),
        .wr_axi_awaddr  (wr_axi_awaddr),
        .wr_axi_awvalid (wr_axi_awvalid),
        .wr_axi_awready (wr_axi_awready),
        .wr_axi_wdata   (wr_axi_wdata),
        .wr_axi_wstrb   (wr_axi_wstrb),
        .wr_axi_wvalid  (wr_axi_wvalid),
        .wr_axi_wready  (wr_axi_wready),
        .wr_axi_bresp   (wr_axi_bresp),
        .wr_axi_bvalid  (wr_axi_bvalid),
        .wr_axi_bready  (wr_axi_bready),
        .wr_axi_araddr  (wr_axi_araddr),
        .wr_axi_arvalid (wr_axi_arvalid),
        .wr_axi_arready (wr_axi_arready),
        .wr_axi_rdata   (wr_axi_rdata),
        .wr_axi_rresp   (wr_axi_rresp),
        .wr_axi_rvalid  (wr_axi_rvalid),
        .wr_axi_rready  (wr_axi_rready),
        .wr_axi_rlast   (wr_axi_rlast),

        .link_up        (link_up[l]),
        .activity       (activity[l])
      );
    end
  endgenerate

endmodule
`default_nettype wire
