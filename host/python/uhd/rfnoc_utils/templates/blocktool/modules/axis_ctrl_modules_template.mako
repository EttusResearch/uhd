<%!
  import math
%>\
  axis_ctrl_endpoint #(
    .SYNC_CLKS       (${1 if config['control']['clk_domain'] == "rfnoc_ctrl" else 0}),
    .SLAVE_FIFO_SIZE ($clog2(${config['control']['fifo_depth']}))
  ) axis_ctrl_endpoint_i (
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst      (rfnoc_ctrl_rst),
    .axis_ctrl_clk       (axis_ctrl_clk),
    .axis_ctrl_rst       (axis_ctrl_rst),
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),
    .s_axis_ctrl_tdata   (s_axis_ctrl_tdata),
    .s_axis_ctrl_tlast   (s_axis_ctrl_tlast),
    .s_axis_ctrl_tvalid  (s_axis_ctrl_tvalid),
    .s_axis_ctrl_tready  (s_axis_ctrl_tready),
    .m_axis_ctrl_tdata   (m_axis_ctrl_tdata),
    .m_axis_ctrl_tlast   (m_axis_ctrl_tlast),
    .m_axis_ctrl_tvalid  (m_axis_ctrl_tvalid),
    .m_axis_ctrl_tready  (m_axis_ctrl_tready)
  );
