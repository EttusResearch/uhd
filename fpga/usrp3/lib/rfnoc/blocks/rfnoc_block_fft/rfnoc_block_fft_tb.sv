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
  localparam int CHDR_W        = 64;
  localparam int THIS_PORTID   = 'h123;
  localparam int MTU           = 10;
  localparam int NUM_PORTS     = 1;
  localparam int NUM_HB        = 3;
  localparam int CIC_MAX_DECIM = 255;

  // FFT specific settings
  // FFT settings
  localparam [31:0] FFT_SIZE         = 256;
  localparam [31:0] FFT_SIZE_LOG2    = $clog2(FFT_SIZE);
  const logic [31:0] FFT_DIRECTION    = DUT.FFT_FORWARD;  // Forward
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

  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr             (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr             (rfnoc_chdr_clk, 1'b0);

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(.CHDR_W(CHDR_W)) blk_ctrl = 
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

  task automatic send_sine_wave (
    input int unsigned port
  );
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
        string s;
        chdr_word_t   recv_payload[$], temp_payload[$];
        int           data_bytes;
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
                
                `ASSERT_ERROR(real_val > 32'd0, "FFT bin real part is not greater than 0!");
                `ASSERT_ERROR(cplx_val == 32'd0, "FFT bin complex part is not 0!");
              end else begin
                // Assert all other FFT bins should be 0 for both complex and real parts
                `ASSERT_ERROR(real_val == 32'd0, "FFT bin real part is not 0!");
                `ASSERT_ERROR(cplx_val == 32'd0, "FFT bin complex part is not 0!");
              end
            end
          end
        end
      end
    join
  endtask

  initial begin : tb_main
    const int port = 0;
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
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == DUT.NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();

    //-------------------------------------------------------------------------
    // Setup FFT
    //-------------------------------------------------------------------------

    test.start_test("Setup FFT", 10us);
    write_reg(port, DUT.SR_FFT_SIZE_LOG2, FFT_SIZE_LOG2);
    write_reg(port, DUT.SR_FFT_DIRECTION, FFT_DIRECTION);
    write_reg(port, DUT.SR_FFT_SCALING, FFT_SCALING);
    write_reg(port, DUT.SR_FFT_SHIFT_CONFIG, FFT_SHIFT_CONFIG);
    write_reg(port, DUT.SR_MAGNITUDE_OUT, DUT.COMPLEX_OUT); // Enable real/imag out
    test.end_test();

    //-------------------------------------------------------------------------76
    // Test sine wave
    //-------------------------------------------------------------------------

    test.start_test("Test sine wave", 20us);
    send_sine_wave (port);
    test.end_test();

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
