// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rf_core_200m_x440_tb.sv
//
// Description:  Basic data-flow testbench for rf_core_200m_x440.

module rf_core_200m_x440_tb;

  timeunit 1ns / 1ps;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;

  localparam int NUM_ADC_CHANNELS = 1;
  localparam int NUM_DAC_CHANNELS = 1;
  localparam int RADIO_SPC = 2;

  localparam int TX_IN_TRANSFERS      = 120;
  localparam int RX_IN_TRANSFERS      = 240;
  localparam int FLUSH_ZERO_TRANSFERS = 200;

  // Clock periods assuming data_clk = 2x PRC but might be any multiple of it.
  //   data_clk     = 2x pll_ref_clk
  //   data_clk_2x  = 4x pll_ref_clk
  //   rfdc_clk     = 6x pll_ref_clk
  localparam realtime PLL_REF_CLK_PERIOD = 12.0;
  localparam realtime DATA_CLK_PERIOD    = PLL_REF_CLK_PERIOD / 2;
  localparam realtime DATA_CLK_2X_PERIOD = PLL_REF_CLK_PERIOD / 4;
  localparam realtime RFDC_CLK_PERIOD    = PLL_REF_CLK_PERIOD / 6;

  logic pll_ref_clk;
  logic data_clk;
  logic data_clk_2x;
  logic rfdc_clk;

  logic tx_resampler_reset_pulse_dclk;
  logic rx_resampler_reset_pulse_dclk;

  // TX path
  AxiStreamIf #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    dac_in_m_axis_if (.clk(data_clk),  .rst());
  AxiStreamIf #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    dac_out_s_axis_if (.clk(rfdc_clk), .rst());

  // RX path
  AxiStreamIf #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    adc_in_m_axis_if (.clk(rfdc_clk),  .rst());
  AxiStreamIf #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    adc_out_s_axis_if (.clk(data_clk), .rst());

  // Separate master/slave BFMs for TX path
  AxiStreamBfm #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) tx_bfm;

  // Separate master/slave BFMs for RX path
  AxiStreamBfm #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) rx_bfm;

  typedef AxiStreamBfm #(.DATA_WIDTH(32*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1))
    ::AxisPacket_t AxisPacket_t;

  AxisPacket_t packet_in;
  AxisPacket_t packet_out;

  logic [16*RADIO_SPC-1:0]     adc_data_in_i_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready;
  logic [16*RADIO_SPC-1:0]     adc_data_in_q_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready;

  logic [32*RADIO_SPC-1:0]     dac_data_out_tdata [0:NUM_DAC_CHANNELS-1];
  logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tvalid;

  logic [32*RADIO_SPC-1:0]     adc_data_out_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid;

  logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tready;

  sim_clock_gen #(.PERIOD(PLL_REF_CLK_PERIOD), .AUTOSTART(0))  pll_ref_clk_gen
    (.clk(pll_ref_clk), .rst());
  sim_clock_gen #(.PERIOD(DATA_CLK_PERIOD), .AUTOSTART(0))     data_clk_gen
    (.clk(data_clk),    .rst());
  sim_clock_gen #(.PERIOD(DATA_CLK_2X_PERIOD), .AUTOSTART(0))  data_clk_2x_gen
    (.clk(data_clk_2x), .rst());
  sim_clock_gen #(.PERIOD(RFDC_CLK_PERIOD), .AUTOSTART(0))     rfdc_clk_gen
    (.clk(rfdc_clk),    .rst());

  // Connect user DAC input stream to DUT
  assign dac_in_m_axis_if.tready  = dac_data_in_tready[0];

  // Connect DUT DAC output stream to RFDC-facing sink stream
  assign dac_out_s_axis_if.tdata  = dac_data_out_tdata[0];
  assign dac_out_s_axis_if.tvalid = dac_data_out_tvalid[0];
  assign dac_out_s_axis_if.tlast  = '1;

  // Split packed IQ input stream ({Qn,In} per slot) into separate I/Q buses for RFDC ADC input
  for (genvar i = 0; i < RADIO_SPC; i++) begin : gen_adc_iq_split
    assign adc_data_in_i_tdata[0][16*i +: 16] = adc_in_m_axis_if.tdata[32*i      +: 16];
    assign adc_data_in_q_tdata[0][16*i +: 16] = adc_in_m_axis_if.tdata[32*i + 16 +: 16];
  end
  assign adc_in_m_axis_if.tready  = adc_data_in_i_tready[0];

  // Connect DUT ADC output stream to user-facing sink stream
  assign adc_out_s_axis_if.tdata  = adc_data_out_tdata[0];
  assign adc_out_s_axis_if.tvalid = adc_data_out_tvalid[0];
  assign adc_out_s_axis_if.tlast  = '1;

  rf_core_200m_x440 #(
    .NUM_ADC_CHANNELS(NUM_ADC_CHANNELS),
    .NUM_DAC_CHANNELS(NUM_DAC_CHANNELS)
  ) dut (
    .data_clk                     (data_clk),
    .data_clk_2x                  (data_clk_2x),
    .rfdc_clk                     (rfdc_clk),
    .pll_ref_clk                  (pll_ref_clk),
    .rx_resampler_reset_pulse_dclk(rx_resampler_reset_pulse_dclk),
    .tx_resampler_reset_pulse_dclk(tx_resampler_reset_pulse_dclk),
    .s_axi_config_clk             (data_clk),
    .adc_data_in_i_tdata          (adc_data_in_i_tdata),
    .adc_data_in_i_tready         (adc_data_in_i_tready),
    .adc_data_in_i_tvalid         ('{adc_in_m_axis_if.tvalid}),
    .adc_data_in_q_tdata          (adc_data_in_q_tdata),
    .adc_data_in_q_tready         (adc_data_in_q_tready),
    .adc_data_in_q_tvalid         ('{adc_in_m_axis_if.tvalid}),
    .dac_data_out_tdata           (dac_data_out_tdata),
    .dac_data_out_tready          (dac_out_s_axis_if.tready),
    .dac_data_out_tvalid          (dac_data_out_tvalid),
    .adc_data_out_tdata           (adc_data_out_tdata),
    .adc_data_out_tvalid          (adc_data_out_tvalid),
    .dac_data_in_tdata            ('{dac_in_m_axis_if.tdata}),
    .dac_data_in_tready           (dac_data_in_tready),
    .dac_data_in_tvalid           (dac_in_m_axis_if.tvalid),
    .invert_adc_iq_rclk           ('0),
    .invert_dac_iq_rclk           ('0),
    .dsp_info_sclk                (),
    .axi_status_sclk              (),
    .rfdc_info_sclk               (),
    .adc_enable_data_rclk         ('1),
    .adc_rfdc_axi_resetn_rclk     ('1),
    .version_info                 ()
  );

  initial begin : tb_main
    int tx_num_received;
    int rx_num_received;
    logic [32*RADIO_SPC-1:0] tx_ref_word;
    logic [32*RADIO_SPC-1:0] rx_ref_word;
    bit tx_clean;
    bit rx_clean;

    test.start_tb("rf_core_200m_x440_tb");

    // start clock generators
    pll_ref_clk_gen.start();
    data_clk_gen.start();
    data_clk_2x_gen.start();
    rfdc_clk_gen.start();

    tx_resampler_reset_pulse_dclk = 1'b0;
    rx_resampler_reset_pulse_dclk = 1'b0;

    // Initialize BFMs with separate master and slave instances
    tx_bfm = new(dac_in_m_axis_if, dac_out_s_axis_if);
    tx_bfm.set_master_stall_prob(0);
    tx_bfm.set_slave_stall_prob(0);
    tx_bfm.run();

    rx_bfm = new(adc_in_m_axis_if, adc_out_s_axis_if);
    rx_bfm.set_master_stall_prob(0);
    rx_bfm.set_slave_stall_prob(0);
    rx_bfm.run();

    // -------------------------------------------------------------------------
    // TX Tests
    // -------------------------------------------------------------------------
    @(posedge data_clk);
    tx_resampler_reset_pulse_dclk = 1'b1;
    @(posedge data_clk);
    tx_resampler_reset_pulse_dclk = 1'b0;

    repeat (64) @(posedge data_clk);

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

    test.start_test("TX constant output stable and non-zero");
    while (tx_bfm.try_get(packet_out));

    // Constant pattern [Q,I] = {0x4000,0x2000} (2 SPC = 64 bits @ data_clk)
    repeat (TX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back({16'h4000, 16'h2000, 16'h4000, 16'h2000});
      tx_bfm.put(packet_in);
    end
    tx_bfm.wait_send();

    // Drop older outputs and keep the final 64 transfers to check steady-state.
    // TX takes 2 SPC input and interpolates by 3, packs 2 SPC output, so TX_IN_TRANSFERS*3 output transfers.
    tx_num_received = tx_bfm.num_received();
    `ASSERT_ERROR(tx_num_received >= 64,
      "TX constant test: not enough output transfers to check steady-state")
    repeat (tx_num_received - 64) begin
      tx_bfm.get(packet_out);
    end

    tx_bfm.get(packet_out);
    tx_ref_word = packet_out.data[0];

    `ASSERT_ERROR(!$isunknown(tx_ref_word),
      "TX constant reference output contains unknown X bits")
    `ASSERT_ERROR(tx_ref_word != '0,
      "TX constant steady-state output is zero")

    for (int i = 1; i < 64; i++) begin
      tx_bfm.get(packet_out);
      `ASSERT_ERROR(!$isunknown(packet_out.data[0]),
        $sformatf("TX constant output contains unknown X bits at steady-state sample %0d", i))
      `ASSERT_ERROR(packet_out.data[0] === tx_ref_word,
        $sformatf("TX constant output not stable at steady-state sample %0d. Got %h, expected %h",
                  i, packet_out.data[0], tx_ref_word))
    end

    test.end_test();

    test.start_test("TX - Verify number of transfers received");
    pll_ref_clk_gen.clk_wait_r(100);
    while (tx_bfm.try_get(packet_out));
    repeat (TX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back('0);
      tx_bfm.put(packet_in);
    end
    tx_bfm.wait_send();
    pll_ref_clk_gen.clk_wait_r(100);

    // TX_IN_TRANSFERS transfers × RADIO_SPC SPC * interp3 / RADIO_SPC SPC
    tx_num_received = tx_bfm.num_received();
    `ASSERT_ERROR(tx_num_received == TX_IN_TRANSFERS * 3,
      $sformatf("Number of TX output transfers incorrect. Exp=%0d, Got=%0d",
        TX_IN_TRANSFERS * 3, tx_num_received))
    test.end_test();

    // -------------------------------------------------------------------------
    // RX Tests
    // -------------------------------------------------------------------------
    @(posedge data_clk);
    rx_resampler_reset_pulse_dclk = 1'b1;
    @(posedge data_clk);
    rx_resampler_reset_pulse_dclk = 1'b0;

    repeat (64) @(posedge data_clk);

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

    test.start_test("RX constant output stable and non-zero");
    while (rx_bfm.try_get(packet_out));

    // Constant pattern: 2 SPC packed as {Q1,I1,Q0,I0} = 64 bits @ rfdc_clk
    repeat (RX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back({16'h3000, 16'h1800, 16'h3000, 16'h1800});
      rx_bfm.put(packet_in);
    end
    rx_bfm.wait_send();

    // RX decimates by 3; keep the final 16 transfers to check steady-state.
    rx_num_received = rx_bfm.num_received();
    `ASSERT_ERROR(rx_num_received >= 16,
      "RX constant test: not enough output transfers to check steady-state")
    repeat (rx_num_received - 16) begin
      rx_bfm.get(packet_out);
    end

    rx_bfm.get(packet_out);
    rx_ref_word = packet_out.data[0];

    `ASSERT_ERROR(!$isunknown(rx_ref_word),
      "RX constant reference output contains unknown X bits")
    `ASSERT_ERROR(rx_ref_word != '0,
      "RX constant steady-state output is zero")

    for (int i = 1; i < 16; i++) begin
      rx_bfm.get(packet_out);
      `ASSERT_ERROR(!$isunknown(packet_out.data[0]),
        $sformatf("RX constant output contains unknown X bits at steady-state sample %0d", i))
      `ASSERT_ERROR(packet_out.data[0] === rx_ref_word,
        $sformatf("RX constant output not stable at steady-state sample %0d. Got %h, expected %h",
                  i, packet_out.data[0], rx_ref_word))
    end

    test.end_test();

    test.start_test("RX - Verify number of transfers received");
    pll_ref_clk_gen.clk_wait_r(100);
    while (rx_bfm.try_get(packet_out));
    repeat (RX_IN_TRANSFERS) begin
      packet_in = new();
      packet_in.data.push_back('0);
      rx_bfm.put(packet_in);
    end
    rx_bfm.wait_send();
    pll_ref_clk_gen.clk_wait_r(100);

    // RX_IN_TRANSFERS * RADIO_SPC samples / dec3 / RADIO_SPC
    rx_num_received = rx_bfm.num_received();
    `ASSERT_ERROR(rx_num_received == RX_IN_TRANSFERS / 3,
      $sformatf("Number of RX output transfers incorrect. Exp=%0d, Got=%0d",
        RX_IN_TRANSFERS / 3, rx_num_received))
    test.end_test();

    // kill all clocks
    pll_ref_clk_gen.kill();
    data_clk_gen.kill();
    data_clk_2x_gen.kill();
    rfdc_clk_gen.kill();

    test.end_tb(0);
  end

endmodule
