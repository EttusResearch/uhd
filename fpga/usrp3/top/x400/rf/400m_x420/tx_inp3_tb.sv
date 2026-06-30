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

    // Coefficients from ip/fir_inp3_2spc/fir_inp3_2spc.xci (PARAM_VALUE.CoefficientVector)
    localparam int signed FIR_COEFFS [0:NUM_FIR_TAPS-1] = '{
      -7, 0, 24, 37, 0, -78, -107, 0, 189, 244, 0, -389, -484, 0, 723, 873,
      0, -1245, -1473, 0, 2029, 2364, 0, -3177, -3668, 0, 4862, 5592, 0,
      -7418, -8579, 0, 11675, 13820, 0, -20461, -26115, 0, 53699, 108144,
      131069, 108144, 53699, 0, -26115, -20461, 0, 13820, 11675, 0, -8579,
      -7418, 0, 5592, 4862, 0, -3668, -3177, 0, 2364, 2029, 0, -1473, -1245,
      0, 873, 723, 0, -484, -389, 0, 244, 189, 0, -107, -78, 0, 37, 24, 0,
      -7
    };

    // Clock periods: pll_ref_clk is the base clock.
    //   data_clk    = 2x pll_ref_clk
    //   data_clk_2x = 4x pll_ref_clk
    //   rfdc_clk    = 6x pll_ref_clk
    localparam realtime PLL_REF_CLK_PERIOD  = 12.0;
    localparam realtime DATA_CLK_PERIOD     = PLL_REF_CLK_PERIOD / 2;
    localparam realtime DATA_CLK_2X_PERIOD  = PLL_REF_CLK_PERIOD / 4;
    localparam realtime RFDC_CLK_PERIOD     = PLL_REF_CLK_PERIOD / 6;

    // signal definition
    logic pll_ref_clk;
    logic data_clk;
    logic data_clk_2x;
    logic rfdc_clk;
    logic reset;

    // AXI Stream interfaces
    // Input: 128 bits @ data_clk (4 SPC)
    AxiStreamIf  #(.DATA_WIDTH(128), .TUSER(0), .TKEEP(0), .TLAST(1))
      m_axis_if (.clk(data_clk), .rst(reset));
    // Output: 128 bits @ rfdc_clk (4 SPC)
    AxiStreamIf  #(.DATA_WIDTH(128), .TUSER(0), .TKEEP(0), .TLAST(1))
      s_axis_if (.clk(rfdc_clk), .rst(reset));
    AxiStreamBfm #(.DATA_WIDTH(128), .TUSER(0), .TKEEP(0), .TLAST(1)) axi_stream_bfm;

    // FIR monitor interfaces/BFM (capture via AXI BFM instead of direct polling)
    AxiStreamIf  #(.DATA_WIDTH(480), .TUSER(0), .TKEEP(0), .TLAST(1))
      fir_s_axis_if (.clk(data_clk_2x), .rst(reset));
    AxiStreamBfm #(.DATA_WIDTH(480), .TUSER(0), .TKEEP(0), .TLAST(1)) fir_stream_bfm;

    typedef AxiStreamBfm #(.DATA_WIDTH(128), .TUSER(0), .TKEEP(0), .TLAST(1))
      ::AxisPacket_t AxisPacket_t;
    AxisPacket_t packet_in;
    AxisPacket_t packet_out;
    typedef AxiStreamBfm #(.DATA_WIDTH(480), .TUSER(0), .TKEEP(0), .TLAST(1))
      ::AxisPacket_t FirAxisPacket_t;
    FirAxisPacket_t fir_packet_out;

    // reset pulse in data_clk domain
    logic reset_pulse_dclk;

    // generate clocks
    sim_clock_gen #(.PERIOD(PLL_REF_CLK_PERIOD), .AUTOSTART(0))  pll_ref_clk_gen
      (.clk(pll_ref_clk), .rst(reset));
    sim_clock_gen #(.PERIOD(DATA_CLK_PERIOD), .AUTOSTART(0))     data_clk_gen
      (.clk(data_clk),    .rst(reset_pulse_dclk));
    sim_clock_gen #(.PERIOD(DATA_CLK_2X_PERIOD), .AUTOSTART(0))  data_clk_2x_gen
      (.clk(data_clk_2x), .rst());
    sim_clock_gen #(.PERIOD(RFDC_CLK_PERIOD), .AUTOSTART(0))     rfdc_clk_gen
      (.clk(rfdc_clk),    .rst());

    tx_inp3 dut (
      .data_clk          (data_clk),
      .data_clk_2x       (data_clk_2x),
      .rfdc_clk          (rfdc_clk),
      .pll_ref_clk       (pll_ref_clk),
      .reset_pulse_dclk  (reset_pulse_dclk),
      .dac_data_in_tdata (m_axis_if.tdata),
      .dac_data_in_tvalid(m_axis_if.tvalid),
      .dac_data_out_tdata (s_axis_if.tdata),
      .dac_data_out_tvalid(s_axis_if.tvalid)
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
        rfdc_clk_gen.start();
        data_clk_gen.start();
        data_clk_2x_gen.start();

        // start BFMs
        axi_stream_bfm = new(m_axis_if, s_axis_if);
        axi_stream_bfm.set_master_stall_prob(0);
        axi_stream_bfm.set_slave_stall_prob(0);
        axi_stream_bfm.run();

        fir_stream_bfm = new(null, fir_s_axis_if);
        fir_stream_bfm.set_slave_stall_prob(0);
        fir_stream_bfm.run();

        // reset the DUT
        pll_ref_clk_gen.reset(10);
        wait(reset == 0);
        data_clk_gen.reset(1);
        wait(reset_pulse_dclk == 0);

        // wait for the reset sequence to complete
        wait(dut.resetn_dclk == 1);
        pll_ref_clk_gen.clk_wait_r(10);

        test.start_test("running data through the FIR filter");

        // Generate samples with 0x4000 on I, 0x0000 on Q
        // 128-bit word = [Q3,I3,Q2,I2,Q1,I1,Q0,I0]
        packet_in = new();
        repeat (NUM_SAMPLES) begin
          packet_in.data.push_back({4{16'h0, 16'h4000}});
        end
        axi_stream_bfm.put(packet_in);
        axi_stream_bfm.wait_complete();

        // wait for all samples to come out
        begin
          int low_count = 0;
          while (low_count < 100) begin
            @(posedge rfdc_clk);
            if (s_axis_if.tvalid == 0)
              low_count++;
            else
              low_count = 0;
          end
        end
        $display("All output samples received at time %0t", $time);

        // get all packets available from the bfm
        begin
          automatic int num_out_packets;
          num_out_packets = axi_stream_bfm.num_received();
          $display("Number of output packets: %0d", num_out_packets);
          `ASSERT_ERROR(num_out_packets > 0, "No output packets received")
          repeat (num_out_packets) begin
            axi_stream_bfm.get(packet_out);
          end
          // For interpolation with DC input, steady-state output should match
          // input (FIR DC gain / round-clip preserves the value).
          `ASSERT_ERROR(packet_out.data[$] === packet_in.data[$],
            $sformatf("Last output word mismatch. Got %h, expected %h",
                      packet_out.data[$], packet_in.data[$]))
        end

        test.end_test();

        test.start_test("capture impulse response");

        // Flush the filter with zeros
        packet_in = new();
        repeat (120) begin
          packet_in.data.push_back(128'h0);
        end
        axi_stream_bfm.put(packet_in);
        axi_stream_bfm.wait_complete();

        // Drain output packets
        while (fir_stream_bfm.try_get(fir_packet_out));

        // Send a single impulse on I sample 0, rest zeros.
        // Use enough beats so the full impulse response drains out.
        packet_in = new();
        for (int i = 0; i < 100; i++) begin
          if (i == 0) begin
            // impulse on I0 only: [Q3,I3,Q2,I2,Q1,I1,Q0,I0]
            packet_in.data.push_back({112'h0, 16'h1});
          end else begin
            packet_in.data.push_back(128'h0);
          end
        end
        axi_stream_bfm.put(packet_in);

        // Capture FIR output via AXI BFM and check the impulse response.
        // The FIR has 12 output paths (40 bits each in 480-bit tdata), but only
        // the even paths (0,2,4,6,8,10) are active due to interpolation-by-3
        // with 4 SPC input at data_clk_2x. Interleaving even paths per time
        // step reconstructs the full coefficient sequence.
        begin
          int signed reconstructed[$];
          int found_match = 0;

          // Capture 2 times the number of input samples as FIR filter operates
          // on half the samples at a faster clock.
          while (fir_stream_bfm.num_received() < 200) begin
            data_clk_2x_gen.clk_wait_f(1);
          end

          // Get sample by sample from the FIR output stream
          while (fir_stream_bfm.try_get(fir_packet_out)) begin
            for (int k = 0; k < 12; k += 2) begin
              logic signed [34:0] sample;
              // Each packet has just one data word (as we fixed tlast to 1)
              sample = fir_packet_out.data[0][k*40 +: 35];
              reconstructed.push_back(int'(sample));
            end
          end

          // Search for the full coefficient sequence in the reconstructed stream.
          if (reconstructed.size() >= NUM_FIR_TAPS) begin
            // find first non-zero sample
            int first_non_zero = -1;
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

            `ASSERT_ERROR(found_match,
              $sformatf({"Did not find full FIR coefficient sequence",
                         " in reconstructed output (reconstructed size=%0d)"},
                        reconstructed.size()))
          end else begin
            `ASSERT_ERROR(0, $sformatf({"Not enough FIR output samples captured to find a match",
                                        " (captured %0d, need at least %0d)"},
                                       reconstructed.size(), NUM_FIR_TAPS))
          end
        end

        test.end_test();

        // test number of samples through the filter
        test.start_test("ensure all samples are processed");
        // drain the output stream
        wait(s_axis_if.tvalid == 0);
        rfdc_clk_gen.clk_wait_r(2);
        while (axi_stream_bfm.try_get(packet_out));

        // push a known number of samples and ensure the same number comes out
        // (multiplied by interpolation factor of 3)
        for (int s = 0; s < 4; s++) begin
          // increase stall probability each iteration to test with different stalling patterns
          axi_stream_bfm.set_master_stall_prob(s * 20);
          packet_in = new();
          repeat (NUM_SAMPLES) begin
            packet_in.data.push_back('0);
          end
          axi_stream_bfm.put(packet_in);

          while (axi_stream_bfm.num_received() < NUM_SAMPLES * 3) begin
            rfdc_clk_gen.clk_wait_f(1);
          end
          rfdc_clk_gen.clk_wait_r(100);
          `ASSERT_ERROR(axi_stream_bfm.num_received() == NUM_SAMPLES * 3,
            $sformatf("Number of output samples mismatch. Got %0d, expected %0d",
                      axi_stream_bfm.num_received(), NUM_SAMPLES * 3))

          while(axi_stream_bfm.try_get(packet_out)) begin
            // do nothing, just empty the received packets
            ;
          end
        end
        test.end_test();

        // wait a few cycles to see the simulation results
        pll_ref_clk_gen.clk_wait_r(10);

        // kill clock generators
        pll_ref_clk_gen.kill();
        data_clk_gen.kill();
        data_clk_2x_gen.kill();
        rfdc_clk_gen.kill();

        // end simulation
        test.end_tb(0);
    end

endmodule
