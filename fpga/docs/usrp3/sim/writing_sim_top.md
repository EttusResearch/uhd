# Writing a Top-level Simulation Module

The top-level simulation module will instantiate the DUT and implement
self-checking behavior. Testbenches can be written in any language
(SystemVerilog, Verilog, VHDL) but to take advantage of our repository of
simulation libraries, it is recommended that SystemVerilog be used.

An example testbench is shown below. This example is for an RFNoC block. If
creating a testbench for RFNoC, it is recommended to use the RFNoC ModTool to
generate a testbench template for your RFNoC block.

See \ref md_usrp3_sim_simulation_libraries for documentation on the simulation
libraries used in this example.

    //
    // Copyright 2020 Ettus Research, A National Instruments Company
    //
    // SPDX-License-Identifier: LGPL-3.0-or-later
    //
    // Module: rfnoc_block_example_tb
    //
    // Description: An example top-level testbench
    //

    `default_nettype none


    module rfnoc_block_example_tb;

      `include "test_exec.svh"

      import PkgTestExec::*;
      import PkgChdrUtils::*;
      import PkgRfnocBlockCtrlBfm::*;

      //---------------------------------------------------------------------------
      // Testbench Configuration
      //---------------------------------------------------------------------------

      localparam [ 9:0] THIS_PORTID     = 10'h123;
      localparam int    CHDR_W          = 64;
      localparam int    ITEM_W          = 32;
      localparam int    NUM_PORTS_I     = 1;
      localparam int    NUM_PORTS_O     = 1;
      localparam int    MTU             = 13;
      localparam int    SPP             = 64;
      localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
      localparam int    STALL_PROB      = 25;      // Default BFM stall probability
      localparam real   CHDR_CLK_PER    = 5.0;     // 200 MHz
      localparam real   CTRL_CLK_PER    = 25.0;    // 40 MHz

      //---------------------------------------------------------------------------
      // Clocks and Resets
      //---------------------------------------------------------------------------

      bit rfnoc_chdr_clk;
      bit rfnoc_ctrl_clk;

      sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
      sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());

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

      rfnoc_block_example #(
        .THIS_PORTID         (THIS_PORTID),
        .CHDR_W              (CHDR_W),
        .MTU                 (MTU)
      ) dut (
        .rfnoc_chdr_clk      (rfnoc_chdr_clk),
        .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
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
      // Main Test Process
      //---------------------------------------------------------------------------
      
      initial begin : tb_main

        // Initialize the test exec object for this testbench
        test.start_tb("example");

        // Start the BFMs running
        //blk_ctrl.run();

        //--------------------------------
        // Reset
        //--------------------------------

        test.start_test("Flush block then reset it", 10us);
        //blk_ctrl.flush_and_reset();
        test.end_test();

        //--------------------------------
        // Test Sequences
        //--------------------------------

        test.start_test("This is an example test", 10us);
        // < Add test code here >
        test.end_test();
        
        //--------------------------------
        // Finish Up
        //--------------------------------

        // Display final statistics and results
        test.end_tb();
      end : tb_main

    endmodule : rfnoc_block_example_tb


    `default_nettype wire


