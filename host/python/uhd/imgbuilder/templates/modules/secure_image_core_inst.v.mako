<%page args="config, top_config"/>\
<%
  if not config.noc_blocks and not config.modules and not config.transport_adapters:
    return STOP_RENDERING

  def p(pad_len):
    return lambda text: ("{text:<" + str(pad_len) + "}").format(text=text)
%>\

  //---------------------------------------------------------------------------
  // Secure Image Core
  //---------------------------------------------------------------------------
% for clk_rst, attr_name in (('clk', 'clocks'), ('rst', 'resets')):
<%
  clk_rst_wires = [
    c for c in getattr(config.device, attr_name)
    if c not in getattr(top_config.device, attr_name)]
%>\
  % for c in clk_rst_wires:
    % if loop.first:
    // Custom ${attr_name} for secure image core
    % endif
  wire ${c['name']}_${clk_rst};
  % endfor
% endfor
  // IO ports for secure image core
<%
  io_port_wires = [
    io_port for name, io_port in config.device.io_ports.items()
    if name not in top_config.device.io_ports]
%>\
% for io_port in io_port_wires:
  % for wire in io_port["wires"]:
  wire ${config.render_wire_width(wire, 12)} ${wire["name"]};
  % endfor
% endfor

% for block_name, block_info in config.noc_blocks.items():
  % if loop.first:
  // Block ports for secure image core
  % endif
  // ${block_name}
  % for port_name, port_info in block_info['data']['inputs'].items():
  wire [BLOCK_CHDR_W-1:0] s_${block_name}_${port_name}_tdata ;
  wire                    s_${block_name}_${port_name}_tlast ;
  wire                    s_${block_name}_${port_name}_tready;
  wire                    s_${block_name}_${port_name}_tvalid;
  % endfor
  % for port_name, port_info in block_info['data']['outputs'].items():
  wire [BLOCK_CHDR_W-1:0] m_${block_name}_${port_name}_tdata ;
  wire                    m_${block_name}_${port_name}_tlast ;
  wire                    m_${block_name}_${port_name}_tready;
  wire                    m_${block_name}_${port_name}_tvalid;
  % endfor
% endfor
% for ta_name, ta in config.transport_adapters.items():
  % if loop.first:
  // TA data ports for secure image core
  % endif
<% num_ports = sum(p.get('num_ports', 1) for p in ta.data.get('inputs', {}).values()) %>\
  % for n in range(num_ports):
  wire [CHDR_W-1:0] xb_to_${ta_name}_${n}_tdata ;
  wire              xb_to_${ta_name}_${n}_tlast ;
  wire              xb_to_${ta_name}_${n}_tvalid;
  wire              xb_to_${ta_name}_${n}_tready;
  % endfor
  % for n in range(num_ports):
  wire [CHDR_W-1:0] ${ta_name}_${n}_to_xb_tdata ;
  wire              ${ta_name}_${n}_to_xb_tlast ;
  wire              ${ta_name}_${n}_to_xb_tvalid;
  wire              ${ta_name}_${n}_to_xb_tready;
  % endfor
% endfor

  secure_image_core #(
% if config.noc_blocks:
    .BLOCK_PORT_IDS({${",".join(f"{b['port_index']}" for b in config.noc_blocks.values())}}),
