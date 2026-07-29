//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_ms
//
// Description:
//
//   A digital up-converter block for RFNoC that supports parallel processing
//   of multiple samples per clock cycle (SPC).
//
// Parameters:
//
//   THIS_PORTID    : Control crossbar port to which this block is connected
//   CHDR_W         : AXIS-CHDR data bus width
//   MTU            : Maximum transmission unit (i.e., maximum packet size in
//                    CHDR words is 2**MTU).
//   NUM_PORTS      : Number of DUC signal processing chains
//   NUM_HB         : Number of half-band filter blocks to include (0-3)
//   CIC_MAX_INTERP : Maximum interpolation to support in the CIC filter
//   NIPC           : Number of items per clock cycle, which also sets the SPC.
//

`default_nettype none


module rfnoc_block_duc_ms 
  import ctrlport_pkg::*;
#(
  parameter logic [9:0] THIS_PORTID     = 0,
  parameter int         CHDR_W          = 64,
  parameter logic [5:0] MTU             = 10,
  parameter int         NUM_PORTS       = 2,
  parameter int         NUM_HB          = 3,
  parameter int         CIC_MAX_INTERP  = 255,
  parameter int         NIPC            = 1,
  parameter logic [5:0] CTRL_CLK_IDX    = 6'h3F,
  parameter logic [5:0] TB_CLK_IDX      = 6'h3F
) (
  // RFNoC Framework Clocks and Resets
  input wire rfnoc_chdr_clk,
  input wire ce_clk,
  input wire rfnoc_ctrl_clk,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [NUM_PORTS-1:0][CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [NUM_PORTS-1:0]             s_rfnoc_chdr_tlast,
  input  wire [NUM_PORTS-1:0]             s_rfnoc_chdr_tvalid,
  output wire [NUM_PORTS-1:0]             s_rfnoc_chdr_tready,

  // AXIS-CHDR Output Ports (to framework)
  output wire [NUM_PORTS-1:0][CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [NUM_PORTS-1:0]             m_rfnoc_chdr_tlast,
  output wire [NUM_PORTS-1:0]             m_rfnoc_chdr_tvalid,
  input  wire [NUM_PORTS-1:0]             m_rfnoc_chdr_tready,

  // AXIS-Ctrl Input Port (from framework)
  input  wire [CTRLPORT_DATA_W-1:0] s_rfnoc_ctrl_tdata,
  input  wire                       s_rfnoc_ctrl_tlast,
  input  wire                       s_rfnoc_ctrl_tvalid,
  output wire                       s_rfnoc_ctrl_tready,

  // AXIS-Ctrl Output Port (to framework)
  output wire [CTRLPORT_DATA_W-1:0] m_rfnoc_ctrl_tdata,
  output wire                       m_rfnoc_ctrl_tlast,
  output wire                       m_rfnoc_ctrl_tvalid,
  input  wire                       m_rfnoc_ctrl_tready,

  // RFNoC Backend Interface
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status
);
  import rfnoc_chdr_utils_pkg::*;
  import rfnoc_block_duc_ms_pkg::*;

  localparam int SAMP_W = 32;
  localparam int SPC    = NIPC;

  // Software compatibility version
  localparam logic [(CTRLPORT_DATA_W/2)-1:0] COMPAT_MAJOR  = 1;
  localparam logic [(CTRLPORT_DATA_W/2)-1:0] COMPAT_MINOR  = 0;


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  logic                                ctrlport_req_wr;
  logic                                ctrlport_req_rd;
  logic [         CTRLPORT_ADDR_W-1:0] ctrlport_req_addr;
  logic [         CTRLPORT_DATA_W-1:0] ctrlport_req_data;
  logic [      CTRLPORT_BYTE_EN_W-1:0] ctrlport_req_byte_en =
                                         {CTRLPORT_BYTE_EN_W{1'b1}};
  logic                                ctrlport_req_has_time;
  logic [         CTRLPORT_TIME_W-1:0] ctrlport_req_time;
  logic                                ctrlport_resp_ack;
  logic [          CTRLPORT_STS_W-1:0] ctrlport_resp_status;
  logic [         CTRLPORT_DATA_W-1:0] ctrlport_resp_data;

  logic [NUM_PORTS-1:0][SPC-1:0][ SAMP_W-1:0] from_noc_tdata;
  logic [NUM_PORTS-1:0]                       from_noc_tlast;
  logic [NUM_PORTS-1:0]                       from_noc_tvalid;
  logic [NUM_PORTS-1:0]                       from_noc_tready;
  logic [NUM_PORTS-1:0][CHDR_TIMESTAMP_W-1:0] from_noc_ttimestamp;
  logic [NUM_PORTS-1:0]                       from_noc_thas_time;
  logic [NUM_PORTS-1:0][   CHDR_LENGTH_W-1:0] from_noc_tlength;
  logic [NUM_PORTS-1:0]                       from_noc_teob;

  logic [NUM_PORTS-1:0][SPC-1:0][ SAMP_W-1:0] to_noc_tdata;
  logic [NUM_PORTS-1:0]                       to_noc_tlast;
  logic [NUM_PORTS-1:0]                       to_noc_tvalid;
  logic [NUM_PORTS-1:0]                       to_noc_tready;
  logic [NUM_PORTS-1:0][   CHDR_LENGTH_W-1:0] to_noc_tlength;
  logic [NUM_PORTS-1:0]                       to_noc_teob;
  logic [NUM_PORTS-1:0][CHDR_TIMESTAMP_W-1:0] to_noc_ttimestamp;
  logic [NUM_PORTS-1:0]                       to_noc_thas_time;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  logic ce_rst;

  noc_shell_duc_ms #(
    .THIS_PORTID    (THIS_PORTID   ),
    .CHDR_W         (CHDR_W        ),
    .CTRL_CLK_IDX   (CTRL_CLK_IDX  ),
    .TB_CLK_IDX     (TB_CLK_IDX    ),
    .MTU            (MTU           ),
    .NUM_PORTS      (NUM_PORTS     ),
    .NUM_HB         (NUM_HB        ),
    .CIC_MAX_INTERP (CIC_MAX_INTERP),
    .NIPC           (NIPC          )
  ) noc_shell_duc_ms_i (
    .rfnoc_chdr_clk          (rfnoc_chdr_clk        ),
    .rfnoc_ctrl_clk          (rfnoc_ctrl_clk        ),
    .ce_clk                  (ce_clk                ),
    .rfnoc_chdr_rst          (                      ),
    .rfnoc_ctrl_rst          (                      ),
    .ce_rst                  (ce_rst                ),
    .rfnoc_core_config       (rfnoc_core_config     ),
    .rfnoc_core_status       (rfnoc_core_status     ),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata    ),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast    ),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid   ),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready   ),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata    ),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast    ),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid   ),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready   ),
    .s_rfnoc_ctrl_tdata      (s_rfnoc_ctrl_tdata    ),
    .s_rfnoc_ctrl_tlast      (s_rfnoc_ctrl_tlast    ),
    .s_rfnoc_ctrl_tvalid     (s_rfnoc_ctrl_tvalid   ),
    .s_rfnoc_ctrl_tready     (s_rfnoc_ctrl_tready   ),
    .m_rfnoc_ctrl_tdata      (m_rfnoc_ctrl_tdata    ),
    .m_rfnoc_ctrl_tlast      (m_rfnoc_ctrl_tlast    ),
    .m_rfnoc_ctrl_tvalid     (m_rfnoc_ctrl_tvalid   ),
    .m_rfnoc_ctrl_tready     (m_rfnoc_ctrl_tready   ),
    .ctrlport_clk            (                      ),
    .ctrlport_rst            (                      ),
    .m_ctrlport_req_wr       (ctrlport_req_wr       ),
    .m_ctrlport_req_rd       (ctrlport_req_rd       ),
    .m_ctrlport_req_addr     (ctrlport_req_addr     ),
    .m_ctrlport_req_data     (ctrlport_req_data     ),
    .m_ctrlport_req_has_time (ctrlport_req_has_time ),
    .m_ctrlport_req_time     (ctrlport_req_time     ),
    .m_ctrlport_resp_ack     (ctrlport_resp_ack     ),
    .m_ctrlport_resp_data    (ctrlport_resp_data    ),
    .axis_data_clk           (                      ),
    .axis_data_rst           (                      ),
    .m_in_axis_tdata         (from_noc_tdata        ),
    .m_in_axis_tkeep         (                      ),
    .m_in_axis_tlast         (from_noc_tlast        ),
    .m_in_axis_tvalid        (from_noc_tvalid       ),
    .m_in_axis_tready        (from_noc_tready       ),
    .m_in_axis_ttimestamp    (from_noc_ttimestamp   ),
    .m_in_axis_thas_time     (from_noc_thas_time    ),
    .m_in_axis_tlength       (from_noc_tlength      ),
    .m_in_axis_teov          (                      ),
    .m_in_axis_teob          (from_noc_teob         ),
    .s_out_axis_tdata        (to_noc_tdata          ),
    .s_out_axis_tkeep        ('1                    ),
    .s_out_axis_tlast        (to_noc_tlast          ),
    .s_out_axis_tvalid       (to_noc_tvalid         ),
    .s_out_axis_tready       (to_noc_tready         ),
    .s_out_axis_ttimestamp   (to_noc_ttimestamp     ),
    .s_out_axis_thas_time    (to_noc_thas_time      ),
    .s_out_axis_tlength      (to_noc_tlength        ),
    .s_out_axis_teov         ('0                    ),
    .s_out_axis_teob         (to_noc_teob           )
  );

  //---------------------------------------------------------------------------
  // Split DUC Control Port IF into shared and whole per-port register region
  //---------------------------------------------------------------------------
  logic                          ctrlport_shared_req_wr;
  logic                          ctrlport_shared_req_rd;
  logic [   CTRLPORT_ADDR_W-1:0] ctrlport_shared_req_addr;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_shared_req_data;
  logic [CTRLPORT_BYTE_EN_W-1:0] ctrlport_shared_req_byte_en;
  logic                          ctrlport_shared_req_has_time;
  logic [   CTRLPORT_TIME_W-1:0] ctrlport_shared_req_time;
  logic                          ctrlport_shared_resp_ack;
  logic [    CTRLPORT_STS_W-1:0] ctrlport_shared_resp_status;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_shared_resp_data;

  logic                          ctrlport_ports_req_wr;
  logic                          ctrlport_ports_req_rd;
  logic [   CTRLPORT_ADDR_W-1:0] ctrlport_ports_req_addr;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_ports_req_data;
  logic [CTRLPORT_BYTE_EN_W-1:0] ctrlport_ports_req_byte_en;
  logic                          ctrlport_ports_req_has_time;
  logic [   CTRLPORT_TIME_W-1:0] ctrlport_ports_req_time;
  logic                          ctrlport_ports_resp_ack;
  logic [    CTRLPORT_STS_W-1:0] ctrlport_ports_resp_status;
  logic [   CTRLPORT_DATA_W-1:0] ctrlport_ports_resp_data;

  ctrlport_decoder_param #(
    .NUM_SLAVES (2                                       ),
    .PORT_BASE  ({CTRLPORT_ADDR_W'(DUC_PORT_BASE_ADDR),
                  CTRLPORT_ADDR_W'(DUC_SHARED_BASE_ADDR)}),
    .PORT_ADDR_W({DUC_PORT_RANGE_W, DUC_SHARED_ADDR_W}   )
  ) ctrlport_decoder_param_shared_ports (
    .ctrlport_clk            (ce_clk                        ),
    .ctrlport_rst            (ce_rst                        ),
    .s_ctrlport_req_wr       (ctrlport_req_wr               ),
    .s_ctrlport_req_rd       (ctrlport_req_rd               ),
    .s_ctrlport_req_addr     (ctrlport_req_addr             ),
    .s_ctrlport_req_data     (ctrlport_req_data             ),
    .s_ctrlport_req_byte_en  (ctrlport_req_byte_en          ),
    .s_ctrlport_req_has_time (ctrlport_req_has_time         ),
    .s_ctrlport_req_time     (ctrlport_req_time             ),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack             ),
    .s_ctrlport_resp_status  (ctrlport_resp_status          ),
    .s_ctrlport_resp_data    (ctrlport_resp_data            ),
    // slave connections
    .m_ctrlport_req_wr       ({ctrlport_ports_req_wr,
                               ctrlport_shared_req_wr}      ),
    .m_ctrlport_req_rd       ({ctrlport_ports_req_rd,
                               ctrlport_shared_req_rd}      ),
    .m_ctrlport_req_addr     ({ctrlport_ports_req_addr,
                               ctrlport_shared_req_addr}    ),
    .m_ctrlport_req_data     ({ctrlport_ports_req_data,
                               ctrlport_shared_req_data}    ),
    .m_ctrlport_req_byte_en  ({ctrlport_ports_req_byte_en,
                               ctrlport_shared_req_byte_en} ),
    .m_ctrlport_req_has_time ({ctrlport_ports_req_has_time,
                               ctrlport_shared_req_has_time}),
    .m_ctrlport_req_time     ({ctrlport_ports_req_time,
                               ctrlport_shared_req_time}    ),
    .m_ctrlport_resp_ack     ({ctrlport_ports_resp_ack,
                               ctrlport_shared_resp_ack}    ),
    .m_ctrlport_resp_status  ({ctrlport_ports_resp_status,
                               ctrlport_shared_resp_status} ),
    .m_ctrlport_resp_data    ({ctrlport_ports_resp_data,
                               ctrlport_shared_resp_data}   )
  );

  //---------------------------------------------------------------------------
  // Split DUC Control Port Interfaces into per-port DUC instances
  // Note: the port base is 0 since the upstream decoder already took care
  //       of splitting the address range.
  //---------------------------------------------------------------------------
  logic [NUM_PORTS-1:0]                         ctrlport_port_req_wr;
  logic [NUM_PORTS-1:0]                         ctrlport_port_req_rd;
  logic [NUM_PORTS-1:0][   CTRLPORT_ADDR_W-1:0] ctrlport_port_req_addr;
  logic [NUM_PORTS-1:0][   CTRLPORT_DATA_W-1:0] ctrlport_port_req_data;
  logic [NUM_PORTS-1:0][CTRLPORT_BYTE_EN_W-1:0] ctrlport_port_req_byte_en;
  logic [NUM_PORTS-1:0]                         ctrlport_port_req_has_time;
  logic [NUM_PORTS-1:0][   CTRLPORT_TIME_W-1:0] ctrlport_port_req_time;
  logic [NUM_PORTS-1:0]                         ctrlport_port_resp_ack;
  logic [NUM_PORTS-1:0][    CTRLPORT_STS_W-1:0] ctrlport_port_resp_status;
  logic [NUM_PORTS-1:0][   CTRLPORT_DATA_W-1:0] ctrlport_port_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES  (NUM_PORTS),
    .BASE_ADDR   (CTRLPORT_ADDR_W'('h0)),
    .SLAVE_ADDR_W(DUC_PORT_ADDR_W)
  ) ctrlport_decoder_ports (
    .ctrlport_clk            (ce_clk                     ),
    .ctrlport_rst            (ce_rst                     ),
    .s_ctrlport_req_wr       (ctrlport_ports_req_wr      ),
    .s_ctrlport_req_rd       (ctrlport_ports_req_rd      ),
    .s_ctrlport_req_addr     (ctrlport_ports_req_addr    ),
    .s_ctrlport_req_data     (ctrlport_ports_req_data    ),
    .s_ctrlport_req_byte_en  (ctrlport_ports_req_byte_en ),
    .s_ctrlport_req_has_time (ctrlport_ports_req_has_time),
    .s_ctrlport_req_time     (ctrlport_ports_req_time    ),
    .s_ctrlport_resp_ack     (ctrlport_ports_resp_ack    ),
    .s_ctrlport_resp_status  (ctrlport_ports_resp_status ),
    .s_ctrlport_resp_data    (ctrlport_ports_resp_data   ),
    // slave connections
    .m_ctrlport_req_wr       (ctrlport_port_req_wr       ),
    .m_ctrlport_req_rd       (ctrlport_port_req_rd       ),
    .m_ctrlport_req_addr     (ctrlport_port_req_addr     ),
    .m_ctrlport_req_data     (ctrlport_port_req_data     ),
    .m_ctrlport_req_byte_en  (ctrlport_port_req_byte_en  ),
    .m_ctrlport_req_has_time (ctrlport_port_req_has_time ),
    .m_ctrlport_req_time     (ctrlport_port_req_time     ),
    .m_ctrlport_resp_ack     (ctrlport_port_resp_ack     ),
    .m_ctrlport_resp_status  (ctrlport_port_resp_status  ),
    .m_ctrlport_resp_data    (ctrlport_port_resp_data    )
  );

  for (genvar ch_idx = 0; ch_idx < NUM_PORTS; ch_idx = ch_idx + 1) begin : gen_multisample_chains
    rfnoc_block_duc_ms_channel #(
      // MTU is in CHDR words, but the channel operates on SAMP_W*SPC-wide
      // data words. Adjust so internal counters can represent the maximum
      // number of data words per packet.
      .NUM_HB         (NUM_HB),
      .CIC_MAX_INTERP (CIC_MAX_INTERP),
      .SAMP_W         (SAMP_W),
      .SPC            (SPC),
      .SPC_MTU_LOG2   (MTU + $clog2(CHDR_W / (SAMP_W * SPC)))
    ) rfnoc_block_duc_ms_channel_i (
      .ce_clk                (ce_clk                            ),
      .ce_rst                (ce_rst                            ),
      .ctrlport_req_wr       (ctrlport_port_req_wr[ch_idx]      ),
      .ctrlport_req_rd       (ctrlport_port_req_rd[ch_idx]      ),
      .ctrlport_req_addr     (ctrlport_port_req_addr[ch_idx]    ),
      .ctrlport_req_data     (ctrlport_port_req_data[ch_idx]    ),
      .ctrlport_req_byte_en  (ctrlport_port_req_byte_en[ch_idx] ),
      .ctrlport_req_has_time (ctrlport_port_req_has_time[ch_idx]),
      .ctrlport_req_time     (ctrlport_port_req_time[ch_idx]    ),
      .ctrlport_resp_ack     (ctrlport_port_resp_ack[ch_idx]    ),
      .ctrlport_resp_status  (ctrlport_port_resp_status[ch_idx] ),
      .ctrlport_resp_data    (ctrlport_port_resp_data[ch_idx]   ),
      .m_axis_data_tdata     (from_noc_tdata[ch_idx]            ),
      .m_axis_data_tlast     (from_noc_tlast[ch_idx]            ),
      .m_axis_data_tvalid    (from_noc_tvalid[ch_idx]           ),
      .m_axis_data_tready    (from_noc_tready[ch_idx]           ),
      .m_axis_data_ttimestamp(from_noc_ttimestamp[ch_idx]       ),
      .m_axis_data_thas_time (from_noc_thas_time[ch_idx]        ),
      .m_axis_data_tlength   (from_noc_tlength[ch_idx]          ),
      .m_axis_data_teob      (from_noc_teob[ch_idx]             ),
      .s_axis_data_tdata     (to_noc_tdata[ch_idx]              ),
      .s_axis_data_tlast     (to_noc_tlast[ch_idx]              ),
      .s_axis_data_tvalid    (to_noc_tvalid[ch_idx]             ),
      .s_axis_data_tready    (to_noc_tready[ch_idx]             ),
      .s_axis_data_teob      (to_noc_teob[ch_idx]               ),
      .s_axis_data_ttimestamp(to_noc_ttimestamp[ch_idx]         ),
      .s_axis_data_thas_time (to_noc_thas_time[ch_idx]          ),
      .s_axis_data_tlength   (to_noc_tlength[ch_idx]            )
    );
  end : gen_multisample_chains

  //---------------------------------------------------------------------------
  // Handle ctrlport_shared registers (currently just capability reporting)
  //---------------------------------------------------------------------------
  always_ff @(posedge ce_clk) begin
    ctrlport_shared_resp_ack    <= 1'b0;
    ctrlport_shared_resp_status <= ctrlport_pkg::CTRL_STS_OKAY;
    ctrlport_shared_resp_data   <= 32'h0;
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
        REG_CIC_MAX_INTERP: begin
          ctrlport_shared_resp_data <= CIC_MAX_INTERP;
          ctrlport_shared_resp_ack  <= 1'b1;
        end
        REG_SPC: begin
          ctrlport_shared_resp_data <= SPC;
          ctrlport_shared_resp_ack  <= 1'b1;
        end
        default: begin
          ctrlport_shared_resp_status <= ctrlport_pkg::CTRL_STS_CMDERR;
          ctrlport_shared_resp_ack    <= 1'b1;
        end
      endcase
    end
    if (ctrlport_shared_req_wr) begin
      ctrlport_shared_resp_status <= ctrlport_pkg::CTRL_STS_CMDERR;
      ctrlport_shared_resp_ack    <= 1'b1;
    end
    if (ce_rst) begin
      ctrlport_shared_resp_ack    <= 1'b0;
      ctrlport_shared_resp_status <= 'X;
      ctrlport_shared_resp_data   <= 'X;
    end
  end


endmodule : rfnoc_block_duc_ms


`default_nettype wire
