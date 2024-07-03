<%page args="io_ports, config, width_pad=8, final_ports=True"/>\
  // IO ports /////////////////////////

<%
  trl = "," if config.get_ta_gen_mode() == "fixed" or not final_ports else ""
%>\
%for name, io_port in io_ports.items():
  // ${name}
  %for wire in io_port["wires"]:
  ${wire["direction"]} wire ${config.render_wire_width(wire, width_pad)} ${wire["name"]}${trl if loop.last and loop.parent.last else ","}
  %endfor
%endfor
