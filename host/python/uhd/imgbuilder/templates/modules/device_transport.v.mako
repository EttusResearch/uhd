<%page args="transports"/>
  // Transport Adapters ///////////////

%for i, transport in enumerate(transports):
  // Transport ${i} (${transport["name"]})
  input  wire [CHDR_W-1:0] s_${transport["name"]}_tdata,
  input  wire              s_${transport["name"]}_tlast,
  input  wire              s_${transport["name"]}_tvalid,
  output wire              s_${transport["name"]}_tready,
  output wire [CHDR_W-1:0] m_${transport["name"]}_tdata,
  output wire              m_${transport["name"]}_tlast,
  output wire              m_${transport["name"]}_tvalid,
  input  wire              m_${transport["name"]}_tready${"," if i < len(transports) - 1 else ""}
%endfor
