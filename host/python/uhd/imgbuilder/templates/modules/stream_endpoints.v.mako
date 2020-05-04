<%page args="seps"/>\
<%
  import math
  import re

  axis_inputs = {}
  axis_outputs = {}
  for i, sep in enumerate(seps):
    inputs = ""
    outputs = ""
    for data_i in range(0,seps[sep]["num_data_i"]):
      inputs = "s_{0}_in%d_{1}, " % (data_i) + inputs
    axis_inputs[sep] = inputs[:-2]
    for data_o in range(0,seps[sep]["num_data_o"]):
      outputs = "m_{0}_out%d_{1}, " % (data_o) + outputs
    axis_outputs[sep] = outputs[:-2]
%>\
\
%for i, sep in enumerate(seps):
<%
# If buff_size == 0, then we assume that we will never transmit through this SEP
buff_size = seps[sep]["buff_size"]
if buff_size > 0:
    buff_size = int(math.ceil(math.log(buff_size, 2)))
    # FIXME MTU assumed to be 10 here -- forcing to at least accommodate 2 pkts
    buff_size = max(buff_size, 10+1)
else:
    buff_size = 5
%>\
  wire [CHDR_W-1:0] ${axis_outputs[sep].format(sep,"tdata")};
  wire              ${axis_outputs[sep].format(sep,"tlast")};
  wire              ${axis_outputs[sep].format(sep,"tvalid")};
  wire              ${axis_outputs[sep].format(sep,"tready")};
  wire [CHDR_W-1:0] ${axis_inputs[sep].format(sep,"tdata")};
  wire              ${axis_inputs[sep].format(sep,"tlast")};
  wire              ${axis_inputs[sep].format(sep,"tvalid")};
  wire              ${axis_inputs[sep].format(sep,"tready")};
  wire [31:0]       m_${sep}_ctrl_tdata , s_${sep}_ctrl_tdata ;
  wire              m_${sep}_ctrl_tlast , s_${sep}_ctrl_tlast ;
  wire              m_${sep}_ctrl_tvalid, s_${sep}_ctrl_tvalid;
  wire              m_${sep}_ctrl_tready, s_${sep}_ctrl_tready;

  chdr_stream_endpoint #(
    .PROTOVER           (PROTOVER),
    .CHDR_W             (CHDR_W),
    .AXIS_CTRL_EN       (${int(seps[sep]["ctrl"])}),
    .AXIS_DATA_EN       (${int(seps[sep]["data"])}),
    .NUM_DATA_I         (${int(seps[sep]["num_data_i"])}),
    .NUM_DATA_O         (${int(seps[sep]["num_data_o"])}),
    .INST_NUM           (${i}),
    .CTRL_XBAR_PORT     (${i+1}),
    .INGRESS_BUFF_SIZE  (${buff_size}),
    .MTU                (MTU),
    .REPORT_STRM_ERRS   (1)
  ) ${sep}_i (
    .rfnoc_chdr_clk     (rfnoc_chdr_clk    ),
    .rfnoc_chdr_rst     (rfnoc_chdr_rst    ),
    .rfnoc_ctrl_clk     (rfnoc_ctrl_clk    ),
    .rfnoc_ctrl_rst     (rfnoc_ctrl_rst    ),
    .device_id          (device_id         ),
    .s_axis_chdr_tdata  (xb_to_${sep}_tdata  ),
    .s_axis_chdr_tlast  (xb_to_${sep}_tlast  ),
    .s_axis_chdr_tvalid (xb_to_${sep}_tvalid ),
    .s_axis_chdr_tready (xb_to_${sep}_tready ),
    .m_axis_chdr_tdata  (${sep}_to_xb_tdata  ),
    .m_axis_chdr_tlast  (${sep}_to_xb_tlast  ),
    .m_axis_chdr_tvalid (${sep}_to_xb_tvalid ),
    .m_axis_chdr_tready (${sep}_to_xb_tready ),
    .s_axis_data_tdata  ({${axis_inputs[sep].format(sep,"tdata")}}),
    .s_axis_data_tlast  ({${axis_inputs[sep].format(sep,"tlast")}}),
    .s_axis_data_tvalid ({${axis_inputs[sep].format(sep,"tvalid")}}),
    .s_axis_data_tready ({${axis_inputs[sep].format(sep,"tready")}}),
    .m_axis_data_tdata  ({${axis_outputs[sep].format(sep,"tdata")}}),
    .m_axis_data_tlast  ({${axis_outputs[sep].format(sep,"tlast")}}),
    .m_axis_data_tvalid ({${axis_outputs[sep].format(sep,"tvalid")}}),
    .m_axis_data_tready ({${axis_outputs[sep].format(sep,"tready")}}),
    .s_axis_ctrl_tdata  (s_${sep}_ctrl_tdata ),
    .s_axis_ctrl_tlast  (s_${sep}_ctrl_tlast ),
    .s_axis_ctrl_tvalid (s_${sep}_ctrl_tvalid),
    .s_axis_ctrl_tready (s_${sep}_ctrl_tready),
    .m_axis_ctrl_tdata  (m_${sep}_ctrl_tdata ),
    .m_axis_ctrl_tlast  (m_${sep}_ctrl_tlast ),
    .m_axis_ctrl_tvalid (m_${sep}_ctrl_tvalid),
    .m_axis_ctrl_tready (m_${sep}_ctrl_tready),
    .strm_seq_err_stb   (                  ),
    .strm_data_err_stb  (                  ),
    .strm_route_err_stb (                  ),
    .signal_data_err    (1'b0              )
  );

%endfor
