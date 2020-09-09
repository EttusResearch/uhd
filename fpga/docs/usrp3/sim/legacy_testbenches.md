# Legacy Testbenches

Some existing testbenches use the following testbench style and infrastructure.
This documentation is from UHD 3.x but is included here due to its continued
use.

Below is a sample legacy SystemVerilog testbench.

    //
    // Copyright 2015 Ettus Research LLC
    //
    
    `timescale 1ns/1ps
    `define NS_PER_TICK     1
    `define NUM_TEST_CASES  3
    
    `include "sim_clks_rsts.vh"
    `include "sim_exec_report.vh"
    `include "sim_cvita_lib.sv"
    
    module example_fifo_tb();
      `TEST_BENCH_INIT("example_fifo_tb",`NUM_TEST_CASES,`NS_PER_TICK)
    
      // Define all clocks and resets
      `DEFINE_CLK(bus_clk, 1000/166.6667, 50) //166MHz bus_clk
      `DEFINE_RESET(bus_rst, 0, 100)          //100ns for GSR to deassert
    
      cvita_stream_t chdr_i (.clk(bus_clk));
      cvita_stream_t chdr_o (.clk(bus_clk));
    
      // Initialize DUT
      axi_fifo #(.WIDTH(65), .SIZE(24)) dut_single (
        .clk(bus_clk),
        .reset(bus_rst),
        .clear(1'b0),
        
        .i_tdata({chdr_i.axis.tlast, chdr_i.axis.tdata}),
        .i_tvalid(chdr_i.axis.tvalid),
        .i_tready(chdr_i.axis.tready),
      
        .o_tdata({chdr_o.axis.tlast, chdr_o.axis.tdata}),
        .o_tvalid(chdr_o.axis.tvalid),
        .o_tready(chdr_o.axis.tready),
        
        .space(),
        .occupied()
      );
    
      //Testbench variables
      cvita_hdr_t   header, header_out;
      cvita_stats_t stats;

      //------------------------------------------
      //Main thread for testbench execution
      //------------------------------------------
      initial begin : tb_main
    
        `TEST_CASE_START("Wait for reset");
        while (bus_rst) @(posedge bus_clk);
        `TEST_CASE_DONE((~bus_rst));
        
        repeat (200) @(posedge bus_clk);
    
        header = '{
          pkt_type:DATA, has_time:0, eob:0, seqno:12'h666,
          length:0, sid:$random, timestamp:64'h0};
    
        `TEST_CASE_START("Fill up empty FIFO then drain (short packet)");
          chdr_o.axis.tready = 0;
          chdr_i.push_ramp_pkt(16, 64'd0, 64'h100, header);
          chdr_o.axis.tready = 1;
          chdr_o.wait_for_pkt_get_info(header_out, stats);
          `ASSERT_ERROR(stats.count==16,            "Bad packet: Length mismatch");
          `ASSERT_ERROR(header.sid==header_out.sid, "Bad packet: Wrong SID");
          `ASSERT_ERROR(chdr_i.axis.tready,         "Bus not ready");
        `TEST_CASE_DONE(1);

        header = '{
          pkt_type:DATA, has_time:1, eob:0, seqno:12'h666, 
          length:0, sid:$random, timestamp:64'h0};
    
        `TEST_CASE_START("Concurrent read and write (single packet)");
          chdr_o.axis.tready = 1;
          fork
              begin
                chdr_i.push_ramp_pkt(20, 64'd0, 64'h100, header);
              end
              begin
                chdr_o.wait_for_pkt_get_info(header_out, stats);
              end
          join
        `ASSERT_ERROR(stats.count==20,      "Bad packet: Length mismatch");
        `TEST_CASE_DONE(1);
      end
    endmodule


Each testbench should have the following basic components:

## Timescale Defines and Includes

    `timescale 1ns/1ps
    `define NS_PER_TICK     1
    `define NUM_TEST_CASES  3
    
    `include "sim_clks_rsts.vh"
    `include "sim_exec_report.vh"
    `include "sim_cvita_lib.sv"

In addition to the timescale, the infrastructure needs to know the number of 
nanoseconds per simulator tick. This can be a floating point number.


In addition to the timescale, you may include any Verilog/SystemVerilog headers here.

## Main Module Definition

    `include "sim_exec_report.vh"

    module example_fifo_tb();
      `TEST_BENCH_INIT("example_fifo_tb",`NUM_TEST_CASES,`NS_PER_TICK)

      ...

      //------------------------------------------
      //Main thread for testbench execution
      //------------------------------------------
      initial begin : tb_main

        ...

      end
    endmodule

The name of the main module must match the ``SIM_TOP`` variable value in the Makefile.
To register this module with the framework, the ``TEST_BENCH_INIT`` macro must be called.
This macro is defined in ``<repo>/usrp3/sim/general/sim_exec_report.vh``.

``TEST_BENCH_INIT``:

    // Initializes state for a test bench.
    // This macro *must be* called within the testbench module but 
    // outside the primary initial block
    // Its sets up boilerplate code for:
    // - Logging to console
    // - Test execution tracking
    // - Gathering test results
    // - Bounding execution time based on the SIM_RUNTIME_US vdef
    //
    // Usage: `TEST_BENCH_INIT(test_name,min_tc_run_count,ns_per_tick)
    // where
    //  - tb_name:          Name of the testbench. (Only used during reporting)
    //  - min_tc_run_count: Number of test cases in testbench. (Used to detect stalls and inf-loops)
    //  - ns_per_tick:      The time_unit_base from the timescale declaration

