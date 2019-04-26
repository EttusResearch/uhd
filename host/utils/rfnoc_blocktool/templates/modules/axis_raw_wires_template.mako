<%page args="mode, num_inputs, num_outputs"/>\
<%
  if mode == "shell":
    sl_pre = "s_"
    ma_pre = "m_"
    in_wire = "input  "
    out_wire = "output "
    term = ","
  elif mode == "block":
    sl_pre = ""
    ma_pre = ""
    in_wire = ""
    out_wire = ""
    term = ";"
%>\
%for idx, port in enumerate(config['data']['inputs']):
  <%
    port_tmp = config['data']['inputs'][port]
  %>\
  // Payload Stream to User Logic: ${port}
  ${out_wire}wire [${port_tmp['item_width']*port_tmp['nipc']-1}:0]        ${ma_pre}${port}_payload_tdata${term}
  ${out_wire}wire [${port_tmp['nipc']-1}:0]         ${ma_pre}${port}_payload_tkeep${term}
  ${out_wire}wire               ${ma_pre}${port}_payload_tlast${term}
  ${out_wire}wire               ${ma_pre}${port}_payload_tvalid${term}
  ${in_wire}wire               ${ma_pre}${port}_payload_tready${term}
  // Context Stream to User Logic: ${port}
  ${out_wire}wire [CHDR_W-1:0]  ${ma_pre}${port}_context_tdata${term}
  ${out_wire}wire [3:0]         ${ma_pre}${port}_context_tuser${term}
  ${out_wire}wire               ${ma_pre}${port}_context_tlast${term}
  ${out_wire}wire               ${ma_pre}${port}_context_tvalid${term}
  ${in_wire}wire               ${ma_pre}${port}_context_tready${term if (term == ";") or (idx < num_inputs - 1) or (num_outputs > 0) else ""}
%endfor

%for idx, port in enumerate(config['data']['outputs']):
  <%
    port_tmp = config['data']['outputs'][port]
  %>\
  // Payload Stream from User Logic: ${port}
  ${in_wire}wire [${port_tmp['item_width'] * port_tmp['nipc'] - 1}:0]        ${sl_pre}${port}_payload_tdata${term}
  ${in_wire}wire [${port_tmp['nipc'] - 1}:0]         ${sl_pre}${port}_payload_tkeep${term}
  ${in_wire}wire               ${sl_pre}${port}_payload_tlast${term}
  ${in_wire}wire               ${sl_pre}${port}_payload_tvalid${term}
  ${out_wire}wire               ${sl_pre}${port}_payload_tready${term}
  // Context Stream from User Logic: ${port}
  ${in_wire}wire [CHDR_W-1:0]  ${sl_pre}${port}_context_tdata${term}
  ${in_wire}wire [3:0]         ${sl_pre}${port}_context_tuser${term}
  ${in_wire}wire               ${sl_pre}${port}_context_tlast${term}
  ${in_wire}wire               ${sl_pre}${port}_context_tvalid${term}
  ${out_wire}wire               ${sl_pre}${port}_context_tready${term if (term == ";") or (idx < num_outputs -1) else ""}
%endfor
