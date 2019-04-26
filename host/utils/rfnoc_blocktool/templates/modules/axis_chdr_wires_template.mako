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
    term=";"
%>\
%for idx, port in enumerate(config['data']['inputs']):
  // Payload Stream to User Logic: ${port}
  ${out_wire}wire [CHDR_W-1:0]  ${ma_pre}${port}_chdr_tdata${term}
  ${out_wire}wire               ${ma_pre}${port}_chdr_tlast${term}
  ${out_wire}wire               ${ma_pre}${port}_chdr_tvalid${term}
  ${in_wire}wire               ${ma_pre}${port}_chdr_tready${term if (term == ";") or (idx < num_inputs -1) or (num_outputs > 0) else ""}
%endfor

%for idx, port in enumerate(config['data']['outputs']):
  ${in_wire}wire [CHDR_W-1:0]  ${sl_pre}${port}_chdr_tdata${term}
  ${in_wire}wire               ${sl_pre}${port}_chdr_tlast${term}
  ${in_wire}wire               ${sl_pre}${port}_chdr_tvalid${term}
  ${out_wire}wire               ${sl_pre}${port}_chdr_tready${term if (term == ";") or (idx < num_outputs -1) else ""}
%endfor
