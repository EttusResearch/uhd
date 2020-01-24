# Writing a Testbench

Writing a unit test or system level test is easy with the Vivado makefile infrastructure! 
Most of the overhead of building and running a testbench is handled by the build tools. 
Even recurring tasks like reporting and monitoring are implemented by framework libraries.

Each executable FPGA unit test must have the following components:

1. A Makefile
2. A Testbench top-level module

## Testbench Makefile

The Testbench Makefile tell the build tools what to build, where to build it, dependency information and runtime information.
The build infrastructure will handle the how-to part for each supported simulation tool.

Here is a sample Makefile (you are encouraged to use this as a starting point)

    #
    # Copyright 2015 Ettus Research LLC
    #
    
    #-------------------------------------------------
    # Top-of-Makefile
    #-------------------------------------------------
    # Define BASE_DIR to point to the "top" dir
    BASE_DIR = $(abspath ../../..)
    # Include viv_sim_preample after defining BASE_DIR
    include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak
    
    #-------------------------------------------------
    # Design Specific
    #-------------------------------------------------
    # Define part using PART_ID (<device>/<package>/<speedgrade>)
    ARCH = kintex7
    PART_ID = xc7k410t/ffg900/-2
    
    # Include makefiles and sources for the DUT and its dependencies
    include $(BASE_DIR)/../lib/fifo/Makefile.srcs
    include $(BASE_DIR)/../lib/axi/Makefile.srcs
    include $(BASE_DIR)/../lib/control/Makefile.srcs
    
    DESIGN_SRCS = $(abspath \
    $(FIFO_SRCS) \
    $(AXI_SRCS) \
    $(CONTROL_LIB_SRCS) \
    )
    
    #-------------------------------------------------
    # IP Specific
    #-------------------------------------------------
    # If simulation contains IP, define the IP_DIR and point
    # it to the base level IP directory
    IP_DIR = ../../ip
    
    # Include makefiles and sources for all IP components
    # *after* defining the IP_DIR
    include $(IP_DIR)/ddr3_32bit/Makefile.inc
    include $(IP_DIR)/axi_intercon_2x64_128/Makefile.inc
    include $(IP_DIR)/fifo_short_2clk/Makefile.inc
    include $(IP_DIR)/fifo_4k_2clk/Makefile.inc
    include $(IP_DIR)/axi4_bram_1kx64/Makefile.inc
    
    DESIGN_SRCS += $(abspath \
    $(IP_DDR3_32BIT_SRCS) \
    $(IP_AXI_INTERCON_2X64_128_SRCS) \
    $(IP_FIFO_4K_2CLK_SRCS) \
    $(IP_FIFO_SHORT_2CLK_SRCS) \
    $(IP_AXI4_BRAM_1KX64_SRCS) \
    )
    
    #-------------------------------------------------
    # Testbench Specific
    #-------------------------------------------------
    include $(BASE_DIR)/../sim/general/Makefile.srcs
    include $(BASE_DIR)/../sim/axi/Makefile.srcs
    
    # Define only one toplevel module
    SIM_TOP = dram_fifo_tb
    # Simulation runtime in microseconds
    SIM_RUNTIME_US = 80
    
    SIM_SRCS = \
    $(abspath dram_fifo_tb.sv) \
    $(abspath axis_dram_fifo_single.sv) \
    $(IP_DDR3_32BIT_SIM_OUTS) \
    $(SIM_GENERAL_SRCS) \
    $(SIM_AXI_SRCS)
    
    #-------------------------------------------------
    # Bottom-of-Makefile
    #-------------------------------------------------
    # Include all simulator specific makefiles here
    # Each should define a unique target to simulate
    # e.g. xsim, vsim, etc and a common "clean" target
    include $(BASE_DIR)/../tools/make/viv_simulator.mak

You will notice that the Makefile has 5 distinct sections.

### Section 1: Boilerplate

    #-------------------------------------------------
    # Top-of-Makefile
    #-------------------------------------------------
    # Define BASE_DIR to point to the "top" dir
    BASE_DIR = $(abspath ../../..)
    # Include viv_sim_preample after defining BASE_DIR
    include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

Before declaring any variables or using any recipes, the following must be done (in order):

- Define `BASE_DIR` to tell the build system where the `<repo>/usrp3/top` directory is relative to the
  current testbench directory.
- Include `viv_sim_preamble.mak` to initialize boilerplate variables and functions

### Section 2: Design Specific

    #-------------------------------------------------
    # Design Specific
    #-------------------------------------------------
    # Define part using PART_ID (<device>/<package>/<speedgrade>)
    ARCH = kintex7
    PART_ID = xc7k410t/ffg900/-2
    
    # Include makefiles and sources for the DUT and its dependencies
    include $(BASE_DIR)/../lib/fifo/Makefile.srcs
    include $(BASE_DIR)/../lib/axi/Makefile.srcs
    include $(BASE_DIR)/../lib/control/Makefile.srcs
    
    DESIGN_SRCS = $(abspath \
    $(FIFO_SRCS) \
    $(AXI_SRCS) \
    $(CONTROL_LIB_SRCS) \
    )

