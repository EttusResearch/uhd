//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Parameterized multi-sample FIR filter with AXI-stream interface.
//
// Description:
//
//   For each sample per cycle a separate FIR filter with the given number of coefficients is
//   instantiated. The filter is implemented as a chain of multiply-accumulate slices.
//   A shift register is used to store the input samples as long as they are needed.
//   The FIR filters are fed with the appropriate samples from the shift register.
//   The indices into the shift registers are calculated at compile time.
//
//   For the most efficient DSP slice inference use these settings:
//   IN_WIDTH < 25, COEFF_WIDTH < 18, ACCUM_WIDTH < 48
//
// Parameters (widths are in bits):
//
//   IN_WIDTH                 - Input width of a single sample
//   NUM_SPC                  - Samples per cycle
//   COEFF_WIDTH              - Coefficient width
//   OUT_WIDTH                - Output width of a single sample
//   NUM_COEFFS               - Number of coefficients / taps
//   CLIP_BITS                - If IN_WIDTH != OUT_WIDTH, number of MSBs to drop
//   ACCUM_WIDTH              - Accumulator width
//   COEFFS_VEC               - Vector of NUM_COEFFS values, each of width COEFF_WIDTH to
//                              initialize coeffs. Defaults to an impulse.
//   RELOADABLE_COEFFS        - Enable (1) or disable (0) reloading coefficients at runtime (via reload bus)
//   BLANK_OUTPUT             - Disable (1) or enable (0) output when initially filling internal pipeline
//   USE_EMBEDDED_REGS_COEFFS - Reduce register usage by only using embedded registers in DSP slices.
//                              Updating taps while streaming will cause temporary output corruption!
//  
// Notes:
// - If using USE_EMBEDDED_REGS_COEFFS, coefficients must be written at least once as COEFFS_VEC is ignored!
// - If using RELOADABLE_COEFFS, coefficients must be written in reverse order!
// - Optimizations: For static coefficients (RELOADABLE_COEFFS = 0), zero and symmetric coefficient
//                  optimizations are automatically enabled to reduce DSP slice utilization.
//                  Symmetric coefficients save ~50% of DSP slices by reusing computations.
//                  Zero coefficients are skipped, saving DSP slices proportionally
//                  (e.g., halfband filters save ~50%).

