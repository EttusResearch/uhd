<%
  # Get the number of input and outputs port names
  num_inputs  = len(config['data']['inputs'])
  num_outputs = len(config['data']['outputs'])
%>\
    // AXI-Stream Clock and Reset
    .axis_data_clk (axis_data_clk),
    .axis_data_rst (axis_data_rst),
%for idx, port_name in enumerate(config['data']['inputs']):
    // Data Stream to User Logic: ${port_name}
    .m_${port_name}_axis_tdata      (m_${port_name}_axis_tdata),
    .m_${port_name}_axis_tkeep      (m_${port_name}_axis_tkeep),
    .m_${port_name}_axis_tlast      (m_${port_name}_axis_tlast),
    .m_${port_name}_axis_tvalid     (m_${port_name}_axis_tvalid),
    .m_${port_name}_axis_tready     (m_${port_name}_axis_tready),
    .m_${port_name}_axis_ttimestamp (m_${port_name}_axis_ttimestamp),
    .m_${port_name}_axis_thas_time  (m_${port_name}_axis_thas_time),
    .m_${port_name}_axis_tlength    (m_${port_name}_axis_tlength),
    .m_${port_name}_axis_teov       (m_${port_name}_axis_teov),
    .m_${port_name}_axis_teob       (m_${port_name}_axis_teob)${"," if (idx < num_inputs - 1) or (num_outputs > 0) else ""}
%endfor
%for idx, port_name in enumerate(config['data']['outputs']):
    // Data Stream from User Logic: ${port_name}
    .s_${port_name}_axis_tdata      (s_${port_name}_axis_tdata),
    .s_${port_name}_axis_tkeep      (s_${port_name}_axis_tkeep),
    .s_${port_name}_axis_tlast      (s_${port_name}_axis_tlast),
    .s_${port_name}_axis_tvalid     (s_${port_name}_axis_tvalid),
    .s_${port_name}_axis_tready     (s_${port_name}_axis_tready),
    .s_${port_name}_axis_ttimestamp (s_${port_name}_axis_ttimestamp),
    .s_${port_name}_axis_thas_time  (s_${port_name}_axis_thas_time),
    .s_${port_name}_axis_tlength    (s_${port_name}_axis_tlength),
    .s_${port_name}_axis_teov       (s_${port_name}_axis_teov),
    .s_${port_name}_axis_teob       (s_${port_name}_axis_teob)${"," if (idx < num_outputs -1) else ""}
%endfor
