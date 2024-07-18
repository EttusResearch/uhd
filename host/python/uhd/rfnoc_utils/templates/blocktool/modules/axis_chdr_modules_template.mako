  //---------------------
  // Input Data Paths
  //---------------------

<%
  port_index = '0'
%>\
%for port_name, port_info in config['data']['inputs'].items():
<%
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
%if num_ports != 1:
  for (i = 0; i < ${num_ports}; i = i + 1) begin: gen_input_${port_name}
    chdr_to_chdr_data #(
      .CHDR_W (CHDR_W)
    ) chdr_to_chdr_data_in_${port_name} (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[(${port_index}+i)*CHDR_W+:CHDR_W]),
      .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[${port_index}+i]),
      .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[${port_index}+i]),
      .s_axis_chdr_tready (s_rfnoc_chdr_tready[${port_index}+i]),
      .m_axis_chdr_tdata  (m_${port_name}_chdr_tdata[i*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_${port_name}_chdr_tlast[i]),
      .m_axis_chdr_tvalid (m_${port_name}_chdr_tvalid[i]),
      .m_axis_chdr_tready (m_${port_name}_chdr_tready[i]),
      .flush_en           (data_i_flush_en),
      .flush_timeout      (data_i_flush_timeout),
      .flush_active       (data_i_flush_active[${port_index}+i]),
      .flush_done         (data_i_flush_done[${port_index}+i])
    );
  end
%else:
  chdr_to_chdr_data #(
    .CHDR_W (CHDR_W)
  ) chdr_to_chdr_data_in_${port_name} (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[(${port_index})*CHDR_W+:CHDR_W]),
    .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[${port_index}]),
    .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[${port_index}]),
    .s_axis_chdr_tready (s_rfnoc_chdr_tready[${port_index}]),
    .m_axis_chdr_tdata  (m_${port_name}_chdr_tdata),
    .m_axis_chdr_tlast  (m_${port_name}_chdr_tlast),
    .m_axis_chdr_tvalid (m_${port_name}_chdr_tvalid),
    .m_axis_chdr_tready (m_${port_name}_chdr_tready),
    .flush_en           (data_i_flush_en),
    .flush_timeout      (data_i_flush_timeout),
    .flush_active       (data_i_flush_active[${port_index}]),
    .flush_done         (data_i_flush_done[${port_index}])
  );
%endif

<%
  port_index = port_index + '+' + str(num_ports) if (port_index != '0') else str(num_ports)
%>\
%endfor
  //---------------------
  // Output Data Paths
  //---------------------

<%
  port_index = '0'
%>\
%for port_name, port_info in config['data']['outputs'].items():
<%
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
%if num_ports != 1:
  for (i = 0; i < ${num_ports}; i = i + 1) begin: gen_output_${port_name}
    chdr_to_chdr_data #(
      .CHDR_W (CHDR_W)
    ) chdr_to_chdr_data_out_${port_name} (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(${port_index}+i)*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[${port_index}+i]),
      .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[${port_index}+i]),
      .m_axis_chdr_tready (m_rfnoc_chdr_tready[${port_index}+i]),
      .s_axis_chdr_tdata  (s_${port_name}_chdr_tdata[i*CHDR_W+:CHDR_W]),
      .s_axis_chdr_tlast  (s_${port_name}_chdr_tlast[i]),
      .s_axis_chdr_tvalid (s_${port_name}_chdr_tvalid[i]),
      .s_axis_chdr_tready (s_${port_name}_chdr_tready[i]),
      .flush_en           (data_o_flush_en),
      .flush_timeout      (data_o_flush_timeout),
      .flush_active       (data_o_flush_active[${port_index}+i]),
      .flush_done         (data_o_flush_done[${port_index}+i])
    );
  end
%else:
  chdr_to_chdr_data #(
    .CHDR_W (CHDR_W)
  ) chdr_to_chdr_data_out_${port_name} (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(${port_index})*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[${port_index}]),
    .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[${port_index}]),
    .m_axis_chdr_tready (m_rfnoc_chdr_tready[${port_index}]),
    .s_axis_chdr_tdata  (s_${port_name}_chdr_tdata),
    .s_axis_chdr_tlast  (s_${port_name}_chdr_tlast),
    .s_axis_chdr_tvalid (s_${port_name}_chdr_tvalid),
    .s_axis_chdr_tready (s_${port_name}_chdr_tready),
    .flush_en           (data_o_flush_en),
    .flush_timeout      (data_o_flush_timeout),
    .flush_active       (data_o_flush_active[${port_index}]),
    .flush_done         (data_o_flush_done[${port_index}])
  );
%endif

<%
  port_index = port_index + '+' + str(num_ports) if (port_index != '0') else str(num_ports)
%>\
%endfor