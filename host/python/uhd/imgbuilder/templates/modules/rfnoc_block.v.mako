<%page args="block_id, block_number, block_name, block, block_params, block_ports"/>\
\
<%
  import re

  # Create two strings, one for the input and one for the output, that each
  # contains all the signal names to be connected to the input or output 
  # AXIS-CHDR ports of this block.
  axis_inputs = ""
  axis_outputs = ""
  for port_desc in block_ports:
    if port_desc[0] == block_name:
      port_name = port_desc[1]
      if port_desc[2] == "input":
        axis_inputs = "{0}_%s_%s_{1}, " % (block_name, port_name) + axis_inputs
      elif port_desc[2] == "output":
        axis_outputs = "{0}_%s_%s_{1}, " % (block_name, port_name) + axis_outputs
  axis_inputs = axis_inputs[:-2]
  axis_outputs = axis_outputs[:-2]
%>\

  //-----------------------------------
  // ${block_name}
  //-----------------------------------

%for clock in block.clocks:
  %if not clock["name"] in ["rfnoc_chdr", "rfnoc_ctrl"]:
  wire              ${block_name}_${clock["name"]}_clk;
  %endif
%endfor
  wire [CHDR_W-1:0] ${axis_inputs.format("s", "tdata ")};
  wire              ${axis_inputs.format("s", "tlast ")};
  wire              ${axis_inputs.format("s", "tvalid")};
  wire              ${axis_inputs.format("s", "tready")};
  wire [CHDR_W-1:0] ${axis_outputs.format("m", "tdata ")};
  wire              ${axis_outputs.format("m", "tlast ")};
  wire              ${axis_outputs.format("m", "tvalid")};
  wire              ${axis_outputs.format("m", "tready")};

%if hasattr(block, "io_ports"):
  %for name, io_port in block.io_ports.items():
  // ${name}
    %for wire in io_port["wires"]:
  wire [${"%4d" % (wire["width"]-1)}:0] ${block_name}_${wire["name"]};
    %endfor
  %endfor

%endif
  rfnoc_block_${block.module_name} #(
    .THIS_PORTID         (${block_id}),
    .CHDR_W              (CHDR_W),
%for name, value in block_params.items():
    .${"%-20s" % name}(${value}),
%endfor
    .MTU                 (MTU)
  ) b_${block_name}_${block_number} (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
%for clock in block.clocks:
  %if not clock["name"] in ["rfnoc_chdr", "rfnoc_ctrl"]:
    .${"%-20s" % (clock["name"] + "_clk")}(${block_name}_${clock["name"]}_clk),
  %endif
%endfor
    .rfnoc_core_config   (rfnoc_core_config[512*${block_number + 1}-1:512*${block_number}]),
    .rfnoc_core_status   (rfnoc_core_status[512*${block_number + 1}-1:512*${block_number}]),
%if hasattr(block, "io_ports"):
  %for name, io_port in block.io_ports.items():
    %for wire in io_port["wires"]:
    .${"%-20s" % wire["name"]}(${block_name}_${wire["name"]}),
    %endfor
  %endfor
%endif
    .s_rfnoc_chdr_tdata  ({${axis_inputs.format("s", "tdata ")}}),
    .s_rfnoc_chdr_tlast  ({${axis_inputs.format("s", "tlast ")}}),
    .s_rfnoc_chdr_tvalid ({${axis_inputs.format("s", "tvalid")}}),
    .s_rfnoc_chdr_tready ({${axis_inputs.format("s", "tready")}}),
    .m_rfnoc_chdr_tdata  ({${axis_outputs.format("m", "tdata ")}}),
    .m_rfnoc_chdr_tlast  ({${axis_outputs.format("m", "tlast ")}}),
    .m_rfnoc_chdr_tvalid ({${axis_outputs.format("m", "tvalid")}}),
    .m_rfnoc_chdr_tready ({${axis_outputs.format("m", "tready")}}),
    .s_rfnoc_ctrl_tdata  (s_${block_name}_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_${block_name}_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_${block_name}_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_${block_name}_ctrl_tready),
    .m_rfnoc_ctrl_tdata  (m_${block_name}_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_${block_name}_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_${block_name}_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_${block_name}_ctrl_tready)
  );
