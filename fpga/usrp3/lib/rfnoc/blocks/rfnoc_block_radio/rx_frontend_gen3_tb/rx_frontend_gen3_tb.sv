//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rx_frontend_gen3_tb
//
// Description: Testbench for rx_frontend_gen3.
//

`default_nettype none


module rx_frontend_gen3_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;

  localparam real CLK_FREQUENCY = 100e6;
  localparam real CLK_PERIOD_NS = 1.0e9 / CLK_FREQUENCY;
  localparam real PI            = 2.0 * $acos(0.0);

  // Register offsets
  localparam SR_MAG_CORRECTION     = 0;
  localparam SR_PHASE_CORRECTION   = 1;
  localparam SR_OFFSET_I           = 2;
  localparam SR_OFFSET_Q           = 3;
  localparam SR_IQ_MAPPING         = 4;
  localparam SR_HET_PHASE_INCR     = 5;

  // SR_IQ_MAPPING bit positions
  localparam SWAP_IQ     = 1 << 0;
  localparam REAL_MODE   = 1 << 1;
  localparam INVERT_Q    = 1 << 2;
  localparam INVERT_I    = 1 << 3;
  localparam DOWNCONVERT = 1 << 4;
  localparam BYPASS_ALL  = 1 << 7;

  // SR_OFFSET_* bit position
  localparam OFFSET_FIXED = 1 << 31;  // Fixed bit for SR_OFFSET register
  localparam OFFSET_SET   = 1 << 30;  // Set bit for SR_OFFSET register


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk, rst;
  sim_clock_gen #(.PERIOD(CLK_PERIOD_NS)) clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  localparam BYPASS_DC_OFFSET_CORR = 0;
  localparam BYPASS_IQ_COMP        = 0;
  localparam BYPASS_REALMODE_DSP   = 0;
  localparam DEVICE                = "7SERIES";

  logic               sync_in;
  logic               set_stb = 0;
  logic        [ 7:0] set_addr = 0;
  logic        [31:0] set_data = 0;
  logic               adc_stb;
  logic signed [15:0] adc_i = 0;
  logic signed [15:0] adc_q = 0;
  logic               rx_stb;
  logic signed [15:0] rx_i;
  logic signed [15:0] rx_q;

  rx_frontend_gen3 #(
    .SR_MAG_CORRECTION     (SR_MAG_CORRECTION),
    .SR_PHASE_CORRECTION   (SR_PHASE_CORRECTION),
    .SR_OFFSET_I           (SR_OFFSET_I),
    .SR_OFFSET_Q           (SR_OFFSET_Q),
    .SR_IQ_MAPPING         (SR_IQ_MAPPING),
    .SR_HET_PHASE_INCR     (SR_HET_PHASE_INCR),
    .BYPASS_DC_OFFSET_CORR (BYPASS_DC_OFFSET_CORR),
    .BYPASS_IQ_COMP        (BYPASS_IQ_COMP),
    .BYPASS_REALMODE_DSP   (BYPASS_REALMODE_DSP),
    .DEVICE                (DEVICE)
  ) rx_frontend_gen3_i (
    .clk      (clk),
    .reset    (rst),
    .sync_in  (sync_in),
    .set_stb  (set_stb),
    .set_addr (set_addr),
    .set_data (set_data),
    .adc_stb  (adc_stb),
    .adc_i    (adc_i),
    .adc_q    (adc_q),
    .rx_stb   (rx_stb),
    .rx_i     (rx_i),
    .rx_q     (rx_q)
  );


  //---------------------------------------------------------------------------
  // Input Generator
  //---------------------------------------------------------------------------


  real amplitude = 0.4375;               // Pick your favorite amplitude < 1.0
  real test_freq = CLK_FREQUENCY / 80.0; // Pick a frequency < CLK_FREQUENCY

  real    fp_amp = amplitude * (2**15);       // Scale amplitude to signed 16-bit
  real    f_norm = test_freq / CLK_FREQUENCY; // Normalized frequency
  longint count;

  assign adc_stb = 1;

  initial begin : gen_input
    count = 0;
    forever begin
      clk_gen.clk_wait_r();
      if (rst) begin
        count = 0;
        continue;
      end
      adc_i <= fp_amp * $cos(2.0*PI*f_norm*count);
      adc_q <= fp_amp * $sin(2.0*PI*f_norm*count);
      count++;
    end
  end : gen_input


  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  task automatic sr_write(logic [7:0] addr, logic [31:0] data);
    @clk;
    set_data <= data;
    set_addr <= addr;
    set_stb  <= 1;
    @clk;
    set_stb <= 0;
  endtask : sr_write


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    // Initialize the test exec object for this testbench
    test.start_tb("rx_frontend_gen3_tb");

    //--------------------------------
    // Reset
    //--------------------------------

    // Reset long enough for the FIR filter to flush. This isn't required for
    // synthesized code, but avoids getting 'X stuck in the round_sd module in
    // simulation.
    test.start_test("Reset", 10us);
    clk_gen.reset(50);
    @rst;
    test.end_test();

    //--------------------------------
    // Initialize Registers
    //--------------------------------

    test.start_test("Initialize registers", 10us);

    // Set IQ mapping (Enable down-conversion)
    sr_write(SR_IQ_MAPPING, DOWNCONVERT);

    // Set IQ compensation (no compensation)
    sr_write(SR_MAG_CORRECTION,   'h0);
    sr_write(SR_PHASE_CORRECTION, 'h0);

    // Set DC offset correction (fixed offset of 0)
    sr_write(SR_OFFSET_I, OFFSET_FIXED | OFFSET_SET | 'h00000000);
    sr_write(SR_OFFSET_Q, OFFSET_FIXED | OFFSET_SET | 'h00000000);

    // Set down-converter direction (+pi/2 per clock cycle)
    sr_write(SR_HET_PHASE_INCR, 'b0);

    test.end_test();

    //--------------------------------
    // Test Waveforms
    //--------------------------------

    test.start_test("Test Waveforms", 100us);

    clk_gen.clk_wait_r(2000);
    $display("Done! Check the waveforms in the GUI.  :-)");

    test.end_test();

    //--------------------------------
    // End TB
    //--------------------------------

    test.end_tb();

  end : tb_main

endmodule : rx_frontend_gen3_tb


`default_nettype wire
