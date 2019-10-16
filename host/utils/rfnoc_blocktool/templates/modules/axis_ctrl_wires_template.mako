<%page args="mode"/>\
<%
  if mode == "shell":
    sl_pre = "s_"
    ma_pre = "m_"
    in_wire = "input  "
    out_wire = "output "
    term = ","
  elif mode == "block":
    sl_pre = "s_"
    ma_pre = "m_"
    in_wire = ""
    out_wire = ""
    term = ";"
%>\
  // AXIS-Ctrl to User Logic
  ${out_wire}wire [31:0]        ${ma_pre}axis_ctrl_tdata${term}
  ${out_wire}wire               ${ma_pre}axis_ctrl_tlast${term}
  ${out_wire}wire               ${ma_pre}axis_ctrl_tvalid${term}
  ${in_wire}wire               ${ma_pre}axis_ctrl_tready${term}
  // AXIS-Ctrl Control from User Logic
  ${in_wire}wire [31:0]        ${sl_pre}axis_ctrl_tdata${term}
  ${in_wire}wire               ${sl_pre}axis_ctrl_tlast${term}
  ${in_wire}wire               ${sl_pre}axis_ctrl_tvalid${term}
  ${out_wire}wire               ${sl_pre}axis_ctrl_tready${term}
