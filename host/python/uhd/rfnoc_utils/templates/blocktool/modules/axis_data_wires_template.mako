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
%for idx, port_name in enumerate(config['data']['inputs']):
<%
  port_info = config['data']['inputs'][port_name]
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
%if num_ports != 1:
  // Data Stream to User Logic: ${port_name}
  ${out_wire}wire [${num_ports}*${port_info['item_width']}*${port_info['nipc']}-1:0]   ${ma_pre}${port_name}_axis_tdata${term}
  ${out_wire}wire [${num_ports}*${port_info['nipc']}-1:0]      ${ma_pre}${port_name}_axis_tkeep${term}
  ${out_wire}wire [${num_ports}-1:0]        ${ma_pre}${port_name}_axis_tlast${term}
  ${out_wire}wire [${num_ports}-1:0]        ${ma_pre}${port_name}_axis_tvalid${term}
  ${in_wire}wire [${num_ports}-1:0]        ${ma_pre}${port_name}_axis_tready${term}
  ${out_wire}wire [${num_ports}*64-1:0]     ${ma_pre}${port_name}_axis_ttimestamp${term}
  ${out_wire}wire [${num_ports}-1:0]        ${ma_pre}${port_name}_axis_thas_time${term}
  ${out_wire}wire [${num_ports}*16-1:0]     ${ma_pre}${port_name}_axis_tlength${term}
  ${out_wire}wire [${num_ports}-1:0]        ${ma_pre}${port_name}_axis_teov${term}
  ${out_wire}wire [${num_ports}-1:0]        ${ma_pre}${port_name}_axis_teob${term if (term == ";") or (idx < num_inputs - 1) or (num_outputs > 0) else ""}
%else:
  // Data Stream to User Logic: ${port_name}
  ${out_wire}wire [${port_info['item_width']}*${port_info['nipc']}-1:0]    ${ma_pre}${port_name}_axis_tdata${term}
  ${out_wire}wire [${port_info['nipc']}-1:0]       ${ma_pre}${port_name}_axis_tkeep${term}
  ${out_wire}wire               ${ma_pre}${port_name}_axis_tlast${term}
  ${out_wire}wire               ${ma_pre}${port_name}_axis_tvalid${term}
  ${in_wire}wire               ${ma_pre}${port_name}_axis_tready${term}
  ${out_wire}wire [63:0]        ${ma_pre}${port_name}_axis_ttimestamp${term}
  ${out_wire}wire               ${ma_pre}${port_name}_axis_thas_time${term}
  ${out_wire}wire [15:0]        ${ma_pre}${port_name}_axis_tlength${term}
  ${out_wire}wire               ${ma_pre}${port_name}_axis_teov${term}
  ${out_wire}wire               ${ma_pre}${port_name}_axis_teob${term if (term == ";") or (idx < num_inputs - 1) or (num_outputs > 0) else ""}
%endif
%endfor
%for idx, port_name in enumerate(config['data']['outputs']):
<%
  port_info = config['data']['outputs'][port_name]
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
%if num_ports != 1:
  // Data Stream from User Logic: ${port_name}
  ${in_wire}wire [${num_ports}*${port_info['item_width']}*${port_info['nipc']}-1:0]   ${sl_pre}${port_name}_axis_tdata${term}
  ${in_wire}wire [${num_ports}*${port_info['nipc']}-1:0]      ${sl_pre}${port_name}_axis_tkeep${term}
  ${in_wire}wire [${num_ports}-1:0]        ${sl_pre}${port_name}_axis_tlast${term}
  ${in_wire}wire [${num_ports}-1:0]        ${sl_pre}${port_name}_axis_tvalid${term}
  ${out_wire}wire [${num_ports}-1:0]        ${sl_pre}${port_name}_axis_tready${term}
  ${in_wire}wire [${num_ports}*64-1:0]     ${sl_pre}${port_name}_axis_ttimestamp${term}
  ${in_wire}wire [${num_ports}-1:0]        ${sl_pre}${port_name}_axis_thas_time${term}
  ${in_wire}wire [${num_ports}*16-1:0]     ${sl_pre}${port_name}_axis_tlength${term}
  ${in_wire}wire [${num_ports}-1:0]        ${sl_pre}${port_name}_axis_teov${term}
  ${in_wire}wire [${num_ports}-1:0]        ${sl_pre}${port_name}_axis_teob${term if (term == ";") or (idx < num_inputs - 1) else ""}
%else:
  // Data Stream from User Logic: ${port_name}
  ${in_wire}wire [${port_info['item_width']}*${port_info['nipc']}-1:0]    ${sl_pre}${port_name}_axis_tdata${term}
  ${in_wire}wire [${port_info['nipc'] - 1}:0]         ${sl_pre}${port_name}_axis_tkeep${term}
  ${in_wire}wire               ${sl_pre}${port_name}_axis_tlast${term}
  ${in_wire}wire               ${sl_pre}${port_name}_axis_tvalid${term}
  ${out_wire}wire               ${sl_pre}${port_name}_axis_tready${term}
  ${in_wire}wire [63:0]        ${sl_pre}${port_name}_axis_ttimestamp${term}
  ${in_wire}wire               ${sl_pre}${port_name}_axis_thas_time${term}
  ${in_wire}wire [15:0]        ${sl_pre}${port_name}_axis_tlength${term}
  ${in_wire}wire               ${sl_pre}${port_name}_axis_teov${term}
  ${in_wire}wire               ${sl_pre}${port_name}_axis_teob${term if (term == ";") or (idx < num_outputs - 1) else ""}
%endif
%endfor
