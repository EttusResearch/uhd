<%page args="i, block_name, config, core_domain='top'"/>\
\
<%
  block = config.noc_blocks[block_name]
  block_params = config.get_hdl_parameters(block_name)
  ctrl_clock = block.get('ctrl_clock')
  timebase_clock = block.get('timebase_clock')
  clocks = config.clocks
  block_domain = block.get('domain')
  # Create two strings, one for the input and one for the output, that each
  # contains all the signal names to be connected to the input or output
  # AXIS-CHDR ports of this block.
  axis_inputs = ""
  axis_outputs = ""
  for port_name, port_info in block['data']['inputs'].items():
    axis_inputs = "{0}_%s_%s_{1}, " % (block_name, port_name) + axis_inputs
  for port_name, port_info in block['data']['outputs'].items():
    axis_outputs = "{0}_%s_%s_{1}, " % (block_name, port_name) + axis_outputs
  axis_inputs = axis_inputs[:-2]
  axis_outputs = axis_outputs[:-2]
  wire_suffix = '_s' if core_domain == 'secure_core' else ''
%>\

  //-----------------------------------
  // ${block_name}
  //-----------------------------------

% for clock in block.desc.clocks:
<% if clock['name'] in config.DEFAULT_CLK_NAMES: continue %>\
  wire                    ${block_name}_${clock["name"]}_clk${wire_suffix};
% endfor
% if core_domain != 'secure_core': ## In the secure core, these are ports, not wires
  % if axis_inputs:
  wire [BLOCK_CHDR_W-1:0] ${axis_inputs.format("s", "tdata ")};
  wire                    ${axis_inputs.format("s", "tlast ")};
  wire                    ${axis_inputs.format("s", "tvalid")};
  wire                    ${axis_inputs.format("s", "tready")};
  % endif
  % if axis_outputs:
  wire [BLOCK_CHDR_W-1:0] ${axis_outputs.format("m", "tdata ")};
  wire                    ${axis_outputs.format("m", "tlast ")};
  wire                    ${axis_outputs.format("m", "tvalid")};
  wire                    ${axis_outputs.format("m", "tready")};
  % endif
% endif

% if hasattr(block, "io_ports"):
  % for name, io_port in block.io_ports.items():
  // ${name}
    % for wire in io_port["wires"]:
  wire ${config.render_wire_width(wire)} ${block_name}_${wire["name"]}${wire_suffix};
    % endfor
  % endfor

% endif

  rfnoc_block_${block.desc.module_name} #(
    .THIS_PORTID         (BLOCK_PORT_IDS[${i}]),
    .CHDR_W              (BLOCK_CHDR_W),
%for name, value in block_params.items():
    .${"%-20s" % name}(${value}),
%endfor
<%
    if ctrl_clock and '.' not in ctrl_clock:
        ctrl_clock = '_device_.' + ctrl_clock
    if timebase_clock and '.' not in timebase_clock:
        timebase_clock = '_device_.' + timebase_clock
    ctrl_clk_index = clocks.get(ctrl_clock, {}).get('index')
    timebase_clk_index = clocks.get(timebase_clock, {}).get('index')
%>\
%if ctrl_clk_index is not None:
    .CTRL_CLK_IDX        (${ctrl_clk_index}),
%endif
%if timebase_clk_index is not None:
    .TB_CLK_IDX          (${timebase_clk_index}),
%endif
    .MTU                 (BLOCK_MTU)
  ) b_${block_name}_${'s' if core_domain == 'secure_core' else ''}${i} (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
%for clock in block.desc.clocks:
  %if not clock["name"] in config.DEFAULT_CLK_NAMES:
    .${f"%-20s" % (clock["name"] + "_clk")}(${block_name}_${clock["name"]}_clk${wire_suffix}),
  %endif
%endfor
    .rfnoc_core_config   (rfnoc_core_config[512*${i + 1}-1:512*${i}]),
    .rfnoc_core_status   (rfnoc_core_status[512*${i + 1}-1:512*${i}]),
% for name, io_port in block.io_ports.items():
  % for wire in io_port["wires"]:
    .${f"{wire['name']:20}"}(${block_name}_${wire["name"]}${wire_suffix}),
  % endfor
% endfor
%if axis_inputs:
    .s_rfnoc_chdr_tdata  ({${axis_inputs.format("s", "tdata ")}}),
    .s_rfnoc_chdr_tlast  ({${axis_inputs.format("s", "tlast ")}}),
    .s_rfnoc_chdr_tvalid ({${axis_inputs.format("s", "tvalid")}}),
    .s_rfnoc_chdr_tready ({${axis_inputs.format("s", "tready")}}),
%endif
%if axis_outputs:
    .m_rfnoc_chdr_tdata  ({${axis_outputs.format("m", "tdata ")}}),
    .m_rfnoc_chdr_tlast  ({${axis_outputs.format("m", "tlast ")}}),
    .m_rfnoc_chdr_tvalid ({${axis_outputs.format("m", "tvalid")}}),
    .m_rfnoc_chdr_tready ({${axis_outputs.format("m", "tready")}}),
%endif
    .s_rfnoc_ctrl_tdata  (s_${block_name}_ctrl_tdata ),
    .s_rfnoc_ctrl_tlast  (s_${block_name}_ctrl_tlast ),
    .s_rfnoc_ctrl_tvalid (s_${block_name}_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_${block_name}_ctrl_tready),
    .m_rfnoc_ctrl_tdata  (m_${block_name}_ctrl_tdata ),
    .m_rfnoc_ctrl_tlast  (m_${block_name}_ctrl_tlast ),
    .m_rfnoc_ctrl_tvalid (m_${block_name}_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_${block_name}_ctrl_tready)
  );
