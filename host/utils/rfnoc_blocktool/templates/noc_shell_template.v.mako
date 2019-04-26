<%!
import math
%>
//
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_${config['module_name']}
// Description:
//
// Parameters:
//
// Signals:

module noc_shell_${config['module_name']} #(
  parameter CHDR_W = 64,
  parameter [9:0] THIS_PORTID = 10'd0,
  parameter [5:0] MTU = 0
)(
  // Framework Interface
  //------------------------------------------------------------
  // RFNoC Framework Clocks and Resets
%for clock in config['clocks']:
  input  wire ${clock['name']}_clk,
  ${"output" if clock['name'] in ["rfnoc_chdr", "rfnoc_ctrl"] else "input"} wire ${clock['name']}_rst,
% endfor

  // RFNoC Backend Interface
  input  wire [511:0]          rfnoc_core_config,
  output wire [511:0]          rfnoc_core_status,
  <%
  num_inputs = len(config['data']['inputs'])
  num_outputs = len(config['data']['outputs'])
  %>
  // CHDR Input Ports (from framework)
  input  wire [(${num_inputs}*CHDR_W)-1:0] s_rfnoc_chdr_tdata,
  input  wire [${num_inputs-1}:0]            s_rfnoc_chdr_tlast,
  input  wire [${num_inputs-1}:0]            s_rfnoc_chdr_tvalid,
  output wire [${num_inputs-1}:0]            s_rfnoc_chdr_tready,

  // CHDR Output Ports (to framework)
  output wire [(${num_outputs}*CHDR_W)-1:0] m_rfnoc_chdr_tdata,
  output wire [${num_outputs-1}:0]            m_rfnoc_chdr_tlast,
  output wire [${num_outputs-1}:0]            m_rfnoc_chdr_tvalid,
  input  wire [${num_outputs-1}:0]            m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]           s_rfnoc_ctrl_tdata,
  input  wire                  s_rfnoc_ctrl_tlast,
  input  wire                  s_rfnoc_ctrl_tvalid,
  output wire                  s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]           m_rfnoc_ctrl_tdata,
  output wire                  m_rfnoc_ctrl_tlast,
  output wire                  m_rfnoc_ctrl_tvalid,
  input  wire                  m_rfnoc_ctrl_tready,

  // Client Interface
  //------------------------------------------------------------
  output wire               ctrlport_clk,
  output wire               ctrlport_rst,

%if config['control']['fpga_iface'] == "ctrlport":
<%include file="modules/ctrlport_wires_template.mako" args="mode='shell'"/>
%elif config['control']['fpga_iface'] == "axis_ctrl":
<%include file="modules/axis_ctrl_wires_template.mako" args="mode='shell'"/>
%else:
<%include file="control wires template.mako"/>
%endif

  output wire               axis_data_clk,
  output wire               axis_data_rst,

%if config['data']['fpga_iface'] == "axis_chdr":
  <%include file="modules/axis_chdr_wires_template.mako" args="mode='shell', num_inputs=num_inputs, num_outputs=num_outputs"/>
%elif config['data']['fpga_iface'] == "axis_rawdata":
  <%include file="modules/axis_raw_wires_template.mako" args="mode='shell', num_inputs=num_inputs, num_outputs=num_outputs"/>
%else:
  <%include file="data wires template.mako"/>
%endif
);

  // ---------------------------------------------------
  //  Backend Interface
  // ---------------------------------------------------
  wire         data_i_flush_en;
  wire [31:0]  data_i_flush_timeout;
  wire [63:0]  data_i_flush_active;
  wire [63:0]  data_i_flush_done;
  wire         data_o_flush_en;
  wire [31:0]  data_o_flush_timeout;
  wire [63:0]  data_o_flush_active;
  wire [63:0]  data_o_flush_done;


  backend_iface #(
    .NOC_ID(32'h${format(config['noc_id'], "08X")}),
    .NUM_DATA_I(${num_inputs}),
    .NUM_DATA_O(${num_outputs}),
    .CTRL_FIFOSIZE(${math.ceil(math.log2(config['control']['fifo_depth']))}),
    .MTU(MTU)
  ) backend_iface_i (
    .rfnoc_chdr_clk(rfnoc_chdr_clk),
    .rfnoc_chdr_rst(rfnoc_chdr_rst),
    .rfnoc_ctrl_clk(rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst(rfnoc_ctrl_rst),
    .rfnoc_core_config(rfnoc_core_config),
    .rfnoc_core_status(rfnoc_core_status),
    .data_i_flush_en(data_i_flush_en),
    .data_i_flush_timeout(data_i_flush_timeout),
    .data_i_flush_active(data_i_flush_active),
    .data_i_flush_done(data_i_flush_done),
    .data_o_flush_en(data_o_flush_en),
    .data_o_flush_timeout(data_o_flush_timeout),
    .data_o_flush_active(data_o_flush_active),
    .data_o_flush_done(data_o_flush_done)
  );

  // ---------------------------------------------------
  //  Control Path
  // ---------------------------------------------------

  assign ctrlport_clk = ${config['control']['clk_domain']}_clk;
  assign ctrlport_rst = ${config['control']['clk_domain']}_rst;

%if config['control']['fpga_iface'] == "axis_ctrl":
<%include file="modules/axis_ctrl_modules_template.mako"/>
%elif config['control']['fpga_iface'] == "ctrlport":
<%include file="modules/ctrlport_modules_template.mako"/>
%else:
<%include file="control module template.mako"/>
%endif

  // ---------------------------------------------------
  //  Data Path
  // ---------------------------------------------------
  assign axis_data_clk = ${config['data']['clk_domain']}_clk;
  assign axis_data_rst = ${config['data']['clk_domain']}_rst;
%if config['data']['fpga_iface'] == "axis_rawdata":
<%include file="modules/axis_raw_modules_template.mako"/>
%elif config['data']['fpga_iface'] == "axis_chdr":
<%include file="modules/axis_chdr_modules_template.mako"/>
%else:
<%include file="data module template.mako"/>
%endif

endmodule // noc_shell_${config['module_name']}
