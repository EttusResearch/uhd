    // CtrlPort Clock and Reset
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    // CtrlPort Master
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
%if config['control']['ctrlport']['byte_mode']:
    .m_ctrlport_req_byte_en    (m_ctrlport_req_byte_en),
%endif
%if config['control']['ctrlport']['timed']:
    .m_ctrlport_req_has_time   (m_ctrlport_req_has_time),
    .m_ctrlport_req_time       (m_ctrlport_req_time),
%endif
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
%if config['control']['ctrlport']['has_status']:
    .m_ctrlport_resp_status    (m_ctrlport_resp_status),
%endif
    .m_ctrlport_resp_data      (m_ctrlport_resp_data),
%if config['control']['interface_direction'] != "slave":
    // CtrlPort Slave
    .s_ctrlport_req_wr         (s_ctrlport_req_wr),
    .s_ctrlport_req_rd         (s_ctrlport_req_rd),
    .s_ctrlport_req_addr       (s_ctrlport_req_addr),
    .s_ctrlport_req_portid     (s_ctrlport_req_portid),
  %if config['control']['interface_direction'] == "remote_master_slave":
    .s_ctrlport_req_rem_epid   (s_ctrlport_req_rem_epid),
    .s_ctrlport_req_rem_portid (s_ctrlport_req_rem_portid),
  %endif
    .s_ctrlport_req_data       (s_ctrlport_req_data),
  %if config['control']['ctrlport']['byte_mode']:
    .s_ctrlport_req_byte_en    (s_ctrlport_req_byte_en),
  %endif
  %if config['control']['ctrlport']['timed']:
    .s_ctrlport_req_has_time   (s_ctrlport_req_has_time),
    .s_ctrlport_req_time       (s_ctrlport_req_time),
  %endif
    .s_ctrlport_resp_ack       (s_ctrlport_resp_ack),
  %if config['control']['ctrlport']['has_status']:
    .s_ctrlport_resp_status    (s_ctrlport_resp_status),
  %endif
    .s_ctrlport_resp_data      (s_ctrlport_resp_data),
%endif
