<%page args="num_inputs, num_outputs"/>

%for idx, input in enumerate(config['data']['inputs']):
  .m_${input}_chdr_tdata(${input}_chdr_tdata),
  .m_${input}_chdr_tlast(${input}_chdr_tlast),
  .m_${input}_chdr_tvalid(${input}_chdr_tvalid),
  .m_${input}_chdr_tready(${input}_chdr_tready)${"," if (idx < num_inputs -1) or (num_outputs > 0) else ""}
%endfor

%for idx, output in enumerate(config['data']['outputs']):
  .s_${output}_chdr_tdata(${output}_chdr_tdata),
  .s_${output}_chdr_tlast(${output}_chdr_tlast),
  .s_${output}_chdr_tvalid(${output}_chdr_tvalid),
  .s_${output}_chdr_tready(${output}_chdr_tready)${"," if (idx < num_outputs -1) else ""}
%endfor
