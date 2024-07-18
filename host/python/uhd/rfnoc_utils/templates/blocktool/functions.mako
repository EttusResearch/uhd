<%def name="num_ports_str(direction)">\
<%
  # Generate a string for the number of input or output CHDR ports. This takes
  # into account any parameters that affect the number of ports. The direction 
  # argument should be the string 'inputs' or 'outputs'.
  num_ports_cnt = 0
  num_ports_str = ''
  data_ports = config.get('data', {}).get(direction, {})
  for port_name, port_info in data_ports.items():
    if 'num_ports' in port_info:
      if str(port_info['num_ports']).isdecimal():
        num_ports_cnt = num_ports_cnt + int(port_info['num_ports'])
      else:
        num_ports_str = num_ports_str + '+' + str(port_info['num_ports'])
    else:
      num_ports_cnt = num_ports_cnt + 1
  num_ports_str = str(num_ports_cnt) + num_ports_str
%>\
${num_ports_str}\
</%def>

<%def name="num_ports_in_str()">\
## Generate a string for the number of input CHDR ports. This takes into
## account any parameters that affect the number of ports.
${num_ports_str('inputs')}\
</%def>

<%def name="num_ports_out_str()">\
## Generate a string for the number of output CHDR ports. This takes into
## account any parameters that affect the number of ports.
${num_ports_str('outputs')}\
</%def>
