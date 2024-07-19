<%page args="connections, config"/>\
<%
  sources = []
  destinations = []
  for connection in connections:
    sources.append((connection["srcblk"], connection["srcport"]))
    destinations.append((connection["dstblk"], connection["dstport"]))
%>\
% for block_name, block_info in config.noc_blocks.items():
  % for port_name, port_info in block_info['data']['inputs'].items():
    % if (block_name, port_name) not in destinations:
  assign s_${block_name}_${port_name}_tdata  = {CHDR_W{1'b0}};
  assign s_${block_name}_${port_name}_tlast  = 1'b0;
  assign s_${block_name}_${port_name}_tvalid = 1'b0;
    % endif
  % endfor
  % for port_name, port_info in block_info['data']['outputs'].items():
    % if (block_name, port_name) not in sources:
  assign m_${block_name}_${port_name}_tready = 1'b1;
    % endif
  % endfor
% endfor
