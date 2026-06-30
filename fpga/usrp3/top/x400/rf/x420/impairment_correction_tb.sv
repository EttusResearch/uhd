// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  impairment_correction_tb.sv
//
// Description:  Testbench for impairment_correction.sv

module impairment_correction_tb;
    timeunit 1ns / 1ps;

    import PkgTestExec::*;
    import XmlSvPkgIQ_IMPAIRMENT_REGMAP::*;
    import ctrlport_pkg::*;
    import ctrlport_bfm_pkg::*;
    import impairment_correction_test_pkg::*;
    import PkgAxiStreamBfm::*;

    // test definitions
    localparam int NUM_COEFFS = 15;
    localparam int NUM_SPC = 4;
    localparam int CLK_PERIOD = 10;

    // signal definition
    logic clk;
    logic reset;

    // there is no tlast but this signal is required for the BFM to work
    AxiStreamIf  #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*NUM_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) m_axis_if (.clk(clk), .rst(reset));
    AxiStreamIf  #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*NUM_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) s_axis_if (.clk(clk), .rst(reset));
    AxiStreamBfm #(.DATA_WIDTH(IQ_SAMPLE_WIDTH*NUM_SPC), .TUSER(0), .TKEEP(0), .TLAST(1)) axi_stream_bfm;

    // create a new instance of the test class
    ImpairmentCorrectionTestClass #(.NUM_SPC(NUM_SPC), .NUM_COEFFS(NUM_COEFFS)) bfm;

    // control port interfaces
    ctrlport_if s_ctrlport(.clk(clk),.rst(reset));
    ctrlport_bfm ctrl_bfm;

    // generate clocks
    sim_clock_gen #(.PERIOD(CLK_PERIOD)) clk_gen (.clk(clk), .rst(reset));

    impairment_correction #(
      .NUM_SPC   (NUM_SPC),
      .NUM_COEFFS(NUM_COEFFS)
    ) dut (
      .clk          (clk),
      .reset        (reset),
      .s_axis_tdata (m_axis_if.tdata),
      .s_axis_tvalid(m_axis_if.tvalid),
      .s_axis_tready(m_axis_if.tready),
      .m_axis_tdata (s_axis_if.tdata),
      .m_axis_tvalid(s_axis_if.tvalid),
      .m_axis_tready(s_axis_if.tready),
      .s_ctrlport   (s_ctrlport)
    );

    // tie off tlast since there is no packetization in this design
    assign s_axis_if.tlast = '1;

    initial begin : tb_main
        // Display testbench start message
        test.start_tb("impairment_correction_tb");

        // start bfms
        ctrl_bfm = new(s_ctrlport);
        ctrl_bfm.run();
        axi_stream_bfm = new(m_axis_if, s_axis_if);
        // Master stall probability is not used here as the interface to connect
        // to is the RFDC, which generates data each clock cycle without
        // backpressure.
        axi_stream_bfm.set_master_stall_prob(0);
        axi_stream_bfm.run();
        bfm = new(axi_stream_bfm, ctrl_bfm, 0);

        // reset the DUT and run the tests
        clk_gen.reset(127);
        bfm.run_all_tests();

        // 100 more cycles to end simulation
        repeat(100) @(negedge clk);
        test.end_tb(0);
        clk_gen.kill();
    end

endmodule
