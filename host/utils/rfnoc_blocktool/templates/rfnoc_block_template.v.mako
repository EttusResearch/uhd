<%!
import math
%>
//
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_${config['module_name']}
// Description:
//
// Parameters:
//
// Signals:

<%
num_inputs = len(config['data']['inputs'])
num_outputs = len(config['data']['outputs'])
%>

module rfnoc_block_${config['module_name']} #(
  parameter  [9:0] THIS_PORTID = 10'd0,
  parameter        CHDR_W      = 64,
  parameter  [5:0] MTU         = ${math.ceil(math.log2(config['data']['mtu']))}
)(
  // RFNoC Framework Clocks and Resets
%for clock in config['clocks']:
  input  wire                   ${clock['name']}_clk,
  %if not clock['name'] in ["rfnoc_chdr", "rfnoc_ctrl"]:
  input  wire                   ${clock['name']}_rst,
  %endif
%endfor
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // 2 CHDR Input Ports (from framework)
  input  wire [(CHDR_W*${num_inputs})-1:0]  s_rfnoc_chdr_tdata,
  input  wire [1:0]             s_rfnoc_chdr_tlast,
  input  wire [1:0]             s_rfnoc_chdr_tvalid,
  output wire [1:0]             s_rfnoc_chdr_tready,
  // 2 CHDR Output Ports (to framework)
  output wire [(CHDR_W*${num_outputs})-1:0]  m_rfnoc_chdr_tdata,
  output wire [1:0]             m_rfnoc_chdr_tlast,
  output wire [1:0]             m_rfnoc_chdr_tvalid,
  input  wire [1:0]             m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready
);

%for clock in config['clocks']:
  %if clock['name'] in ["rfnoc_chdr", "rfnoc_ctrl"]:
  wire ${clock['name']}_rst;
  %endif
%endfor

  wire               ctrlport_clk;
  wire               ctrlport_rst;

  wire               axis_data_clk;
  wire               axis_data_rst;

%if config['control']['fpga_iface'] == "ctrlport":
<%include file="modules/ctrlport_wires_template.mako" args="mode='block'"/>
%elif config['control']['fpga_iface'] == "axis_ctrl":
<%include file="modules/axis_ctrl_wires_template.mako" args="mode='block'"/>
%else:
<%include file="control wires template.mako"/>
%endif

%if config['data']['fpga_iface'] == "axis_chdr":
<%include file="modules/axis_chdr_wires_template.mako" args="mode='block', num_inputs=num_inputs, num_outputs=num_outputs"/>
%elif config['data']['fpga_iface'] == "axis_rawdata":
<%include file="modules/axis_raw_wires_template.mako" args="mode='block', num_inputs=num_inputs, num_outputs=num_outputs"/>
%else:
<%include file="data wires template.mako"/>
%endif

//NoC Shell
noc_shell_${config['module_name']} #(
  .CHDR_W      (CHDR_W),
  .THIS_PORTID (THIS_PORTID),
  .MTU         (MTU)
) noc_shell (
%for clock in config['clocks']:
  .${clock['name']}_clk(${clock['name']}_clk),
  .${clock['name']}_rst(${clock['name']}_rst),
%endfor

  // RFNoC Backend Interface
  .rfnoc_core_config(rfnoc_core_config),
  .rfnoc_core_status(rfnoc_core_status),

  // CHDR Input Ports (from framework)
  .s_rfnoc_chdr_tdata(s_rfnoc_chdr_tdata),
  .s_rfnoc_chdr_tlast(s_rfnoc_chdr_tlast),
  .s_rfnoc_chdr_tvalid(s_rfnoc_chdr_tvalid),
  .s_rfnoc_chdr_tready(s_rfnoc_chdr_tready),

  // CHDR Output Ports (to framework)
  .m_rfnoc_chdr_tdata(m_rfnoc_chdr_tdata),
  .m_rfnoc_chdr_tlast(m_rfnoc_chdr_tlast),
  .m_rfnoc_chdr_tvalid(m_rfnoc_chdr_tvalid),
  .m_rfnoc_chdr_tready(m_rfnoc_chdr_tready),
  // AXIS-Ctrl Input Port (from framework)
  .s_rfnoc_ctrl_tdata(s_rfnoc_ctrl_tdata),
  .s_rfnoc_ctrl_tlast(s_rfnoc_ctrl_tlast),
  .s_rfnoc_ctrl_tvalid(s_rfnoc_ctrl_tvalid),
  .s_rfnoc_ctrl_tready(s_rfnoc_ctrl_tready),
  // AXIS-Ctrl Output Port (to framework)
  .m_rfnoc_ctrl_tdata(m_rfnoc_ctrl_tdata),
  .m_rfnoc_ctrl_tlast(m_rfnoc_ctrl_tlast),
  .m_rfnoc_ctrl_tvalid(m_rfnoc_ctrl_tvalid),
  .m_rfnoc_ctrl_tready(m_rfnoc_ctrl_tready),

  // Client Interface
  //------------------------------------------------------------
  .ctrlport_clk(ctrlport_clk),
  .ctrlport_rst(ctrlport_rst),

%if config['control']['fpga_iface'] == "ctrlport":
<%include file="modules/ctrlport_connect_template.mako"/>\
%elif config['control']['fpga_iface'] == "axis_ctrl":
<%include file="modules/axis_ctrl_connect_template.mako"/>\
%else:
<%include file="control connect template.mako"/>\
%endif

  .axis_data_clk(axis_data_clk),
  .axis_data_rst(axis_data_rst),

%if config['data']['fpga_iface'] == "axis_chdr":
<%include file="modules/axis_chdr_connect_template.mako" args="num_inputs=num_inputs, num_outputs=num_outputs"/>\
%elif config['data']['fpga_iface'] == "axis_rawdata":
<%include file="modules/axis_raw_connect_template.mako" args="num_inputs=num_inputs, num_outputs=num_outputs"/>\
%else:
<%include file="data connect template.mako"/>\
%endif
);


//user code goes here

endmodule // rfnoc_block_${config['module_name']}
