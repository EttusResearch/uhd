<%page args="mode"/>\
\
<%
  if mode == "shell":
    sl_pre = "s_"
    ma_pre = "m_"
    sl_wire = "input  "
    ma_wire = "output "
    term = ","
  elif mode == "block":
    sl_pre = "s_"
    ma_pre = "m_"
    sl_wire = ""
    ma_wire = ""
    term = ";"
%>\
  // CtrlPort Master
  ${ma_wire}wire               ${ma_pre}ctrlport_req_wr${term}
  ${ma_wire}wire               ${ma_pre}ctrlport_req_rd${term}
  ${ma_wire}wire [19:0]        ${ma_pre}ctrlport_req_addr${term}
  ${ma_wire}wire [31:0]        ${ma_pre}ctrlport_req_data${term}
%if config['control']['ctrlport']['byte_mode']:
  ${ma_wire}wire [3:0]         ${ma_pre}ctrlport_req_byte_en${term}
%endif
%if config['control']['ctrlport']['timed']:
  ${ma_wire}wire               ${ma_pre}ctrlport_req_has_time${term}
  ${ma_wire}wire [63:0]        ${ma_pre}ctrlport_req_time${term}
%endif
  ${sl_wire}wire               ${ma_pre}ctrlport_resp_ack${term}
%if config['control']['ctrlport']['has_status']:
  ${sl_wire}wire [1:0]         ${ma_pre}ctrlport_resp_status${term}
%endif
  ${sl_wire}wire [31:0]        ${ma_pre}ctrlport_resp_data${term}
%if config['control']['interface_direction'] != "slave":
  // CtrlPort Slave
  ${sl_wire}wire               ${sl_pre}ctrlport_req_wr${term}
  ${sl_wire}wire               ${sl_pre}ctrlport_req_rd${term}
  ${sl_wire}wire [19:0]        ${sl_pre}ctrlport_req_addr${term}
  ${sl_wire}wire [9:0]         ${sl_pre}ctrlport_req_portid${term}
  %if config['control']['interface_direction'] == "remote_master_slave":
  ${sl_wire}wire [15:0]        ${sl_pre}ctrlport_req_rem_epid${term}
  ${sl_wire}wire [9:0]         ${sl_pre}ctrlport_req_rem_portid${term}
  %endif
  ${sl_wire}wire [31:0]        ${sl_pre}ctrlport_req_data${term}
  %if config['control']['ctrlport']['byte_mode']:
  ${sl_wire}wire [3:0]         ${sl_pre}ctrlport_req_byte_en${term}
  %endif
  %if config['control']['ctrlport']['timed']:
  ${sl_wire}wire               ${sl_pre}ctrlport_req_has_time${term}
  ${sl_wire}wire [63:0]        ${sl_pre}ctrlport_req_time${term}
  %endif
  ${ma_wire}wire               ${sl_pre}ctrlport_resp_ack${term}
  %if config['control']['ctrlport']['has_status']:
  ${ma_wire}wire [1:0]         ${sl_pre}ctrlport_resp_status${term}
  %endif
  ${ma_wire}wire [31:0]        ${sl_pre}ctrlport_resp_data${term}
%endif
