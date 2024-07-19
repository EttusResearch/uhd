<%page args="connections, core_domain='top', clk_or_rst='clk'"/>\
\
%for connection in connections:
<%
  def format_name(blk, port):
    if blk == "_device_":
      return f"{port}_{clk_or_rst}"
    return f"{blk}_{port}_{clk_or_rst}"
  src_wire_suffix = '_s' if core_domain == 'secure_core' and connection['srcblk'] != "_device_" else ''
  dst_wire_suffix = '_s' if core_domain == 'secure_core' and connection['dstblk'] != "_device_" else ''
  src_wire_name = format_name(connection['srcblk'], connection['srcport'])
  dst_wire_name = format_name(connection['dstblk'], connection['dstport'])
%>\
  assign ${dst_wire_name}${dst_wire_suffix} = ${src_wire_name}${src_wire_suffix};
%endfor
