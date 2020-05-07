//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_vector_iir_tb
//
// Description: Testbench for the vector_iir RFNoC block.
//

`default_nettype none


module rfnoc_block_vector_iir_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  `include "rfnoc_block_vector_iir_regs.vh"


  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'h11120000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = 64;    // CHDR size in bits
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS       = 2;
  localparam int    NUM_PORTS_I     = 0+NUM_PORTS;
  localparam int    NUM_PORTS_O     = 0+NUM_PORTS;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 50;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam real   CE_CLK_PER      = 4.0;   // 250 MHz

  localparam int  MAX_DELAY   = (2**MTU)*(CHDR_W/ITEM_W)-1;
  localparam int  NUM_PKTS    = 50;       // Number of packets to test
  localparam int  VECTOR_SIZE = SPP;      // Vector size to test
  localparam real ERROR       = 2.0**-12; // Target 72dB of dynamic range


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER) ce_clk_gen (.clk(ce_clk), .rst());


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

  // AXIS-CHDR Interfaces
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS_I] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS_O] (rfnoc_chdr_clk, 1'b0);

  // Block Controller BFM
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // CHDR word and item/sample data types
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_bfm_input_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
    end
  end
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_bfm_output_connections
    initial begin
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  // DUT Slave (Input) Port Signals
  logic [CHDR_W*NUM_PORTS_I-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tready;

  // DUT Master (Output) Port Signals
  logic [CHDR_W*NUM_PORTS_O-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT connections
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_dut_input_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];
  end
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_dut_output_connections
    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end

  rfnoc_block_vector_iir #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS),
    .MAX_DELAY           (MAX_DELAY)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk              (ce_clk),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready)
  );


  //---------------------------------------------------------------------------
  // Filter Model
  //---------------------------------------------------------------------------

  task automatic iir_filter (
    input real  alpha,
    input real  beta,
    input real  in[],
    output real out[]
  );
    out = new[in.size()];
    for (int i = 0; i < in.size(); i++) begin
      real yd = i >= 1 ? out[i-1] : 0.0;
      out[i] = in[i]*beta + yd*alpha;
      `ASSERT_FATAL(abs(out[i]) <= 1.0,
        "Expected value for filtered data falls outside allowed range.");
    end
  endtask : iir_filter


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Write a 32-bit register
  task automatic write_reg(int port, bit [19:0] addr, bit [31:0] value);
    blk_ctrl.reg_write((2**VECTOR_IIR_ADDR_W)*port + addr, value);
  endtask : write_reg

  // Read a 32-bit register
  task automatic read_reg(int port, bit [19:0] addr, output logic [63:0] value);
    blk_ctrl.reg_read((2**VECTOR_IIR_ADDR_W)*port + addr, value[31: 0]);
  endtask : read_reg

  // Real to fixed-point
  function bit [15:0] real_to_fxp (real x);
    return int'($floor(x * ((2**15)-1)));
  endfunction

  // Fixed-point to real
  function real fxp_to_real(bit [15:0] x);
    return real'($signed(x))/((2**15)-1);
  endfunction

  // Absolute value
  function real abs(real x);
    return (x > 0.0) ? x : -x;
  endfunction


  //---------------------------------------------------------------------------
  // Register Test Tasks
  //---------------------------------------------------------------------------

  // Test a read/write register for correct functionality
  //
  //   port          : Replay block port to use
  //   addr          : Register byte address
  //   mask          : Mask of the bits we expect to be writable
  //   initial_value : Value we expect to read initially
  //
  task automatic test_read_write_reg(
    int          port,
    bit   [19:0] addr,
    bit   [31:0] mask = 32'hFFFFFFFF,
    logic [31:0] initial_value = '0
  );
    string       err_msg;
    logic [31:0] value;
    logic [31:0] expected;

    err_msg = $sformatf("Register 0x%X failed read/write test: ", addr);

    // Check initial value
    expected = initial_value;
    read_reg(port, addr, value);
    `ASSERT_ERROR(value === expected, {err_msg, "initial value"});

    // Test writing 0
    expected = (initial_value & ~mask);
    write_reg(port, addr, '0);
    read_reg(port, addr, value);
    `ASSERT_ERROR(value === expected, {err_msg, "write zero"});

    // Write maximum value
    expected = (initial_value & ~mask) | mask;
    write_reg(port, addr, '1);
    read_reg(port, addr, value);
    `ASSERT_ERROR(value === expected, {err_msg, "write max value"});

    // Restore original value
    write_reg(port, addr, initial_value);
  endtask : test_read_write_reg


  //---------------------------------------------------------------------------
  // Test registers
  //---------------------------------------------------------------------------

  task automatic test_registers(int port = 0);
    test.start_test("Test registers", 100us);

    // Test Delay (Vector Length) register. The MAX_DELAY portion is
    // ready-only. DELAY portion is read/write.
    test_read_write_reg(
      port,
      REG_DELAY,
      {$clog2(MAX_DELAY+1){1'b1}} << REG_DELAY_POS,
      (MAX_DELAY << REG_MAX_DELAY_POS) | ({$clog2(MAX_DELAY+1){1'bX}} << REG_DELAY_POS)
    );

    // Test Alpha register
    test_read_write_reg(
      port,
      REG_ALPHA,
      ((1<<REG_ALPHA_LEN)-1) << REG_ALPHA_POS,
      {REG_ALPHA_LEN{1'bX}} << REG_ALPHA_POS
    );

    // Test Beta register
    test_read_write_reg(
      port,
      REG_BETA,
      ((1<<REG_BETA_LEN)-1) << REG_BETA_POS,
      {REG_BETA_LEN{1'bX}} << REG_BETA_POS
    );
    test.end_test();
  endtask : test_registers


  //---------------------------------------------------------------------------
  // Test impulse and step response
  //---------------------------------------------------------------------------

  task automatic test_impulse_and_step(int port = 0);
    real in_I[], in_Q[], out_I[], out_Q[];
    real alpha, beta;

    test.start_test("Check impulse and step response", 100us);

    alpha = 0.7;
    beta = 0.3;
    write_reg(port, REG_DELAY, VECTOR_SIZE);
    write_reg(port, REG_ALPHA, real_to_fxp(alpha));
    write_reg(port, REG_BETA,  real_to_fxp(beta));

    // Generate input and golden output vector
    in_I = new[NUM_PKTS];
    in_Q = new[NUM_PKTS];
    for (int n = 0; n < NUM_PKTS; n++) begin
      // First half is an impulse, second half is a step
      in_I[n] = (n == 0 || n >= NUM_PKTS/2) ?  1.0 : 0.0;
      in_Q[n] = (n == 0 || n >= NUM_PKTS/2) ? -1.0 : 0.0;
    end
    iir_filter(alpha, beta, in_I, out_I);
    iir_filter(alpha, beta, in_Q, out_Q);
    // Send, receive and validate data
    fork
      begin : send_packets
        item_t samples[$];
        for (int n = 0; n < NUM_PKTS; n++) begin
          for (int k = 0; k < VECTOR_SIZE; k++) begin
            samples[k] = {real_to_fxp(in_I[n]), real_to_fxp(in_Q[n])};
          end
          blk_ctrl.send_items(port, samples);
        end
      end
      begin : check_packets
        item_t samples[$];
        real recv_i, recv_q;
        for (int n = 0; n < NUM_PKTS; n++) begin
          blk_ctrl.recv_items(port, samples);
          `ASSERT_ERROR(samples.size() == VECTOR_SIZE,
            "Received packet has incorrect number of samples");
          for (int k = 0; k < VECTOR_SIZE; k++) begin
            recv_i = fxp_to_real(samples[k][31:16]);
            recv_q = fxp_to_real(samples[k][15:0]);
            `ASSERT_ERROR(abs(recv_i - out_I[n]) < ERROR, "Incorrect I value");
            `ASSERT_ERROR(abs(recv_q - out_Q[n]) < ERROR, "Incorrect Q value");
          end
        end
      end
    join
    test.end_test();
  endtask : test_impulse_and_step


  //---------------------------------------------------------------------------
  // Test quarter rate sine response (vector stride)
  //---------------------------------------------------------------------------

  task automatic test_vector_stride(int port = 0);
    real in_I[], in_Q[], out_I[], out_Q[];
    real alpha, beta;

    test.start_test("Check quarter rate complex sine response (vector stride)", 100us);
    alpha = 0.9;
    beta = 0.1;
    write_reg(port, REG_DELAY, VECTOR_SIZE);
    write_reg(port, REG_ALPHA, real_to_fxp(alpha));
    write_reg(port, REG_BETA, real_to_fxp(beta));
    // Generate input and golden output vector
    in_I = new[NUM_PKTS];
    in_Q = new[NUM_PKTS];
    for (int n = 0; n < NUM_PKTS; n++) begin
      // First half is an impulse, second half is a step
      in_I[n] = (n % 4 == 1 || n % 4 == 3) ? 0.0 : ((n % 4 == 0) ? 1.0 : -1.0);  // cos
      in_Q[n] = (n % 4 == 0 || n % 4 == 2) ? 0.0 : ((n % 4 == 1) ? 1.0 : -1.0);  // sin
    end
    iir_filter(alpha, beta, in_I, out_I);
    iir_filter(alpha, beta, in_Q, out_Q);
    // Send, receive and validate data
    fork
      begin : send_packets
        item_t samples[$];
        for (int n = 0; n < NUM_PKTS; n++) begin
          for (int k = 0; k < VECTOR_SIZE; k++) begin
            samples[k] = {real_to_fxp(in_I[n]), real_to_fxp(in_Q[n])};
          end
          blk_ctrl.send_items(port, samples);
        end
      end
      begin : check_packets
        item_t samples[$];
        real recv_i, recv_q;
        for (int n = 0; n < NUM_PKTS; n++) begin
          blk_ctrl.recv_items(port, samples);
          `ASSERT_ERROR(samples.size() == VECTOR_SIZE,
            "Received packet has incorrect number of samples");
          for (int k = 0; k < VECTOR_SIZE; k++) begin
            recv_i = fxp_to_real(samples[k][31:16]);
            recv_q = fxp_to_real(samples[k][15:0]);
            `ASSERT_ERROR(abs(recv_i - out_I[n]) < ERROR, "Incorrect I value");
            `ASSERT_ERROR(abs(recv_q - out_Q[n]) < ERROR, "Incorrect Q value");
          end
        end
      end
    join
    test.end_test();
  endtask : test_vector_stride


  //---------------------------------------------------------------------------
  // Test quarter rate sine response (sample stride)
  //---------------------------------------------------------------------------

  task automatic test_sample_stride(int port = 0);
    real in_I[], in_Q[], out_I[], out_Q[];
    real alpha, beta;

    test.start_test("Check quarter rate complex sine response (sample stride)", 100us);
    alpha = 0.01;
    beta = 0.99;
    write_reg(port, REG_DELAY, VECTOR_SIZE);
    write_reg(port, REG_ALPHA, real_to_fxp(alpha));
    write_reg(port, REG_BETA, real_to_fxp(beta));
    // Generate input and golden output vector
    in_I = new[NUM_PKTS];
    in_Q = new[NUM_PKTS];
    for (int n = 0; n < NUM_PKTS; n++) begin
      // First half is an impulse, second half is a step
      in_I[n] = (n % 4 == 1 || n % 4 == 3) ? 0.0 : ((n % 4 == 0) ? 1.0 : -1.0);  // cos
      in_Q[n] = (n % 4 == 0 || n % 4 == 2) ? 0.0 : ((n % 4 == 1) ? 1.0 : -1.0);  // sin
    end
    iir_filter(alpha, beta, in_I, out_I);
    iir_filter(alpha, beta, in_Q, out_Q);
    // Send, receive and validate data
    fork
      begin
        item_t samples[$];
        for (int n = 0; n < NUM_PKTS; n++) begin
          for (int k = 0; k < VECTOR_SIZE; k++) begin
            if (k % 4 == 0)
              samples[k] = {real_to_fxp(in_I[n]), real_to_fxp(in_Q[n])};
            else if (k % 4 == 2)
              samples[k] = {real_to_fxp(in_Q[n]), real_to_fxp(in_I[n])};
            else
              samples[k] = {real_to_fxp(0.0), real_to_fxp(0.0)};
          end
          blk_ctrl.send_items(port, samples);
        end
      end
      begin
        item_t samples[$];
        real recv_i, recv_q;
        for (int n = 0; n < NUM_PKTS; n++) begin
          blk_ctrl.recv_items(port, samples);
          `ASSERT_ERROR(samples.size() == VECTOR_SIZE,
            "Received packet has incorrect number of samples");
          for (int k = 0; k < VECTOR_SIZE; k++) begin
            recv_i = fxp_to_real(samples[k][31:16]);
            recv_q = fxp_to_real(samples[k][15:0]);
            if (k % 4 == 0) begin
              `ASSERT_ERROR(abs(recv_i - out_I[n]) < 0.01, "Incorrect I value");
              `ASSERT_ERROR(abs(recv_q - out_Q[n]) < 0.01, "Incorrect Q value");
            end else if (k % 4 == 2) begin
              `ASSERT_ERROR(abs(recv_i - out_Q[n]) < 0.01, "Incorrect I value");
              `ASSERT_ERROR(abs(recv_q - out_I[n]) < 0.01, "Incorrect Q value");
            end else begin
              `ASSERT_ERROR(abs(recv_i) < 0.01, "Incorrect I value");
              `ASSERT_ERROR(abs(recv_q) < 0.01, "Incorrect Q value");
            end
          end
        end
      end
    join
    test.end_test();
  endtask : test_sample_stride


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_vector_iir_tb");

    // Start the BFMs running
    blk_ctrl.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Flush block then reset it", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();

    //--------------------------------
    // Verify Block Info
    //--------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS_I, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS_O, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    for (int port = 0; port < NUM_PORTS; port++) begin
      // Run these tests on all ports
      test_registers(port);
      test_impulse_and_step(port);
    end
    test_vector_stride();
    test_sample_stride();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_vector_iir_tb


`default_nettype wire
