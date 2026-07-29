//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc
//
// Description:
//
//   A digital up-converter block for RFNoC. This version chooses between the
//   legacy single-sample-per-cycle version and the newer
//   multiple-sample-per-cycle implementation based on the NIPC parameter.
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
//   NIPC           : Number of items per clock. Use 1 for the single-sample
//                    implementation, or > 1 for multi-sample.
//   USE_MS         : Force use of the multisample variant, even if NIPC = 1
//

`default_nettype none


module rfnoc_block_duc #(
  bit [9:0] THIS_PORTID     = '0,
  int       CHDR_W          = 64,
  bit [5:0] MTU             = 10,
  int       NUM_PORTS       = 2,
  int       NUM_HB          = 3,
  int       CIC_MAX_INTERP  = 255,
  int       NIPC            = 1,
  bit       USE_MS          = 0,
  bit [5:0] CTRL_CLK_IDX    = 6'h3F,
  bit [5:0] TB_CLK_IDX      = 6'h3F
) (
  // RFNoC Framework Clocks and Resets
  input  wire                        rfnoc_chdr_clk,
  input  wire                        ce_clk,
  input  wire                        rfnoc_ctrl_clk,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [CHDR_W*NUM_PORTS-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [CHDR_W*NUM_PORTS-1:0] m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [                31:0] s_rfnoc_ctrl_tdata,
  input  wire                        s_rfnoc_ctrl_tlast,
  input  wire                        s_rfnoc_ctrl_tvalid,
  output wire                        s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [                31:0] m_rfnoc_ctrl_tdata,
  output wire                        m_rfnoc_ctrl_tlast,
  output wire                        m_rfnoc_ctrl_tvalid,
  input  wire                        m_rfnoc_ctrl_tready,
  // RFNoC Backend Interface
  input  wire [               511:0] rfnoc_core_config,
  output wire [               511:0] rfnoc_core_status
);

  if (NIPC == 1 && !USE_MS) begin : gen_ss
    rfnoc_block_duc_ss #(
      .THIS_PORTID    (THIS_PORTID   ),
      .CHDR_W         (CHDR_W        ),
      .NUM_PORTS      (NUM_PORTS     ),
      .MTU            (MTU           ),
      .NUM_HB         (NUM_HB        ),
      .CIC_MAX_INTERP (CIC_MAX_INTERP)
    ) rfnoc_block_duc_ss_i (
      .rfnoc_chdr_clk      (rfnoc_chdr_clk     ),
      .ce_clk              (ce_clk             ),
      .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata ),
      .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast ),
      .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
      .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
      .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata ),
      .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast ),
      .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
      .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
      .rfnoc_core_config   (rfnoc_core_config  ),
      .rfnoc_core_status   (rfnoc_core_status  ),
      .rfnoc_ctrl_clk      (rfnoc_ctrl_clk     ),
      .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata ),
      .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast ),
      .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
      .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
      .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata ),
      .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast ),
      .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
      .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready)
    );
  end else begin : gen_ms
    rfnoc_block_duc_ms #(
      .THIS_PORTID    (THIS_PORTID   ),
      .CHDR_W         (CHDR_W        ),
      .NUM_PORTS      (NUM_PORTS     ),
      .MTU            (MTU           ),
      .NUM_HB         (NUM_HB        ),
      .CIC_MAX_INTERP (CIC_MAX_INTERP),
      .NIPC           (NIPC          ),
      .CTRL_CLK_IDX   (CTRL_CLK_IDX  ),
      .TB_CLK_IDX     (TB_CLK_IDX    )
    ) rfnoc_block_duc_ms_i (
      .rfnoc_chdr_clk      (rfnoc_chdr_clk     ),
      .ce_clk              (ce_clk             ),
      .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata ),
      .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast ),
      .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
      .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
      .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata ),
      .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast ),
      .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
      .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
      .rfnoc_core_config   (rfnoc_core_config  ),
      .rfnoc_core_status   (rfnoc_core_status  ),
      .rfnoc_ctrl_clk      (rfnoc_ctrl_clk     ),
      .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata ),
      .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast ),
      .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
      .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
      .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata ),
      .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast ),
      .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
      .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready)
    );
  end

endmodule : rfnoc_block_duc


`default_nettype wire
