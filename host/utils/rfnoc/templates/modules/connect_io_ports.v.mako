<%page args="connections, io_ports, names"/>\
\
%for connection in connections:
<%
  src_name = connection["srcblk"]
  src = io_ports[(src_name, connection["srcport"], names[0])]
  dst_name = connection["dstblk"]
  dst = io_ports[(dst_name, connection["dstport"], names[1])]
%>\
  %for src_wire, dst_wire in zip(src["wires"], dst["wires"]):
<%
  swire = src_wire["name"] if src_name == "_device_" else "%s_%s" % (src_name, src_wire["name"])
  dwire = dst_wire["name"] if dst_name == "_device_" else "%s_%s" % (dst_name, dst_wire["name"])
  if src_wire["direction"] == "output":
    swire, dwire = dwire, swire
%>\
  assign ${dwire} = ${swire};
  %endfor

%endfor
