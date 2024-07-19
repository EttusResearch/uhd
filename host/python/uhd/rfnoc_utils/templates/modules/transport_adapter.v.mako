<%page args="config, ta_name, ta, core_domain='top'"/>\
<%
  clocks = config.clocks
  ta_domain = ta.get('domain')
  hdl_params = config.get_hdl_parameters(ta_name)
  # We assume that inputs and outputs are symmetric
  num_ports = sum(p.get('num_ports', 1) for p in ta.data.get('inputs', {}).values())
  # Create two strings, one for the input and one for the output, that each
  # contains all the signal names to be connected to the input or output
  # AXIS-CHDR ports of this transport adapter.
  axis_inputs = ", ".join([f"xb_to_{ta_name}_{num_port}_{{0}}" for num_port in reversed(range(num_ports))])
  axis_outputs = ", ".join([f"{ta_name}_{num_port}_to_xb_{{0}}" for num_port in reversed(range(num_ports))])
  wire_suffix = '_s' if core_domain == 'secure_core' else ''
%>\

  //-----------------------------------
  // ${ta_name}
  //-----------------------------------

% for clock in ta.desc.clocks:
<% if clock['name'] in config.DEFAULT_CLK_NAMES: continue %>\
  wire          ${ta_name}_${clock["name"]}_clk${wire_suffix};
% endfor
% if num_ports and ta_domain != 'secure_core': ## In the secure core, these are ports
  wire [PORT_W-1:0] ${axis_inputs.format("tdata ")};
  wire              ${axis_inputs.format("tlast ")};
  wire              ${axis_inputs.format("tvalid")};
  wire              ${axis_inputs.format("tready")};
  wire [PORT_W-1:0] ${axis_outputs.format("tdata ")};
  wire              ${axis_outputs.format("tlast ")};
  wire              ${axis_outputs.format("tvalid")};
  wire              ${axis_outputs.format("tready")};
% endif

% for name, io_port in getattr(ta, 'io_ports', {}).items():
  % if loop.first:
  // IO ports
  % endif
  // ${name}
  % for wire in io_port["wires"]:
  wire ${config.render_wire_width(wire)} ${ta_name}_${wire["name"]}${wire_suffix};
  % endfor
% endfor

  ${ta.desc.module_name} #(
%for name, value in hdl_params.items():
  % if name not in ('CHDR_W', 'PROTOVER'):
    .${"%-20s" % name}(${value}),
  % endif
%endfor
    .CHDR_W              (CHDR_W),
    .PROTOVER            (PROTOVER)
  ) ${ta_name}${'_s' if core_domain == 'secure_core' else ''} (
%for clock in ta.desc.clocks:
  % if loop.first:
    // Clocks specific to this TA type
  % endif
  %if not clock["name"] in config.DEFAULT_CLK_NAMES:
    .${f"%-20s" % (clock["name"] + "_clk")}(${ta_name}_${clock["name"]}_clk${wire_suffix}),
  %endif
%endfor
% for name, io_port in ta.io_ports.items():
    % if loop.first:
    // IO ports
    % endif
  % for wire in io_port["wires"]:
    .${f"{wire['name']:20}"}(${ta_name}_${wire["name"]}${wire_suffix}),
  % endfor
% endfor
% if num_ports:
    // CHDR Ports for Crossbar
    .s_rfnoc_chdr_tdata  ({${axis_outputs.format("tdata ")}}),
    .s_rfnoc_chdr_tlast  ({${axis_outputs.format("tlast ")}}),
    .s_rfnoc_chdr_tvalid ({${axis_outputs.format("tvalid")}}),
    .s_rfnoc_chdr_tready ({${axis_outputs.format("tready")}}),
    .m_rfnoc_chdr_tdata  ({${axis_inputs.format("tdata ")}}),
    .m_rfnoc_chdr_tlast  ({${axis_inputs.format("tlast ")}}),
    .m_rfnoc_chdr_tvalid ({${axis_inputs.format("tvalid")}}),
    .m_rfnoc_chdr_tready ({${axis_inputs.format("tready")}}),
% endif
    // Standard Clocks and Resets (used by all TA types)
    .core_arst           (core_arst),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst      (rfnoc_ctrl_rst),
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_chdr_rst      (rfnoc_chdr_rst)
  );
