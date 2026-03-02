<%
  in_ports = list(config['data']['inputs'].keys())
  out_ports = list(config['data']['outputs'].keys())
%>\
%for in_port, out_port in zip(in_ports, out_ports):
  assign s_${out_port}_chdr_tdata  = m_${in_port}_chdr_tdata;
  assign s_${out_port}_chdr_tlast  = m_${in_port}_chdr_tlast;
  assign s_${out_port}_chdr_tvalid = m_${in_port}_chdr_tvalid;
  assign m_${in_port}_chdr_tready  = s_${out_port}_chdr_tready;
%endfor
