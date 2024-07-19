<%page args="config, routes"/>\
\
<%
  import re
  import numpy as np
  sep2xb = ""
  xb2sep = ""
  # Only connect the transport adapter ports that have routes enabled.
  port = 0
  for sep in reversed(list(config.stream_endpoints.keys())):
    if np.count_nonzero(routes[port]!=0) > 0:
      sep2xb += "%s_to_xb_wire, " % sep
    else:
      sep2xb += "<nc>, "
    if np.count_nonzero(routes[:,port]!=0) > 0:
      xb2sep += "xb_to_%s_wire, " % sep
    else:
      xb2sep += "<nc>, "
    port += 1
  if config.get_ta_gen_mode() == 'fixed':
    for transport in reversed(config.device.transports):
      if np.count_nonzero(routes[port]) > 0:
        sep2xb += "s_%s_wire, " % transport["name"]
      else:
        sep2xb += "<nc>, "
      if np.count_nonzero(routes[:,port]) > 0:
        xb2sep += "m_%s_wire, " % transport["name"]
      else:
        xb2sep += "<nc>, "
      port += 1
  else:
    for ta_info in reversed(config.get_ta_info()):
      for port_i in reversed(range(ta_info['num_ports'])):
        if np.count_nonzero(routes[port]) > 0:
          sep2xb += f"{ta_info['name']}_{port_i}_to_xb_wire, "
        else:
          sep2xb += "<nc>, "
        if np.count_nonzero(routes[:,port]) > 0:
          xb2sep += f"xb_to_{ta_info['name']}_{port_i}_wire, "
        else:
          xb2sep += "<nc>, "
        port += 1

  sep2xb = sep2xb[:-2]
  xb2sep = xb2sep[:-2]
%>\
    .s_axis_tdata   ({${re.sub("<nc>", "{PORT_W{1'b0}}", re.sub("wire", "tdata ", sep2xb))}}),
    .s_axis_tlast   ({${re.sub("<nc>", "          1'b0", re.sub("wire", "tlast ", sep2xb))}}),
    .s_axis_tvalid  ({${re.sub("<nc>", "          1'b0", re.sub("wire", "tvalid", sep2xb))}}),
    .s_axis_tready  ({${re.sub("<nc>", "          1'bZ", re.sub("wire", "tready", sep2xb))}}),
    .m_axis_tdata   ({${re.sub("<nc>", "{PORT_W{1'bZ}}", re.sub("wire", "tdata ", xb2sep))}}),
    .m_axis_tlast   ({${re.sub("<nc>", "          1'bZ", re.sub("wire", "tlast ", xb2sep))}}),
    .m_axis_tvalid  ({${re.sub("<nc>", "          1'bZ", re.sub("wire", "tvalid", xb2sep))}}),
    .m_axis_tready  ({${re.sub("<nc>", "          1'b1", re.sub("wire", "tready", xb2sep))}}),
