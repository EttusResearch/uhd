<%page args="connections, blocks, block_descs, seps"/>\
\
<%
  sources = []
  destinations = []
  for connection in connections:
    sources.append((connection["srcblk"], connection["srcport"]))
    destinations.append((connection["dstblk"], connection["dstport"]))
%>\
%for sep in seps:
  %for input in range(seps[sep]["num_data_i"]):
    %if not (sep, "in%d" % (input)) in destinations:
  assign s_${sep}_in${input}_tdata  =  'h0;
  assign s_${sep}_in${input}_tlast  = 1'b0;
  assign s_${sep}_in${input}_tvalid = 1'b0;
    %endif
  %endfor
  %for output in range(seps[sep]["num_data_o"]):
    %if not (sep, "out%d" % (output)) in sources:
  assign m_${sep}_out${output}_tready = 1'b1;
    %endif
  %endfor
%endfor
%for block in blocks:
  %for input in block_descs[blocks[block]["block_desc"]].data["inputs"]:
    %if not (block, input) in destinations:
  assign s_${block}_${input}_tdata  = ${block_descs[blocks[block]["block_desc"]].chdr_width}'h0;
  assign s_${block}_${input}_tlast  = 1'b0;
  assign s_${block}_${input}_tvalid = 1'b0;
    %endif
  %endfor
  %for output in block_descs[blocks[block]["block_desc"]].data["outputs"]:
    %if not (block, output) in sources:
  assign m_${block}_${output}_tready = 1'b1;
    %endif
  %endfor
%endfor
