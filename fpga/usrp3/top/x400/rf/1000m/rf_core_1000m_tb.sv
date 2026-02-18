// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rf_core_1000m_tb.sv
//
// Description:  Testbench for IQ impairments within RF core 1000 MHz

module rf_core_1000m_tb;

    timeunit 1ns / 1ps;

    import PkgTestExec::*;
    import XmlSvPkgIQ_IMPAIRMENT_REGMAP::*;
    import ctrlport_pkg::*;
    import ctrlport_bfm_pkg::*;
    import impairment_correction_test_pkg::*;
    import XmlSvPkgRF_CORE_REGMAP::*;
    import PkgAxiStreamBfm::*;

    // test definitions
    localparam int NUM_ADC_CHANNELS = 1;
    localparam int NUM_DAC_CHANNELS = 1;
    localparam int NUM_COEFFS = 15;
    localparam int RADIO_SPC = 4;
    localparam int CLK_PERIOD = 10;

    // signal definition
    logic clk;
    logic reset;

    AxiStreamIf  #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) dac_m_axis_if (.clk(clk), .rst(reset));
    AxiStreamIf  #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) dac_s_axis_if (.clk(clk), .rst(reset));
    AxiStreamIf  #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) adc_m_axis_if (.clk(clk), .rst(reset));
    AxiStreamIf  #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) adc_s_axis_if (.clk(clk), .rst(reset));

    AxiStreamBfm #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) dac_bfm;
    AxiStreamBfm #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*RADIO_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) adc_bfm;

    // control port interfaces
    ctrlport_if s_ctrlport(.clk(clk),.rst(reset));
    ctrlport_bfm ctrl_bfm;
    logic [CTRLPORT_STS_W-1:0] s_ctrlport_resp_status;

    // generate clocks
    sim_clock_gen #(.PERIOD(CLK_PERIOD)) clk_gen (.clk(clk), .rst(reset));

    // define ADC / DAC data interfaces
    logic [16*RADIO_SPC-1:0] adc_data_in_i_tdata [0:NUM_ADC_CHANNELS-1];
    logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready;
    logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tvalid;
    logic [16*RADIO_SPC-1:0] adc_data_in_q_tdata [0:NUM_ADC_CHANNELS-1];
    logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready;
    logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tvalid;
    logic [32*RADIO_SPC-1:0] adc_data_out_tdata [0:NUM_ADC_CHANNELS-1];
    logic [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid;

    logic [32*RADIO_SPC-1:0] dac_data_in_tdata [0:NUM_ADC_CHANNELS-1];
    logic [NUM_ADC_CHANNELS-1:0] dac_data_in_tready;
    logic [NUM_ADC_CHANNELS-1:0] dac_data_in_tvalid;
    logic [32*RADIO_SPC-1:0] dac_data_out_tdata [0:NUM_ADC_CHANNELS-1];
    logic [NUM_ADC_CHANNELS-1:0] dac_data_out_tready;
    logic [NUM_ADC_CHANNELS-1:0] dac_data_out_tvalid;

    // link interfaces to data signals
    // DAC IN
    assign dac_data_in_tdata[0]  = dac_m_axis_if.tdata;
    assign dac_data_in_tvalid[0] = dac_m_axis_if.tvalid;
    assign dac_m_axis_if.tready  = dac_data_in_tready[0];
    // DAC out
    assign dac_s_axis_if.tdata   = dac_data_out_tdata[0];
    assign dac_s_axis_if.tvalid  = dac_data_out_tvalid[0];
    assign dac_s_axis_if.tlast   = '1; // tie off tlast since there is no packetization in this design
    assign dac_data_out_tready[0]= dac_s_axis_if.tready;
    // ADC IN, which has to be split for I and Q
    for (genvar i = 0; i < RADIO_SPC; i = i+1) begin : gen_adc_data
        assign adc_data_in_i_tdata[0][16*i+:16] = adc_m_axis_if.tdata[32*i+:16];
        assign adc_data_in_q_tdata[0][16*i+:16] = adc_m_axis_if.tdata[32*i+16+:16];
    end
    assign adc_data_in_i_tvalid[0] = adc_m_axis_if.tvalid;
    assign adc_data_in_q_tvalid[0] = adc_m_axis_if.tvalid;
    assign adc_m_axis_if.tready   = adc_data_in_i_tready[0];
    always_comb begin
      assert (adc_data_in_i_tready[0] === adc_data_in_q_tready[0]) else
        $error("adc_data_in_i_tready and adc_data_in_q_tready must match at all times");
    end
    // ADC out (no tready)
    assign adc_s_axis_if.tdata = adc_data_out_tdata[0];
    assign adc_s_axis_if.tvalid = adc_data_out_tvalid[0];
    assign adc_s_axis_if.tlast  = '1; // tie off tlast since there is no packetization in this design

    // type cast
    assign s_ctrlport.resp.status = ctrlport_status_t'(s_ctrlport_resp_status);

    // DUT
    rf_core_1000m #(
      .RADIO_SPC (RADIO_SPC)
    ) rf_core_inst (
      .rfdc_clk                (clk),
      .s_axi_config_clk        ('0),
      .ctrlport_rst            (s_ctrlport.rst),
      .s_ctrlport_req_wr       (s_ctrlport.req.wr),
      .s_ctrlport_req_rd       (s_ctrlport.req.rd),
      .s_ctrlport_req_addr     (s_ctrlport.req.addr),
      .s_ctrlport_req_data     (s_ctrlport.req.data),
      .s_ctrlport_resp_ack     (s_ctrlport.resp.ack),
      .s_ctrlport_resp_status  (s_ctrlport_resp_status),
      .s_ctrlport_resp_data    (s_ctrlport.resp.data),
      .adc_data_in_i_tdata     (adc_data_in_i_tdata),
      .adc_data_in_i_tready    (adc_data_in_i_tready),
      .adc_data_in_i_tvalid    (adc_data_in_i_tvalid),
      .adc_data_in_q_tdata     (adc_data_in_q_tdata),
      .adc_data_in_q_tready    (adc_data_in_q_tready),
      .adc_data_in_q_tvalid    (adc_data_in_q_tvalid),
      .dac_data_out_tdata      (dac_data_out_tdata),
      .dac_data_out_tready     (dac_data_out_tready),
      .dac_data_out_tvalid     (dac_data_out_tvalid),
      .adc_data_out_tdata      (adc_data_out_tdata),
      .adc_data_out_tvalid     (adc_data_out_tvalid),
      .dac_data_in_tdata       (dac_data_in_tdata),
      .dac_data_in_tready      (dac_data_in_tready),
      .dac_data_in_tvalid      (dac_data_in_tvalid),
      .invert_adc_iq_rclk      ('0),
      .invert_dac_iq_rclk      ('0),
      .dsp_info_sclk           (),
      .axi_status_sclk         (),
      .rfdc_info_sclk          (),
      .adc_enable_data_rclk    ('1),
      .adc_rfdc_axi_resetn_rclk('0),
      .version_info            ()
    );

    // create a new instance of the test class
    ImpairmentCorrectionTestClass #(.NUM_SPC(RADIO_SPC), .NUM_COEFFS(NUM_COEFFS)) tx_bfm;
    ImpairmentCorrectionTestClass #(.NUM_SPC(RADIO_SPC), .NUM_COEFFS(NUM_COEFFS)) rx_bfm;

    initial begin : tb_main
        // Display testbench start message
        test.start_tb("rf_core_1000m_tb (TX)");

        // start bfms
        ctrl_bfm = new(s_ctrlport);
        ctrl_bfm.run();

        dac_bfm = new(dac_m_axis_if, dac_s_axis_if);
        // Master stall probability is not used here as the interface to connect
        // to is the RFDC, which generates data each clock cycle without
        // backpressure.
        dac_bfm.set_master_stall_prob(0);
        dac_bfm.run();
        tx_bfm = new(dac_bfm, ctrl_bfm, kTX_IQ_IMPAIRMENTS);

        adc_bfm = new(adc_m_axis_if, adc_s_axis_if);
        // Master stall probability is not used here as the interface to connect
        // to is the RFDC, which generates data each clock cycle without
        // backpressure. Slave stall probability is also set to 0 since there is
        // no tready on the ADC output interface.
        adc_bfm.set_master_stall_prob(0);
        adc_bfm.set_slave_stall_prob(0);
        adc_bfm.run();
        rx_bfm = new(adc_bfm, ctrl_bfm, kRX_IQ_IMPAIRMENTS);

        // reset the DUT
        clk_gen.reset(127);

        // run tests and switch test printout to RX
        tx_bfm.run_all_tests();
        test.end_tb(0);
        test.start_tb("rf_core_1000m_tb (RX)");
        rx_bfm.run_all_tests();

        // 100 more cycles to end simulation
        repeat(100) @(negedge clk);
        test.end_tb(0);
        clk_gen.kill();
    end

endmodule
