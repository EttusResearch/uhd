//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fifo_uram
//
// Description:
//
//   UltraRAM-based AXI-Stream FIFO for Xilinx UltraScale+ devices.
//   Since UltraRAM requires multiple pipeline stages on the read path,
//   an axi_fifo_short is placed on the output to buffer data and
//   provide standard AXI-Stream backpressure. Reads from the UltraRAM
//   are only issued when the output FIFO has enough space to absorb all
//   in-flight data through the pipeline.
//
//   Simple Dual Port: Port A writes, Port B reads. Follows Xilinx UG901
//   coding style for UltraRAM pipeline inference.
//
// Parameters:
//
//   WIDTH  : Data width in bits. UltraRAM native width is 72 bits; Vivado
//            handles wider or narrower widths automatically.
//   DEPTH  : FIFO depth (number of entries). Native UltraRAM depth is
//            4096 entries. The actual depth is rounded up to next multiple of
//            4096.
//

module axi_fifo_uram #(
  int WIDTH = 72,
  int DEPTH = 4096
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
  output logic      [15:0] space,
  output logic      [15:0] occupied
);

  // Number of pipeline registers absorbable into the URAM cascade.
  // Each URAM block in the cascade contributes one pipeline stage.
  // cascade_depth = 2^max(0, SIZE-12), with a minimum of 1.
  localparam int URAM_BLOCK_DEPTH = 4096;
  localparam int CASCADE_DEPTH = (DEPTH + URAM_BLOCK_DEPTH - 1) / URAM_BLOCK_DEPTH;
  localparam int ACTUAL_DEPTH = CASCADE_DEPTH * URAM_BLOCK_DEPTH;
  localparam int SIZE = $clog2(ACTUAL_DEPTH);

  // Total read latency through UltraRAM pipeline:
  //   1 (memreg) + NBPIPE (mem_pipe_reg stages) + 1 (dout)
  //   = NBPIPE + 2 clock cycles
  localparam int NBPIPE = CASCADE_DEPTH;
  localparam int READ_LATENCY = NBPIPE + 2;

  if (READ_LATENCY >= 32) begin
    $error("axi_fifo_uram: READ_LATENCY (%0d) must be less than 32 to fit in space check.", READ_LATENCY);
  end

  //-------------------------------------------------
  // Write / Read address pointers
  //-------------------------------------------------
  // Extra MSB for full/empty disambiguation
  logic unsigned [SIZE:0]   wr_addr, rd_addr;
  logic unsigned [SIZE-1:0] wr_ptr, rd_ptr;

  always_comb begin
    wr_ptr = wr_addr[SIZE-1:0];
    rd_ptr = rd_addr[SIZE-1:0];
  end

  logic empty, full, write, read;

  always_comb begin
    empty = (wr_addr == rd_addr);
    full  = (wr_addr[SIZE] != rd_addr[SIZE]) && (wr_ptr == rd_ptr);
    write = i_tvalid & i_tready;
    read  = o_tready & o_tvalid;
  end

  assign i_tready = ~full;

  // Issue a URAM read when data is available and the output FIFO has
  // enough free space to absorb all in-flight data in the read pipeline.
  logic [5:0] out_space;
  logic can_read;

  always_comb begin
    can_read = ~empty & (out_space > READ_LATENCY[5:0]);
  end

  always_ff @(posedge clk) begin
    if (reset | clear) begin
      wr_addr <= '0;
    end else if (write) begin
      wr_addr <= wr_addr + 1'b1;
      if (wr_ptr == ACTUAL_DEPTH - 1) begin
        wr_addr <= '0;
        wr_addr[SIZE] <= ~wr_addr[SIZE];
      end
    end
  end

  always_ff @(posedge clk) begin
    if (reset | clear) begin
      rd_addr <= '0;
    end else if (can_read) begin
      rd_addr <= rd_addr + 1'b1;
      if (rd_ptr == ACTUAL_DEPTH - 1) begin
        rd_addr <= '0;
        rd_addr[SIZE] <= ~rd_addr[SIZE];
      end
    end
  end

  //-------------------------------------------------
  // UltraRAM with pipelined read output
  //
  // Coding style matches the Xilinx UG901 SDP UltraRAM template exactly
  // so that Vivado can absorb mem_pipe_reg[] and memreg into the URAM
  // primitive's built-in pipeline registers (OREG). Key requirements:
  //   - Single always block for read+write under a common mem_en
  //   - No reset on the enable pipeline or data pipeline registers
  //   - Pipeline enables derived from mem_en (maps to OREG_CE)
  //-------------------------------------------------

  logic mem_en;
  always_comb begin
    mem_en = write | can_read;
  end

  // the code below this line follows the UG901 template for SDP UltraRAM
  // -------------- start of template code --------------
  (* ram_style = "ultra" *)
  logic [WIDTH-1:0] mem [ACTUAL_DEPTH-1:0];

  logic [WIDTH-1:0] memreg;
  logic [WIDTH-1:0] mem_pipe_reg [NBPIPE-1:0];
  logic             mem_en_pipe_reg [NBPIPE:0];
  logic [WIDTH-1:0] dout;

  // RAM : Simple Dual Port - Port A writes, Port B reads
  // Both under common mem_en to match URAM inference template
  always_ff @(posedge clk) begin
    if (mem_en) begin
      if (write) begin
        mem[wr_ptr] <= i_tdata;
      end
      memreg <= mem[rd_ptr];
    end
  end

  // The enable of the RAM goes through a pipeline to produce a
  // series of pipelined enable signals required to control the data
  // pipeline. No reset - must match template for URAM register absorption.
  always_ff @(posedge clk) begin
    mem_en_pipe_reg[0] <= mem_en;
    for (int i = 0; i < NBPIPE; i = i + 1) begin
      mem_en_pipe_reg[i+1] <= mem_en_pipe_reg[i];
    end
  end

  // RAM output data goes through a pipeline.
  always_ff @(posedge clk) begin
    if (mem_en_pipe_reg[0]) begin
      mem_pipe_reg[0] <= memreg;
    end

    for (int i = 0; i < NBPIPE-1; i = i + 1) begin
      if (mem_en_pipe_reg[i+1]) begin
        mem_pipe_reg[i+1] <= mem_pipe_reg[i];
      end
    end

    if (mem_en_pipe_reg[NBPIPE]) begin
      dout <= mem_pipe_reg[NBPIPE-1];
    end
  end
  // -------------- end of template code --------------

  // Read-validity pipeline: tracks which pipeline slots contain actual
  // new read data vs stale data from write-only mem_en cycles. This is
  // separate from mem_en_pipe_reg to avoid interfering with URAM absorption.
  logic [NBPIPE+1:0] rd_valid_pipe;

  always_ff @(posedge clk) begin
    if (reset | clear)
      rd_valid_pipe <= '0;
    else
      rd_valid_pipe <= {rd_valid_pipe[NBPIPE:0], can_read};
  end

  logic dout_valid;
  always_comb begin
    dout_valid = rd_valid_pipe[NBPIPE+1];
  end

  //-------------------------------------------------
  // Output FIFO
  //
  // An axi_fifo_short (32 entries) buffers the pipelined
  // URAM output and provides AXI-Stream backpressure.
  // Overflow is prevented by the can_read space check.
  //-------------------------------------------------

  axi_fifo_short #(.WIDTH(WIDTH)) out_fifo (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear),
    .i_tdata  (dout),
    .i_tvalid (dout_valid),
    .i_tready (),
    .o_tdata  (o_tdata),
    .o_tvalid (o_tvalid),
    .o_tready (o_tready),
    .space    (out_space),
    .occupied ()
  );

  //-------------------------------------------------
  // Space and occupied (diagnostics, not exact)
  //-------------------------------------------------
  // Based on the read pattern the short FIFO may not be used to full capacity.
  // The overall useable space in all conditions is the URAM depth and the short
  // FIFO depth combined minus the number of elements needed for compensating
  // the read latency pipeline. When the short FIFO is filled to a level where
  // can_read deasserts, no more reads will be issued even though the elements
  // in the short FIFO may be free.
  localparam logic [15:0] NUMLINES = CASCADE_DEPTH * URAM_BLOCK_DEPTH + 32 - READ_LATENCY;

  always_ff @(posedge clk) begin
    if (reset | clear)
      space <= NUMLINES;
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

endmodule // axi_fifo_uram
