<%page args="user_clocks"/>\
\
%for i, clock in enumerate(user_clocks):
<%
  uc_name = clock
  ip_name = user_clocks[clock]["ip"]
  port_in = user_clocks[clock]["port_in"]
  port_out = user_clocks[clock]["port_out"]
%>\
wire ${uc_name}_${port_in}_clk;
  wire ${uc_name}_${port_out}_clk;

  ${ip_name} #() ${uc_name} (
    .${port_out}(${uc_name}_${port_out}_clk),
    .${port_in}(${uc_name}_${port_in}_clk)
  );
%endfor
