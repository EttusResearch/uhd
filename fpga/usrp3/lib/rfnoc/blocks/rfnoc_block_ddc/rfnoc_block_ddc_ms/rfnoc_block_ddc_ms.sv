//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc_ms
//
// Description:
//   Multi-sample (SPC>1) capable implementation for rfnoc_block_ddc.
//
// Ctrlport hierarchy (from top-level block perspective):
//
//     noc_shell ctrlport
//     └── ctrlport_decoder_param (NUM_SLAVES = 2)
//         ├── slave[0]: shared regs path
//         │   └── shared readback/capability registers
//         └── slave[1]: per-port path
//             └── ctrlport_decoder (NUM_SLAVES = NUM_PORTS)
//                 ├── port[0]
//                 │   └── ctrlport_decoder_param (NUM_SLAVES = 2)
//                 │       ├── slave[0]: axi_rate_change_ms
//                 │       └── slave[1]: ddc_ms
//                 │           └── ctrlport_decoder_param (NUM_SLAVES = 2)
//                 │               ├── slave[0]: SR registers
//                 │               └── slave[1]: DDS registers
//                 ├── port[1]
//                 │   └── ctrlport_decoder_param (NUM_SLAVES = 2)
//                 │       ├── slave[0]: axi_rate_change_ms
//                 │       └── slave[1]: ddc_ms
//                 │           └── ctrlport_decoder_param (NUM_SLAVES = 2)
//                 │               ├── slave[0]: SR registers
//                 │               └── slave[1]: DDS registers
//                 └── ...
//                     └── port[NUM_PORTS-1]
//                         └── ctrlport_decoder_param (NUM_SLAVES = 2)
//                             ├── slave[0]: axi_rate_change_ms
//                             └── slave[1]: ddc_ms
//                                 └── ctrlport_decoder_param (NUM_SLAVES = 2)
//                                     ├── slave[0]: SR registers
//                                     └── slave[1]: DDS registers
//
// Parameters:
//   NUM_PORTS     : Number of parallel DDC chains to implement. Each chain has its
//                   own set of decimation controls and takes a separate input data
//                   stream.
//   NUM_HB        : Number of halfband filter stages to implement in each DDC chain.
//                   This determines the maximum decimation rate of the halfband filters,
//                   which is 2^NUM_HB.
//   CIC_MAX_DECIM : Maximum decimation rate of the CIC filter in each DDC chain.
//                   The total maximum decimation rate of the DDC is
//                   CIC_MAX_DECIM * 2^NUM_HB.
//   SAMP_W        : Width of an I+Q sample.
//   SPC           : Number of samples processed per clock cycle.
//                   This determines the width of the input and output data streams,
//                   which are NUM_PORTS*SAMP_W*SPC bits wide.
//                   The DDC will process SPC samples from each port in parallel
//                   every clock cycle.
//

module rfnoc_block_ddc_ms
  import ctrlport_pkg::*;
