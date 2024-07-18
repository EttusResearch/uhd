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
    chdr_to_axis_pyld_ctxt #(
      .CHDR_W              (CHDR_W),
      .ITEM_W              (${port_info['item_width']}),
      .NIPC                (${port_info['nipc']}),
      .SYNC_CLKS           (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
      .CONTEXT_FIFO_SIZE   ($clog2(${port_info['context_fifo_depth']})),
      .PAYLOAD_FIFO_SIZE   ($clog2(${port_info['payload_fifo_depth']})),
      .CONTEXT_PREFETCH_EN (1)
    ) chdr_to_axis_pyld_ctxt_in_${port_name} (
      .axis_chdr_clk         (rfnoc_chdr_clk),
      .axis_chdr_rst         (rfnoc_chdr_rst),
      .axis_data_clk         (axis_data_clk),
      .axis_data_rst         (axis_data_rst),
      .s_axis_chdr_tdata     (s_rfnoc_chdr_tdata[((${port_index}+i)*CHDR_W)+:CHDR_W]),
      .s_axis_chdr_tlast     (s_rfnoc_chdr_tlast[${port_index}+i]),
      .s_axis_chdr_tvalid    (s_rfnoc_chdr_tvalid[${port_index}+i]),
      .s_axis_chdr_tready    (s_rfnoc_chdr_tready[${port_index}+i]),
      .m_axis_payload_tdata  (m_${port_name}_payload_tdata[(${port_info['item_width']}*${port_info['nipc']})*i+:(${port_info['item_width']}*${port_info['nipc']})]),
      .m_axis_payload_tkeep  (m_${port_name}_payload_tkeep[${port_info['nipc']}*i+:${port_info['nipc']}]),
      .m_axis_payload_tlast  (m_${port_name}_payload_tlast[i]),
      .m_axis_payload_tvalid (m_${port_name}_payload_tvalid[i]),
      .m_axis_payload_tready (m_${port_name}_payload_tready[i]),
      .m_axis_context_tdata  (m_${port_name}_context_tdata[CHDR_W*i+:CHDR_W]),
      .m_axis_context_tuser  (m_${port_name}_context_tuser[4*i+:4]),
      .m_axis_context_tlast  (m_${port_name}_context_tlast[i]),
      .m_axis_context_tvalid (m_${port_name}_context_tvalid[i]),
      .m_axis_context_tready (m_${port_name}_context_tready[i]),
      .flush_en              (data_i_flush_en),
      .flush_timeout         (data_i_flush_timeout),
      .flush_active          (data_i_flush_active[${port_index}+i]),
      .flush_done            (data_i_flush_done[${port_index}+i])
    );
  end
