// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rf_core_400m_x420_tb.sv
//
// Description:  Basic data-flow testbench for rf_core_400m_x420.

module rf_core_400m_x420_tb;

  timeunit 1ns / 1ps;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import ctrlport_pkg::*;
  import ctrlport_bfm_pkg::*;
  import XmlSvPkgIQ_IMPAIRMENT_REGMAP::*;
  import XmlSvPkgRF_CORE_REGMAP::*;
  import XmlSvPkgDC_OFFSET_REGMAP::*;

  localparam int NUM_ADC_CHANNELS = 1;
  localparam int NUM_DAC_CHANNELS = 1;
  localparam int RADIO_SPC = 4;

  localparam int TX_IN_TRANSFERS = 120;
  localparam int RX_IN_TRANSFERS = 240;
  localparam int FLUSH_ZERO_TRANSFERS = 200;
  localparam int NUM_IMPAIR_COEFFS = 15;
  localparam int COEFFS_FRACTIONAL_BITS = 23;
  localparam int COEFF_ONE = (1 << COEFFS_FRACTIONAL_BITS);

  // Clock periods: pll_ref_clk is the base clock.
  //   data_clk    = 2x pll_ref_clk
  //   data_clk_2x = 4x pll_ref_clk
  //   rfdc_clk    = 6x pll_ref_clk
  localparam realtime PLL_REF_CLK_PERIOD = 12.0;
  localparam realtime DATA_CLK_PERIOD = PLL_REF_CLK_PERIOD / 2;
  localparam realtime DATA_CLK_2X_PERIOD = PLL_REF_CLK_PERIOD / 4;
  localparam realtime RFDC_CLK_PERIOD = PLL_REF_CLK_PERIOD / 6;

  logic pll_ref_clk;
  logic data_clk;
  logic data_clk_2x;
  logic rfdc_clk;

  logic tx_resampler_reset_pulse_dclk;
  logic rx_resampler_reset_pulse_dclk;

  // RFDC-facing ADC interfaces (source for RX path)
  AxiStreamIf  #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    adc_in_m_axis_if (.clk(rfdc_clk), .rst());
  // User-facing ADC output interfaces (sink for RX path)
  AxiStreamIf  #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    adc_out_s_axis_if (.clk(data_clk), .rst());

  // User-facing DAC input interfaces (source for TX path)
  AxiStreamIf  #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    dac_in_m_axis_if (.clk(data_clk), .rst());
  // RFDC-facing DAC output interfaces (sink for TX path)
  AxiStreamIf  #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    dac_out_s_axis_if (.clk(rfdc_clk), .rst());

  AxiStreamBfm #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) tx_bfm;
  AxiStreamBfm #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) rx_bfm;

  typedef AxiStreamBfm #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    ::AxisPacket_t AxisPacket_t;
  AxisPacket_t packet_in;
  AxisPacket_t packet_out;

  logic          ctrlport_rst;
  ctrlport_if    s_ctrlport (.clk(data_clk), .rst(ctrlport_rst));
  ctrlport_bfm   ctrl_bfm;
  logic [CTRLPORT_STS_W-1:0] s_ctrlport_resp_status_raw;
  assign s_ctrlport.resp.status = ctrlport_status_t'(s_ctrlport_resp_status_raw);

  logic [16*RADIO_SPC-1:0]     adc_data_in_i_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready;
  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tvalid;
  logic [16*RADIO_SPC-1:0]     adc_data_in_q_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready;
  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tvalid;

  logic [32*RADIO_SPC-1:0]     dac_data_out_tdata [0:NUM_DAC_CHANNELS-1];
  logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tready;
  logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tvalid;

  logic [32*RADIO_SPC-1:0]     adc_data_out_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid;

  logic [32*RADIO_SPC-1:0]     dac_data_in_tdata [0:NUM_DAC_CHANNELS-1];
  logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tready;
  logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tvalid;

  logic [NUM_ADC_CHANNELS-1:0] invert_adc_iq_rclk;
  logic [NUM_DAC_CHANNELS-1:0] invert_dac_iq_rclk;
  logic [9:0]                  dsp_info_sclk;
  logic [15:0]                 axi_status_sclk;
  logic [15:0]                 rfdc_info_sclk;
  logic                        adc_enable_data_rclk;
  logic                        adc_rfdc_axi_resetn_rclk;
  logic [95:0]                 version_info;

  sim_clock_gen #(.PERIOD(PLL_REF_CLK_PERIOD), .AUTOSTART(0))  pll_ref_clk_gen
    (.clk(pll_ref_clk), .rst());
  sim_clock_gen #(.PERIOD(DATA_CLK_PERIOD), .AUTOSTART(0))     data_clk_gen
    (.clk(data_clk),    .rst());
  sim_clock_gen #(.PERIOD(DATA_CLK_2X_PERIOD), .AUTOSTART(0))  data_clk_2x_gen
    (.clk(data_clk_2x), .rst());
  sim_clock_gen #(.PERIOD(RFDC_CLK_PERIOD), .AUTOSTART(0))     rfdc_clk_gen
    (.clk(rfdc_clk),    .rst());

  // Connect user DAC input stream to DUT
  assign dac_data_in_tdata[0] = dac_in_m_axis_if.tdata;
  assign dac_data_in_tvalid[0] = dac_in_m_axis_if.tvalid;
  assign dac_in_m_axis_if.tready = dac_data_in_tready[0];

  // Connect DUT DAC output stream to RFDC-facing sink stream
  assign dac_out_s_axis_if.tdata = dac_data_out_tdata[0];
  assign dac_out_s_axis_if.tvalid = dac_data_out_tvalid[0];
  assign dac_out_s_axis_if.tlast = '1;
  assign dac_data_out_tready[0] = dac_out_s_axis_if.tready;

  // Split RFDC ADC input stream into I/Q buses
  for (genvar i = 0; i < RADIO_SPC; i = i + 1) begin : gen_adc_iq_split
    assign adc_data_in_i_tdata[0][16*i +: 16] = adc_in_m_axis_if.tdata[32*i +: 16];
    assign adc_data_in_q_tdata[0][16*i +: 16] = adc_in_m_axis_if.tdata[32*i + 16 +: 16];
  end
  assign adc_data_in_i_tvalid[0] = adc_in_m_axis_if.tvalid;
  assign adc_data_in_q_tvalid[0] = adc_in_m_axis_if.tvalid;
  assign adc_in_m_axis_if.tready = adc_data_in_i_tready[0];

  always_comb begin
    assert (adc_data_in_i_tready[0] === adc_data_in_q_tready[0]) else
      $error("adc_data_in_i_tready and adc_data_in_q_tready must match");
  end

  // Connect DUT ADC output stream to user-facing sink stream
  assign adc_out_s_axis_if.tdata = adc_data_out_tdata[0];
  assign adc_out_s_axis_if.tvalid = adc_data_out_tvalid[0];
  assign adc_out_s_axis_if.tlast = '1;

  rf_core_400m_x420 #(
    .RADIO_SPC(RADIO_SPC)
  ) dut (
    .data_clk                    (data_clk),
    .data_clk_2x                 (data_clk_2x),
    .rfdc_clk                    (rfdc_clk),
    .pll_ref_clk                 (pll_ref_clk),
    .rx_resampler_reset_pulse_dclk(rx_resampler_reset_pulse_dclk),
    .tx_resampler_reset_pulse_dclk(tx_resampler_reset_pulse_dclk),
    .s_axi_config_clk            (data_clk),
    .ctrlport_rst                (ctrlport_rst),
    .s_ctrlport_req_wr           (s_ctrlport.req.wr),
    .s_ctrlport_req_rd           (s_ctrlport.req.rd),
    .s_ctrlport_req_addr         (s_ctrlport.req.addr),
    .s_ctrlport_req_data         (s_ctrlport.req.data),
    .s_ctrlport_resp_ack         (s_ctrlport.resp.ack),
    .s_ctrlport_resp_status      (s_ctrlport_resp_status_raw),
    .s_ctrlport_resp_data        (s_ctrlport.resp.data),
    .adc_data_in_i_tdata         (adc_data_in_i_tdata),
    .adc_data_in_i_tready        (adc_data_in_i_tready),
    .adc_data_in_i_tvalid        (adc_data_in_i_tvalid),
    .adc_data_in_q_tdata         (adc_data_in_q_tdata),
    .adc_data_in_q_tready        (adc_data_in_q_tready),
    .adc_data_in_q_tvalid        (adc_data_in_q_tvalid),
    .dac_data_out_tdata          (dac_data_out_tdata),
    .dac_data_out_tready         (dac_data_out_tready),
    .dac_data_out_tvalid         (dac_data_out_tvalid),
    .adc_data_out_tdata          (adc_data_out_tdata),
    .adc_data_out_tvalid         (adc_data_out_tvalid),
    .dac_data_in_tdata           (dac_data_in_tdata),
    .dac_data_in_tready          (dac_data_in_tready),
    .dac_data_in_tvalid          (dac_data_in_tvalid),
    .invert_adc_iq_rclk          (invert_adc_iq_rclk),
    .invert_dac_iq_rclk          (invert_dac_iq_rclk),
    .dsp_info_sclk               (dsp_info_sclk),
    .axi_status_sclk             (axi_status_sclk),
    .rfdc_info_sclk              (rfdc_info_sclk),
    .adc_enable_data_rclk        (adc_enable_data_rclk),
    .adc_rfdc_axi_resetn_rclk    (adc_rfdc_axi_resetn_rclk),
    .version_info                (version_info)
  );

  task automatic initialize_iq_impairment_filter(
    input logic [CTRLPORT_ADDR_W-1:0] base
  );
    // Set group delay to 0, scale I path to 1.0, and zero all FIR taps.
    ctrl_bfm.write(base + kDELAY_REG, '0);
    ctrl_bfm.write(base + kIINLINE_COEFF_REG, COEFF_ONE);
    for (int i = 0; i < NUM_IMPAIR_COEFFS; i++) begin
      ctrl_bfm.write(base + kICROSS_COEFF_REG, '0);
      ctrl_bfm.write(base + kQINLINE_COEFF_REG, i == (NUM_IMPAIR_COEFFS-1) ? COEFF_ONE : '0);
    end
  endtask

  initial begin : tb_main
    int tx_const_num_received;
    int rx_const_num_received;
    logic [RADIO_SPC-1:0][31:0] tx_const_word_ref;
    logic [RADIO_SPC-1:0][31:0] rx_const_word_ref;
    bit tx_clean;
    bit rx_clean;

    test.start_tb("rf_core_400m_x420_tb");

    // start clock generators
    pll_ref_clk_gen.start();
    rfdc_clk_gen.start();
    data_clk_gen.start();
    data_clk_2x_gen.start();

    ctrlport_rst = 1'b0;
    invert_adc_iq_rclk = '0;
    invert_dac_iq_rclk = '0;
    adc_enable_data_rclk = 1'b1;
    adc_rfdc_axi_resetn_rclk = 1'b1;

    tx_resampler_reset_pulse_dclk = 1'b0;
    rx_resampler_reset_pulse_dclk = 1'b0;

    // Initialize the data paths.
    // The stall probability is disabled in both directions on the receiver
    // (slave) as the core is designed for the RFDC data paths, which have to
    // transfer in each clock cycle. Therefore internal modules do no account
    // for the ready flags in some cases.
    tx_bfm = new(dac_in_m_axis_if, dac_out_s_axis_if);
    tx_bfm.set_slave_stall_prob(0);
    tx_bfm.run();

    rx_bfm = new(adc_in_m_axis_if, adc_out_s_axis_if);
    rx_bfm.set_slave_stall_prob(0);
    rx_bfm.run();

    ctrl_bfm = new(s_ctrlport);
    ctrl_bfm.run();

    // -------------------------------------------------------------------------
    // TX Tests
    // -------------------------------------------------------------------------
    @(posedge data_clk);
    tx_resampler_reset_pulse_dclk = 1'b1;
    @(posedge data_clk);
    tx_resampler_reset_pulse_dclk = 1'b0;

    repeat (64) @(posedge data_clk);

    test.start_test("initialize TX IQ impairment FIR coefficients");
    initialize_iq_impairment_filter(kTX_IQ_IMPAIRMENTS);
    test.end_test();

    test.start_test("initial TX zero flush clears X at outputs");

    while (tx_bfm.try_get(packet_out));

    repeat (FLUSH_ZERO_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back('0);
      tx_bfm.put(packet_in);
    end
    tx_bfm.wait_send();

    tx_clean = '0;
    while (tx_bfm.try_get(packet_out)) begin
      if (packet_out.data[0] == '0) begin
        tx_clean = '1;
      end
    end
    `ASSERT_ERROR(tx_clean,
      "TX zero flush did not produce clean (non-X) output transfers")

    test.end_test();

    test.start_test("TX constant IQ output stable and non-zero");
    while (tx_bfm.try_get(packet_out));

    // Constant IQ pattern [Q,I] = {0x4000,0x2000} on all lanes.
    repeat (TX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back({4{16'h4000, 16'h2000}});
      tx_bfm.put(packet_in);
    end
    tx_bfm.wait_send();

    // Drop older outputs and keep the final 64 transfers to check steady-state.
    tx_const_num_received = tx_bfm.num_received();
    `ASSERT_ERROR(tx_const_num_received >= 64,
      "TX constant-IQ test: not enough output transfers to check steady-state")
    repeat (tx_const_num_received - 64) begin
      tx_bfm.get(packet_out);
    end

    tx_bfm.get(packet_out);
    tx_const_word_ref = packet_out.data[0];

    `ASSERT_ERROR(!$isunknown(tx_const_word_ref),
      "TX constant-IQ reference output contains unknown X bits")

    for (int lane = 0; lane < RADIO_SPC; lane++) begin
      logic [15:0] i_lane;
      logic [15:0] q_lane;
      i_lane = tx_const_word_ref[lane][15:0];
      q_lane = tx_const_word_ref[lane][31:16];
      `ASSERT_ERROR(i_lane != 16'h0000,
        $sformatf("TX constant-IQ steady-state I lane %0d is zero", lane))
      `ASSERT_ERROR(q_lane != 16'h0000,
        $sformatf("TX constant-IQ steady-state Q lane %0d is zero", lane))
    end

    for (int i = 1; i < 64; i++) begin
      tx_bfm.get(packet_out);
      `ASSERT_ERROR(!$isunknown(packet_out.data[0]),
        $sformatf("TX constant-IQ output contains unknown X bits at steady-state sample %0d", i))
      `ASSERT_ERROR(packet_out.data[0] === tx_const_word_ref,
        $sformatf({"TX constant-IQ output not stable at steady-state sample %0d.",
                   " Got %h, expected %h"},
                  i, packet_out.data[0], tx_const_word_ref))
    end

    test.end_test();

    test.start_test("TX DC offset correction");
    ctrl_bfm.write(kTX_DC_OFFSET + kOFFSET_VALUE_REG, 32'h02340456);
    ctrl_bfm.write(kTX_DC_OFFSET + kCONTROL_REG, 1'b1);

    repeat (TX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back({4{16'h4000, 16'h2000}});
      tx_bfm.put(packet_in);
    end
    tx_bfm.wait_send();

    tx_const_num_received = tx_bfm.num_received();
    repeat (tx_const_num_received) begin
      tx_bfm.get(packet_out);
    end
    for (int lane = 0; lane < RADIO_SPC; lane++) begin
      logic signed [15:0] i_lane;
      logic signed [15:0] q_lane;
      i_lane = $signed(packet_out.data[0][32*lane +: 16]);
      q_lane = $signed(packet_out.data[0][32*lane + 16 +: 16]);
      `ASSERT_ERROR(i_lane == ($signed(tx_const_word_ref[lane][15:0]) - 16'sh0456),
        $sformatf("TX DC offset correction I lane %0d incorrect. Exp=%h, Got=%h",
          lane,
          $signed(tx_const_word_ref[lane][15:0]) - 16'sh0456,
          i_lane))
      `ASSERT_ERROR(q_lane == ($signed(tx_const_word_ref[lane][31:16]) - 16'sh0234),
        $sformatf("TX DC offset correction Q lane %0d incorrect. Exp=%h, Got=%h",
          lane,
          $signed(tx_const_word_ref[lane][31:16]) - 16'sh0234,
          q_lane))
    end
    test.end_test();

    test.start_test("TX - Verify number of samples received");
    // wait for remaining samples to be received before checking the count
    pll_ref_clk_gen.clk_wait_r(100);
    while (tx_bfm.try_get(packet_out));
    repeat (TX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back('0);
      tx_bfm.put(packet_in);
    end
    tx_bfm.wait_send();
    pll_ref_clk_gen.clk_wait_r(100);

    tx_const_num_received = tx_bfm.num_received();
    `ASSERT_ERROR(tx_const_num_received == TX_IN_TRANSFERS * 3,
      $sformatf("Number of TX output samples received incorrect. Exp=%0d, Got=%0d",
        TX_IN_TRANSFERS * 3, tx_const_num_received))
    test.end_test();

    // -------------------------------------------------------------------------
    // RX Tests
    // -------------------------------------------------------------------------
    @(posedge data_clk);
    rx_resampler_reset_pulse_dclk = 1'b1;
    @(posedge data_clk);
    rx_resampler_reset_pulse_dclk = 1'b0;

    repeat (64) @(posedge data_clk);

    test.start_test("initialize RX IQ impairment FIR coefficients");
    initialize_iq_impairment_filter(kRX_IQ_IMPAIRMENTS);
    test.end_test();

    test.start_test("initial RX zero flush clears X at outputs");

    while (rx_bfm.try_get(packet_out));

    repeat (FLUSH_ZERO_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back('0);
      rx_bfm.put(packet_in);
    end
    rx_bfm.wait_send();

    rx_clean = '0;
    while (rx_bfm.try_get(packet_out)) begin
      if (packet_out.data[0] == '0) begin
        rx_clean = '1;
      end
    end
    `ASSERT_ERROR(rx_clean,
      "RX zero flush did not produce clean (non-X) output transfers")

    test.end_test();

    test.start_test("RX constant IQ output stable and non-zero");
    while (rx_bfm.try_get(packet_out));

    // Constant IQ pattern [Q,I] = {0x3000,0x1800} on all lanes.
    repeat (RX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back({4{16'h3000, 16'h1800}});
      rx_bfm.put(packet_in);
    end
    rx_bfm.wait_send();

    // RX downsamples 3:1; keep the final 16 transfers to check steady-state.
    rx_const_num_received = rx_bfm.num_received();
    `ASSERT_ERROR(rx_const_num_received >= 16,
      "RX constant-IQ test: not enough output transfers to check steady-state")
    repeat (rx_const_num_received - 16) begin
      rx_bfm.get(packet_out);
    end

    rx_bfm.get(packet_out);
    rx_const_word_ref = packet_out.data[0];

    `ASSERT_ERROR(!$isunknown(rx_const_word_ref),
      "RX constant-IQ reference output contains unknown X bits")

    for (int lane = 0; lane < RADIO_SPC; lane++) begin
      logic [15:0] i_lane;
      logic [15:0] q_lane;
      i_lane = rx_const_word_ref[lane][15:0];
      q_lane = rx_const_word_ref[lane][31:16];
      `ASSERT_ERROR(i_lane != 16'h0000,
        $sformatf("RX constant-IQ steady-state I lane %0d is zero", lane))
      `ASSERT_ERROR(q_lane != 16'h0000,
        $sformatf("RX constant-IQ steady-state Q lane %0d is zero", lane))
    end

    for (int i = 1; i < 16; i++) begin
      rx_bfm.get(packet_out);
      `ASSERT_ERROR(!$isunknown(packet_out.data[0]),
        $sformatf("RX constant-IQ output contains unknown X bits at steady-state sample %0d", i))
      `ASSERT_ERROR(packet_out.data[0] === rx_const_word_ref,
        $sformatf({"RX constant-IQ output not stable at steady-state sample %0d.",
                   " Got %h, expected %h"},
                  i, packet_out.data[0], rx_const_word_ref))
    end

    test.end_test();

    test.start_test("RX DC offset correction");
    ctrl_bfm.write(kRX_DC_OFFSET + kOFFSET_VALUE_REG, 32'h05670089);
    ctrl_bfm.write(kRX_DC_OFFSET + kCONTROL_REG, 1'b1);

    repeat (RX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back({4{16'h3000, 16'h1800}});
      rx_bfm.put(packet_in);
    end
    rx_bfm.wait_send();

    rx_const_num_received = rx_bfm.num_received();
    repeat (rx_const_num_received) begin
      rx_bfm.get(packet_out);
    end
    for (int lane = 0; lane < RADIO_SPC; lane++) begin
      logic signed [15:0] i_lane;
      logic signed [15:0] q_lane;
      i_lane = $signed(packet_out.data[0][32*lane +: 16]);
      q_lane = $signed(packet_out.data[0][32*lane + 16 +: 16]);
      `ASSERT_ERROR(i_lane == ($signed(rx_const_word_ref[lane][15:0]) - 16'sh0089),
        $sformatf("RX DC offset correction I lane %0d incorrect. Exp=%h, Got=%h",
          lane,
          $signed(rx_const_word_ref[lane][15:0]) - 16'sh0089,
          i_lane))
      `ASSERT_ERROR(q_lane == ($signed(rx_const_word_ref[lane][31:16]) - 16'sh0567),
        $sformatf("RX DC offset correction Q lane %0d incorrect. Exp=%h, Got=%h",
          lane,
          $signed(rx_const_word_ref[lane][31:16]) - 16'sh0567,
          q_lane))
    end
    test.end_test();

    test.start_test("RX - Verify number of samples received");
    // wait for remaining samples to be received before checking the count
    pll_ref_clk_gen.clk_wait_r(100);
    while(rx_bfm.try_get(packet_out));
    repeat(RX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back('0);
      rx_bfm.put(packet_in);
    end
    rx_bfm.wait_send();
    pll_ref_clk_gen.clk_wait_r(100);

    rx_const_num_received = rx_bfm.num_received();
    `ASSERT_ERROR(rx_const_num_received == RX_IN_TRANSFERS / 3,
      $sformatf("Number of RX output samples received incorrect. Exp=%0d, Got=%0d",
        RX_IN_TRANSFERS / 3, rx_const_num_received))
    test.end_test();

    // kill all clocks
    pll_ref_clk_gen.kill();
    data_clk_gen.kill();
    data_clk_2x_gen.kill();
    rfdc_clk_gen.kill();

    test.end_tb(0);
  end

endmodule
