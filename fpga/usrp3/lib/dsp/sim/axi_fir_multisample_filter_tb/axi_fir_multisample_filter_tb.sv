//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fir_multisample_filter_tb
//
// Description:
//
//   Testbench for axi_fir_multisample_filter.
//
// Parameters:
//   NUM_SPC:      How much sample per cycle is set
//   NUM_COEFFS:   Number of coefficients / taps
//   RELOADABLE_COEFFS        - Enable (1) or disable (0) reloading coefficients at runtime (via reload bus)
//   BLANK_OUTPUT             - Disable (1) or enable (0) output when initially filling internal pipeline
//   USE_EMBEDDED_REGS_COEFFS - Reduce register usage by only using embedded registers in DSP slices.
//                              Updating taps while streaming will cause temporary output corruption!
//

module axi_fir_multisample_filter_tb #(
  parameter       NUM_SPC                  = 8,
  parameter       NUM_COEFFS               = 41,
  parameter       RELOADABLE_COEFFS         = 1,
  parameter       BLANK_OUTPUT              = 1,
  parameter       USE_EMBEDDED_REGS_COEFFS  = 1
) ();

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Simulation parameters
  localparam real AXI_CLK_PER  = 10.0; // 100 MHz

  localparam int  STALL_PROB   = 38;  // BFM stall probability, default 38

  // DUT parameters to test
  localparam int IN_WIDTH                 = 16;
  localparam int AXI_WIDTH                = IN_WIDTH * NUM_SPC;
  localparam int COEFF_WIDTH              = 16;
  localparam int OUT_WIDTH                = 16;
  localparam int CLIP_BITS                = $clog2(NUM_COEFFS);
  localparam int ACCUM_WIDTH              = IN_WIDTH + COEFF_WIDTH + CLIP_BITS - 1;
  localparam int PIPELINE_DELAY           = NUM_COEFFS + 5;

  // How many groups of multisampled-input needed
  localparam int IN_GROUP_NUM             = $ceil(NUM_COEFFS*1.0/NUM_SPC);
  // +10 is pipeline compensation
  localparam int FLUSH_CYCLE              = ((NUM_SPC < NUM_COEFFS) ? NUM_COEFFS : NUM_SPC ) +10;

  localparam logic [COEFF_WIDTH*NUM_COEFFS-1:0] COEFFS_VEC_0 = {
    16'sd158,   16'sd0,     16'sd33,    -16'sd0,    -16'sd256,
    16'sd553,   16'sd573,   -16'sd542,  -16'sd1012, 16'sd349,
    16'sd1536,  16'sd123,   -16'sd2097, -16'sd1012, 16'sd1633,
    16'sd1608,  -16'sd3077, -16'sd5946, 16'sd3370,  16'sd10513,
    -16'sd19295, // 16'sd19295, change to negative to avoid clipping
    16'sd10513, 16'sd3370,  -16'sd5946, -16'sd3077, 16'sd1608,
    16'sd1633,  -16'sd1012, -16'sd2097, 16'sd123,   16'sd1536,
    16'sd349,   -16'sd1012, -16'sd542,  16'sd573,   16'sd553,
    -16'sd256,  -16'sd0,    16'sd33,    16'sd0,     16'sd158
  };

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit axi_clk, axi_rst, axi_clear=0;
  sim_clock_gen #(.PERIOD(AXI_CLK_PER)) axi_clk_gen (.clk(axi_clk), .rst(axi_rst));

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Deafult interface data width is 64bits
  typedef AxiStreamBfm #(AXI_WIDTH)::AxisPacket_t    AxisPacket_in;
  typedef AxiStreamBfm #(AXI_WIDTH)::AxisPacket_t    AxisPacket_out;
  typedef AxiStreamBfm #(COEFF_WIDTH)::AxisPacket_t  AxisPacket_reload;

  AxiStreamIf #(AXI_WIDTH)       AxisIf_m             (axi_clk, axi_rst);
  AxiStreamIf #(AXI_WIDTH)       AxisIf_s             (axi_clk, axi_rst);
  AxiStreamIf #(COEFF_WIDTH)     AxisIf_reload        (axi_clk, axi_rst);

  // Connect BFM to interface
  AxiStreamBfm #(AXI_WIDTH)       AxisIf_sample_bfm = new(AxisIf_m,AxisIf_s);
  AxiStreamBfm #(COEFF_WIDTH)     AxisIf_coeff_bfm  = new(AxisIf_reload, null);

  AxisPacket_in packet_in;
  AxisPacket_out packet_out;

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  // DUT Slave (Input) Port Signals
  logic [NUM_SPC*IN_WIDTH-1:0]             s_axis_data_tdata;
  logic                                    s_axis_data_tlast;
  logic                                    s_axis_data_tvalid;
  logic                                    s_axis_data_tready;

  // DUT Master (Output) Port Signals
  logic [NUM_SPC*OUT_WIDTH-1:0]            m_axis_data_tdata;
  logic                                    m_axis_data_tlast;
  logic                                    m_axis_data_tvalid;
  logic                                    m_axis_data_tready;

  // DUT Coefficient Reload (Input) Port Signals
  logic [COEFF_WIDTH-1:0]                  s_axis_reload_tdata;
  logic                                    s_axis_reload_tlast;
  logic                                    s_axis_reload_tvalid;
  logic                                    s_axis_reload_tready;

  // Random Coeff
  logic [COEFF_WIDTH*NUM_COEFFS-1:0] COEFFS_VEC_RANDOM;

  // Link to interface
  assign s_axis_data_tdata                = AxisIf_m.tdata;
  assign s_axis_data_tlast                = AxisIf_m.tlast;
  assign s_axis_data_tvalid               = AxisIf_m.tvalid;
  assign AxisIf_m.tready                  = s_axis_data_tready;

  assign AxisIf_s.tdata                   = m_axis_data_tdata;
  assign AxisIf_s.tlast                   = m_axis_data_tlast;
  assign AxisIf_s.tvalid                  = m_axis_data_tvalid;
  assign m_axis_data_tready               = AxisIf_s.tready;

  assign s_axis_reload_tdata              = AxisIf_reload.tdata;
  assign s_axis_reload_tlast              = AxisIf_reload.tlast;
  assign s_axis_reload_tvalid             = AxisIf_reload.tvalid;
  assign AxisIf_reload.tready             = s_axis_reload_tready;

  // Map the array of AXI to a flat vector for the DUT
  axi_fir_multisample_filter #(
    .IN_WIDTH(IN_WIDTH),
    .NUM_SPC(NUM_SPC),
    .COEFF_WIDTH(COEFF_WIDTH),
    .OUT_WIDTH(OUT_WIDTH),
    .NUM_COEFFS(NUM_COEFFS),
    .CLIP_BITS(CLIP_BITS),
    .ACCUM_WIDTH(ACCUM_WIDTH),
    .COEFFS_VEC(COEFFS_VEC_0),
    .RELOADABLE_COEFFS(RELOADABLE_COEFFS),
    .BLANK_OUTPUT(BLANK_OUTPUT),
    .USE_EMBEDDED_REGS_COEFFS(USE_EMBEDDED_REGS_COEFFS)
  ) axi_fir__multisample_filter_i(
    .clk(axi_clk),
    .reset(axi_rst),
    .clear(axi_clear),
    .s_axis_data_tdata(s_axis_data_tdata),
    .s_axis_data_tlast(s_axis_data_tlast),
    .s_axis_data_tvalid(s_axis_data_tvalid),
    .s_axis_data_tready(s_axis_data_tready),
    .m_axis_data_tdata(m_axis_data_tdata),
    .m_axis_data_tlast(m_axis_data_tlast),
    .m_axis_data_tvalid(m_axis_data_tvalid),
    .m_axis_data_tready(m_axis_data_tready),
    .s_axis_reload_tdata(s_axis_reload_tdata),
    .s_axis_reload_tvalid(s_axis_reload_tvalid),
    .s_axis_reload_tlast(s_axis_reload_tlast),
    .s_axis_reload_tready(s_axis_reload_tready)
  );


  //---------------------------------------------------------------------------
  // Local Functions and Tasks
  //---------------------------------------------------------------------------

  // Local reset function
  task reset_axi (input int rst_cyc = 100);
    axi_clk_gen.clk_wait_f();
    axi_clk_gen.reset();
    wait(axi_rst == 0);
    repeat (rst_cyc) axi_clk_gen.clk_wait_f();
  endtask : reset_axi

  // Local random coeff generation function
 function automatic void random_coeff_generation (output [COEFF_WIDTH*NUM_COEFFS-1:0] COEFFS_VEC_RANDOM);
    logic signed [COEFF_WIDTH-1:0] random_coeff;
    for (int i = 0; i < NUM_COEFFS; i++) begin
      // Truncate 32 bits to 16 bits and make it signed
      random_coeff = Rand #(COEFF_WIDTH)::rand_sbit_range(-2000, 2000);
      COEFFS_VEC_RANDOM[i*COEFF_WIDTH +: COEFF_WIDTH] = random_coeff;
    end
  endfunction:random_coeff_generation

  // Local function to collect single samples
  // until all the samples collected, add into packet.
  // Flush : 0 (send valid data)/ 1 (automatically flush)
  task add_sample (input logic [IN_WIDTH-1:0] sample, input int flush = 0);
    static logic [AXI_WIDTH-1:0] sample_collected = ('0);
    static int count_SPC = 0; // NUM_SPC samples as one group

    // append sample
    sample_collected [count_SPC * IN_WIDTH +: IN_WIDTH] = sample;
    count_SPC = count_SPC + 1;

    // fill remaining data until vector is complete
    if (flush == 1) begin
      for (int i = count_SPC; i<NUM_SPC; i++) begin
        sample_collected [count_SPC * IN_WIDTH +: IN_WIDTH] = '0;
      end
      count_SPC = NUM_SPC;
    end
    if (count_SPC == NUM_SPC) begin
      packet_in.data.push_back(sample_collected);
      count_SPC = 0;
      sample_collected = ('0);
    end
  endtask: add_sample

  // Local function to flush data inside the filter
  task flush_axi();
    // Flush the possible empty place from last pacekt
    for (int i = 0; i < FLUSH_CYCLE; i++) begin
      add_sample('0, 1);
    end
    AxisIf_sample_bfm.put(packet_in.copy());
    packet_in.empty();
  endtask: flush_axi

  // Local task: get one single sample when called.
  // Automatically grab through AXI_BFM from DUT and pop out
  task get_sample (output logic [IN_WIDTH-1:0] sample, input logic initialize = 0);
    static logic [AXI_WIDTH-1:0] sample_collected = ('0);
    // count up from 0 to NUM_SPC - 1
    static int count_SPC = 0;
    // count down from number of elements in the packet to 0
    static int count_elements = 0;

    // set both counters to end of range
    if (initialize) begin
      count_SPC = NUM_SPC-1;
      count_elements = 0;
    end
    // get new vector from packet
    if (count_SPC == NUM_SPC-1) begin
      // get a new packet
      if ( count_elements == 0 ) begin
        AxisIf_sample_bfm.get(packet_out);
        count_elements = packet_out.data.size();
      end
      sample_collected = packet_out.data.pop_front();
      count_SPC = 0;
      count_elements = count_elements - 1;
    end else begin
      count_SPC = count_SPC + 1;
    end

    sample = sample_collected [count_SPC*IN_WIDTH +: IN_WIDTH];
  endtask: get_sample


  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main

    string s;
    $sformat(s, {
      "tb_axi_fir_multisample_filter SPC= %0d NUM_COEFFS=%0d RELOADABLE=%0d ",
      "BLANK_OUTPUT=%0d EMBEDDED_REGS=%0d"}, NUM_SPC, NUM_COEFFS,
      RELOADABLE_COEFFS, BLANK_OUTPUT, USE_EMBEDDED_REGS_COEFFS);

    // stop all clock events for simulation performance
    axi_clk_gen.kill();

    // Display testbench start message
    test.start_tb(s);
    axi_clk_gen.revive();

    // Set stall probability
    AxisIf_sample_bfm.set_master_stall_prob(STALL_PROB);
    AxisIf_sample_bfm.set_slave_stall_prob(STALL_PROB);
    AxisIf_coeff_bfm.set_master_stall_prob(STALL_PROB);
    AxisIf_coeff_bfm.set_slave_stall_prob(STALL_PROB);

    // Start the BFMs running
    AxisIf_sample_bfm.run();
    AxisIf_coeff_bfm.run();

    // initialize variables
    random_coeff_generation(COEFFS_VEC_RANDOM);
    packet_in = new();
    packet_out = new();

    //-------------------------------------------------------------------------
    // Reset
    //-------------------------------------------------------------------------

    test.start_test("Wait for Reset", 10us);
    reset_axi();
    test.end_test();

    //-------------------------------------------------------------------------
    // Initial load of coefficients VEC_0 (If USE_EMBEDDED_REGS_COEFFS enabled)
    //-------------------------------------------------------------------------

    begin
      automatic AxisPacket_reload packet_reload = new();

      // If using embedded register, coefficients must be preloaded
      if (USE_EMBEDDED_REGS_COEFFS) begin
        test.start_test("Initial load of coefficients VEC_0", 10us);
        // Generate packet which contains new coefficients and enqueue it for transfer
        packet_reload.empty();
        // Reload must send data in reverse direction
        for (int i= NUM_COEFFS-1 ; i>=0; i--) begin
          packet_reload.data.push_back(COEFFS_VEC_0[COEFF_WIDTH*i +: COEFF_WIDTH]);
        end
        AxisIf_coeff_bfm.put(packet_reload);
        AxisIf_coeff_bfm.wait_complete();
        test.end_test();
      end
    end

    //-----------------------------------------------------------------------
    // Test impulse response with default coefficients
    //-----------------------------------------------------------------------
    //
    // Sending an impulse should cause the coefficients to be output.
    //
    //-----------------------------------------------------------------------

    begin
      logic signed [COEFF_WIDTH-1:0] i_coeff, i_samp_int;
      string s;

      test.start_test("Test impulse response (default coefficients)", 20us);

      packet_in.empty();

      // Send single sample to DUT
      // Function will automatically packet and send
      add_sample(16'h7FFF);
      for (int i = 1; i < NUM_COEFFS; i++) begin
        add_sample(16'h0000);
      end
      // Compensate the unfilled data in last group
      add_sample('0, 1);
      AxisIf_sample_bfm.put(packet_in.copy());

      // Enqueue flushing packet and Residue to push the data out
      packet_in.empty();
      flush_axi();

      // If BLANK_OUTPUT enabled, internal pipeline fullfilled.
      // The correct output is supposed to apprear after.
      // Keep grabbing until the first non_zero output
      if (BLANK_OUTPUT == 0) begin
        do begin
          get_sample(i_samp_int, 0);
        end while(i_samp_int== ('0));
      end

      // Correctness check
      for (int i=0 ; i< NUM_COEFFS; i++) begin
        if (BLANK_OUTPUT == 0 && i==0) begin
          i_samp_int = i_samp_int; // verify the first one, which is already grabbed out
        end else begin
          get_sample(i_samp_int, i == 0); // ask one single sample from output
        end
        i_coeff = $signed(COEFFS_VEC_0[COEFF_WIDTH*i +: COEFF_WIDTH]);

        $sformat(
          s, "Incorrect I value received on sample %0d! Expected: %0d, Received: %0d",
          i, i_coeff, i_samp_int);
        `ASSERT_ERROR(
          (i_samp_int == i_coeff) || (i_samp_int-1 == i_coeff) || (i_samp_int+1 == i_coeff),
          s);
      end

      test.end_test();
    end

    //-----------------------------------------------------------------------
    // Load random coefficients
    //-----------------------------------------------------------------------

    // If RELOADABLE_COEFFS disabled, skip the rest
    if (RELOADABLE_COEFFS==1) begin
      begin
        automatic AxisPacket_reload packet_reload = new();

        test.start_test("Load random coefficients", 10us);
        // Generate packet which contains new coefficients and enqueue it for transfer
        packet_reload.empty();
        // reload must send data in reverse direction
        for (int i= NUM_COEFFS-1 ; i>=0; i--) begin
          packet_reload.data.push_back(COEFFS_VEC_RANDOM[COEFF_WIDTH*i +: COEFF_WIDTH]);
        end
        AxisIf_coeff_bfm.put(packet_reload);
        AxisIf_coeff_bfm.wait_complete();

        test.end_test();
      end

      //-----------------------------------------------------------------------
      // Test impulse response with random coefficients
      //-----------------------------------------------------------------------
      //
      // Sending an impulse should cause the coefficients to be output.
      //
      //-----------------------------------------------------------------------

      begin
        logic signed [COEFF_WIDTH-1:0] i_coeff, i_samp_int;
        string s;

        test.start_test("Test impulse response (random coefficients)", 20us);

        packet_in.empty();

        // Send single sample to DUT
        // Function will automatically packet and send
        add_sample(16'h7FFF);
        for (int i = 1; i < NUM_COEFFS; i++) begin
          add_sample(16'h0000);
        end
        // Compensate the unfilled data in last group
        add_sample('0, 1);
        AxisIf_sample_bfm.put(packet_in.copy());

        // Enqueue flushing packet and Residue to push the data out
        packet_in.empty();
        flush_axi();

        // When BLANK_OUTPUT enabled, no extra ignore needed
        if (BLANK_OUTPUT == 0) begin
          do begin
            get_sample(i_samp_int, 0);
          end while(i_samp_int== ('0));
        // Ignore the packet as the result of last flushing
        end else begin
          AxisIf_sample_bfm.get(packet_out);
        end

        //  Correctness check
        for (int i=0 ; i< NUM_COEFFS; i++) begin
          if (!(BLANK_OUTPUT == 0 && i==0)) begin
            get_sample(i_samp_int, i == 0); // ask one single sample from output
          end
          i_coeff = $signed(COEFFS_VEC_RANDOM[COEFF_WIDTH*i +: COEFF_WIDTH]);

          $sformat(
            s, "Incorrect I value received on sample %0d! Expected: %0d, Received: %0d",
            i, i_coeff, i_samp_int);
          `ASSERT_ERROR(
            (i_samp_int == i_coeff) || (i_samp_int-1 == i_coeff) || (i_samp_int+1 == i_coeff),
            s);
        end

        test.end_test();
      end

      //-----------------------------------------------------------------------
      // Test step response with random coefficients
      //-----------------------------------------------------------------------

      begin
        logic signed [COEFF_WIDTH-1:0] i_samp_int;
        string s;
        int coeff_sum;

        test.start_test("Test step response (random coefficients)", 20us);

        packet_in.empty();

        // Send single sample to DUT
        // Function will automatically packet and send
        for (int i = 0; i < NUM_COEFFS; i++) begin
          add_sample(16'h7FFF);
        end
        // Compensate the unfilled data in last group
        add_sample('0, 1);
        AxisIf_sample_bfm.put(packet_in.copy());

        // Enqueue flushing packet and Residue to push the data out
        packet_in.empty();
        flush_axi();

        // When BLANK_OUTPUT enabled, no extra ignore needed
        if (BLANK_OUTPUT == 0) begin
          do begin
            get_sample(i_samp_int, 0);
          end while(i_samp_int== ('0));
        // Ignore the packet as the result of last flushing
        end else begin
          AxisIf_sample_bfm.get(packet_out);
        end

        // Correctness check
        for (int i=0 ; i< NUM_COEFFS; i++) begin
          if (BLANK_OUTPUT == 0 && i==0) begin
            i_samp_int = i_samp_int; // verify the first one, which is already grabbed out
          end else begin
            get_sample(i_samp_int, i == 0); // ask one single sample from output
          end
          coeff_sum += $signed(COEFFS_VEC_RANDOM[COEFF_WIDTH*i +: COEFF_WIDTH]);

          $sformat(
            s, "Incorrect I value received on sample %0d! Expected: %0d, Received: %0d",
            i, coeff_sum, i_samp_int);
          `ASSERT_ERROR(
            (i_samp_int == coeff_sum) || (i_samp_int-1 == coeff_sum) || (i_samp_int+1 == coeff_sum),
            s);
        end

        test.end_test();
      end
     end
    //-------------------------------------------------------------------------
    // All done!
    //-------------------------------------------------------------------------
    // End the testbench and stop the clock module
    test.end_tb(0);
    axi_clk_gen.kill();

  end : tb_main
endmodule
