<%page args="io_ports"/>\
<%import six%>\
//// IO ports //////////////////////////////////
%for name, io_port in six.iteritems(io_ports):
//  ${name}
  %for wire in io_port["wires"]:
  ${wire["direction"]} wire [${"%3d" % wire["width"]}-1:0] ${wire["name"]},
  %endfor
%endfor