%else:
  chdr_to_axis_pyld_ctxt #(
    .CHDR_W              (CHDR_W),
    .ITEM_W              (${port_info['item_width']}),
    .NIPC                (${port_info['nipc']}),
    .SYNC_CLKS           (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
    .CONTEXT_FIFO_SIZE   ($clog2(${port_info['context_fifo_depth']})),
    .PAYLOAD_FIFO_SIZE   ($clog2(${port_info['payload_fifo_depth']})),
    .CONTEXT_PREFETCH_EN (1)
  ) chdr_to_axis_pyld_ctxt_in_${port_name} (
    .axis_chdr_clk         (rfnoc_chdr_clk),
    .axis_chdr_rst         (rfnoc_chdr_rst),
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    .s_axis_chdr_tdata     (s_rfnoc_chdr_tdata[(${port_index})*CHDR_W+:CHDR_W]),
    .s_axis_chdr_tlast     (s_rfnoc_chdr_tlast[${port_index}]),
    .s_axis_chdr_tvalid    (s_rfnoc_chdr_tvalid[${port_index}]),
    .s_axis_chdr_tready    (s_rfnoc_chdr_tready[${port_index}]),
    .m_axis_payload_tdata  (m_${port_name}_payload_tdata),
    .m_axis_payload_tkeep  (m_${port_name}_payload_tkeep),
    .m_axis_payload_tlast  (m_${port_name}_payload_tlast),
    .m_axis_payload_tvalid (m_${port_name}_payload_tvalid),
    .m_axis_payload_tready (m_${port_name}_payload_tready),
    .m_axis_context_tdata  (m_${port_name}_context_tdata),
    .m_axis_context_tuser  (m_${port_name}_context_tuser),
    .m_axis_context_tlast  (m_${port_name}_context_tlast),
    .m_axis_context_tvalid (m_${port_name}_context_tvalid),
    .m_axis_context_tready (m_${port_name}_context_tready),
    .flush_en              (data_i_flush_en),
    .flush_timeout         (data_i_flush_timeout),
    .flush_active          (data_i_flush_active[${port_index}]),
    .flush_done            (data_i_flush_done[${port_index}])
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
    axis_pyld_ctxt_to_chdr #(
      .CHDR_W              (CHDR_W),
      .ITEM_W              (${port_info['item_width']}),
      .NIPC                (${port_info['nipc']}),
      .SYNC_CLKS           (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
      .CONTEXT_FIFO_SIZE   ($clog2(${port_info['context_fifo_depth']})),
      .PAYLOAD_FIFO_SIZE   ($clog2(${port_info['payload_fifo_depth']})),
      .MTU                 (MTU),
      .CONTEXT_PREFETCH_EN (1)
    ) axis_pyld_ctxt_to_chdr_out_${port_name} (
      .axis_chdr_clk         (rfnoc_chdr_clk),
      .axis_chdr_rst         (rfnoc_chdr_rst),
      .axis_data_clk         (axis_data_clk),
      .axis_data_rst         (axis_data_rst),
      .m_axis_chdr_tdata     (m_rfnoc_chdr_tdata[(${port_index}+i)*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast     (m_rfnoc_chdr_tlast[${port_index}+i]),
      .m_axis_chdr_tvalid    (m_rfnoc_chdr_tvalid[${port_index}+i]),
      .m_axis_chdr_tready    (m_rfnoc_chdr_tready[${port_index}+i]),
      .s_axis_payload_tdata  (s_${port_name}_payload_tdata[(${port_info['item_width']}*${port_info['nipc']})*i+:(${port_info['item_width']}*${port_info['nipc']})]),
      .s_axis_payload_tkeep  (s_${port_name}_payload_tkeep[${port_info['nipc']}*i+:${port_info['nipc']}]),
      .s_axis_payload_tlast  (s_${port_name}_payload_tlast[i]),
      .s_axis_payload_tvalid (s_${port_name}_payload_tvalid[i]),
      .s_axis_payload_tready (s_${port_name}_payload_tready[i]),
      .s_axis_context_tdata  (s_${port_name}_context_tdata[CHDR_W*i+:CHDR_W]),
      .s_axis_context_tuser  (s_${port_name}_context_tuser[4*i+:4]),
      .s_axis_context_tlast  (s_${port_name}_context_tlast[i]),
      .s_axis_context_tvalid (s_${port_name}_context_tvalid[i]),
      .s_axis_context_tready (s_${port_name}_context_tready[i]),
      .framer_errors         (),
      .flush_en              (data_o_flush_en),
      .flush_timeout         (data_o_flush_timeout),
      .flush_active          (data_o_flush_active[${port_index}+i]),
      .flush_done            (data_o_flush_done[${port_index}+i])
    );
  end
%else:
  axis_pyld_ctxt_to_chdr #(
    .CHDR_W              (CHDR_W),
    .ITEM_W              (${port_info['item_width']}),
    .NIPC                (${port_info['nipc']}),
    .SYNC_CLKS           (${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),
    .CONTEXT_FIFO_SIZE   ($clog2(${port_info['context_fifo_depth']})),
    .PAYLOAD_FIFO_SIZE   ($clog2(${port_info['payload_fifo_depth']})),
    .MTU                 (MTU),
    .CONTEXT_PREFETCH_EN (1)
  ) axis_pyld_ctxt_to_chdr_out_${port_name} (
    .axis_chdr_clk         (rfnoc_chdr_clk),
    .axis_chdr_rst         (rfnoc_chdr_rst),
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    .m_axis_chdr_tdata     (m_rfnoc_chdr_tdata[(${port_index})*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast     (m_rfnoc_chdr_tlast[${port_index}]),
    .m_axis_chdr_tvalid    (m_rfnoc_chdr_tvalid[${port_index}]),
    .m_axis_chdr_tready    (m_rfnoc_chdr_tready[${port_index}]),
    .s_axis_payload_tdata  (s_${port_name}_payload_tdata),
    .s_axis_payload_tkeep  (s_${port_name}_payload_tkeep),
    .s_axis_payload_tlast  (s_${port_name}_payload_tlast),
    .s_axis_payload_tvalid (s_${port_name}_payload_tvalid),
    .s_axis_payload_tready (s_${port_name}_payload_tready),
    .s_axis_context_tdata  (s_${port_name}_context_tdata),
    .s_axis_context_tuser  (s_${port_name}_context_tuser),
    .s_axis_context_tlast  (s_${port_name}_context_tlast),
    .s_axis_context_tvalid (s_${port_name}_context_tvalid),
    .s_axis_context_tready (s_${port_name}_context_tready),
    .framer_errors         (),
    .flush_en              (data_o_flush_en),
    .flush_timeout         (data_o_flush_timeout),
    .flush_active          (data_o_flush_active[${port_index}]),
    .flush_done            (data_o_flush_done[${port_index}])
  );
%endif

<%
  port_index = port_index + '+' + str(num_ports) if (port_index != '0') else str(num_ports)
%>\
%endfor
