<%!
import math
%>\
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
    chdr_to_axis_data #(
      .CHDR_W         (CHDR_W),
      .ITEM_W         (${port_info['item_width']}),
      .NIPC           (${port_info['nipc']}),
      .SYNC_CLKS      (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
      .INFO_FIFO_SIZE ($clog2(${port_info['info_fifo_depth']})),
      .PYLD_FIFO_SIZE ($clog2(${port_info['payload_fifo_depth']}))
    ) chdr_to_axis_data_in_${port_name} (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .axis_data_clk      (axis_data_clk),
      .axis_data_rst      (axis_data_rst),
      .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[((${port_index}+i)*CHDR_W)+:CHDR_W]),
      .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[${port_index}+i]),
      .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[${port_index}+i]),
      .s_axis_chdr_tready (s_rfnoc_chdr_tready[${port_index}+i]),
      .m_axis_tdata       (m_${port_name}_axis_tdata[(${port_info['item_width']}*${port_info['nipc']})*i+:(${port_info['item_width']}*${port_info['nipc']})]),
      .m_axis_tkeep       (m_${port_name}_axis_tkeep[${port_info['nipc']}*i+:${port_info['nipc']}]),
      .m_axis_tlast       (m_${port_name}_axis_tlast[i]),
      .m_axis_tvalid      (m_${port_name}_axis_tvalid[i]),
      .m_axis_tready      (m_${port_name}_axis_tready[i]),
      .m_axis_ttimestamp  (m_${port_name}_axis_ttimestamp[64*i+:64]),
      .m_axis_thas_time   (m_${port_name}_axis_thas_time[i]),
      .m_axis_tlength     (m_${port_name}_axis_tlength[16*i+:16]),
      .m_axis_teov        (m_${port_name}_axis_teov[i]),
      .m_axis_teob        (m_${port_name}_axis_teob[i]),
      .flush_en           (data_i_flush_en),
      .flush_timeout      (data_i_flush_timeout),
      .flush_active       (data_i_flush_active[${port_index}+i]),
      .flush_done         (data_i_flush_done[${port_index}+i])
    );
  end
%else:
  chdr_to_axis_data #(
    .CHDR_W         (CHDR_W),
    .ITEM_W         (${port_info['item_width']}),
    .NIPC           (${port_info['nipc']}),
    .SYNC_CLKS      (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
    .INFO_FIFO_SIZE ($clog2(${port_info['info_fifo_depth']})),
    .PYLD_FIFO_SIZE ($clog2(${port_info['payload_fifo_depth']}))
  ) chdr_to_axis_data_in_${port_name} (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .axis_data_clk      (axis_data_clk),
    .axis_data_rst      (axis_data_rst),
    .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[(${port_index})*CHDR_W+:CHDR_W]),
    .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[${port_index}]),
    .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[${port_index}]),
    .s_axis_chdr_tready (s_rfnoc_chdr_tready[${port_index}]),
    .m_axis_tdata       (m_${port_name}_axis_tdata),
    .m_axis_tkeep       (m_${port_name}_axis_tkeep),
    .m_axis_tlast       (m_${port_name}_axis_tlast),
    .m_axis_tvalid      (m_${port_name}_axis_tvalid),
    .m_axis_tready      (m_${port_name}_axis_tready),
    .m_axis_ttimestamp  (m_${port_name}_axis_ttimestamp),
    .m_axis_thas_time   (m_${port_name}_axis_thas_time),
    .m_axis_tlength     (m_${port_name}_axis_tlength),
    .m_axis_teov        (m_${port_name}_axis_teov),
    .m_axis_teob        (m_${port_name}_axis_teob),
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
  port_info = config['data']['outputs'][port_name]
  num_ports = 1 if 'num_ports' not in port_info else port_info['num_ports']
