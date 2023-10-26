<%!
import math
%>\
<%namespace name="func" file="/functions.mako"/>\
//
// Copyright ${year} Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_${config['module_name']}
//
// Description:
//
//   <Add block description here>
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//

`default_nettype none

<%
    has_inputs = 'inputs' in config.get('data', {})
    has_outputs = 'outputs' in config.get('data', {})
    fpga_data_iface = config.get('data', {}).get('fpga_iface')
%>\
module rfnoc_block_${config['module_name']} #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10${"," if ('parameters' in config) else ""}
%if 'parameters' in config:
<% param_count = 1 %>\
%for param, value in config['parameters'].items():
  parameter       ${'{:<15}'.format(param)} = ${value}${',' if param_count < len(config['parameters']) else ''}
<% param_count = param_count+1 %>\
% endfor
%endif
)(
  // RFNoC Framework Clocks and Resets
%for clock in config['clocks']:
  input  wire                   ${clock['name']}_clk,
%endfor
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
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

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
%if config['control']['fpga_iface'] == "ctrlport":
  wire               ctrlport_clk;
  wire               ctrlport_rst;
%elif config['control']['fpga_iface'] == "axis_ctrl":
  wire               axis_ctrl_clk;
  wire               axis_ctrl_rst;
%endif
%if fpga_data_iface == "axis_chdr":
  wire               axis_chdr_clk;
  wire               axis_chdr_rst;
%elif fpga_data_iface == "axis_pyld_ctxt":
  wire               axis_data_clk;
  wire               axis_data_rst;
%elif fpga_data_iface == "axis_data":
  wire               axis_data_clk;
  wire               axis_data_rst;
%endif
%if config['control']['fpga_iface'] == "ctrlport":
<%include file="/modules/ctrlport_wires_template.mako" args="mode='block'"/>\
%elif config['control']['fpga_iface'] == "axis_ctrl":
<%include file="/modules/axis_ctrl_wires_template.mako" args="mode='block'"/>\
%endif
%if fpga_data_iface == "axis_chdr":
<%include file="/modules/axis_chdr_wires_template.mako" args="mode='block'"/>\
%elif fpga_data_iface == "axis_pyld_ctxt":
<%include file="/modules/axis_pyld_ctxt_wires_template.mako" args="mode='block'"/>\
%elif fpga_data_iface == "axis_data":
<%include file="/modules/axis_data_wires_template.mako" args="mode='block'"/>\
%endif

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_${config['module_name']} #(
    .CHDR_W              (CHDR_W),
    .THIS_PORTID         (THIS_PORTID),
    .MTU                 (MTU)${"," if ('parameters' in config) else ""}
%if 'parameters' in config:
<% param_count = 1 %>\
%for param, value in config['parameters'].items():
    .${'{:<19}'.format(param)} (${param})${',' if param_count < len(config['parameters']) else ''}
<% param_count = param_count+1 %>\
% endfor
%endif
  ) noc_shell_${config['module_name']}_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
  %for clock in config['clocks']:
    .${'{:<19}'.format(clock['name']+'_clk')} (${clock['name']}_clk),
  %endfor
    // Reset Outputs
  %for clock in config['clocks']:
    .${'{:<19}'.format(clock['name']+'_rst')} (),
  %endfor
%if has_inputs:
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
%endif
%if has_outputs:
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
%endif
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

%if config['control']['fpga_iface'] == "ctrlport":
<%include file="/modules/ctrlport_connect_template.mako"/>\
%elif config['control']['fpga_iface'] == "axis_ctrl":
<%include file="/modules/axis_ctrl_connect_template.mako"/>\
%endif

%if fpga_data_iface == "axis_chdr":
<%include file="/modules/axis_chdr_connect_template.mako"/>\
%elif fpga_data_iface == "axis_pyld_ctxt":
<%include file="/modules/axis_pyld_ctxt_connect_template.mako"/>\
%elif fpga_data_iface == "axis_data":
<%include file="/modules/axis_data_connect_template.mako"/>\
%endif

    //---------------------------
    // RFNoC Backend Interface
    //---------------------------
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status)
  );

  //---------------------------------------------------------------------------
  // User Logic
  //---------------------------------------------------------------------------

  // < Replace this section with your logic >

  // Nothing to do yet, so just drive control signals to default values
%if config['control']['fpga_iface'] == "ctrlport":
  assign m_ctrlport_resp_ack = 1'b0;
  %if config['control']['ctrlport']['has_status']:
  assign m_ctrlport_resp_status = 2'b0;
  %endif
  %if config['control']['interface_direction'] != "slave":
  assign s_ctrlport_req_wr = 1'b0;
  assign s_ctrlport_req_rd = 1'b0;
  %endif
%elif config['control']['fpga_iface'] == "axis_ctrl":
  assign m_axis_ctrl_tready = 1'b0;
  assign s_axis_ctrl_tvalid = 1'b0;
%endif
%if fpga_data_iface == "axis_chdr":
  %for port_name, port_info in config['data']['inputs'].items():
    %if 'num_ports' in port_info:
  assign m_${port_name}_chdr_tready = {${port_info['num_ports']}{1'b0}};
    %else:
  assign m_${port_name}_chdr_tready = 1'b0;
    %endif
  %endfor
  %for port_name, port_info in config['data']['outputs'].items():
    %if 'num_ports' in port_info:
  assign s_${port_name}_chdr_tvalid = {${port_info['num_ports']}{1'b0}};
    %else:
  assign s_${port_name}_chdr_tvalid = 1'b0;
    %endif
  %endfor
%elif fpga_data_iface == "axis_pyld_ctxt":
  %for port_name, port_info in config['data']['inputs'].items():
    %if 'num_ports' in port_info:
  assign m_${port_name}_payload_tready = {${port_info['num_ports']}{1'b0}};
  assign m_${port_name}_context_tready = {${port_info['num_ports']}{1'b0}};
    %else:
  assign m_${port_name}_payload_tready = 1'b0;
  assign m_${port_name}_context_tready = 1'b0;
    %endif
  %endfor
  %for port_name, port_info in config['data']['outputs'].items():
    %if 'num_ports' in port_info:
  assign s_${port_name}_payload_tvalid = {${port_info['num_ports']}{1'b0}};
  assign s_${port_name}_context_tvalid = {${port_info['num_ports']}{1'b0}};
    %else:
  assign s_${port_name}_payload_tvalid = 1'b0;
  assign s_${port_name}_context_tvalid = 1'b0;
    %endif
  %endfor
%elif fpga_data_iface == "axis_data":
  %for port_name, port_info in config['data']['inputs'].items():
    %if 'num_ports' in port_info:
  assign m_${port_name}_axis_tready = {${port_info['num_ports']}{1'b0}};
    %else:
  assign m_${port_name}_axis_tready = 1'b0;
    %endif
  %endfor
  %for port_name, port_info in config['data']['outputs'].items():
    %if 'num_ports' in port_info:
  assign s_${port_name}_axis_tvalid = {${port_info['num_ports']}{1'b0}};
    %else:
  assign s_${port_name}_axis_tvalid = 1'b0;
    %endif
  %endfor
%endif

endmodule // rfnoc_block_${config['module_name']}


`default_nettype wire
