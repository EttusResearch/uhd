//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fir_filter_tb
//
// Description: Testbench for rfnoc_block_fir_filter
//


module rfnoc_block_fir_filter_tb #(
  parameter int NUM_PORTS = 2
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;

  `include "rfnoc_fir_filter_regs.vh"


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  localparam int NOC_ID = 32'hF112_0000;

  // Simulation parameters
  localparam real CHDR_CLK_PER = 6.0; // 166 MHz
  localparam real CE_CLK_PER   = 5.0; // 200 MHz
  localparam int  STALL_PROB   = 25;  // BFM stall probability

  // DUT parameters to test
  localparam int CHDR_W      = 64;
  localparam int SAMP_W      = 32;
  localparam int THIS_PORTID = 'h123;
  localparam int MTU         = 8;
  //
  localparam int NUM_COEFFS               = 41;
  localparam int COEFF_WIDTH              = 16;
  localparam int RELOADABLE_COEFFS        = 1;
  localparam int SYMMETRIC_COEFFS         = 1;
  localparam int SKIP_ZERO_COEFFS         = 1;
  localparam int USE_EMBEDDED_REGS_COEFFS = 1;

  localparam logic [COEFF_WIDTH*NUM_COEFFS-1:0] COEFFS_VEC_0 = {
     16'sd158,   16'sd0,     16'sd33,    -16'sd0,    -16'sd256,
     16'sd553,   16'sd573,   -16'sd542,  -16'sd1012, 16'sd349,
     16'sd1536,  16'sd123,   -16'sd2097, -16'sd1012, 16'sd1633,
     16'sd1608,  -16'sd3077, -16'sd5946, 16'sd3370,  16'sd10513,
     16'sd19295,
     16'sd10513, 16'sd3370,  -16'sd5946, -16'sd3077, 16'sd1608,
     16'sd1633,  -16'sd1012, -16'sd2097, 16'sd123,   16'sd1536,
     16'sd349,   -16'sd1012, -16'sd542,  16'sd573,   16'sd553,
     -16'sd256,  -16'sd0,    16'sd33,    16'sd0,     16'sd158
  };

  localparam logic [COEFF_WIDTH*NUM_COEFFS-1:0] COEFFS_VEC_1 = {
    16'sd32767,  16'sd0,      -16'sd32767, 16'sd0,      16'sd32767,
    -16'sd32767, 16'sd32767,  -16'sd32767, 16'sd32767,  -16'sd32767,
    16'sd32767,  16'sd32767,  16'sd32767,  16'sd32767,  16'sd32767,
    -16'sd32767, -16'sd32767, -16'sd32767, -16'sd32767, -16'sd32767,
    16'sd32767,
    -16'sd32767, -16'sd32767, -16'sd32767, -16'sd32767, -16'sd32767,
    16'sd32767,  16'sd32767,  16'sd32767,  16'sd32767,  16'sd32767,
    -16'sd32767, 16'sd32767,  -16'sd32767, 16'sd32767,  -16'sd32767,
    16'sd32767,  16'sd0,      -16'sd32767, 16'sd0,      16'sd32767
  };

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CHDR_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER)   ce_clk_gen         (.clk(ce_clk),         .rst());


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  typedef ChdrData #(CHDR_W, SAMP_W)::chdr_word_t chdr_word_t;

  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(CHDR_W, SAMP_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i]);
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tready;

  logic [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT
  genvar i;
  for (i = 0; i < NUM_PORTS; i++) begin : gen_dut_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];

    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end


  rfnoc_block_fir_filter #(
    .THIS_PORTID              (THIS_PORTID),
    .CHDR_W                   (CHDR_W),
    .NUM_PORTS                (NUM_PORTS),
    .MTU                      (MTU),
    .COEFF_WIDTH              (COEFF_WIDTH),
    .NUM_COEFFS               (NUM_COEFFS),
    .COEFFS_VEC               (COEFFS_VEC_0),
    .RELOADABLE_COEFFS        (RELOADABLE_COEFFS),
    .SYMMETRIC_COEFFS         (SYMMETRIC_COEFFS),
    .SKIP_ZERO_COEFFS         (SKIP_ZERO_COEFFS),
    .USE_EMBEDDED_REGS_COEFFS (USE_EMBEDDED_REGS_COEFFS)
  ) rfnoc_block_fir_filter_i (
    .ce_clk              (ce_clk),
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .rfnoc_ctrl_clk      (backend.ctrl_clk),
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
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Translate the desired register access to a ctrlport write request.
  task automatic write_reg(int port, byte addr, bit [31:0] value);
    blk_ctrl.reg_write(port * (2**FIR_FILTER_ADDR_W) + addr, value);
  endtask : write_reg

  // Translate the desired register access to a ctrlport read request.
  task automatic read_reg(int port, byte addr, output logic [31:0] value);
    blk_ctrl.reg_read(port * (2**FIR_FILTER_ADDR_W), value);
  endtask : read_reg



  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    // Display testbench start message
    test.start_tb("rfnoc_block_fir_filter_tb");

    // Start the BFMs running
    blk_ctrl.run();


    //-------------------------------------------------------------------------
    // Reset
    //-------------------------------------------------------------------------
    
    test.start_test("Wait for Reset", 10us);
    fork
      blk_ctrl.reset_chdr();
      blk_ctrl.reset_ctrl();
    join;
    test.end_test();


    //-------------------------------------------------------------------------
    // Check NoC ID and Block Info
    //-------------------------------------------------------------------------
    
    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();


    // Test all ports
    for (int port = 0; port < NUM_PORTS; port++) begin : port_loop

      //-----------------------------------------------------------------------
      // Check filter length
      //-----------------------------------------------------------------------

      begin
        int num_coeffs, num_coeffs_to_send;

        test.start_test("Check filter length", 20us);

        read_reg(port, REG_FIR_NUM_COEFFS, num_coeffs);
        `ASSERT_ERROR(num_coeffs, "Incorrect number of coefficients");

        // If using symmetric coefficients, send just first half
        if (SYMMETRIC_COEFFS) begin
          num_coeffs_to_send = num_coeffs/2 + num_coeffs[0];
        end else begin
          num_coeffs_to_send = num_coeffs;
        end

        // If using embedded register, coefficients must be preloaded
        if (USE_EMBEDDED_REGS_COEFFS) begin
          int i;
          for (i = 0; i < num_coeffs_to_send-1; i++) begin
            write_reg(port, REG_FIR_LOAD_COEFF, COEFFS_VEC_0[COEFF_WIDTH*i +: COEFF_WIDTH]);
          end
          write_reg(port, REG_FIR_LOAD_COEFF_LAST, COEFFS_VEC_0[COEFF_WIDTH*i +: COEFF_WIDTH]);
        end
        
        test.end_test();
      end


      //-----------------------------------------------------------------------
      // Test impulse response with default coefficients
      //-----------------------------------------------------------------------
      //
      // Sending an impulse should cause the coefficients to be output.
      //
      //-----------------------------------------------------------------------

      begin
        chdr_word_t send_payload[$];
        chdr_word_t recv_payload[$];
        int num_bytes;
        logic signed [15:0] i_samp, q_samp, i_coeff, q_coeff;
        string s;

        test.start_test("Test impulse response (default coefficients)", 20us);

        // Generate packet containing an impulse and enqueue it for transfer
        send_payload = {};
        send_payload.push_back({16'b0, 16'b0, 16'h7FFF, 16'h7FFF});
        for (int i = 0; i < NUM_COEFFS/2; i++) begin
          send_payload.push_back(0);
        end
        blk_ctrl.send(port, send_payload, NUM_COEFFS*4);

        // Enqueue two packets with zeros to push out the impulse from the
        // pipeline (one to push out the data and one to overcome some pipeline
        // registering).
        send_payload = {};
        for (int i = 0; i < NUM_COEFFS/2+1; i++) begin
          send_payload.push_back(0);
        end
        for (int n = 0; n < 2; n++) begin
          blk_ctrl.send(port, send_payload, NUM_COEFFS*4);
        end

        // Receive the result
        blk_ctrl.recv(port, recv_payload, num_bytes);

        // Check the length of the packet
        `ASSERT_ERROR(
          num_bytes == NUM_COEFFS*4,
          "Received packet didn't have expected length"
        );

        for (int i = 0; i < NUM_COEFFS; i++) begin
          // Compute the expected sample
          i_coeff = $signed(COEFFS_VEC_0[COEFF_WIDTH*i +: COEFF_WIDTH]);
          q_coeff = i_coeff;

          // Grab the next sample
          {i_samp, q_samp} = recv_payload[i/2][i[0]*32 +: 32];

          // Check I / Q values
          $sformat(
            s, "Incorrect I value received on sample %0d! Expected: %0d, Received: %0d", 
            i, i_coeff, i_samp);
          `ASSERT_ERROR(
            (i_samp == i_coeff) || (i_samp-1 == i_coeff) || (i_samp+1 == i_coeff), s);
          $sformat(
            s, "Incorrect Q value received on sample %0d! Expected: %0d, Received: %0d", 
            i, q_coeff, q_samp);
          `ASSERT_ERROR(
            (q_samp == q_coeff) || (q_samp-1 == q_coeff) || (q_samp+1 == q_coeff), s);
        end

        test.end_test();
      end


      //-----------------------------------------------------------------------
      // Load new coefficients
      //-----------------------------------------------------------------------

      begin
        int i;
        int num_coeffs_to_send;

        // If using symmetric coefficients, send just first half
        if (SYMMETRIC_COEFFS) begin
          num_coeffs_to_send = NUM_COEFFS/2 + NUM_COEFFS[0];
        end else begin
          num_coeffs_to_send = NUM_COEFFS;
        end

        test.start_test("Load new coefficients", 20us);
        for (i = 0; i < num_coeffs_to_send-1; i++) begin
          write_reg(port, REG_FIR_LOAD_COEFF, COEFFS_VEC_1[COEFF_WIDTH*i +: COEFF_WIDTH]);
        end
        write_reg(port, REG_FIR_LOAD_COEFF_LAST, COEFFS_VEC_1[COEFF_WIDTH*i +: COEFF_WIDTH]);
        test.end_test();    
      end


      //-----------------------------------------------------------------------
      // Test impulse response with new coefficients
      //-----------------------------------------------------------------------
      //
      // Sending an impulse should cause the coefficients to be output.
      //
      //-----------------------------------------------------------------------

      begin
        chdr_word_t send_payload[$];
        chdr_word_t recv_payload[$];
        int num_bytes;
        logic signed [15:0] i_samp, q_samp, i_coeff, q_coeff;
        string s;

        test.start_test("Test impulse response (loaded coefficients)", 20us);

        // Generate packet containing an impulse and enqueue it for transfer
        send_payload = {};
        send_payload.push_back({16'b0, 16'b0, 16'h7FFF, 16'h7FFF});
        for (int i = 0; i < NUM_COEFFS/2; i++) begin
          send_payload.push_back(0);
        end
        blk_ctrl.send(port, send_payload, NUM_COEFFS*4);

        // Enqueue two packets with zeros to push out the impulse from the
        // pipeline (one to push out the data and one to overcome some pipeline
        // registering).
        send_payload = {};
        for (int i = 0; i < NUM_COEFFS/2+1; i++) begin
          send_payload.push_back(0);
        end
        for (int n = 0; n < 2; n++) begin
          blk_ctrl.send(port, send_payload, NUM_COEFFS*4);
        end

        // Ignore the first two packets (discard the extra data we put in when
        // we checked the default coefficients).
        blk_ctrl.recv(port, recv_payload, num_bytes);
        blk_ctrl.recv(port, recv_payload, num_bytes);

        // Receive the result
        blk_ctrl.recv(port, recv_payload, num_bytes);

        // Check the length of the packet
        `ASSERT_ERROR(
          num_bytes == NUM_COEFFS*4,
          "Received packet didn't have expected length"
        );

        for (int i = 0; i < NUM_COEFFS; i++) begin
          // Compute the expected sample
          i_coeff = $signed(COEFFS_VEC_1[COEFF_WIDTH*i +: COEFF_WIDTH]);
          q_coeff = i_coeff;

          // Grab the next sample
          {i_samp, q_samp} = recv_payload[i/2][i[0]*32 +: 32];

          // Check I / Q values
          $sformat(
            s, "Incorrect I value received on sample %0d! Expected: %0d, Received: %0d", 
            i, i_coeff, i_samp);
          `ASSERT_ERROR(
            (i_samp == i_coeff) || (i_samp-1 == i_coeff) || (i_samp+1 == i_coeff), s);
          $sformat(
            s, "Incorrect Q value received on sample %0d! Expected: %0d, Received: %0d", 
            i, q_coeff, q_samp);
          `ASSERT_ERROR(
            (q_samp == q_coeff) || (q_samp-1 == q_coeff) || (q_samp+1 == q_coeff), s);
        end

        test.end_test();
      end


      //-----------------------------------------------------------------------
      // Test step response
      //-----------------------------------------------------------------------

      begin
        chdr_word_t send_payload[$];
        chdr_word_t recv_payload[$];
        int num_bytes;
        int coeff_sum;
        logic signed [15:0] i_samp, q_samp;
        string s;

        test.start_test("Test step response", 20us);

        // Generate a step function packet
        send_payload = {};
        for (int i = 0; i < NUM_COEFFS/2+1; i++) begin
          send_payload.push_back({16'h7FFF,16'h7FFF,16'h7FFF,16'h7FFF});
        end

        // Enqueue step function two times, once to fill up the pipeline and
        // another to get the actual response.
        for (int n = 0; n < 2; n++) begin
          blk_ctrl.send(port, send_payload, NUM_COEFFS*4);
        end

        // Enqueue two packets with zeros to push out the impulse from the
        // pipeline (one to push out the data and one to overcome some pipeline
        // registering).
        send_payload = {};
        for (int i = 0; i < NUM_COEFFS/2+1; i++) begin
          send_payload.push_back(0);
        end
        for (int n = 0; n < 2; n++) begin
          blk_ctrl.send(port, send_payload, NUM_COEFFS*4);
        end

        // Ignore the first two packets (discard the extra data we put in
        // during the previous test).
        for (int n = 0; n < 3; n++) begin
          blk_ctrl.recv(port, recv_payload, num_bytes);
        end

        // Receive the result
        blk_ctrl.recv(port, recv_payload, num_bytes);

        // Check the length of the packet
        `ASSERT_ERROR(
          num_bytes == NUM_COEFFS*4,
          "Received packet didn't have expected length"
        );

        // Calculate sum of all the coefficients
        coeff_sum = 0;
        for (int i = 0; i < NUM_COEFFS; i++) begin
            coeff_sum += $signed(COEFFS_VEC_1[COEFF_WIDTH*i +: COEFF_WIDTH]);
        end

        for (int i = 0; i < NUM_COEFFS; i++) begin
          // Grab the next sample
          {i_samp, q_samp} = recv_payload[i/2][i[0]*32 +: 32];

          // Check I / Q values
          $sformat(
            s, "Incorrect I value received on sample %0d! Expected: %0d, Received: %0d", 
            i, coeff_sum, i_samp);
          `ASSERT_ERROR(
            (i_samp == coeff_sum) || (i_samp-1 == coeff_sum) || (i_samp+1 == coeff_sum),
            s
          );
          $sformat(
            s, "Incorrect Q value received on sample %0d! Expected: %0d, Received: %0d", 
            i, coeff_sum, q_samp);
          `ASSERT_ERROR(
            (q_samp == coeff_sum) || (q_samp-1 == coeff_sum) || (q_samp+1 == coeff_sum),
            s
          );
        end

        test.end_test();
      end

    end : port_loop


    //-------------------------------------------------------------------------
    // All done!
    //-------------------------------------------------------------------------

    test.end_tb(1);

  end : tb_main
endmodule
