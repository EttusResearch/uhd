// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rx_dec3_tb.sv
//
// Description:  Testbench for rx_dec3.sv

module rx_dec3_tb;
    timeunit 1ns / 1ps;

    `include "test_exec.svh"

    import PkgTestExec::*;
    import PkgAxiStreamBfm::*;

    localparam int NUM_SAMPLES = 600;
    localparam int NUM_FIR_TAPS = 77;

    // Coefficients from ip/fir_dec3_2spc/fir_dec3_2spc.xci (PARAM_VALUE.CoefficientVector)
    localparam int signed FIR_COEFFS [0:NUM_FIR_TAPS-1] = '{
      2, 5, 0, -13, -20, 0, 40, 55, 0, -95, -121, 0, 191, 236, 0, -351,
      -422, 0, 599, 708, 0, -974, -1136, 0, 1534, 1778, 0, -2392, -2783, 0,
      3825, 4547, 0, -6775, -8668, 0, 17881, 36039, 43691, 36039, 17881, 0,
      -8668, -6775, 0, 4547, 3825, 0, -2783, -2392, 0, 1778, 1534, 0, -1136,
      -974, 0, 708, 599, 0, -422, -351, 0, 236, 191, 0, -121, -95, 0, 55,
      40, 0, -20, -13, 0, 5, 2
    };

    // Clock periods: Assume data_clk is 2x PRC, but it could be any integer
    // multiple of it.
    //   rfdc_clk    = 6x base_clk = 3x data_clk
    //   data_clk    = 2x pll_ref_clk
    localparam realtime PLL_REF_CLK_PERIOD = 12.0;
    localparam realtime RFDC_CLK_PERIOD  = PLL_REF_CLK_PERIOD / 6;
    localparam realtime DATA_CLK_PERIOD  = PLL_REF_CLK_PERIOD / 2;

    // signal definition
    logic pll_ref_clk;
    logic data_clk;
    logic rfdc_clk;
    logic reset_dclk;

    // AXI Stream interfaces
    AxiStreamIf  #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1)) m_axis_if
      (.clk(rfdc_clk), .rst());
    AxiStreamIf  #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1)) s_axis_if
      (.clk(data_clk), .rst(reset_dclk));
    AxiStreamBfm #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1)) axi_stream_bfm;

    // FIR monitor interface/BFM (capture via AXI BFM instead of direct polling)
    AxiStreamIf  #(.DATA_WIDTH(80), .TUSER(0), .TKEEP(0), .TLAST(1)) fir_s_axis_if
      (.clk(rfdc_clk), .rst());
    AxiStreamBfm #(.DATA_WIDTH(80), .TUSER(0), .TKEEP(0), .TLAST(1)) fir_stream_bfm;

    typedef AxiStreamBfm #(.DATA_WIDTH(64), .TUSER(0), .TKEEP(0), .TLAST(1))
      ::AxisPacket_t AxisPacket_t;
    AxisPacket_t packet_in;
    AxisPacket_t packet_out;
    typedef AxiStreamBfm #(.DATA_WIDTH(80), .TUSER(0), .TKEEP(0), .TLAST(1))
      ::AxisPacket_t FirAxisPacket_t;
    FirAxisPacket_t fir_packet_out;

    // generate clocks
    sim_clock_gen #(.PERIOD(PLL_REF_CLK_PERIOD), .AUTOSTART(0)) pll_ref_clk_gen
      (.clk(pll_ref_clk),  .rst());
    sim_clock_gen #(.PERIOD(DATA_CLK_PERIOD), .AUTOSTART(0)) data_clk_gen
      (.clk(data_clk),    .rst(reset_dclk));
    sim_clock_gen #(.PERIOD(RFDC_CLK_PERIOD), .AUTOSTART(0)) rfdc_clk_gen
      (.clk(rfdc_clk),    .rst());

    rx_dec3 dut (
      .rfdc_clk            (rfdc_clk),
      .data_clk            (data_clk),
      .pll_ref_clk         (pll_ref_clk),
      .reset_pulse_dclk    (reset_dclk),
      .adc_data_in_tdata   (m_axis_if.tdata),
      .adc_data_in_tvalid  (m_axis_if.tvalid),
      .adc_data_in_tready  (m_axis_if.tready),
      .adc_data_out_tdata  (s_axis_if.tdata),
      .adc_data_out_tvalid (s_axis_if.tvalid)
    );

    // tie off tlast and tready on the output side
    assign s_axis_if.tlast  = '1;
    assign s_axis_if.tready = '1;

    // Present DUT FIR output on an AXI stream interface for BFM-based capture
    assign fir_s_axis_if.tdata  = dut.fir_tdata;
    assign fir_s_axis_if.tvalid = dut.fir_tvalid;
    assign fir_s_axis_if.tlast  = '1;

    initial begin : tb_main
        test.start_tb("rx_dec3_tb");

        // start clock generators
        rfdc_clk_gen.start();
        data_clk_gen.start();
        pll_ref_clk_gen.start();

        // start BFMs
        axi_stream_bfm = new(m_axis_if, s_axis_if);
        axi_stream_bfm.set_master_stall_prob(0);
        axi_stream_bfm.set_slave_stall_prob(0);
        axi_stream_bfm.run();

        fir_stream_bfm = new(null, fir_s_axis_if);
        fir_stream_bfm.set_slave_stall_prob(0);
        fir_stream_bfm.run();

        // reset the DUT
        rfdc_clk_gen.reset(10);
        data_clk_gen.reset(10);
        wait(reset_dclk == 0);

        test.start_test("running data through the FIR stuff");

        // Generate samples with 0x4000 in all 16-bit fields
        // 128-bit word = 8 x 16-bit fields, each set to 0x4000
        repeat (NUM_SAMPLES) begin
          packet_in = new();
          packet_in.data.push_back({2{16'h1234, 16'h4000}});
          axi_stream_bfm.put(packet_in);
        end
        axi_stream_bfm.wait_send();

        // get all packets available from the bfm
        begin
          int num_out_packets;
          num_out_packets = axi_stream_bfm.num_received();
          $display("Number of output packets: %0d", num_out_packets);
          `ASSERT_ERROR(num_out_packets > 0, "No output packets received")
          repeat (num_out_packets) begin
            axi_stream_bfm.get(packet_out);
          end
          // check the last packet to contain the same data as on the input
          `ASSERT_ERROR(packet_out.data[$] === packet_in.data[$],
            $sformatf("Last output word mismatch. Got %h, expected %h",
                      packet_out.data[$], packet_in.data[$]));
        end

        test.end_test();

        test.start_test("check impulse responses");

        // Reset alone does not clear all FIR internal data state in this setup.
        // Explicitly flush with zeros before starting impulse reconstruction.
        repeat (120) begin
          packet_in = new();
          packet_in.data.push_back(128'h0);
          axi_stream_bfm.put(packet_in);
        end
        axi_stream_bfm.wait_send();
        wait(s_axis_if.tvalid == 0);

        // Drop any residual packets that may still be queued at the monitor.
        while (fir_stream_bfm.try_get(fir_packet_out));

        //assemble three packets of impulse responses which are 4 samples away
        //(3+1) from each other to output another part of the impulse response
        begin
          localparam int NUM_SAMPLES_IMPULSE_RESPONSE = 120;
          // The FIR phase is reversed in order compared to the input sample order.
          // run0->phase2, run1->phase1, run2->phase0.
          localparam int PHASE_MAP [0:2] = '{2, 1, 0};
          int reconstructed[NUM_SAMPLES_IMPULSE_RESPONSE] = '{default:0};

          for (int i = 0; i < 3; i++) begin
            packet_in = new();
            // 120 samples with 2 SPC = 60 packets
            for (int j = 0; j < NUM_SAMPLES_IMPULSE_RESPONSE/2; j++) begin
              // we need to put in an impulse response at the ith sample of
              // every burst. Therefore we take transfer j (which will be zero
              // for the first two phases and 1 for the last phase) and then
              // insert the impulse at sample index i%2 of that transfer. The
              // rest of the samples are zero.
              if (j==(i/2)) begin
                automatic logic [63:0] impulse = '0;
                impulse[32*(i%2) +: 16] = 16'h1;
                packet_in.data.push_back(impulse);
              end else
                packet_in.data.push_back('0);
            end
            axi_stream_bfm.put(packet_in);

            // Read FIR packets captured by the FIR monitor BFM.
            for (int p = 0; p < NUM_SAMPLES_IMPULSE_RESPONSE/3; p++) begin
              logic signed [33:0] sample;
              int dst;
              fir_stream_bfm.get(fir_packet_out);
              for (int s=0; s < 2; s++) begin
                // Take a single sample out of the FIR output
                sample = fir_packet_out.data[0][s*40 +: 34];
                // Map the sample to the corresponding position in the
                // reconstructed impulse response.
                // The position is 1 out of 3 phases (p), where 1 item in a
                // group of 3 is filled in each run.
                // The phase map is needed to identify which of the 3 samples in
                // the group is filled and the sample index (s) is needed to
                // step to the next group of 3 for the next sample.
                dst = p*3 + PHASE_MAP[i] + s*3;
                if (dst < NUM_SAMPLES_IMPULSE_RESPONSE) begin
                  reconstructed[dst] = sample;
                end
              end
            end
          end

          // check the main lobe of the impulse response matches the
          // coefficients
          foreach (FIR_COEFFS[idx]) begin
            if (reconstructed[idx] != FIR_COEFFS[idx]) begin
              `ASSERT_ERROR(0, $sformatf({"FIR output sample mismatch at sample index %0d.",
                                          " Got %d, expected %d (coeff index %0d)"},
                                         idx, reconstructed[idx], FIR_COEFFS[idx], idx));
            end
          end

          // check zero samples after the main lobe
          for (int idx = NUM_FIR_TAPS; idx < NUM_SAMPLES_IMPULSE_RESPONSE; idx++) begin
            if (reconstructed[idx] != 0) begin
              `ASSERT_ERROR(0, $sformatf({"Non-zero FIR output sample after main lobe",
                                          " at sample index %0d. Got %d, expected 0"},
                                         idx, reconstructed[idx]));
            end
          end
        end
        test.end_test();

        // test number of samples through the filter
        test.start_test("ensure all samples are processed");
        begin
          localparam int EXPECTED_OUT_TRANSFERS = NUM_SAMPLES / 3;
          int out_transfers;

          // Drain any residual transfers from previous test.
          wait(s_axis_if.tvalid == 0);
          while (axi_stream_bfm.try_get(packet_out));

          // Push a known number of input transfers and verify decimated output
          // transfer count for varying input stall probabilities.
          for (int s = 0; s < 4; s++) begin
            $display("Testing with master stall probability %0.1f%%", s * 20.0);
            axi_stream_bfm.set_master_stall_prob(s * 20);


            repeat (NUM_SAMPLES) begin
              packet_in = new();
              packet_in.data.push_back('0);
              axi_stream_bfm.put(packet_in);
            end
            axi_stream_bfm.wait_send();

            // wait some time to get all outputs and print some debug output
            data_clk_gen.clk_wait_r(100);
            $display("Received %0d output samples so far...", axi_stream_bfm.num_received());
            $display("Received %0d FIR output samples so far...", fir_stream_bfm.num_received());

            out_transfers = axi_stream_bfm.num_received();

            `ASSERT_ERROR(out_transfers == EXPECTED_OUT_TRANSFERS,
              $sformatf({"Number of output transfers mismatch for stall=%0d%%.",
                         " Got %0d, expected %0d"},
                        s * 20, out_transfers, EXPECTED_OUT_TRANSFERS))

            while (axi_stream_bfm.try_get(packet_out));
          end
        end
        test.end_test();

        // kill clock generators
        pll_ref_clk_gen.kill();
        rfdc_clk_gen.kill();
        data_clk_gen.kill();

        // end simulation
        test.end_tb(0);
    end

endmodule
