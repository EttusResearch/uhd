<%!
import math
%>\
<%namespace name="func" file="/functions.mako"/>\
//
// Copyright ${year} Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_${config['module_name']}
//
// Description:
//
//   This is a tool-generated NoC-shell for the ${config['module_name']} block.
//   See the RFNoC specification for more information about NoC shells.
//
// Parameters:
//
//   THIS_PORTID  : Control crossbar port to which this block is connected
//   CHDR_W       : AXIS-CHDR data bus width
//   CTRL_CLK_IDX : The index of the control clock for this block. This is used
//                  to populate the backend interface, from where UHD can query
//                  the clock index and thus auto-deduct which clock is used.
//   TB_CLK_IDX   : The index of the timebase clock for this block. This is used
//                  to populate the backend interface, from where UHD can query
//                  the clock index and thus auto-deduct which clock is used.
//   MTU          : Maximum transmission unit (i.e., maximum packet size in
//                  CHDR words is 2**MTU).
//

`default_nettype none


module noc_shell_${config['module_name']} #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] CTRL_CLK_IDX    = 6'h3F,
  parameter [5:0] TB_CLK_IDX      = 6'h3F,
  parameter [5:0] MTU             = 10${"," if ('parameters' in config) else ""}
%if 'parameters' in config:
<% param_count = 1 %>\
%for param, value in config['parameters'].items():
  parameter       ${'{:<15}'.format(param)} = ${value}${',' if param_count < len(config['parameters']) else ''}
<% param_count = param_count+1 %>\
% endfor
%endif
) (
  //---------------------
  // Framework Interface
  //---------------------

  // RFNoC Framework Clocks
%for clock in config['clocks']:
  input  wire ${clock['name']}_clk,
% endfor

  // NoC Shell Generated Resets
%for clock in config['clocks']:
  output wire ${clock['name']}_rst,
% endfor

<%
    has_inputs = 'inputs' in config.get('data', {})
    has_outputs = 'outputs' in config.get('data', {})
%>\
%if has_inputs:
  // AXIS-CHDR Input Ports (from framework)
  input  wire [(${func.num_ports_in_str()})*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(${func.num_ports_in_str()})-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(${func.num_ports_in_str()})-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(${func.num_ports_in_str()})-1:0]        s_rfnoc_chdr_tready,
%endif
%if has_outputs:
  // AXIS-CHDR Output Ports (to framework)
  output wire [(${func.num_ports_out_str()})*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(${func.num_ports_out_str()})-1:0]        m_rfnoc_chdr_tlast,
  output wire [(${func.num_ports_out_str()})-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(${func.num_ports_out_str()})-1:0]        m_rfnoc_chdr_tready,
%endif

  // AXIS-Ctrl Control Input Port (from framework)
  input  wire [31:0]           s_rfnoc_ctrl_tdata,
  input  wire                  s_rfnoc_ctrl_tlast,
  input  wire                  s_rfnoc_ctrl_tvalid,
  output wire                  s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Control Output Port (to framework)
  output wire [31:0]           m_rfnoc_ctrl_tdata,
  output wire                  m_rfnoc_ctrl_tlast,
  output wire                  m_rfnoc_ctrl_tvalid,
  input  wire                  m_rfnoc_ctrl_tready,

  //---------------------
  // Client Interface
  //---------------------

%if config['control']['fpga_iface'] == "ctrlport":
  // CtrlPort Clock and Reset
  output wire               ctrlport_clk,
  output wire               ctrlport_rst,
<%include file="/modules/ctrlport_wires_template.mako" args="mode='shell'"/>\
%elif config['control']['fpga_iface'] == "axis_ctrl":
  // AXIS-Ctrl Clock and Reset
  output wire               axis_ctrl_clk,
  output wire               axis_ctrl_rst,
<%include file="/modules/axis_ctrl_wires_template.mako" args="mode='shell'"/>\
%endif


<%
    fpga_data_iface = config.get('data', {}).get('fpga_iface')
%>\
%if fpga_data_iface == "axis_chdr":
  // AXIS-CHDR Clock and Reset
  output wire               axis_chdr_clk,
  output wire               axis_chdr_rst,
<%include file="/modules/axis_chdr_wires_template.mako" args="mode='shell'"/>\
%elif fpga_data_iface == "axis_pyld_ctxt":
  // AXI-Stream Payload Context Clock and Reset
  output wire               axis_data_clk,
  output wire               axis_data_rst,
<%include file="/modules/axis_pyld_ctxt_wires_template.mako" args="mode='shell'"/>\
%elif fpga_data_iface == "axis_data":
  // AXI-Stream Data Clock and Reset
  output wire               axis_data_clk,
  output wire               axis_data_rst,
<%include file="/modules/axis_data_wires_template.mako" args="mode='shell'"/>\
%endif

  // RFNoC Backend Interface
  input  wire [511:0]       rfnoc_core_config,
  output wire [511:0]       rfnoc_core_status
);

  //---------------------------------------------------------------------------
  //  Backend Interface
  //---------------------------------------------------------------------------
%if has_inputs:
  wire         data_i_flush_en;
  wire [31:0]  data_i_flush_timeout;
  wire [63:0]  data_i_flush_active;
  wire [63:0]  data_i_flush_done;
%endif
%if has_outputs:
  wire         data_o_flush_en;
  wire [31:0]  data_o_flush_timeout;
  wire [63:0]  data_o_flush_active;
  wire [63:0]  data_o_flush_done;
%endif

  backend_iface #(
    .NOC_ID        (32'h${format(config['noc_id'], "08X")}),
    .NUM_DATA_I    (${func.num_ports_in_str()}),
    .NUM_DATA_O    (${func.num_ports_out_str()}),
    .CTRL_FIFOSIZE ($clog2(${config['control']['fifo_depth']})),
    .MTU           (MTU)
  ) backend_iface_i (
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_chdr_rst       (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst       (rfnoc_ctrl_rst),
%if has_inputs:
    .data_i_flush_en      (data_i_flush_en),
    .data_i_flush_timeout (data_i_flush_timeout),
    .data_i_flush_active  (data_i_flush_active),
    .data_i_flush_done    (data_i_flush_done),
%else:
    .data_i_flush_en      (),
    .data_i_flush_timeout (),
    .data_i_flush_active  (64'b0),
    .data_i_flush_done    (64'b1),
%endif
%if has_outputs:
    .data_o_flush_en      (data_o_flush_en),
    .data_o_flush_timeout (data_o_flush_timeout),
    .data_o_flush_active  (data_o_flush_active),
    .data_o_flush_done    (data_o_flush_done),
%else:
    .data_o_flush_en      (),
    .data_o_flush_timeout (),
    .data_o_flush_active  (64'b0),
    .data_o_flush_done    (64'b1),
%endif
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status)
  );

<% reset_comment_block = False %>\
%for clock in config['clocks']:
  %if clock['name'] not in ["rfnoc_chdr", "rfnoc_ctrl"]:
    %if not reset_comment_block:
  //---------------------------------------------------------------------------
  //  Reset Generation
  //---------------------------------------------------------------------------
<% reset_comment_block = True %>
%endif
  wire ${clock['name']}_rst_pulse;

  pulse_synchronizer #(.MODE ("POSEDGE")) pulse_synchronizer_${clock['name']} (
    .clk_a(rfnoc_chdr_clk), .rst_a(1'b0), .pulse_a (rfnoc_chdr_rst), .busy_a (),
    .clk_b(${clock['name']}_clk), .pulse_b (${clock['name']}_rst_pulse)
  );

  pulse_stretch_min #(.LENGTH(32)) pulse_stretch_min_${clock['name']} (
    .clk(${clock['name']}_clk), .rst(1'b0),
    .pulse_in(${clock['name']}_rst_pulse), .pulse_out(${clock['name']}_rst)
  );

%endif
%endfor
  //---------------------------------------------------------------------------
  //  Control Path
  //---------------------------------------------------------------------------

%if config['control']['fpga_iface'] == "axis_ctrl":
  assign axis_ctrl_clk = ${config['control']['clk_domain']}_clk;
  assign axis_ctrl_rst = ${config['control']['clk_domain']}_rst;

<%include file="/modules/axis_ctrl_modules_template.mako"/>\
%elif config['control']['fpga_iface'] == "ctrlport":
  assign ctrlport_clk = ${config['control']['clk_domain']}_clk;
  assign ctrlport_rst = ${config['control']['clk_domain']}_rst;

<%include file="/modules/ctrlport_modules_template.mako"/>\
%endif

%if has_inputs or has_outputs:
  //---------------------------------------------------------------------------
  //  Data Path
  //---------------------------------------------------------------------------

  genvar i;

%if fpga_data_iface == "axis_pyld_ctxt":
  assign axis_data_clk = ${config['data']['clk_domain']}_clk;
  assign axis_data_rst = ${config['data']['clk_domain']}_rst;

<%include file="/modules/axis_pyld_ctxt_modules_template.mako"/>\
%elif fpga_data_iface == "axis_chdr":
  assign axis_chdr_clk = ${config['data']['clk_domain']}_clk;
  assign axis_chdr_rst = ${config['data']['clk_domain']}_rst;

<%include file="/modules/axis_chdr_modules_template.mako"/>\
%elif fpga_data_iface == "axis_data":
  assign axis_data_clk = ${config['data']['clk_domain']}_clk;
  assign axis_data_rst = ${config['data']['clk_domain']}_rst;

<%include file="/modules/axis_data_modules_template.mako"/>\
%endif
%endif
endmodule // noc_shell_${config['module_name']}


`default_nettype wire
