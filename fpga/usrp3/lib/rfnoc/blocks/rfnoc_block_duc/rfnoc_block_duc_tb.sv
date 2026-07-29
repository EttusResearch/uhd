//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_tb
//
// Description:
//   Connectivity smoke test for the rfnoc_block_duc top-level wrapper.
//
//   Verifies that the wrapper connects the backend, CHDR, and CTRL interfaces
//   through the selected DUC implementation. The test is instantiated with
//   parameters that exercise both generate branches.
//
// Parameters:
//   CHDR_W: CHDR word width
//   NIPC:   Number of items processed per clock cycle
//

module rfnoc_block_duc_tb #(
  parameter int CHDR_W = 64,
  parameter int NIPC   = 1
) ();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrData::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  localparam real CHDR_CLK_PER = 5.0;
  localparam real CE_CLK_PER   = 4.0;

  localparam int SAMP_W          = 32;
  localparam int THIS_PORTID     = 'h123;
  localparam int MTU             = 8;
  localparam int NUM_PORTS       = 1;
  localparam int NUM_HB          = 3;
  localparam int CIC_MAX_INTERP  = 255;
  localparam int NOC_ID          = 32'hD0C00000;
  localparam int PKT_SIZE_BYTES  = 256 * (SAMP_W/8);

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CHDR_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER)   ce_clk_gen         (.clk(ce_clk),         .rst());

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  RfnocBlockCtrlBfm #(CHDR_W, SAMP_W) blk_ctrl =
    new(backend, m_ctrl, s_ctrl);

  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_master_stall_prob(i, 0);
      blk_ctrl.set_slave_stall_prob(i, 0);
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

  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_dut_connections
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];

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
    .CIC_MAX_INTERP (CIC_MAX_INTERP),
    .NIPC           (NIPC)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .ce_clk              (ce_clk),
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
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready)
  );

  // Hold the multisample DDS in reset during simulator startup.
  if (NIPC > 1) begin : gen_startup_reset
    for (genvar port = 0; port < NUM_PORTS; port++) begin : gen_port_reset
      initial begin
        force dut.gen_ms.rfnoc_block_duc_ms_i
          .gen_multisample_chains[port].rfnoc_block_duc_ms_channel_i
          .duc_dds_ms_i.rst_stretch = 1'b1;
        wait (dut.gen_ms.rfnoc_block_duc_ms_i
          .gen_multisample_chains[port].rfnoc_block_duc_ms_channel_i
          .duc_dds_ms_i.rst);
        release dut.gen_ms.rfnoc_block_duc_ms_i
          .gen_multisample_chains[port].rfnoc_block_duc_ms_channel_i
          .duc_dds_ms_i.rst_stretch;
      end
    end
  end

  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    test.start_tb($sformatf("rfnoc_block_duc_tb (NIPC=%0d)", NIPC));

    blk_ctrl.run();

    test.start_test("Wait for Reset", 10us);
    fork
      blk_ctrl.reset_chdr();
      blk_ctrl.reset_ctrl();
    join
    test.end_test();

    test.start_test("Verify Block Info", 10us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id()     == NOC_ID,    "Incorrect NOC_ID");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O");
    `ASSERT_ERROR(blk_ctrl.get_mtu()        == MTU,       "Incorrect MTU");
    test.end_test();

    test.end_tb(0);

    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    ce_clk_gen.kill();
  end

endmodule
