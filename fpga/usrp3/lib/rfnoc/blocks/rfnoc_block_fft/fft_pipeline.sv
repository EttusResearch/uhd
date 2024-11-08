//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_pipeline
//
// Description:
//
//   The module contains all FFT processing for a single channel, including
//   cyclic prefix removal, cyclic prefix insertion, FFT/IFFT, and logic to
//   change the output order of the FFT data.
//
//   The data is input on the data input (i_t*) and output on the data output
//   (o_t*) ports. There must be one FFT/IFFT per packet, plus cyclic-prefix to
//   be removed, if applicable.
//
//   The "global FFT settings" are treated as fixed values that won't change
//   for the duration of a single FFT/IFFT. These should only be updated when
//   everything is idle and there is no data in flight.
//
//   The fft_config_t* input contains the per-FFT settings for the Xilinx FFT
//   core and you should write once per FFT.
//
//   The cp_rem_t* and cp_ins_t* are the cyclic prefix removal and insertion
//   lengths. You should write one length per FFT/IFFT.
//
// Parameters:
//
//   MAX_FFT_SIZE_LOG2 : Set to the log base 2 of the maximum FFT size to be
//                       supported. For example, a value of 14 means the
//                       maximum FFT size is 2**14 = 4096.
//   EN_CONFIG_FIFO    : When 1, the fft_config_tdata AXI-Stream input is used
//                       in order to allow a unique configuration per FFT
//                       operation. If EN_CONFIG_FIFO is 0, then the fft_config
//                       input is used instead and it is assumed to be static
//                       for the duration of the FFT operation and must only
//                       change while the module is idle.
//   EN_CP_REMOVAL     : Controls whether to include the cyclic prefix removal
//                       logic.
//   EN_CP_INSERTION   : Controls whether to include the cyclic prefix
//                       insertion logic. If included, EN_FFT_ORDER must be 1.
//   EN_FFT_ORDER      : Set to 1 to add the optional FFT reorder core. Set to
//                       0 to remove it and save resources. Removing it also
//                       disable CP insertion.
//   EN_MAGNITUDE      : Set to 1 to add the magnitude output calculation core.
//                       Set to 0 to remove it and save resources.
//   EN_MAGNITUDE_SQ   : Set to 1 to add the magnitude squared output
//                       calculation core. Set to 0 to remove it and save
//                       resources.
//   USE_APPROX_MAG    : Control which magnitude calculation to use. Set to 1
//                       to use a simpler circuit that gives pretty good
//                       results in order to save resources. Set to 0 to use
//                       the CORDIC IP to calculate the magnitude.
//

