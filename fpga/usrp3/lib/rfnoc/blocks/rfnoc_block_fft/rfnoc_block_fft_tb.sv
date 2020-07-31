//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fft_tb
//
// Description:  Testbench for rfnoc_block_fft
//

module rfnoc_block_fft_tb();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;

  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Simulation parameters
  localparam real CHDR_CLK_PER   = 5.0;   // Clock rate
  localparam int  SPP            = 256;   // Samples per packet
  localparam int  PKT_SIZE_BYTES = SPP*4; // Bytes per packet
  localparam int  STALL_PROB     = 25;    // BFM stall probability

  // Block configuration
  localparam int NOC_ID        = 32'hFF70_0000;
  localparam int CHDR_W        = 64;
  localparam int ITEM_W        = 32;
  localparam int THIS_PORTID   = 'h123;
  localparam int MTU           = 10;
  localparam int NUM_PORTS     = 1;
  localparam int NUM_HB        = 3;
  localparam int CIC_MAX_DECIM = 255;

  // FFT specific settings
  // FFT settings
  localparam [31:0] FFT_SIZE         = 256;
  localparam [31:0] FFT_SIZE_LOG2    = $clog2(FFT_SIZE);
  localparam [31:0] FFT_SCALING      = 12'b011010101010;           // Conservative scaling of 1/N
  localparam [31:0] FFT_SHIFT_CONFIG = 0;                          // Normal FFT shift
  localparam FFT_BIN                 = FFT_SIZE/8 + FFT_SIZE/2;    // 1/8 sample rate freq + FFT shift
  localparam NUM_ITERATIONS  = 10;

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;

  sim_clock_gen #(CHDR_CLK_PER)  rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CHDR_CLK_PER)  rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr             (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr             (rfnoc_chdr_clk, 1'b0);

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = 
    new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  initial begin
    blk_ctrl.connect_master_data_port(0, m_chdr, PKT_SIZE_BYTES);
    blk_ctrl.connect_slave_data_port(0, s_chdr);
    blk_ctrl.set_master_stall_prob(0, STALL_PROB);
    blk_ctrl.set_slave_stall_prob(0, STALL_PROB);
  end

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  rfnoc_block_fft #(
    .THIS_PORTID            (0  ),
    .CHDR_W                 (64 ),
    .MTU                    (MTU),
    
    .EN_MAGNITUDE_OUT       (0  ),
    .EN_MAGNITUDE_APPROX_OUT(1  ),
    .EN_MAGNITUDE_SQ_OUT    (1  ),
    .EN_FFT_SHIFT           (1  )
  ) DUT (
    .rfnoc_chdr_clk     (backend.chdr_clk),
    .ce_clk             (backend.chdr_clk),
    .s_rfnoc_chdr_tdata (m_chdr.tdata    ),
    .s_rfnoc_chdr_tlast (m_chdr.tlast    ),
    .s_rfnoc_chdr_tvalid(m_chdr.tvalid   ),
    .s_rfnoc_chdr_tready(m_chdr.tready   ),
    
    .m_rfnoc_chdr_tdata (s_chdr.tdata    ),
    .m_rfnoc_chdr_tlast (s_chdr.tlast    ),
    .m_rfnoc_chdr_tvalid(s_chdr.tvalid   ),
    .m_rfnoc_chdr_tready(s_chdr.tready   ),
    
    .rfnoc_core_config  (backend.cfg     ),
    .rfnoc_core_status  (backend.sts     ),
    .rfnoc_ctrl_clk     (backend.ctrl_clk),
    
    .s_rfnoc_ctrl_tdata (m_ctrl.tdata    ),
    .s_rfnoc_ctrl_tlast (m_ctrl.tlast    ),
    .s_rfnoc_ctrl_tvalid(m_ctrl.tvalid   ),
    .s_rfnoc_ctrl_tready(m_ctrl.tready   ),
    
    .m_rfnoc_ctrl_tdata (s_ctrl.tdata    ),
    .m_rfnoc_ctrl_tlast (s_ctrl.tlast    ),
    .m_rfnoc_ctrl_tvalid(s_ctrl.tvalid   ),
    .m_rfnoc_ctrl_tready(s_ctrl.tready   )
  );

  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Translate the desired register access to a ctrlport write request.
  task automatic write_reg(int port, byte addr, bit [31:0] value);
    blk_ctrl.reg_write(256*8*port + addr*8, value);
  endtask : write_reg

  // Translate the desired register access to a ctrlport read request.
  task automatic read_user_reg(int port, byte addr, output logic [63:0] value);
    blk_ctrl.reg_read(256*8*port + addr*8 + 0, value[31: 0]);
    blk_ctrl.reg_read(256*8*port + addr*8 + 4, value[63:32]);
  endtask : read_user_reg

  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  task automatic test_sine_wave (
    input int unsigned port
  );
    test.start_test("Test sine wave", 20us);

    write_reg(port, DUT.SR_FFT_SIZE_LOG2, FFT_SIZE_LOG2);
    write_reg(port, DUT.SR_FFT_DIRECTION, DUT.FFT_FORWARD);
    write_reg(port, DUT.SR_FFT_SCALING, FFT_SCALING);
    write_reg(port, DUT.SR_FFT_SHIFT_CONFIG, FFT_SHIFT_CONFIG);
    write_reg(port, DUT.SR_MAGNITUDE_OUT, DUT.COMPLEX_OUT); // Enable real/imag out

    // Send a sine wave
    fork
      begin
        chdr_word_t send_payload[$];

        for (int n = 0; n < NUM_ITERATIONS; n++) begin
          for (int i = 0; i < (FFT_SIZE/8); i++) begin
            send_payload.push_back({ 16'h5A82, 16'h5A82, 16'h7FFF, 16'h0000});
            send_payload.push_back({-16'h5A82, 16'h5A82, 16'h0000, 16'h7FFF});
            send_payload.push_back({-16'h5A82,-16'h5A82,-16'h7FFF, 16'h0000});
            send_payload.push_back({ 16'h5A82,-16'h5A82, 16'h0000,-16'h7FFF});
          end

          blk_ctrl.send(port, send_payload);
          blk_ctrl.wait_complete(port);
          send_payload = {};
        end
      end

      begin
        string       msg;
        chdr_word_t  recv_payload[$], temp_payload[$];
        int          data_bytes;
        logic [15:0] real_val;
        logic [15:0] cplx_val;

        for (int n = 0; n < NUM_ITERATIONS; n++) begin
          blk_ctrl.recv(port, recv_payload, data_bytes);

          `ASSERT_ERROR(recv_payload.size * 2 == FFT_SIZE, "received wrong amount of data");

          for (int k = 0; k < FFT_SIZE/2; k++) begin
            chdr_word_t payload_word;
            payload_word = recv_payload.pop_front();

            for (int i = 0; i < 2; i++) begin
              {real_val, cplx_val} = payload_word;
              payload_word = payload_word[63:32];

              if (2*k+i == FFT_BIN) begin
                // Assert that for the special case of a 1/8th sample rate sine wave input, 
                // the real part of the corresponding 1/8th sample rate FFT bin should always be greater than 0 and
                // the complex part equal to 0.
                $sformat(msg, 
                  "On iteration %0d, sample %0d, FFT real part is 0x%X, expected value > 0", 
                  n, 2*k+i, real_val);
                  `ASSERT_ERROR(real_val > 32'd0, msg);
                $sformat(msg,
                  "On iteration %0d, sample %0d, FFT complex part is 0x%X, expected 0", 
                  n, 2*k+i, cplx_val);
                  `ASSERT_ERROR(cplx_val == 32'd0, msg);
              end else begin
                // Assert all other FFT bins should be 0 for both complex and real parts
                $sformat(msg, 
                  "On iteration %0d, sample %0d, FFT real part is 0x%X, expected value 0", 
                  n, 2*k+i, real_val);
                  `ASSERT_ERROR(real_val == 32'd0, msg);
                $sformat(msg,
                  "On iteration %0d, sample %0d, FFT complex part is 0x%X, expected 0", 
                  n, 2*k+i, cplx_val);
                  `ASSERT_ERROR(cplx_val == 32'd0, msg);
              end
            end
          end
        end
      end
    join

    test.end_test();
  endtask : test_sine_wave


  task automatic test_short (
    input int unsigned port
  );
    item_t samples[$], spectrum[$], recv[$];
    string msg;

    test.start_test("Test short FFT", 10us);

    write_reg(port, DUT.SR_FFT_SIZE_LOG2, 3);
    write_reg(port, DUT.SR_FFT_DIRECTION, DUT.FFT_FORWARD);
    write_reg(port, DUT.SR_FFT_SCALING, 0);         // No scaling
    write_reg(port, DUT.SR_FFT_SHIFT_CONFIG, 10);   // Bypass shifting

    // Samples to input to FFT (expected output of IFFT)
    samples = '{ 
      32'h000E_0000,  // {16'd14, 16'd0}, // Vivado won't allow concatenation
      32'h000F_0000,  // {16'd15, 16'd0}, // in dynamic types.
      32'h0010_0000,  // {16'd16, 16'd0},
      32'h0011_0000,  // {16'd17, 16'd0},
      32'h0012_0000,  // {16'd18, 16'd0},
      32'h0013_0000,  // {16'd19, 16'd0},
      32'h0014_0000,  // {16'd20, 16'd0},
      32'h0015_0000   // {16'd21, 16'd0}
    };

    // Expected spectrum output by FFT (values to input to IFFT)
    spectrum = '{ 
      32'h008C_0000,  // { 16'sd140, 16'd0},
      32'hFFFC_000A,  // {-16'sd4,   16'd10},
      32'hFFFC_0004,  // {-16'sd4,   16'd4},
      32'hFFFC_0002,  // {-16'sd4,   16'd2},
      32'hFFFC_0000,  // {-16'sd4,   16'd0},
      32'hFFFC_FFFE,  // {-16'sd4,  -16'd2},
      32'hFFFC_FFFC,  // {-16'sd4,  -16'd4},
      32'hFFFC_FFF6   // {-16'sd4,  -16'd10}
    };

    blk_ctrl.send_items(port, samples);
    blk_ctrl.recv_items(port, recv);

    foreach (recv[i]) begin
      if (recv[i] != spectrum[i]) begin
        $sformat(msg, "On sample %d, received (%d,%d), expected (%d,%d)", i,
          signed'(recv[i][31:16]), signed'(recv[i][15:0]),
          signed'(spectrum[i][31:16]), signed'(spectrum[i][15:0]));
        `ASSERT_ERROR(0, msg);
      end
    end

    test.end_test();

    test.start_test("Test short IFFT", 10us);

    write_reg(port, DUT.SR_FFT_DIRECTION, DUT.FFT_REVERSE);
    write_reg(port, DUT.SR_FFT_SCALING, 12'b11);
    blk_ctrl.send_items(port, spectrum);
    blk_ctrl.recv_items(port, recv);

    foreach (recv[i]) begin
      if (recv[i] != samples[i]) begin
        $sformat(msg, "On sample %d, received (%d,%d), expected (%d,%d)", i,
          signed'(recv[i][31:16]), signed'(recv[i][15:0]),
          signed'(samples[i][31:16]), signed'(samples[i][15:0]));
        `ASSERT_ERROR(0, msg);
      end
    end

    test.end_test();
  endtask : test_short


  task automatic test_regs (
    input int unsigned port
  );
    logic [31:0] val;

    test.start_test("Test registers", 10us);

    write_reg(port, DUT.SR_FFT_RESET, 1);
    write_reg(port, DUT.SR_FFT_RESET, 0);

    // SR_FFT_SIZE_LOG2
    read_user_reg(port, DUT.RB_FFT_SIZE_LOG2, val);
    `ASSERT_ERROR(val == DUT.DEFAULT_FFT_SIZE, "FFT_SIZE_LOG2 is incorrect");
    write_reg(port, DUT.SR_FFT_SIZE_LOG2, 32'hFFFFFFFF);
    read_user_reg(port, DUT.RB_FFT_SIZE_LOG2, val);
    `ASSERT_ERROR(val == 32'hFF, "FFT_SIZE_LOG2 is incorrect");
    write_reg(port, DUT.SR_FFT_SIZE_LOG2, 32'h0);
    read_user_reg(port, DUT.RB_FFT_SIZE_LOG2, val);
    `ASSERT_ERROR(val == 32'h0, "FFT_SIZE_LOG2 is incorrect");
    write_reg(port, DUT.SR_FFT_SIZE_LOG2, DUT.DEFAULT_FFT_SIZE);
    read_user_reg(port, DUT.RB_FFT_SIZE_LOG2, val);
    `ASSERT_ERROR(val == DUT.DEFAULT_FFT_SIZE, "FFT_SIZE_LOG2 is incorrect");

    // SR_MAGNITUDE_OUT
    read_user_reg(port, DUT.RB_MAGNITUDE_OUT, val);
    `ASSERT_ERROR(val == 32'h0, "MAGNITUDE_OUT is incorrect");
    write_reg(port, DUT.SR_MAGNITUDE_OUT, 32'hFFFFFFFF);
    read_user_reg(port, DUT.RB_MAGNITUDE_OUT, val);
    `ASSERT_ERROR(val == 32'h3, "MAGNITUDE_OUT is incorrect");
    write_reg(port, DUT.SR_MAGNITUDE_OUT, 32'h0);
    read_user_reg(port, DUT.RB_MAGNITUDE_OUT, val);
    `ASSERT_ERROR(val == 32'h0, "MAGNITUDE_OUT is incorrect");

    // SR_FFT_DIRECTION
    read_user_reg(port, DUT.RB_FFT_DIRECTION, val);
    `ASSERT_ERROR(val == DUT.DEFAULT_FFT_DIRECTION, "FFT_DIRECTION is incorrect");
    write_reg(port, DUT.SR_FFT_DIRECTION, 32'hFFFFFFFF);
    read_user_reg(port, DUT.RB_FFT_DIRECTION, val);
    `ASSERT_ERROR(val == 32'h1, "FFT_DIRECTION is incorrect");
    write_reg(port, DUT.SR_FFT_DIRECTION, 32'h0);
    read_user_reg(port, DUT.RB_FFT_DIRECTION, val);
    `ASSERT_ERROR(val == 32'h0, "FFT_DIRECTION is incorrect");
    write_reg(port, DUT.SR_FFT_DIRECTION, DUT.DEFAULT_FFT_DIRECTION);
    read_user_reg(port, DUT.RB_FFT_DIRECTION, val);
    `ASSERT_ERROR(val == DUT.DEFAULT_FFT_DIRECTION, "FFT_DIRECTION is incorrect");

    // SR_FFT_SCALING
    read_user_reg(port, DUT.RB_FFT_SCALING, val);
    `ASSERT_ERROR(val == DUT.DEFAULT_FFT_SCALING, "FFT_SCALING is incorrect");
    write_reg(port, DUT.SR_FFT_SCALING, 32'hFFFFFFFF);
    read_user_reg(port, DUT.RB_FFT_SCALING, val);
    `ASSERT_ERROR(val == 32'hFFF, "FFT_SCALING is incorrect");
    write_reg(port, DUT.SR_FFT_SCALING, 32'h0);
    read_user_reg(port, DUT.RB_FFT_SCALING, val);
    `ASSERT_ERROR(val == 32'h0, "FFT_SCALING is incorrect");
    write_reg(port, DUT.SR_FFT_SCALING, DUT.DEFAULT_FFT_SCALING);
    read_user_reg(port, DUT.RB_FFT_SCALING, val);
    `ASSERT_ERROR(val == DUT.DEFAULT_FFT_SCALING, "FFT_SCALING is incorrect");

    // SR_FFT_SHIFT_CONFIG
    read_user_reg(port, DUT.RB_FFT_SHIFT_CONFIG, val);
    `ASSERT_ERROR(val == 32'b0, "FFT_SHIFT_CONFIG is incorrect");
    write_reg(port, DUT.SR_FFT_SHIFT_CONFIG, 32'hFFFFFFFF);
    read_user_reg(port, DUT.RB_FFT_SHIFT_CONFIG, val);
    `ASSERT_ERROR(val == 32'h3, "FFT_SHIFT_CONFIG is incorrect");
    write_reg(port, DUT.SR_FFT_SHIFT_CONFIG, 32'h0);
    read_user_reg(port, DUT.RB_FFT_SHIFT_CONFIG, val);
    `ASSERT_ERROR(val == 32'h0, "FFT_SHIFT_CONFIG is incorrect");
    write_reg(port, DUT.SR_FFT_SHIFT_CONFIG, 32'b0);
    read_user_reg(port, DUT.RB_FFT_SHIFT_CONFIG, val);
    `ASSERT_ERROR(val == 32'b0, "FFT_SHIFT_CONFIG is incorrect");

    test.end_test();
  endtask : test_regs


  initial begin : tb_main
    static int port = 0;
    test.start_tb("rfnoc_block_fft_tb");

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

    //-------------------------------------------------------------------------
    // Tests
    //-------------------------------------------------------------------------

    test_regs(port);
    test_short(port);
    test_sine_wave(port);

    //-------------------------------------------------------------------------
    // Finish
    //-------------------------------------------------------------------------

    // End the TB, but don't $finish, since we don't want to kill other 
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
  end
endmodule
