<%page args="connections"/>\
\
%for connection in connections:
<%
  srcblk = connection["srcblk"]
  dstblk = connection["dstblk"]
  srcport = "in" if connection["srcport"] == None else connection["srcport"]
  dstport = "out" if connection["dstport"] == None else connection["dstport"]
%>\
  assign s_${dstblk}_${dstport}_tdata = m_${srcblk}_${srcport}_tdata;
  assign s_${dstblk}_${dstport}_tlast = m_${srcblk}_${srcport}_tlast;
  assign s_${dstblk}_${dstport}_tvalid = m_${srcblk}_${srcport}_tvalid;
  assign m_${srcblk}_${srcport}_tready = s_${dstblk}_${dstport}_tready;

%endfor