`default_nettype none


module fft_pipeline
  import xfft_config_pkg::*;
#(
  int MAX_FFT_SIZE_LOG2 = 12,
  bit EN_CONFIG_FIFO    = 1,
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

  // FFT IP Configuration
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
  input  wire [DATA_W-1:0] i_tdata,
  input  wire              i_tlast,
  input  wire              i_tvalid,
  output wire              i_tready,

  // Data Output Packets
  output wire [DATA_W-1:0] o_tdata,
  output wire              o_tlast,
  output wire              o_tvalid,
  input  wire              o_tready
);

  //---------------------------------------------------------------------------
  // Input FIFOs
  //---------------------------------------------------------------------------

  logic [FFT_CONFIG_W-1:0] fft_config_fifo_tdata;
  logic                    fft_config_fifo_tvalid;
  logic                    fft_config_fifo_tready;

  logic [    CP_LEN_W-1:0] cp_rem_fifo_tdata;
  logic                    cp_rem_fifo_tvalid;
  logic                    cp_rem_fifo_tready;

  logic [    CP_LEN_W-1:0] cp_ins_fifo_tdata;
  logic                    cp_ins_fifo_tvalid;
  logic                    cp_ins_fifo_tready;

  logic [DATA_W-1:0] fft_fifo_tdata;
  logic              fft_fifo_tlast;
  logic              fft_fifo_tvalid;
  logic              fft_fifo_tready;

  if (EN_CONFIG_FIFO) begin : gen_config_fifo
    axi_fifo #(
      .WIDTH(FFT_CONFIG_W),
      .SIZE (1           )
    ) axi_fifo_fft_config (
      .clk     (clk                   ),
      .reset   (rst                   ),
      .clear   (1'b0                  ),
      .i_tdata (fft_config_tdata      ),
      .i_tvalid(fft_config_tvalid     ),
      .i_tready(fft_config_tready     ),
      .o_tdata (fft_config_fifo_tdata ),
      .o_tvalid(fft_config_fifo_tvalid),
      .o_tready(fft_config_fifo_tready),
      .space   (                      ),
      .occupied(                      )
    );
  end else begin : gen_no_config_fifo
    assign fft_config_tready = 1'b1;
  end

  if (EN_CP_REMOVAL) begin : gen_cp_rem_fifo
    axi_fifo #(
      .WIDTH(CP_LEN_W),
      .SIZE (1       )
    ) axi_fifo_cp_rem (
      .clk     (clk               ),
      .reset   (rst               ),
      .clear   (1'b0              ),
      .i_tdata (cp_rem_tdata      ),
      .i_tvalid(cp_rem_tvalid     ),
      .i_tready(cp_rem_tready     ),
      .o_tdata (cp_rem_fifo_tdata ),
      .o_tvalid(cp_rem_fifo_tvalid),
      .o_tready(cp_rem_fifo_tready),
      .space   (                  ),
      .occupied(                  )
    );
  end else begin : gen_no_cp_remo_fifo
    assign cp_rem_tready      = 1'b1;
    assign cp_rem_fifo_tdata  = '0;
    assign cp_rem_fifo_tvalid = 1'b1;
  end

  if (EN_CP_INSERTION) begin : gen_cp_ins_fifo
    axi_fifo #(
      .WIDTH(CP_LEN_W),
      .SIZE (1       )
    ) axi_fifo_cp_ins (
      .clk     (clk               ),
      .reset   (rst               ),
      .clear   (1'b0              ),
      .i_tdata (cp_ins_tdata      ),
      .i_tvalid(cp_ins_tvalid     ),
      .i_tready(cp_ins_tready     ),
      .o_tdata (cp_ins_fifo_tdata ),
      .o_tvalid(cp_ins_fifo_tvalid),
      .o_tready(cp_ins_fifo_tready),
      .space   (                  ),
      .occupied(                  )
    );
  end else begin : gen_no_cp_ins_fifo
    assign cp_ins_tready      = 1'b1;
    assign cp_ins_fifo_tdata  = '0;
    assign cp_ins_fifo_tvalid = 1'b1;
  end

  axi_fifo #(
    .WIDTH(1+DATA_W),
    .SIZE (1       )
  ) axi_fifo_fft (
    .clk     (clk                             ),
    .reset   (rst                             ),
    .clear   (1'b0                            ),
    .i_tdata ({i_tlast, i_tdata}              ),
    .i_tvalid(i_tvalid                        ),
    .i_tready(i_tready                        ),
    .o_tdata ({fft_fifo_tlast, fft_fifo_tdata}),
    .o_tvalid(fft_fifo_tvalid                 ),
    .o_tready(fft_fifo_tready                 ),
    .space   (                                ),
    .occupied(                                )
  );


  //---------------------------------------------------------------------------
  // Cyclic Prefix Removal
  //---------------------------------------------------------------------------

  logic [31:0] cp_rem_out_tdata;
  logic        cp_rem_out_tlast;
  logic        cp_rem_out_tvalid;
  logic        cp_rem_out_tready;

  if (EN_CP_REMOVAL) begin : gen_cp_removal
    cp_removal #(
      .CP_LEN_W (CP_LEN_W  ),
      .DATA_W   (DATA_W    )
    ) cp_removal_i (
      .clk          (clk               ),
      .rst          (rst               ),
      .cp_len_tdata (cp_rem_fifo_tdata ),
      .cp_len_tvalid(cp_rem_fifo_tvalid),
      .cp_len_tready(cp_rem_fifo_tready),
      .i_tdata      (fft_fifo_tdata    ),
      .i_tlast      (fft_fifo_tlast    ),
      .i_tvalid     (fft_fifo_tvalid   ),
      .i_tready     (fft_fifo_tready   ),
      .o_tdata      (cp_rem_out_tdata  ),
      .o_tlast      (cp_rem_out_tlast  ),
      .o_tvalid     (cp_rem_out_tvalid ),
      .o_tready     (cp_rem_out_tready )
    );
  end else begin : gen_no_cp_removal
    assign cp_rem_out_tdata  = fft_fifo_tdata;
    assign cp_rem_out_tlast  = fft_fifo_tlast;
    assign cp_rem_out_tvalid = fft_fifo_tvalid;
    assign fft_fifo_tready   = cp_rem_out_tready;
  end


  //---------------------------------------------------------------------------
  // XFFT Configuration Handling
  //---------------------------------------------------------------------------

  logic [31:0] fft_in_tdata;
  logic        fft_in_tlast;
  logic        fft_in_tvalid;
  logic        fft_in_tready;

  logic [FFT_CONFIG_W-1:0] fft_config_core_tdata;
  logic                    fft_config_core_tvalid;
  logic                    fft_config_core_tready;

  // Create a register that indicates the first word transfer of packet
  // (analogous to TLAST).
  logic fft_in_tfirst = 1'b1;

  always_ff @(posedge clk) begin
    if (rst) begin
      fft_in_tfirst <= 1'b1;
    end else if (fft_in_tvalid && fft_in_tready) begin
      fft_in_tfirst <= fft_in_tlast;
    end
  end

  always_comb begin
    if (EN_CONFIG_FIFO) begin
      // In FIFO mode we require one configuration write for each FFT/IFFT
      // packet that is input. This mode was used when the XFFT IP handled the
      // CP insertion but was no longer needed when the CP insertion was moved
      // to the reorder block. We keep it in the design in case we want to use
      // a mode that requires this again in the future.

      // Only pass FFT data from cp_rem_out to fft_in when the configuration
      // FIFO has a configuration for us.
      fft_in_tdata      = cp_rem_out_tdata;
      fft_in_tlast      = cp_rem_out_tlast;
      fft_in_tvalid     = cp_rem_out_tvalid && fft_config_fifo_tvalid;
      cp_rem_out_tready = fft_in_tready     && fft_config_fifo_tvalid;

      // Pass configuration from the fft_config_fifo to fft_config_core. Write
      // the configuration when the first sample is input into the FFT core and
      // pop the configuration off the configuration FIFO when the last sample
      // is input into the FFT core.
      fft_config_core_tdata  = fft_config_fifo_tdata;
      fft_config_core_tvalid = fft_in_tvalid && fft_in_tready && fft_in_tfirst;
      fft_config_fifo_tready = fft_in_tvalid && fft_in_tready && fft_in_tlast;

    end else begin
      // In non-FIFO mode we use whatever configuration value is on the
      // fft_config input.

      // Pass FFT data from cp_rem_out to fft_in
      fft_in_tdata      = cp_rem_out_tdata;
      fft_in_tlast      = cp_rem_out_tlast;
      fft_in_tvalid     = cp_rem_out_tvalid;
      cp_rem_out_tready = fft_in_tready;

      // Write the configuration when the first sample is input into the FFT
      // core.
      fft_config_core_tdata  = fft_config;
      fft_config_core_tvalid = fft_in_tvalid && fft_in_tready && fft_in_tfirst;
      fft_config_fifo_tready = 1'b1;
    end
  end

  //synthesis translate_off
  always_ff @(posedge clk) begin
    if (fft_config_core_tvalid && !fft_config_core_tready) begin
      $error("FFT configuration was not accepted by the XFFT core");
    end
  end
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // FFT IP Core
  //---------------------------------------------------------------------------

  logic [31:0] fft_out_tdata;
  logic        fft_out_tlast;
  logic        fft_out_tvalid;
  logic        fft_out_tready;

  logic event_tlast_unexpected;
  logic event_tlast_missing;

  xfft_wrapper #(
    .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2)
  ) xfft_wrapper_i (
    .aclk                       (clk                                          ),
    .aresetn                    (~rst                                         ),
    .s_axis_config_tdata        (fft_config_core_tdata                        ),
    .s_axis_config_tvalid       (fft_config_core_tvalid                       ),
    .s_axis_config_tready       (fft_config_core_tready                       ),
    .s_axis_data_tdata          ({ fft_in_tdata[15:0], fft_in_tdata[31:16] }  ),
    .s_axis_data_tlast          (fft_in_tlast                                 ),
    .s_axis_data_tvalid         (fft_in_tvalid                                ),
    .s_axis_data_tready         (fft_in_tready                                ),
    .m_axis_data_tdata          ({ fft_out_tdata[15:0], fft_out_tdata[31:16] }),
    .m_axis_data_tuser          (                                             ),
    .m_axis_data_tlast          (fft_out_tlast                                ),
    .m_axis_data_tvalid         (fft_out_tvalid                               ),
    .m_axis_data_tready         (fft_out_tready                               ),
    .m_axis_status_tdata        (                                             ),
    .m_axis_status_tvalid       (                                             ),
    .m_axis_status_tready       (1'b1                                         ),
    .event_frame_started        (                                             ),
    .event_tlast_unexpected     (event_tlast_unexpected                       ),
    .event_tlast_missing        (event_tlast_missing                          ),
    .event_fft_overflow         (event_fft_overflow                           ),
    .event_status_channel_halt  (                                             ),
    .event_data_in_channel_halt (                                             ),
    .event_data_out_channel_halt(                                             )
  );

  //synthesis translate_off
  always_ff @(posedge clk) begin
    // The packets are not being correctly sized if we get an unexpected or missing TLAST.
    assert (event_tlast_unexpected != 1'b1) else $error("FFT TLAST unexpected");
    assert (event_tlast_missing    != 1'b1) else $error("FFT TLAST missing");
    // Overflow can occur depending on the scaling settings and input data.
    assert (event_fft_overflow     != 1'b1) else $warning("FFT overflow");
  end
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // Magnitude and Data Order Post-Processing
  //---------------------------------------------------------------------------

  if (EN_FFT_ORDER || EN_MAGNITUDE || EN_MAGNITUDE_SQ) begin : gen_fft_post_processing
    logic [  DATA_W-1:0] pp_in_tdata;
    logic [CP_LEN_W-1:0] pp_in_tuser;
    logic                pp_in_tlast;
    logic                pp_in_tvalid;
    logic                pp_in_tready;

    // Only transfer data when both the data and CP FIFOs have their data
    // available.
    assign pp_in_tdata    = fft_out_tdata;
    assign pp_in_tuser    = cp_ins_fifo_tdata;
    assign pp_in_tlast    = fft_out_tlast;
    assign pp_in_tvalid   = fft_out_tvalid && cp_ins_fifo_tvalid;
    assign fft_out_tready = pp_in_tready   && cp_ins_fifo_tvalid;

    // Pop the CP off the FIFO at the end of the packet
    assign cp_ins_fifo_tready = pp_in_tready && pp_in_tvalid && pp_in_tlast;

    fft_post_processing #(
      .EN_FFT_ORDER     (EN_FFT_ORDER     ),
      .EN_CP_INSERTION  (EN_CP_INSERTION  ),
      .EN_MAGNITUDE     (EN_MAGNITUDE     ),
      .EN_MAGNITUDE_SQ  (EN_MAGNITUDE_SQ  ),
      .USE_APPROX_MAG   (USE_APPROX_MAG   ),
      .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2)
    ) fft_post_processing_i (
      .clk          (clk          ),
      .rst          (rst          ),
      .fft_order_sel(fft_order    ),
      .magnitude_sel(magnitude    ),
      .fft_size_log2(fft_size_log2),
      .s_axis_tdata (pp_in_tdata  ),
      .s_axis_tuser (pp_in_tuser  ),
      .s_axis_tlast (pp_in_tlast  ),
      .s_axis_tvalid(pp_in_tvalid ),
      .s_axis_tready(pp_in_tready ),
      .m_axis_tdata (o_tdata      ),
      .m_axis_tlast (o_tlast      ),
      .m_axis_tvalid(o_tvalid     ),
      .m_axis_tready(o_tready     )
    );
  end else begin : gen_no_fft_post_processing
    assign cp_ins_fifo_tready = 1'b1;
    assign o_tdata            = fft_out_tdata;
    assign o_tlast            = fft_out_tlast;
    assign o_tvalid           = fft_out_tvalid;
    assign fft_out_tready     = o_tready;
  end

endmodule : fft_pipeline


`default_nettype wire
