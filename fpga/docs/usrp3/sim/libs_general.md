# General Purpose Libraries

## Execution and Reporting (sim\_exec\_report.vh)

Macros to do boilerplate testbench initialization and utilities to define test cases

#### TEST\_BENCH\_INIT

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
    //

#### TEST\_CASE\_START

    // Indicates the start of a test case
    // This macro *must be* called inside the primary initial block
    //
    // Usage: `TEST_CASE_START(test_name)
    // where
    //  - test_name:        The name of the test.
    //

#### TEST\_CASE\_DONE
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

#### ASSERT\_FATAL

    // Wrapper around a an assert.
    // ASSERT_FATAL throws an error assertion and halts the simulator
    // if cond is not satisfied
    //
    // Usage: `ASSERT_FATAL(cond,msg)
    // where
    //  - cond: Condition for the assert
    //  - msg:  Message for the assert
    //

#### ASSERT\_ERROR

    // Wrapper around a an assert.
    // ASSERT_ERROR throws an error assertion and fails the test case
    // if cond is not satisfied. The simulator will *not* halt
    //
    // Usage: `ASSERT_ERROR(cond,msg)
    // where
    //  - cond: Condition for the assert
    //  - msg:  Message for the assert
    //

#### ASSERT\_WARNING

    // Wrapper around a an assert.
    // ASSERT_WARNING throws an warning assertion but does not fail the
    // test case if cond is not satisfied. The simulator will *not* halt
    //
    // Usage: `ASSERT_WARNING(cond,msg)
    // where
    //  - cond: Condition for the assert
    //  - msg:  Message for the assert
    //

## Clocks and Resets (sim\_clks\_rsts.vh)

Shortcut macros to create typical clock and reset signals.

#### DEFINE\_CLK

    // Generates a persistent clock that starts at t=0 and runs forever
    //
    // Usage: `DEFINE_CLK(clk_name,period,duty_cycle)
    // where
    //  - clk_name:   The clock net to be generated
    //  - period:     Period of the clock in simulator ticks
    //  - duty_cycle: Percentage duty cycle
    //

#### DEFINE\_LATE\_START\_CLK

    // Generates a clock that starts at the specified time and runs forever
    //
    // Usage: `DEFINE_LATE_START_CLK(clk_name,period,duty_cycle,start_time,start_time_res)
    // where
    //  - clk_name:   The clock net to be generated
    //  - period:     Period of the clock in simulator ticks
    //  - duty_cycle: Percentage duty cycle
    //  - start_time: Start time for clock in simulator ticks
    //  - start_time_res: Start time resolution (must be > timescale increment and < start_time)
    //

#### DEFINE_RESET

    // Generates an active high reset
    //
    // Usage: `DEFINE_RESET(reset_name,reset_time,reset_duration)
    // where
    //  - reset_name:     The reset net to be generated
    //  - reset_time:     Time at which reset will be asserted (i.e. rst=1)
    //  - reset_duration: Duration of reset assertion
    //

#### DEFINE_RESET_N

    // Generates an active low reset
    //
    // Usage: `DEFINE_RESET_N(reset_name,reset_time,reset_duration)
    // where
    //  - reset_name:     The reset net to be generated
    //  - reset_time:     Time at which reset will be asserted (i.e. rst=0)
    //  - reset_duration: Duration of reset assertion
    //

## File I/O (sim\_file\_io.sv)

### interface data\_file\_t

Defines a ``data_file_t`` interface with the following functions:

#### ctor

    // Create a handle to a data_file with
    // - FILENAME: Name of the file
    // - FORMAT: Data format (HEX, DEC, OCT, BIN, FLOAT)
    // - DWIDTH: Width of each element stored in the file (one line per word)
    //

#### open

    // Open the data file for reading or writing.
    //
    // Usage: open(mode)
    // where
    //  - mode: RW mode (Choose from: READ, WRITE, APPEND)
    //

#### close

    // Close an open data file. No-op if file isn't already open
    //
    // Usage: close()
    //

#### is_eof

    // Is end-of-file reached.
    //
    // Usage: is_eof() Returns eof
    // where
    // - eof: A boolean
    //

#### readline

    // Read a line from the datafile
    //
    // Usage: readline() Returns data
    // where
    // - data: A logic array of width DWIDTH containing the read word
    //

#### writeline

    // Write a line to the datafile
    //
    // Usage: writeline(data) 
    // where
    // - data: A logic array of width DWIDTH to write to the file
    //