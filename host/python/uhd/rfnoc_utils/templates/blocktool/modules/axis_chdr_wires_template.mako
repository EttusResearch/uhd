<%page args="mode"/>\
<%
  if mode == "shell":
    sl_pre = "s_"
    ma_pre = "m_"
    in_wire = "input  "
    out_wire = "output "
    term = ","
  elif mode == "block":
    sl_pre = "s_"
    ma_pre = "m_"
    in_wire = ""
    out_wire = ""
    term = ";"
  # Get the number of input and outputs port names
  num_inputs  = len(config['data']['inputs'])
  num_outputs = len(config['data']['outputs'])
%>\
%for idx, port in enumerate(config['data']['inputs']):
<%
  port_info = config['data']['inputs'][port]
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
  // Framework to User Logic: ${port}
%if num_ports != 1:
  ${out_wire}wire [${num_ports}*CHDR_W-1:0]  ${ma_pre}${port}_chdr_tdata${term}
  ${out_wire}wire [${num_ports}-1:0]         ${ma_pre}${port}_chdr_tlast${term}
  ${out_wire}wire [${num_ports}-1:0]         ${ma_pre}${port}_chdr_tvalid${term}
  ${in_wire}wire [${num_ports}-1:0]         ${ma_pre}${port}_chdr_tready${term if (term == ";") or (idx < num_inputs -1) or (num_outputs > 0) else ""}
%else:
  ${out_wire}wire [CHDR_W-1:0]  ${ma_pre}${port}_chdr_tdata${term}
  ${out_wire}wire               ${ma_pre}${port}_chdr_tlast${term}
  ${out_wire}wire               ${ma_pre}${port}_chdr_tvalid${term}
  ${in_wire}wire               ${ma_pre}${port}_chdr_tready${term if (term == ";") or (idx < num_inputs -1) or (num_outputs > 0) else ""}
%endif
%endfor
%for idx, port in enumerate(config['data']['outputs']):
<%
  port_info = config['data']['outputs'][port]
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
  // User Logic to Framework: ${port}
%if num_ports != 1:
  ${in_wire}wire [${num_ports}*CHDR_W-1:0]  ${sl_pre}${port}_chdr_tdata${term}
  ${in_wire}wire [${num_ports}-1:0]         ${sl_pre}${port}_chdr_tlast${term}
  ${in_wire}wire [${num_ports}-1:0]         ${sl_pre}${port}_chdr_tvalid${term}
  ${out_wire}wire [${num_ports}-1:0]         ${sl_pre}${port}_chdr_tready${term if (term == ";") or (idx < num_outputs -1) else ""}
%else:
  ${in_wire}wire [CHDR_W-1:0]  ${sl_pre}${port}_chdr_tdata${term}
  ${in_wire}wire               ${sl_pre}${port}_chdr_tlast${term}
  ${in_wire}wire               ${sl_pre}${port}_chdr_tvalid${term}
  ${out_wire}wire               ${sl_pre}${port}_chdr_tready${term if (term == ";") or (idx < num_outputs -1) else ""}
%endif
%endfor
