//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_switchboard
//
// Description:
//
//   The Switchboard RFNoC block routes any input CHDR stream to any output port.
//   Routing is 1 to 1, that is, an input port can only be connected to one
//   output port, and vice versa. Data sent on disconnected inputs will stall.
//
//       Input Port                        Output Port
//               ┌───────┐            ┌───────┐
//               │       ├────────────┤       │
//           0 ──┤ Demux │    ┌───────┤  Mux  ├─ 0
//               │       ├─┐  │  ┌────┤       │
//               └───────┘ │  │  │    └───────┘
//               ┌───────┐ │  │  │
//               │       ├─┼──┘  │    ┌───────┐
//           1 ──┤ Demux │ └─────┼────┤       │
//               │       ├───────┼────┤  Mux  ├─ 1
//               └───────┘       │  ┌─┤       │
//               ┌───────┐       │  │ └───────┘
//               │       ├───────┘  │
//           2 ──┤ Demux │          │
//               │       ├──────────┘
//               └───────┘
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   NUM_INPUTS  : The number of input ports
//   NUM_OUTPUTS : The number of output ports
//

`default_nettype none


module rfnoc_block_switchboard #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       NUM_INPUTS      = 1,
  parameter       NUM_OUTPUTS     = 1
)(
  // RFNoC Framework Clocks and Resets
  input  wire                   rfnoc_chdr_clk,
  input  wire                   rfnoc_ctrl_clk,
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [(NUM_INPUTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(NUM_INPUTS)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(NUM_INPUTS)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(NUM_INPUTS)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(NUM_OUTPUTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(NUM_OUTPUTS)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(NUM_OUTPUTS)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(NUM_OUTPUTS)-1:0]        m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready
);

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire               ctrlport_clk;
  wire               ctrlport_rst;
  wire               axis_chdr_clk;
  wire               axis_chdr_rst;
  // CtrlPort Master
  wire               m_ctrlport_req_wr;
  wire               m_ctrlport_req_rd;
  wire [19:0]        m_ctrlport_req_addr;
  wire [31:0]        m_ctrlport_req_data;
  reg                m_ctrlport_resp_ack;
  reg [31:0]         m_ctrlport_resp_data;
  // Framework to User Logic: in
  wire [NUM_INPUTS*CHDR_W-1:0]  m_in_chdr_tdata;
  wire [NUM_INPUTS-1:0]         m_in_chdr_tlast;
  wire [NUM_INPUTS-1:0]         m_in_chdr_tvalid;
  wire [NUM_INPUTS-1:0]         m_in_chdr_tready;
  // User Logic to Framework: out
  wire [NUM_OUTPUTS*CHDR_W-1:0]  s_out_chdr_tdata;
  wire [NUM_OUTPUTS-1:0]         s_out_chdr_tlast;
  wire [NUM_OUTPUTS-1:0]         s_out_chdr_tvalid;
  wire [NUM_OUTPUTS-1:0]         s_out_chdr_tready;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_switchboard #(
    .CHDR_W              (CHDR_W),
    .THIS_PORTID         (THIS_PORTID),
    .MTU                 (MTU),
    .NUM_INPUTS          (NUM_INPUTS),
    .NUM_OUTPUTS         (NUM_OUTPUTS)
  ) noc_shell_switchboard_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    // Reset Outputs
    .rfnoc_chdr_rst      (),
    .rfnoc_ctrl_rst      (),
    // RFNoC Backend Interface
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // CtrlPort Clock and Reset
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    // CtrlPort Master
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data      (m_ctrlport_resp_data),

    // AXIS-CHDR Clock and Reset
    .axis_chdr_clk (axis_chdr_clk),
    .axis_chdr_rst (axis_chdr_rst),
    // AXIS-CHDR to User Logic
    .m_in_chdr_tdata  (m_in_chdr_tdata),
    .m_in_chdr_tlast  (m_in_chdr_tlast),
    .m_in_chdr_tvalid (m_in_chdr_tvalid),
    .m_in_chdr_tready (m_in_chdr_tready),
    // AXIS-CHDR from User Logic
    .s_out_chdr_tdata  (s_out_chdr_tdata),
    .s_out_chdr_tlast  (s_out_chdr_tlast),
    .s_out_chdr_tvalid (s_out_chdr_tvalid),
    .s_out_chdr_tready (s_out_chdr_tready)
  );

  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  `include "rfnoc_block_switchboard_regs.vh"

  localparam INPUT_W    = (NUM_INPUTS < 3) ? 1 : $clog2(NUM_INPUTS);
  localparam OUTPUT_W   = (NUM_OUTPUTS < 3) ? 1 : $clog2(NUM_OUTPUTS);
  localparam MAX_W      = (INPUT_W > OUTPUT_W) ? INPUT_W : OUTPUT_W;
  reg [NUM_INPUTS*OUTPUT_W-1:0] reg_demux_select    = 0;
  reg [NUM_OUTPUTS*INPUT_W-1:0] reg_mux_select      = 0;

  wire [MAX_W-1:0] addr_port;
  wire [SWITCH_ADDR_W-1:0] addr_offset;
  assign addr_port = 
    (NUM_OUTPUTS < 2) ? 0 : m_ctrlport_req_addr[SWITCH_ADDR_W +: MAX_W];
  assign addr_offset = m_ctrlport_req_addr[0 +: SWITCH_ADDR_W];

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      m_ctrlport_resp_ack    <= 0;
      m_ctrlport_resp_data   <= 'bX;
      reg_demux_select <= 0;
      reg_mux_select <= 0;
    
    end else begin
      // Default assignments
      m_ctrlport_resp_ack    <= 0;
      m_ctrlport_resp_data   <= 0;

      // Handle register reads
      if (m_ctrlport_req_rd) begin
        m_ctrlport_resp_ack <= 1;
        case (addr_offset)
          REG_DEMUX_SELECT  : m_ctrlport_resp_data[0 +: OUTPUT_W]
            <= reg_demux_select[addr_port*OUTPUT_W +: OUTPUT_W];
          REG_MUX_SELECT    : m_ctrlport_resp_data[0 +: INPUT_W] 
            <= reg_mux_select[addr_port*INPUT_W +: INPUT_W];
        endcase
      
      // Handle register writes
      end else if (m_ctrlport_req_wr) begin
        m_ctrlport_resp_ack <= 1;
        case (addr_offset)
          REG_DEMUX_SELECT  : reg_demux_select[addr_port*OUTPUT_W +: OUTPUT_W]
            <= m_ctrlport_req_data[0 +: OUTPUT_W];
          REG_MUX_SELECT    : reg_mux_select[addr_port*INPUT_W +: INPUT_W] 
            <= m_ctrlport_req_data[0 +: INPUT_W];
        endcase
      end
    end
  end

  //---------------------------------------------------------------------------
  // User Logic
  //---------------------------------------------------------------------------

  wire [NUM_OUTPUTS*CHDR_W-1:0] tdata_demux [NUM_INPUTS-1:0];
  wire [NUM_OUTPUTS-1:0]        tlast_demux [NUM_INPUTS-1:0];
  wire [NUM_OUTPUTS-1:0]        tvalid_demux [NUM_INPUTS-1:0];
  wire [NUM_OUTPUTS-1:0]        tready_demux [NUM_INPUTS-1:0];

  wire [NUM_INPUTS*CHDR_W-1:0]  tdata_mux [NUM_OUTPUTS-1:0];
  wire [NUM_INPUTS-1:0]         tlast_mux [NUM_OUTPUTS-1:0];
  wire [NUM_INPUTS-1:0]         tvalid_mux [NUM_OUTPUTS-1:0];
  wire [NUM_INPUTS-1:0]         tready_mux [NUM_OUTPUTS-1:0];

  generate
    genvar in_m;
    genvar out_m;
    for (in_m = 0; in_m < NUM_INPUTS; in_m = in_m + 1) begin : gen_medium_in
      for (out_m = 0; out_m < NUM_OUTPUTS; out_m = out_m + 1) begin : gen_medium_out
        assign tdata_mux[out_m][in_m*CHDR_W +: CHDR_W]
          = tdata_demux[in_m][out_m*CHDR_W +: CHDR_W];
        assign tlast_mux[out_m][in_m] = tlast_demux[in_m][out_m];
        assign tvalid_mux[out_m][in_m] = tvalid_demux[in_m][out_m];
        assign tready_demux[in_m][out_m] = tready_mux[out_m][in_m];
      end
    end
  
    genvar in;  
    if (NUM_OUTPUTS < 2) begin : gen_static_in
      for (in = 0; in < NUM_INPUTS; in = in + 1) begin : gen_static_in_loop
        assign tdata_demux[in] = m_in_chdr_tdata[in*CHDR_W +: CHDR_W];
        assign tlast_demux[in] = m_in_chdr_tlast[in];
        assign tvalid_demux[in] = m_in_chdr_tvalid[in];
        assign m_in_chdr_tready[in] = tready_demux[in];
      end
  
    end else begin : gen_demux
      for (in = 0; in < NUM_INPUTS; in = in + 1) begin : gen_demux_loop
        axi_demux #(
          .WIDTH(CHDR_W),
          .SIZE(NUM_OUTPUTS)
        ) axi_demux_i (
          .clk(axis_chdr_clk),
          .reset(axis_chdr_rst),
          .clear(1'b0),
          .header(),
          .dest(reg_demux_select[in*OUTPUT_W +: OUTPUT_W]),
          .i_tdata(m_in_chdr_tdata[in*CHDR_W +: CHDR_W]),
          .i_tlast(m_in_chdr_tlast[in]),
          .i_tvalid(m_in_chdr_tvalid[in]),
          .i_tready(m_in_chdr_tready[in]),
          .o_tdata(tdata_demux[in]),
          .o_tlast(tlast_demux[in]),
          .o_tvalid(tvalid_demux[in]),
          .o_tready(tready_demux[in])
        );
      end
    end
  
    genvar out;
    if (NUM_INPUTS < 2) begin : gen_static_out
      for (out = 0; out < NUM_OUTPUTS; out = out + 1) begin : gen_static_out_loop
        assign s_out_chdr_tdata[out*CHDR_W +: CHDR_W] = tdata_mux[out];
        assign s_out_chdr_tlast[out] = tlast_mux[out];
        assign s_out_chdr_tvalid[out] = tvalid_mux[out];
        assign tready_mux[out] = s_out_chdr_tready[out];
      end
  
    end else begin : gen_mux
      for (out = 0; out < NUM_OUTPUTS; out = out + 1) begin : gen_mux_loop
        axi_mux_select #(
          .WIDTH(CHDR_W),
          .SWITCH_ON_LAST(1'b1),
          .SIZE(NUM_INPUTS)
        ) axi_mux_select_i (
          .clk(axis_chdr_clk),
          .reset(axis_chdr_rst),
          .clear(1'b0),
        .select(reg_mux_select[out*INPUT_W +: INPUT_W]),
          .i_tdata(tdata_mux[out]),
          .i_tlast(tlast_mux[out]),
          .i_tvalid(tvalid_mux[out]),
          .i_tready(tready_mux[out]),
          .o_tdata(s_out_chdr_tdata[out*CHDR_W +: CHDR_W]),
          .o_tlast(s_out_chdr_tlast[out]),
          .o_tvalid(s_out_chdr_tvalid[out]),
          .o_tready(s_out_chdr_tready[out])
        );
      end
    end
  endgenerate

endmodule // rfnoc_block_switchboard


`default_nettype wire
