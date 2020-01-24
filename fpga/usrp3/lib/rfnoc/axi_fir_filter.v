//
// Copyright 2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Parameterized FIR filter with AXI stream interface.
// Has several optimizations to resource utilization such as
// using half the number of DSP slices for symmetric coefficients,
// skipping coefficients that are always set to zero, and using
// internal DSP slice registers to hold coefficients.
//
// For the most efficient DSP slice inference use these settings:
// - IN_WIDTH < 25, COEFF_WIDTH < 18, ACCUM_WIDTH < 48
//
// Parameters:
//   IN_WIDTH                 - Input width
//   COEFF_WIDTH              - Coefficient width
//   OUT_WIDTH                - Output width
//   NUM_COEFFS               - Number of coefficients / taps
//   CLIP_BITS                - If IN_WIDTH != OUT_WIDTH, number of MSBs to drop
//   ACCUM_WIDTH              - Accumulator width
//   COEFFS_VEC               - Vector of NUM_COEFFS values each of width COEFF_WIDTH to
//                              initialize coeffs. Defaults to an impulse.
//   RELOADABLE_COEFFS        - Enable (1) or disable (0) reloading coefficients at runtime (via reload bus)
//   BLANK_OUTPUT             - Disable (1) or enable (0) output when filling internal pipeline
//   SYMMETRIC_COEFFS         - Reduce multiplier usage by approx half if coefficients are symmetric
//   SKIP_ZERO_COEFFS         - Reduce multiplier usage by assuming zero valued coefficients in
//                              DEFAULT_COEFFS are always zero. Useful for halfband filters.
//   USE_EMBEDDED_REGS_COEFFS - Reduce register usage by only using embedded registers in DSP slices.
//                              Updating taps while streaming will cause temporary output corruption!
//
// Notes:
// - If using USE_EMBEDDED_REGS_COEFFS, coefficients must be written at least once as COEFFS_VEC is ignored!
// - If using SYMMETRIC_COEFFS, only send half the coeffients! i.e. NUM_COEFFS = 11, send the first 6.
//
module axi_fir_filter #(
  parameter IN_WIDTH                  = 16,
  parameter COEFF_WIDTH               = 16,
  parameter OUT_WIDTH                 = 16,
  parameter NUM_COEFFS                = 41,
  parameter CLIP_BITS                 = $clog2(NUM_COEFFS),
  parameter ACCUM_WIDTH               = IN_WIDTH+COEFF_WIDTH+$clog2(NUM_COEFFS)-1,
  parameter [NUM_COEFFS*COEFF_WIDTH-1:0] COEFFS_VEC =
      {{1'b0,{(COEFF_WIDTH-1){1'b1}}},{(COEFF_WIDTH*(NUM_COEFFS-1)){1'b0}}},
  parameter RELOADABLE_COEFFS         = 1,
  parameter BLANK_OUTPUT              = 1,
  // Optimizations
  parameter SYMMETRIC_COEFFS          = 1,
  parameter SKIP_ZERO_COEFFS          = 0,
  parameter USE_EMBEDDED_REGS_COEFFS  = 1
)(
  input clk,
  input reset,
  input clear,
  input [IN_WIDTH-1:0] s_axis_data_tdata,
  input s_axis_data_tlast,
  input s_axis_data_tvalid,
  output s_axis_data_tready,
  output [OUT_WIDTH-1:0] m_axis_data_tdata,
  output m_axis_data_tlast,
  output m_axis_data_tvalid,
  input m_axis_data_tready,
  input [COEFF_WIDTH-1:0] s_axis_reload_tdata,
  input s_axis_reload_tvalid,
  input s_axis_reload_tlast,
  output s_axis_reload_tready
);

  localparam NUM_SLICES       = SYMMETRIC_COEFFS ?
                                    NUM_COEFFS/2 + NUM_COEFFS[0] :  // Manual round up, Vivado complains when using $ceil()
                                    NUM_COEFFS;
  localparam ODD_LEN          = NUM_COEFFS[0];
  localparam PIPELINE_DELAY   = NUM_SLICES+4; // +4 pipeline depth in fir_filter_slice.v

  wire [ACCUM_WIDTH-1:0] m_axis_data_tdata_int;
  wire m_axis_data_tvalid_int, m_axis_data_tready_int, m_axis_data_tlast_int;

  ///////////////////////////////////////////////////////
  //
  // Coefficient loading / reloading
  //
  ///////////////////////////////////////////////////////
  reg [COEFF_WIDTH-1:0] coeffs[0:NUM_SLICES-1];
  reg coeff_load_stb = 1'b1;
  generate
    integer k;
    if (RELOADABLE_COEFFS) begin
      // Use DSP slice registers to hold coefficients. While loading
      // coefficients, input sample data should be throttled if corrupted
      // output samples are unacceptable.
      if (USE_EMBEDDED_REGS_COEFFS) begin
        always @(*) begin
          coeff_load_stb <= s_axis_reload_tvalid & s_axis_reload_tready;
        end
      // Use shift register to hold coefficients. Coefficients are loaded
      // into fir filter slice on tlast.
      end else begin
        always @(posedge clk) begin
          if (reset | clear) begin
            for (k = 0; k < NUM_SLICES; k = k + 1) begin
              coeffs[k] <= COEFFS_VEC[COEFF_WIDTH*k +: COEFF_WIDTH];
            end
            // Initialize coefficients at reset
            coeff_load_stb <= 1'b1;
          end else begin
            if (s_axis_reload_tvalid & s_axis_reload_tready) begin
              for (k = NUM_SLICES-1; k > 0; k = k - 1) begin
                coeffs[k-1] <= coeffs[k];
              end
              coeffs[NUM_SLICES-1] <= s_axis_reload_tdata;
            end
            coeff_load_stb <= s_axis_reload_tvalid & s_axis_reload_tready & s_axis_reload_tlast;
          end
        end
      end
    // Coefficients are static
    end else begin
      always @(*) begin
        for (k = 0; k < NUM_SLICES; k = k + 1) begin
          coeffs[k]      <= COEFFS_VEC[COEFF_WIDTH*k +: COEFF_WIDTH];
          coeff_load_stb <= 1'b1;
        end
      end
    end
  endgenerate

  assign s_axis_reload_tready = 1'b1;

  ///////////////////////////////////////////////////////
  //
  // Systolic FIR Filter
  //
  ///////////////////////////////////////////////////////
  //
  // Block Diagram
  // - Configuration: SYMMETRIC_COEFFS = 1 and USE_EMBEDDED_REGS_COEFFS = 1
  //
  //           +-------+
  // Sample In | Shift | Sample In delayed NUM_COEFF
  // +-------->|  Reg  |------------------------------------------------------------->
  //       |   +-------+             |                                |
  //       |                         v                                v
  //       |                      +-----+                          +-----+
  //       |                      |     |                          |     |
  //       |                      +-----+                          +-----+
  //       |                         |                                |
  //       |   +--+   +--+           |  Sample   +--+   +--+          |
  //       |   |  |   |  |           |  Forward  |  |   |  |          |
  //       '-->|  |-->|  |-----------^---------->|  |-->|  |----------^-------------->
  //           |  |   |  |       |   |           |  |   |  |      |   |
  //           +--+   +--+       v   v           +--+   +--+      v   v
  //                         +------------+                   +------------+
  //                         | Pre-Adder  |                   | Pre-Adder  |
  //                         +------------+                   +------------+
  //                               |                                |
  //                               v                                v
  //                            +-----+                          +-----+
  // *----------------------*   |     |                          |     |
  // | Note: Coeffs are     |   +-----+                          +-----+
  // | loaded backwards     |      |                                |
  // | for proper alignment |      |         .----------------------^----------------<
  // *----------------------*      |         |                      |
  //           +--+   +--+         v         |   +--+   +--+        v
  //  Coeff In |  |   |  |   +------------+  |   |  |   |  |  +------------+
  //      .--->|  |-->|  |-->| Multiplier |  '-->|  |-->|  |->| Multiplier |
  //      |    |  |   |  |   +------------+      |  | | |  |  +------------+
  //      |    +--+   +--+         |             +--+ | +--+        |
  //      |                        |                  |             |
  //      '------------------------^------------------'             |
  //                               |           Coeff                |
  //                               v           Forward              v
  //                            +-----+                          +-----+
  //                            |     |                          |     |
  //                            +-----+                          +-----+
  //                               |                                |
  //                               v          +--+ Sample           v          +--+
  //                         +------------+   |  | Out        +------------+   |  |
  //                         |   Adder    |-->|  |----------->|   Adder    |-->|  |-->
  //                         +------------+   |  |            +------------+   |  |
  //                                          +--+                             +--+
  //
  ///////////////////////////////////////////////////////
  genvar i, l;
  generate
    // Counter to track pipeline fullness
    reg [$clog2(PIPELINE_DELAY):0] cnt;
    always @(posedge clk) begin
      if (reset | clear) begin
        cnt <= 0;
      end else if (s_axis_data_tvalid & s_axis_data_tready) begin
        if (cnt < PIPELINE_DELAY) begin
          cnt <= cnt + 1;
        end
      end
    end

    // Sample delay shift register for efficient implementation
    // when using symmetric coefficients
    reg [IN_WIDTH-1:0] sample_shift_reg[0:NUM_COEFFS-1];
    integer n;
    initial begin
      for (n = 0; n < NUM_COEFFS; n = n + 1) begin
        sample_shift_reg[n] <= 0;
      end
    end
    always @(posedge clk) begin
      if (s_axis_data_tvalid & s_axis_data_tready) begin
        for (n = 1; n < NUM_COEFFS; n = n + 1) begin
          sample_shift_reg[n] <= sample_shift_reg[n-1];
        end
          sample_shift_reg[0] <= s_axis_data_tdata;
      end
    end

    // tlast shift register
    reg [PIPELINE_DELAY-1:0] tlast_shift_reg = 0;
    integer m;
    always @(posedge clk) begin
      if (s_axis_data_tvalid & s_axis_data_tready) begin
        for (m = 1; m < PIPELINE_DELAY; m = m + 1) begin
          tlast_shift_reg[m] <= tlast_shift_reg[m-1];
        end
        tlast_shift_reg[0]   <= s_axis_data_tlast;
      end
    end

    wire [IN_WIDTH-1:0] sample_in[0:NUM_SLICES];        // Use [0:NUM_SLICES] instead of
    wire [ACCUM_WIDTH-1:0] sample_accum[0:NUM_SLICES];  //   [0:NUM_SLICES-1] to make the
    wire [COEFF_WIDTH-1:0] coeff_forward[0:NUM_SLICES]; //   generate loop easier to read
    assign sample_in[0]              = s_axis_data_tdata;
    assign sample_accum[0]           = 0;
    assign coeff_forward[NUM_SLICES] = s_axis_reload_tdata;

    // Build up FIR filter with multiply-accumulate slices (fir_filter_slice)
    for (i = 0; i < NUM_SLICES; i = i + 1) begin
      // Map zero'd out coefficients to simple register delays.
      if ((SKIP_ZERO_COEFFS == 1) && (COEFFS_VEC[COEFF_WIDTH*i +: COEFF_WIDTH] == 0)) begin
        reg [ACCUM_WIDTH-1:0] sample_accum_reg;
        reg [IN_WIDTH-1:0] sample_in_reg[0:1];
        reg [COEFF_WIDTH-1:0] coeff_in_reg;
        always @(posedge clk) begin
          if (reset | clear) begin
            sample_in_reg[0] <= 0;
            sample_in_reg[1] <= 0;
            sample_accum_reg <= 0;
            coeff_in_reg     <= 0;
          end else begin
            if (s_axis_data_tvalid & s_axis_data_tready) begin
              sample_in_reg[0] <= sample_in[i];
              sample_in_reg[1] <= sample_in_reg[0];
              sample_accum_reg <= sample_accum[i];
            end
            if (coeff_load_stb) begin
              coeff_in_reg     <= coeff_forward[i+1];
            end
          end
        end
        assign sample_in[i+1]    = sample_in_reg[1];
        assign sample_accum[i+1] = sample_accum_reg;
        assign coeff_forward[i]  = coeff_in_reg;
      end else begin
        fir_filter_slice #(
          .IN_WIDTH(IN_WIDTH),
          .COEFF_WIDTH(COEFF_WIDTH),
          .ACCUM_WIDTH(ACCUM_WIDTH),
          .OUT_WIDTH(ACCUM_WIDTH))
        fir_filter_slice (
          .clk(clk),
          .reset(reset),
          .clear(clear),
          .sample_in_stb(s_axis_data_tvalid & s_axis_data_tready),
          .sample_in_a(sample_in[i]),
          // sample_in_b is used to implement symmetric coefficients, always 0 if SYMMETRIC_COEFFS = 0
          .sample_in_b(((SYMMETRIC_COEFFS == 0) || ((ODD_LEN == 1) && (i == NUM_SLICES-1))) ? {IN_WIDTH{1'b0}} : sample_shift_reg[NUM_COEFFS-1]),
          .sample_forward(sample_in[i+1]),
          // For proper coeffient loading, coeff_forward must be shifted in backwards. coeffs[] is already backwards.
          .coeff_in(((USE_EMBEDDED_REGS_COEFFS == 1) && (RELOADABLE_COEFFS == 1)) ? coeff_forward[i+1] : coeffs[i]),
          .coeff_forward(coeff_forward[i]),
          .coeff_load_stb(coeff_load_stb),
          .sample_accum(sample_accum[i]),
          .sample_out(sample_accum[i+1]));
      end
    end
    assign m_axis_data_tdata_int  = (BLANK_OUTPUT == 1) & (cnt < PIPELINE_DELAY) ? 0    : sample_accum[NUM_SLICES];
    assign m_axis_data_tvalid_int = (BLANK_OUTPUT == 1) & (cnt < PIPELINE_DELAY) ? 1'b0 : s_axis_data_tvalid;
    assign m_axis_data_tlast_int  = (BLANK_OUTPUT == 1) ? ((cnt < PIPELINE_DELAY) ? 1'b0 : tlast_shift_reg[PIPELINE_DELAY-1]) : s_axis_data_tlast;
    assign s_axis_data_tready     = m_axis_data_tready_int;
  endgenerate

  axi_round_and_clip #(
    .WIDTH_IN(ACCUM_WIDTH),
    .WIDTH_OUT(OUT_WIDTH),
    .CLIP_BITS(CLIP_BITS))
  inst_axi_round_and_clip (
    .clk(clk),
    .reset(reset | clear),
    .i_tdata(m_axis_data_tdata_int),
    .i_tlast(m_axis_data_tlast_int),
    .i_tvalid(m_axis_data_tvalid_int),
    .i_tready(m_axis_data_tready_int),
    .o_tdata(m_axis_data_tdata),
    .o_tlast(m_axis_data_tlast),
    .o_tvalid(m_axis_data_tvalid),
    .o_tready(m_axis_data_tready));

endmodule
