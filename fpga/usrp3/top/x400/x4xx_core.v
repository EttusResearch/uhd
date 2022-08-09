//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0
//
// Module: x4xx_core
//
// Description:
//
//   This module contains the core infrastructure for RFNoC, such as the
//   motherboard register, timekeeper, and RFNoC image core.
//
// Parameters:
//
//   NUM_DBOARDS    : Number of daughter boards
//   REG_DWIDTH     : Width of the AXI4-Lite data bus (must be 32 or 64)
//   REG_AWIDTH     : Width of the address bus
//   CHDR_CLK_RATE  : rfnoc_chdr_clk rate in Hz
//   NUM_CHANNELS   : Total number of channels
//   CHDR_W         : CHDR width used by RFNoC
//   MTU            : Log2 of maximum transmission unit in CHDR_W sized words
//   RFNOC_PROTOVER : RFNoC protocol version (major[7:0], minor[7:0])
//   RADIO_SPC      : Number of samples per radio clock cycle
//   RF_BANDWIDTH   : RF bandwidth of each radio channel
//


module x4xx_core #(
  parameter NUM_DBOARDS    = 2,
  parameter REG_DWIDTH     = 32,
  parameter REG_AWIDTH     = 32,
  parameter CHDR_CLK_RATE  = 200000000,
  parameter NUM_CHANNELS   = 4,
  parameter CHDR_W         = 64,
  parameter MTU            = $clog2(8192 / (CHDR_W/8)),
  parameter RFNOC_PROTOVER = {8'd1, 8'd0},
  parameter RADIO_SPC      = 1,
  parameter RF_BANDWIDTH   = 200
) (
  // Clocks and resets
  input wire radio_clk,
  input wire radio_rst,
  input wire radio_clk_2x,

  input wire rfnoc_chdr_clk,
  input wire rfnoc_chdr_rst,
  input wire rfnoc_ctrl_clk,
  input wire rfnoc_ctrl_rst,

  // DRAM Bank 0
  input wire          dram0_sys_clk_p,
  input wire          dram0_sys_clk_n,
  output wire         dram0_ck_t,
  output wire         dram0_ck_c,
  output wire         dram0_cs_n,
  output wire         dram0_act_n,
  output wire [ 16:0] dram0_adr,
  output wire [  1:0] dram0_ba,
  output wire         dram0_bg,
  output wire         dram0_cke,
  output wire         dram0_odt,
  output wire         dram0_reset_n,
  inout  wire [  7:0] dram0_dm_dbi_n,
  inout  wire [ 63:0] dram0_dq,
  inout  wire [  7:0] dram0_dqs_t,
  inout  wire [  7:0] dram0_dqs_c,

  // DRAM Bank 1
  input wire          dram1_sys_clk_p,
  input wire          dram1_sys_clk_n,
  output wire         dram1_ck_t,
  output wire         dram1_ck_c,
  output wire         dram1_cs_n,
  output wire         dram1_act_n,
  output wire [ 16:0] dram1_adr,
  output wire [  1:0] dram1_ba,
  output wire         dram1_bg,
  output wire         dram1_cke,
  output wire         dram1_odt,
  output wire         dram1_reset_n,
  inout  wire [  7:0] dram1_dm_dbi_n,
  inout  wire [ 63:0] dram1_dq,
  inout  wire [  7:0] dram1_dqs_t,
  inout  wire [  7:0] dram1_dqs_c,

  // AXI-Lite interface (for motherboard registers)
  input                     s_axi_aclk,
  input                     s_axi_aresetn,
  input  [  REG_AWIDTH-1:0] s_axi_awaddr,
  input                     s_axi_awvalid,
  output                    s_axi_awready,
  input  [  REG_DWIDTH-1:0] s_axi_wdata,
  input  [REG_DWIDTH/8-1:0] s_axi_wstrb,
  input                     s_axi_wvalid,
  output                    s_axi_wready,
  output [             1:0] s_axi_bresp,
  output                    s_axi_bvalid,
  input                     s_axi_bready,
  input  [  REG_AWIDTH-1:0] s_axi_araddr,
  input                     s_axi_arvalid,
  output                    s_axi_arready,
  output [  REG_DWIDTH-1:0] s_axi_rdata,
  output [             1:0] s_axi_rresp,
  output                    s_axi_rvalid,
  input                     s_axi_rready,

  // PPS and Clock Control
  input         pps_radioclk,
  output [ 1:0] pps_select,
  output [ 1:0] trig_io_select,
  output        pll_sync_trigger,
  output [ 7:0] pll_sync_delay,
  input         pll_sync_done,
  output [ 7:0] pps_brc_delay,
  output [25:0] pps_prc_delay,
  output [ 1:0] prc_rc_divider,
  output        pps_rc_enabled,

  // Radio Data
  input  [32*RADIO_SPC*NUM_CHANNELS-1:0] rx_data,
  input  [             NUM_CHANNELS-1:0] rx_stb,
  output [             NUM_CHANNELS-1:0] rx_running,
  //
  output [32*RADIO_SPC*NUM_CHANNELS-1:0] tx_data,
  input  [             NUM_CHANNELS-1:0] tx_stb,
  output [             NUM_CHANNELS-1:0] tx_running,

  // DMA
  output [CHDR_W-1:0] dmao_tdata,
  output              dmao_tlast,
  output              dmao_tvalid,
  input               dmao_tready,

  input  [CHDR_W-1:0] dmai_tdata,
  input               dmai_tlast,
  input               dmai_tvalid,
  output              dmai_tready,

  // e2v (Ethernet to CHDR)
  output [CHDR_W*8-1:0] v2e_tdata,
  output [       8-1:0] v2e_tvalid,
  output [       8-1:0] v2e_tlast,
  input  [       8-1:0] v2e_tready,
  // v2e (CHDR to Ethernet)
  input  [CHDR_W*8-1:0] e2v_tdata,
  input  [       8-1:0] e2v_tlast,
  input  [       8-1:0] e2v_tvalid,
  output [       8-1:0] e2v_tready,

  // GPIO to DIO board (Domain: rfnoc_ctrl_clk)
  output wire [11:0] gpio_en_a,
  output wire [11:0] gpio_en_b,
  // GPIO to DIO board (async)
  input  wire [11:0] gpio_in_a,
  input  wire [11:0] gpio_in_b,
  output wire [11:0] gpio_out_a,
  output wire [11:0] gpio_out_b,
  // PS GPIO Control
  input  wire [11:0] ps_gpio_out_a,
  output wire [11:0] ps_gpio_in_a,
  input  wire [11:0] ps_gpio_ddr_a,
  input  wire [11:0] ps_gpio_out_b,
  output wire [11:0] ps_gpio_in_b,
  input  wire [11:0] ps_gpio_ddr_b,

  // Misc
  input  [31:0] qsfp_port_0_0_info,
  input  [31:0] qsfp_port_0_1_info,
  input  [31:0] qsfp_port_0_2_info,
  input  [31:0] qsfp_port_0_3_info,
  input  [31:0] qsfp_port_1_0_info,
  input  [31:0] qsfp_port_1_1_info,
  input  [31:0] qsfp_port_1_2_info,
  input  [31:0] qsfp_port_1_3_info,
  output [15:0] device_id,
  output        mfg_test_en_fabric_clk,
  output        mfg_test_en_gty_rcv_clk,
  input         fpga_aux_ref,

  // Radio Control Ports
  output wire [63:0] radio_time,
  output wire        radio_time_stb,

  output wire [  1*NUM_DBOARDS-1:0] m_ctrlport_radio_req_wr,
  output wire [  1*NUM_DBOARDS-1:0] m_ctrlport_radio_req_rd,
  output wire [ 20*NUM_DBOARDS-1:0] m_ctrlport_radio_req_addr,
  output wire [ 32*NUM_DBOARDS-1:0] m_ctrlport_radio_req_data,
  input  wire [  1*NUM_DBOARDS-1:0] m_ctrlport_radio_resp_ack,
  input  wire [  2*NUM_DBOARDS-1:0] m_ctrlport_radio_resp_status,
  input  wire [ 32*NUM_DBOARDS-1:0] m_ctrlport_radio_resp_data,

  // RF Reset Control
  output wire start_nco_reset,
  input  wire nco_reset_done,
  output wire adc_reset_pulse,
  output wire dac_reset_pulse,

  // Version (Constant)
  // Each component consists of a 96-bit vector (refer to versioning_utils.vh)
  input wire [64*96-1:0] version_info
);

  //---------------------------------------------------------------------------
  // AXI-Lite to CtrlPort Bridge
  //---------------------------------------------------------------------------

  wire [19:0] ctrlport_req_addr;
  wire [ 3:0] ctrlport_req_byte_en;
  wire [31:0] ctrlport_req_data;
  wire        ctrlport_req_has_time;
  wire [ 9:0] ctrlport_req_portid;
  wire        ctrlport_req_rd;
  wire [15:0] ctrlport_req_rem_epid;
  wire [ 9:0] ctrlport_req_rem_portid;
  wire [63:0] ctrlport_req_time;
  wire        ctrlport_req_wr;
  wire        ctrlport_resp_ack;
  wire [31:0] ctrlport_resp_data;
  wire [ 1:0] ctrlport_resp_status;

  `include "../../lib/rfnoc/core/ctrlport.vh"

  axil_ctrlport_master #(
    .TIMEOUT        (10             ),
    .AXI_AWIDTH     (REG_AWIDTH     ),
    .CTRLPORT_AWIDTH(CTRLPORT_ADDR_W)
  ) axil_ctrlport_master_i (
    .s_axi_aclk                (s_axi_aclk),
    .s_axi_aresetn             (s_axi_aresetn),
    .s_axi_awaddr              (s_axi_awaddr),
    .s_axi_awvalid             (s_axi_awvalid),
    .s_axi_awready             (s_axi_awready),
    .s_axi_wdata               (s_axi_wdata),
    .s_axi_wstrb               (s_axi_wstrb),
    .s_axi_wvalid              (s_axi_wvalid),
    .s_axi_wready              (s_axi_wready),
    .s_axi_bresp               (s_axi_bresp),
    .s_axi_bvalid              (s_axi_bvalid),
    .s_axi_bready              (s_axi_bready),
    .s_axi_araddr              (s_axi_araddr),
    .s_axi_arvalid             (s_axi_arvalid),
    .s_axi_arready             (s_axi_arready),
    .s_axi_rdata               (s_axi_rdata),
    .s_axi_rresp               (s_axi_rresp),
    .s_axi_rvalid              (s_axi_rvalid),
    .s_axi_rready              (s_axi_rready),
    .m_ctrlport_req_wr         (ctrlport_req_wr),
    .m_ctrlport_req_rd         (ctrlport_req_rd),
    .m_ctrlport_req_addr       (ctrlport_req_addr),
    .m_ctrlport_req_portid     (ctrlport_req_portid),
    .m_ctrlport_req_rem_epid   (ctrlport_req_rem_epid),
    .m_ctrlport_req_rem_portid (ctrlport_req_rem_portid),
    .m_ctrlport_req_data       (ctrlport_req_data),
    .m_ctrlport_req_byte_en    (ctrlport_req_byte_en),
    .m_ctrlport_req_has_time   (ctrlport_req_has_time),
    .m_ctrlport_req_time       (ctrlport_req_time),
    .m_ctrlport_resp_ack       (ctrlport_resp_ack),
    .m_ctrlport_resp_status    (ctrlport_resp_status),
    .m_ctrlport_resp_data      (ctrlport_resp_data)
  );


  //---------------------------------------------------------------------------
  // Common Components
  //---------------------------------------------------------------------------

  wire ctrlport_rst;

  reset_sync reset_sync_ctrlport (
    .clk       (s_axi_aclk),
    .reset_in  (~s_axi_aresetn),
    .reset_out (ctrlport_rst)
  );

  wire [  1*NUM_DBOARDS-1:0] ctrlport_radio_req_wr;
  wire [  1*NUM_DBOARDS-1:0] ctrlport_radio_req_rd;
  wire [ 20*NUM_DBOARDS-1:0] ctrlport_radio_req_addr;
  wire [ 32*NUM_DBOARDS-1:0] ctrlport_radio_req_data;
  wire [  4*NUM_DBOARDS-1:0] ctrlport_radio_req_byte_en;
  wire [  1*NUM_DBOARDS-1:0] ctrlport_radio_req_has_time;
  wire [ 64*NUM_DBOARDS-1:0] ctrlport_radio_req_time;
  wire [  1*NUM_DBOARDS-1:0] ctrlport_radio_resp_ack;
  wire [  2*NUM_DBOARDS-1:0] ctrlport_radio_resp_status;
  wire [ 32*NUM_DBOARDS-1:0] ctrlport_radio_resp_data;

  x4xx_core_common #(
    .CHDR_CLK_RATE  (CHDR_CLK_RATE),
    .CHDR_W         (CHDR_W),
    .RFNOC_PROTOVER (RFNOC_PROTOVER),
    .NUM_DBOARDS    (NUM_DBOARDS),
    .PCIE_PRESENT   (0)
  ) x4xx_core_common_i (
    .radio_clk                        (radio_clk),
    .radio_clk_2x                     (radio_clk_2x),
    .radio_rst                        (radio_rst),
    .rfnoc_chdr_clk                   (rfnoc_chdr_clk),
    .rfnoc_chdr_rst                   (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk                   (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst                   (rfnoc_ctrl_rst),
    .ctrlport_clk                     (s_axi_aclk),
    .ctrlport_rst                     (ctrlport_rst),
    .s_ctrlport_req_wr                (ctrlport_req_wr),
    .s_ctrlport_req_rd                (ctrlport_req_rd),
    .s_ctrlport_req_addr              (ctrlport_req_addr),
    .s_ctrlport_req_portid            (ctrlport_req_portid),
    .s_ctrlport_req_rem_epid          (ctrlport_req_rem_epid),
    .s_ctrlport_req_rem_portid        (ctrlport_req_rem_portid),
    .s_ctrlport_req_data              (ctrlport_req_data),
    .s_ctrlport_req_byte_en           (ctrlport_req_byte_en),
    .s_ctrlport_req_has_time          (ctrlport_req_has_time),
    .s_ctrlport_req_time              (ctrlport_req_time),
    .s_ctrlport_resp_ack              (ctrlport_resp_ack),
    .s_ctrlport_resp_status           (ctrlport_resp_status),
    .s_ctrlport_resp_data             (ctrlport_resp_data),
    .pps_radioclk                     (pps_radioclk),
    .pps_select                       (pps_select),
    .trig_io_select                   (trig_io_select),
    .pll_sync_trigger                 (pll_sync_trigger),
    .pll_sync_delay                   (pll_sync_delay),
    .pll_sync_done                    (pll_sync_done),
    .pps_brc_delay                    (pps_brc_delay),
    .pps_prc_delay                    (pps_prc_delay),
    .prc_rc_divider                   (prc_rc_divider),
    .pps_rc_enabled                   (pps_rc_enabled),
    .radio_spc                        (RADIO_SPC),
    .radio_time                       (radio_time),
    .sample_rx_stb                    (radio_time_stb),
    .time_ignore_bits                 ($clog2(RADIO_SPC)),
    .gpio_in_a                        (gpio_in_a),
    .gpio_in_b                        (gpio_in_b),
    .gpio_out_a                       (gpio_out_a),
    .gpio_out_b                       (gpio_out_b),
    .gpio_en_a                        (gpio_en_a),
    .gpio_en_b                        (gpio_en_b),
    .gpio_in_fabric_a                 (),
    .gpio_in_fabric_b                 (),
    .gpio_out_fabric_a                (12'b0),
    .gpio_out_fabric_b                (12'b0),
    .ps_gpio_out_a                    (ps_gpio_out_a),
    .ps_gpio_in_a                     (ps_gpio_in_a),
    .ps_gpio_ddr_a                    (ps_gpio_ddr_a),
    .ps_gpio_out_b                    (ps_gpio_out_b),
    .ps_gpio_in_b                     (ps_gpio_in_b),
    .ps_gpio_ddr_b                    (ps_gpio_ddr_b),
    .s_radio_ctrlport_req_wr          (ctrlport_radio_req_wr),
    .s_radio_ctrlport_req_rd          (ctrlport_radio_req_rd),
    .s_radio_ctrlport_req_addr        (ctrlport_radio_req_addr),
    .s_radio_ctrlport_req_data        (ctrlport_radio_req_data),
    .s_radio_ctrlport_req_byte_en     (ctrlport_radio_req_byte_en),
    .s_radio_ctrlport_req_has_time    (ctrlport_radio_req_has_time),
    .s_radio_ctrlport_req_time        (ctrlport_radio_req_time),
    .s_radio_ctrlport_resp_ack        (ctrlport_radio_resp_ack),
    .s_radio_ctrlport_resp_status     (ctrlport_radio_resp_status),
    .s_radio_ctrlport_resp_data       (ctrlport_radio_resp_data),
    .m_radio_ctrlport_req_wr          (m_ctrlport_radio_req_wr),
    .m_radio_ctrlport_req_rd          (m_ctrlport_radio_req_rd),
    .m_radio_ctrlport_req_addr        (m_ctrlport_radio_req_addr),
    .m_radio_ctrlport_req_data        (m_ctrlport_radio_req_data),
    .m_radio_ctrlport_resp_ack        (m_ctrlport_radio_resp_ack),
    .m_radio_ctrlport_resp_status     (m_ctrlport_radio_resp_status),
    .m_radio_ctrlport_resp_data       (m_ctrlport_radio_resp_data),
    .start_nco_reset                  (start_nco_reset),
    .nco_reset_done                   (nco_reset_done),
    .adc_reset_pulse                  (adc_reset_pulse),
    .dac_reset_pulse                  (dac_reset_pulse),
    .tx_running                       (tx_running),
    .rx_running                       (rx_running),
    .qsfp_port_0_0_info               (qsfp_port_0_0_info),
    .qsfp_port_0_1_info               (qsfp_port_0_1_info),
    .qsfp_port_0_2_info               (qsfp_port_0_2_info),
    .qsfp_port_0_3_info               (qsfp_port_0_3_info),
    .qsfp_port_1_0_info               (qsfp_port_1_0_info),
    .qsfp_port_1_1_info               (qsfp_port_1_1_info),
    .qsfp_port_1_2_info               (qsfp_port_1_2_info),
    .qsfp_port_1_3_info               (qsfp_port_1_3_info),
    .device_id                        (device_id),
    .mfg_test_en_fabric_clk           (mfg_test_en_fabric_clk),
    .mfg_test_en_gty_rcv_clk          (mfg_test_en_gty_rcv_clk),
    .fpga_aux_ref                     (fpga_aux_ref),
    .version_info                     (version_info)
  );

  // Provide information for ctrlport timed commands
  assign radio_time_stb = rx_stb[0];


  //---------------------------------------------------------------------------
  // DRAM
  //---------------------------------------------------------------------------

  `ifndef DRAM_CH
    `define DRAM_CH 0
  `endif
  `ifndef DRAM_W
    `define DRAM_W 64
  `endif

  localparam DRAM_AXI_DWIDTH = `DRAM_W;
  localparam ENABLE_DRAM     = (`DRAM_CH > 0);
  localparam DRAM_NUM_PORTS  = `DRAM_CH;

  wire dram0_ui_clk;
  wire dram0_ui_clk_sync_rst;

  wire dram_clk = dram0_ui_clk;
  wire dram_rst = dram0_ui_clk_sync_rst;
  wire sys_rst  = rfnoc_chdr_rst;

  wire [               32*DRAM_NUM_PORTS-1:0] dram_axi_araddr;
  wire [                2*DRAM_NUM_PORTS-1:0] dram_axi_arburst;
  wire [                4*DRAM_NUM_PORTS-1:0] dram_axi_arcache;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_arid;
  wire [                8*DRAM_NUM_PORTS-1:0] dram_axi_arlen;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_arlock;
  wire [                3*DRAM_NUM_PORTS-1:0] dram_axi_arprot;
  wire [                4*DRAM_NUM_PORTS-1:0] dram_axi_arqos;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_arready;
  wire [                4*DRAM_NUM_PORTS-1:0] dram_axi_arregion;
  wire [                3*DRAM_NUM_PORTS-1:0] dram_axi_arsize;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_arvalid;
  wire [               32*DRAM_NUM_PORTS-1:0] dram_axi_awaddr;
  wire [                2*DRAM_NUM_PORTS-1:0] dram_axi_awburst;
  wire [                4*DRAM_NUM_PORTS-1:0] dram_axi_awcache;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_awid;
  wire [                8*DRAM_NUM_PORTS-1:0] dram_axi_awlen;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_awlock;
  wire [                3*DRAM_NUM_PORTS-1:0] dram_axi_awprot;
  wire [                4*DRAM_NUM_PORTS-1:0] dram_axi_awqos;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_awready;
  wire [                4*DRAM_NUM_PORTS-1:0] dram_axi_awregion;
  wire [                3*DRAM_NUM_PORTS-1:0] dram_axi_awsize;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_awvalid;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_bid;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_bready;
  wire [                2*DRAM_NUM_PORTS-1:0] dram_axi_bresp;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_bvalid;
  wire [  DRAM_AXI_DWIDTH*DRAM_NUM_PORTS-1:0] dram_axi_rdata;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_rid;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_rlast;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_rready;
  wire [                2*DRAM_NUM_PORTS-1:0] dram_axi_rresp;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_rvalid;
  wire [  DRAM_AXI_DWIDTH*DRAM_NUM_PORTS-1:0] dram_axi_wdata;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_wlast;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_wready;
  wire [DRAM_AXI_DWIDTH/8*DRAM_NUM_PORTS-1:0] dram_axi_wstrb;
  wire [                1*DRAM_NUM_PORTS-1:0] dram_axi_wvalid;

  x4xx_dram #(
    .ENABLE_DRAM (ENABLE_DRAM),
    .AXI_DWIDTH  (DRAM_AXI_DWIDTH),
    .NUM_PORTS   (DRAM_NUM_PORTS)
  ) x4xx_dram_i (
    .sys_rst                  (sys_rst),
    .dram0_sys_clk_p          (dram0_sys_clk_p),
    .dram0_sys_clk_n          (dram0_sys_clk_n),
    .dram1_sys_clk_p          (dram1_sys_clk_p),
    .dram1_sys_clk_n          (dram1_sys_clk_n),
    .dram0_ui_clk             (dram0_ui_clk),
    .dram0_ui_clk_sync_rst    (dram0_ui_clk_sync_rst),
    .dram1_ui_clk             (),
    .dram1_ui_clk_sync_rst    (),
    .dram0_ck_t               (dram0_ck_t),
    .dram0_ck_c               (dram0_ck_c),
    .dram0_cs_n               (dram0_cs_n),
    .dram0_act_n              (dram0_act_n),
    .dram0_adr                (dram0_adr),
    .dram0_ba                 (dram0_ba),
    .dram0_bg                 (dram0_bg),
    .dram0_cke                (dram0_cke),
    .dram0_odt                (dram0_odt),
    .dram0_reset_n            (dram0_reset_n),
    .dram0_dm_dbi_n           (dram0_dm_dbi_n),
    .dram0_dq                 (dram0_dq),
    .dram0_dqs_t              (dram0_dqs_t),
    .dram0_dqs_c              (dram0_dqs_c),
    .dram1_ck_t               (dram1_ck_t),
    .dram1_ck_c               (dram1_ck_c),
    .dram1_cs_n               (dram1_cs_n),
    .dram1_act_n              (dram1_act_n),
    .dram1_adr                (dram1_adr),
    .dram1_ba                 (dram1_ba),
    .dram1_bg                 (dram1_bg),
    .dram1_cke                (dram1_cke),
    .dram1_odt                (dram1_odt),
    .dram1_reset_n            (dram1_reset_n),
    .dram1_dm_dbi_n           (dram1_dm_dbi_n),
    .dram1_dq                 (dram1_dq),
    .dram1_dqs_t              (dram1_dqs_t),
    .dram1_dqs_c              (dram1_dqs_c),
    .dram_clk                 (dram_clk),
    .dram_rst                 (dram_rst),
    .dram_init_calib_complete (),
    .dram_axi_araddr          (dram_axi_araddr),
    .dram_axi_arburst         (dram_axi_arburst),
    .dram_axi_arcache         (dram_axi_arcache),
    .dram_axi_arid            (dram_axi_arid),
    .dram_axi_arlen           (dram_axi_arlen),
    .dram_axi_arlock          (dram_axi_arlock),
    .dram_axi_arprot          (dram_axi_arprot),
    .dram_axi_arqos           (dram_axi_arqos),
    .dram_axi_arready         (dram_axi_arready),
    .dram_axi_arregion        (dram_axi_arregion),
    .dram_axi_arsize          (dram_axi_arsize),
    .dram_axi_arvalid         (dram_axi_arvalid),
    .dram_axi_awaddr          (dram_axi_awaddr),
    .dram_axi_awburst         (dram_axi_awburst),
    .dram_axi_awcache         (dram_axi_awcache),
    .dram_axi_awid            (dram_axi_awid),
    .dram_axi_awlen           (dram_axi_awlen),
    .dram_axi_awlock          (dram_axi_awlock),
    .dram_axi_awprot          (dram_axi_awprot),
    .dram_axi_awqos           (dram_axi_awqos),
    .dram_axi_awready         (dram_axi_awready),
    .dram_axi_awregion        (dram_axi_awregion),
    .dram_axi_awsize          (dram_axi_awsize),
    .dram_axi_awvalid         (dram_axi_awvalid),
    .dram_axi_bid             (dram_axi_bid),
    .dram_axi_bready          (dram_axi_bready),
    .dram_axi_bresp           (dram_axi_bresp),
    .dram_axi_bvalid          (dram_axi_bvalid),
    .dram_axi_rdata           (dram_axi_rdata),
    .dram_axi_rid             (dram_axi_rid),
    .dram_axi_rlast           (dram_axi_rlast),
    .dram_axi_rready          (dram_axi_rready),
    .dram_axi_rresp           (dram_axi_rresp),
    .dram_axi_rvalid          (dram_axi_rvalid),
    .dram_axi_wdata           (dram_axi_wdata),
    .dram_axi_wlast           (dram_axi_wlast),
    .dram_axi_wready          (dram_axi_wready),
    .dram_axi_wstrb           (dram_axi_wstrb),
    .dram_axi_wvalid          (dram_axi_wvalid)
  );


  //---------------------------------------------------------------------------
  // RFNoC Image Core
  //---------------------------------------------------------------------------

  // Calculate how may bits wide each channel is
  localparam CHAN_W = 32 * RADIO_SPC;

  rfnoc_image_core #(
    .CHDR_W     (CHDR_W),
    .MTU        (MTU),
    .PROTOVER   (RFNOC_PROTOVER),
    .RADIO_NIPC (RADIO_SPC)
  ) rfnoc_image_core_i (
    .chdr_aclk                      (rfnoc_chdr_clk),
    .ctrl_aclk                      (rfnoc_ctrl_clk),
    .core_arst                      (rfnoc_ctrl_rst),
    .radio_clk                      (radio_clk),
    .radio_2x_clk                   (radio_clk_2x),
    .dram_clk                       (dram_clk),
    .device_id                      (device_id),
    .m_ctrlport_radio0_req_wr       (ctrlport_radio_req_wr      [0* 1+: 1]),
    .m_ctrlport_radio0_req_rd       (ctrlport_radio_req_rd      [0* 1+: 1]),
    .m_ctrlport_radio0_req_addr     (ctrlport_radio_req_addr    [0*20+:20]),
    .m_ctrlport_radio0_req_data     (ctrlport_radio_req_data    [0*32+:32]),
    .m_ctrlport_radio0_req_byte_en  (ctrlport_radio_req_byte_en [0* 4+: 4]),
    .m_ctrlport_radio0_req_has_time (ctrlport_radio_req_has_time[0* 1+: 1]),
    .m_ctrlport_radio0_req_time     (ctrlport_radio_req_time    [0*64+:64]),
    .m_ctrlport_radio0_resp_ack     (ctrlport_radio_resp_ack    [0* 1+: 1]),
    .m_ctrlport_radio0_resp_status  (ctrlport_radio_resp_status [0* 2+: 2]),
    .m_ctrlport_radio0_resp_data    (ctrlport_radio_resp_data   [0*32+:32]),
    .m_ctrlport_radio1_req_wr       (ctrlport_radio_req_wr      [1* 1+: 1]),
    .m_ctrlport_radio1_req_rd       (ctrlport_radio_req_rd      [1* 1+: 1]),
    .m_ctrlport_radio1_req_addr     (ctrlport_radio_req_addr    [1*20+:20]),
    .m_ctrlport_radio1_req_data     (ctrlport_radio_req_data    [1*32+:32]),
    .m_ctrlport_radio1_req_byte_en  (ctrlport_radio_req_byte_en [1* 4+: 4]),
    .m_ctrlport_radio1_req_has_time (ctrlport_radio_req_has_time[1* 1+: 1]),
    .m_ctrlport_radio1_req_time     (ctrlport_radio_req_time    [1*64+:64]),
    .m_ctrlport_radio1_resp_ack     (ctrlport_radio_resp_ack    [1* 1+: 1]),
    .m_ctrlport_radio1_resp_status  (ctrlport_radio_resp_status [1* 2+: 2]),
    .m_ctrlport_radio1_resp_data    (ctrlport_radio_resp_data   [1*32+:32]),
    .radio_rx_stb_radio1            ({    rx_stb[3],                 rx_stb[2]               }),
    .radio_rx_data_radio1           ({   rx_data[3*CHAN_W+:CHAN_W], rx_data[2*CHAN_W+:CHAN_W]}),
    .radio_rx_running_radio1        ({rx_running[3],             rx_running[2]               }),
    .radio_tx_stb_radio1            ({    tx_stb[3],                 tx_stb[2]               }),
    .radio_tx_data_radio1           ({   tx_data[3*CHAN_W+:CHAN_W], tx_data[2*CHAN_W+:CHAN_W]}),
    .radio_tx_running_radio1        ({tx_running[3],             tx_running[2]               }),
    .radio_rx_stb_radio0            ({    rx_stb[1],                 rx_stb[0]               }),
    .radio_rx_data_radio0           ({   rx_data[1*CHAN_W+:CHAN_W], rx_data[0*CHAN_W+:CHAN_W]}),
    .radio_rx_running_radio0        ({rx_running[1],             rx_running[0]               }),
    .radio_tx_stb_radio0            ({    tx_stb[1],                 tx_stb[0]               }),
    .radio_tx_data_radio0           ({   tx_data[1*CHAN_W+:CHAN_W], tx_data[0*CHAN_W+:CHAN_W]}),
    .radio_tx_running_radio0        ({tx_running[1],             tx_running[0]               }),
    .radio_time                     (radio_time),
    .axi_rst                        (dram_rst),
    .m_axi_awid                     (dram_axi_awid),
    .m_axi_awaddr                   (dram_axi_awaddr),
    .m_axi_awlen                    (dram_axi_awlen),
    .m_axi_awsize                   (dram_axi_awsize),
    .m_axi_awburst                  (dram_axi_awburst),
    .m_axi_awlock                   (dram_axi_awlock),
    .m_axi_awcache                  (dram_axi_awcache),
    .m_axi_awprot                   (dram_axi_awprot),
    .m_axi_awqos                    (dram_axi_awqos),
    .m_axi_awregion                 (dram_axi_awregion),
    .m_axi_awuser                   (),
    .m_axi_awvalid                  (dram_axi_awvalid),
    .m_axi_awready                  (dram_axi_awready),
    .m_axi_wdata                    (dram_axi_wdata),
    .m_axi_wstrb                    (dram_axi_wstrb),
    .m_axi_wlast                    (dram_axi_wlast),
    .m_axi_wuser                    (),
    .m_axi_wvalid                   (dram_axi_wvalid),
    .m_axi_wready                   (dram_axi_wready),
    .m_axi_bid                      (dram_axi_bid),
    .m_axi_bresp                    (dram_axi_bresp),
    .m_axi_buser                    (0),
    .m_axi_bvalid                   (dram_axi_bvalid),
    .m_axi_bready                   (dram_axi_bready),
    .m_axi_arid                     (dram_axi_arid),
    .m_axi_araddr                   (dram_axi_araddr),
    .m_axi_arlen                    (dram_axi_arlen),
    .m_axi_arsize                   (dram_axi_arsize),
    .m_axi_arburst                  (dram_axi_arburst),
    .m_axi_arlock                   (dram_axi_arlock),
    .m_axi_arcache                  (dram_axi_arcache),
    .m_axi_arprot                   (dram_axi_arprot),
    .m_axi_arqos                    (dram_axi_arqos),
    .m_axi_arregion                 (dram_axi_arregion),
    .m_axi_aruser                   (),
    .m_axi_arvalid                  (dram_axi_arvalid),
    .m_axi_arready                  (dram_axi_arready),
    .m_axi_rid                      (dram_axi_rid),
    .m_axi_rdata                    (dram_axi_rdata),
    .m_axi_rresp                    (dram_axi_rresp),
    .m_axi_rlast                    (dram_axi_rlast),
    .m_axi_ruser                    (0),
    .m_axi_rvalid                   (dram_axi_rvalid),
    .m_axi_rready                   (dram_axi_rready),
    .s_eth0_tdata                   (e2v_tdata  [0*CHDR_W +: CHDR_W]),
    .s_eth0_tlast                   (e2v_tlast  [0*     1 +:      1]),
    .s_eth0_tvalid                  (e2v_tvalid [0*     1 +:      1]),
    .s_eth0_tready                  (e2v_tready [0*     1 +:      1]),
    .m_eth0_tdata                   (v2e_tdata  [0*CHDR_W +: CHDR_W]),
    .m_eth0_tlast                   (v2e_tlast  [0*     1 +:      1]),
    .m_eth0_tvalid                  (v2e_tvalid [0*     1 +:      1]),
    .m_eth0_tready                  (v2e_tready [0*     1 +:      1]),
    .s_eth1_tdata                   (e2v_tdata  [1*CHDR_W +: CHDR_W]),
    .s_eth1_tlast                   (e2v_tlast  [1*     1 +:      1]),
    .s_eth1_tvalid                  (e2v_tvalid [1*     1 +:      1]),
    .s_eth1_tready                  (e2v_tready [1*     1 +:      1]),
    .m_eth1_tdata                   (v2e_tdata  [1*CHDR_W +: CHDR_W]),
    .m_eth1_tlast                   (v2e_tlast  [1*     1 +:      1]),
    .m_eth1_tvalid                  (v2e_tvalid [1*     1 +:      1]),
    .m_eth1_tready                  (v2e_tready [1*     1 +:      1]),
    .s_eth2_tdata                   (e2v_tdata  [2*CHDR_W +: CHDR_W]),
    .s_eth2_tlast                   (e2v_tlast  [2*     1 +:      1]),
    .s_eth2_tvalid                  (e2v_tvalid [2*     1 +:      1]),
    .s_eth2_tready                  (e2v_tready [2*     1 +:      1]),
    .m_eth2_tdata                   (v2e_tdata  [2*CHDR_W +: CHDR_W]),
    .m_eth2_tlast                   (v2e_tlast  [2*     1 +:      1]),
    .m_eth2_tvalid                  (v2e_tvalid [2*     1 +:      1]),
    .m_eth2_tready                  (v2e_tready [2*     1 +:      1]),
    .s_eth3_tdata                   (e2v_tdata  [3*CHDR_W +: CHDR_W]),
    .s_eth3_tlast                   (e2v_tlast  [3*     1 +:      1]),
    .s_eth3_tvalid                  (e2v_tvalid [3*     1 +:      1]),
    .s_eth3_tready                  (e2v_tready [3*     1 +:      1]),
    .m_eth3_tdata                   (v2e_tdata  [3*CHDR_W +: CHDR_W]),
    .m_eth3_tlast                   (v2e_tlast  [3*     1 +:      1]),
    .m_eth3_tvalid                  (v2e_tvalid [3*     1 +:      1]),
    .m_eth3_tready                  (v2e_tready [3*     1 +:      1]),
    .s_eth4_tdata                   (e2v_tdata  [4*CHDR_W +: CHDR_W]),
    .s_eth4_tlast                   (e2v_tlast  [4*     1 +:      1]),
    .s_eth4_tvalid                  (e2v_tvalid [4*     1 +:      1]),
    .s_eth4_tready                  (e2v_tready [4*     1 +:      1]),
    .m_eth4_tdata                   (v2e_tdata  [4*CHDR_W +: CHDR_W]),
    .m_eth4_tlast                   (v2e_tlast  [4*     1 +:      1]),
    .m_eth4_tvalid                  (v2e_tvalid [4*     1 +:      1]),
    .m_eth4_tready                  (v2e_tready [4*     1 +:      1]),
    .s_dma_tdata                    (dmai_tdata),
    .s_dma_tlast                    (dmai_tlast),
    .s_dma_tvalid                   (dmai_tvalid),
    .s_dma_tready                   (dmai_tready),
    .m_dma_tdata                    (dmao_tdata),
    .m_dma_tlast                    (dmao_tlast),
    .m_dma_tvalid                   (dmao_tvalid),
    .m_dma_tready                   (dmao_tready)
  );

endmodule
