//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_${config['module_name']}_tb
//

`default_nettype none


module rfnoc_block_${config['module_name']}_tb;

  // Simulation Timing
  timeunit 1ns;
  timeprecision 1ps;

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  // Parameters
  localparam [9:0]  THIS_PORTID  = 10'h17;
  localparam [15:0] THIS_EPID    = 16'hDEAD;
  localparam int    CHDR_W       = 64;
  localparam int    SPP          = 201;
  localparam int    LPP          = ((SPP+1)/2);
  localparam int    NUM_PKTS     = 50;

  localparam int    PORT_SRCSNK  = 0;
  localparam int    PORT_LOOP    = 1;

  //adjust clocks to testbench needs
  localparam int    CHDR_CLK_PER = 5; // 200 MHz
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
  localparam int    ${clock['name'].upper()}_CLK_PER = 5; // 200 MHz
  %endif
%endfor

  // Clock and Reset Definition
  bit rfnoc_chdr_clk;
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
  bit ${clock['name']}_clk;
  %endif
%endfor

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
  sim_clock_gen #(${clock['name'].upper()}_CLK_PER) ${clock['name']}_clk_gen (.clk(${clock['name']}_clk), .rst());
  %endif
%endfor

  // ----------------------------------------
  // Instantiate DUT
  // ----------------------------------------

  // Connections to DUT as interfaces:
  RfnocBackendIf        backend (rfnoc_chdr_clk, rfnoc_chdr_clk); // Required backend iface
  AxiStreamIf #(32)     m_ctrl  (rfnoc_chdr_clk);                 // Required control iface
  AxiStreamIf #(32)     s_ctrl  (rfnoc_chdr_clk);                 // Required control iface
  AxiStreamIf #(CHDR_W) m0_chdr (rfnoc_chdr_clk);                 // Optional data iface
  AxiStreamIf #(CHDR_W) m1_chdr (rfnoc_chdr_clk);                 // Optional data iface
  AxiStreamIf #(CHDR_W) s0_chdr (rfnoc_chdr_clk);                 // Optional data iface
  AxiStreamIf #(CHDR_W) s1_chdr (rfnoc_chdr_clk);                 // Optional data iface

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(.CHDR_W(CHDR_W)) blk_ctrl;

  // DUT
  rfnoc_block_${config['module_name']} #(
    .THIS_PORTID        (THIS_PORTID),
    .CHDR_W             (CHDR_W),
    .MTU                (10)
  ) dut (
    .rfnoc_chdr_clk     (backend.chdr_clk),
    .rfnoc_ctrl_clk     (backend.ctrl_clk),
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
    .${clock['name']}_clk(${clock['name']}_clk_gen.clk),
    .${clock['name']}_rst(${clock['name']}_clk_gen.rst),
  %endif
%endfor
    .rfnoc_core_config  (backend.cfg),
    .rfnoc_core_status  (backend.sts),
    .s_rfnoc_chdr_tdata ({m1_chdr.tdata  , m0_chdr.tdata  }),
    .s_rfnoc_chdr_tlast ({m1_chdr.tlast  , m0_chdr.tlast  }),
    .s_rfnoc_chdr_tvalid({m1_chdr.tvalid , m0_chdr.tvalid }),
    .s_rfnoc_chdr_tready({m1_chdr.tready , m0_chdr.tready }),
    .m_rfnoc_chdr_tdata ({s1_chdr.tdata , s0_chdr.tdata }),
    .m_rfnoc_chdr_tlast ({s1_chdr.tlast , s0_chdr.tlast }),
    .m_rfnoc_chdr_tvalid({s1_chdr.tvalid, s0_chdr.tvalid}),
    .m_rfnoc_chdr_tready({s1_chdr.tready, s0_chdr.tready}),
    .s_rfnoc_ctrl_tdata (m_ctrl.tdata  ),
    .s_rfnoc_ctrl_tlast (m_ctrl.tlast  ),
    .s_rfnoc_ctrl_tvalid(m_ctrl.tvalid ),
    .s_rfnoc_ctrl_tready(m_ctrl.tready ),
    .m_rfnoc_ctrl_tdata (s_ctrl.tdata ),
    .m_rfnoc_ctrl_tlast (s_ctrl.tlast ),
    .m_rfnoc_ctrl_tvalid(s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready(s_ctrl.tready)
  );

  // ----------------------------------------
  // Test Process
  // ----------------------------------------
  TestExec test;
  initial begin
    // Shared Variables
    // ----------------------------------------
    timeout_t    timeout;
    ctrl_word_t  rvalue;
    rvalue = 0;

    // Initialize
    // ----------------------------------------
    test = new("noc_block_${config['module_name']}_tb.v");
    test.start_tb();

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    test.end_tb();
  end

endmodule
