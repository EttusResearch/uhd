<%
    import datetime
    [protover_major, protover_minor, *_] = config.rfnoc_version.split('.')
%>//
% if hasattr(config, 'copyright'):
// ${config.copyright}
//
% endif
% if hasattr(config, 'license'):
// ${config.license}
//
% endif
// Module: rfnoc_image_core (for ${config.device.type})
//
// Description:
//
//   The RFNoC Image Core contains the Verilog description of the RFNoC design
//   to be loaded onto the FPGA.
//
//   This file was automatically generated by the RFNoC image builder tool.
//   Re-running that tool will overwrite this file!
//
% if args.get('source'):
// Source: ${args.get('source')}
% endif
%if not args.get('no_date'):
// File generated on: ${datetime.datetime.now().isoformat()}
% endif
% if args.get('source_hash') and not args.get('no_hash'):
// Source SHA256: ${args.get('source_hash')}
% endif
//

`default_nettype none


module rfnoc_image_core #(
  parameter [31:0] CHDR_W     = ${config.chdr_width},
  parameter [31:0] PORT_W     = ${config.chdr_width},
% if config.get_ta_gen_mode() == 'fixed':
% for port in config.device.transports:
  parameter [31:0] ${'{:<10}'.format(port["name"].upper() + "_W")} = ${port["width"]},
% endfor
% endif
  parameter        MTU        = 10,
  parameter [15:0] PROTOVER   = {8'd${protover_major}, 8'd${protover_minor}},
  parameter        RADIO_NIPC = 1
) (
  // Clocks
  input  wire         chdr_aclk,
  input  wire         ctrl_aclk,
  input  wire         core_arst,
%for clock in config.device.clocks:
  %if clock["name"] not in config.DEFAULT_CLK_NAMES:
  input  wire         ${clock["name"]}_clk,
  %endif
%endfor
  // Basic
  input  wire [  15:0] device_id,

<%include file="/modules/device_io_ports.v.mako" args="io_ports=config.device.io_ports, config=config"/>\
% if config.get_ta_gen_mode() == 'fixed':
## If the mode is 'user_defined', then the wires for the TAs are defined in the
## IO ports.
<%include
  file="/modules/device_transport.v.mako"
  args="transports=config.device.transports"/>\
% endif
);

  // BLOCK_CHDR_W is the CHDR width between blocks, and between blocks and
  // stream endpoints. It defaults to CHDR_W, but can be set to be a different
  // value than the global CHDR width, and therefore gets its own constant.
  localparam BLOCK_CHDR_W  = ${config.block_chdr_width};
  localparam BYTE_MTU      = MTU + $clog2(CHDR_W/8);
  localparam BLOCK_MTU     = BYTE_MTU - $clog2(BLOCK_CHDR_W/8);
%for i, sep in enumerate(config.stream_endpoints):
<%
  ep_name = sep.upper()
  all_blocks = config.get_module_list('noc_blocks')
%>\
  localparam ${ep_name + "_W"}   = ${config.stream_endpoints[sep]['chdr_width']};
  localparam ${ep_name + "_MTU"} = BYTE_MTU - $clog2(${ep_name + "_W"}/8);
%endfor
  localparam int BLOCK_PORT_IDS [${len(all_blocks)}] = '{${", ".join(f"{b['port_index']}" for b in all_blocks.values())}};

  wire rfnoc_chdr_clk, rfnoc_chdr_rst;
  wire rfnoc_ctrl_clk, rfnoc_ctrl_rst;

% if config.get_ta_gen_mode() == 'user_defined':
  //---------------------------------------------------------------------------
  // Transport Adapters
  //---------------------------------------------------------------------------

% for ta_name, ta in config.transport_adapters.items():
<%include
  file="/modules/transport_adapter.v.mako"
  args="ta_name=ta_name, ta=ta, config=config"
/>\
% endfor

% endif

  //---------------------------------------------------------------------------
  // CHDR Crossbar
  //---------------------------------------------------------------------------
  `define STRINGIFY(x) `"x`"
  `ifdef CHDR_CROSSBAR_OPTIMIZE
    localparam CHDR_CROSSBAR_OPTIMIZE = `STRINGIFY(`CHDR_CROSSBAR_OPTIMIZE);
  `else
    localparam CHDR_CROSSBAR_OPTIMIZE = "AREA";
  `endif

<%include file="/modules/sep_xb_wires.v.mako" args="seps=config.stream_endpoints"/>\
<%
  import numpy as np
  # Generate the list of CHDR widths for the crossbar ports. It is reversed so
  # that we can directly list it into the CHDR_WIDTHS parameter.
  cb_port_widths_rev = []
  for sep in reversed(config.stream_endpoints):
    cb_port_widths_rev.append(sep.upper() + "_W")
  if config.get_ta_gen_mode() == 'fixed':
    for transport in reversed(config.device.transports):
      cb_port_widths_rev.append(transport["name"].upper() + "_W")
  else:
    ## Unless we also enable that in HDL, transport adapters always match the CHDR_W
    for ta_info in reversed(config.get_ta_info()):
        cb_port_widths_rev += [ta_info['width']] * ta_info['num_ports']
  num_ports = len(cb_port_widths_rev)
  # This is a safe way to calculate number of TAs, regardless of TA gen mode
  num_tas = num_ports - len(config.stream_endpoints)
  # Convert the crossbar routes to a Verilog bit vector
  routes = np.flip(np.array(config.crossbar_routes).astype(int))
%>\

  chdr_crossbar_nxn #(
    .PORT_W         (PORT_W),
    .NPORTS         (${num_ports}),
    .CHDR_WIDTHS    ({ ${", ".join(width for width in cb_port_widths_rev)} }),
    .DEFAULT_PORT   (0),
    .ROUTES         ({
% for row in routes:
                      ${num_ports}'b${"".join(str(b) for b in row)}${"" if loop.last else ","}
% endfor
    }),
    .BYTE_MTU       (BYTE_MTU),
    .ROUTE_TBL_SIZE (6),
    .MUX_ALLOC      ("ROUND-ROBIN"),
    .OPTIMIZE       (CHDR_CROSSBAR_OPTIMIZE),
    .NPORTS_MGMT    (${num_tas}),
    .EXT_RTCFG_PORT (0),
    .PROTOVER       (PROTOVER)
  ) chdr_crossbar_nxn_i (
    .clk            (rfnoc_chdr_clk),
    .reset          (rfnoc_chdr_rst),
    .device_id      (device_id),
<%include file="/modules/chdr_xb_sep_transport.v.mako" args="config=config, routes=routes"/>\
    .ext_rtcfg_stb  (1'h0),
    .ext_rtcfg_addr (16'h0),
    .ext_rtcfg_data (32'h0),
    .ext_rtcfg_ack  ()
  );


  //---------------------------------------------------------------------------
  // Stream Endpoints
  //---------------------------------------------------------------------------

<%include file="/modules/stream_endpoints.v.mako" args="seps=config.stream_endpoints"/>\
<%
    from collections import OrderedDict
    ctrl_seps = OrderedDict((k, v) for k, v in config.stream_endpoints.items() if v.get('ctrl'))
%>
  //---------------------------------------------------------------------------
  // Control Crossbar
  //---------------------------------------------------------------------------

  wire [31:0] m_core_ctrl_tdata,  s_core_ctrl_tdata;
  wire        m_core_ctrl_tlast,  s_core_ctrl_tlast;
  wire        m_core_ctrl_tvalid, s_core_ctrl_tvalid;
  wire        m_core_ctrl_tready, s_core_ctrl_tready;
<%include file="/modules/ctrl_crossbar.v.mako" args="seps=ctrl_seps, blocks=all_blocks"/>\


  //---------------------------------------------------------------------------
  // RFNoC Core Kernel
  //---------------------------------------------------------------------------

  wire [(512*${len(all_blocks)})-1:0] rfnoc_core_config, rfnoc_core_status;

<%
  lp = lambda text: f"{text:>02}"
  num_transports = \
      len(config.device.transports) if config.get_ta_gen_mode() == 'fixed' \
      else sum(ta_info['num_ports'] for ta_info in config.get_ta_info())
%>\
  rfnoc_core_kernel #(
    .PROTOVER            (PROTOVER),
    .DEVICE_TYPE         (16'h${config.device.type_id}),
    .DEVICE_FAMILY       ("${config.device.family}"),
    .SAFE_START_CLKS     (0),
    .NUM_BLOCKS          (${len(all_blocks)}),
    .NUM_STREAM_ENDPOINTS(${len(config.stream_endpoints)}),
    .NUM_ENDPOINTS_CTRL  (${len(ctrl_seps)}),
    .NUM_TRANSPORTS      (${num_transports}),
    .NUM_EDGES           (${len(config.edge_table)}),
    .CHDR_XBAR_PRESENT   (1),
    .EDGE_TBL            ({
% for edge in list(reversed(config.edge_table)):
      {10'd${edge[0]|lp}, 6'd${edge[1]|lp}, 10'd${edge[2]|lp}, 6'd${edge[3]|lp}}${"" if loop.last else ","}
% endfor
    })
  ) core_kernel_i (
    .chdr_aclk          (chdr_aclk),
    .chdr_aclk_locked   (1'b1),
    .ctrl_aclk          (ctrl_aclk),
    .ctrl_aclk_locked   (1'b1),
    .core_arst          (core_arst),
    .core_chdr_clk      (rfnoc_chdr_clk),
    .core_chdr_rst      (rfnoc_chdr_rst),
    .core_ctrl_clk      (rfnoc_ctrl_clk),
    .core_ctrl_rst      (rfnoc_ctrl_rst),
    .s_axis_ctrl_tdata  (s_core_ctrl_tdata),
    .s_axis_ctrl_tlast  (s_core_ctrl_tlast),
    .s_axis_ctrl_tvalid (s_core_ctrl_tvalid),
    .s_axis_ctrl_tready (s_core_ctrl_tready),
    .m_axis_ctrl_tdata  (m_core_ctrl_tdata),
    .m_axis_ctrl_tlast  (m_core_ctrl_tlast),
    .m_axis_ctrl_tvalid (m_core_ctrl_tvalid),
    .m_axis_ctrl_tready (m_core_ctrl_tready),
    .device_id          (device_id),
    .rfnoc_core_config  (rfnoc_core_config),
    .rfnoc_core_status  (rfnoc_core_status)
  );


  //---------------------------------------------------------------------------
  // Blocks
  //---------------------------------------------------------------------------
% for i, name in enumerate(config.noc_blocks):
<%include
  file="/modules/rfnoc_block.v.mako"
  args="i=i, block_name=name, config=config"
/>\
% endfor

% for i, name in enumerate(config.modules):
  % if loop.first:
  //---------------------------------------------------------------------------
  // Generic Modules
  //---------------------------------------------------------------------------

  % endif
<%include
  file="/modules/module.v.mako"
  args="i=i, module_name=name, config=config"
/>\
% endfor
% if getattr(config, 'secure_config', None):
<%include
  file="/modules/secure_image_core_inst.v.mako"
  args="config=config.secure_config, top_config=config "
/>\
% endif
  //---------------------------------------------------------------------------
  // Static Router
  //---------------------------------------------------------------------------

<%
  block_con = [con for con in config.connections if con['srctype'] == 'output']
  if config.secure_config:
    block_con += [con for con in config.secure_config.connections if con['srctype'] == 'output']
%>\
<%include file="/modules/static_router.v.mako" args="connections=block_con"/>\

  //---------------------------------------------------------------------------
  // Unused Block Ports
  //---------------------------------------------------------------------------

<%include
  file="/modules/drive_unused_ports.v.mako"
  args="connections=block_con, config=config"
/>\

  //---------------------------------------------------------------------------
  // Unused IO Ports
  //---------------------------------------------------------------------------

<%
  io_con = [con for con in config.connections if con['srctype'] in ('master', 'broadcaster')]
  if config.secure_config:
    io_con += [
      con for con in config.secure_config.connections
      if con['srctype'] in ('master', 'broadcaster')]
%>
<%include
  file="/modules/drive_unused_ioports.v.mako"
  args="connections=io_con, config=config"
/>\


  //---------------------------------------------------------------------------
  // Clock Domains and Resets
  //---------------------------------------------------------------------------

<%include
  file="/modules/connect_clk_domains.v.mako"
  args="connections=config.clk_domains, clk_or_rst='clk'"
/>\
<%include
  file="/modules/connect_clk_domains.v.mako"
  args="connections=config.resets, clk_or_rst='rst'"
/>\


  //---------------------------------------------------------------------------
  // IO Port Connection
  //---------------------------------------------------------------------------

  // Master/Slave Connections:
<%include
  file="/modules/connect_io_ports.v.mako"
  args="connections=[c for c in config.connections if c['srctype'] == 'master']"
/>\
  // Broadcaster/Listener Connections:
<%include
  file="/modules/connect_io_ports.v.mako"
  args="connections=[c for c in config.connections if c['srctype'] == 'broadcaster']"
/>\
endmodule


`default_nettype wire
