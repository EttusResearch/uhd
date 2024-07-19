<%page args="seps, blocks"/>\
\
<%
  import re
  axisstr = ""
  for block in reversed(list(blocks.keys())):
    axisstr += "{0}_%s_ctrl_{1}, " % block
  for sep in reversed(list(seps.keys())):
    axisstr += "{0}_%s_ctrl_{1}, " % sep
  axisstr += "{0}_core_ctrl_{1}"
%>\
%for block in blocks:
  wire [31:0] m_${block}_ctrl_tdata,  s_${block}_ctrl_tdata;
  wire        m_${block}_ctrl_tlast,  s_${block}_ctrl_tlast;
  wire        m_${block}_ctrl_tvalid, s_${block}_ctrl_tvalid;
  wire        m_${block}_ctrl_tready, s_${block}_ctrl_tready;
%endfor

  axis_ctrl_crossbar_nxn #(
    .WIDTH            (32),
    .NPORTS           (${len(seps) + len(blocks) + 1}),
    .TOPOLOGY         ("TORUS"),
    .INGRESS_BUFF_SIZE(5),
    .ROUTER_BUFF_SIZE (5),
    .ROUTING_ALLOC    ("WORMHOLE"),
    .SWITCH_ALLOC     ("PRIO")
  ) ctrl_xb_i (
    .clk              (rfnoc_ctrl_clk),
    .reset            (rfnoc_ctrl_rst),
    .s_axis_tdata     ({${axisstr.format("m", "tdata ")}}),
    .s_axis_tvalid    ({${axisstr.format("m", "tvalid")}}),
    .s_axis_tlast     ({${axisstr.format("m", "tlast ")}}),
    .s_axis_tready    ({${axisstr.format("m", "tready")}}),
    .m_axis_tdata     ({${axisstr.format("s", "tdata ")}}),
    .m_axis_tvalid    ({${axisstr.format("s", "tvalid")}}),
    .m_axis_tlast     ({${axisstr.format("s", "tlast ")}}),
    .m_axis_tready    ({${axisstr.format("s", "tready")}}),
    .deadlock_detected()
  );