The testbench must also have at least one initial block that consists tests cases (covered later). 
For the sake of convention it should be called ``tb_main``. *All test cases must live in ``tb_main``*. You may
have other initial block but they must not call macros from ``sim_exec_report.vh`` because the code
there is not thread-safe.

## Test Cases

A test case in this context is defined as an independent entity that validates an aspect of the DUT behavior
and which is independent from other test cases i.e. the result of one test case should ideally not affect others.


Test cases are wrapped in the ``TEST_CASE_START`` and ``TEST_CASE_DONE`` macros:

    `TEST_CASE_START("Fill up empty FIFO then drain (short packet)");
      chdr_o.axis.tready = 0;
      chdr_i.push_ramp_pkt(16, 64'd0, 64'h100, header);
      chdr_o.axis.tready = 1;
      chdr_o.wait_for_pkt_get_info(header_out, stats);
      `ASSERT_ERROR(stats.count==16,            "Bad packet: Length mismatch");
      `ASSERT_ERROR(header.sid==header_out.sid, "Bad packet: Wrong SID");
      `ASSERT_ERROR(chdr_i.axis.tready,         "Bus not ready");
    `TEST_CASE_DONE(1);

Here are the signatures of the two macros:

``TEST_CASE_START``:

    // Indicates the start of a test case
    // This macro *must be* called inside the primary initial block
    //
    // Usage: `TEST_CASE_START(test_name)
    // where
    //  - test_name:        The name of the test.
    //

``TEST_CASE_DONE``:

    // Indicates the end of a test case
    // This macro *must be* called inside the primary initial block
    // The pass/fail status of test case is determined based on the
    // the user specified outcome and the number of fatal or error
    // ASSERTs triggered in the test case.
    //
    // Usage: `TEST_CASE_DONE(test_result)
    // where
    //  - test_result:  User specified outcome
    //

In addition to the test case status, it is also possible to have asserts within
a test case. We have wrappers for the different kinds of SystemVerilog asserts
that additionally fail the test case in case the assert fails. An assert triggered
in a test case will not affect the outcome of another (except for a fatal assert which
halts the simulator). Supported assert macros:

    // Wrapper around a an assert.
    // ASSERT_FATAL throws an error assertion and halts the simulator
    // if cond is not satisfied
    //
    // Usage: `ASSERT_FATAL(cond,msg)
    // where
    //  - cond: Condition for the assert
    //  - msg:  Message for the assert
    //
    
    
    // Wrapper around a an assert.
    // ASSERT_ERROR throws an error assertion and fails the test case
    // if cond is not satisfied. The simulator will *not* halt
    //
    // Usage: `ASSERT_ERROR(cond,msg)
    // where
    //  - cond: Condition for the assert
    //  - msg:  Message for the assert
    //
    
    
    // Wrapper around a an assert.
    // ASSERT_WARNING throws an warning assertion but does not fail the
    // test case if cond is not satisfied. The simulator will *not* halt
    //
    // Usage: `ASSERT_WARNING(cond,msg)
    // where
    //  - cond: Condition for the assert
    //  - msg:  Message for the assert
    //
