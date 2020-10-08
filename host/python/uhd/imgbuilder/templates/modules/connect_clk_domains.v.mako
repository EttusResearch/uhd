<%page args="connections, clocks"/>\
\
%for connection in connections:
<%
  src_name = connection["srcblk"] # Should always be "_device_"
  src = clocks[(src_name, connection["srcport"])]
  dst_name = connection["dstblk"]
  dst = clocks[(dst_name, connection["dstport"])]
%>\
  assign ${dst_name}_${dst["name"]}_clk = ${src["name"]}_clk;
%endfor
