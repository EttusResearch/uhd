<%namespace name="func" file="/functions.mako"/>\
//
// Copyright ${year} Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_${config['module_name']}_tb
//
// Description: Testbench for the ${config['module_name']} RFNoC block.
//

`default_nettype none

<%
    has_inputs = 'inputs' in config.get('data', {})
    has_outputs = 'outputs' in config.get('data', {})
%>
module rfnoc_block_${config['module_name']}_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'h${format(config['noc_id'], "08X")};
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = ${'{:<4}'.format(str(config['chdr_width'])+';')}   // CHDR size in bits
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
%if 'parameters' in config:
%for param, value in config['parameters'].items():
  localparam int    ${'{:<15}'.format(param)} = ${value};
% endfor
%endif
  localparam int    NUM_PORTS_I     = ${func.num_ports_in_str()};
  localparam int    NUM_PORTS_O     = ${func.num_ports_out_str()};
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
  localparam real   ${'{:<15}'.format(clock['name'].upper()+'_CLK_PER')} = 4.0;   // 250 MHz
  %endif
%endfor

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
  bit ${clock['name']}_clk;
  %endif
%endfor

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
  sim_clock_gen #(${clock['name'].upper()}_CLK_PER) ${clock['name']}_clk_gen (.clk(${clock['name']}_clk), .rst());
  %endif
%endfor

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

%if has_inputs:
  // AXIS-CHDR Interfaces
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS_I] (rfnoc_chdr_clk, 1'b0);
%endif
%if has_outputs:
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS_O] (rfnoc_chdr_clk, 1'b0);
%endif

%if has_inputs or has_outputs:
  // Block Controller BFM
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // CHDR word and item/sample data types
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;
%else:
  // Block Controller BFM
  RfnocBlockCtrlBfmCtrlOnly blk_ctrl = new(backend, m_ctrl, s_ctrl);
%endif

%if has_inputs or has_outputs:
  // Connect block controller to BFMs
%endif
%if has_inputs:
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_bfm_input_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
    end
  end
%endif
%if has_outputs:
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_bfm_output_connections
    initial begin
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end
%endif

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

%if has_inputs:
  // DUT Slave (Input) Port Signals
  logic [CHDR_W*NUM_PORTS_I-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tready;
%endif

%if has_outputs:
  // DUT Master (Output) Port Signals
  logic [CHDR_W*NUM_PORTS_O-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tready;
%endif

%if has_inputs or has_outputs:
  // Map the array of BFMs to a flat vector for the DUT connections
%endif
%if has_inputs:
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_dut_input_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];
  end
%endif
%if has_outputs:
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_dut_output_connections
    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end
%endif

  rfnoc_block_${config['module_name']} #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU)${"," if ('parameters' in config) else ""}
%if 'parameters' in config:
<% param_count = 1 %>\
%for param, value in config['parameters'].items():
    .${'{:<19}'.format(param)} (${param})${',' if param_count < len(config['parameters']) else ''}
<% param_count = param_count+1 %>\
% endfor
%endif
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
    .${'{:<19}'.format(clock['name']+'_clk')} (${clock['name']}_clk),
  %endif
%endfor
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
%if has_inputs:
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
%endif
%if has_outputs:
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
%endif
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
    test.start_tb("rfnoc_block_${config['module_name']}_tb");

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

    // <Add your test code here>
    test.start_test("<Name your first test>", 100us);
    `ASSERT_WARNING(0, "This testbench doesn't test anything yet!");
    test.end_test();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_${config['module_name']}_tb


`default_nettype wire
