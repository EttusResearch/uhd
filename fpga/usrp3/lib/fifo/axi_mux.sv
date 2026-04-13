//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_mux
//
// Description:
//
//   Takes arbitrary number of AXI streams and merges them to into a single
//   output channel. Bubble cycles are inserted after each packet.
//
// Parameters:
//
//   PRIO           : Controls the arbitration scheme.
//                    0 - Round-robin
//                    1 - Priority (lower number ports get priority)
//   WIDTH          : Width of each AXI-Stream (width of TDATA).
//   PRE_FIFO_SIZE  : Log2 of the input buffer FIFO. Set to 0 for no FIFO.
//   POST_FIFO_SIZE : Log2 of the output buffer FIFO. Set to 0 for no FIFO.
//   SIZE           : Number of input ports to the multiplexer.
//   ALLOC          : Controls the port allocation scheme.
//                    0 - Sequential (scan one port per cycle)
//                    1 - Lookahead (combinational next-port selection).
//                    The lookahead allocation scheme reduces the packet
//                    switching time to at most 2 cycles.
//

`default_nettype none


module axi_mux #(
  parameter int PRIO           = 0,
  parameter int WIDTH          = 64,
  parameter int PRE_FIFO_SIZE  = 0,
  parameter int POST_FIFO_SIZE = 0,
  parameter int SIZE           = 4,
  parameter int ALLOC          = 0
) (
  input  wire                  clk,
  input  wire                  reset,
  input  wire                  clear,

  // Input streams
  input  wire [WIDTH*SIZE-1:0] i_tdata,
  input  wire [      SIZE-1:0] i_tlast,
  input  wire [      SIZE-1:0] i_tvalid,
  output wire [      SIZE-1:0] i_tready,

  // Single output stream
  output wire [     WIDTH-1:0] o_tdata,
  output wire                  o_tlast,
  output wire                  o_tvalid,
  input  wire                  o_tready
);

  wire [WIDTH*SIZE-1:0] i_tdata_int;
  wire [      SIZE-1:0] i_tlast_int;
  wire [      SIZE-1:0] i_tvalid_int;
  wire [      SIZE-1:0] i_tready_int;

  wire [WIDTH-1:0] o_tdata_int;
  wire             o_tlast_int;
  wire             o_tvalid_int;
  wire             o_tready_int;

  reg [$clog2(SIZE)-1:0] st_port;
  reg                    st_active;

  //---------------------------------------------------------------------------
  // Input FIFO
  //---------------------------------------------------------------------------

  genvar n;
  generate
    if (PRE_FIFO_SIZE == 0) begin : gen_no_pre_fifo
      assign i_tdata_int  = i_tdata;
      assign i_tlast_int  = i_tlast;
      assign i_tvalid_int = i_tvalid;
      assign i_tready     = i_tready_int;
    end else begin : gen_pre_fifo
      for (n = 0; n < SIZE; n = n + 1) begin
        axi_fifo #(
          .WIDTH(WIDTH+1      ),
          .SIZE (PRE_FIFO_SIZE)
        ) axi_fifo (
          .clk     (clk                                                ),
          .reset   (reset                                              ),
          .clear   (clear                                              ),
          .i_tdata ({i_tlast[n],i_tdata[WIDTH*(n+1)-1:WIDTH*n]}        ),
          .i_tvalid(i_tvalid[n]                                        ),
          .i_tready(i_tready[n]                                        ),
          .o_tdata ({i_tlast_int[n],i_tdata_int[WIDTH*(n+1)-1:WIDTH*n]}),
          .o_tvalid(i_tvalid_int[n]                                    ),
          .o_tready(i_tready_int[n]                                    ),
          .space   (                                                   ),
          .occupied(                                                   )
        );
      end
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Multiplexer Logic
  //---------------------------------------------------------------------------
  // The multiplexer logic works the same for both allocation schemes.
  // First in the inactive state (st_active == 0) the next port with
  // valid data is selected based on the scheme and the state becomes active.
  // Second the end of the packet is detected based on the tlast signal
  // and the state becomes inactive again.
  // st_port represents the currently selected port.

  if (ALLOC == 0) begin
    // Sequential allocation: scan through ports one at a time until a valid one is found
    always @(posedge clk) begin
      if (reset) begin
        st_port <= 0;
        st_active <= 1'b0;
      end else begin
        // Wait for end of packet.
        if (st_active) begin
          if (o_tlast_int & o_tvalid_int & o_tready_int) begin
            st_active <= 1'b0;
            st_port <= (st_port == SIZE-1) ? 0 : st_port + 1;
          end
        // Set the state to active if the current port has valid data to transfer.
        end else if (i_tvalid_int[st_port]) begin
          st_active <= 1'b1;
        // Sequentially look at the next port.
        end else begin
          st_port <= (st_port == SIZE-1) ? 0 : st_port + 1;
        end
      end
    end
  end else begin

    // Mask for potential ports to select from. See description below for details.
    reg [SIZE-1:0] potential_ports = '1;

    always @(posedge clk) begin
      if (reset) begin
          st_port <= 0;
          st_active <= 1'b0;
      end else begin
        if (st_active) begin
          // Time during ongoing packet transmission is used to determine the
          // set of ports to consider after the packet.
          // Assuming that the PRIO set is disabled only the ports after the
          // current port are considered.
          // Assume we have 4 ports and port 1 is currently active.
          // During the packet transmission port 0 and 3 want to
          // transmit data. Because of the potential port logic only
          // port 2 and 3 are considered for the next packet. Port 3
          // will be the next to transfer data followed by port 0 due to
          // the wrap at the end of the port list.
          // This is guaranteeing a round robin scheme that lower indices or the
          // port itself cannot starve other input ports.
          for (int i = 0; i < SIZE; i = i + 1) begin
            if (i > st_port) begin
              potential_ports[i] <= '1;
            end else begin
              potential_ports[i] <= '0;
            end
          end
          // If there are no more ports to consider or PRIO is enabled all ports
          // starting from index 0 are considered.
          if (st_port == SIZE-1 || PRIO) begin
            potential_ports <= '1;
          end
          // Upon packet completion, the active flag is deasserted and the next
          // port is selected combinationally.
          if (o_tlast_int & o_tvalid_int & o_tready_int) begin
            st_active <= 1'b0;
          end
        end else begin
          // The next port is selected as soon as one of the considered ports
          // has valid data. The function iterates through the potential ports
          // and selects the first one with valid data.
          st_active <= |(i_tvalid_int & potential_ports);
          for (int i = 0; i < SIZE; i = i + 1) begin
            if (i_tvalid_int[i] && potential_ports[i]) begin
              st_port <= i;
              break;
            end
          end

          // In case no port has valid data all ports are considered in
          // the next cycle. This is important for the round-robin
          // scheme if the higher index ports do not have data to
          // transmit. Enabling all ports is similar to searching for
          // the next port from index 0.
          // In case a port has valid data the value of potential_ports is not
          // of interest in the next clock cycle. Therefore the value can be
          // assigned unconditionally.
          potential_ports <= '1;
        end
      end
    end
  end

  genvar i;
  generate
    for (i=0; i<SIZE; i=i+1) begin : gen_tready
      assign i_tready_int[i] = st_active & o_tready_int & (st_port == i);
    end
  endgenerate

  assign o_tvalid_int = st_active & i_tvalid_int[st_port];
  assign o_tlast_int  = i_tlast_int[st_port];

  genvar j;
  generate
    for (j=0; j<WIDTH; j=j+1) begin : gen_tdata
     assign o_tdata_int[j] = i_tdata_int[st_port*WIDTH+j];
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Output FIFO
  //---------------------------------------------------------------------------

  generate
    if (POST_FIFO_SIZE == 0) begin
      assign o_tdata      = o_tdata_int;
      assign o_tlast      = o_tlast_int;
      assign o_tvalid     = o_tvalid_int;
      assign o_tready_int = o_tready;
    end else begin
      axi_fifo #(
        .WIDTH(WIDTH+1       ),
        .SIZE (POST_FIFO_SIZE)
      ) axi_fifo (
        .clk     (clk                      ),
        .reset   (reset                    ),
        .clear   (clear                    ),
        .i_tdata ({o_tlast_int,o_tdata_int}),
        .i_tvalid(o_tvalid_int             ),
        .i_tready(o_tready_int             ),
        .o_tdata ({o_tlast,o_tdata}        ),
        .o_tvalid(o_tvalid                 ),
        .o_tready(o_tready                 ),
        .space   (                         ),
        .occupied(                         )
      );
    end
  endgenerate

endmodule


`default_nettype wire
