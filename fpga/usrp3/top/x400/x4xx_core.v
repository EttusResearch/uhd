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
//   NUM_DBOARDS     : Number of daughter boards
//   REG_DWIDTH      : Width of the AXI4-Lite data bus (must be 32 or 64)
//   REG_AWIDTH      : Width of the address bus
//   CHDR_CLK_RATE   : rfnoc_chdr_clk rate in Hz
//   NUM_CHANNELS    : Total number of channels
//   NUM_CH_PER_DB   : Number of channels per daughterboard and radio core
//   CHDR_W          : CHDR width used by RFNoC
//   DMA_W           : Width of the DMA port interface
//   NET_CHDR_W      : CHDR width used by the network interface ports
//   ENET_W          : Width of the Ethernet buses (each port may have a unique
//                     width, but all signals have equal width for simplicity).
//   ENET_WIDTHS     : Actual width of each Ethernet port's interface. This is
//                     a packed array of 32-bit integers with port 0 in the
//                     right-most position.
//   MTU             : Log2 of maximum transmission unit in CHDR_W sized words
//   RFNOC_PROTOVER  : RFNoC protocol version (major[7:0], minor[7:0])
//   RADIO_SPC       : Number of samples per radio clock cycle
//   NUM_TIMEKEEPERS : Number of timekeepers
//   RF_BANDWIDTH    : RF bandwidth of each radio channel
//


