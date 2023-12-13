//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cpld_interface.v
//
// Description:
//
//   This module comprises the logic on the FPGA to connect with the
//   motherboard CPLD.
//
//   As a first step each request source is transferred to PRC domain.
//   Timestamps are removed in the timer modules. All requests are bundled in
//   one combiner and then split / decoded across all targets. By inserting of
//   ctrlport termination modules a ctrlport answer will be available for all
//   addresses to avoid blocking the main ctrlport combiner.
//

`default_nettype none


module cpld_interface (
  // Clocks
  input  wire        s_axi_aclk,
  input  wire        s_axi_aresetn,
  input  wire        pll_ref_clk,
  input  wire        radio_clk,

  // Reset (domain: pll_ref_clk)
  input  wire        ctrlport_rst,

  // AXI4-Lite: Write address port (domain: s_axi_aclk)
  input  wire [16:0] s_axi_awaddr,
  input  wire        s_axi_awvalid,
  output wire        s_axi_awready,
  // AXI4-Lite: Write data port (domain: s_axi_aclk)
  input  wire [31:0] s_axi_wdata,
  input  wire [ 3:0] s_axi_wstrb,
  input  wire        s_axi_wvalid,
  output wire        s_axi_wready,
  // AXI4-Lite: Write response port (domain: s_axi_aclk)
  output wire[ 1:0]  s_axi_bresp,
  output wire        s_axi_bvalid,
  input  wire        s_axi_bready,
  // AXI4-Lite: Read address port (domain: s_axi_aclk)
  input  wire [16:0] s_axi_araddr,
  input  wire        s_axi_arvalid,
  output wire        s_axi_arready,
  // AXI4-Lite: Read data port (domain: s_axi_aclk)
  output wire [31:0] s_axi_rdata,
  output wire [ 1:0] s_axi_rresp,
  output wire        s_axi_rvalid,
  input  wire        s_axi_rready,

  // Control port from / to application (domain: radio_clk)
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire [ 3:0] s_ctrlport_req_byte_en,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  // SPI Bus to connect to MB CPLD
  output wire [ 1:0] ss,
  output wire        sclk,
  output wire        mosi,
  input  wire        miso,

  // QSFP port LED (domain: any)
  input  wire [ 3:0] qsfp0_led_active,
  input  wire [ 3:0] qsfp0_led_link,
  input  wire [ 3:0] qsfp1_led_active,
  input  wire [ 3:0] qsfp1_led_link,

  // iPass present signals
  input  wire [ 1:0] ipass_present_n,

  // Version (Constant)
  output wire [95:0] version_info
);

  `include "../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/pl_cpld_regmap_utils.vh"
  // Variant-dependent register map.
  `ifdef X440
    `include "cpld/regmap/x440/mb_cpld_pl_regmap_utils.vh"
  `else // Use X410 as the default variant for regmap.
    `include "cpld/regmap/x410/mb_cpld_pl_regmap_utils.vh"
  `endif
  `include "cpld/regmap/pl_cpld_base_regmap_utils.vh"

  //---------------------------------------------------------------------------
  // Internal clocks and resets
  //---------------------------------------------------------------------------

  wire s_axi_areset;
  wire ctrlport_clk;
  assign s_axi_areset = ~s_axi_aresetn;
  assign ctrlport_clk = pll_ref_clk;

  //---------------------------------------------------------------------------
  // MPM Endpoint connection
  //---------------------------------------------------------------------------
  // Translate AXI lite to control port.
  // Timeout based on the 40 MHz AXI clock is about 0.839 seconds.

  wire [19:0] mpm_endpoint_ctrlport_axi_clk_req_addr;
  wire [ 3:0] mpm_endpoint_ctrlport_axi_clk_req_byte_en;
  wire [31:0] mpm_endpoint_ctrlport_axi_clk_req_data;
  wire [ 9:0] mpm_endpoint_ctrlport_axi_clk_req_portid;
  wire        mpm_endpoint_ctrlport_axi_clk_req_rd;
  wire [15:0] mpm_endpoint_ctrlport_axi_clk_req_rem_epid;
  wire [ 9:0] mpm_endpoint_ctrlport_axi_clk_req_rem_portid;
  wire        mpm_endpoint_ctrlport_axi_clk_req_wr;
  wire        mpm_endpoint_ctrlport_axi_clk_resp_ack;
  wire [31:0] mpm_endpoint_ctrlport_axi_clk_resp_data;
  wire [ 1:0] mpm_endpoint_ctrlport_axi_clk_resp_status;

  axil_ctrlport_master #(
    .TIMEOUT         (25),
    .AXI_AWIDTH      (17),
    .CTRLPORT_AWIDTH (17)
  ) mpm_endpoint (
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
    .m_ctrlport_req_wr         (mpm_endpoint_ctrlport_axi_clk_req_wr),
    .m_ctrlport_req_rd         (mpm_endpoint_ctrlport_axi_clk_req_rd),
    .m_ctrlport_req_addr       (mpm_endpoint_ctrlport_axi_clk_req_addr),
    .m_ctrlport_req_portid     (mpm_endpoint_ctrlport_axi_clk_req_portid),
    .m_ctrlport_req_rem_epid   (mpm_endpoint_ctrlport_axi_clk_req_rem_epid),
    .m_ctrlport_req_rem_portid (mpm_endpoint_ctrlport_axi_clk_req_rem_portid),
    .m_ctrlport_req_data       (mpm_endpoint_ctrlport_axi_clk_req_data),
    .m_ctrlport_req_byte_en    (mpm_endpoint_ctrlport_axi_clk_req_byte_en),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (mpm_endpoint_ctrlport_axi_clk_resp_ack),
    .m_ctrlport_resp_status    (mpm_endpoint_ctrlport_axi_clk_resp_status),
    .m_ctrlport_resp_data      (mpm_endpoint_ctrlport_axi_clk_resp_data)
  );

  // Transfer AXI clock based MPM endpoint control port request to pll_ref_clk
  // domain.
  wire [19:0] mpm_endpoint_ctrlport_req_addr;
  wire [ 3:0] mpm_endpoint_ctrlport_req_byte_en;
  wire [31:0] mpm_endpoint_ctrlport_req_data;
  wire        mpm_endpoint_ctrlport_req_rd;
  wire        mpm_endpoint_ctrlport_req_wr;
  wire        mpm_endpoint_ctrlport_resp_ack;
  wire [31:0] mpm_endpoint_ctrlport_resp_data;
  wire [ 1:0] mpm_endpoint_ctrlport_resp_status;

  ctrlport_clk_cross ctrlport_clk_cross_mpm (
    .rst                       (s_axi_areset),
    .s_ctrlport_clk            (s_axi_aclk),
    .s_ctrlport_req_wr         (mpm_endpoint_ctrlport_axi_clk_req_wr),
    .s_ctrlport_req_rd         (mpm_endpoint_ctrlport_axi_clk_req_rd),
    .s_ctrlport_req_addr       (mpm_endpoint_ctrlport_axi_clk_req_addr),
    .s_ctrlport_req_portid     (mpm_endpoint_ctrlport_axi_clk_req_portid),
    .s_ctrlport_req_rem_epid   (mpm_endpoint_ctrlport_axi_clk_req_rem_epid),
    .s_ctrlport_req_rem_portid (mpm_endpoint_ctrlport_axi_clk_req_rem_portid),
    .s_ctrlport_req_data       (mpm_endpoint_ctrlport_axi_clk_req_data),
    .s_ctrlport_req_byte_en    (mpm_endpoint_ctrlport_axi_clk_req_byte_en),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       (64'b0),
    .s_ctrlport_resp_ack       (mpm_endpoint_ctrlport_axi_clk_resp_ack),
    .s_ctrlport_resp_status    (mpm_endpoint_ctrlport_axi_clk_resp_status),
    .s_ctrlport_resp_data      (mpm_endpoint_ctrlport_axi_clk_resp_data),
    .m_ctrlport_clk            (ctrlport_clk),
    .m_ctrlport_req_wr         (mpm_endpoint_ctrlport_req_wr),
    .m_ctrlport_req_rd         (mpm_endpoint_ctrlport_req_rd),
    .m_ctrlport_req_addr       (mpm_endpoint_ctrlport_req_addr),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (mpm_endpoint_ctrlport_req_data),
    .m_ctrlport_req_byte_en    (mpm_endpoint_ctrlport_req_byte_en),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (mpm_endpoint_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (mpm_endpoint_ctrlport_resp_status),
    .m_ctrlport_resp_data      (mpm_endpoint_ctrlport_resp_data)
  );

  //---------------------------------------------------------------------------
  // User Application Request
  //---------------------------------------------------------------------------
  // Transfer request to pll_ref_clk domain.

  wire [19:0] app_ctrlport_req_addr;
  wire [ 3:0] app_ctrlport_req_byte_en;
  wire [31:0] app_ctrlport_req_data;
  wire        app_ctrlport_req_rd;
  wire        app_ctrlport_req_wr;
  wire        app_ctrlport_resp_ack;
  wire [31:0] app_ctrlport_resp_data;
  wire [ 1:0] app_ctrlport_resp_status;

  ctrlport_clk_cross ctrlport_clk_cross_app (
    .rst                       (ctrlport_rst),
    .s_ctrlport_clk            (radio_clk),
    .s_ctrlport_req_wr         (s_ctrlport_req_wr),
    .s_ctrlport_req_rd         (s_ctrlport_req_rd),
    .s_ctrlport_req_addr       (s_ctrlport_req_addr),
    .s_ctrlport_req_portid     (),
    .s_ctrlport_req_rem_epid   (),
    .s_ctrlport_req_rem_portid (),
    .s_ctrlport_req_data       (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en    (s_ctrlport_req_byte_en),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       (64'b0),
    .s_ctrlport_resp_ack       (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (s_ctrlport_resp_status),
    .s_ctrlport_resp_data      (s_ctrlport_resp_data),
    .m_ctrlport_clk            (ctrlport_clk),
    .m_ctrlport_req_wr         (app_ctrlport_req_wr),
    .m_ctrlport_req_rd         (app_ctrlport_req_rd),
    .m_ctrlport_req_addr       (app_ctrlport_req_addr),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (app_ctrlport_req_data),
    .m_ctrlport_req_byte_en    (app_ctrlport_req_byte_en),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (app_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (app_ctrlport_resp_status),
    .m_ctrlport_resp_data      (app_ctrlport_resp_data)
  );

  //---------------------------------------------------------------------------
  // QSFP LED Controller
  //---------------------------------------------------------------------------

  wire [19:0] led_ctrlport_req_addr;
  wire [ 3:0] led_ctrlport_req_byte_en;
  wire [31:0] led_ctrlport_req_data;
  wire        led_ctrlport_req_rd;
  wire        led_ctrlport_req_wr;
  wire        led_ctrlport_resp_ack;
  wire [31:0] led_ctrlport_resp_data;
  wire [ 1:0] led_ctrlport_resp_status;

  qsfp_led_controller
    # (.LED_REGISTER_ADDRESS(MB_CPLD + PL_REGISTERS + LED_REGISTER))
    qsfp_led_controller_i (
      .ctrlport_clk            (ctrlport_clk),
      .ctrlport_rst            (ctrlport_rst),
      .m_ctrlport_req_wr       (led_ctrlport_req_wr),
      .m_ctrlport_req_rd       (led_ctrlport_req_rd),
      .m_ctrlport_req_addr     (led_ctrlport_req_addr),
      .m_ctrlport_req_data     (led_ctrlport_req_data),
      .m_ctrlport_req_byte_en  (led_ctrlport_req_byte_en),
      .m_ctrlport_resp_ack     (led_ctrlport_resp_ack),
      .m_ctrlport_resp_status  (led_ctrlport_resp_status),
      .m_ctrlport_resp_data    (led_ctrlport_resp_data),
      .qsfp0_led_active        (qsfp0_led_active),
      .qsfp0_led_link          (qsfp0_led_link),
      .qsfp1_led_active        (qsfp1_led_active),
      .qsfp1_led_link          (qsfp1_led_link));

  //---------------------------------------------------------------------------
  // iPass present controller
  //---------------------------------------------------------------------------

  wire [19:0] ipass_ctrlport_req_addr;
  wire [ 3:0] ipass_ctrlport_req_byte_en;
  wire [31:0] ipass_ctrlport_req_data;
  wire        ipass_ctrlport_req_rd;
  wire        ipass_ctrlport_req_wr;
  wire        ipass_ctrlport_resp_ack;
  wire [31:0] ipass_ctrlport_resp_data;
  wire [ 1:0] ipass_ctrlport_resp_status;

  wire ipass_enable;

  ipass_present_controller ipass_present_controller_i (
    .ctrlport_clk           (ctrlport_clk),
    .ctrlport_rst           (ctrlport_rst),
    .m_ctrlport_req_wr      (ipass_ctrlport_req_wr),
    .m_ctrlport_req_rd      (ipass_ctrlport_req_rd),
    .m_ctrlport_req_addr    (ipass_ctrlport_req_addr),
    .m_ctrlport_req_data    (ipass_ctrlport_req_data),
    .m_ctrlport_req_byte_en (ipass_ctrlport_req_byte_en),
    .m_ctrlport_resp_ack    (ipass_ctrlport_resp_ack),
    .m_ctrlport_resp_status (ipass_ctrlport_resp_status),
    .m_ctrlport_resp_data   (ipass_ctrlport_resp_data),
    .enable                 (ipass_enable),
    .ipass_present_n        (ipass_present_n)
  );

  //---------------------------------------------------------------------------
  // Combine all incoming combiner requests and provide to targets
  //---------------------------------------------------------------------------

  wire [19:0] m_ctrlport_req_addr;
  wire [31:0] m_ctrlport_req_data;
  wire        m_ctrlport_req_rd;
  wire        m_ctrlport_req_wr;
  wire        m_ctrlport_resp_ack;
  wire [31:0] m_ctrlport_resp_data;
  wire [ 1:0] m_ctrlport_resp_status;

  ctrlport_combiner #(
    .NUM_MASTERS (4),
    .PRIORITY    (1)
  ) ctrlport_combiner_i (
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    .s_ctrlport_req_wr         ({ipass_ctrlport_req_wr, led_ctrlport_req_wr, mpm_endpoint_ctrlport_req_wr, app_ctrlport_req_wr}),
    .s_ctrlport_req_rd         ({ipass_ctrlport_req_rd, led_ctrlport_req_rd, mpm_endpoint_ctrlport_req_rd, app_ctrlport_req_rd}),
    .s_ctrlport_req_addr       ({ipass_ctrlport_req_addr, led_ctrlport_req_addr, mpm_endpoint_ctrlport_req_addr, app_ctrlport_req_addr}),
    .s_ctrlport_req_portid     (),
    .s_ctrlport_req_rem_epid   (),
    .s_ctrlport_req_rem_portid (),
    .s_ctrlport_req_data       ({ipass_ctrlport_req_data, led_ctrlport_req_data, mpm_endpoint_ctrlport_req_data, app_ctrlport_req_data}),
    .s_ctrlport_req_byte_en    ({ipass_ctrlport_req_byte_en, led_ctrlport_req_byte_en, mpm_endpoint_ctrlport_req_byte_en, app_ctrlport_req_byte_en}),
    .s_ctrlport_req_has_time   ({4{1'b0}}),
    .s_ctrlport_req_time       ({4{64'b0}}),
    .s_ctrlport_resp_ack       ({ipass_ctrlport_resp_ack, led_ctrlport_resp_ack, mpm_endpoint_ctrlport_resp_ack, app_ctrlport_resp_ack}),
    .s_ctrlport_resp_status    ({ipass_ctrlport_resp_status, led_ctrlport_resp_status, mpm_endpoint_ctrlport_resp_status, app_ctrlport_resp_status}),
    .s_ctrlport_resp_data      ({ipass_ctrlport_resp_data, led_ctrlport_resp_data, mpm_endpoint_ctrlport_resp_data, app_ctrlport_resp_data}),
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_portid     (),
    .m_ctrlport_req_rem_epid   (),
    .m_ctrlport_req_rem_portid (),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (m_ctrlport_resp_status),
    .m_ctrlport_resp_data      (m_ctrlport_resp_data)
  );


  // Split for CPLD facing requests and others.
  wire [19:0] base_reg_ctrlport_req_addr;
  wire [31:0] base_reg_ctrlport_req_data;
  wire        base_reg_ctrlport_req_rd;
  wire        base_reg_ctrlport_req_wr;
  wire        base_reg_ctrlport_resp_ack;
  wire [31:0] base_reg_ctrlport_resp_data;
  wire [ 1:0] base_reg_ctrlport_resp_status;

  wire [19:0] spi_master_ctrlport_req_addr;
  wire [31:0] spi_master_ctrlport_req_data;
  wire        spi_master_ctrlport_req_rd;
  wire        spi_master_ctrlport_req_wr;
  wire        spi_master_ctrlport_resp_ack;
  wire [31:0] spi_master_ctrlport_resp_data;
  wire [ 1:0] spi_master_ctrlport_resp_status;

  wire [19:0] unused_fpga_intermediate_ctrlport_req_addr;
  wire [31:0] unused_fpga_intermediate_ctrlport_req_data;
  wire        unused_fpga_intermediate_ctrlport_req_rd;
  wire        unused_fpga_intermediate_ctrlport_req_wr;
  wire        unused_fpga_intermediate_ctrlport_resp_ack;
  wire [31:0] unused_fpga_intermediate_ctrlport_resp_data;
  wire [ 1:0] unused_fpga_intermediate_ctrlport_resp_status;

  wire [19:0] unused_fpga_msbs_ctrlport_req_addr;
  wire [31:0] unused_fpga_msbs_ctrlport_req_data;
  wire        unused_fpga_msbs_ctrlport_req_rd;
  wire        unused_fpga_msbs_ctrlport_req_wr;
  wire        unused_fpga_msbs_ctrlport_resp_ack;
  wire [31:0] unused_fpga_msbs_ctrlport_resp_data;
  wire [ 1:0] unused_fpga_msbs_ctrlport_resp_status;

  ctrlport_splitter #(
    .NUM_SLAVES (4)
  ) ctrlport_splitter_i (
    .ctrlport_clk            (ctrlport_clk),
    .ctrlport_rst            (ctrlport_rst),
    .s_ctrlport_req_wr       (m_ctrlport_req_wr),
    .s_ctrlport_req_rd       (m_ctrlport_req_rd),
    .s_ctrlport_req_addr     (m_ctrlport_req_addr),
    .s_ctrlport_req_data     (m_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (),
    .s_ctrlport_req_has_time (),
    .s_ctrlport_req_time     (),
    .s_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (m_ctrlport_resp_status),
    .s_ctrlport_resp_data    (m_ctrlport_resp_data),
    .m_ctrlport_req_wr       ({unused_fpga_msbs_ctrlport_req_wr,   unused_fpga_intermediate_ctrlport_req_wr,   base_reg_ctrlport_req_wr,   spi_master_ctrlport_req_wr}),
    .m_ctrlport_req_rd       ({unused_fpga_msbs_ctrlport_req_rd,   unused_fpga_intermediate_ctrlport_req_rd,   base_reg_ctrlport_req_rd,   spi_master_ctrlport_req_rd}),
    .m_ctrlport_req_addr     ({unused_fpga_msbs_ctrlport_req_addr, unused_fpga_intermediate_ctrlport_req_addr, base_reg_ctrlport_req_addr, spi_master_ctrlport_req_addr}),
    .m_ctrlport_req_data     ({unused_fpga_msbs_ctrlport_req_data, unused_fpga_intermediate_ctrlport_req_data, base_reg_ctrlport_req_data, spi_master_ctrlport_req_data}),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({unused_fpga_msbs_ctrlport_resp_ack,    unused_fpga_intermediate_ctrlport_resp_ack,    base_reg_ctrlport_resp_ack,    spi_master_ctrlport_resp_ack}),
    .m_ctrlport_resp_status  ({unused_fpga_msbs_ctrlport_resp_status, unused_fpga_intermediate_ctrlport_resp_status, base_reg_ctrlport_resp_status, spi_master_ctrlport_resp_status}),
    .m_ctrlport_resp_data    ({unused_fpga_msbs_ctrlport_resp_data,   unused_fpga_intermediate_ctrlport_resp_data,   base_reg_ctrlport_resp_data,   spi_master_ctrlport_resp_data})
  );

  //---------------------------------------------------------------------------
  // Targets for Requests
  //---------------------------------------------------------------------------

  wire [15:0] db_clock_divider;
  wire [15:0] mb_clock_divider;

  cpld_interface_regs #(
    .BASE_ADDRESS  (BASE),
    .NUM_ADDRESSES (BASE_SIZE)
  ) cpld_interface_regs_i (
    .ctrlport_clk           (ctrlport_clk),
    .ctrlport_rst           (ctrlport_rst),
    .s_ctrlport_req_wr      (base_reg_ctrlport_req_wr),
    .s_ctrlport_req_rd      (base_reg_ctrlport_req_rd),
    .s_ctrlport_req_addr    (base_reg_ctrlport_req_addr),
    .s_ctrlport_req_data    (base_reg_ctrlport_req_data),
    .s_ctrlport_resp_ack    (base_reg_ctrlport_resp_ack),
    .s_ctrlport_resp_status (base_reg_ctrlport_resp_status),
    .s_ctrlport_resp_data   (base_reg_ctrlport_resp_data),
    .mb_clock_divider       (mb_clock_divider),
    .db_clock_divider       (db_clock_divider),
    .ipass_enable           (ipass_enable),
    .version_info           (version_info)
  );

  ctrlport_spi_master #(
    .CPLD_ADDRESS_WIDTH     (15),
    .MB_CPLD_BASE_ADDRESS   (MB_CPLD),
    .DB_0_CPLD_BASE_ADDRESS (DB0_CPLD),
    .DB_1_CPLD_BASE_ADDRESS (DB1_CPLD)
  ) ctrlport_spi_master_i (
    .ctrlport_clk           (ctrlport_clk),
    .ctrlport_rst           (ctrlport_rst),
    .s_ctrlport_req_wr      (spi_master_ctrlport_req_wr),
    .s_ctrlport_req_rd      (spi_master_ctrlport_req_rd),
    .s_ctrlport_req_addr    (spi_master_ctrlport_req_addr),
    .s_ctrlport_req_data    (spi_master_ctrlport_req_data),
    .s_ctrlport_resp_ack    (spi_master_ctrlport_resp_ack),
    .s_ctrlport_resp_status (spi_master_ctrlport_resp_status),
    .s_ctrlport_resp_data   (spi_master_ctrlport_resp_data),
    .ss                     (ss),
    .sclk                   (sclk),
    .mosi                   (mosi),
    .miso                   (miso),
    .mb_clock_divider       (mb_clock_divider),
    .db_clock_divider       (db_clock_divider)
  );

  //---------------------------------------------------------------------------
  // Invalid target address spaces
  //---------------------------------------------------------------------------

  ctrlport_terminator #(
    .START_ADDRESS (BASE + BASE_SIZE),
    .LAST_ADDRESS  (MB_CPLD-1)
  ) ctrlport_terminator_intermediate (
    .ctrlport_clk           (ctrlport_clk),
    .ctrlport_rst           (ctrlport_rst),
    .s_ctrlport_req_wr      (unused_fpga_intermediate_ctrlport_req_wr),
    .s_ctrlport_req_rd      (unused_fpga_intermediate_ctrlport_req_rd),
    .s_ctrlport_req_addr    (unused_fpga_intermediate_ctrlport_req_addr),
    .s_ctrlport_req_data    (unused_fpga_intermediate_ctrlport_req_data),
    .s_ctrlport_resp_ack    (unused_fpga_intermediate_ctrlport_resp_ack),
    .s_ctrlport_resp_status (unused_fpga_intermediate_ctrlport_resp_status),
    .s_ctrlport_resp_data   (unused_fpga_intermediate_ctrlport_resp_data)
  );

  ctrlport_terminator #(
    .START_ADDRESS (DB1_CPLD + DB1_CPLD_SIZE),
    .LAST_ADDRESS  (2**CTRLPORT_ADDR_W-1)
  ) ctrlport_terminator_msbs (
    .ctrlport_clk           (ctrlport_clk),
    .ctrlport_rst           (ctrlport_rst),
    .s_ctrlport_req_wr      (unused_fpga_msbs_ctrlport_req_wr),
    .s_ctrlport_req_rd      (unused_fpga_msbs_ctrlport_req_rd),
    .s_ctrlport_req_addr    (unused_fpga_msbs_ctrlport_req_addr),
    .s_ctrlport_req_data    (unused_fpga_msbs_ctrlport_req_data),
    .s_ctrlport_resp_ack    (unused_fpga_msbs_ctrlport_resp_ack),
    .s_ctrlport_resp_status (unused_fpga_msbs_ctrlport_resp_status),
    .s_ctrlport_resp_data   (unused_fpga_msbs_ctrlport_resp_data)
  );

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="PL_CPLD_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This register map is available from the PS via AXI and MPM endpoint.
//    Its size is 128K (17 bits). Only the 17 LSBs are used as address in this documentation.
//  </info>
//  <group name="PL_CPLD_WINDOWS">
//    <window name="BASE" offset="0x0" size="0x40" targetregmap="CPLD_INTERFACE_REGMAP"/>
//    <window name="MB_CPLD" offset="0x8000" size="0x8000" targetregmap="MB_CPLD_PL_REGMAP">
//      <info>
//        All registers of the MB CPLD (PL part).
//      </info>
//    </window>
//    <window name="DB0_CPLD" offset="0x10000" size="0x8000">
//      <info>
//        All registers of the first DB CPLD. Register map will be added later on.
//      </info>
//    </window>
//    <window name="DB1_CPLD" offset="0x18000" size="0x8000">
//      <info>
//        All registers of the second DB CPLD. Register map will be added later on.
//      </info>
//    </window>
//  </group>
//</regmap>
//XmlParse xml_off
