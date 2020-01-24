//
// Copyright 2015 Ettus Research LLC
//

// Generates a persistent clock that starts at t=0 and runs forever
//
// Usage: `DEFINE_CLK(clk_name,period,duty_cycle)
// where
//  - clk_name:   The clock net to be generated
//  - period:     Period of the clock in simulator ticks
//  - duty_cycle: Percentage duty cycle
//
`define DEFINE_CLK(clk_name, period, duty_cycle) \
    reg clk_name = 0; \
    always begin \
      clk_name = 1; \
      #((1.0*period)*(1.0*duty_cycle/100)); \
      clk_name = 0; \
      #((1.000*period)*(1.0-(1.0*duty_cycle/100))); \
    end

// Generates a persistent clock that starts at t=0 and runs forever
//
// Usage: `DEFINE_CLK(clk_name,period,duty_cycle)
// where
//  - clk_name:   The clock net to be generated
//  - period:     Period of the clock in simulator ticks
//  - duty_cycle: Percentage duty cycle
//
`define DEFINE_DIFF_CLK(clk_name_p, clk_name_n, period, duty_cycle) \
    reg clk_name_p = 0; \
    reg clk_name_n = 1; \
    always begin \
      clk_name_p = 1; \
      clk_name_n = 0; \
      #((1.0*period)*(1.0*duty_cycle/100)); \
      clk_name_p = 0; \
      clk_name_n = 1; \
      #((1.000*period)*(1.0-(1.0*duty_cycle/100))); \
    end

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
`define DEFINE_LATE_START_CLK(clk_name, period, duty_cycle, start_time, start_time_res) \
    reg clk_name = 0; \
    reg clk_name``_locked = 0; \
    always begin \
      while (!clk_name``_locked) #start_time_res; \
      clk_name = 1; \
      #((1.0*period)*(1.0*duty_cycle/100)); \
      clk_name = 0; \
      #((1.000*period)*(1.0-(1.0*duty_cycle/100))); \
    end \
    initial begin \
        #(start_time) clk_name``_locked = 1; \
    end

// Generates an active high reset
//
// Usage: `DEFINE_RESET(reset_name,reset_time,reset_duration)
// where
//  - reset_name:     The reset net to be generated
//  - reset_time:     Time at which reset will be asserted (i.e. rst=1)
//  - reset_duration: Duration of reset assertion
//
`define DEFINE_RESET(reset_name, reset_time, reset_duration) \
    reg reset_name = (reset_time==0); \
    initial begin \
        #(reset_time) reset_name = 1; \
        #(reset_time+reset_duration) reset_name = 0; \
    end

// Generates an active low reset
//
// Usage: `DEFINE_RESET_N(reset_name,reset_time,reset_duration)
// where
//  - reset_name:     The reset net to be generated
//  - reset_time:     Time at which reset will be asserted (i.e. rst=0)
//  - reset_duration: Duration of reset assertion
//
`define DEFINE_RESET_N(reset_name, reset_time, reset_duration) \
    reg reset_name = (reset_time!=0); \
    initial begin \
        #(reset_time) reset_name = 0; \
        #(reset_time+reset_duration) reset_name = 1; \
    end
