//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgTestExec
//
// Description: This package provides infrastructure for tracking the state of
// testbench execution and the results of each test.
//



package PkgTestExec;

  timeunit 1ns;
  timeprecision 1ps;

  typedef enum { SEV_INFO, SEV_WARNING, SEV_ERROR, SEV_FATAL } severity_t;

  typedef std::process timeout_t;

  class TestExec;

    string tb_name;                   // Name of the testbench
    string current_test;              // Name of the current test being run
    int    num_started, num_finished; // Number of tests started and finished
    int    num_passed;                // Number of tests that have passed
    int    num_assertions;            // Number of assertions checked for the current test
    time   start_time, end_time;      // Start and end time of the testbench
    bit    stop_on_error = 1;         // Configuration option to stop when an error occurs
    bit    done = 0;                  // Flag that sets when tb is finished
    bit    test_status[$];            // Pass/fail status of each test

    timeout_t tb_timeout;             // Handle to timeout for the overall testbench
    timeout_t test_timeout;           // Handle to timeout for current test

    semaphore test_sem;               // Testbench semaphore


    function new();
      $timeformat(-9, 0, " ns", 12);
      this.test_sem = new(1);
    endfunction : new


    // Call start_tb() at the start of a testbench to initialize state tracking
    // properties.
    //
    //    time_limit:  Max simulation time allowed before at timeout occurs.
    //                 This is a catch-all, in case things go very wrong during
    //                 simulation and cause it to never end.
    //
    task start_tb(string tb_name, realtime time_limit = 10ms);
      // Get the sempahore, to prevent multiple overlapping instances of the
      // same testbench.
      test_sem.get();
      done = 0;
      $display("========================================================");
      $display("TESTBENCH STARTED: %s", tb_name);
      $display("========================================================");
      this.tb_name = tb_name;
      start_time   = $time;
      test_status  = {};
      num_started  = 0;
      num_finished = 0;
      num_passed   = 0;
      start_timeout(
        tb_timeout,
        time_limit,
        $sformatf("Testbench \"%s\" time limit exceeded", tb_name),
        SEV_FATAL
      );
    endtask : start_tb


    // Call end_tb() at the end of a testbench to report final statistics and,
    // optionally, end simulation.
    //
    //    finish:  Set to 1 (default) to cause $finish() to be called at the
    //             end of simulation, cuasing the simulator to close.
    //
    function void end_tb(bit finish = 1);
      assert (num_started == num_finished) else begin
        $fatal(1, "Testbench ended before test completed");
      end
      end_time = $time;
      $display("========================================================");
      $display("TESTBENCH FINISHED: %s", tb_name);
      $display(" - Time elapsed:  %0t", end_time - start_time);
      $display(" - Tests Run:     %0d", num_finished);
      $display(" - Tests Passed:  %0d", num_passed);
      $display(" - Tests Failed:  %0d", num_started - num_passed);
      $display("Result: %s",
        (num_started == num_passed) && (num_finished > 0)
        ? "PASSED   " : "FAILED!!!");
      $display("========================================================");

      end_timeout(tb_timeout);

      done = 1;
      if (finish) $finish();

      // Release the semaphore to allow new instances of the testbench to run
      test_sem.put();
    endfunction : end_tb


    // Call at the start of each test with the name of the test.
    //
    //   test_name:  String name for the test to be started
    //
    task start_test(string test_name, realtime time_limit = 0);
      // Make sure a there isn't already a test running
      assert (num_started == num_finished) else begin
        $fatal(1, "Test started before completing previous test");
      end

      // Create a timeout for this test
      if (time_limit > 0) begin
        start_timeout(
          test_timeout,
          time_limit,
          $sformatf("Test \"%s\" time limit exceeded", test_name),
          SEV_FATAL
        );
      end

      current_test = test_name;
      num_started++;
      $display("[TEST CASE %3d] (t = %t) BEGIN: %s...", num_started, $time, test_name);
      test_status.push_back(1);   // Set status to 1 (passed) initially
      num_assertions = 0;
    endtask : start_test


    // Call end_test() at the end of each test.
    //
    //   test_result:  Optional value to indicate the overall pass/fail result
    //                 of the test. Use non-zero for pass, 0 for fail.
    //
    task end_test(int test_result = 1);
      bit passed;

      assert (num_started == num_finished + 1) else begin
        $fatal(1, "No test running");
      end

      end_timeout(test_timeout);

      passed = test_status[num_started-1] && test_result;
      num_finished++;
      $display("[TEST CASE %3d] (t = %t) DONE... %s",
               num_started, $time, passed ? "Passed" : "FAILED");

      if (passed) num_passed++;

      current_test = "";
    endtask : end_test


    // Assert the given expression and call $error() if it fails. Simulation
    // will also be stopped (using $stop) if stop_on_error is true.
    //
    //   expr:     The expression value to be asserted
    //   message:  String to report if the assertion fails
    //
    function void assert_error(int expr, string message = "");
      num_assertions++;
      assert (expr) else begin
        test_status[num_started] = 0;
        $error(message);
        if (stop_on_error) $stop;
      end
    endfunction : assert_error


    // Assert the given expression and call $fatal() if it fails.
    //
    //   expr:     The expression value to be asserted
    //   message:  String to report if the assertion fails
    //
    function void assert_fatal(int expr, string message = "");
      num_assertions++;
      assert (expr) else begin
        test_status[num_started] = 0;
        $fatal(1, message);
      end
    endfunction : assert_fatal


    // Assert the given expression and call $warning() if it fails.
    //
    //   expr:     The expression value to be asserted
    //   message:  String to report if the assertion fails
    //
    function void assert_warning(int expr, string message = "");
      num_assertions++;
      assert (expr) else begin
        $warning(message);
      end
    endfunction : assert_warning


    // Assert the given expression and call the appropriate severity or fatal
    // task if the expression fails.
    //
    //   expr:      The expression value to be asserted
    //   message:   String to report if the assertion fails
    //   severity:  Indicates the type of severity task that should be used if
    //              the assertion fails ($info, $warning, $error, $fatal).
    //              Default value is SEV_ERROR.
    //
    function void assert_sev(
      int        expr,
      string     message = "",
      severity_t severity = SEV_ERROR
    );
      num_assertions++;
      assert (expr) else begin
        case (severity)
          SEV_INFO: begin
            $info(message);
          end
          SEV_WARNING: begin
            $warning(message);
          end
          SEV_ERROR: begin
            $error(message);
            test_status[num_started] = 0;
            if (stop_on_error) $stop;
          end
          default: begin  // SEV_FATAL
            test_status[num_started] = 0;
            $fatal(1, message);
          end
        endcase
      end
    endfunction : assert_sev


    // Create a timeout that will expire after the indicated delay, causing an
    // error if the timeout is not canceled before the delay has elapsed.
    //
    //   handle:         A handle to the timeout process created. Use this
    //                   handle to end the timeout.
    //   timeout_delay:  Amount of time to wait before the timeout expires.
    //   message:        String message to display if the timeout expires.
    //   severity:       Indicates the type of severity task that should be
    //                   used if the timeout expires. Default is SEV_ERROR.
    //
    task start_timeout(
      output timeout_t  handle,
      input  realtime   timeout_delay,
      input  string     message = "Timeout",
      input  severity_t severity = SEV_ERROR
    );
      mailbox #(std::process) pid = new(1);
      fork
        begin : timeout
          pid.put(process::self());
          #(timeout_delay);
          assert_sev(0, {"Timeout: ", message}, severity);
        end
      join_none
      #0;   // Start the timeout process so we can get its process ID

      // Return the process ID
      pid.get(handle);
    endtask


    // Cancel the timeout with the given handle. This kills the process
    // running the timeout delay.
    //
    //   handle:  Handle created by start_timeout() for the timeout process.
    //
    function void end_timeout(timeout_t handle);
      if (handle != null)
        if (handle.status != process::FINISHED) handle.kill();
    endfunction;

  endclass : TestExec


  //---------------------------------------------------------------------------
  // Test Object
  //---------------------------------------------------------------------------
  //
  // This is the default TestExec object instance shared by all instances of
  // the running the testbench.
  //
  //---------------------------------------------------------------------------

  TestExec test = new();

endpackage : PkgTestExec