%>\
%if num_ports != 1:
  for (i = 0; i < ${num_ports}; i = i + 1) begin: gen_output_${port_name}
    axis_data_to_chdr #(
      .CHDR_W          (CHDR_W),
      .ITEM_W          (${port_info['item_width']}),
      .NIPC            (${port_info['nipc']}),
      .SYNC_CLKS       (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
      .INFO_FIFO_SIZE  ($clog2(${port_info['info_fifo_depth']})),
      .PYLD_FIFO_SIZE  ($clog2(${port_info['payload_fifo_depth']})),
      .MTU             (MTU),
      .SIDEBAND_AT_END (${int(port_info['sideband_at_end']) if 'sideband_at_end' in port_info else 1})
    ) axis_data_to_chdr_out_${port_name} (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .axis_data_clk      (axis_data_clk),
      .axis_data_rst      (axis_data_rst),
      .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(${port_index}+i)*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[${port_index}+i]),
      .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[${port_index}+i]),
      .m_axis_chdr_tready (m_rfnoc_chdr_tready[${port_index}+i]),
      .s_axis_tdata       (s_${port_name}_axis_tdata[(${port_info['item_width']}*${port_info['nipc']})*i+:(${port_info['item_width']}*${port_info['nipc']})]),
      .s_axis_tkeep       (s_${port_name}_axis_tkeep[${port_info['nipc']}*i+:${port_info['nipc']}]),
      .s_axis_tlast       (s_${port_name}_axis_tlast[i]),
      .s_axis_tvalid      (s_${port_name}_axis_tvalid[i]),
      .s_axis_tready      (s_${port_name}_axis_tready[i]),
      .s_axis_ttimestamp  (s_${port_name}_axis_ttimestamp[64*i+:64]),
      .s_axis_thas_time   (s_${port_name}_axis_thas_time[i]),
      .s_axis_tlength     (s_${port_name}_axis_tlength[16*i+:16]),
      .s_axis_teov        (s_${port_name}_axis_teov[i]),
      .s_axis_teob        (s_${port_name}_axis_teob[i]),
      .flush_en           (data_o_flush_en),
      .flush_timeout      (data_o_flush_timeout),
      .flush_active       (data_o_flush_active[${port_index}+i]),
      .flush_done         (data_o_flush_done[${port_index}+i])
    );
  end
%else:
  axis_data_to_chdr #(
    .CHDR_W          (CHDR_W),
    .ITEM_W          (${port_info['item_width']}),
    .NIPC            (${port_info['nipc']}),
    .SYNC_CLKS       (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
    .INFO_FIFO_SIZE  ($clog2(${port_info['info_fifo_depth']})),
    .PYLD_FIFO_SIZE  ($clog2(${port_info['payload_fifo_depth']})),
    .MTU             (MTU),
    .SIDEBAND_AT_END (${int(port_info['sideband_at_end']) if 'sideband_at_end' in port_info else 1})
  ) axis_data_to_chdr_out_${port_name} (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .axis_data_clk      (axis_data_clk),
    .axis_data_rst      (axis_data_rst),
    .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(${port_index})*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[${port_index}]),
    .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[${port_index}]),
    .m_axis_chdr_tready (m_rfnoc_chdr_tready[${port_index}]),
    .s_axis_tdata       (s_${port_name}_axis_tdata),
    .s_axis_tkeep       (s_${port_name}_axis_tkeep),
    .s_axis_tlast       (s_${port_name}_axis_tlast),
    .s_axis_tvalid      (s_${port_name}_axis_tvalid),
    .s_axis_tready      (s_${port_name}_axis_tready),
    .s_axis_ttimestamp  (s_${port_name}_axis_ttimestamp),
    .s_axis_thas_time   (s_${port_name}_axis_thas_time),
    .s_axis_tlength     (s_${port_name}_axis_tlength),
    .s_axis_teov        (s_${port_name}_axis_teov),
    .s_axis_teob        (s_${port_name}_axis_teob),
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
