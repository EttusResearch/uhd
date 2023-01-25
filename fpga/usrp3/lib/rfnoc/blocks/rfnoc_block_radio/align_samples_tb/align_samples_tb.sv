//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: align_samples_tb
//
// Description:
//
//   Testbench for align_samples.
//

`default_nettype none

module align_samples_tb #(

  parameter int SAMP_W    = 8,
  parameter int SPC       = 4,
  parameter int USER_W    = 8,
  parameter bit PIPE_IN   = 1,
  parameter bit PIPE_OUT  = 1

  ) ();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;
  import PkgRandom::*;

  localparam real CLK_PERIOD = 10.0;

  localparam bit USE_RANDOM = 1;    // Use random vs. sequential data.

  localparam int DATA_W  = SPC*SAMP_W;
  localparam int SHIFT_W = $clog2(DATA_W/SAMP_W);
  localparam int NUM_TESTS = 1000;

  typedef logic [SPC-1:0][SAMP_W-1:0] data_t;

  // Store data and validity of checker
  typedef struct packed {
    logic              valid;
    logic [DATA_W-1:0] data;
    logic [USER_W-1:0] user;
  } v_data_t;

  typedef v_data_t queue_t [$];

  // Bit to indicate done sending tests
  bit done      = 0;

  // Total number of words input and tested
  int in_count  = 0;
  int out_count = 0;

  // Indicate first word of a test
  bit first     = 0;
  bit first_reg = 0;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------
  bit clk;
  bit rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  // Input data signals
  data_t              i_data;
  logic [ USER_W-1:0] i_user;
  bit   [SHIFT_W-1:0] i_shift;
  bit                 i_dir;
  bit                 i_push;
  bit                 i_cfg_en;

  // Output data signals
  data_t             o_data;
  logic [USER_W-1:0] o_user;

  align_samples #(

    .SAMP_W  (SAMP_W  ),
    .SPC     (SPC     ),
    .USER_W  (USER_W  ),
    .PIPE_IN (PIPE_IN ),
    .PIPE_OUT(PIPE_OUT)

  ) align_samples_dut (

    .clk     (clk     ),
    .i_data  (i_data  ),
    .i_user  (i_user  ),
    .i_push  (i_push  ),
    .i_dir   (i_dir   ),
    .i_shift (i_shift ),
    .i_cfg_en(i_cfg_en),
    .o_data  (o_data  ),
    .o_user  (o_user  )

  );

  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // Send test data
  task automatic test_data(bit dir, bit [SHIFT_W-1:0] shift, int num_words);
    // Counts to assign sequential data
    int data_count = 0;
    int user_count = 0;

    // Sync with rising clock edge
    clk_gen.clk_wait_r();

    // Set shift and direction only on first rising edge of clk
    i_shift  <= shift;
    i_dir    <= dir;
    i_cfg_en <= 1;

    clk_gen.clk_wait_r();

    // Shift and direction will not be read after first rising edge of clk
    i_shift  <= 1'bX;
    i_dir    <= 'X;
    i_cfg_en <= 0;

    // Send number of words based on argument
    for (int word_cnt=0; word_cnt < num_words;) begin
      // 50% probability of changing push
      if ($urandom_range(0,1)) begin
        i_push <= 1;
        first  <= (word_cnt == 0); // Indicate first word of test
        word_cnt++; // Increment word_cnt only when i_push is asserted to indicate a word was sent
        in_count++;

        // Assign random data
        if (USE_RANDOM) begin
          i_data <= Rand#(DATA_W)::rand_logic();
          i_user <= Rand#(USER_W)::rand_bit();
        end else begin
          // Give the data an incrementing count
          i_user <= user_count++;
          for(int i=0; i < SPC; i++) begin
            i_data[i] <= data_count++;
          end
        end

      end else begin
        i_push <= 0;
        i_data <= 'X;
        i_user <= 'X;
      end

      clk_gen.clk_wait_r();

      // Reset data values at end of test
      i_push <= 0;
      i_data <= 'X;
      i_user <= 'X;

    end

  endtask: test_data

  // Generate test data with random shifting configuration
  task automatic run_gen();

    bit               dir       = 0; // Random direction
    bit [SHIFT_W-1:0] shift     = 0; // Random shift in range 0:SPC-1
    int               num_words = 0; // Random number of words to send per test

    in_count = 0; // Reset input word count

    repeat(NUM_TESTS) begin
      dir       = $urandom_range(0, 1);
      shift     = $urandom_range(0, SPC-1);
      num_words = $urandom_range(1, 5);

      test_data(dir, shift, num_words);
    end

    // Indicate done generating tests
    done = 1;
    $display("Input    %0d words", in_count);

  endtask : run_gen

  // Check valid output data against expected data
  task automatic run_check();

    // Queue to hold expected data for checker
    queue_t data_q;

    // Config signals
    bit               dir   = 0;
    bit [SHIFT_W-1:0] shift = 0;

    // Data comparison signals
    v_data_t           val_data;          // Struct to hold valid, data and user for queue
    v_data_t           queue_data;        // Struct to hold valid, data and user for queue
    data_t             exp_data     = 'X; // Expected data to compare to output data
    logic [USER_W-1:0] exp_user     = 'X; // Expected user data to compare to output user data
    data_t             i_prev_data  = 'X; // Carry previous input data
    data_t             o_prev_data  = 'X; // Carry previous output data
    logic [USER_W-1:0] i_prev_user  = 'X; // Carry previous output user data
    logic [USER_W-1:0] o_prev_user  = 'X; // Carry previous output user data

    // Latency
    int push_cnt = 0;                     // Count number of times i_push is asserted
    int val_hold = 0;                     // Hold data as invalid for corner cases
    int latency  = 0;
    out_count = 0; // Reset checked word count

    // Latency based on input and output pipeline parameters
    // Add extra cycle of latency for right shift
    latency = (PIPE_IN && PIPE_OUT) ? 2 :
              (PIPE_IN  ^ PIPE_OUT) ? 1 :
                                      0;

    // Give initial values to queue
    val_data.data  = 'X;
    val_data.user  = 'X;
    val_data.valid = 1'b0;

    // If additional latency from pipeline, queue additional data
    if (latency == 2) begin
      data_q.push_back(val_data);
    end

    while (!done) begin  // Ideally check until last value
      // Compare output data to expected data only if expected data is valid
      if (val_data.valid) begin
        data_t             act_data;  // Actual data output
        logic [USER_W-1:0] act_user;  // Actual user output
        out_count++;

        // If no pipeline delay, compare expected data with output data from
        // previous clock cycle.
        if (latency == 0) begin
          act_data = o_prev_data;
          act_user = o_prev_user;
        end else begin
          act_data = o_data;
          act_user = o_user;
        end

        if (dir == 0 && first_reg == 1 && first == 1) begin
          // If first word is left shifted, only check data shifted to account
          // for ModelSim/Vivado behavior.
          for (int val_check = SPC-1; val_check > shift; val_check--) begin
            `ASSERT_ERROR(exp_data[val_check] === act_data[val_check],
              $sformatf({"Output data sample %0d not equal to expected. ",
              "Expected 0x%X Read 0x%X"},
              val_check, exp_data[val_check], act_data[val_check]));
          end
        end else begin
          `ASSERT_ERROR(exp_data === act_data,
            $sformatf({"Output data not equal to expected data. ",
            "Expected 0x%X Read 0x%X"}, exp_data, act_data));
        end
        `ASSERT_ERROR(exp_user === act_user,
          $sformatf({"Output user data not equal to expected user data. ",
          "Expected 0x%X Read 0x%X"}, exp_user, act_user));
      end

      // Check input if config changed and set expected data to invalid
      if (i_cfg_en) begin
        // Check corner cases
        if (i_dir == 0 && dir == 1) begin                // Right to Left
          val_hold = 1;
        end else if (!i_dir && !dir) begin               // Left to Left
          if (i_shift == 1 && shift == 3) begin          // Left 3 to Left 1
            val_hold = 1;
          end else if (i_shift == 1 && shift == 2) begin // Left 2 to Left 1
            val_hold = 1;
          end else if (i_shift == 2 && shift == 1) begin // Left 1 to Left 2
            val_hold = 1;
          end
        end else if (i_dir && dir) begin                 // Right to Right
          if (i_shift == 2 && shift == 3) begin          // Right 3 to Right 2
            val_hold =  1;
          end
        end else begin
          val_hold = 0;
        end

        // If no pipeline delay, config will not disable expected data
        if (latency == 0) begin
          if (i_dir && dir) begin                        // Right to Right
            if (i_shift == 2 && shift == 3) begin        // Right 3 to Right 2
              val_data.valid = 1'b0;
            end
          end else if (i_dir) begin
            val_data.valid = 1'b0;
          end
        end else begin
          val_data.valid = 1'b0;
        end

        // Update config signals
        shift          = i_shift;
        dir            = i_dir;
        push_cnt       = 0;

      end

      // Check input if push was asserted
      if (i_push) begin

        // Calculate new expected data and user
        exp_data = (shift == 0) ? i_data :
                   (dir   == 1) ? (i_prev_data >> (shift*SAMP_W)) | (i_data      << ((SPC-shift) * SAMP_W)) :
                                  (i_data      << (shift*SAMP_W)) | (i_prev_data >> ((SPC-shift) * SAMP_W));
        val_data.data = exp_data;

        exp_user      = i_user;
        val_data.user = exp_user;

        // Update validity of checker
        if (latency == 0) begin
          val_data.valid = push_cnt >= (val_hold + dir) ? 1'b1 : 1'b0;
        end else begin
          val_data.valid = (push_cnt >= val_hold + latency + dir - 1) ? 1'b1 : 1'b0;
        end
        // Store data to carry

        i_prev_data = i_data;
        o_prev_data = o_data;
        first_reg   = first;

        i_prev_user = i_user;
        o_prev_user = o_user;

        // Push new expected data to queue
        data_q.push_back(val_data);

        // Pop next expected value off queue
        queue_data = data_q.pop_front();
        exp_data   = queue_data.data;
        exp_user   = queue_data.user;

        push_cnt++;
      end

      clk_gen.clk_wait_r();

    end

    $display("Verified %0d words on the output", out_count);

  endtask : run_check

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;
    tb_name = $sformatf( {

      "align_samples_tb\n",
      "SAMP_W   = %03d\n",
      "SPC      = %03d\n",
      "USER_W   = %03d\n",
      "PIPE_IN  = %03d\n",
      "PIPE_OUT = %03d"},
      SAMP_W, SPC, USER_W, PIPE_IN, PIPE_OUT

    );
    test.start_tb(tb_name, 100ms);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    clk_gen.start();

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

    test.start_test("Generate and check data", 1ms);

    fork
      run_gen();
      run_check();
    join

    test.end_test();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();

  end : tb_main

endmodule : align_samples_tb

`default_nettype wire
