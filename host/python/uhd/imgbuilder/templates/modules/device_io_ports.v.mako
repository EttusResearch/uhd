<%page args="io_ports"/>\
  // IO ports /////////////////////////

%for name, io_port in io_ports.items():
  // ${name}
  %for wire in io_port["wires"]:
  ${wire["direction"]} wire [${"%4d" % (wire["width"]-1)}:0] ${wire["name"]},
  %endfor
%endfor
