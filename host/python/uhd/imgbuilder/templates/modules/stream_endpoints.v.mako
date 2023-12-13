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
  ep_name = sep.upper()
%>\
  // If requested buffer size is 0, use the minimum SRL-based FIFO size.
  // Otherwise, make sure it's at least two MTU-sized packets.
  localparam REQ_BUFF_SIZE_${ep_name} = ${str(seps[sep]["buff_size"])};
  localparam INGRESS_BUFF_SIZE_${ep_name} =
    REQ_BUFF_SIZE_${ep_name} == 0             ? 5         :
    REQ_BUFF_SIZE_${ep_name} < 2*(2**${ep_name}_MTU) ? ${ep_name}_MTU+1 :
                                         $clog2(REQ_BUFF_SIZE_${ep_name});

  wire [BLOCK_CHDR_W-1:0] ${axis_outputs[sep].format(sep,"tdata")};
  wire                    ${axis_outputs[sep].format(sep,"tlast")};
  wire                    ${axis_outputs[sep].format(sep,"tvalid")};
  wire                    ${axis_outputs[sep].format(sep,"tready")};
  wire [BLOCK_CHDR_W-1:0] ${axis_inputs[sep].format(sep,"tdata")};
  wire                    ${axis_inputs[sep].format(sep,"tlast")};
  wire                    ${axis_inputs[sep].format(sep,"tvalid")};
  wire                    ${axis_inputs[sep].format(sep,"tready")};
  wire [            31:0] m_${sep}_ctrl_tdata,  s_${sep}_ctrl_tdata;
  wire                    m_${sep}_ctrl_tlast,  s_${sep}_ctrl_tlast;
  wire                    m_${sep}_ctrl_tvalid, s_${sep}_ctrl_tvalid;
  wire                    m_${sep}_ctrl_tready, s_${sep}_ctrl_tready;

  chdr_stream_endpoint #(
    .DEVICE_FAMILY      ("${config.device.family}"),
    .PROTOVER           (PROTOVER),
    .CHDR_W             (${ep_name + "_W"}),
    .BLOCK_CHDR_W       (BLOCK_CHDR_W),
    .AXIS_CTRL_EN       (${int(seps[sep]["ctrl"])}),
    .AXIS_DATA_EN       (${int(seps[sep]["data"])}),
    .NUM_DATA_I         (${int(seps[sep]["num_data_i"])}),
    .NUM_DATA_O         (${int(seps[sep]["num_data_o"])}),
    .INST_NUM           (${i}),
    .CTRL_XBAR_PORT     (${i+1}),
    .INGRESS_BUFF_SIZE  (INGRESS_BUFF_SIZE_${ep_name}),
    .MTU                (${ep_name + "_MTU"}),
    .REPORT_STRM_ERRS   (1)
  ) ${sep}_i (
    .rfnoc_chdr_clk     (rfnoc_chdr_clk),
    .rfnoc_chdr_rst     (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk     (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst     (rfnoc_ctrl_rst),
    .device_id          (device_id),
    .s_axis_chdr_tdata  (xb_to_${sep}_tdata),
    .s_axis_chdr_tlast  (xb_to_${sep}_tlast),
    .s_axis_chdr_tvalid (xb_to_${sep}_tvalid),
    .s_axis_chdr_tready (xb_to_${sep}_tready),
    .m_axis_chdr_tdata  (${sep}_to_xb_tdata),
    .m_axis_chdr_tlast  (${sep}_to_xb_tlast),
    .m_axis_chdr_tvalid (${sep}_to_xb_tvalid),
    .m_axis_chdr_tready (${sep}_to_xb_tready),
    .s_axis_data_tdata  ({${axis_inputs[sep].format(sep,"tdata")}}),
    .s_axis_data_tlast  ({${axis_inputs[sep].format(sep,"tlast")}}),
    .s_axis_data_tvalid ({${axis_inputs[sep].format(sep,"tvalid")}}),
    .s_axis_data_tready ({${axis_inputs[sep].format(sep,"tready")}}),
    .m_axis_data_tdata  ({${axis_outputs[sep].format(sep,"tdata")}}),
    .m_axis_data_tlast  ({${axis_outputs[sep].format(sep,"tlast")}}),
    .m_axis_data_tvalid ({${axis_outputs[sep].format(sep,"tvalid")}}),
    .m_axis_data_tready ({${axis_outputs[sep].format(sep,"tready")}}),
    .s_axis_ctrl_tdata  (s_${sep}_ctrl_tdata),
    .s_axis_ctrl_tlast  (s_${sep}_ctrl_tlast),
    .s_axis_ctrl_tvalid (s_${sep}_ctrl_tvalid),
    .s_axis_ctrl_tready (s_${sep}_ctrl_tready),
    .m_axis_ctrl_tdata  (m_${sep}_ctrl_tdata),
    .m_axis_ctrl_tlast  (m_${sep}_ctrl_tlast),
    .m_axis_ctrl_tvalid (m_${sep}_ctrl_tvalid),
    .m_axis_ctrl_tready (m_${sep}_ctrl_tready),
    .strm_seq_err_stb   (),
    .strm_data_err_stb  (),
    .strm_route_err_stb (),
    .signal_data_err    (1'b0)
  );

%endfor
