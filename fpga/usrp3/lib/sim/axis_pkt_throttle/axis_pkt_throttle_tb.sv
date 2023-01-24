//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: align_samples_tb
//
// Description:
//
//   Testbench for axis_pkt_throttle.
//

`default_nettype none


module axis_pkt_throttle_tb ();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;
  import PkgRandom::*;

  localparam real CLK_PERIOD  = 10.0;
  localparam int  THROTTLE_W  = 8;
  localparam int  DATA_W      = 32;
  localparam int  MTU         = 4;
  localparam int  MAX_PKT_LEN = 2**MTU;
  localparam int  NUM_PACKETS = 100;

  localparam int FRAC_W   = THROTTLE_W/2;
  localparam int WHOLE_W  = THROTTLE_W - FRAC_W;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD))
    clk_gen (.clk(clk), .rst(rst));

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic [THROTTLE_W-1:0] throttle;

  logic [DATA_W-1:0] i_tdata;
  logic              i_tlast;
  logic              i_tvalid;
  logic              i_tready;

  logic [DATA_W-1:0] o_tdata;
  logic              o_tlast;
  logic              o_tvalid;
  logic              o_tready;

  axis_pkt_throttle #(
    .THROTTLE_W(THROTTLE_W),
    .DATA_W    (DATA_W),
    .MTU       (MTU)
  ) axis_pkt_throttle_dut (
    .clk     (clk     ),
    .rst     (rst     ),
    .throttle(throttle),
    .i_tdata (i_tdata ),
    .i_tlast (i_tlast ),
    .i_tvalid(i_tvalid),
    .i_tready(i_tready),
    .o_tdata (o_tdata ),
    .o_tlast (o_tlast ),
    .o_tvalid(o_tvalid),
    .o_tready(o_tready)
  );

  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // Run a test using the following parameters.
  //
  //   num_pkts          : Number of packets to generate, each with a random
  //                       length.
  //   input_stall_prob  : Probability of a stall on the input, a whole number
  //                       from 0 to 99.
  //   output_stall_prob : Probability of a stall on the output, a whole number
  //                       from 0 to 99.
  //   rate              : Floating point value in the range (0, 1.0].
  //   min_pkt_length    : Minimum packet length to generate.
  //
  task automatic run_test(
    int  num_pkts,
    real rate              = 1.0,
    int  input_stall_prob  = 0,
    int  output_stall_prob = 0,
    int  min_pkt_length    = 1
  );
    real actual_rate;
    test.start_test(
      $sformatf({ "num_pkts=%0d, rate=%0.3f, input_stall_prob=%0d, ",
        "output_stall_prob=%0d, min_pkt_length=%0d"},
        num_pkts, rate, input_stall_prob, output_stall_prob)
    );
    throttle = (1.0/rate-1.0) * 2.0**FRAC_W;
    actual_rate = 1.0/(real'(throttle)/(2.0**FRAC_W) + 1.0);
    $display("Setting throttle to 0x%X (rate = %0.3f)", throttle, actual_rate);
    i_tdata  <= 'X;
    i_tlast  <= 'X;
    i_tvalid <= 0;
    o_tready <= 0;
    @(posedge clk);

    fork

      // The writer generates random packets to input to the DUT
      begin : writer
        logic [DATA_W-1:0] data = 0;
        for (int pkt_count = 0; pkt_count < num_pkts; pkt_count++) begin
          int pkt_length = $urandom_range(min_pkt_length, MAX_PKT_LEN);
          for (int word_count = 0; word_count < pkt_length; word_count++) begin
            // Write the next word
            i_tdata  <= data;
            i_tlast  <= (word_count == pkt_length-1);
            i_tvalid <= 1;
            do @(posedge clk); while (!(i_tvalid && i_tready));
            data = data + 1;
            // Randomly stall between words
            if ($urandom_range(99) < input_stall_prob) begin
              i_tdata <= 'X;
              i_tlast <= 'X;
              i_tvalid <= 0;
              do @(posedge clk); while ($urandom_range(99) < input_stall_prob);
            end
          end
        end
      end : writer

      // The data_checker verifies that the data output is correct and handles
      // random stalling of the output stream.
      begin : data_checker
        logic [DATA_W-1:0] data = 0;
        for (int pkt_count = 0; pkt_count < num_pkts; pkt_count++) begin
          int word_count = 0;
          forever begin
            @(posedge clk);
            if (o_tvalid && o_tready) begin
              `ASSERT_ERROR(
                o_tdata == data,
                $sformatf({
                  "Data didn't match expected on packet %0d word offset %0d. ",
                  "Expected %X, read %X"},
                  pkt_count, word_count, data, o_tdata
                )
              )
              data++;
              if (i_tlast) break;
              word_count++;
            end
            // Randomly stall this cycle
            o_tready <= ($urandom_range(99) >= output_stall_prob);
          end
        end
      end : data_checker

      // The throttle_checker measures the packet rate and confirms that it
      // matches the configured rate.
      begin : throttle_checker
        bit      sop = 0;      // Start of packet indicator
        realtime sop_time;     // Time at which the packet started
        int      pkt_length;   // Counter to measure packet length (number of transfers)
        int      pkt_duration; // Counter to measure packet duration (from start to tlast)

        // Wait for the start of the first packet
        forever begin
          @(posedge clk);
          if (o_tvalid && o_tready) begin
            sop_time = $realtime;
            sop = o_tlast;
            break;
          end
        end

        // Iterate through packets
        pkt_length = 1;
        pkt_duration = 1;
        for (int pkt_count = 0; pkt_count < num_pkts; pkt_count++) begin : pkt_loop
          int exp_pkt_cycles;

          forever begin : cycle_loop
            @(posedge clk);
            if (!sop) pkt_duration++;
            if (o_tvalid && o_tready) begin : transfer_cycle
              // Calculate the minimum allowed time between packets assuming
              // continuous data.
              exp_pkt_cycles = int'(real'(pkt_length) / actual_rate);

              // Check if this is the first transfer of a packet. If so, verify
              // that the duration of the previous packet was not shorter than
              // the configured rate would allow.
              if (sop) begin : first_transfer
                int pkt_cycles;

                // Calculate the actual time between packets.
                pkt_cycles = ($realtime - sop_time) / CLK_PERIOD;

                if (input_stall_prob == 0 && output_stall_prob == 0) begin
                  // If there are no stalls, the actual time should exactly
                  // match the expected, or one less due to rounding.
                  `ASSERT_ERROR(
                    pkt_cycles == exp_pkt_cycles || pkt_cycles == exp_pkt_cycles-1,
                    $sformatf({
                      "Time for packet %0d did not match expected range.\n",
                      "  Actual Rate:     %f\n",
                      "  Packet Length:   %0d\n",
                      "  Packet Cycles:   %0d\n",
                      "  Expected Cycles: %0d"},
                      pkt_count, actual_rate, pkt_length, pkt_cycles, exp_pkt_cycles
                    )
                  )
                end else begin
                  // If there are stalls, the actual length should never be
                  // less than the min expected, but could be substantially
                  // more.
                  `ASSERT_ERROR(
                    pkt_cycles >= exp_pkt_cycles-1,
                    $sformatf({
                      "Time for packet %0d was less than expected.\n",
                      "  Actual Rate:     %f\n",
                      "  Packet Length:   %0d\n",
                      "  Packet Cycles:   %0d\n",
                      "  Expected Cycles: %0d"},
                      pkt_count, actual_rate, pkt_length, pkt_cycles, exp_pkt_cycles
                    )
                  )
                end

                // Setup measurement of the packet we just started
                sop = o_tlast;
                pkt_length = 1;
                pkt_duration = 1;
                sop_time = $realtime;
                break;
              end : first_transfer
              else begin : subsequent_transfer
                pkt_length++;
              end : subsequent_transfer

              sop = o_tlast;
            end : transfer_cycle
            else begin : idle_cycle
              // Calculate the minimum allowed time between packets assuming
              // continuous data.
              int exp_pkt_cycles = int'(real'(pkt_length) / actual_rate);

              // If the packet duration was longer than the expected packet
              // time, then we should NOT be gating packet flow.
              if (pkt_duration > exp_pkt_cycles) begin
                `ASSERT_ERROR(
                  axis_pkt_throttle_dut.gate == 0,
                  "The throttle gate engaged when it should not have."
                )
              end
            end

            // If we're on the last packet, then there won't be another start
            // of packet to measure against, so there's nothing left to check.
            if (sop && (pkt_count == num_pkts-1)) break;
          end : cycle_loop
        end : pkt_loop
      end : throttle_checker

    join
    test.end_test();
  endtask : run_test

  //---------------------------------------------------------------------------
  // Underflow Tracker
  //---------------------------------------------------------------------------

  int   uflow_rise_count = 0;
  int   uflow_fall_count = 0;
  logic uflow_prev = 0;

  always @(posedge clk) begin
    uflow_prev <= axis_pkt_throttle_dut.underflow;

    if (axis_pkt_throttle_dut.underflow && !uflow_prev) begin
      uflow_rise_count <= uflow_rise_count + 1;
    end

    if (!axis_pkt_throttle_dut.underflow && uflow_prev) begin
      uflow_fall_count <= uflow_fall_count + 1;
    end
  end

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    tb_name = $sformatf("axis_pkt_throttle");
    test.start_tb(tb_name, 20ms);

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset", 1ms);
    clk_gen.reset();
    if (rst) @rst;
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    begin
      // List various rate settings and handshake stall behaviors to test
      automatic real rates[] = '{
        0.10,
        0.25,
        0.50,
        0.75,
        0.90,
        1.00
      };
      automatic real stall_probs[] = '{
        0,
        25,
        50,
        75
      };

      // Test each permutation of rates and stall probabilities
      foreach (rates[i]) begin
        foreach (stall_probs[j]) begin
          foreach (stall_probs[k]) begin
            run_test(NUM_PACKETS, rates[i], stall_probs[j], stall_probs[k]);
          end
        end
      end
    end

    begin
      // Test really slow packets to make sure the internal counters handle
      // underflow correctly.
      localparam int NUM_UFLOW_PACKETS = 4;
      run_test(NUM_UFLOW_PACKETS, 0.1, 99, 99, MAX_PKT_LEN/2);
      // Follow up with regular packets
      run_test(2);
      `ASSERT_ERROR(
        uflow_rise_count == uflow_fall_count && uflow_rise_count == NUM_UFLOW_PACKETS,
        "Underflow did not occur as expected."
      )
    end

    //--------------------------------
    // Finish Up
    //--------------------------------

    test.end_tb();

  end : tb_main

endmodule : axis_pkt_throttle_tb


`default_nettype wire
