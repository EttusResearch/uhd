<%page args="seps"/>\
\
%for sep in seps:
  wire [PORT_W-1:0] xb_to_${sep}_tdata ;
  wire              xb_to_${sep}_tlast ;
  wire              xb_to_${sep}_tvalid;
  wire              xb_to_${sep}_tready;
  wire [PORT_W-1:0] ${sep}_to_xb_tdata ;
  wire              ${sep}_to_xb_tlast ;
  wire              ${sep}_to_xb_tvalid;
  wire              ${sep}_to_xb_tready;
%endfor
