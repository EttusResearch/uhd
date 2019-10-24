<%page args="connections, block_ports"/>\
<%
  sources = []
  destinations = []
  for connection in connections:
    sources.append((connection["srcblk"], connection["srcport"]))
    destinations.append((connection["dstblk"], connection["dstport"]))
%>\
%for (block_name, port_name, direction) in block_ports:
  %if direction == "input":
    %if not (block_name, port_name) in destinations:
  assign s_${block_name}_${port_name}_tdata  = {CHDR_W{1'b0}};
  assign s_${block_name}_${port_name}_tlast  = 1'b0;
  assign s_${block_name}_${port_name}_tvalid = 1'b0;
    %endif
  %elif direction == "output":
    %if not (block_name, port_name) in sources:
  assign m_${block_name}_${port_name}_tready = 1'b1;
    %endif
  %endif
%endfor
