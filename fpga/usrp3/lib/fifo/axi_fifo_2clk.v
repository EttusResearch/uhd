/////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Ettus Research, A National Instruments Company
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fifo_2clk.v
//
// Purpose:
// An asynchronous clock crossing for AXI-Stream buses
// The width (WIDTH) and depth (SIZE) of the FIFO is configurable
// For depths less than the technology's SRL threshold, an SRL
// will be instantiated. For depths less the minimum RAM block
// depth (that corresponds to the max width), a single BRAM block
// will be instantiated. For other larger depths, a BRAM block
// plus a regular axi_fifo will be instantiated. The depth of the
// combined FIFO in that case will be larger than the user request.
//
// Requirements:
// Implementation for fifo_short_2clk, fifo_4k_2clk that infer SRL
// and BRAM based clock-crossing FIFOs respectively
//
//////////////////////////////////////////////////////////////////////

module axi_fifo_2clk #(
  parameter WIDTH     = 69,       // Width of input/output data word
  parameter SIZE      = 9,        // log2 of the depth of the FIFO
  parameter PIPELINE  = "NONE",   // Which ports to pipeline? {NONE, IN, OUT, INOUT}
  parameter DEVICE    = "7SERIES" // FPGA technology identifier (for optimal inference)
)(
  input  wire             reset,
  input  wire             i_aclk,
  input  wire [WIDTH-1:0] i_tdata,
  input  wire             i_tvalid,
  output wire             i_tready,
  input  wire             o_aclk,
  output wire [WIDTH-1:0] o_tdata,
  output wire             o_tvalid,
  input  wire             o_tready
);

  wire i_arst, o_arst;
  synchronizer #(.INITIAL_VAL(1'b1)) i_rst_sync_i (
    .clk(i_aclk), .rst(1'b0), .in(reset), .out(i_arst)
  );
  synchronizer #(.INITIAL_VAL(1'b1)) o_rst_sync_i (
    .clk(o_aclk), .rst(1'b0), .in(reset), .out(o_arst)
  );

  //----------------------------------------------
  // Pipeline Logic
  //----------------------------------------------

  wire [WIDTH-1:0] i_pipe_tdata,  o_pipe_tdata;
  wire             i_pipe_tvalid, o_pipe_tvalid;
  wire             i_pipe_tready, o_pipe_tready;

  generate
    if (PIPELINE == "IN" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH(WIDTH)) in_pipe_i (
        .clk(i_aclk), .reset(i_arst), .clear(1'b0),
        .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
        .o_tdata(i_pipe_tdata), .o_tvalid(i_pipe_tvalid), .o_tready(i_pipe_tready),
        .space(), .occupied()
      );
    end else begin
      assign {i_pipe_tdata, i_pipe_tvalid} = {i_tdata, i_tvalid}; 
      assign i_tready = i_pipe_tready;
    end

    if (PIPELINE == "OUT" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH(WIDTH)) out_pipe_i (
        .clk(o_aclk), .reset(o_arst), .clear(1'b0),
        .i_tdata(o_pipe_tdata), .i_tvalid(o_pipe_tvalid), .i_tready(o_pipe_tready),
        .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
        .space(), .occupied()
      );
    end else begin
      assign {o_tdata, o_tvalid} = {o_pipe_tdata, o_pipe_tvalid};
      assign o_pipe_tready = o_tready;
    end
  endgenerate

  //----------------------------------------------
  // FIFO Logic
  //----------------------------------------------

  wire [WIDTH-1:0] o_ext_tdata;
  wire             o_ext_tvalid;
  wire             o_ext_tready;

  // Derive constants based on device.
  // First triple of values is for Intel's MAX10 FPGAs. The FIFO generator for
  // those devices supports embedded memory only (SRL_THRESHOLD = 0).
  // The later triple has been optimized for Xilinx 7Series FPGAs. They also
  // work for Spartan6 but may not be optimal.
  localparam BASE_WIDTH     = (DEVICE == "MAX10") ? 36 : 72;
  localparam SRL_THRESHOLD  = (DEVICE == "MAX10") ?  0 :  5;
  localparam RAM_THRESHOLD  = (DEVICE == "MAX10") ?  8 :  9;

  // How many parallel FIFOs to instantiate to fit WIDTH
  localparam NUM_FIFOS = ((WIDTH-1)/BASE_WIDTH)+1;
  localparam INT_WIDTH = BASE_WIDTH * NUM_FIFOS;

  wire [INT_WIDTH-1:0] wr_data, rd_data;
  wire [NUM_FIFOS-1:0] full, empty;
  wire                 wr_en, rd_en;

  // Read/write logic for FIFO sections
  assign wr_data       = {{(INT_WIDTH-WIDTH){1'b0}}, i_pipe_tdata};
  assign wr_en         = i_pipe_tready & i_pipe_tvalid;
  assign i_pipe_tready = &(~full);
  assign o_ext_tdata   = rd_data[WIDTH-1:0];
  assign o_ext_tvalid  = &(~empty);
  assign rd_en         = o_ext_tready & o_ext_tvalid;

  // FIFO IP instantiation
  genvar i;
  generate
    for (i = 0; i < NUM_FIFOS; i = i + 1) begin: fifo_section
      if (SIZE <= SRL_THRESHOLD) begin
        fifo_short_2clk impl_srl_i (
          .rst          (i_arst),
          .wr_clk       (i_aclk),
          .din          (wr_data[((i+1)*BASE_WIDTH)-1:i*BASE_WIDTH]),
          .wr_en        (wr_en),
          .full         (full[i]),
          .wr_data_count(),
          .rd_clk       (o_aclk),
          .dout         (rd_data[((i+1)*BASE_WIDTH)-1:i*BASE_WIDTH]),
          .rd_en        (rd_en),
          .empty        (empty[i]),
          .rd_data_count()
        );
      end else begin
        fifo_4k_2clk impl_bram_i (
          .rst          (i_arst),
          .wr_clk       (i_aclk),
          .din          (wr_data[((i+1)*BASE_WIDTH)-1:i*BASE_WIDTH]),
          .wr_en        (wr_en),
          .full         (full[i]),
          .wr_data_count(),
          .rd_clk       (o_aclk),
          .dout         (rd_data[((i+1)*BASE_WIDTH)-1:i*BASE_WIDTH]),
          .rd_en        (rd_en),
          .empty        (empty[i]),
          .rd_data_count()
        );
      end
    end
  endgenerate

  //----------------------------------------------
  // Extension FIFO (for large sizes)
  //----------------------------------------------

  generate
    if (SIZE > RAM_THRESHOLD) begin
      wire [WIDTH-1:0] ext_pipe_tdata;
      wire             ext_pipe_tvalid;
      wire             ext_pipe_tready;

      // Add a register slice between BRAM cascades
      axi_fifo_flop2 #(.WIDTH(WIDTH)) ext_fifo_pipe_i (
        .clk(o_aclk), .reset(o_arst), .clear(1'b0),
        .i_tdata(o_ext_tdata), .i_tvalid(o_ext_tvalid), .i_tready(o_ext_tready),
        .o_tdata(ext_pipe_tdata), .o_tvalid(ext_pipe_tvalid), .o_tready(ext_pipe_tready),
        .space(), .occupied()
      );

      // Bolt on an extension FIFO if the requested depth is larger than the BRAM
      // 2clk FIFO primitive (IP)
      axi_fifo_bram #(.WIDTH(WIDTH), .SIZE(SIZE)) ext_fifo_i (
        .clk(o_aclk), .reset(o_arst), .clear(1'b0),
        .i_tdata(ext_pipe_tdata), .i_tvalid(ext_pipe_tvalid), .i_tready(ext_pipe_tready),
        .o_tdata(o_pipe_tdata), .o_tvalid(o_pipe_tvalid), .o_tready(o_pipe_tready),
        .space(), .occupied()
      );
    end else begin
      assign {o_pipe_tdata, o_pipe_tvalid} = {o_ext_tdata, o_ext_tvalid};
      assign o_ext_tready = o_pipe_tready;
    end
  endgenerate

endmodule