module axi_fir_multisample_filter #(
  int IN_WIDTH                  = 16,
  int NUM_SPC                   = 4,
  int COEFF_WIDTH               = 16,
  int OUT_WIDTH                 = 16,
  int NUM_COEFFS                = 41,
  int CLIP_BITS                 = $clog2(NUM_COEFFS),
  int ACCUM_WIDTH               = IN_WIDTH+COEFF_WIDTH+$clog2(NUM_COEFFS)-1,
  bit [COEFF_WIDTH-1:0] COEFFS_VEC [NUM_COEFFS] = '{0: (1 << (COEFF_WIDTH-1)) - 1, default: 0},
  bit RELOADABLE_COEFFS         = 1,
  bit BLANK_OUTPUT              = 1,
  bit USE_EMBEDDED_REGS_COEFFS  = 1
)(
  // clocks and control signals
  input  logic clk,
  input  logic reset,
  input  logic clear,

  // AXI stream data input interface
  input  logic [NUM_SPC*IN_WIDTH-1:0] s_axis_data_tdata,
  input  logic s_axis_data_tlast,
  input  logic s_axis_data_tvalid,
  output logic s_axis_data_tready,

  // AXI stream data output interface
  output logic [NUM_SPC*OUT_WIDTH-1:0] m_axis_data_tdata,
  output logic m_axis_data_tlast,
  output logic m_axis_data_tvalid,
  input  logic m_axis_data_tready,

  // AXI stream coefficient interface
  input  logic [COEFF_WIDTH-1:0] s_axis_reload_tdata,
  input  logic s_axis_reload_tvalid,
  input  logic s_axis_reload_tlast,
  output logic s_axis_reload_tready
);

  // define function to calculate the number of zero coefficients up to the
  // given index
  function int count_zeros_up_to_index (
    input int index
  );
    int count;
    begin
      count = 0;
      for (int i = 0; i < index; i = i + 1) begin
        if (COEFFS_VEC[i] == '0) begin
          count = count + 1;
        end
      end
      return count;
    end
  endfunction
  // function to check coefficient symmetry
  function bit check_coeffs_symmetric();
    begin
      check_coeffs_symmetric = 1'b1;
      for (int i = 0; i < NUM_COEFFS/2; i = i + 1) begin
        if (COEFFS_VEC[i] != COEFFS_VEC[NUM_COEFFS-1-i]) begin
          check_coeffs_symmetric = 1'b0;
        end
      end
      return check_coeffs_symmetric;
    end
  endfunction

  localparam bit OPTIMIZE_FOR_ZERO_COEFFS      = (RELOADABLE_COEFFS == 0);
  localparam bit OPTIMIZE_FOR_SYMMETRIC_COEFFS = (RELOADABLE_COEFFS == 0)
    ? check_coeffs_symmetric() : 0;
  localparam int NUM_SLICES      = OPTIMIZE_FOR_SYMMETRIC_COEFFS
    ? NUM_COEFFS/2 + NUM_COEFFS[0] : NUM_COEFFS;
  localparam int PIPELINE_DELAY  = NUM_SLICES + 5 // +4 pipeline depth in fir_filter_slice.v, +1 of shift register
    - (OPTIMIZE_FOR_ZERO_COEFFS ? count_zeros_up_to_index(NUM_SLICES) : 0);
  localparam int SHIFT_REG_WIDTH = (NUM_SPC + 1) * NUM_COEFFS; // length of shift register

  logic [ACCUM_WIDTH-1:0] m_axis_data_tdata_int [NUM_SPC-1:0];
  logic [NUM_SPC-1:0]     m_axis_data_tvalid_int;
  logic [NUM_SPC-1:0]     m_axis_data_tready_int;
  logic [NUM_SPC-1:0]     m_axis_data_tlast_int;
  logic [NUM_SPC-1:0]     m_axis_data_tvalid_array;
  logic [NUM_SPC-1:0]     m_axis_data_tlast_array;

  ///////////////////////////////////////////////////////
  //
  // Coefficient loading / reloading
  //
  ///////////////////////////////////////////////////////

  reg [COEFF_WIDTH-1:0] coeffs[0:NUM_COEFFS-1];
  reg coeff_load_stb = 1'b1;
  generate
    if (RELOADABLE_COEFFS) begin
      // Use DSP slice registers to hold coefficients. While loading
      // coefficients, input sample data should be throttled if corrupted
      // output samples are unacceptable
      if (USE_EMBEDDED_REGS_COEFFS) begin
        always @(*) begin
          coeff_load_stb <= s_axis_reload_tvalid & s_axis_reload_tready;
        end
      end else begin
        always @(posedge clk) begin
          if (reset | clear) begin
            for (int k = 0; k < NUM_COEFFS; k = k + 1) begin
              coeffs[k] <= COEFFS_VEC[k];
            end
            // Initialize coefficients at reset
            coeff_load_stb <= 1'b1;
          end else begin
            if (s_axis_reload_tvalid & s_axis_reload_tready) begin
              // Inverted direction to reload coeff
              for (int k = NUM_COEFFS-1; k > 0; k = k - 1) begin
                coeffs[k] <= coeffs[k-1];
              end
              coeffs[0] <= s_axis_reload_tdata;
            end
            coeff_load_stb <= s_axis_reload_tvalid & s_axis_reload_tready & s_axis_reload_tlast;
          end
       end
      end
    end else begin
      // Coefficients are static
      initial begin
        for (int k = NUM_COEFFS-1; k >= 0; k = k - 1) begin
          coeffs[k]      <= COEFFS_VEC[k];
          coeff_load_stb <= 1'b1;
        end
      end
    end
  endgenerate

  assign s_axis_reload_tready = 1'b1;

  ///////////////////////////////////////////////////////
  //
  // Multisample FIR Filter
  //
  ///////////////////////////////////////////////////////

  reg [IN_WIDTH-1:0] data_shift_reg [0 : SHIFT_REG_WIDTH-1];

  initial begin
    for (int k= 0; k < SHIFT_REG_WIDTH; k = k + 1) begin
      data_shift_reg[k] <= 0;
    end
  end

  // s_axis_data_tdata given as x[n-1],x[n-2],...,x[2],x[1],x[0].
  // data_shift_reg is organized to contain samples in natural order.
  // data_shift_reg index ... | 5  | 4 |  3  | 2  | 1  |  0 |
  //  cycle 0:            ....| ?  | ?  | ?  | x0 | x1 | x2 |
  //  cycle 1:            ....| x0 | x1 | x2 | x3 | x4 | x5 |
  // Data from axi data port is stored in reversed order starting from shift register index 0.
  // For remaining indices samples are shifted by NUM_SPC each cycle.
  // data_shift_reg works like:
  //                          <---- shift by NUM_SPC

  always @(posedge clk) begin
    if (s_axis_data_tvalid & s_axis_data_tready) begin
      // Wire input to lower register position
      for (int k = 0; k < NUM_SPC; k = k + 1) begin
        automatic int k_flipped = NUM_SPC-k-1;
        data_shift_reg[k] <= s_axis_data_tdata [k_flipped *IN_WIDTH +: IN_WIDTH ];
      end
      // Shift contents by NUM_SPC to the upper position of shift register
      for (int k = NUM_SPC; k < SHIFT_REG_WIDTH; k = k + 1) begin
        data_shift_reg[k] <= data_shift_reg[k-NUM_SPC];
      end
    end
  end

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

  // tlast shift register
  reg [PIPELINE_DELAY-1:0] tlast_shift_reg = 0;
  always @(posedge clk) begin
    if (s_axis_data_tvalid & s_axis_data_tready) begin
      for (int k = 1; k < PIPELINE_DELAY; k = k + 1) begin
        tlast_shift_reg[k] <= tlast_shift_reg[k-1];
      end
      tlast_shift_reg[0]   <= s_axis_data_tlast;
    end
  end

  // Instantiate NUM_SPC-numbers of DSP-chain
  generate
    for (genvar k = 0; k < NUM_SPC; k = k + 1) begin : gen_DSP_chain
      // K_FLIPPED: refer to the documentation above of data_shift_reg
      localparam int K_FLIPPED = NUM_SPC-k-1;

      logic [ACCUM_WIDTH-1:0] sample_accum  [0 : NUM_SLICES];   //  [0:NUM_COEFFS] to make the
      logic [COEFF_WIDTH-1:0] coeff_forward [0 : NUM_SLICES];   //  generate loop easier to read

      assign sample_accum[0]  =  0;
      assign coeff_forward[0] = s_axis_reload_tdata;

      // Build up FIR filter with multiply-accumulate slices (fir_filter_slice).
      // Now generate the slices for each chain
      for (genvar j = 0; j < NUM_SLICES ; j = j + 1) begin  : gen_slice
        // skip zero coefficients: always for static coeffs
        if (OPTIMIZE_FOR_ZERO_COEFFS && COEFFS_VEC[j] == '0) begin
          assign sample_accum[j+1] = sample_accum[j];
          always_ff @(posedge clk) begin
            if (reset | clear) begin
              coeff_forward[j+1] <= '0;
            end else if (coeff_load_stb) begin
              coeff_forward[j+1] <= coeff_forward[j];
            end
          end
        end else begin
          localparam int GAP = OPTIMIZE_FOR_ZERO_COEFFS ? count_zeros_up_to_index(j) : 0;
          localparam bit APPLY_SYMMETRY = (OPTIMIZE_FOR_SYMMETRIC_COEFFS == 1) // enabled?
            && !((NUM_COEFFS[0] == 1) && (j == NUM_SLICES-1));  // not the center tap for odd NUM_COEFFS

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
            // NUM_SPC is added j times due to the pipeline delay.
            // +1 for selecting the next index for the FIR result calculation
            // K_FLIPPED is the offset into data_shift_reg.
            // If zero tap filter slices are skipped, we need to pick samples
            // GAP cycles earlier to keep the correct alignment which corresponds
            // to subtracting GAP*NUM_SPC from the buffer index.
            .sample_in_a(data_shift_reg[ (j)*(NUM_SPC+1) - GAP*NUM_SPC + K_FLIPPED ]),
            // sample_in_b is used to implement symmetric coefficients
            // If not symmetric or middle tap of odd number of coeffs, tie to zero
            // otherwise select the mirrored sample from the shift register.
            // Mirror index is NUM_COEFFS-1-j, since sample_in_b inside fir_filter_slice
            // has one cycle delay less that sample_in_a, we need to take it one cycle
            // later such that both values are in-sync. Thus we add another NUM_SPC to the index.
            .sample_in_b(APPLY_SYMMETRY ? data_shift_reg[ (NUM_COEFFS-1-j) + (j-GAP+1)*NUM_SPC + K_FLIPPED ] : '0),
            .sample_forward(),
            // For proper coeffient loading, coeff_forward must be shifted in backwards. coeffs[] is already backwards
            .coeff_in(((USE_EMBEDDED_REGS_COEFFS == 1) && (RELOADABLE_COEFFS == 1)) ? coeff_forward[j] : coeffs[j]),
            .coeff_forward(coeff_forward[j+1]),
            .coeff_load_stb(coeff_load_stb),
            .sample_accum(sample_accum[j]),
            // Connect the accumulator chain from previous to next slice instance
            .sample_out(sample_accum[j+1])
          );
        end
      end : gen_slice

      always_comb begin
        // zero data and valid bit for the ring-in of the pipeline
        if (BLANK_OUTPUT == 1 && cnt < PIPELINE_DELAY) begin
          m_axis_data_tdata_int[k]  = '0;
          m_axis_data_tvalid_int[k] = '0;
        end else begin
          m_axis_data_tdata_int[k]  = sample_accum[NUM_SLICES];
          m_axis_data_tvalid_int[k] = s_axis_data_tvalid;
        end
        // tlast is masked the same way during ring-in.
        // Depending on the blanking mode tlast will be delayed or taken from the input.
        if (BLANK_OUTPUT == 1) begin
          if (cnt < PIPELINE_DELAY) begin
            m_axis_data_tlast_int[k] = 1'b0;
          end else begin
            m_axis_data_tlast_int[k] = tlast_shift_reg[PIPELINE_DELAY-1];
          end
        end else begin
          m_axis_data_tlast_int[k] = s_axis_data_tlast;
        end
      end

      axi_round_and_clip #(
        .WIDTH_IN(ACCUM_WIDTH),
        .WIDTH_OUT(OUT_WIDTH),
        .CLIP_BITS(CLIP_BITS))
      inst_axi_round_and_clip (
        .clk(clk),
        .reset(reset | clear),
        .i_tdata(m_axis_data_tdata_int[k]),
        .i_tlast(m_axis_data_tlast_int[k]),
        .i_tvalid(m_axis_data_tvalid_int[k]),
        .i_tready(m_axis_data_tready_int[k]),  // output
        .o_tdata(m_axis_data_tdata[k*OUT_WIDTH +: OUT_WIDTH]),
        .o_tlast(m_axis_data_tlast_array[k]),
        .o_tvalid(m_axis_data_tvalid_array[k]),
        .o_tready(m_axis_data_tready) //input
      );
    end  : gen_DSP_chain
  endgenerate

  assign s_axis_data_tready = m_axis_data_tready_int[0];
  assign m_axis_data_tvalid = m_axis_data_tvalid_array[0];
  assign m_axis_data_tlast  = m_axis_data_tlast_array[0];

endmodule