% endif
    .CHDR_W        (CHDR_W),
    .BLOCK_CHDR_W  (BLOCK_CHDR_W),
    .PROTOVER      (PROTOVER),
    .BLOCK_MTU     (BLOCK_MTU)
  ) secure_image_core_i (
    .chdr_aclk                     (chdr_aclk),
    .ctrl_aclk                     (ctrl_aclk),
    .core_arst                     (core_arst),
    .rfnoc_ctrl_clk                (rfnoc_ctrl_clk      ),
    .rfnoc_chdr_clk                (rfnoc_chdr_clk      ),
    .rfnoc_ctrl_rst                (rfnoc_ctrl_rst      ),
    .rfnoc_chdr_rst                (rfnoc_chdr_rst      ),
% for clk_rst, attr_name, default_names in (('clk', 'clocks', config.DEFAULT_CLK_NAMES), ('rst', 'resets', config.DEFAULT_RST_NAMES)):
  % for c in getattr(config.device, attr_name):
    % if loop.first:
    // ${attr_name} for secure image core
    % endif
<% if c['name'] in default_names: continue %>\
    .${f"{c['name']}_{clk_rst}" | p(30)}(${f"{c['name']}_{clk_rst}" | p(20)}),
  % endfor
% endfor
    // IO ports
% for name, io_port in config.device.io_ports.items():
  % for wire in io_port["wires"]:
    .${f"{wire['name']:30}"}(${f"{wire['name']:20}"}),
  % endfor
% endfor
% for block_name, block_info in config.noc_blocks.items():
  % if loop.first:
    // Block ports for secure image core
  % endif
    // ${block_name}
  % for port_name, port_info in block_info['data']['inputs'].items():
    .${f"s_{block_name}_{port_name}_tdata " | p(30)}(${f"s_{block_name}_{port_name}_tdata " | p(20)}),
    .${f"s_{block_name}_{port_name}_tlast " | p(30)}(${f"s_{block_name}_{port_name}_tlast " | p(20)}),
    .${f"s_{block_name}_{port_name}_tready" | p(30)}(${f"s_{block_name}_{port_name}_tready " | p(20)}),
    .${f"s_{block_name}_{port_name}_tvalid" | p(30)}(${f"s_{block_name}_{port_name}_tvalid" | p(20)}),
  % endfor
  % for port_name, port_info in block_info['data']['outputs'].items():
    .${f"m_{block_name}_{port_name}_tdata " | p(30)}(${f"m_{block_name}_{port_name}_tdata " | p(20)}),
    .${f"m_{block_name}_{port_name}_tlast " | p(30)}(${f"m_{block_name}_{port_name}_tlast " | p(20)}),
    .${f"m_{block_name}_{port_name}_tready" | p(30)}(${f"m_{block_name}_{port_name}_tready " | p(20)}),
    .${f"m_{block_name}_{port_name}_tvalid" | p(30)}(${f"m_{block_name}_{port_name}_tvalid" | p(20)}),
  % endfor
    .${f"s_{block_name}_ctrl_tdata " | p(30)}(${f"s_{block_name}_ctrl_tdata " | p(20)}),
    .${f"s_{block_name}_ctrl_tlast " | p(30)}(${f"s_{block_name}_ctrl_tlast " | p(20)}),
    .${f"s_{block_name}_ctrl_tready" | p(30)}(${f"s_{block_name}_ctrl_tready" | p(20)}),
    .${f"s_{block_name}_ctrl_tvalid" | p(30)}(${f"s_{block_name}_ctrl_tvalid" | p(20)}),
    .${f"m_{block_name}_ctrl_tdata " | p(30)}(${f"m_{block_name}_ctrl_tdata " | p(20)}),
    .${f"m_{block_name}_ctrl_tlast " | p(30)}(${f"m_{block_name}_ctrl_tlast " | p(20)}),
    .${f"m_{block_name}_ctrl_tready" | p(30)}(${f"m_{block_name}_ctrl_tready" | p(20)}),
    .${f"m_{block_name}_ctrl_tvalid" | p(30)}(${f"m_{block_name}_ctrl_tvalid" | p(20)}),
% endfor
% for ta_name, ta in config.transport_adapters.items():
  % if loop.first:
    // TA data ports for secure image core
  % endif
<%
  num_ports = sum(p.get('num_ports', 1) for p in ta.data.get('inputs', {}).values())
%>\
  % for n in range(num_ports):
    .${f"xb_to_{ta_name}_{n}_tdata " | p(30)}(${f"xb_to_{ta_name}_{n}_tdata " | p(20)}),
    .${f"xb_to_{ta_name}_{n}_tlast " | p(30)}(${f"xb_to_{ta_name}_{n}_tlast " | p(20)}),
    .${f"xb_to_{ta_name}_{n}_tready" | p(30)}(${f"xb_to_{ta_name}_{n}_tready" | p(20)}),
    .${f"xb_to_{ta_name}_{n}_tvalid" | p(30)}(${f"xb_to_{ta_name}_{n}_tvalid" | p(20)}),
  % endfor
  % for n in range(num_ports):
    .${f"{ta_name}_{n}_to_xb_tdata " | p(30)}(${f"{ta_name}_{n}_to_xb_tdata " | p(20)}),
    .${f"{ta_name}_{n}_to_xb_tlast " | p(30)}(${f"{ta_name}_{n}_to_xb_tlast " | p(20)}),
    .${f"{ta_name}_{n}_to_xb_tready" | p(30)}(${f"{ta_name}_{n}_to_xb_tready" | p(20)}),
    .${f"{ta_name}_{n}_to_xb_tvalid" | p(30)}(${f"{ta_name}_{n}_to_xb_tvalid" | p(20)}),
  % endfor
% endfor
% if config.noc_blocks:
    // Backend interface
<%
  secure_module_indices = list(reversed([b["blk_index"] for b in config.noc_blocks.values()]))
%>\
    .rfnoc_core_config             ({
% for secure_module_i in secure_module_indices:
        rfnoc_core_config[512*${secure_module_i + 1}-1:512*${secure_module_i}]${"," if not loop.last else ""}
% endfor
    }),
    .rfnoc_core_status             ({
% for secure_module_i in secure_module_indices:
        rfnoc_core_status[512*${secure_module_i + 1}-1:512*${secure_module_i}]${"," if not loop.last else ""}
% endfor
    }),
% endif
    .__terminate(1'b0)
  );
