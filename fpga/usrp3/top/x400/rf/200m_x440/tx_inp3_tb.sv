// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  tx_inp3_tb.sv
//
// Description:  Testbench for tx_inp3.sv

module tx_inp3_tb;
    timeunit 1ns / 1ps;

    `include "test_exec.svh"

    import PkgTestExec::*;
    import PkgAxiStreamBfm::*;

    localparam int NUM_SAMPLES = 300;
    localparam int NUM_FIR_TAPS = 81;

    // Coefficients from ip/fir_inp3_1spc/fir_inp3_1spc.xci (PARAM_VALUE.CoefficientVector)
    localparam int signed FIR_COEFFS [0:NUM_FIR_TAPS-1] = '{
      -7, 0, 24, 37, 0, -78, -107, 0, 189, 244, 0, -389, -484, 0, 723, 873,
      0, -1245, -1473, 0, 2029, 2364, 0, -3177, -3668, 0, 4862, 5592, 0,
      -7418, -8579, 0, 11675, 13820, 0, -20461, -26115, 0, 53699, 108144,
      131069, 108144, 53699, 0, -26115, -20461, 0, 13820, 11675, 0, -8579,
      -7418, 0, 5592, 4862, 0, -3668, -3177, 0, 2364, 2029, 0, -1473, -1245,
      0, 873, 723, 0, -484, -389, 0, 244, 189, 0, -107, -78, 0, 37, 24, 0,
      -7
    };

    // Clock periods: Assume data_clk is 2x PRC, but it could be any integer
    // multiple of it.
    //   data_clk    = 2x pll_ref_clk
    //   data_clk_2x = 2x data_clk = 4x pll_ref_clk
    //   rfdc_clk    = 3x data_clk = 6x pll_ref_clk
    localparam realtime PLL_REF_CLK_PERIOD = 12.0;
    localparam realtime DATA_CLK_PERIOD    = PLL_REF_CLK_PERIOD / 2;
    localparam realtime DATA_CLK_2X_PERIOD = PLL_REF_CLK_PERIOD / 4;
    localparam realtime RFDC_CLK_PERIOD    = PLL_REF_CLK_PERIOD / 6;

    // signal definition
    logic pll_ref_clk;
    logic data_clk;
    logic data_clk_2x;
    logic rfdc_clk;
    logic reset_pulse_dclk;

    // AXI Stream interfaces
    // Input: 64 bits @ data_clk (2 SPC)
    AxiStreamIf  #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1))
      m_axis_if (.clk(data_clk), .rst(reset_pulse_dclk));
    // Output: 64 bits @ rfdc_clk (2 SPC)
    AxiStreamIf  #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1))
      s_axis_if (.clk(rfdc_clk), .rst(reset_pulse_dclk));
    AxiStreamBfm #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1)) axi_stream_bfm;

    // FIR monitor interfaces/BFM (capture via AXI BFM instead of direct polling)
    AxiStreamIf  #(.DATA_WIDTH(240), .TUSER(0), .TKEEP(0), .TLAST(1))
      fir_s_axis_if (.clk(data_clk_2x), .rst(reset_pulse_dclk));
    AxiStreamBfm #(.DATA_WIDTH(240), .TUSER(0), .TKEEP(0), .TLAST(1)) fir_stream_bfm;

    typedef AxiStreamBfm #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1))
      ::AxisPacket_t AxisPacket_t;
    AxisPacket_t packet_in;
    AxisPacket_t packet_out;
    typedef AxiStreamBfm #(.DATA_WIDTH(240), .TUSER(0), .TKEEP(0), .TLAST(1))
      ::AxisPacket_t FirAxisPacket_t;
    FirAxisPacket_t fir_packet_out;

    // generate clocks
    sim_clock_gen #(.PERIOD(PLL_REF_CLK_PERIOD), .AUTOSTART(0))  pll_ref_clk_gen
      (.clk(pll_ref_clk), .rst());
    sim_clock_gen #(.PERIOD(DATA_CLK_PERIOD), .AUTOSTART(0))     data_clk_gen
      (.clk(data_clk),    .rst(reset_pulse_dclk));
    sim_clock_gen #(.PERIOD(DATA_CLK_2X_PERIOD), .AUTOSTART(0))  data_clk_2x_gen
      (.clk(data_clk_2x), .rst());
    sim_clock_gen #(.PERIOD(RFDC_CLK_PERIOD), .AUTOSTART(0))     rfdc_clk_gen
      (.clk(rfdc_clk),    .rst());

    tx_inp3 dut (
      .data_clk            (data_clk),
      .data_clk_2x         (data_clk_2x),
      .rfdc_clk            (rfdc_clk),
      .pll_ref_clk         (pll_ref_clk),
      .reset_pulse_dclk    (reset_pulse_dclk),
      .dac_data_in_tdata   (m_axis_if.tdata),
      .dac_data_in_tvalid  (m_axis_if.tvalid),
      .dac_data_out_tdata  (s_axis_if.tdata),
      .dac_data_out_tvalid (s_axis_if.tvalid)
    );

    // tx module is always ready to accept data
    assign m_axis_if.tready = '1;

    // tie off tlast and tready on the output side
    assign s_axis_if.tlast  = '1;
    assign s_axis_if.tready = '1;

    // Present DUT FIR output on an AXI stream interface for BFM-based capture
    assign fir_s_axis_if.tdata  = dut.fir_tdata;
    assign fir_s_axis_if.tvalid = dut.fir_tvalid;
    assign fir_s_axis_if.tlast  = '1;
    assign fir_s_axis_if.tready = '1;

    initial begin : tb_main
        test.start_tb("tx_inp3_tb");

        // start clock generators
        pll_ref_clk_gen.start();
        data_clk_gen.start();
        data_clk_2x_gen.start();
        rfdc_clk_gen.start();

        // start BFMs
        axi_stream_bfm = new(m_axis_if, s_axis_if);
        axi_stream_bfm.set_master_stall_prob(0);
        axi_stream_bfm.set_slave_stall_prob(0);
        axi_stream_bfm.run();

        fir_stream_bfm = new(null, fir_s_axis_if);
        fir_stream_bfm.set_slave_stall_prob(0);
        fir_stream_bfm.run();

        // reset the DUT
        data_clk_gen.reset(1);
        wait(reset_pulse_dclk == 0);
        // wait for the reset sequence to complete
        pll_ref_clk_gen.clk_wait_r(10);

        test.start_test("running data through the FIR filter");

        // Generate samples with 0x4000 on I, 0x2442 on Q
        // 64-bit word = [Q1,I1,Q0,I0] (2 SPC)
        repeat (NUM_SAMPLES) begin
          packet_in = new();
          packet_in.data.push_back({16'h2442, 16'h4000, 16'h2442, 16'h4000});
          axi_stream_bfm.put(packet_in);
        end
        axi_stream_bfm.wait_send();

        // wait for all the samples to come out
        wait(s_axis_if.tvalid == 0);

        // get all packets available from the bfm
        begin
          automatic int num_out_packets;
          num_out_packets = axi_stream_bfm.num_received();
          $display("Number of output packets: %0d", num_out_packets);
          `ASSERT_ERROR(num_out_packets > 0, "No output packets received")
          repeat (num_out_packets) begin
            axi_stream_bfm.get(packet_out);
          end
          // For DC input I=0x4000 Q=0x2442, steady-state output (2 SPC) should have
          // both slots at the same DC value: [Q1,I1,Q0,I0] = [0x2442,0x4000,0x2442,0x4000].
          `ASSERT_ERROR(packet_out.data[$] === 64'h2442_4000_2442_4000,
            $sformatf("Last output word mismatch. Got %h, expected 2442_4000_2442_4000",
                      packet_out.data[$]))
        end

        test.end_test();

        test.start_test("capture impulse response");

        // Flush the filter with zeros
        repeat (NUM_SAMPLES) begin
          packet_in = new();
          packet_in.data.push_back(64'h0);
          axi_stream_bfm.put(packet_in);
        end
        axi_stream_bfm.wait_send();

        // Drain output packets
        while (fir_stream_bfm.try_get(fir_packet_out));

        // Send a single impulse on I sample 0, rest zeros.
        // Use enough beats so the full impulse response drains out.
        packet_in = new();
        for (int i = 0; i < 100; i++) begin
          if (i == 0) begin
            // impulse on I0 only: [Q1,I1,Q0,I0] (2 SPC)
            packet_in.data.push_back({16'h0, 16'h0, 16'h0, 16'h1});
          end else begin
            packet_in.data.push_back('0);
          end
        end
        axi_stream_bfm.put(packet_in);

        // Capture FIR output via AXI BFM and reconstruct the impulse response.
        // The FIR has 6 output paths (40 bits each in 240-bit tdata):
        //   paths 0,2,4 = I samples for interpolation phases 0,1,2.
        // Collecting I0,I1,I2 per data_clk beat directly gives the full
        // coefficient sequence in order: coeff[3k], coeff[3k+1], coeff[3k+2].
        begin
          int signed reconstructed[$];
          int found_match = 0;
          int first_non_zero = -1;

          while (fir_stream_bfm.num_received() < 100) begin
            data_clk_gen.clk_wait_f(1);
          end

          // Get sample by sample from the FIR output stream
          while (fir_stream_bfm.try_get(fir_packet_out)) begin
            for (int k = 0; k < 6; k += 2) begin  // paths 0,2,4 = I channels
              logic signed [34:0] sample;
              sample = fir_packet_out.data[0][k*40 +: 35];
              reconstructed.push_back(int'(sample));
            end
          end

          // Search for the full coefficient sequence in the reconstructed
          // stream.
          `ASSERT_ERROR(reconstructed.size() >= NUM_FIR_TAPS,
                        $sformatf({"Not enough FIR output samples captured to find a match",
                                   " (captured %0d, need at least %0d)"},
                                   reconstructed.size(), NUM_FIR_TAPS))
          // find first non-zero sample
          foreach (reconstructed[i]) begin
            if (reconstructed[i] != 0) begin
              first_non_zero = i;
              break;
            end
          end
          `ASSERT_ERROR(first_non_zero >= 0, "Did not find any non-zero samples in FIR output")
          // check vector length
          `ASSERT_ERROR(reconstructed.size() - first_non_zero >= NUM_FIR_TAPS,
            $sformatf({"Not enough FIR output samples after first non-zero sample to find a",
                        " match (non-zero at index %0d, total reconstructed size %0d, need at",
                        " least %0d)"},
                      first_non_zero, reconstructed.size(), NUM_FIR_TAPS))
          // check the samples starting from the first non-zero sample against
          // the coefficients
          found_match = 1;
          foreach (FIR_COEFFS[i]) begin
            automatic int sample_index = first_non_zero + i;
            if (reconstructed[sample_index] !== FIR_COEFFS[i]) begin
              found_match = 0;
              `ASSERT_ERROR(0, $sformatf({"FIR output sample mismatch at sample index %0d",
                                          " (coeff index %0d). Got %d, expected %d"},
                                          sample_index, i,
                                          reconstructed[sample_index], FIR_COEFFS[i]))
            end
          end

          // check remaining samples are zero
          // (filter should have fully flushed out after the main lobe)
          for (int i = first_non_zero + NUM_FIR_TAPS; i < reconstructed.size(); i++) begin
            if (reconstructed[i] != 0) begin
              found_match = 0;
              `ASSERT_ERROR(0, $sformatf({"Non-zero FIR output sample after main lobe",
                                          " at sample index %0d. Got %d, expected 0"},
                                          i, reconstructed[i]))
            end
          end
        end

        test.end_test();

        // test number of samples through the filter
        test.start_test("ensure all samples are processed");
        // drain the output stream
        wait(s_axis_if.tvalid == 0);
        rfdc_clk_gen.clk_wait_r(2);
        while (axi_stream_bfm.try_get(packet_out));
        while (fir_stream_bfm.try_get(fir_packet_out));

        // push a known number of samples and ensure the same number comes out
        // (multiplied by interpolation factor of 3)
        for (int s = 0; s < 4; s++) begin
          // increase stall probability each iteration to test with different
          // stalling patterns
          $display("Testing with master stall probability %0.1f%%", s * 20.0);
          axi_stream_bfm.set_master_stall_prob(s * 20);
          packet_in = new();
          repeat (NUM_SAMPLES) begin
            packet_in.data.push_back({$urandom(), $urandom()});
          end
          axi_stream_bfm.put(packet_in);

          pll_ref_clk_gen.clk_wait_r(1000);
          $display("Received %0d output samples so far...", axi_stream_bfm.num_received());
          $display("Received %0d FIR output samples so far...", fir_stream_bfm.num_received());

          rfdc_clk_gen.clk_wait_r(100);
          `ASSERT_ERROR(axi_stream_bfm.num_received() == NUM_SAMPLES * 3,
            $sformatf("Number of output samples mismatch. Got %0d, expected %0d",
                      axi_stream_bfm.num_received(), NUM_SAMPLES * 3))

          while(axi_stream_bfm.try_get(packet_out));
          while(fir_stream_bfm.try_get(fir_packet_out));
        end
        test.end_test();

        // kill clock generators
        pll_ref_clk_gen.kill();
        data_clk_gen.kill();
        data_clk_2x_gen.kill();
        rfdc_clk_gen.kill();

        // end simulation
        test.end_tb(0);
    end

endmodule
