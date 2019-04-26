<%page args="num_inputs, num_outputs"/>\
\
%for idx, input in enumerate(config['data']['inputs']):
  // Payload Stream to User Logic: ${input}
  .m_${input}_payload_tdata(${input}_payload_tdata),
  .m_${input}_payload_tkeep(${input}_payload_tkeep),
  .m_${input}_payload_tlast(${input}_payload_tlast),
  .m_${input}_payload_tvalid(${input}_payload_tvalid),
  .m_${input}_payload_tready(${input}_payload_tready),
  // Context Stream to User Logic: ${input}
  .m_${input}_context_tdata(${input}_context_tdata),
  .m_${input}_context_tuser(${input}_context_tuser),
  .m_${input}_context_tlast(${input}_context_tlast),
  .m_${input}_context_tvalid(${input}_context_tvalid),
  .m_${input}_context_tready(${input}_context_tready)${"," if (idx < num_inputs - 1) or (num_outputs > 0) else ""}
%endfor

%for idx, output in enumerate(config['data']['outputs']):
  // Payload Stream from User Logic: ${output}
  .s_${output}_payload_tdata(${output}_payload_tdata),
  .s_${output}_payload_tkeep(${output}_payload_tkeep),
  .s_${output}_payload_tlast(${output}_payload_tlast),
  .s_${output}_payload_tvalid(${output}_payload_tvalid),
  .s_${output}_payload_tready(${output}_payload_tready),
  // Context Stream from User Logic: ${output}
  .s_${output}_context_tdata(${output}_context_tdata),
  .s_${output}_context_tuser(${output}_context_tuser),
  .s_${output}_context_tlast(${output}_context_tlast),
  .s_${output}_context_tvalid(${output}_context_tvalid),
  .s_${output}_context_tready(${output}_context_tready)${"," if (idx < num_outputs -1) else ""}
%endfor
