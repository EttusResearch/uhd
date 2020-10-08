<%page args="seps, transports"/>\
\
<%
  import re
  sep2xb = ""
  xb2sep = ""
  for sep in reversed(list(seps.keys())):
    sep2xb += "%s_to_xb_wire, " % sep
    xb2sep += "xb_to_%s_wire, " % sep
  for transport in reversed(transports):
    sep2xb += "s_%s_wire, " % transport["name"]
    xb2sep += "m_%s_wire, " % transport["name"]
  sep2xb = sep2xb[:-2]
  xb2sep = xb2sep[:-2]
%>\
    .s_axis_tdata   ({${re.sub("wire", "tdata ", sep2xb)}}),
    .s_axis_tlast   ({${re.sub("wire", "tlast ", sep2xb)}}),
    .s_axis_tvalid  ({${re.sub("wire", "tvalid", sep2xb)}}),
    .s_axis_tready  ({${re.sub("wire", "tready", sep2xb)}}),
    .m_axis_tdata   ({${re.sub("wire", "tdata ", xb2sep)}}),
    .m_axis_tlast   ({${re.sub("wire", "tlast ", xb2sep)}}),
    .m_axis_tvalid  ({${re.sub("wire", "tvalid", xb2sep)}}),
    .m_axis_tready  ({${re.sub("wire", "tready", xb2sep)}}),
