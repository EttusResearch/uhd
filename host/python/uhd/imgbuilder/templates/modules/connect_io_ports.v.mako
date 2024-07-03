<%page args="connections, core_domain='top'"/>\
\
%for connection in connections:
<%
  src_name = connection["srcblk"]
  dst_name = connection["dstblk"]
  src = connection["src_iosig"]
  dst = connection["dst_iosig"]
  src_module_prefix = '' if src_name == '_device_' else f"{src_name}_"
  dst_module_prefix = '' if dst_name == '_device_' else f"{dst_name}_"
  src_wire_suffix = '_s' if core_domain == 'secure_core' and src_name != "_device_" else ''
  dst_wire_suffix = '_s' if core_domain == 'secure_core' and dst_name != "_device_" else ''
  src_slice = f"[{connection['srcslice']}]" if connection['srcslice'] else ''
  dst_slice = f"[{connection['dstslice']}]" if connection['dstslice'] else ''
%>\
  %for src_wire, dst_wire in zip(src["wires"], dst["wires"]):
<%
  swire = src_module_prefix + src_wire['name'] + src_wire_suffix
  dwire = dst_module_prefix + dst_wire['name'] + dst_wire_suffix
  if src_wire["direction"] == "output":
    swire, dwire = dwire, swire
%>\
  assign ${dwire}${dst_slice} = ${swire}${src_slice};
  %endfor

%endfor