module x4xx_core #(
  parameter            NUM_DBOARDS     = 2,
  parameter            REG_DWIDTH      = 32,
  parameter            REG_AWIDTH      = 32,
  parameter            CHDR_CLK_RATE   = 200000000,
  parameter            NUM_CH_PER_DB   = 2,
  parameter            NUM_CHANNELS    = NUM_CH_PER_DB*NUM_DBOARDS,
  parameter            CHDR_W          = 64,
  parameter            DMA_W           = 64,
  parameter            NET_CHDR_W      = 64,
  parameter [    31:0] ENET_W          = 64,
  parameter [32*8-1:0] ENET_WIDTHS     = {8{ENET_W}},
  parameter            MTU             = $clog2(8192 / (CHDR_W/8)),
  parameter            RFNOC_PROTOVER  = {8'd1, 8'd0},
  parameter            RADIO_SPC       = 1,
  parameter            NUM_TIMEKEEPERS = NUM_DBOARDS,
  parameter            RF_BANDWIDTH    = 200
) (
  // Clocks and resets
  input wire [NUM_DBOARDS-1:0] radio_clk,
  input wire [NUM_DBOARDS-1:0] radio_rst,
  input wire [NUM_DBOARDS-1:0] radio_clk_2x,

  input wire rfnoc_chdr_clk,
  input wire rfnoc_chdr_rst,
  input wire rfnoc_ctrl_clk,
  input wire rfnoc_ctrl_rst,

  input wire ce_clk,

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
  input  [ 1:0] pps_radioclk,
  output [ 1:0] pps_select,
  output [ 1:0] trig_io_select,
  output        pll_sync_trigger,
  output [ 7:0] pll_sync_delay,
  input         pll_sync_done,
  output [ 7:0] pps_brc_delay,
  output [25:0] pps_prc_delay,
  output [ 4:0] prc_rc0_divider,
  output [ 4:0] prc_rc1_divider,
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
  output [DMA_W-1:0] dmao_tdata,
  output             dmao_tlast,
  output             dmao_tvalid,
  input              dmao_tready,

  input  [DMA_W-1:0] dmai_tdata,
  input              dmai_tlast,
  input              dmai_tvalid,
  output             dmai_tready,

  // e2v (Ethernet to CHDR)
  output [ENET_W*8-1:0] v2e_tdata,
  output [       8-1:0] v2e_tvalid,
  output [       8-1:0] v2e_tlast,
  input  [       8-1:0] v2e_tready,
  // v2e (CHDR to Ethernet)
  input  [ENET_W*8-1:0] e2v_tdata,
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
  output wire [64*NUM_TIMEKEEPERS-1:0] radio_time,
  output wire [   NUM_TIMEKEEPERS-1:0] radio_time_stb,

  output wire [  1*NUM_DBOARDS-1:0] m_ctrlport_radio_req_wr,
  output wire [  1*NUM_DBOARDS-1:0] m_ctrlport_radio_req_rd,
  output wire [ 20*NUM_DBOARDS-1:0] m_ctrlport_radio_req_addr,
  output wire [ 32*NUM_DBOARDS-1:0] m_ctrlport_radio_req_data,
  input  wire [  1*NUM_DBOARDS-1:0] m_ctrlport_radio_resp_ack,
  input  wire [  2*NUM_DBOARDS-1:0] m_ctrlport_radio_resp_status,
  input  wire [ 32*NUM_DBOARDS-1:0] m_ctrlport_radio_resp_data,

  // RF Reset Control
  output wire                       start_nco_reset,
  input  wire                       nco_reset_done,
  output wire [NUM_TIMEKEEPERS-1:0] adc_reset_pulse,
  output wire [NUM_TIMEKEEPERS-1:0] dac_reset_pulse,

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
    .CHDR_CLK_RATE   (CHDR_CLK_RATE),
    .CHDR_W          (NET_CHDR_W),
    .RFNOC_PROTOVER  (RFNOC_PROTOVER),
    .NUM_DBOARDS     (NUM_DBOARDS),
    .NUM_CH_PER_DB   (NUM_CH_PER_DB),
    .NUM_TIMEKEEPERS (NUM_TIMEKEEPERS),
    .PCIE_PRESENT    (0)
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
    .prc_rc0_divider                  (prc_rc0_divider),
    .prc_rc1_divider                  (prc_rc1_divider),
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
  genvar tk_i;
  generate
    for (tk_i = 0; tk_i < NUM_TIMEKEEPERS; tk_i = tk_i + 1) begin : gen_time_stb
      assign radio_time_stb[tk_i] = rx_stb[tk_i*NUM_CH_PER_DB];
    end
  endgenerate


  //---------------------------------------------------------------------------
  // DRAM
  //---------------------------------------------------------------------------

  `ifndef DRAM_CH
    `define DRAM_CH 0
  `endif
  `ifndef DRAM_BANKS
    `define DRAM_BANKS 0
  `endif
  `ifndef DRAM_W
    `define DRAM_W 64
  `endif

  localparam ENABLE_DRAM         = (`DRAM_CH > 0 && `DRAM_BANKS > 0);
  localparam DRAM_AXI_DWIDTH     = `DRAM_W;
  localparam DRAM_NUM_BANKS      = ENABLE_DRAM ? `DRAM_BANKS               : 1;
  localparam DRAM_PORTS_PER_BANK = ENABLE_DRAM ? `DRAM_CH / DRAM_NUM_BANKS : 1;

  wire dram0_ui_clk;
  wire dram0_ui_clk_sync_rst;

  wire dram_clk = dram0_ui_clk;
  wire dram_rst = dram0_ui_clk_sync_rst;
  wire sys_rst  = rfnoc_chdr_rst;

  wire [               32*DRAM_PORTS_PER_BANK-1:0] dram0_axi_araddr;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arburst;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arcache;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arid;
  wire [                8*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arlen;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arlock;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arprot;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arqos;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arready;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arregion;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arsize;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_arvalid;
  wire [               32*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awaddr;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awburst;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awcache;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awid;
  wire [                8*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awlen;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awlock;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awprot;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awqos;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awready;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awregion;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awsize;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_awvalid;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_bid;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_bready;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram0_axi_bresp;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_bvalid;
  wire [  DRAM_AXI_DWIDTH*DRAM_PORTS_PER_BANK-1:0] dram0_axi_rdata;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_rid;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_rlast;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_rready;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram0_axi_rresp;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_rvalid;
  wire [  DRAM_AXI_DWIDTH*DRAM_PORTS_PER_BANK-1:0] dram0_axi_wdata;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_wlast;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_wready;
  wire [DRAM_AXI_DWIDTH/8*DRAM_PORTS_PER_BANK-1:0] dram0_axi_wstrb;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram0_axi_wvalid;

  wire [               32*DRAM_PORTS_PER_BANK-1:0] dram1_axi_araddr;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arburst;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arcache;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arid;
  wire [                8*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arlen;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arlock;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arprot;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arqos;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arready;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arregion;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arsize;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_arvalid;
  wire [               32*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awaddr;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awburst;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awcache;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awid;
  wire [                8*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awlen;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awlock;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awprot;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awqos;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awready;
  wire [                4*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awregion;
  wire [                3*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awsize;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_awvalid;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_bid;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_bready;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram1_axi_bresp;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_bvalid;
  wire [  DRAM_AXI_DWIDTH*DRAM_PORTS_PER_BANK-1:0] dram1_axi_rdata;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_rid;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_rlast;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_rready;
  wire [                2*DRAM_PORTS_PER_BANK-1:0] dram1_axi_rresp;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_rvalid;
  wire [  DRAM_AXI_DWIDTH*DRAM_PORTS_PER_BANK-1:0] dram1_axi_wdata;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_wlast;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_wready;
  wire [DRAM_AXI_DWIDTH/8*DRAM_PORTS_PER_BANK-1:0] dram1_axi_wstrb;
  wire [                1*DRAM_PORTS_PER_BANK-1:0] dram1_axi_wvalid;

  x4xx_dram #(
    .ENABLE_DRAM    (ENABLE_DRAM),
    .AXI_DWIDTH     (DRAM_AXI_DWIDTH),
    .NUM_BANKS      (DRAM_NUM_BANKS),
    .PORTS_PER_BANK (DRAM_PORTS_PER_BANK)
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
    .dram0_axi_araddr         (dram0_axi_araddr),
    .dram0_axi_arburst        (dram0_axi_arburst),
    .dram0_axi_arcache        (dram0_axi_arcache),
    .dram0_axi_arid           (dram0_axi_arid),
    .dram0_axi_arlen          (dram0_axi_arlen),
    .dram0_axi_arlock         (dram0_axi_arlock),
    .dram0_axi_arprot         (dram0_axi_arprot),
    .dram0_axi_arqos          (dram0_axi_arqos),
    .dram0_axi_arready        (dram0_axi_arready),
    .dram0_axi_arregion       (dram0_axi_arregion),
    .dram0_axi_arsize         (dram0_axi_arsize),
    .dram0_axi_arvalid        (dram0_axi_arvalid),
    .dram0_axi_awaddr         (dram0_axi_awaddr),
    .dram0_axi_awburst        (dram0_axi_awburst),
    .dram0_axi_awcache        (dram0_axi_awcache),
    .dram0_axi_awid           (dram0_axi_awid),
    .dram0_axi_awlen          (dram0_axi_awlen),
    .dram0_axi_awlock         (dram0_axi_awlock),
    .dram0_axi_awprot         (dram0_axi_awprot),
    .dram0_axi_awqos          (dram0_axi_awqos),
    .dram0_axi_awready        (dram0_axi_awready),
    .dram0_axi_awregion       (dram0_axi_awregion),
    .dram0_axi_awsize         (dram0_axi_awsize),
    .dram0_axi_awvalid        (dram0_axi_awvalid),
    .dram0_axi_bid            (dram0_axi_bid),
    .dram0_axi_bready         (dram0_axi_bready),
    .dram0_axi_bresp          (dram0_axi_bresp),
    .dram0_axi_bvalid         (dram0_axi_bvalid),
    .dram0_axi_rdata          (dram0_axi_rdata),
    .dram0_axi_rid            (dram0_axi_rid),
    .dram0_axi_rlast          (dram0_axi_rlast),
    .dram0_axi_rready         (dram0_axi_rready),
    .dram0_axi_rresp          (dram0_axi_rresp),
    .dram0_axi_rvalid         (dram0_axi_rvalid),
    .dram0_axi_wdata          (dram0_axi_wdata),
    .dram0_axi_wlast          (dram0_axi_wlast),
    .dram0_axi_wready         (dram0_axi_wready),
    .dram0_axi_wstrb          (dram0_axi_wstrb),
    .dram0_axi_wvalid         (dram0_axi_wvalid),
    .dram1_axi_araddr         (dram1_axi_araddr),
    .dram1_axi_arburst        (dram1_axi_arburst),
    .dram1_axi_arcache        (dram1_axi_arcache),
    .dram1_axi_arid           (dram1_axi_arid),
    .dram1_axi_arlen          (dram1_axi_arlen),
    .dram1_axi_arlock         (dram1_axi_arlock),
    .dram1_axi_arprot         (dram1_axi_arprot),
    .dram1_axi_arqos          (dram1_axi_arqos),
    .dram1_axi_arready        (dram1_axi_arready),
    .dram1_axi_arregion       (dram1_axi_arregion),
    .dram1_axi_arsize         (dram1_axi_arsize),
    .dram1_axi_arvalid        (dram1_axi_arvalid),
    .dram1_axi_awaddr         (dram1_axi_awaddr),
    .dram1_axi_awburst        (dram1_axi_awburst),
    .dram1_axi_awcache        (dram1_axi_awcache),
    .dram1_axi_awid           (dram1_axi_awid),
    .dram1_axi_awlen          (dram1_axi_awlen),
    .dram1_axi_awlock         (dram1_axi_awlock),
    .dram1_axi_awprot         (dram1_axi_awprot),
    .dram1_axi_awqos          (dram1_axi_awqos),
    .dram1_axi_awready        (dram1_axi_awready),
    .dram1_axi_awregion       (dram1_axi_awregion),
    .dram1_axi_awsize         (dram1_axi_awsize),
    .dram1_axi_awvalid        (dram1_axi_awvalid),
    .dram1_axi_bid            (dram1_axi_bid),
    .dram1_axi_bready         (dram1_axi_bready),
    .dram1_axi_bresp          (dram1_axi_bresp),
    .dram1_axi_bvalid         (dram1_axi_bvalid),
    .dram1_axi_rdata          (dram1_axi_rdata),
    .dram1_axi_rid            (dram1_axi_rid),
    .dram1_axi_rlast          (dram1_axi_rlast),
    .dram1_axi_rready         (dram1_axi_rready),
    .dram1_axi_rresp          (dram1_axi_rresp),
    .dram1_axi_rvalid         (dram1_axi_rvalid),
    .dram1_axi_wdata          (dram1_axi_wdata),
    .dram1_axi_wlast          (dram1_axi_wlast),
    .dram1_axi_wready         (dram1_axi_wready),
    .dram1_axi_wstrb          (dram1_axi_wstrb),
    .dram1_axi_wvalid         (dram1_axi_wvalid)
  );


  //---------------------------------------------------------------------------
  // RFNoC Image Core
  //---------------------------------------------------------------------------

  // Calculate how many bits wide each channel is
  localparam CHAN_W = 32 * RADIO_SPC;
  wire [NUM_CH_PER_DB-1:0]        rx_stb0, rx_stb1;
  wire [NUM_CH_PER_DB-1:0]        tx_stb0, tx_stb1;
  wire [NUM_CH_PER_DB-1:0]        rx_running0, rx_running1;
  wire [NUM_CH_PER_DB-1:0]        tx_running0, tx_running1;
  wire [CHAN_W*NUM_CH_PER_DB-1:0] rx_data0, rx_data1;
  wire [CHAN_W*NUM_CH_PER_DB-1:0] tx_data0, tx_data1;

  // Separate out signals to the radio blocks, 1 per dboard, 2 assumed dboards
  // [signal name]0 is lower half of a signal, and [signal name]1 is top half
  // ex: rx_stb is 1 bit per channel with a width of NUM_CHANNELS
  //     rx_stb0 is lower half, NUM_CH_PER_DB-1:0
  //     rx_stb1 is top half, starting at NUM_CH_PER_DB
  // Datapaths follow the same pattern but with CHAN_W factored in
  // output paths can just concatenate top/bottom half signals
  assign rx_stb0    = rx_stb[NUM_CH_PER_DB*0+:NUM_CH_PER_DB];
  assign rx_stb1    = rx_stb[NUM_CH_PER_DB*1+:NUM_CH_PER_DB];
  assign rx_running = { rx_running1, rx_running0};
  assign rx_data0   = rx_data[CHAN_W*NUM_CH_PER_DB*0+:CHAN_W*NUM_CH_PER_DB];
  assign rx_data1   = rx_data[CHAN_W*NUM_CH_PER_DB*1+:CHAN_W*NUM_CH_PER_DB];
  assign tx_stb0    = tx_stb[NUM_CH_PER_DB*0+:NUM_CH_PER_DB];
  assign tx_stb1    = tx_stb[NUM_CH_PER_DB*1+:NUM_CH_PER_DB];
  assign tx_running = { tx_running1, tx_running0};
  assign tx_data    = { tx_data1, tx_data0};

  localparam PORT_W  = 512;  // Set to max value; unused bits will be ignored.
  localparam ENET0_W = ENET_WIDTHS[32*0 +: 32];
  localparam ENET1_W = ENET_WIDTHS[32*1 +: 32];
  localparam ENET2_W = ENET_WIDTHS[32*2 +: 32];
  localparam ENET3_W = ENET_WIDTHS[32*3 +: 32];
  localparam ENET4_W = ENET_WIDTHS[32*4 +: 32];

  rfnoc_image_core #(
    .CHDR_W     (CHDR_W),
    .PORT_W     (PORT_W),
    .ETH0_W     (ENET0_W),
    .ETH1_W     (ENET1_W),
    .ETH2_W     (ENET2_W),
    .ETH3_W     (ENET3_W),
    .ETH4_W     (ENET4_W),
    .DMA_W      (DMA_W),
    .MTU        (MTU),
    .PROTOVER   (RFNOC_PROTOVER),
    .RADIO_NIPC (RADIO_SPC)
  ) rfnoc_image_core_i (
    .chdr_aclk                      (rfnoc_chdr_clk),
    .ctrl_aclk                      (rfnoc_ctrl_clk),
    .core_arst                      (rfnoc_ctrl_rst),
  `ifdef X440
    .radio0_clk                     (radio_clk[0]),
    .radio0_2x_clk                  (radio_clk_2x[0]),
    .radio1_clk                     (radio_clk[1]),
    .radio1_2x_clk                  (radio_clk_2x[1]),
  `else
    .radio_clk                      (radio_clk[0]),
    .radio_2x_clk                   (radio_clk_2x[0]),
  `endif
    .dram_clk                       (dram_clk),
    .ce_clk                         (ce_clk),
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
    .radio_rx_stb_radio1            ({       rx_stb1}),
    .radio_rx_data_radio1           ({      rx_data1}),
    .radio_rx_running_radio1        ({   rx_running1}),
    .radio_tx_stb_radio1            ({       tx_stb1}),
    .radio_tx_data_radio1           ({      tx_data1}),
    .radio_tx_running_radio1        ({   tx_running1}),
    .radio_rx_stb_radio0            ({       rx_stb0}),
    .radio_rx_data_radio0           ({      rx_data0}),
    .radio_rx_running_radio0        ({   rx_running0}),
    .radio_tx_stb_radio0            ({       tx_stb0}),
    .radio_tx_data_radio0           ({      tx_data0}),
    .radio_tx_running_radio0        ({   tx_running0}),
  `ifdef X440
    .radio_time0                    (radio_time[0*64+:64]),
    .radio_time1                    (radio_time[1*64+:64]),
  `else
    .radio_time                     (radio_time[0*64+:64]),
  `endif
    .dram0_axi_rst                  (dram_rst),
    .dram0_m_axi_awid               (dram0_axi_awid),
    .dram0_m_axi_awaddr             (dram0_axi_awaddr),
    .dram0_m_axi_awlen              (dram0_axi_awlen),
    .dram0_m_axi_awsize             (dram0_axi_awsize),
    .dram0_m_axi_awburst            (dram0_axi_awburst),
    .dram0_m_axi_awlock             (dram0_axi_awlock),
    .dram0_m_axi_awcache            (dram0_axi_awcache),
    .dram0_m_axi_awprot             (dram0_axi_awprot),
    .dram0_m_axi_awqos              (dram0_axi_awqos),
    .dram0_m_axi_awregion           (dram0_axi_awregion),
    .dram0_m_axi_awuser             (),
    .dram0_m_axi_awvalid            (dram0_axi_awvalid),
    .dram0_m_axi_awready            (dram0_axi_awready),
    .dram0_m_axi_wdata              (dram0_axi_wdata),
    .dram0_m_axi_wstrb              (dram0_axi_wstrb),
    .dram0_m_axi_wlast              (dram0_axi_wlast),
    .dram0_m_axi_wuser              (),
    .dram0_m_axi_wvalid             (dram0_axi_wvalid),
    .dram0_m_axi_wready             (dram0_axi_wready),
    .dram0_m_axi_bid                (dram0_axi_bid),
    .dram0_m_axi_bresp              (dram0_axi_bresp),
    .dram0_m_axi_buser              (0),
    .dram0_m_axi_bvalid             (dram0_axi_bvalid),
    .dram0_m_axi_bready             (dram0_axi_bready),
    .dram0_m_axi_arid               (dram0_axi_arid),
    .dram0_m_axi_araddr             (dram0_axi_araddr),
    .dram0_m_axi_arlen              (dram0_axi_arlen),
    .dram0_m_axi_arsize             (dram0_axi_arsize),
    .dram0_m_axi_arburst            (dram0_axi_arburst),
    .dram0_m_axi_arlock             (dram0_axi_arlock),
    .dram0_m_axi_arcache            (dram0_axi_arcache),
    .dram0_m_axi_arprot             (dram0_axi_arprot),
    .dram0_m_axi_arqos              (dram0_axi_arqos),
    .dram0_m_axi_arregion           (dram0_axi_arregion),
    .dram0_m_axi_aruser             (),
    .dram0_m_axi_arvalid            (dram0_axi_arvalid),
    .dram0_m_axi_arready            (dram0_axi_arready),
    .dram0_m_axi_rid                (dram0_axi_rid),
    .dram0_m_axi_rdata              (dram0_axi_rdata),
    .dram0_m_axi_rresp              (dram0_axi_rresp),
    .dram0_m_axi_rlast              (dram0_axi_rlast),
    .dram0_m_axi_ruser              (0),
    .dram0_m_axi_rvalid             (dram0_axi_rvalid),
    .dram0_m_axi_rready             (dram0_axi_rready),
    .dram1_axi_rst                  (dram_rst),
    .dram1_m_axi_awid               (dram1_axi_awid),
    .dram1_m_axi_awaddr             (dram1_axi_awaddr),
    .dram1_m_axi_awlen              (dram1_axi_awlen),
    .dram1_m_axi_awsize             (dram1_axi_awsize),
    .dram1_m_axi_awburst            (dram1_axi_awburst),
    .dram1_m_axi_awlock             (dram1_axi_awlock),
    .dram1_m_axi_awcache            (dram1_axi_awcache),
    .dram1_m_axi_awprot             (dram1_axi_awprot),
    .dram1_m_axi_awqos              (dram1_axi_awqos),
    .dram1_m_axi_awregion           (dram1_axi_awregion),
    .dram1_m_axi_awuser             (),
    .dram1_m_axi_awvalid            (dram1_axi_awvalid),
    .dram1_m_axi_awready            (dram1_axi_awready),
    .dram1_m_axi_wdata              (dram1_axi_wdata),
    .dram1_m_axi_wstrb              (dram1_axi_wstrb),
    .dram1_m_axi_wlast              (dram1_axi_wlast),
    .dram1_m_axi_wuser              (),
    .dram1_m_axi_wvalid             (dram1_axi_wvalid),
    .dram1_m_axi_wready             (dram1_axi_wready),
    .dram1_m_axi_bid                (dram1_axi_bid),
    .dram1_m_axi_bresp              (dram1_axi_bresp),
    .dram1_m_axi_buser              (0),
    .dram1_m_axi_bvalid             (dram1_axi_bvalid),
    .dram1_m_axi_bready             (dram1_axi_bready),
    .dram1_m_axi_arid               (dram1_axi_arid),
    .dram1_m_axi_araddr             (dram1_axi_araddr),
    .dram1_m_axi_arlen              (dram1_axi_arlen),
    .dram1_m_axi_arsize             (dram1_axi_arsize),
    .dram1_m_axi_arburst            (dram1_axi_arburst),
    .dram1_m_axi_arlock             (dram1_axi_arlock),
    .dram1_m_axi_arcache            (dram1_axi_arcache),
    .dram1_m_axi_arprot             (dram1_axi_arprot),
    .dram1_m_axi_arqos              (dram1_axi_arqos),
    .dram1_m_axi_arregion           (dram1_axi_arregion),
    .dram1_m_axi_aruser             (),
    .dram1_m_axi_arvalid            (dram1_axi_arvalid),
    .dram1_m_axi_arready            (dram1_axi_arready),
    .dram1_m_axi_rid                (dram1_axi_rid),
    .dram1_m_axi_rdata              (dram1_axi_rdata),
    .dram1_m_axi_rresp              (dram1_axi_rresp),
    .dram1_m_axi_rlast              (dram1_axi_rlast),
    .dram1_m_axi_ruser              (0),
    .dram1_m_axi_rvalid             (dram1_axi_rvalid),
    .dram1_m_axi_rready             (dram1_axi_rready),
    .s_eth0_tdata                   (e2v_tdata  [0*ENET_W +: ENET0_W]),
    .s_eth0_tlast                   (e2v_tlast  [0*     1 +:       1]),
    .s_eth0_tvalid                  (e2v_tvalid [0*     1 +:       1]),
    .s_eth0_tready                  (e2v_tready [0*     1 +:       1]),
    .m_eth0_tdata                   (v2e_tdata  [0*ENET_W +: ENET0_W]),
    .m_eth0_tlast                   (v2e_tlast  [0*     1 +:       1]),
    .m_eth0_tvalid                  (v2e_tvalid [0*     1 +:       1]),
    .m_eth0_tready                  (v2e_tready [0*     1 +:       1]),
    .s_eth1_tdata                   (e2v_tdata  [1*ENET_W +: ENET1_W]),
    .s_eth1_tlast                   (e2v_tlast  [1*     1 +:       1]),
    .s_eth1_tvalid                  (e2v_tvalid [1*     1 +:       1]),
    .s_eth1_tready                  (e2v_tready [1*     1 +:       1]),
    .m_eth1_tdata                   (v2e_tdata  [1*ENET_W +: ENET1_W]),
    .m_eth1_tlast                   (v2e_tlast  [1*     1 +:       1]),
    .m_eth1_tvalid                  (v2e_tvalid [1*     1 +:       1]),
    .m_eth1_tready                  (v2e_tready [1*     1 +:       1]),
    .s_eth2_tdata                   (e2v_tdata  [2*ENET_W +: ENET2_W]),
    .s_eth2_tlast                   (e2v_tlast  [2*     1 +:       1]),
    .s_eth2_tvalid                  (e2v_tvalid [2*     1 +:       1]),
    .s_eth2_tready                  (e2v_tready [2*     1 +:       1]),
    .m_eth2_tdata                   (v2e_tdata  [2*ENET_W +: ENET2_W]),
    .m_eth2_tlast                   (v2e_tlast  [2*     1 +:       1]),
    .m_eth2_tvalid                  (v2e_tvalid [2*     1 +:       1]),
    .m_eth2_tready                  (v2e_tready [2*     1 +:       1]),
    .s_eth3_tdata                   (e2v_tdata  [3*ENET_W +: ENET3_W]),
    .s_eth3_tlast                   (e2v_tlast  [3*     1 +:       1]),
    .s_eth3_tvalid                  (e2v_tvalid [3*     1 +:       1]),
    .s_eth3_tready                  (e2v_tready [3*     1 +:       1]),
    .m_eth3_tdata                   (v2e_tdata  [3*ENET_W +: ENET3_W]),
    .m_eth3_tlast                   (v2e_tlast  [3*     1 +:       1]),
    .m_eth3_tvalid                  (v2e_tvalid [3*     1 +:       1]),
    .m_eth3_tready                  (v2e_tready [3*     1 +:       1]),
    .s_eth4_tdata                   (e2v_tdata  [4*ENET_W +: ENET4_W]),
    .s_eth4_tlast                   (e2v_tlast  [4*     1 +:       1]),
    .s_eth4_tvalid                  (e2v_tvalid [4*     1 +:       1]),
    .s_eth4_tready                  (e2v_tready [4*     1 +:       1]),
    .m_eth4_tdata                   (v2e_tdata  [4*ENET_W +: ENET4_W]),
    .m_eth4_tlast                   (v2e_tlast  [4*     1 +:       1]),
    .m_eth4_tvalid                  (v2e_tvalid [4*     1 +:       1]),
    .m_eth4_tready                  (v2e_tready [4*     1 +:       1]),
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
