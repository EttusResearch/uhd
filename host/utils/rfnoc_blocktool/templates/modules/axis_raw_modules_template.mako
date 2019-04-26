
%for idx, input in enumerate(config['data']['inputs']):
  <%
    port_tmp = config['data']['inputs'][input]
  %>
  chdr_to_axis_raw_data #(
    .CHDR_W(CHDR_W),
    .ITEM_W(${port_tmp['item_width']}),
    .NIPC(${port_tmp['nipc']}),
    .SYNC_CLKS(${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),              
    .CONTEXT_FIFO_SIZE(${port_tmp['context_fifo_depth']}),      
    .PAYLOAD_FIFO_SIZE(${port_tmp['payload_fifo_depth']}),      
    .CONTEXT_PREFETCH_EN(1)     
  ) chdr_to_axis_raw_data_i${idx} (
    .axis_chdr_clk(rfnoc_chdr_clk),
    .axis_chdr_rst(rfnoc_chdr_rst),
    .axis_data_clk(axis_data_clk),
    .axis_data_rst(axis_data_rst),
    .s_axis_chdr_tdata(s_rfnoc_chdr_tdata[(${idx}*CHDR_W)+:CHDR_W]),
    .s_axis_chdr_tlast(s_rfnoc_chdr_tlast[${idx}]),  
    .s_axis_chdr_tvalid(s_rfnoc_chdr_tvalid[${idx}]),
    .s_axis_chdr_tready(s_rfnoc_chdr_tready[${idx}]),
    .m_axis_payload_tdata(m_${input}_payload_tdata),
    .m_axis_payload_tkeep(m_${input}_payload_tkeep),
    .m_axis_payload_tlast(m_${input}_payload_tlast),
    .m_axis_payload_tvalid(m_${input}_payload_tvalid),
    .m_axis_payload_tready(m_${input}_payload_tready),
    .m_axis_context_tdata(m_${input}_context_tdata),
    .m_axis_context_tuser(m_${input}_context_tuser),
    .m_axis_context_tlast(m_${input}_context_tlast),
    .m_axis_context_tvalid(m_${input}_context_tvalid),
    .m_axis_context_tready(m_${input}_context_tready),
    .flush_en(data_i_flush_en),
    .flush_timeout(data_i_flush_timeout),
    .flush_active(data_i_flush_active[${idx}]), 
    .flush_done(data_i_flush_done[${idx}])
  );
%endfor

%for idx, output in enumerate(config['data']['outputs']):
  <%
    port_tmp = config['data']['outputs'][output]
  %>
  axis_raw_data_to_chdr #(
    .CHDR_W(CHDR_W),
    .ITEM_W(${port_tmp['item_width']}),
    .NIPC(${port_tmp['nipc']}),
    .SYNC_CLKS(${1 if config['data']['clk_domain'] == "rfnoc_chdr" else 0}),              
    .CONTEXT_FIFO_SIZE(${port_tmp['context_fifo_depth']}),      
    .PAYLOAD_FIFO_SIZE(${port_tmp['payload_fifo_depth']}),      
    .CONTEXT_PREFETCH_EN(1),    
    .MTU(MTU)
  ) axis_raw_data_to_chdr_i${idx} (
    .axis_chdr_clk(rfnoc_chdr_clk),
    .axis_chdr_rst(rfnoc_chdr_rst),
    .axis_data_clk(axis_data_clk),
    .axis_data_rst(axis_data_rst),
    .m_axis_chdr_tdata(m_rfnoc_chdr_tdata[(${idx}*CHDR_W)+:CHDR_W]),
    .m_axis_chdr_tlast(m_rfnoc_chdr_tlast[${idx}]),    
    .m_axis_chdr_tvalid(m_rfnoc_chdr_tvalid[${idx}]),
    .m_axis_chdr_tready(m_rfnoc_chdr_tready[${idx}]),
    .s_axis_payload_tdata(s_${output}_payload_tdata),
    .s_axis_payload_tkeep(s_${output}_payload_tkeep),
    .s_axis_payload_tlast(s_${output}_payload_tlast),
    .s_axis_payload_tvalid(s_${output}_payload_tvalid),
    .s_axis_payload_tready(s_${output}_payload_tready),
    .s_axis_context_tdata(s_${output}_context_tdata),
    .s_axis_context_tuser(s_${output}_context_tuser),
    .s_axis_context_tlast(s_${output}_context_tlast),
    .s_axis_context_tvalid(s_${output}_context_tvalid),
    .s_axis_context_tready(s_${output}_context_tready),
    .framer_errors(),   
    .flush_en(data_o_flush_en),
    .flush_timeout(data_o_flush_timeout),
    .flush_active(data_o_flush_active[${idx}]),
    .flush_done(data_o_flush_done[${idx}])
  );
%endfor
