<%
  in_ports = list(config['data']['inputs'].keys())
  out_ports = list(config['data']['outputs'].keys())
%>\
%for in_port, out_port in zip(in_ports, out_ports):
  // Payload loopback
  assign s_${out_port}_payload_tdata  = m_${in_port}_payload_tdata;
  assign s_${out_port}_payload_tkeep  = m_${in_port}_payload_tkeep;
  assign s_${out_port}_payload_tlast  = m_${in_port}_payload_tlast;
  assign s_${out_port}_payload_tvalid = m_${in_port}_payload_tvalid;
  assign m_${in_port}_payload_tready  = s_${out_port}_payload_tready;
  // Context loopback
  assign s_${out_port}_context_tdata  = m_${in_port}_context_tdata;
  assign s_${out_port}_context_tuser  = m_${in_port}_context_tuser;
  assign s_${out_port}_context_tlast  = m_${in_port}_context_tlast;
  assign s_${out_port}_context_tvalid = m_${in_port}_context_tvalid;
  assign m_${in_port}_context_tready  = s_${out_port}_context_tready;
%endfor
