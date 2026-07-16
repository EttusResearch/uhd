//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fifo_xpm_2clk
//
// Description:
//
//   Asynchronous dual-clock FIFO implemented with Xilinx XPM FIFO
//   primitives. The FIFO uses first-word fall-through read mode and
//   provides the interface used by fifo_short_2clk.
//
// Parameters:
//
//   WIDTH: Width of the input and output data buses.
//   DEPTH: Number of entries in the FIFO. Must be a power of two between
//          16 and 4194304.
//

module fifo_xpm_2clk #(
  int WIDTH = 72,
  int DEPTH = 32
)(
  input  logic             rst,
  input  logic             wr_clk,
  input  logic [WIDTH-1:0] din,
  input  logic             wr_en,
  output logic             full,
  output logic [$clog2(DEPTH):0] wr_data_count,

  input  logic             rd_clk,
  output logic [WIDTH-1:0] dout,
  input  logic             rd_en,
  output logic             empty,
  output logic [$clog2(DEPTH):0] rd_data_count
);

  if (WIDTH < 1 || WIDTH > 4096) begin : gen_invalid_width
    $error("WIDTH must be between 1 and 4096 for xpm_fifo_async, but got WIDTH=%0d", WIDTH);
  end

  if (DEPTH < 16 || DEPTH > 4194304 || (DEPTH & (DEPTH - 1)) != 0) begin : gen_invalid_depth
    $error("DEPTH must be a power of two between 16 and 4194304 for xpm_fifo_async, but got DEPTH=%0d", DEPTH);
  end

  localparam int COUNT_WIDTH = $clog2(DEPTH) + 1;

  xpm_fifo_async #(
    .CASCADE_HEIGHT      (0),
    .CDC_SYNC_STAGES     (3),
    .DOUT_RESET_VALUE    ("0"),
    .ECC_MODE            ("no_ecc"),
    .FIFO_MEMORY_TYPE    ("auto"),
    .FIFO_READ_LATENCY   (0),
    .FIFO_WRITE_DEPTH    (DEPTH),
    .FULL_RESET_VALUE    (1),
    .PROG_EMPTY_THRESH   (0),
    .PROG_FULL_THRESH    (0),
    .RD_DATA_COUNT_WIDTH (COUNT_WIDTH),
    .READ_DATA_WIDTH     (WIDTH),
    .READ_MODE           ("fwft"),
    .RELATED_CLOCKS      (0),
    .SIM_ASSERT_CHK      (0),
    .USE_ADV_FEATURES    ("0404"),
    .WAKEUP_TIME         (0),
    .WRITE_DATA_WIDTH    (WIDTH),
    .WR_DATA_COUNT_WIDTH (COUNT_WIDTH)
  ) xpm_fifo_async_i (
    .almost_empty  (),
    .almost_full   (),
    .data_valid    (),
    .dbiterr       (),
    .dout          (dout),
    .empty         (empty),
    .full          (full),
    .overflow      (),
    .prog_empty    (),
    .prog_full     (),
    .rd_data_count (rd_data_count),
    .rd_rst_busy   (),
    .sbiterr       (),
    .underflow     (),
    .wr_ack        (),
    .wr_data_count (wr_data_count),
    .wr_rst_busy   (),
    .din           (din),
    .injectdbiterr (1'b0),
    .injectsbiterr (1'b0),
    .rd_clk        (rd_clk),
    .rd_en         (rd_en),
    .rst           (rst),
    .sleep         (1'b0),
    .wr_clk        (wr_clk),
    .wr_en         (wr_en)
  );

endmodule
