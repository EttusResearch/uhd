//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_tb
//
// Description:  Testbench for rfnoc_block_duc
//


module rfnoc_block_duc_tb();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;

  `include "rfnoc_block_duc_regs.vh"


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Simulation parameters
  localparam real CHDR_CLK_PER   = 5.0;   // CHDR clock rate
  localparam real DUC_CLK_PER    = 4.0;   // DUC IP clock rate
  localparam int  EXTENDED_TEST  = 0;     // Perform a longer test
  localparam int  SPP            = 128;   // Samples per packet
  localparam int  PKT_SIZE_BYTES = SPP*4; // Bytes per packet
  localparam int  STALL_PROB     = 25;    // BFM stall probability

  // Block configuration
  localparam int CHDR_W         = 64;
  localparam int SAMP_W         = 32;
  localparam int THIS_PORTID    = 'h123;
  localparam int MTU            = 8;
  localparam int NUM_PORTS      = 1;
  localparam int NUM_HB         = 3;
  localparam int CIC_MAX_INTERP = 128;
  localparam int NOC_ID         = 32'hD0C00000;


  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CHDR_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(DUC_CLK_PER)  duc_clk_gen        (.clk(ce_clk), .rst());


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
  RfnocBlockCtrlBfm #(CHDR_W, SAMP_W) blk_ctrl =
    new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
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

  rfnoc_block_duc #(
    .THIS_PORTID    (THIS_PORTID),
    .CHDR_W         (CHDR_W),
    .NUM_PORTS      (NUM_PORTS),
    .MTU            (MTU),
    .NUM_HB         (NUM_HB),
    .CIC_MAX_INTERP (CIC_MAX_INTERP)
  ) rfnoc_block_duc_i (
    .rfnoc_chdr_clk          (backend.chdr_clk),
    .ce_clk                  (ce_clk),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready),
    .rfnoc_core_config       (backend.cfg),
    .rfnoc_core_status       (backend.sts),
    .rfnoc_ctrl_clk          (backend.ctrl_clk),
    .s_rfnoc_ctrl_tdata      (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast      (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid     (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready     (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata      (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast      (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid     (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready     (s_ctrl.tready)
  );


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Translate the desired register access to a ctrlport write request.
  task automatic write_reg(int port, byte unsigned addr, bit [31:0] value);
    blk_ctrl.reg_write(256*8*port + addr*8, value);
  endtask : write_reg


  // Translate the desired register access to a ctrlport read request.
  task automatic read_user_reg(int port, byte unsigned addr, output logic [63:0] value);
    blk_ctrl.reg_read(256*8*port + addr*8 + 0, value[31: 0]);
    blk_ctrl.reg_read(256*8*port + addr*8 + 4, value[63:32]);
  endtask : read_user_reg


  // Set the interpolation rate
  task automatic set_interp_rate(int port, int interp_rate);
    begin
      logic [7:0] cic_rate = 8'd0;
      logic [7:0] hb_enables = 2'b0;

      int _interp_rate = interp_rate;

      // Calculate which half bands to enable and whatever is left over set the CIC
      while ((_interp_rate[0] == 0) && (hb_enables < NUM_HB)) begin
        hb_enables += 1'b1;
        _interp_rate = _interp_rate >> 1;
      end

      // CIC rate cannot be set to 0
      cic_rate = (_interp_rate[7:0] == 8'd0) ? 8'd1 : _interp_rate[7:0];
      `ASSERT_ERROR(hb_enables <= NUM_HB, "Enabled halfbands may not exceed total number of half bands.");
      `ASSERT_ERROR(cic_rate > 0 && cic_rate <= CIC_MAX_INTERP,
       "CIC Interpolation rate must be positive, not exceed the max cic interpolation rate, and cannot equal 0!");

      // Setup DUC
      $display("Set interpolation to %0d", interp_rate);
      $display("- Number of enabled HBs: %0d", hb_enables);
      $display("- CIC Rate:              %0d", cic_rate);
      write_reg(port, SR_M_ADDR, interp_rate);                 // Set interpolation rate in AXI rate change
      write_reg(port, SR_INTERP_ADDR, {hb_enables, cic_rate}); // Enable HBs, set CIC rate
    end
  endtask


  // Test sending packets of ones
  task automatic send_ones(int port, int interp_rate, bit has_time);
    begin
      const bit [63:0] start_time = 64'h0123456789ABCDEF;

      set_interp_rate(port, interp_rate);

      // Setup DUC
      write_reg(port, SR_CONFIG_ADDR, 32'd1);         // Enable clear EOB
      write_reg(port, SR_FREQ_ADDR, 32'd0);           // CORDIC phase increment
      write_reg(port, SR_SCALE_IQ_ADDR, (1 << 14));   // Scaling, set to 1

      fork
        begin
          chdr_word_t send_payload[$];
          packet_info_t pkt_info;

          $display("Send ones");

          // Generate a payload of all ones
          send_payload = {};
          for (int i = 0; i < PKT_SIZE_BYTES/8; i++) begin
            send_payload.push_back({16'hffff, 16'hffff, 16'hffff, 16'hffff});
          end

          // Send two packets with EOB on the second packet
          pkt_info = 0;
          pkt_info.has_time  = has_time;
          pkt_info.timestamp = start_time;
          blk_ctrl.send_packets(port, send_payload, /*data_bytes*/, /*metadata*/, pkt_info);
          pkt_info.timestamp = start_time + SPP;
          pkt_info.eob = 1;
          blk_ctrl.send_packets(port, send_payload, /*data_bytes*/, /*metadata*/, pkt_info);
          
          $display("Send ones complete");
        end
        begin
          string s;
          chdr_word_t samples;
          int data_bytes;
          chdr_word_t recv_payload[$];
          chdr_word_t metadata[$];
          packet_info_t pkt_info;

          $display("Check incoming samples");
          for (int i = 0; i < 2*interp_rate; i++) begin
            blk_ctrl.recv_adv(port, recv_payload, data_bytes, metadata, pkt_info);

            // Check the packet size
            $sformat(s, "incorrect (drop) packet size! expected: %0d, actual: %0d", PKT_SIZE_BYTES/8, recv_payload.size());
            `ASSERT_ERROR(recv_payload.size() == PKT_SIZE_BYTES/8, s);

            // Check the timestamp
            if (has_time) begin
              bit [63:0] expected_time;
              // Calculate what the timestamp should be
              expected_time = start_time + i * SPP;
              $sformat(s, "Incorrect timestamp: has_time = %0d, timestamp = 0x%0X, expected 0x%0X",
                       pkt_info.has_time, pkt_info.timestamp, expected_time);
              `ASSERT_ERROR(pkt_info.has_time == 1 && pkt_info.timestamp == expected_time, s);
            end else begin
              `ASSERT_ERROR(pkt_info.has_time == 0, "Packet has timestamp when it shouldn't");
            end

            // Check EOB
            if (i == 2*interp_rate-1) begin
              `ASSERT_ERROR(pkt_info.eob == 1, "EOB not set on last packet");
            end else begin
              `ASSERT_ERROR(pkt_info.eob == 0, 
                $sformatf("EOB unexpectedly set on packet %0d", i));
            end

            // Check the sample values
            samples = 64'd0;
            for (int j = 0; j < PKT_SIZE_BYTES/8; j++) begin
              samples = recv_payload[j];
              $sformat(s, "Ramp word %0d invalid! Expected a real value, Received: %0d", 2*j, samples);
              `ASSERT_ERROR(samples >= 0, s);
            end
          end
          $display("Check complete");
        end
      join
    end
  endtask


  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    const int port = 0;
    test.start_tb("rfnoc_block_duc_tb");

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
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU value");
    test.end_test();


    //-------------------------------------------------------------------------
    // Test read-back regs
    //-------------------------------------------------------------------------

    begin
      logic [63:0] val64;
      test.start_test("Test registers", 10us);
      read_user_reg(port, RB_NUM_HB, val64);
      `ASSERT_ERROR(val64 == NUM_HB, "Register NUM_HB didn't read back expected value");
      read_user_reg(port, RB_CIC_MAX_INTERP, val64);
      `ASSERT_ERROR(val64 ==CIC_MAX_INTERP, "Register RB_CIC_MAX_INTERP didn't read back expected value");
      test.end_test();
    end


    //-------------------------------------------------------------------------
    // Test various interpolation rates (no timestamp)
    //-------------------------------------------------------------------------

    begin
      test.start_test("Test interpolation rates (with timestamp)", 0.5ms);

      $display("Note: This test will take a long time!");
      send_ones(port, 1,  1);   // HBs enabled: 0, CIC rate: 1
      send_ones(port, 2,  1);   // HBs enabled: 1, CIC rate: 1
      send_ones(port, 3,  1);   // HBs enabled: 0, CIC rate: 3
      send_ones(port, 4,  1);   // HBs enabled: 2, CIC rate: 1
      send_ones(port, 6,  1);   // HBs enabled: 1, CIC rate: 3
      send_ones(port, 8,  1);   // HBs enabled: 2, CIC rate: 2
      send_ones(port, 12, 1);   // HBs enabled: 2, CIC rate: 3
      send_ones(port, 13, 1);   // HBs enabled: 0, CIC rate: 13
      send_ones(port, 16, 1);   // HBs enabled: 2, CIC rate: 3
      send_ones(port, 40, 1);   // HBs enabled: 2, CIC rate: 20

      test.end_test();
    end


    //-------------------------------------------------------------------------
    // Test various interpolation rates (without timestamp)
    //-------------------------------------------------------------------------

    begin
      test.start_test("Test interpolation rates (no timestamp)", 0.5ms);

      send_ones(port, 1,  0);   // HBs enabled: 0, CIC rate: 1
      send_ones(port, 3,  0);   // HBs enabled: 0, CIC rate: 3

      test.end_test();
    end


    //-------------------------------------------------------------------------
    // Test timed tune
    //-------------------------------------------------------------------------

    // This test has not been implemented because the RFNoC FFT has not been
    // ported yet.


    //-------------------------------------------------------------------------
    // Finish
    //-------------------------------------------------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    duc_clk_gen.kill();
  end
endmodule
