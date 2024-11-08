//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_pipeline_wrapper
//
// Description:
//
//   This module takes a multiple-item-per-cycle data stream and splits the
//   processing of packets across multiple FFT instances, each processing a
//   single item/sample per cycle.
//
//   See fft_pipeline for documentation of the fft_pipeline parameters and
//   ports. To avoid duplication, it is not repeated here.
//
// Parameters:
//
//   NIPC : Items/samples per cycle on the input/output data port (i_t*, o_t*d)
//

`default_nettype none


module fft_pipeline_wrapper
  import xfft_config_pkg::*;
#(
  int NIPC = 1,

  // Parameters for fft_pipeline
  int MAX_FFT_SIZE_LOG2 = 12,
  bit EN_CONFIG_FIFO    = 0,
  bit EN_CP_REMOVAL     = 1,
  bit EN_CP_INSERTION   = 1,
  bit EN_FFT_ORDER      = 1,
  bit EN_MAGNITUDE      = 1,
  bit EN_MAGNITUDE_SQ   = 1,
  bit USE_APPROX_MAG    = 1,

  localparam int FFT_CONFIG_W    = fft_config_w(MAX_FFT_SIZE_LOG2),
  localparam int DATA_W          = 32,
  localparam int FFT_SIZE_LOG2_W = $clog2(MAX_FFT_SIZE_LOG2+1),
  localparam int CP_LEN_W        = MAX_FFT_SIZE_LOG2
) (
  input  wire clk,
  input  wire rst,

  // Global FFT settings
  input  wire [                1:0] fft_order,
  input  wire [                1:0] magnitude,
  input  wire [FFT_SIZE_LOG2_W-1:0] fft_size_log2,
  input  wire [   FFT_CONFIG_W-1:0] fft_config,

  // FFT Configuration
  input  wire [FFT_CONFIG_W-1:0] fft_config_tdata,
  input  wire                    fft_config_tvalid,
  output wire                    fft_config_tready,

  // CP Removal Length
  input  wire [CP_LEN_W-1:0] cp_rem_tdata,
  input  wire                cp_rem_tvalid,
  output wire                cp_rem_tready,

  // CP Insertion Length
  input  wire [CP_LEN_W-1:0] cp_ins_tdata,
  input  wire                cp_ins_tvalid,
  output reg                 cp_ins_tready,

  // FFT Event Monitoring
  output wire event_fft_overflow,

  // Data Input Packets
  input  wire [NIPC*DATA_W-1:0] i_tdata,
  input  wire                   i_tlast,
  input  wire                   i_tvalid,
  output wire                   i_tready,

  // Data Output Packets
  output wire [NIPC*DATA_W-1:0] o_tdata,
  output wire                   o_tlast,
  output wire                   o_tvalid,
  input  wire                   o_tready
);

  `include "usrp_utils.svh"

  if (NIPC == 1) begin : gen_one_spc

    fft_pipeline #(
      .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2),
      .EN_CONFIG_FIFO   (EN_CONFIG_FIFO   ),
      .EN_CP_REMOVAL    (EN_CP_REMOVAL    ),
      .EN_CP_INSERTION  (EN_CP_INSERTION  ),
      .EN_FFT_ORDER     (EN_FFT_ORDER     ),
      .EN_MAGNITUDE     (EN_MAGNITUDE     ),
      .EN_MAGNITUDE_SQ  (EN_MAGNITUDE_SQ  ),
      .USE_APPROX_MAG   (USE_APPROX_MAG   )
    ) fft_pipeline_i (
      .clk               (clk               ),
      .rst               (rst               ),
      .fft_order         (fft_order         ),
      .magnitude         (magnitude         ),
      .fft_config        (fft_config        ),
      .fft_size_log2     (fft_size_log2     ),
      .fft_config_tdata  (fft_config_tdata  ),
      .fft_config_tvalid (fft_config_tvalid ),
      .fft_config_tready (fft_config_tready ),
      .cp_rem_tdata      (cp_rem_tdata      ),
      .cp_rem_tvalid     (cp_rem_tvalid     ),
      .cp_rem_tready     (cp_rem_tready     ),
      .cp_ins_tdata      (cp_ins_tdata      ),
      .cp_ins_tvalid     (cp_ins_tvalid     ),
      .cp_ins_tready     (cp_ins_tready     ),
      .event_fft_overflow(event_fft_overflow),
      .i_tdata           (i_tdata           ),
      .i_tlast           (i_tlast           ),
      .i_tvalid          (i_tvalid          ),
      .i_tready          (i_tready          ),
      .o_tdata           (o_tdata           ),
      .o_tlast           (o_tlast           ),
      .o_tvalid          (o_tvalid          ),
      .o_tready          (o_tready          )
    );

  end else begin : gen_multi_spc

    logic [FFT_CONFIG_W-1:0] fft_config_split_tdata [NIPC];
    logic                    fft_config_split_tvalid[NIPC];
    logic                    fft_config_split_tready[NIPC];

    logic [CP_LEN_W-1:0]     cp_rem_split_tdata     [NIPC];
    logic                    cp_rem_split_tvalid    [NIPC];
    logic                    cp_rem_split_tready    [NIPC];

    logic [CP_LEN_W-1:0]     cp_ins_split_tdata     [NIPC];
    logic                    cp_ins_split_tvalid    [NIPC];
    logic                    cp_ins_split_tready    [NIPC];

    logic [DATA_W-1:0]       i_split_tdata          [NIPC];
    logic                    i_split_tlast          [NIPC];
    logic                    i_split_tvalid         [NIPC];
    logic                    i_split_tready         [NIPC];

    logic [DATA_W-1:0]       o_split_tdata          [NIPC];
    logic                    o_split_tlast          [NIPC];
    logic                    o_split_tvalid         [NIPC];
    logic                    o_split_tready         [NIPC];

    logic [NIPC-1:0] event_fft_overflow_split;

    if (EN_CONFIG_FIFO) begin : gen_config_fifo_split
      axis_load_split #(
        .IN_DATA_W    (FFT_CONFIG_W),
        .IN_FIFO_SIZE (1           ),
        .OUT_DATA_W   (FFT_CONFIG_W),
        .OUT_FIFO_SIZE(1           ),
        .OUT_NUM_PORTS(NIPC        )
      ) axis_load_split_fft_config (
        .clk     (clk                    ),
        .rst     (rst                    ),
        .i_tdata (fft_config_tdata       ),
        .i_tuser ('0                     ),
        .i_tlast (1'b1                   ),
        .i_tvalid(fft_config_tvalid      ),
        .i_tready(fft_config_tready      ),
        .o_tdata (fft_config_split_tdata ),
        .o_tuser (                       ),
        .o_tlast (                       ),
        .o_tvalid(fft_config_split_tvalid),
        .o_tready(fft_config_split_tready)
      );
    end else begin : gen_no_config_fifo_split
      assign fft_config_tready = 1'b1;
      for (genvar idx=0; idx < NIPC; idx++) begin : gen_assign
        assign fft_config_split_tvalid[idx] = 1'b0;
      end
    end

    axis_load_split #(
      .IN_DATA_W    (CP_LEN_W),
      .IN_FIFO_SIZE (1       ),
      .OUT_DATA_W   (CP_LEN_W),
      .OUT_FIFO_SIZE(1       ),
      .OUT_NUM_PORTS(NIPC    )
    ) axis_load_split_cp_rem (
      .clk     (clk                ),
      .rst     (rst                ),
      .i_tdata (cp_rem_tdata       ),
      .i_tuser ('0                 ),
      .i_tlast (1'b1               ),
      .i_tvalid(cp_rem_tvalid      ),
      .i_tready(cp_rem_tready      ),
      .o_tdata (cp_rem_split_tdata ),
      .o_tuser (                   ),
      .o_tlast (                   ),
      .o_tvalid(cp_rem_split_tvalid),
      .o_tready(cp_rem_split_tready)
    );

    axis_load_split #(
      .IN_DATA_W    (CP_LEN_W),
      .IN_FIFO_SIZE (1       ),
      .OUT_DATA_W   (CP_LEN_W),
      .OUT_FIFO_SIZE(1       ),
      .OUT_NUM_PORTS(NIPC    )
    ) axis_load_split_cp_ins (
      .clk     (clk                ),
      .rst     (rst                ),
      .i_tdata (cp_ins_tdata       ),
      .i_tuser ('0                 ),
      .i_tlast (1'b1               ),
      .i_tvalid(cp_ins_tvalid      ),
      .i_tready(cp_ins_tready      ),
      .o_tdata (cp_ins_split_tdata ),
      .o_tuser (                   ),
      .o_tlast (                   ),
      .o_tvalid(cp_ins_split_tvalid),
      .o_tready(cp_ins_split_tready)
    );

    axis_load_split #(
      .IN_DATA_W    (NIPC*DATA_W      ),
      .IN_FIFO_SIZE (1                ),
      .OUT_DATA_W   (DATA_W           ),
      .OUT_FIFO_SIZE(MAX_FFT_SIZE_LOG2),
      .OUT_NUM_PORTS(NIPC             ),
      .USER_W       (1                )
    ) axis_load_split_data (
      .clk     (clk           ),
      .rst     (rst           ),
      .i_tdata (i_tdata       ),
      .i_tuser ('0            ),
      .i_tlast (i_tlast       ),
      .i_tvalid(i_tvalid      ),
      .i_tready(i_tready      ),
      .o_tdata (i_split_tdata ),
      .o_tuser (              ),
      .o_tlast (i_split_tlast ),
      .o_tvalid(i_split_tvalid),
      .o_tready(i_split_tready)
    );

    for (genvar samp_i = 0; samp_i < NIPC; samp_i++) begin : gen_pipelines
      fft_pipeline #(
        .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2),
        .EN_CONFIG_FIFO   (EN_CONFIG_FIFO   ),
        .EN_CP_REMOVAL    (EN_CP_REMOVAL    ),
        .EN_CP_INSERTION  (EN_CP_INSERTION  ),
        .EN_FFT_ORDER     (EN_FFT_ORDER     ),
        .EN_MAGNITUDE     (EN_MAGNITUDE     ),
        .EN_MAGNITUDE_SQ  (EN_MAGNITUDE_SQ  ),
        .USE_APPROX_MAG   (USE_APPROX_MAG   )
      ) fft_pipeline_i (
        .clk               (clk                             ),
        .rst               (rst                             ),
        .fft_order         (fft_order                       ),
        .magnitude         (magnitude                       ),
        .fft_size_log2     (fft_size_log2                   ),
        .fft_config        (fft_config                      ),
        .fft_config_tdata  (fft_config_split_tdata  [samp_i]),
        .fft_config_tvalid (fft_config_split_tvalid [samp_i]),
        .fft_config_tready (fft_config_split_tready [samp_i]),
        .cp_rem_tdata      (cp_rem_split_tdata      [samp_i]),
        .cp_rem_tvalid     (cp_rem_split_tvalid     [samp_i]),
        .cp_rem_tready     (cp_rem_split_tready     [samp_i]),
        .cp_ins_tdata      (cp_ins_split_tdata      [samp_i]),
        .cp_ins_tvalid     (cp_ins_split_tvalid     [samp_i]),
        .cp_ins_tready     (cp_ins_split_tready     [samp_i]),
        .event_fft_overflow(event_fft_overflow_split[samp_i]),
        .i_tdata           (i_split_tdata           [samp_i]),
        .i_tlast           (i_split_tlast           [samp_i]),
        .i_tvalid          (i_split_tvalid          [samp_i]),
        .i_tready          (i_split_tready          [samp_i]),
        .o_tdata           (o_split_tdata           [samp_i]),
        .o_tlast           (o_split_tlast           [samp_i]),
        .o_tvalid          (o_split_tvalid          [samp_i]),
        .o_tready          (o_split_tready          [samp_i])
      );
    end

    axis_load_merge #(
      .IN_DATA_W    (DATA_W           ),
      .IN_FIFO_SIZE (MAX_FFT_SIZE_LOG2),
      .IN_NUM_PORTS (NIPC             ),
      .OUT_DATA_W   (NIPC*DATA_W      ),
      .OUT_FIFO_SIZE(1                ),
      .USER_W       (1                )
    ) axis_load_merge_data (
      .clk     (clk           ),
      .rst     (rst           ),
      .i_tdata (o_split_tdata ),
      .i_tuser (              ),
      .i_tlast (o_split_tlast ),
      .i_tvalid(o_split_tvalid),
      .i_tready(o_split_tready),
      .o_tdata (o_tdata       ),
      .o_tuser (              ),
      .o_tlast (o_tlast       ),
      .o_tvalid(o_tvalid      ),
      .o_tready(o_tready      )
    );

    assign event_fft_overflow = |event_fft_overflow_split;

  end

endmodule : fft_pipeline_wrapper


`default_nettype wire
