<%
  # Get the number of input and outputs port names
  num_inputs  = len(config['data']['inputs'])
  num_outputs = len(config['data']['outputs'])
%>\
    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk (axis_data_clk),
    .axis_data_rst (axis_data_rst),
%for idx, port_name in enumerate(config['data']['inputs']):
    // Payload Stream to User Logic: ${port_name}
    .m_${port_name}_payload_tdata  (m_${port_name}_payload_tdata),
    .m_${port_name}_payload_tkeep  (m_${port_name}_payload_tkeep),
    .m_${port_name}_payload_tlast  (m_${port_name}_payload_tlast),
    .m_${port_name}_payload_tvalid (m_${port_name}_payload_tvalid),
    .m_${port_name}_payload_tready (m_${port_name}_payload_tready),
    // Context Stream to User Logic: ${port_name}
    .m_${port_name}_context_tdata  (m_${port_name}_context_tdata),
    .m_${port_name}_context_tuser  (m_${port_name}_context_tuser),
    .m_${port_name}_context_tlast  (m_${port_name}_context_tlast),
    .m_${port_name}_context_tvalid (m_${port_name}_context_tvalid),
    .m_${port_name}_context_tready (m_${port_name}_context_tready)${"," if (idx < num_inputs - 1) or (num_outputs > 0) else ""}
%endfor
%for idx, port_name in enumerate(config['data']['outputs']):
    // Payload Stream from User Logic: ${port_name}
    .s_${port_name}_payload_tdata  (s_${port_name}_payload_tdata),
    .s_${port_name}_payload_tkeep  (s_${port_name}_payload_tkeep),
    .s_${port_name}_payload_tlast  (s_${port_name}_payload_tlast),
    .s_${port_name}_payload_tvalid (s_${port_name}_payload_tvalid),
    .s_${port_name}_payload_tready (s_${port_name}_payload_tready),
    // Context Stream from User Logic: ${port_name}
    .s_${port_name}_context_tdata  (s_${port_name}_context_tdata),
    .s_${port_name}_context_tuser  (s_${port_name}_context_tuser),
    .s_${port_name}_context_tlast  (s_${port_name}_context_tlast),
    .s_${port_name}_context_tvalid (s_${port_name}_context_tvalid),
    .s_${port_name}_context_tready (s_${port_name}_context_tready)${"," if (idx < num_outputs -1) else ""}
%endfor
