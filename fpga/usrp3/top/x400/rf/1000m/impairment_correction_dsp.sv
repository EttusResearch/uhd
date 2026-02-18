// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: impairment_correction_dsp
//
// Description:
//
//  Module for applying impairment correction to the Q path while delaying the I
//  path. The I path is expected to be accurate and only needs to be delayed to
//  match the Q path group delay of the FIR filters and the processing latency.
//  The Q path is comprised of two FIR filters, one for I cross impairments and
//  the other for Q inline impairments. The filters are running at full
//  precision. After summation of both filter outputs the Q path is rounded and
//  clipped back to input format.
//
// Parameters:
//
//   NUM_SPC    : Number of samples per clock cycle
//   NUM_COEFFS : Number of coefficients for FIR filters
//

module impairment_correction_dsp #(
  parameter NUM_SPC     = 4,
  parameter NUM_COEFFS  = 15
) (
  input  logic clk,
  input  logic reset,

  // ----------- data path -----------
  // input signal
  input  logic [NUM_SPC*32-1:0] s_axis_tdata, // 32-bit samples (I (LSB) + Q (MSB))
  input  logic                  s_axis_tvalid,
  output logic                  s_axis_tready,

  // signal with IQ impairments applied
  output logic [NUM_SPC*32-1:0] m_axis_tdata, // 32-bit samples (I (LSB) + Q (MSB))
  output logic                  m_axis_tvalid,
  input  logic                  m_axis_tready,

  // ----------- coefficient path -----------
  // All coefficients are of type Q2.23 (ARM notation)
  // see https://en.wikipedia.org/wiki/Q_(number_format)#ARM_version
  // I inline
  input  logic [24:0] s_axi_iinline_coeff_tdata,
  input  logic        s_axi_iinline_coeff_tvalid,
  output logic        s_axi_iinline_coeff_tready,
  // I cross
  input  logic [24:0] s_axi_icross_coeff_tdata,
  input  logic        s_axi_icross_coeff_tvalid,
  output logic        s_axi_icross_coeff_tready,
  // Q inline
  input  logic [24:0] s_axi_qinline_coeff_tdata,
  input  logic        s_axi_qinline_coeff_tvalid,
  output logic        s_axi_qinline_coeff_tready,

  // ----------- configuration -----------
  // group delay of FIR filters for compensation on I path (unit: samples)
  input logic [$clog2(NUM_COEFFS)-1:0] group_delay
);

  // --------------------------------------------------------------------
  // FIR filter parameter
  // --------------------------------------------------------------------
  // Half the data width is I and the other half is Q.
  // Data width is fixed to 32 bits.
  localparam int IN_WIDTH = 16;
  // DSP48E1 has 25x18 multiplier. As I and Q are 16 bits each the coefficient can be up to 25 bits
  // for maximum precision.
  localparam int COEFF_WIDTH = 25;
  // No bits to be clipped to get full output out of FIR filters.
  localparam int CLIP_BITS = 0;
  // The accumulator width is based on the multiplication width and the number of adders.
  localparam int ACCUM_WIDTH = IN_WIDTH + COEFF_WIDTH + $clog2(NUM_COEFFS);
  // Provide the full precision to the output.
  localparam int OUT_WIDTH = ACCUM_WIDTH;
  // Do not care about the FIR filter initialization behavior.
  localparam int BLANK_OUTPUT = 0;
  // Allow host to reload the coefficients.
  localparam int RELOADABLE_COEFFS = 1;
  // Use carry chain of embedded registers in the DSP block to store the coefficients.
  localparam int USE_EMBEDDED_REGS_COEFFS = 1;
  // Does not apply for embedded coefficients.
  localparam logic [COEFF_WIDTH-1:0] COEFFS_VEC [NUM_COEFFS] = '{default: '0};

  // split input data into I and Q (Q is in MSBs and I in the LSBs)
  logic [IN_WIDTH*NUM_SPC-1:0] s_axis_data_i_tdata;
  logic [IN_WIDTH*NUM_SPC-1:0] s_axis_data_q_tdata;
  for (genvar i = 0; i < NUM_SPC; i = i + 1) begin : gen_split_iq
    assign s_axis_data_i_tdata[i*IN_WIDTH +: IN_WIDTH] = s_axis_tdata[i*2*IN_WIDTH +: IN_WIDTH];
    assign s_axis_data_q_tdata[i*IN_WIDTH +: IN_WIDTH] = s_axis_tdata[i*2*IN_WIDTH+IN_WIDTH +: IN_WIDTH];
  end

  // --------------------------------------------------------------------
  // I inline FIR (1 element filter)
  // --------------------------------------------------------------------
  logic [NUM_SPC*IN_WIDTH-1:0] m_axi_iinline_tdata;
  logic m_axi_iinline_tready;
  logic m_axi_iinline_tvalid;

  axi_fir_multisample_filter #(
    .IN_WIDTH                (IN_WIDTH),
    .NUM_SPC                 (NUM_SPC),
    .COEFF_WIDTH             (COEFF_WIDTH),
    .OUT_WIDTH               (IN_WIDTH),
    .NUM_COEFFS              (1),
    .CLIP_BITS               (2), // removing the 2 integer bits of Q2.23 format
    .ACCUM_WIDTH             (IN_WIDTH + COEFF_WIDTH),
    .COEFFS_VEC              ('{default: '0}),
    .RELOADABLE_COEFFS       (RELOADABLE_COEFFS),
    .BLANK_OUTPUT            (BLANK_OUTPUT),
    .USE_EMBEDDED_REGS_COEFFS(USE_EMBEDDED_REGS_COEFFS)
  ) iinline_filter (
    .clk                 (clk),
    .reset               ('0),
    .clear               ('0),
    .s_axis_data_tdata   (s_axis_data_i_tdata),
    .s_axis_data_tlast   ('0),
    // In order to align the input the valid signal is replicated from the Q filters.
    // Otherwise the 2 registers stages in the clip and round module would allow to consume more
    // data than the Q path filters.
    .s_axis_data_tvalid  (s_axis_tvalid & s_axis_tready),
    // tready is ignored here as the Q path has fewer registers and stalls earlier.
    .s_axis_data_tready  (),
    .m_axis_data_tdata   (m_axi_iinline_tdata),
    .m_axis_data_tlast   (),
    .m_axis_data_tvalid  (m_axi_iinline_tvalid),
    .m_axis_data_tready  (m_axi_iinline_tready),
    .s_axis_reload_tdata (s_axi_iinline_coeff_tdata),
    .s_axis_reload_tvalid(s_axi_iinline_coeff_tvalid),
    .s_axis_reload_tlast ('0),
    .s_axis_reload_tready(s_axi_iinline_coeff_tready)
  );

  // --------------------------------------------------------------------
  // I cross FIR filter
  // --------------------------------------------------------------------
  logic [NUM_SPC*OUT_WIDTH-1:0] m_axi_icross_tdata;
  logic m_axi_icross_tready;
  logic m_axi_icross_tvalid;

  axi_fir_multisample_filter #(
    .IN_WIDTH                (IN_WIDTH),
    .NUM_SPC                 (NUM_SPC),
    .COEFF_WIDTH             (COEFF_WIDTH),
    .OUT_WIDTH               (OUT_WIDTH),
    .NUM_COEFFS              (NUM_COEFFS),
    .CLIP_BITS               (CLIP_BITS),
    .ACCUM_WIDTH             (ACCUM_WIDTH),
    .COEFFS_VEC              (COEFFS_VEC),
    .RELOADABLE_COEFFS       (RELOADABLE_COEFFS),
    .BLANK_OUTPUT            (BLANK_OUTPUT),
    .USE_EMBEDDED_REGS_COEFFS(USE_EMBEDDED_REGS_COEFFS)
  ) icross_filter (
    .clk                 (clk),
    .reset               ('0),
    .clear               ('0),
    .s_axis_data_tdata   (s_axis_data_i_tdata),
    .s_axis_data_tlast   ('0),
    .s_axis_data_tvalid  (s_axis_tvalid),
    .s_axis_data_tready  (s_axis_tready),
    .m_axis_data_tdata   (m_axi_icross_tdata),
    .m_axis_data_tlast   (),
    .m_axis_data_tvalid  (m_axi_icross_tvalid),
    .m_axis_data_tready  (m_axi_icross_tready),
    .s_axis_reload_tdata (s_axi_icross_coeff_tdata),
    .s_axis_reload_tvalid(s_axi_icross_coeff_tvalid),
    .s_axis_reload_tlast ('0),
    .s_axis_reload_tready(s_axi_icross_coeff_tready)
  );

  // --------------------------------------------------------------------
  // Q inline FIR filter
  // --------------------------------------------------------------------
  logic [NUM_SPC*OUT_WIDTH-1:0] m_axi_qinline_tdata;
  logic m_axi_qinline_tready;
  logic m_axi_qinline_tvalid;

  axi_fir_multisample_filter #(
    .IN_WIDTH                (IN_WIDTH),
    .NUM_SPC                 (NUM_SPC),
    .COEFF_WIDTH             (COEFF_WIDTH),
    .OUT_WIDTH               (OUT_WIDTH),
    .NUM_COEFFS              (NUM_COEFFS),
    .CLIP_BITS               (CLIP_BITS),
    .ACCUM_WIDTH             (ACCUM_WIDTH),
    .COEFFS_VEC              (COEFFS_VEC),
    .RELOADABLE_COEFFS       (RELOADABLE_COEFFS),
    .BLANK_OUTPUT            (BLANK_OUTPUT),
    .USE_EMBEDDED_REGS_COEFFS(USE_EMBEDDED_REGS_COEFFS)
  ) qinline_filter (
    .clk                 (clk),
    .reset               ('0),
    .clear               ('0),
    .s_axis_data_tdata   (s_axis_data_q_tdata),
    .s_axis_data_tlast   ('0),
    .s_axis_data_tvalid  (s_axis_tvalid),
    .s_axis_data_tready  (),
    .m_axis_data_tdata   (m_axi_qinline_tdata),
    .m_axis_data_tlast   (),
    .m_axis_data_tvalid  (m_axi_qinline_tvalid),
    .m_axis_data_tready  (m_axi_qinline_tready),
    .s_axis_reload_tdata (s_axi_qinline_coeff_tdata),
    .s_axis_reload_tvalid(s_axi_qinline_coeff_tvalid),
    .s_axis_reload_tlast ('0),
    .s_axis_reload_tready(s_axi_qinline_coeff_tready)
  );

  // --------------------------------------------------------------------
  // Combine filter outputs to new Q path
  // --------------------------------------------------------------------
  // translation between the single axi paths and the parallel for loop below
  logic [NUM_SPC-1:0] sum_tready;
  assign m_axi_qinline_tready = sum_tready[0];
  assign m_axi_icross_tready = sum_tready[0];

  logic [NUM_SPC-1:0] q_final_tvalid;
  assign m_axis_tvalid = q_final_tvalid[0];

  // checking if all paths run in parallel
  // synthesis translate_off
  always_comb begin
    assert (m_axi_icross_tvalid === m_axi_qinline_tvalid)    else $error("m_axi_icross_tvalid and m_axi_qinline_tvalid must be the same");
    assert (sum_tready === {NUM_SPC{sum_tready[0]}})         else $error("sum_tready must be the same for all paths");
    assert (q_final_tvalid === {NUM_SPC{q_final_tvalid[0]}}) else $error("q_final_tvalid must be the same for all paths");
  end
  // synthesis translate_on

  for (genvar i=0; i<NUM_SPC; i=i+1) begin : gen_q_path
    // add the filter outputs (1 bit more due to carry)
    logic signed [OUT_WIDTH:0] sum_reg_in;
    assign sum_reg_in = signed'(m_axi_icross_tdata[i*OUT_WIDTH +: OUT_WIDTH]) +
                        signed'(m_axi_qinline_tdata[i*OUT_WIDTH +: OUT_WIDTH]);

    // output of the flop
    logic [OUT_WIDTH:0] sum_reg_tdata;
    logic sum_reg_tvalid;
    logic sum_reg_tready;

    // add register stage after the adder
    axi_fifo_flop2 #(
      .WIDTH(OUT_WIDTH+1)
    ) adder_flop (
      .clk     (clk),
      .reset   (reset),
      .clear   ('0),
      .i_tdata (sum_reg_in),
      .i_tvalid(m_axi_qinline_tvalid),
      .i_tready(sum_tready[i]),
      .o_tdata (sum_reg_tdata),
      .o_tvalid(sum_reg_tvalid),
      .o_tready(sum_reg_tready),
      .space   (),
      .occupied()
    );

    // clip and round the output
    // CLIP_BITS is derived from the growth of the value through the calculation
    // log2 NUM_COEFFS is from FIR filter, +1 by the sum of FIR outputs,
    // +2 due to fixed point representation Q2.23 (integer part) of coefficients
    axi_round_and_clip #(
      .WIDTH_IN (OUT_WIDTH+1),
      .WIDTH_OUT(IN_WIDTH),
      .CLIP_BITS($clog2(NUM_COEFFS)+3),
      .FIFOSIZE (1)
    ) axi_round_and_clipx (
      .clk     (clk),
      .reset   (reset),
      .i_tdata (sum_reg_tdata),
      .i_tlast ('0),
      .i_tvalid(sum_reg_tvalid),
      .i_tready(sum_reg_tready),
      .o_tdata (m_axis_tdata[i*2*IN_WIDTH+IN_WIDTH +: IN_WIDTH]),
      .o_tlast (),
      .o_tvalid(q_final_tvalid[i]),
      .o_tready(m_axis_tready)
    );
  end

  // --------------------------------------------------------------------
  // dynamic delay on the I path
  // --------------------------------------------------------------------
  // The samples from the I path have to be delayed the same way as the Q path.
  // This is done in two steps.
  // First the samples are delayed by a memory. The delay is a multiple of NUM_SPC.
  // Second a barrel shifter is used to delay by the remaining samples.

  // NUM_SPC is required to be a power of two
  if (NUM_SPC != 2**$clog2(NUM_SPC))
    $error("NUM_SPC must be a power of 2");

  // For delay calculation the latency of the FIR paths have to be considered.
  // unit: clock cycles
  localparam PROCESSING_LATENCY = NUM_COEFFS + 5 // Q FIR filter latency
                                - (1 + 7) // I FIR filter latency
                                + 1  // sum after FIR filter cycles
                                + 2  // round and clip submodule latency
                                - 2; // barrel shift register stages
  if (PROCESSING_LATENCY < 0)
    $error("NUM_COEFFS is too small to match the I path delay to the Q path delay");

  // Maximal delay depends on the processing delay and the maximum group delay.
  // unit: clock cycles
  localparam MAX_RAM_DELAY = PROCESSING_LATENCY + $clog2(NUM_COEFFS);
  localparam LOG_MAX_RAM_DELAY = $clog2(MAX_RAM_DELAY);

  // calculate delays for FIFO and index for barrel shifter
  logic [LOG_MAX_RAM_DELAY:0] ram_delay;
  logic [$clog2(NUM_SPC)-1:0] barrel_index;
  always_ff @(posedge clk) begin
    ram_delay    <= PROCESSING_LATENCY + group_delay/NUM_SPC;
    barrel_index <= group_delay % NUM_SPC;
  end

  // shift register implementation
  logic [IN_WIDTH*NUM_SPC-1:0] data_shift_reg [2**LOG_MAX_RAM_DELAY];
  logic [LOG_MAX_RAM_DELAY-1:0] data_shift_reg_wr_ptr = '0, data_shift_reg_rd_ptr = '0;

  logic [IN_WIDTH*NUM_SPC-1:0] i_delayed_tdata;
  logic i_delayed_tready;
  logic i_delayed_tvalid;

  // no reset needed on the pointer as they are always related to each other
  always_ff @(posedge clk) begin
    if (m_axi_iinline_tvalid & m_axi_iinline_tready) begin
      // memory write access
      data_shift_reg[data_shift_reg_wr_ptr] <= m_axi_iinline_tdata;
      // memory is read in the flop instantiation below

      // update pointers
      data_shift_reg_wr_ptr <= data_shift_reg_wr_ptr + 1;
      // have to add 1 as for write pointer and 1 for compensation of i_delayed_tdata register
      data_shift_reg_rd_ptr <= data_shift_reg_wr_ptr - ram_delay + 1 + 1;
    end
  end

  // shift register output register (read memory output)
  axi_fifo_flop2 #(
  . WIDTH(IN_WIDTH*NUM_SPC)
  ) i_delayed_out_reg (
    .clk     (clk),
    .reset   (reset),
    .clear   ('0),
    .i_tdata (data_shift_reg[data_shift_reg_rd_ptr]),
    // This is not a typo below (it is indeed m_axi_qinline_tvalid)!
    // Beyond this point the I and Q path have 3 FIFO registers of DEPTH 1.
    // The different architectures before this point are matched in number of registers.
    // The valid and ready signals behave differently due to the multisample FIR filters which
    // route the ready and valid signal by skipping most of the data path.
    // To have the same number of transfers this valid signal borrowed from the Q path aligns both
    // paths.
    .i_tvalid(m_axi_qinline_tvalid),
    .i_tready(m_axi_iinline_tready),
    .o_tdata (i_delayed_tdata),
    .o_tvalid(i_delayed_tvalid),
    .o_tready(i_delayed_tready),
    .space   (),
    .occupied()
  );

  // combine two words of NUM_SPC samples into one for the barrel shifter
  logic [IN_WIDTH*NUM_SPC*2-1:0] barrel_reg_tdata;
  logic barrel_reg_tvalid;
  logic barrel_reg_tready;

  axi_fifo_flop2 #(
    .WIDTH(IN_WIDTH*NUM_SPC*2)
  ) i_data_barrel_shifter (
    .clk     (clk),
    .reset   (reset),
    .clear   ('0),
    .i_tdata ({i_delayed_tdata, barrel_reg_tdata[IN_WIDTH*NUM_SPC*2-1:IN_WIDTH*NUM_SPC]}),
    .i_tvalid(i_delayed_tvalid),
    .i_tready(i_delayed_tready),
    .o_tdata (barrel_reg_tdata),
    .o_tvalid(barrel_reg_tvalid),
    .o_tready(barrel_reg_tready),
    .space   (),
    .occupied()
  );

  // select the appropriate samples for I path
  logic [IN_WIDTH*NUM_SPC-1:0] i_final_tdata;

  axi_fifo_flop2 #(
    .WIDTH(IN_WIDTH*NUM_SPC)
  ) i_data_barrel_select (
    .clk     (clk),
    .reset   (reset),
    .clear   ('0),
    .i_tdata (barrel_reg_tdata[(NUM_SPC-barrel_index)*IN_WIDTH +: IN_WIDTH*NUM_SPC]),
    .i_tvalid(barrel_reg_tvalid),
    .i_tready(barrel_reg_tready),
    .o_tdata (i_final_tdata),
    .o_tvalid(), // ignored as the Q path is driving the valid signal
    .o_tready(m_axis_tready),
    .space   (),
    .occupied()
  );

  // map delayed I path into the output
  for (genvar i = 0; i < NUM_SPC; i = i + 1) begin : gen_combine_iq
    assign m_axis_tdata[i*2*IN_WIDTH +: IN_WIDTH] = i_final_tdata[i*IN_WIDTH +: IN_WIDTH];
  end

endmodule