This section contains pointers to sources and other variables for the DUT to function. In the 
example above, we are including all sources from the lib/fifo, lib/axi, lib/control directories.

The following makefile variables are special and must be defined:

- `ARCH`: The architecture targeted for the simulation.
- `PART_ID`: The exact part targeted for the simulation. Format: `<device>/<package>/<speedgrade>` 
- `DESIGN_SRCS`: Space-separated paths to the DUT and all of its dependencies.

### Section 3: IP Specific

    #-------------------------------------------------
    # IP Specific
    #-------------------------------------------------
    # If simulation contains IP, define the IP_DIR and point
    # it to the base level IP directory
    IP_DIR = ../../ip
    
    # Include makefiles and sources for all IP components
    # *after* defining the IP_DIR
    include $(IP_DIR)/ddr3_32bit/Makefile.inc
    include $(IP_DIR)/axi_intercon_2x64_128/Makefile.inc
    include $(IP_DIR)/fifo_short_2clk/Makefile.inc
    include $(IP_DIR)/fifo_4k_2clk/Makefile.inc
    include $(IP_DIR)/axi4_bram_1kx64/Makefile.inc
    
    DESIGN_SRCS += $(abspath \
    $(IP_DDR3_32BIT_SRCS) \
    $(IP_AXI_INTERCON_2X64_128_SRCS) \
    $(IP_FIFO_4K_2CLK_SRCS) \
    $(IP_FIFO_SHORT_2CLK_SRCS) \
    $(IP_AXI4_BRAM_1KX64_SRCS) \
    )

If the DUT depends on any Xilinx IP then this section is required. It tell the tools
which IP cores need to be built in order to run the simulation. The IP specific Makefile
includes handle the "how" part of building IP. As long as the correct Mafefile is included
and the IP XCI sources are added to `DESIGN_SRCS`, the IP intermediates will be built correctly.

The `IP_DIR` variable must be defined to point to the base ip directory that contains XCI sources. 

### Section 4: Testbench Specific

    #-------------------------------------------------
    # Testbench Specific
    #-------------------------------------------------
    include $(BASE_DIR)/../sim/general/Makefile.srcs
    include $(BASE_DIR)/../sim/axi/Makefile.srcs
    
    # Define only one toplevel module
    SIM_TOP = dram_fifo_tb
    # Simulation runtime in microseconds
    SIM_RUNTIME_US = 80
    
    SIM_SRCS = \
    $(abspath dram_fifo_tb.sv) \
    $(abspath axis_dram_fifo_single.sv) \
    $(IP_DDR3_32BIT_SIM_OUTS) \
    $(SIM_GENERAL_SRCS) \
    $(SIM_AXI_SRCS)

This section contains all sources and parameters for the actual testbench. Any simulation
dependency makefiles can be included here.

The following variables must be defined:

- `SIM_TOP`: The toplevel module name for the simulation project
- `SIM_RUNTIME_US`: The maximum runtime of the simulation in microseconds. At this time $finish will be called to terminate the testbench.
- `SIM_SRCS`: This is similar to DESIGN_SRCS except that that should contain a path to `SIM_TOP` and all of its dependencies.

### Section 5: Tool Support

    #-------------------------------------------------
    # Bottom-of-Makefile
    #-------------------------------------------------
    # Include all simulator specific makefiles here
    # Each should define a unique target to simulate
    # e.g. xsim, vsim, etc and a common "clean" target
    include $(BASE_DIR)/../tools/make/viv_simulator.mak

Now that the Makefile knows all the basic information about the testbench, include tool-specific
makefiles to implement simulation targets. Currently the following simulator makefiles exits:

- ``<repo>/tools/make/viv_simulator.mak``

Please refer to the next section for more information about targets


## Testbench Top Level

The top-level module will instantiate the DUT and implement self-checking behavior. 
Test benches could be written in any language (SystemVerilog, Verilog, VHDL) but 
to take advantage of our repository of simulation libraries, it is recommended that SystemVerilog be used.

Here is a sample SystemVerilog top module (you are encouraged to use this as a starting point)

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

### Timescale Defines and Includes

    `timescale 1ns/1ps
    `define NS_PER_TICK     1
    `define NUM_TEST_CASES  3
    
    `include "sim_clks_rsts.vh"
    `include "sim_exec_report.vh"
    `include "sim_cvita_lib.sv"

In addition to the timescale, the infrastructure needs to know the number of 
nanoseconds per simulator tick. This can be a floating point number.


In addition to the timescale, you may include any Verilog/SystemVerilog headers here.

### Main Module Definition

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

### Test Cases

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

### Optional Libraries

It is encouraged to use (and create) reusable libraries in product specific 
test benches. Libraries can provide macros, modules, tasks and functions for
ease-of-use with particular protocols and subsystems.

The \ref md_usrp3_sim_writing_testbenches page has more information.
