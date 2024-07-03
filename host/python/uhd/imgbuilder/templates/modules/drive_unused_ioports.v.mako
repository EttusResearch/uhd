<%page args="connections, config"/>\
<%
  sources = []
  destinations = []
  for connection in connections:
    if connection['srcblk'] == "_device_":
      sources.append(connection['srcport'])
    if connection['dstblk'] == "_device_":
      destinations.append(connection['dstport'])
%>\
% for iop_name, iop_info in config.device.io_ports.items():
  % if iop_name not in sources and iop_name not in destinations:
    % for wire in iop_info['wires']:
      % if wire['direction'] == "output":
<% safe_value = wire.get('safe') if 'safe' in wire else f"{wire['width']}'d0" %>\
  assign ${wire['name']} = ${safe_value};
      % endif
    % endfor
  % endif
% endfor
