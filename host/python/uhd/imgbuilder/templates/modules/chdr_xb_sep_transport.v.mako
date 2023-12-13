<%page args="seps, transports, routes"/>\
\
<%
  import re
  import numpy as np
  sep2xb = ""
  xb2sep = ""
  num_ports = len(seps) + len(transports)
  # Only connect the transport adapter ports that have routes enabled.
  port = 0
  for sep in reversed(list(seps.keys())):
    if np.count_nonzero(routes[port]!=0) > 0:
      sep2xb += "%s_to_xb_wire, " % sep
    else:
      sep2xb += "<nc>, "
    if np.count_nonzero(routes[:,port]!=0) > 0:
      xb2sep += "xb_to_%s_wire, " % sep
    else:
      xb2sep += "<nc>, "
    port += 1
  for transport in reversed(transports):
    if np.count_nonzero(routes[port]) > 0:
      sep2xb += "s_%s_wire, " % transport["name"]
    else:
      sep2xb += "<nc>, "
    if np.count_nonzero(routes[:,port]) > 0:
      xb2sep += "m_%s_wire, " % transport["name"]
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
