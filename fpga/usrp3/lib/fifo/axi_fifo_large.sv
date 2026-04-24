//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fifo_large
//
// Description:
//
//   Buffers incoming AXI-Stream data using a cascade of FIFOs whose depth is
//   controlled by the DEPTH parameter (number of words). For small depths the
//   module instantiates a single axi_fifo with input and output flop2 stages.
//   On UltraScale devices, when the requested depth exceeds a threshold,
//   URAM-based storage is used for the bulk of the capacity with an BRAM
//   cascade for the remaining depth. On other devices a pure BRAM cascade is
//   generated. A register-based pipeline flop is placed at the input and output
//   to break timing paths.
//
// Parameters:
//
//   WIDTH              : Data bus width in bits.
//   DEPTH              : FIFO depth in number of words. Actual depth is rounded
//                        up to the next multiple of 512 (min BRAM depth).
//   DEVICE             : Target device string ("ULTRASCALE", "7SERIES",
//                        "VIRTEX6", "SPARTAN6").
//   MAX_NUM_URAM_BLOCKS: Maximum number of URAM primitives to use.
//                        Set to -1 (default) for no limit.
//

module axi_fifo_large #(
  int WIDTH               = 64,
  int DEPTH               = 4096,
  string DEVICE           = "7SERIES",
  int MAX_NUM_URAM_BLOCKS = -1
) (
  input  logic             clk,
  input  logic             reset,
  input  logic             clear,

  input  logic [WIDTH-1:0] i_tdata,
  input  logic             i_tvalid,
  output logic             i_tready,

  output logic [WIDTH-1:0] o_tdata,
  output logic             o_tvalid,
  input  logic             o_tready,

  output logic      [19:0] space,
  output logic      [19:0] occupied
);

  // ---------------------------------------------------------------------------
  // Parameter checks
  // ---------------------------------------------------------------------------
  // DEPTH limited by space and occupied output width
  if (DEPTH >= 2**20) begin
    $error("axi_fifo_large: DEPTH (%0d) cannot exceed 20 bits of address space.", DEPTH);
  end

  // ---------------------------------------------------------------------------
  // Size threshold for single FIFO vs cascade.
  // ---------------------------------------------------------------------------
  localparam int SIZE_THRESHOLD = (
    (DEVICE == "ULTRASCALE") ? 12 :  // Set to 12 to allow a single URAM block
    (DEVICE == "7SERIES")    ? 14 : (
    (DEVICE == "VIRTEX6")    ? 14 : (
    (DEVICE == "SPARTAN6")   ? 12 : (
    12
  ))));

  // create a single FIFO instance with input and output flop2 stage
  if (DEPTH <= 2**SIZE_THRESHOLD) begin
    axi_fifo_cascade #(.WIDTH(WIDTH), .SIZE($clog2(DEPTH))) single_fifo (
      .clk(clk), .reset(reset), .clear(clear),
      .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(space[15:0]), .occupied(occupied[15:0])
    );
    assign space[19:16] = '0;
    assign occupied[19:16] = '0;

  // create a FIFO cascade
  end else begin
    // -------------------------------------------------------------------------
    // Register based 2 element FIFO to break timing paths.
    // -------------------------------------------------------------------------
    logic [WIDTH-1:0] i_tdata_pre;
    logic             i_tvalid_pre, i_tready_pre;

    axi_fifo_flop2 #(.WIDTH(WIDTH)) pre_fifo (
      .clk(clk), .reset(reset), .clear(clear),
      .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(i_tdata_pre), .o_tvalid(i_tvalid_pre), .o_tready(i_tready_pre),
      .space(), .occupied()
    );


    // -------------------------------------------------------------------------
    // URAM stage
    // -------------------------------------------------------------------------
    // Static information about URAM primitives
    localparam int URAM_BLOCK_WIDTH = 72; // Number of bits storable in a single URAM block
    localparam int URAM_BLOCK_DEPTH = 4096; // Number of words storable in a single URAM block

    // Number of URAM primitives needed per storage line (72 bits wide each).
    localparam int URAM_PER_LINE = (WIDTH + URAM_BLOCK_WIDTH - 1) / URAM_BLOCK_WIDTH;

    // Determine whether to use URAMs at all. URAMs are only available on
    // ULTRASCALE, and we need at least URAM_PER_LINE blocks to store a
    // single line. When MAX_NUM_URAM_BLOCKS is positive but too small to
    // fit even one line, we skip the URAM path entirely and fall through
    // to the BRAM cascade.
    localparam bit USE_URAM = (DEVICE == "ULTRASCALE") &&
      (MAX_NUM_URAM_BLOCKS < 0 || MAX_NUM_URAM_BLOCKS >= URAM_PER_LINE);

    // Compute how much of the FIFO depth can be covered by URAMs
    // This will always return at least 1 as smaller FIFOs are handled by the single FIFO case above.
    localparam int URAM_COUNT_VAL1 = DEPTH / URAM_BLOCK_DEPTH;
    // Cap the URAM depth by the maximum number of URAM blocks if specified.
    localparam int URAM_COUNT_VAL2 = (MAX_NUM_URAM_BLOCKS < 0) ? URAM_COUNT_VAL1 : (MAX_NUM_URAM_BLOCKS / URAM_PER_LINE);
    // return the minimum of the two URAM depth calculations to determine the actual URAM depth to use.
    localparam int URAM_COUNT = (URAM_COUNT_VAL1 < URAM_COUNT_VAL2) ? URAM_COUNT_VAL1 : URAM_COUNT_VAL2;
    localparam int URAM_DEPTH = URAM_COUNT * URAM_BLOCK_DEPTH;

    // --- URAM stage ---
    logic [WIDTH-1:0] uram_out_tdata;
    logic             uram_out_tvalid, uram_out_tready;

    if (USE_URAM) begin : gen_uram_fifo
      axi_fifo_uram #(.WIDTH(WIDTH), .DEPTH(URAM_DEPTH)) uram_fifo (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata(i_tdata_pre),.i_tvalid(i_tvalid_pre), .i_tready(i_tready_pre),
        .o_tdata(uram_out_tdata), .o_tvalid(uram_out_tvalid), .o_tready(uram_out_tready),
        .space(), .occupied()
      );
    end else begin
      // If not using URAM, connect the URAM interface signals to the pre_fifo output.
      assign uram_out_tdata = i_tdata_pre;
      assign uram_out_tvalid = i_tvalid_pre;
      assign i_tready_pre = uram_out_tready;
    end

    // -------------------------------------------------------------------------
    // BRAM stage
    // -------------------------------------------------------------------------
    // BRAM stages in the URAM path reuse the DEPTH of 13 as with a maximum BRAM
    // size of 1024 words the default max cascade height of 8 is reached at a
    // size of 2**13.
    localparam int BRAM_MAX_SIZE = (DEVICE == "ULTRASCALE") ? 13 : SIZE_THRESHOLD;
    localparam int BRAM_MAX_DEPTH = 2**BRAM_MAX_SIZE;
    // Creating a BRAM instance will at least use 512 words for the supported architectures
    localparam int BRAM_MIN_SIZE = 9;
    localparam int BRAM_MIN_DEPTH = 2**BRAM_MIN_SIZE;

    // calculate remaining depth after URAM stage
    localparam int REMAINING_DEPTH = (USE_URAM) ? (DEPTH - URAM_DEPTH) : DEPTH;

    // calculate the number of BRAM stages needed to cover the remaining depth
    // determine depth rounded up to the next multiple of the minimum BRAM depth
    localparam int REMAINING_DEPTH_CEIL = REMAINING_DEPTH + BRAM_MIN_DEPTH - 1;
    localparam int BRAM_DEPTH = (REMAINING_DEPTH_CEIL / BRAM_MIN_DEPTH) * BRAM_MIN_DEPTH;
    // determine the number of max size BRAM stages
    localparam int NUM_MAX_BRAM = REMAINING_DEPTH_CEIL / BRAM_MAX_DEPTH;

    // generate the BRAM stages for the remaining depth
    localparam int BRAM_COUNT = NUM_MAX_BRAM + BRAM_MAX_SIZE - BRAM_MIN_SIZE;
    logic [WIDTH-1:0] c_tdata  [BRAM_COUNT:0];
    logic             c_tvalid [BRAM_COUNT:0], c_tready [BRAM_COUNT:0];

    // connect the URAM output to the first BRAM stage (or to the output if no BRAM stages)
    assign c_tdata[0] = uram_out_tdata;
    assign c_tvalid[0] = uram_out_tvalid;
    assign uram_out_tready = c_tready[0];

    // generate the max sized BRAM stages
    for (genvar j = 0; j < NUM_MAX_BRAM; j++) begin
      axi_fifo #(.WIDTH(WIDTH), .SIZE(BRAM_MAX_SIZE)) max_size_fifo (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata(c_tdata[j]), .i_tvalid(c_tvalid[j]), .i_tready(c_tready[j]),
        .o_tdata(c_tdata[j+1]), .o_tvalid(c_tvalid[j+1]), .o_tready(c_tready[j+1]),
        .space(), .occupied()
      );
    end

    // generate the smaller BRAM stages if needed (use SIZE of -1 to generate a bypass)
    localparam int NUM_SMALL_BRAM = BRAM_MAX_SIZE - BRAM_MIN_SIZE;
    // use bits in REMAINING_DEPTH_CEIL to determine which FIFOs to use and
    // which to bypass
    localparam logic [NUM_SMALL_BRAM-1:0] BRAM_STAGE_SELECT = REMAINING_DEPTH_CEIL[BRAM_MAX_SIZE-1:BRAM_MIN_SIZE];
    for (genvar k = 0; k < NUM_SMALL_BRAM; k++) begin
      localparam int CURRENT_SIZE = BRAM_MAX_SIZE - k - 1;
      localparam int BRAM_SIZE = (BRAM_STAGE_SELECT[NUM_SMALL_BRAM - k - 1]) ? CURRENT_SIZE : -1;
      localparam int IDX = NUM_MAX_BRAM + k;
      axi_fifo #(.WIDTH(WIDTH), .SIZE(BRAM_SIZE)) smaller_fifo (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata(c_tdata[IDX]), .i_tvalid(c_tvalid[IDX]), .i_tready(c_tready[IDX]),
        .o_tdata(c_tdata[IDX + 1]), .o_tvalid(c_tvalid[IDX + 1]), .o_tready(c_tready[IDX + 1]),
        .space(), .occupied()
      );
    end

    // -------------------------------------------------------------------------
    // Flop 2 output stage
    // -------------------------------------------------------------------------
    axi_fifo_flop2 #(.WIDTH(WIDTH)) post_fifo (
      .clk(clk), .reset(reset), .clear(clear),
      .i_tdata(c_tdata[BRAM_COUNT]), .i_tvalid(c_tvalid[BRAM_COUNT]), .i_tready(c_tready[BRAM_COUNT]),
      .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied()
    );

    // -------------------------------------------------------------------------
    // Space and occupied (diagnostics, not exact)
    // -------------------------------------------------------------------------
    localparam int TOTAL_DEPTH = (USE_URAM ? URAM_DEPTH : 0) + BRAM_DEPTH;
    logic read, write;

    always_comb begin
      write = i_tvalid & i_tready;
      read  = o_tvalid & o_tready;
    end

    always_ff @(posedge clk) begin
      if (reset | clear)
        space <= TOTAL_DEPTH;
      else if (read & ~write)
        space <= space + 1'd1;
      else if (write & ~read)
        space <= space - 1'd1;
    end

    always_ff @(posedge clk) begin
      if (reset | clear)
        occupied <= '0;
      else if (read & ~write)
        occupied <= occupied - 1'd1;
      else if (write & ~read)
        occupied <= occupied + 1'd1;
    end
  end

endmodule