#(
  parameter [9:0] THIS_PORTID   = 10'd0,
  parameter       CHDR_W        = 64,
  parameter       NUM_PORTS     = 2,
  parameter [5:0] MTU           = 10,
  parameter       NUM_HB        = 3,
  parameter       CIC_MAX_DECIM = 255,
  parameter       NIPC          = 1
) (
  //---------------------------------------------------------------------------
  // AXIS CHDR Port
  //---------------------------------------------------------------------------

  input wire rfnoc_chdr_clk,
  input wire ce_clk,

  // CHDR inputs from framework
  input  wire [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,

  // CHDR outputs to framework
  output wire [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tready,

  // Backend interface
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status,

  //---------------------------------------------------------------------------
  // AXIS CTRL Port
  //---------------------------------------------------------------------------

  input wire rfnoc_ctrl_clk,

  // CTRL port requests from framework
  input  wire [31:0] s_rfnoc_ctrl_tdata,
  input  wire        s_rfnoc_ctrl_tlast,
  input  wire        s_rfnoc_ctrl_tvalid,
  output wire        s_rfnoc_ctrl_tready,

  // CTRL port requests to framework
  output wire [31:0] m_rfnoc_ctrl_tdata,
  output wire        m_rfnoc_ctrl_tlast,
  output wire        m_rfnoc_ctrl_tvalid,
  input  wire        m_rfnoc_ctrl_tready
);

  localparam SAMP_W = 32;
  localparam SPC    = NIPC;

  //---------------------------------------------------------------------------
  // Internal signals (NoC Shell <-> DSP core)
  //---------------------------------------------------------------------------

  wire        ce_rst;
  wire                           ctrlport_req_wr;
  wire                           ctrlport_req_rd;
  wire [   CTRLPORT_ADDR_W-1:0]  ctrlport_req_addr;
  wire [   CTRLPORT_DATA_W-1:0]  ctrlport_req_data;
  wire [CTRLPORT_BYTE_EN_W-1:0]  ctrlport_req_byte_en = {CTRLPORT_BYTE_EN_W{1'b1}};
  wire                           ctrlport_req_has_time;
  wire [   CTRLPORT_TIME_W-1:0]  ctrlport_req_time;
  wire                           ctrlport_resp_ack;
  wire [    CTRLPORT_STS_W-1:0]  ctrlport_resp_status;
  wire [   CTRLPORT_DATA_W-1:0]  ctrlport_resp_data;

  wire [NUM_PORTS*SAMP_W*SPC-1:0] m_axis_data_tdata;
  wire [            NUM_PORTS-1:0] m_axis_data_tlast;
  wire [            NUM_PORTS-1:0] m_axis_data_tvalid;
  wire [            NUM_PORTS-1:0] m_axis_data_tready;
  wire [         NUM_PORTS*64-1:0] m_axis_data_ttimestamp;
  wire [            NUM_PORTS-1:0] m_axis_data_thas_time;
  wire [         16*NUM_PORTS-1:0] m_axis_data_tlength;
  wire [            NUM_PORTS-1:0] m_axis_data_teob;

  wire [NUM_PORTS*SAMP_W*SPC-1:0] s_axis_data_tdata;
  wire [            NUM_PORTS-1:0] s_axis_data_tlast;
  wire [            NUM_PORTS-1:0] s_axis_data_tvalid;
  wire [            NUM_PORTS-1:0] s_axis_data_tready;
  wire [            NUM_PORTS-1:0] s_axis_data_teob;
  wire [         NUM_PORTS*64-1:0] s_axis_data_ttimestamp;
  wire [            NUM_PORTS-1:0] s_axis_data_thas_time;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_ddc_ms #(
    .THIS_PORTID   (THIS_PORTID),
    .CHDR_W        (CHDR_W),
    .MTU           (MTU),
    .NUM_PORTS     (NUM_PORTS),
    .NUM_HB        (NUM_HB),
    .CIC_MAX_DECIM (CIC_MAX_DECIM),
    .ITEM_W        (SAMP_W),
    .NIPC          (NIPC)
  ) noc_shell_ddc_ms_i (
    .rfnoc_chdr_clk          (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk          (rfnoc_ctrl_clk),
    .ce_clk                  (ce_clk),
    .rfnoc_chdr_rst          (),
    .rfnoc_ctrl_rst          (),
    .ce_rst                  (ce_rst),
    .rfnoc_core_config       (rfnoc_core_config),
    .rfnoc_core_status       (rfnoc_core_status),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata      (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast      (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid     (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready     (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata      (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast      (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid     (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready     (m_rfnoc_ctrl_tready),
    .ctrlport_clk            (),
    .ctrlport_rst            (),
    .m_ctrlport_req_wr       (ctrlport_req_wr),
    .m_ctrlport_req_rd       (ctrlport_req_rd),
    .m_ctrlport_req_addr     (ctrlport_req_addr),
    .m_ctrlport_req_data     (ctrlport_req_data),
    .m_ctrlport_req_has_time (ctrlport_req_has_time),
    .m_ctrlport_req_time     (ctrlport_req_time),
    .m_ctrlport_resp_ack     (ctrlport_resp_ack),
    .m_ctrlport_resp_data    (ctrlport_resp_data),
    .axis_data_clk           (),
    .axis_data_rst           (),
    .m_in_axis_tdata         (m_axis_data_tdata),
    .m_in_axis_tkeep         (),
    .m_in_axis_tlast         (m_axis_data_tlast),
    .m_in_axis_tvalid        (m_axis_data_tvalid),
    .m_in_axis_tready        (m_axis_data_tready),
    .m_in_axis_ttimestamp    (m_axis_data_ttimestamp),
    .m_in_axis_thas_time     (m_axis_data_thas_time),
    .m_in_axis_tlength       (m_axis_data_tlength),
    .m_in_axis_teov          (),
    .m_in_axis_teob          (m_axis_data_teob),
    .s_out_axis_tdata        (s_axis_data_tdata),
    .s_out_axis_tkeep        ({NUM_PORTS*NIPC{1'b1}}),
    .s_out_axis_tlast        (s_axis_data_tlast),
    .s_out_axis_tvalid       (s_axis_data_tvalid),
    .s_out_axis_tready       (s_axis_data_tready),
    .s_out_axis_ttimestamp   (s_axis_data_ttimestamp),
    .s_out_axis_thas_time    (s_axis_data_thas_time),
    .s_out_axis_teov         ({NUM_PORTS{1'b0}}),
    .s_out_axis_teob         (s_axis_data_teob)
  );

  localparam COMPAT_MAJOR  = 16'h1;
  localparam COMPAT_MINOR  = 16'h0;

  import rfnoc_block_ddc_ms_regs_pkg::*;

  //---------------------------------------------------------------------------
  // Split DDC Control Port IF into shared and whole per-port register region
  //---------------------------------------------------------------------------
  wire                          ctrlport_shared_req_wr;
  wire                          ctrlport_shared_req_rd;
  wire [   CTRLPORT_ADDR_W-1:0] ctrlport_shared_req_addr;
  wire [   CTRLPORT_DATA_W-1:0] ctrlport_shared_req_data;
  wire [CTRLPORT_BYTE_EN_W-1:0] ctrlport_shared_req_byte_en;
  wire                          ctrlport_shared_req_has_time;
  wire [   CTRLPORT_TIME_W-1:0] ctrlport_shared_req_time;
  reg                           ctrlport_shared_resp_ack;
  reg  [    CTRLPORT_STS_W-1:0] ctrlport_shared_resp_status;
  reg  [   CTRLPORT_DATA_W-1:0] ctrlport_shared_resp_data;

  wire                          ctrlport_ports_req_wr;
  wire                          ctrlport_ports_req_rd;
  wire [   CTRLPORT_ADDR_W-1:0] ctrlport_ports_req_addr;
  wire [   CTRLPORT_DATA_W-1:0] ctrlport_ports_req_data;
  wire [CTRLPORT_BYTE_EN_W-1:0] ctrlport_ports_req_byte_en;
  wire                          ctrlport_ports_req_has_time;
  wire [   CTRLPORT_TIME_W-1:0] ctrlport_ports_req_time;
  wire                          ctrlport_ports_resp_ack;
  wire [    CTRLPORT_STS_W-1:0] ctrlport_ports_resp_status;
  wire [   CTRLPORT_DATA_W-1:0] ctrlport_ports_resp_data;

  ctrlport_decoder_param #(
    .NUM_SLAVES  (2),
    .PORT_BASE  ({CTRLPORT_ADDR_W'(DDC_PORT_BASE_ADDR), CTRLPORT_ADDR_W'(DDC_SHARED_BASE_ADDR)}),
    .PORT_ADDR_W({DDC_PORT_RANGE_W,                     DDC_SHARED_ADDR_W})
  ) ctrlport_decoder_param_shared_ports (
    .ctrlport_clk            (ce_clk),
    .ctrlport_rst            (ce_rst),
    .s_ctrlport_req_wr       (ctrlport_req_wr),
    .s_ctrlport_req_rd       (ctrlport_req_rd),
    .s_ctrlport_req_addr     (ctrlport_req_addr),
    .s_ctrlport_req_data     (ctrlport_req_data),
    .s_ctrlport_req_byte_en  (ctrlport_req_byte_en),
    .s_ctrlport_req_has_time (ctrlport_req_has_time),
    .s_ctrlport_req_time     (ctrlport_req_time),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack),
    .s_ctrlport_resp_status  (ctrlport_resp_status),
    .s_ctrlport_resp_data    (ctrlport_resp_data),
    // slave connections
    .m_ctrlport_req_wr       ({ctrlport_ports_req_wr,
                               ctrlport_shared_req_wr}),
    .m_ctrlport_req_rd       ({ctrlport_ports_req_rd,
                               ctrlport_shared_req_rd}),
    .m_ctrlport_req_addr     ({ctrlport_ports_req_addr,
                               ctrlport_shared_req_addr}),
    .m_ctrlport_req_data     ({ctrlport_ports_req_data,
                               ctrlport_shared_req_data}),
    .m_ctrlport_req_byte_en  ({ctrlport_ports_req_byte_en,
                               ctrlport_shared_req_byte_en}),
    .m_ctrlport_req_has_time ({ctrlport_ports_req_has_time,
                               ctrlport_shared_req_has_time}),
    .m_ctrlport_req_time     ({ctrlport_ports_req_time,
                               ctrlport_shared_req_time}),
    .m_ctrlport_resp_ack     ({ctrlport_ports_resp_ack,
                               ctrlport_shared_resp_ack}),
    .m_ctrlport_resp_status  ({ctrlport_ports_resp_status,
                               ctrlport_shared_resp_status}),
    .m_ctrlport_resp_data    ({ctrlport_ports_resp_data,
                               ctrlport_shared_resp_data})
  );

  //---------------------------------------------------------------------------
  // Split DDC Control Port Interfaces into per-port DDC instances
  // Note: the port base is 0 since the upstream decoder already took care
  //       of splitting the address range.
  //---------------------------------------------------------------------------
  wire [                   NUM_PORTS-1:0] ctrlport_port_req_wr;
  wire [                   NUM_PORTS-1:0] ctrlport_port_req_rd;
  wire [   CTRLPORT_ADDR_W*NUM_PORTS-1:0] ctrlport_port_req_addr;
  wire [   CTRLPORT_DATA_W*NUM_PORTS-1:0] ctrlport_port_req_data;
  wire [CTRLPORT_BYTE_EN_W*NUM_PORTS-1:0] ctrlport_port_req_byte_en;
  wire [                   NUM_PORTS-1:0] ctrlport_port_req_has_time;
  wire [   CTRLPORT_TIME_W*NUM_PORTS-1:0] ctrlport_port_req_time;
  wire [                   NUM_PORTS-1:0] ctrlport_port_resp_ack;
  wire [    CTRLPORT_STS_W*NUM_PORTS-1:0] ctrlport_port_resp_status;
  wire [   CTRLPORT_DATA_W*NUM_PORTS-1:0] ctrlport_port_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES  (NUM_PORTS),
    .BASE_ADDR   (CTRLPORT_ADDR_W'('h0)),
    .SLAVE_ADDR_W(DDC_PORT_ADDR_W)
  ) ctrlport_decoder_ports (
    .ctrlport_clk            (ce_clk),
    .ctrlport_rst            (ce_rst),
    .s_ctrlport_req_wr       (ctrlport_ports_req_wr),
    .s_ctrlport_req_rd       (ctrlport_ports_req_rd),
    .s_ctrlport_req_addr     (ctrlport_ports_req_addr),
    .s_ctrlport_req_data     (ctrlport_ports_req_data),
    .s_ctrlport_req_byte_en  (ctrlport_ports_req_byte_en),
    .s_ctrlport_req_has_time (ctrlport_ports_req_has_time),
    .s_ctrlport_req_time     (ctrlport_ports_req_time),
    .s_ctrlport_resp_ack     (ctrlport_ports_resp_ack),
    .s_ctrlport_resp_status  (ctrlport_ports_resp_status),
    .s_ctrlport_resp_data    (ctrlport_ports_resp_data),
    // slave connections
    .m_ctrlport_req_wr       (ctrlport_port_req_wr),
    .m_ctrlport_req_rd       (ctrlport_port_req_rd),
    .m_ctrlport_req_addr     (ctrlport_port_req_addr),
    .m_ctrlport_req_data     (ctrlport_port_req_data),
    .m_ctrlport_req_byte_en  (ctrlport_port_req_byte_en),
    .m_ctrlport_req_has_time (ctrlport_port_req_has_time),
    .m_ctrlport_req_time     (ctrlport_port_req_time),
    .m_ctrlport_resp_ack     (ctrlport_port_resp_ack),
    .m_ctrlport_resp_status  (ctrlport_port_resp_status),
    .m_ctrlport_resp_data    (ctrlport_port_resp_data)
  );

  genvar i;
  generate
    for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_multisample_chains
      rfnoc_block_ddc_ms_channel #(
        // MTU is in CHDR words, but the channel operates on SAMP_W*SPC-wide
        // data words. Adjust so internal counters can represent the maximum
        // number of data words per packet.
        .NUM_HB        (NUM_HB),
        .CIC_MAX_DECIM (CIC_MAX_DECIM),
        .SAMP_W        (SAMP_W),
        .SPC           (SPC),
        .SPC_MTU_LOG2  (MTU + $clog2(CHDR_W / (SAMP_W * SPC)))
      ) rfnoc_block_ddc_ms_channel_i (
        .ce_clk                (ce_clk),
        .ce_rst                (ce_rst),
        .ctrlport_req_wr       (ctrlport_port_req_wr[i]),
        .ctrlport_req_rd       (ctrlport_port_req_rd[i]),
        .ctrlport_req_addr     (ctrlport_port_req_addr[i*CTRLPORT_ADDR_W +: CTRLPORT_ADDR_W]),
        .ctrlport_req_data     (ctrlport_port_req_data[i*CTRLPORT_DATA_W +: CTRLPORT_DATA_W]),
        .ctrlport_req_byte_en  (ctrlport_port_req_byte_en[i*CTRLPORT_BYTE_EN_W +: CTRLPORT_BYTE_EN_W]),
        .ctrlport_req_has_time (ctrlport_port_req_has_time[i]),
        .ctrlport_req_time     (ctrlport_port_req_time[i*CTRLPORT_TIME_W +: CTRLPORT_TIME_W]),
        .ctrlport_resp_ack     (ctrlport_port_resp_ack[i]),
        .ctrlport_resp_status  (ctrlport_port_resp_status[i*CTRLPORT_STS_W +: CTRLPORT_STS_W]),
        .ctrlport_resp_data    (ctrlport_port_resp_data[i*CTRLPORT_DATA_W +: CTRLPORT_DATA_W]),
        .m_axis_data_tdata     (m_axis_data_tdata[i*SAMP_W*SPC +: SAMP_W*SPC]),
        .m_axis_data_tlast     (m_axis_data_tlast[i]),
        .m_axis_data_tvalid    (m_axis_data_tvalid[i]),
        .m_axis_data_tready    (m_axis_data_tready[i]),
        .m_axis_data_ttimestamp(m_axis_data_ttimestamp[64*i +: 64]),
        .m_axis_data_thas_time (m_axis_data_thas_time[i]),
        .m_axis_data_tlength   (m_axis_data_tlength[16*i +: 16]),
        .m_axis_data_teob      (m_axis_data_teob[i]),
        .s_axis_data_tdata     (s_axis_data_tdata[i*SAMP_W*SPC +: SAMP_W*SPC]),
        .s_axis_data_tlast     (s_axis_data_tlast[i]),
        .s_axis_data_tvalid    (s_axis_data_tvalid[i]),
        .s_axis_data_tready    (s_axis_data_tready[i]),
        .s_axis_data_teob      (s_axis_data_teob[i]),
        .s_axis_data_ttimestamp(s_axis_data_ttimestamp[64*i +: 64]),
        .s_axis_data_thas_time (s_axis_data_thas_time[i])
      );

    end // gen_multisample_chains
  endgenerate

  //---------------------------------------------------------------------------
  // Handle ctrlport_shared registers (currently just capability reporting)
  //---------------------------------------------------------------------------
  always @(posedge ce_clk) begin
    if (ce_rst) begin
      ctrlport_shared_resp_ack  <= 1'b0;
      ctrlport_shared_resp_status <= CTRL_STS_OKAY;
      ctrlport_shared_resp_data <= 32'h0;
    end else begin
      ctrlport_shared_resp_ack  <= 1'b0;
      ctrlport_shared_resp_status <= CTRL_STS_OKAY;
      ctrlport_shared_resp_data <= 32'h0;
      if (ctrlport_shared_req_rd) begin
        case (ctrlport_shared_req_addr)
          REG_COMPAT_NUM: begin
            ctrlport_shared_resp_data <= {COMPAT_MAJOR, COMPAT_MINOR};
            ctrlport_shared_resp_ack  <= 1'b1;
          end
          REG_NUM_HB: begin
            ctrlport_shared_resp_data <= NUM_HB;
            ctrlport_shared_resp_ack  <= 1'b1;
          end
          REG_CIC_MAX_DECIM: begin
            ctrlport_shared_resp_data <= CIC_MAX_DECIM;
            ctrlport_shared_resp_ack  <= 1'b1;
          end
          REG_SPC: begin
            ctrlport_shared_resp_data <= SPC;
            ctrlport_shared_resp_ack  <= 1'b1;
          end
          default: begin
            ctrlport_shared_resp_status <= CTRL_STS_CMDERR; // unsupported address
            ctrlport_shared_resp_ack  <= 1'b1;
          end
        endcase
      end
      if (ctrlport_shared_req_wr) begin
        ctrlport_shared_resp_status <= CTRL_STS_CMDERR; // read-only registers
        ctrlport_shared_resp_ack  <= 1'b1;
      end
    end
  end

endmodule
