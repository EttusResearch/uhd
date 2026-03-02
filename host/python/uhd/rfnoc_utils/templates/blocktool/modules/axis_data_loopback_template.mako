<%
  in_ports = list(config['data']['inputs'].keys())
  out_ports = list(config['data']['outputs'].keys())
%>\
%for in_port, out_port in zip(in_ports, out_ports):
  assign s_${out_port}_axis_tdata      = m_${in_port}_axis_tdata;
  assign s_${out_port}_axis_tkeep      = m_${in_port}_axis_tkeep;
  assign s_${out_port}_axis_tlast      = m_${in_port}_axis_tlast;
  assign s_${out_port}_axis_tvalid     = m_${in_port}_axis_tvalid;
  assign s_${out_port}_axis_ttimestamp = m_${in_port}_axis_ttimestamp;
  assign s_${out_port}_axis_thas_time  = m_${in_port}_axis_thas_time;
  assign s_${out_port}_axis_tlength    = m_${in_port}_axis_tlength;
  assign s_${out_port}_axis_teov       = m_${in_port}_axis_teov;
  assign s_${out_port}_axis_teob       = m_${in_port}_axis_teob;
  assign m_${in_port}_axis_tready      = s_${out_port}_axis_tready;
%endfor
