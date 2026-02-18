//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Testbench for the DC offset module.
//
// Description:
//
//   Tests bypass mode as well as actual DC offset compensation with random offsets.
//

module dc_offset_tb;
  timeunit 1ns / 1ps;

  import ctrlport_pkg::*;
  import ctrlport_bfm_pkg::*;

  import PkgAxiStreamBfm::*;
  import PkgTestExec::*;
  import PkgRandom::*;
  import PkgComplex::*;

  import XmlSvPkgDC_OFFSET_REGMAP::*;

  // Test parameters
  localparam int NUM_SPC = 4;
  localparam int CLK_PERIOD = 10;
  localparam int NUM_SAMPLES = 1000;

  // Signals
  logic clk;
  logic reset;

  // AXI Stream interfaces
  localparam int AXI_WIDTH = 32*NUM_SPC;
  AxiStreamIf #(.DATA_WIDTH(AXI_WIDTH), .TUSER(0), .TKEEP(0), .TLAST(1)) s_axis_if (.clk(clk), .rst(reset));
  AxiStreamIf #(.DATA_WIDTH(AXI_WIDTH), .TUSER(0), .TKEEP(0), .TLAST(1)) m_axis_if (.clk(clk), .rst(reset));
  AxiStreamBfm #(.DATA_WIDTH(AXI_WIDTH), .TUSER(0), .TKEEP(0), .TLAST(1)) data_bfm = new(s_axis_if ,m_axis_if);

  typedef AxiStreamBfm #(.DATA_WIDTH(AXI_WIDTH), .TUSER(0), .TKEEP(0), .TLAST(1))::AxisPacket_t AxisPacket_in;
  AxisPacket_in packet_in;
  AxisPacket_in packet_out;
  complex_t dc_offset;

  // Control port interface and BFM
  ctrlport_if s_ctrlport(.clk(clk), .rst(reset));
  ctrlport_bfm ctrl_bfm;

  // Clock generator
  sim_clock_gen #(.PERIOD(CLK_PERIOD)) clk_gen (.clk(clk), .rst(reset));

  // DUT instantiation
  dc_offset #(
    .NUM_SPC(NUM_SPC)
  ) dut (
    .clk           (clk),
    .reset         (reset),
    .s_axis_tdata  (s_axis_if.tdata),
    .s_axis_tvalid (s_axis_if.tvalid),
    .s_axis_tready (s_axis_if.tready),
    .s_axis_tlast  (s_axis_if.tlast),
    .m_axis_tdata  (m_axis_if.tdata),
    .m_axis_tvalid (m_axis_if.tvalid),
    .m_axis_tready (m_axis_if.tready),
    .m_axis_tlast  (m_axis_if.tlast),
    .s_ctrlport    (s_ctrlport)
  );

  // Main test sequence
  initial begin : tb_main
    CONTROL_REG control;
    test.start_tb("DC Offset Correction Testbench");

    // Start BFMs
    ctrl_bfm = new(s_ctrlport);
    ctrl_bfm.run();
    data_bfm.run();

    // Reset DUT
    clk_gen.reset(32);

    // Set mode to bypass
    test.start_test("Bypass mode");
    control.MODE = BYPASS;
    ctrl_bfm.write(kCONTROL_REG, control);

    // Generate 1000 random samples
    packet_in = new();
    for (int i = 0; i < NUM_SAMPLES; i++) begin
      packet_in.data.push_back(Rand#(AXI_WIDTH)::rand_bit());
      // needed for comparison later
      packet_in.user.push_back('x);
      packet_in.keep.push_back('x);
    end
    data_bfm.put(packet_in.copy());
    data_bfm.get(packet_out);

    packet_in.verbose = '1;
    assert (packet_in.equal(packet_out)) else $error("Data mismatch in bypass mode");

    test.end_test();

    // Set mode to actual compensation
    for (int i = 0; i < 5; i++) begin
      test.start_test($sformatf("DC offset mode iteration %0d", i));

      control.MODE = CORRECTION;
      ctrl_bfm.write(kCONTROL_REG, control);

      dc_offset = build_complex(frand_range(-1, 1), frand_range(-1, 1));
      ctrl_bfm.write(kOFFSET_VALUE_REG, complex_to_sc16(dc_offset));

      data_bfm.put(packet_in.copy());
      data_bfm.get(packet_out);

      // calculate expected output
      begin
        complex_t expected_sample, input_sample;
        sc16_t output_sample;
        for (int j = 0; j < NUM_SAMPLES; j++) begin
          input_sample = sc16_to_complex(packet_in.data[j]);
          expected_sample = sub(input_sample, dc_offset);
          output_sample = packet_out.data[j];
          assert (output_sample === complex_to_sc16(expected_sample)) else
            $error("Data mismatch at sample %0d: expected %0d + %0dj, got %0d + %0dj",
              j,
              complex_to_sc16(expected_sample).re,
              complex_to_sc16(expected_sample).im,
              output_sample.re,
              output_sample.im);
        end
      end
      test.end_test();
    end

    // Check bypass again
    test.start_test("Bypass mode (2nd time)");
    control.MODE = BYPASS;
    ctrl_bfm.write(kCONTROL_REG, control);
    data_bfm.put(packet_in.copy());
    data_bfm.get(packet_out);
    assert (packet_in.equal(packet_out)) else $error("Data mismatch in bypass mode");
    test.end_test();

    // End simulation
    test.end_tb();
    repeat(10) @(negedge clk);
    $display("dc_offset_tb: PASS");
    $finish;
  end

endmodule