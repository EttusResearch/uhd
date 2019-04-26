%for idx, input in enumerate(config['data']['inputs']):
  chdr_to_chdr_data #(
    .CHDR_W(CHDR_W)
  ) chdr_to_chdr_data_i${idx} (
    .axis_chdr_clk(rfnoc_chdr_clk),
    .axis_chdr_rst(rfnoc_chdr_rst),
    .s_axis_chdr_tdata(s_rfnoc_chdr_tdata[(${idx}*CHDR_W)+:CHDR_W]),
    .s_axis_chdr_tlast(s_rfnoc_chdr_tlast[${idx}]),  
    .s_axis_chdr_tvalid(s_rfnoc_chdr_tvalid[${idx}]),
    .s_axis_chdr_tready(s_rfnoc_chdr_tready[${idx}]),
    .m_axis_chdr_tdata(m_${input}_chdr_tdata),
    .m_axis_chdr_tlast(m_${input}_chdr_tlast),
    .m_axis_chdr_tvalid(m_${input}_chdr_tvalid),
    .m_axis_chdr_tready(m_${input}_chdr_tready),
    .flush_en(data_i_flush_en),
    .flush_timeout(data_i_flush_timeout),
    .flush_active(data_i_flush_active[${idx}]),
    .flush_done(data_i_flush_done[${idx}])
  );
%endfor

%for idx, output in enumerate(config['data']['outputs']):
  chdr_to_chdr_data #(
    .CHDR_W(CHDR_W)
  ) chdr_to_chdr_data_o${idx} (
    .axis_chdr_clk(rfnoc_chdr_clk),
    .axis_chdr_rst(rfnoc_chdr_rst),
    .m_axis_chdr_tdata(m_rfnoc_chdr_tdata[(${idx}*CHDR_W)+:CHDR_W]),
    .m_axis_chdr_tlast(m_rfnoc_chdr_tlast[${idx}]),    
    .m_axis_chdr_tvalid(m_rfnoc_chdr_tvalid[${idx}]),
    .m_axis_chdr_tready(m_rfnoc_chdr_tready[${idx}]),
    .s_axis_chdr_tdata(s_${output}_chdr_tdata),
    .s_axis_chdr_tlast(s_${output}_chdr_tlast),
    .s_axis_chdr_tvalid(s_${output}_chdr_tvalid),
    .s_axis_chdr_tready(s_${output}_chdr_tready),
    .flush_en(data_o_flush_en),
    .flush_timeout(data_o_flush_timeout),
    .flush_active(data_o_flush_active[${idx}]),
    .flush_done(data_o_flush_done[${idx}])
  );
%endfor
