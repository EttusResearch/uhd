<%page args="i, module_name, config, core_domain='top'"/>\
<%
  module = config.modules[module_name]
  module_domain = module.get('domain')
  module_params = config.get_hdl_parameters(module_name)
  wire_suffix = '_s' if core_domain == 'secure_core' else ''
%>\

  //-----------------------------------
  // ${module_name}
  //-----------------------------------
% for clock in module.desc.clocks:
<% if clock['name'] in config.DEFAULT_CLK_NAMES: continue %>\
  wire          ${module_name}_${clock["name"]}_clk${wire_suffix};
% endfor
% for reset in module.desc.resets:
<% if clock['name'] in config.DEFAULT_RST_NAMES: continue %>\
  wire          ${module_name}_${reset["name"]}_rst${wire_suffix};
% endfor
% for name, io_port in getattr(module, 'io_ports', {}).items():
  % for wire in io_port["wires"]:
  wire ${config.render_wire_width(wire)} ${module_name}_${wire["name"]}${wire_suffix};
  % endfor
% endfor

<%
  if module.io_ports:
    last_signal_group = 'io_ports'
  elif module.desc.resets:
    last_signal_group = 'resets'
  elif module.desc.clocks:
    last_signal_group = 'clocks'
%>\
  ${module.desc.module_name} #(
%for name, value in module_params.items():
    .${f"{name:20s}"}(${value})${"" if loop.last else ","}
%endfor
  ) m_${module_name}_${'s' if core_domain == 'secure_core' else ''}${i} (
% for clock in module.desc.clocks:
    .${f"{clock['name']:20s}"}(${module_name}_${clock["name"]}_clk${wire_suffix if clock['name'] not in ("rfnoc_chdr", "rfnoc_ctrl") else ""})${"" if loop.last and last_signal_group == 'clocks' else ","}
% endfor
% for reset in module.desc.resets:
    .${f"{reset['name']:20s}"}(${module_name}_${reset["name"]}_rst${wire_suffix if reset['name'] not in ("core_arst",) else ""})${"" if loop.last and last_signal_group == 'resets' else ","}
% endfor
% for name, io_port in module.io_ports.items():
  % for wire in io_port["wires"]:
    .${f"{wire['name']:20s}"}(${module_name}_${wire["name"]}${wire_suffix})${"" if loop.last and last_signal_group == 'io_ports' else ","}
  % endfor
% endfor
  );

