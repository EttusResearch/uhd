<%
  # Get the number of input and outputs port names
  num_inputs  = len(config['data']['inputs'])
  num_outputs = len(config['data']['outputs'])
%>\
    // AXIS-CHDR Clock and Reset
    .axis_chdr_clk (axis_chdr_clk),
    .axis_chdr_rst (axis_chdr_rst),
    // AXIS-CHDR to User Logic
%for idx, port_name in enumerate(config['data']['inputs']):
    .m_${port_name}_chdr_tdata  (m_${port_name}_chdr_tdata),
    .m_${port_name}_chdr_tlast  (m_${port_name}_chdr_tlast),
    .m_${port_name}_chdr_tvalid (m_${port_name}_chdr_tvalid),
    .m_${port_name}_chdr_tready (m_${port_name}_chdr_tready)${"," if (idx < num_inputs -1) or (num_outputs > 0) else ""}
%endfor
    // AXIS-CHDR from User Logic
%for idx, port_name in enumerate(config['data']['outputs']):
    .s_${port_name}_chdr_tdata  (s_${port_name}_chdr_tdata),
    .s_${port_name}_chdr_tlast  (s_${port_name}_chdr_tlast),
    .s_${port_name}_chdr_tvalid (s_${port_name}_chdr_tvalid),
    .s_${port_name}_chdr_tready (s_${port_name}_chdr_tready)${"," if (idx < num_outputs -1) else ""}
%endfor
